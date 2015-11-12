//
//  BRRWLock.h
//
//  Created by Aaron Voisine on 11/10/15.
//  Copyright (c) 2015 breadwallet LLC
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

#ifndef BRRWLock_h
#define BRRWLock_h

#include <pthread.h>

typedef struct {
    int readerCount;
    int writerCount;
    int writerWaitingCount;
    pthread_mutex_t mutex;
    pthread_cond_t readerGate;
    pthread_cond_t writerGate;
} BRRWLock;

#define BR_RW_LOCK_INITIALIZER ((BRRWLock) {\
    0, 0, 0,\
    (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER,\
    (pthread_cond_t)PTHREAD_COND_INITIALIZER,\
    (pthread_cond_t)PTHREAD_COND_INITIALIZER\
})

inline static int BRRWLockInit(BRRWLock *lock)
{
    int r = 0;
    
    if (! r) r = pthread_mutex_init(&lock->mutex, NULL);
    if (! r) r = pthread_cond_init(&lock->readerGate, NULL);
    if (! r) r = pthread_cond_init(&lock->writerGate, NULL);
    return r;
}

inline static int BRRWLockReadLock(BRRWLock *lock)
{
    int r = pthread_mutex_lock(&lock->mutex);
    
    while (! r && lock->writerCount > 0) {
        r = pthread_cond_wait(&lock->readerGate, &lock->mutex);
    }
    
    lock->readerCount++;
    if (! r) r = pthread_mutex_unlock(&lock->mutex);
    return r;
}

inline static int BRRWLockWriteLock(BRRWLock *lock)
{
    int r = pthread_mutex_lock(&lock->mutex);
    
    lock->writerWaitingCount++;
    
    while (! r && (lock->readerCount > 0 || lock->writerCount > 0)) {
        r = pthread_cond_wait(&lock->writerGate, &lock->mutex);
    }
    
    lock->writerWaitingCount--;
    lock->writerCount++;
    if (! r) r = pthread_mutex_unlock(&lock->mutex);
    return r;
}

inline static int BRRWLockUnlock(BRRWLock *lock)
{
    int r = pthread_mutex_lock(&lock->mutex);
    
    if (lock->writerCount > 0) {
        lock->writerCount--;
        if (! r && lock->writerWaitingCount > 0) r = pthread_cond_signal(&lock->writerGate);
        if (! r) r = pthread_cond_broadcast(&lock->readerGate);
    }
    else {
        lock->readerCount--;
        if (! r && lock->readerCount == 0 && lock->writerWaitingCount > 0) r = pthread_cond_signal(&lock->writerGate);
    }
    
    if (! r) r = pthread_mutex_unlock(&lock->mutex);
    return r;
}

inline static int BRRWLockDestroy(BRRWLock *lock)
{
    int r = 0;
    
    if (! r) r = pthread_mutex_destroy(&lock->mutex);
    if (! r) r = pthread_cond_destroy(&lock->readerGate);
    if (! r) r = pthread_cond_destroy(&lock->writerGate);
    return r;
}

#endif // BRRWLock_h
