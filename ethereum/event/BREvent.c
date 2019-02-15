//
//  BREvent.c
//  BRCore
//
//  Created by Ed Gamble on 5/7/18.
//  Copyright (c) 2018 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

#define PTHREAD_NULL   ((pthread_t) NULL)

#include "BREvent.h"
#include "BREventQueue.h"
#include "BREventAlarm.h"

#define PTHREAD_STACK_SIZE (512 * 1024)
#define PTHREAD_NAME_SIZE   (33)

/* Forward Declarations */
static void *
eventHandlerThread (BREventHandler handler);

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

    // Pthread

    pthread_t thread;
    pthread_cond_t cond;
    pthread_mutex_t lock;
    pthread_mutex_t lockOnStartStop;

    int threadQuit;
};

extern BREventHandler
eventHandlerCreate (const char *name,
                    const BREventType *types[],
                    unsigned int typesCount) {
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

    // Create the PTHREAD CONDition variable
    {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_cond_init(&handler->cond, &attr);
        pthread_condattr_destroy(&attr);
    }

    // Create the PTHREAD LOCK variable
    {
        // The cacheLock is a normal, non-recursive lock
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&handler->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // Create the PTHREAD LOCK-ON-START-STOP variable
    {
        // The cacheLock is a normal, non-recursive lock
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&handler->lockOnStartStop, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    handler->thread = PTHREAD_NULL;

    handler->scratch = (BREvent*) calloc (1, handler->eventSize);
    handler->queue = eventQueueCreate(handler->eventSize, &handler->lock);

    return handler;
}

extern void
eventHandlerSetTimeoutDispatcher (BREventHandler handler,
                                  unsigned int timeInMilliseconds,
                                  BREventDispatcher dispatcher,
                                  BREventTimeoutContext context) {
    pthread_mutex_lock(&handler->lock);
    handler->timeout.tv_sec = timeInMilliseconds / 1000;
    handler->timeout.tv_nsec = 1000000 * (timeInMilliseconds % 1000);
    handler->timeoutContext = context;
    handler->timeoutEventType.eventDispatcher = dispatcher;
    pthread_mutex_unlock(&handler->lock);

    // Signal an event - so that the 'timedwait' starts.
    pthread_cond_signal(&handler->cond);
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
    pthread_setname_np(handler->thread, handler->name);
#else
    pthread_setname_np(handler->name);
#endif

    pthread_mutex_lock(&handler->lock);

    // If we have an timeout event dispatcher, then add an alarm.
    if (NULL != handler->timeoutEventType.eventDispatcher) {
        alarmClockAddAlarmPeriodic(alarmClock,
                                      (BREventAlarmContext) handler,
                                      (BREventAlarmCallback) eventHandlerAlarmCallback,
                                      handler->timeout);
    }

    handler->threadQuit = 0;

    while (!handler->threadQuit) {
        // Check for a queued event
        switch (eventQueueDequeue(handler->queue, handler->scratch)) {
            case EVENT_STATUS_SUCCESS: {
                // If we have one, dispatch
                BREventType *type = handler->scratch->type;
                type->eventDispatcher (handler, handler->scratch);
                break;
            }

            case EVENT_STATUS_NOT_STARTED:
            case EVENT_STATUS_UNKNOWN_TYPE:
            case EVENT_STATUS_NULL_EVENT:
                // impossible?
                // fall through

            case EVENT_STATUS_NONE_PENDING:
                // ... otherwise wait for an event ...
                pthread_cond_wait(&handler->cond, &handler->lock);
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
    pthread_cond_destroy(&handler->cond);
    pthread_mutex_destroy(&handler->lock);
    pthread_mutex_destroy(&handler->lockOnStartStop);

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
 * dispatched, in FIFO order.
 *
 * @param handler
 */
extern void
eventHandlerStart (BREventHandler handler) {
    alarmClockCreateIfNecessary(1);
    pthread_mutex_lock(&handler->lockOnStartStop);
    if (PTHREAD_NULL == handler->thread) {
        // if (0 != pthread_attr_t (...) && 0 != pthread_attr_...() && ...
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setstacksize(&attr, PTHREAD_STACK_SIZE);

        pthread_create(&handler->thread, &attr, (ThreadRoutine) eventHandlerThread, handler);
        pthread_attr_destroy(&attr);
    }
    pthread_mutex_unlock(&handler->lockOnStartStop);
}


/**
 * Stop the handler.  This will clear all pending events.
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
    pthread_mutex_lock(&handler->lockOnStartStop);
    if (PTHREAD_NULL != handler->thread) {
        pthread_mutex_lock(&handler->lock);  // ensure this for restart.
        handler->threadQuit = 1;
        pthread_cond_signal(&handler->cond);
        pthread_mutex_unlock(&handler->lock);  // ensure this for restart.
        pthread_join(handler->thread, NULL);
        // A mini-race here?
        handler->thread = PTHREAD_NULL;

        // Empty the queue completely.
        eventQueueClear(handler->queue);
    }
    pthread_mutex_unlock(&handler->lockOnStartStop);
}

extern int
eventHandlerIsRunning (BREventHandler handler) {
    return PTHREAD_NULL != handler->thread;
}

extern BREventStatus
eventHandlerSignalEvent (BREventHandler handler,
                         BREvent *event) {
    eventQueueEnqueueTail(handler->queue, event);
    pthread_cond_signal(&handler->cond);
    return EVENT_STATUS_SUCCESS;
}

extern BREventStatus
eventHandlerSignalEventOOB (BREventHandler handler,
                            BREvent *event) {
    eventQueueEnqueueHead(handler->queue, event);
    pthread_cond_signal(&handler->cond);
    return EVENT_STATUS_SUCCESS;
}

extern void
eventHandlerClear (BREventHandler handler) {
    eventQueueClear(handler->queue);
}
