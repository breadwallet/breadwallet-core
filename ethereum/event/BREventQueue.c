//
//  BREventQueue.c
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

    // A lock for exclusive access when queueing/dequeueing events.
    pthread_mutex_t *lockToUse;

    // If not provided with a lock, use this one.
    pthread_mutex_t lock;

    // The size of each event
    size_t size;
};

extern BREventQueue
eventQueueCreate (size_t size, pthread_mutex_t *lock) {
    BREventQueue queue = calloc (1, sizeof (struct BREventQueueRecord));

    queue->pending = NULL;
    queue->available = NULL;
    queue->size = size;

    for (int i = 0; i < EVENT_QUEUE_DEFAULT_INITIAL_CAPACITY; i++) {
        BREvent *event = calloc (1, queue->size);
        event->next = queue->available;
        queue->available = event;
    }

    // If a lock was not provided, create the PTHREAD LOCK, as a normal, non-recursive, lock.
    if (NULL != lock) queue->lockToUse = lock;
    else {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&queue->lock, &attr);
        pthread_mutexattr_destroy(&attr);

        queue->lockToUse = &queue->lock;
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
    pthread_mutex_lock(queue->lockToUse);

    eventFreeAll(queue->pending, 1);
    eventFreeAll(queue->available, 0);

    queue->pending = NULL;
    queue->available = NULL;

    pthread_mutex_unlock(queue->lockToUse);
}

extern void
eventQueueDestroy (BREventQueue queue) {
    // Clear the pending and available queues.
    eventQueueClear (queue);

    if (queue->lockToUse == &queue->lock)
        pthread_mutex_destroy(&queue->lock);

    memset (queue, 0, sizeof (struct BREventQueueRecord));
    free (queue);
}

static void
eventQueueEnqueue (BREventQueue queue,
                   const BREvent *event,
                   int tail) {
    pthread_mutex_lock(queue->lockToUse);

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

    pthread_mutex_unlock(queue->lockToUse);
}

extern void
eventQueueEnqueueTail (BREventQueue queue,
                       const BREvent *event) {
    eventQueueEnqueue (queue, event, 1);
}

extern void
eventQueueEnqueueHead (BREventQueue queue,
                       const BREvent *event) {
    eventQueueEnqueue (queue, event, 0);
}

extern BREventStatus
eventQueueDequeue (BREventQueue queue,
                   BREvent *event) {
    BREventStatus status = EVENT_STATUS_SUCCESS;
    
    if (NULL == event)
        return EVENT_STATUS_NULL_EVENT;
    
    pthread_mutex_lock(queue->lockToUse);
    
    // Get the next pending event
    BREvent *this = queue->pending;
    
    // there is none, just return
    if (NULL == this)
        status = EVENT_STATUS_NONE_PENDING;
    
    // otherwise process it.
    else {
        // Remove `this` from the pending list.
        queue->pending = this->next;
        
        // Fill in the provided event;
        this->next = NULL;
        memcpy (event, this, queue->size);
        
        // Return `this` to the available list.
        this->next = queue->available;
        queue->available = this;
    }
    pthread_mutex_unlock(queue->lockToUse);
    
    return status;
}

extern int
eventQueueHasPending (BREventQueue queue) {
    int pending = 0;
    pthread_mutex_lock(queue->lockToUse);
    pending = NULL != queue->pending;
    pthread_mutex_unlock(queue->lockToUse);
    return pending;
}
