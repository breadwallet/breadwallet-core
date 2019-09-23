//
//  BREventQueue.c
//  BRCore
//
//  Created by Ed Gamble on 5/7/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include <string.h>
#include <pthread.h>
#include "BREventQueue.h"

#define EVENT_QUEUE_DEFAULT_INITIAL_CAPACITY   (1)

struct BREventQueueRecord {
    // A linked-list (through event->next) of pending events.
    BREvent *pending;

    // A linked-list (through event->next) of available events
    BREvent *available;

    // If not provided with a lock, use this one.
    pthread_mutex_t lock;

    // A 'cond var'
    pthread_cond_t cond;

    // An 'abort wait' flag
    int abort;

    // The size of each event
    size_t size;
};

extern BREventQueue
eventQueueCreate (size_t size) {
    BREventQueue queue = calloc (1, sizeof (struct BREventQueueRecord));

    queue->pending = NULL;
    queue->available = NULL;
    queue->abort = 0;
    queue->size  = size;

    for (int i = 0; i < EVENT_QUEUE_DEFAULT_INITIAL_CAPACITY; i++) {
        BREvent *event = calloc (1, queue->size);
        event->next = queue->available;
        queue->available = event;
    }

    // Create the PTHREAD CONDition variable
    {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_cond_init(&queue->cond, &attr);
        pthread_condattr_destroy(&attr);
    }

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&queue->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    return queue;
}

static void
eventFreeAll (BREvent *event,
              int destroy) {
    while (NULL != event) {
        // Save the next event so that the upcoming `free` doesn't zero it out.
        BREvent *next = event->next;

        // Apply the `destroyer` if appropriate.
        if (destroy) {
            BREventDestroyer destroyer = event->type->eventDestroyer;
            if (NULL != destroyer) destroyer (event);
        }
        // Actual free and then iterate.
        free (event);
        event = next;
    }
}

extern void
eventQueueClear (BREventQueue queue) {
    pthread_mutex_lock(&queue->lock);

    eventFreeAll(queue->pending, 1);
    eventFreeAll(queue->available, 0);

    queue->pending = NULL;
    queue->available = NULL;

    pthread_mutex_unlock(&queue->lock);
}

extern void
eventQueueDestroy (BREventQueue queue) {
    // Clear the pending and available queues.
    eventQueueClear (queue);

    pthread_cond_destroy(&queue->cond);
    pthread_mutex_destroy(&queue->lock);

    memset (queue, 0, sizeof (struct BREventQueueRecord));
    free (queue);
}

static void
eventQueueEnqueue (BREventQueue queue,
                   const BREvent *event,
                   int tail,
                   int signal) {
    pthread_mutex_lock(&queue->lock);

    // Get the next available event
    BREvent *this = queue->available;
    if (NULL == this) {
        this = (BREvent*) calloc (1, queue->size);
        this->next = NULL;
    }
    // Make the next event no longer available.
    queue->available = this->next;

    // Fill in `this` with event
    memcpy (this, event, event->type->eventSize);
    this->next = NULL;

    // Nothing pending, simply add.
    if (NULL == queue->pending)
        queue->pending = this;
    else if (tail) {
        // Find the last event
        BREvent *last = queue->pending;
        while (NULL != last->next) last = last->next;
        last->next = this;
    }
    else /* (head) */ {
        this->next = queue->pending;
        queue->pending = this;
    }

    if (signal) pthread_cond_signal (&queue->cond);
    pthread_mutex_unlock(&queue->lock);
}

extern void
eventQueueEnqueueTail (BREventQueue queue,
                       const BREvent *event) {
    eventQueueEnqueue (queue, event, 1, 0);
}

extern void
eventQueueEnqueueHead (BREventQueue queue,
                       const BREvent *event) {
    eventQueueEnqueue (queue, event, 0, 0);
}
extern void
eventQueueEnqueueTailSignal (BREventQueue queue,
                             const BREvent *event) {
    eventQueueEnqueue (queue, event, 1, 1);
}

extern void
eventQueueEnqueueHeadSignal (BREventQueue queue,
                             const BREvent *event) {
    eventQueueEnqueue (queue, event, 0, 1);
}

static int
_eventQueueDequeue (BREventQueue queue,
                    BREvent *event) {
    // Get the next pending event
    BREvent *this = queue->pending;

    // if there is one, process it
    if (NULL == this) return 0;

    // Remove `this` from the pending list.
    queue->pending = this->next;

    // Fill in the provided event;
    this->next = NULL;
    memcpy (event, this, queue->size);

    // Return `this` to the available list.
    this->next = queue->available;
    queue->available = this;

    return 1;
}

extern BREventStatus
eventQueueDequeue (BREventQueue queue,
                   BREvent *event) {
   if (NULL == event)
        return EVENT_STATUS_NULL_EVENT;

    pthread_mutex_lock (&queue->lock);
    BREventStatus status = (_eventQueueDequeue (queue, event)
                            ? EVENT_STATUS_SUCCESS
                            : EVENT_STATUS_NONE_PENDING);
    pthread_mutex_unlock(&queue->lock);

    return status;
}

extern BREventStatus
eventQueueDequeueWait (BREventQueue queue,
                       BREvent *event) {
    if (NULL == event)
        return EVENT_STATUS_NULL_EVENT;

    BREventStatus status = EVENT_STATUS_SUCCESS;

    pthread_mutex_lock (&queue->lock);
    while (!queue->abort && !_eventQueueDequeue (queue, event))
        if (0 != pthread_cond_wait (&queue->cond, &queue->lock)) {
            status = EVENT_STATUS_WAIT_ERROR;
            break; /* from while */
        }
    if (queue->abort) status = EVENT_STATUS_WAIT_ABORT;
    pthread_mutex_unlock(&queue->lock);

    return status;
}

extern void
eventQueueDequeueWaitAbort (BREventQueue queue) {
    pthread_mutex_lock (&queue->lock);
    queue->abort = 1;
    pthread_cond_signal (&queue->cond);
    pthread_mutex_unlock (&queue->lock);
}

extern void
eventQueueDequeueWaitAbortReset (BREventQueue queue) {
    pthread_mutex_lock (&queue->lock);
    queue->abort = 0;
    pthread_cond_signal (&queue->cond);
    pthread_mutex_unlock (&queue->lock);
}

extern int
eventQueueHasPending (BREventQueue queue) {
    int pending = 0;
    pthread_mutex_lock(&queue->lock);
    pending = NULL != queue->pending;
    pthread_mutex_unlock(&queue->lock);
    return pending;
}
