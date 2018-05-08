//
//  BREthereumEventQueue.c
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
#include "BREthereumEventQueue.h"

#define EVENT_QUEUE_DEFAULT_INITIAL_CAPACITY   (1)

struct BREthereumEventQueueRecord {
    // A linked-list (through event->next) of pending events.
    BREthereumEvent *pending;

    // A linked-list (through event->next) of available events
    BREthereumEvent *available;

    // A lock for exclusive access when queueing/dequeueing events.
    pthread_mutex_t lock;

    // Boolean to identify we this queue owns (created) the lock.
    int lockOwner;

    // The size of each event
    size_t size;
};

extern BREthereumEventQueue
eventQueueCreate (size_t size, pthread_mutex_t *lock) {
    BREthereumEventQueue queue = calloc (1, sizeof (struct BREthereumEventQueueRecord));

    queue->pending = NULL;
    queue->available = NULL;
    queue->size = size;

    for (int i = 0; i < EVENT_QUEUE_DEFAULT_INITIAL_CAPACITY; i++) {
        BREthereumEvent *event = calloc (1, queue->size);
        event->next = queue->available;
        queue->available = event;
    }

    queue->lock = *lock;
    queue->lockOwner = 0;

    // If a lock was not provided, create the PTHREAD LOCK, as a normal, non-recursive, lock.
    if (NULL == lock) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&queue->lock, &attr);
        pthread_mutexattr_destroy(&attr);

        queue->lockOwner = 1;
    }

    return queue;
}

extern void
eventQueueDestroy (BREthereumEventQueue queue) {
    pthread_mutex_lock(&queue->lock);

    // Move pending to available
    if (NULL != queue->pending) {
        BREthereumEvent *next, *this = queue->pending;
        while (NULL != this->next) {
            next = this->next;
            this->next = queue->available;
            queue->available = this;
            this = next;
        }
        queue->pending = NULL;
    }

    // Free available
    if (NULL != queue->available) {
        BREthereumEvent *next, *this = queue->available;
        while (NULL != this->next) {
            next = this->next;
            free (this);
            this = next;
        }
        queue->available = NULL;
    }

    pthread_mutex_unlock(&queue->lock);

    if (queue->lockOwner)
        pthread_mutex_destroy(&queue->lock);

    memset (queue, 0, sizeof (struct BREthereumEventQueueRecord));
}

extern void
eventQueueEnqueue (BREthereumEventQueue queue,
                   const BREthereumEvent *event) {
    pthread_mutex_lock(&queue->lock);

    // Get the next available event
    BREthereumEvent *this = queue->available;
    if (NULL == this) {
        this = (BREthereumEvent*) calloc (1, queue->size);
        this->next = NULL;
    }
    // Make the next event no longer available.
    queue->available = this->next;

    // Fill in `this` with event
    memcpy (this, event, queue->size);
    this->next = NULL;

    // Add to the end of pending
    if (NULL == queue->pending)
        queue->pending = this;
    else {
        // Find the last event
        BREthereumEvent *last = queue->pending;
        while (NULL != last->next) last = last->next;
        last->next = this;
    }

    pthread_mutex_unlock(&queue->lock);
}

extern BRethereumEventStatus
eventQueueDequeue (BREthereumEventQueue queue,
                   BREthereumEvent *event) {
    BRethereumEventStatus status = EVENT_STATUS_SUCCESS;
    
    if (NULL == event)
        return EVENT_STATUS_NULL_EVENT;
    
    pthread_mutex_lock(&queue->lock);
    
    // Get the next pending event
    BREthereumEvent *this = queue->pending;
    
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
    pthread_mutex_unlock(&queue->lock);
    
    return status;
}

extern int
eventQueueHasPending (BREthereumEventQueue queue) {
    int pending = 0;
    pthread_mutex_lock(&queue->lock);
    pending = NULL != queue->pending;
    pthread_mutex_unlock(&queue->lock);
    return pending;
}
