//
//  BREvent.c
//  BRCore
//
//  Created by Ed Gamble on 5/7/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include "BREvent.h"
#include "BREventQueue.h"
#include "BREventAlarm.h"

#define PTHREAD_NULL   ((pthread_t) NULL)
#define PTHREAD_STACK_SIZE (512 * 1024)
#define PTHREAD_NAME_SIZE   (33)

/* Forward Declarations */
static void *
eventHandlerThread (BREventHandler handler);

static void
eventHandlerCreateThread (BREventHandler handler);

static pthread_t
eventHandlerGetThread (BREventHandler handler);

static void
eventHandlerJoinThread (BREventHandler handler);

//
// Event Handler
//
struct BREventHandlerRecord {
    char name[PTHREAD_NAME_SIZE];

    // Types
    size_t typesCount;
    const BREventType **types;

    // Queue
    size_t eventSize;
    BREventQueue queue;
    BREvent *scratch;

    // (Optional) Timeout

    ///
    /// The Handler specific timeout event - `filled` with the dispatcher
    ////
    BREventType timeoutEventType;

    ///
    /// The Handler specific timeout context.
    ///
    BREventTimeoutContext timeoutContext;

    ///
    /// The timeout period
    ///
    struct timespec timeout;

    ///
    /// The timeout alarm id, if one exists
    ///
    BREventAlarmId timeoutAlarmId;

    // The thread handling events.
    pthread_t thread;

    // A lock on the `thread` field. This lock is required because the
    // `thread` field is accessed by `eventHandlerIsCurrentThread()`, where
    // `stateLock` cannot be held. The `stateLock` is held while calling
    // `pthread_join` on the running thread in `eventHandlerStop()`; thus if
    // `eventHandlerIsCurrentThread()` were to use `stateLock` and it was
    // called by the event handler thread, we would deadlock. And so,
    // `threadLock` was born. It can be acquired on its own OR while
    // `stateLock` is already held. The reverse CANNOT be done (holding
    // `threadLock` and trying to acquire `stateLock`).
    pthread_mutex_t threadLock;

    // A lock on internal state. It protects the logical consistency of the
    // event handler.
    pthread_mutex_t stateLock;

    // A lock for protecting the dispatch call.  Optional but recommended.
    pthread_mutex_t *lockOnDispatch;
};

extern BREventHandler
eventHandlerCreate (const char *name,
                    const BREventType *types[],
                    unsigned int typesCount,
                    pthread_mutex_t *lockOnDispatch) {
    BREventHandler handler = calloc (1, sizeof (struct BREventHandlerRecord));

    // Fill in the timeout event.  Leave the dispatcher NULL until the dispatcher is provided.
    handler->timeoutEventType.eventName = "Timeout Event";
    handler->timeoutEventType.eventSize = sizeof(BREventTimeout);
    handler->timeoutEventType.eventDispatcher = NULL;

    // Handle the event types.  Ensure we account for the (implicit) timeout event.
    handler->typesCount = typesCount;
    handler->types = types;
    handler->eventSize = handler->timeoutEventType.eventSize;

    strlcpy (handler->name, name, PTHREAD_NAME_SIZE);

    // Update `eventSize` with the largest sized event
    for (int i = 0; i < handler->typesCount; i++) {
        const BREventType *type = handler->types[i];

        if (handler->eventSize < type->eventSize)
            handler->eventSize = type->eventSize;
    }

    handler->timeoutAlarmId = ALARM_ID_NONE;
    handler->lockOnDispatch = lockOnDispatch;

    // Create the PTHREAD LOCK variable
    {
        // The stateLock is a normal, non-recursive lock
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&handler->stateLock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // Create the PTHREAD LOCK variable
    {
        // The threadLock is a normal, non-recursive lock
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&handler->threadLock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    handler->thread = PTHREAD_NULL;

    handler->scratch = (BREvent*) calloc (1, handler->eventSize);
    handler->queue = eventQueueCreate (handler->eventSize);

    return handler;
}

extern void
eventHandlerSetTimeoutDispatcher (BREventHandler handler,
                                  unsigned int timeInMilliseconds,
                                  BREventDispatcher dispatcher,
                                  BREventTimeoutContext context) {
    pthread_mutex_lock (&handler->stateLock);
    handler->timeout.tv_sec = timeInMilliseconds / 1000;
    handler->timeout.tv_nsec = 1000000 * (timeInMilliseconds % 1000);
    handler->timeoutContext = context;
    handler->timeoutEventType.eventDispatcher = dispatcher;
    pthread_mutex_unlock (&handler->stateLock);
}

static void
eventHandlerAlarmCallback (BREventHandler handler,
                           struct timespec expiration,
                           BREventAlarmClock clock) {
    BREventTimeout event =
    { { NULL, &handler->timeoutEventType }, handler->timeoutContext, expiration};
    eventHandlerSignalEventOOB (handler, (BREvent*) &event);
}

typedef void* (*ThreadRoutine) (void*);

static void *
eventHandlerThread (BREventHandler handler) {
#if defined (__ANDROID__)
    pthread_setname_np (pthread_self(), handler->name);
#else
    pthread_setname_np (handler->name);
#endif

    int timeToQuit = 0;

    while (!timeToQuit) {
        // Check for a queued event
        switch (eventQueueDequeueWait (handler->queue, handler->scratch)) {
            case EVENT_STATUS_SUCCESS:
                // We got an event, dispatch
                if (handler->lockOnDispatch) pthread_mutex_lock (handler->lockOnDispatch);
                handler->scratch->type->eventDispatcher (handler, handler->scratch);
                if (handler->lockOnDispatch) pthread_mutex_unlock (handler->lockOnDispatch);
                break;

            case EVENT_STATUS_WAIT_ABORT:
                timeToQuit = 1;
                break;

            case EVENT_STATUS_WAIT_ERROR:
                // Just try again.
                break;

            case EVENT_STATUS_NOT_STARTED:
            case EVENT_STATUS_UNKNOWN_TYPE:
            case EVENT_STATUS_NULL_EVENT:
            case EVENT_STATUS_NONE_PENDING:
                assert (0);
                break;
        }
    }

    return NULL;
}

extern void
eventHandlerDestroy (BREventHandler handler) {
    // First stop...
    eventHandlerStop(handler);

    // ... then kill
    assert (PTHREAD_NULL == handler->thread);
    pthread_mutex_destroy(&handler->threadLock);
    pthread_mutex_destroy(&handler->stateLock);

    // release memory
    eventQueueDestroy(handler->queue);
    free (handler->scratch);
    free (handler);
}

//
// Start / Stop
//

/**
 * Start the handler.  It is possible that events will already be queued; they will all be
 * dispatched, in FIFO order.  If there is a periodic alarm; it will be added to the alarmClock.
 *
 * @param handler
 */
extern void
eventHandlerStart (BREventHandler handler) {
    alarmClockCreateIfNecessary(1);
    pthread_mutex_lock(&handler->stateLock);
    if (PTHREAD_NULL == eventHandlerGetThread (handler)) {
        // If we have an timeout event dispatcher, then add an alarm.
        if (NULL != handler->timeoutEventType.eventDispatcher) {
            handler->timeoutAlarmId = alarmClockAddAlarmPeriodic (alarmClock,
                                                                  (BREventAlarmContext) handler,
                                                                  (BREventAlarmCallback) eventHandlerAlarmCallback,
                                                                  handler->timeout);
        }

        eventHandlerCreateThread (handler);
    }
    pthread_mutex_unlock(&handler->stateLock);
}


/**
 * Stop the handler.  This will clear all pending events.  If there is a periodic alarm, it will
 * be removed from the alarmClock.
 *
 * @note There is a tiny race here, I think.  Before this function returns and after the queue
 * has been cleared, another event can be added.  This is prevented by stopping the threads that
 * submit to this queue before stopping this queue's thread.  Or prior to a subsequent start,
 * clear this handler (But, `eventHandlerStart()` does not clear the thread on start.)
 *
 * @param handler
 */
extern void
eventHandlerStop (BREventHandler handler) {
    pthread_mutex_lock(&handler->stateLock);
    if (PTHREAD_NULL != eventHandlerGetThread (handler)) {
        // Remove a timeout alarm, if it exists.
        if (ALARM_ID_NONE != handler->timeoutAlarmId) {
            alarmClockRemAlarm (alarmClock, handler->timeoutAlarmId);
            handler->timeoutAlarmId = ALARM_ID_NONE;
        }

        // Quit the thread by aborting the queue wait.
        eventQueueDequeueWaitAbort (handler->queue);

        // Wait for the thread.
        eventHandlerJoinThread (handler);

        // TODO: Empty the queue completely?  Or not?
        eventQueueDequeueWaitAbortReset (handler->queue);
        eventHandlerClear (handler);
    }
    pthread_mutex_unlock(&handler->stateLock);
}

extern int
eventHandlerIsCurrentThread (BREventHandler handler) {
    // This function does NOT hold the `stateLock` as it is
    // not required to perform this check (and actually can't
    // as it would cause a deadlock). Its not required because
    // we are checking against `pthread_self()`. There is no
    // chance of a false positive, even if `eventHandlerJoinThread()`
    // was in progress in another thread.
    return pthread_self() == eventHandlerGetThread (handler);
}

extern int
eventHandlerIsRunning (BREventHandler handler) {
    // This function does hold the `stateLock`. If the lock were
    // not held and `eventHandlerJoinThread()` was in progress in
    // another thread, the result here would be inaccurate.
    pthread_mutex_lock(&handler->stateLock);
    int isRunning = PTHREAD_NULL != eventHandlerGetThread (handler);
    pthread_mutex_lock(&handler->stateLock);
    return isRunning;
}

extern BREventStatus
eventHandlerSignalEvent (BREventHandler handler,
                         BREvent *event) {
    eventQueueEnqueueTailSignal (handler->queue, event);
    return EVENT_STATUS_SUCCESS;
}

extern BREventStatus
eventHandlerSignalEventOOB (BREventHandler handler,
                            BREvent *event) {
    eventQueueEnqueueHeadSignal (handler->queue, event);
    return EVENT_STATUS_SUCCESS;
}

extern void
eventHandlerClear (BREventHandler handler) {
    eventQueueClear(handler->queue);
}

static void
eventHandlerCreateThread (BREventHandler handler) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize(&attr, PTHREAD_STACK_SIZE);

    pthread_mutex_lock(&handler->threadLock);
    pthread_create(&handler->thread, &attr, (ThreadRoutine) eventHandlerThread, handler);
    pthread_mutex_unlock(&handler->threadLock);

    pthread_attr_destroy(&attr);
}

static pthread_t
eventHandlerGetThread (BREventHandler handler) {
    pthread_mutex_lock(&handler->threadLock);
    pthread_t handlerThread = handler->thread;
    pthread_mutex_unlock(&handler->threadLock);
    return handlerThread;
}

static void
eventHandlerJoinThread (BREventHandler handler) {
    pthread_mutex_lock(&handler->threadLock);
    pthread_t handlerThread = handler->thread;
    pthread_mutex_unlock(&handler->threadLock);

    pthread_join (handlerThread, NULL);

    pthread_mutex_lock(&handler->threadLock);
    handler->thread = PTHREAD_NULL;
    pthread_mutex_unlock(&handler->threadLock);
}