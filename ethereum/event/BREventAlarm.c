//
//  BREventAlarm.c
//  BRCore
//
//  Created by Ed Gamble on 5/7/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include <string.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include "support/BRAssert.h"
#include "support/BRArray.h"
#include "BREvent.h"
#include "BREventAlarm.h"

#define PTHREAD_NULL   ((pthread_t) NULL)
#define PTHREAD_STACK_SIZE   (32 * 1024)

/* Explicitly import (from BREvent.c) */
extern void
eventHandlerInvokeTimeout (BREventHandler handler);

/* `struct timespec` support */

static struct timespec
getTime () {
    struct timeval  now;
    struct timespec time;

    gettimeofday(&now, NULL);

    time.tv_sec = now.tv_sec;
    time.tv_nsec = 1000 * now.tv_usec;

    return time;
}

/* Unused
static inline struct timespec
timespecAdd (struct timespec *t1, struct timespec *t2) {
    long tv_nsec = t1->tv_nsec + t2->tv_nsec;
    long tv_sec  = t1->tv_sec  + t2->tv_sec;
    if (tv_nsec >= 1000000000) {
        tv_nsec -= 1000000000;
        tv_sec  += 1;
    }
    return (struct timespec) { .tv_sec = tv_sec, .tv_nsec = tv_nsec };
}
*/

static inline void
timespecInc (struct timespec *t1, struct timespec *t2) {
    t1->tv_nsec += t2->tv_nsec;
    t1->tv_sec  += t2->tv_sec;
    if (t1->tv_nsec >= 1000000000) {
        t1->tv_nsec -= 1000000000;
        t1->tv_sec  += 1;
    }
}

static inline int
timespecCompare (struct timespec *t1, struct timespec *t2) {
    return (t1->tv_sec > t2->tv_sec
            ? +1
            : (t1->tv_sec < t2->tv_sec
               ? -1
               : (t1->tv_nsec > t2->tv_nsec
                  ? +1
                  : (t1->tv_nsec < t2->tv_nsec
                     ? -1
                     : 0))));
}

/**
 */
BREventAlarmClock alarmClock = NULL;

typedef enum {
    ALARM_ONE_SHOT,
    ALARM_PERIODIC
} BREventAlarmType;

/**
 */
typedef struct {
    BREventAlarmId identifier;
    BREventAlarmType type;
    BREventAlarmContext context;
    BREventAlarmCallback callback;

    /// The next expiration time.  This is updated if the alarm is periodic.
    struct timespec expiration;

    /// The alarm's period.  For a ONE_SHOT alarm, this is ignored/zeroed.
    struct timespec period;
} BREventAlarm;

static BREventAlarm
alarmCreatePeriodic (BREventAlarmContext context,
                     BREventAlarmCallback callback,
                     struct timespec expiration,  // first expiration...
                     struct timespec period,      // ...thereafter increment
                     BREventAlarmId identifier) {
    return (BREventAlarm) {
        .type = ALARM_PERIODIC,
        .identifier = identifier,
        .context = context,
        .callback = callback,
        .expiration = expiration,
        .period = period };
}

static BREventAlarm
alarmCreate (BREventAlarmContext context,
             BREventAlarmCallback callback,
             struct timespec expiration,
             BREventAlarmId identifier) {
    return (BREventAlarm) {
        .type = ALARM_ONE_SHOT,
        .identifier = identifier,
        .context = context,
        .callback = callback,
        .expiration = expiration,
        .period = { .tv_sec = 0, .tv_nsec = 0 } };
}

static int
alarmIsPeriodic (BREventAlarm *alarm) {
    return ALARM_PERIODIC == alarm->type;
}

static void
alarmPeriodUpdate (BREventAlarm *alarm) {
    timespecInc(&alarm->expiration, &alarm->period);

    // ensure that expiration does not occur in the past
    struct timespec now = getTime();
    if (-1 == timespecCompare(&alarm->expiration, &now)) {
        alarm->expiration = now;
    }
}

static void
alarmExpire (BREventAlarm *alarm, BREventAlarmClock clock) {
    if (NULL != alarm->callback)
        alarm->callback (alarm->context, alarm->expiration, clock);
}

/**
 */
static void
alarmClockAssertRecovery (BREventAlarmClock clock);

struct BREventAlarmClock {
    /// Identifier of the next alarm created.
    BREventAlarmId identifier;

    /// An BRArrayOf alarms, sorted by alarm.expiration ascending.
    BREventAlarm *alarms;

    /// The time of the next timeout
    struct timespec timeout;

    // Thread
    pthread_t thread;
    pthread_cond_t cond;
    pthread_mutex_t lock;
    pthread_mutex_t lockOnStartStop;

    int threadQuit;
};

extern void
alarmClockCreateIfNecessary (int start) {
    if (NULL == alarmClock)
        alarmClock = alarmClockCreate();
    if (start)
        alarmClockStart(alarmClock);
}

extern BREventAlarmClock
alarmClockCreate (void) {
    BREventAlarmClock clock = calloc (1, sizeof (struct BREventAlarmClock));

    clock->identifier = ALARM_ID_NONE;
    array_new(clock->alarms, 5);

    // Create the PTHREAD CONDition variable
    {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_cond_init(&clock->cond, &attr);
        pthread_condattr_destroy(&attr);
    }

    // Create the PTHREAD LOCK variable
    {
        // The cacheLock is a normal, non-recursive lock
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&clock->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // Create the PTHREAD LOCK-ON-START-STOP variable
    {
        // The cacheLock is a normal, non-recursive lock
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&clock->lockOnStartStop, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // No thread.
    clock->thread = PTHREAD_NULL;

    BRAssertDefineRecovery ((BRAssertRecoveryInfo) clock,
                            (BRAssertRecoveryHandler) alarmClockAssertRecovery);

    return clock;
}

extern void
alarmClockDestroy (BREventAlarmClock clock) {
    alarmClockStop(clock);

    BRAssertRemoveRecovery((BRAssertRecoveryInfo) clock);

    assert (PTHREAD_NULL == clock->thread);
    pthread_cond_destroy(&clock->cond);
    pthread_mutex_destroy(&clock->lock);
    pthread_mutex_destroy(&clock->lockOnStartStop);

    array_free (clock->alarms);
    if (clock == alarmClock)
        free (alarmClock);
        alarmClock = NULL;
}

static void
alarmClockInsertAlarm (BREventAlarmClock clock,
                       BREventAlarm alarm) {
    int count = (int) array_count (clock->alarms);
    int index = count;

    for (; index > 0; index--) {
        // If alarm.expiration is not less than 'alarm at index-1', then insert alarm at index
        if (-1 != timespecCompare(&alarm.expiration, &clock->alarms[index - 1].expiration))
            break;
    }
    array_insert (clock->alarms, index, alarm);
}

typedef void* (*ThreadRoutine) (void*);

static void *
alarmClockThread (BREventAlarmClock clock) {

#if defined (__ANDROID__)
    pthread_setname_np(clock->thread, "Core Ethereum Alarm Clock");
#else
    pthread_setname_np("Core Ethereum Alarm Clock");
#endif

    pthread_mutex_lock(&clock->lock);

    clock->threadQuit = 0;

    while (!clock->threadQuit) {
        // Set the next timeout - based on an existing alarm or 'forever in the future'
        clock->timeout = (array_count(clock->alarms) > 0
                          ? clock->alarms[0].expiration
                          : (struct timespec) { .tv_sec = LONG_MAX, .tv_nsec = 0 });

        switch (pthread_cond_timedwait (&clock->cond, &clock->lock, &clock->timeout)) {
            case ETIMEDOUT: {
                // Check if alarm was removed while we slept...
                if (0 == array_count(clock->alarms) ||
                    0 != timespecCompare(&clock->alarms[0].expiration, &clock->timeout)) {
                    // ... ignore the timeout, its alarm is for the birds now
                    break;
                }

                // If we timed-out, then get the alarm that has expired...
                BREventAlarm alarm = clock->alarms[0];
                // ... and remove it from the clock's alarms (for now; if periodic, add it back)
                array_rm (clock->alarms, 0);

                // Expire the alarm - invokes the callback.
                alarmExpire(&alarm, clock);

                // If periodic, update the alarm expiration and reinsert
                if (alarmIsPeriodic(&alarm)) {
                    alarmPeriodUpdate(&alarm);
                    alarmClockInsertAlarm(clock, alarm);
                }

                break;
            }

            default:
                // Update clock->timeout (above, presumably an alarm was added) and wait again.
                // Or clock->threadQuit is set.
                break;
        }
    }

    // Requires as `cond_wait` takes its mutex when signalled.
    pthread_mutex_unlock(&clock->lock);

    return NULL;
}

extern void
alarmClockStart (BREventAlarmClock clock) {
    pthread_mutex_lock(&clock->lockOnStartStop);
    if (PTHREAD_NULL == clock->thread) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setstacksize(&attr, PTHREAD_STACK_SIZE);

        pthread_create(&clock->thread, &attr, (ThreadRoutine) alarmClockThread, clock);
        pthread_attr_destroy(&attr);
    }
    pthread_mutex_unlock(&clock->lockOnStartStop);

}

extern void
alarmClockStop (BREventAlarmClock clock) {
    pthread_mutex_lock(&clock->lockOnStartStop);
    if (PTHREAD_NULL != clock->thread) {
        pthread_mutex_lock(&clock->lock);  // ensure this for restart.
        clock->threadQuit = 1;
        pthread_cond_signal(&clock->cond);
        pthread_mutex_unlock(&clock->lock);  // ensure this for restart.
        pthread_join(clock->thread, NULL);
        // A mini-race here?
        clock->thread = PTHREAD_NULL;
    }
    pthread_mutex_unlock(&clock->lockOnStartStop);
}

extern int
alarmClockIsRunning (BREventAlarmClock clock) {
    return PTHREAD_NULL != clock->thread;
}

static void
alarmClockAssertRecovery (BREventAlarmClock clock) {
    alarmClockStop(clock);
    pthread_mutex_lock(&clock->lockOnStartStop);
    array_clear(clock->alarms);
    pthread_mutex_unlock(&clock->lockOnStartStop);
}

extern BREventAlarmId
alarmClockAddAlarmPeriodic (BREventAlarmClock clock,
                            BREventAlarmContext context,
                            BREventAlarmCallback callback,
                            struct timespec period) {
    pthread_mutex_lock(&clock->lock);
    BREventAlarmId identifier = ++clock->identifier;
    alarmClockInsertAlarm(clock, alarmCreatePeriodic(context, callback, getTime(), period, identifier));
    // Having modified `alarms` we need to compute a new 'next expiration'
    pthread_cond_signal(&clock->cond);
    pthread_mutex_unlock(&clock->lock);
    return identifier;
}

extern BREventAlarmId
alarmClockAddAlarm (BREventAlarmClock clock,
                    BREventAlarmContext context,
                    BREventAlarmCallback callback,
                    struct timespec expiration) {
    pthread_mutex_lock(&clock->lock);
    BREventAlarmId identifier = ++clock->identifier;
    alarmClockInsertAlarm(clock, alarmCreate(context, callback, expiration, identifier));
    // Having modified `alarms` we need to compute a new 'next expiration'
    pthread_cond_signal(&clock->cond);
    pthread_mutex_unlock(&clock->lock);
    return identifier;
}

extern void
alarmClockRemAlarm (BREventAlarmClock clock,
                    BREventAlarmId identifier) {
    pthread_mutex_lock(&clock->lock);
    for (int index = 0; index < array_count (clock->alarms); index++)
        if (identifier == clock->alarms[index].identifier) {
            array_rm(clock->alarms, index);
            break;
        }
    // Having modified `alarms` we need to compute a new 'next expiration'
    pthread_cond_signal(&clock->cond);
    pthread_mutex_unlock(&clock->lock);
}

extern int
alarmClockHasAlarm (BREventAlarmClock clock,
                    BREventAlarmId identifier) {
    int hasAlarm = 0;

    pthread_mutex_lock(&clock->lock);
    for (int index = 0; index < array_count (clock->alarms); index++)
        if (identifier == clock->alarms[index].identifier) {
            hasAlarm = 1;
            break;
        }
    pthread_mutex_unlock(&clock->lock);

    return hasAlarm;
}
