//
//  BRAssert.c
//  BRCore
//
//  Created by Ed Gamble on 2/4/19.
//  Copyright © 2019 breadwallet. All rights reserved.
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

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>         // sleep
#include <limits.h>         // UINT_MAX
#include <assert.h>
#include "BRAssert.h"
#include "../BRArray.h"

#define BRArrayOf(type)             type*

#if defined(TARGET_OS_MAC)
#include <Foundation/Foundation.h>
#define assert_log(...) NSLog(__VA_ARGS__)
#elif defined(__ANDROID__)
#include <android/log.h>
#define assert_log(...) __android_log_print(ANDROID_LOG_INFO, "bread", __VA_ARGS__)
#else
#include <stdio.h>
#define assert_log(...) printf(__VA_ARGS__)
#endif

#define ASSERT_THREAD_NAME      "Core Assert Handler"
#define ASSERT_DEFAULT_RECOVERIES_COUNT         (5)

/**
 * A Recovery Context.  For example, used to invoke BRPeerManagerDisconnect on a BRPeerManager.
 */
typedef struct {
    BRAssertRecoveryInfo info;
    BRAssertRecoveryHandler handler;
} BRAssertRecoveryContext;


static void
BRAssertRecoveryInvoke (BRAssertRecoveryContext *context) {
    if (NULL != context->handler)
        context->handler (context->info);
}

/**
 * The Context - our assert handler context.
 */
typedef struct {
    BRAssertInfo info;
    BRAssertHandler handler;
    BRArrayOf(BRAssertRecoveryContext) recoveries;

    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t cond;

    int timeToQuit;
} BRAssertContext;

/**
 * Allocate a singleton instance of BRAssertContext
 */
static BRAssertContext context_record = {
    NULL, NULL, NULL,
    NULL, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER,
    0
};
static BRAssertContext *context = &context_record;

static void
BRAssertEnsureRecoveries (BRAssertContext *context) {
    if (NULL == context->recoveries)
        array_new (context->recoveries, ASSERT_DEFAULT_RECOVERIES_COUNT);
}
/**
 * Invoke all the context's recoveries.
 */
static void
BRAssertInvokeRecoveries (BRAssertContext *context) {
    size_t count = array_count(context->recoveries);
    for (size_t index = 0; index < count; index++)
        BRAssertRecoveryInvoke (&context->recoveries[index]);
}

/**
 * Invoke the context's handler or it it doesn't exist then optionally exit.
 */
static void
BRAssertInvokeHandler (BRAssertContext *context, int exitIfNoHandler) {
    if (NULL != context->handler)
        context->handler (context->info);
    else if (exitIfNoHandler)
        exit (EXIT_FAILURE);
}

static void
BRAssertInit (void) {
    context->info = NULL;
    context->handler = NULL;
    if (NULL != context->recoveries) array_free(context->recoveries);
    context->thread = NULL;
    // lock - do not touch
    // cond
    // timeToQuit - do not touch (see comment in BRFail)
}

typedef void* (*ThreadRoutine) (void*);         // pthread_create

static void *
BRAssertThread (BRAssertContext *context) {
#if defined (__ANDROID__)
    pthread_setname_np (assert_thread, ASSERT_THREAD_NAME);
#else
    pthread_setname_np (ASSERT_THREAD_NAME);
#endif

    pthread_mutex_lock(&context->lock);
    context->timeToQuit = 0;

    while (0 == pthread_cond_wait(&context->cond, &context->lock)) {
        if (context->timeToQuit) break;

        assert_log ("AssertThread Caught\n");

        // Invoke recovery methods
        BRAssertInvokeRecoveries (context);

        // Invoke (top-level) handler.  If there is no handler, we will exit()
        BRAssertInvokeHandler(context, 1);

        // We run once and only once.  Another BRAssertInstall() is required to get going.
        // Afterall, the point is that BRFail() fails core and *no* threads should be running
        break;
    }

    // Clear out the context.
    BRAssertInit();

    // Required as pthread_cont_wait() takes the lock.
    pthread_mutex_unlock(&context->lock);

    // done
    pthread_exit(0);
}

extern void
BRAssertInstall (BRAssertInfo info, BRAssertHandler handler) {
    pthread_mutex_lock (&context->lock);
    if (NULL == context->thread) {
        context->info = info;
        context->handler = handler;

        BRAssertEnsureRecoveries (context);

        context->timeToQuit = 0;

        {
            pthread_attr_t attr;
            pthread_attr_init (&attr);
            pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
            pthread_attr_setstacksize (&attr, 1024 * 1024);
            pthread_create (&context->thread, &attr, (ThreadRoutine) BRAssertThread, context);
            pthread_attr_destroy(&attr);
        }
    }
    pthread_mutex_unlock(&context->lock);
}

extern void
BRAssertUninstall (void) {
    pthread_mutex_lock (&context->lock);
    if (NULL != context->thread) {
         // Set this flag so that the assert handler thread *will avoid* running recoveries.
        context->timeToQuit = 1;

        // Signal the assert handler thread; it will wakeup, observe `timeToQuit` and then exit.
        pthread_cond_signal (&context->cond);
    }
    // Allow the assert handler to run and quit.
    pthread_mutex_unlock (&context->lock);
}

extern int
BRAssertIsInstalled (void) {
    int isConnected = 0;

    pthread_mutex_lock (&context->lock);
    isConnected = NULL != context->thread;
    pthread_mutex_unlock (&context->lock);

    return isConnected;
}

extern void
__BRFail (const char *file, int line, const char *exp) {
    assert_log ("%s:%u: failed assertion `%s'\n", file, line, exp);

    pthread_cond_signal (&context->cond);
#if defined(DEBUG)
    assert (0);
#endif
    pthread_exit (NULL);
}

extern void
BRAssertDefineRecovery (BRAssertRecoveryInfo info,
                        BRAssertRecoveryHandler handler) {
    int needRecovery = 1;

    pthread_mutex_lock (&context->lock);
    BRAssertEnsureRecoveries (context);

    for (size_t index = 0; index < array_count(context->recoveries); index++)
        if (info == context->recoveries[index].info) {
            context->recoveries[index].handler = handler;
            needRecovery = 0;
            break; // for
        }

    if (needRecovery) {
        BRAssertRecoveryContext recovery = { info, handler };
        array_add (context->recoveries, recovery);
    }
    pthread_mutex_unlock(&context->lock);
}

extern int
BRAssertRemoveRecovery (BRAssertRecoveryInfo info) {
    int removedRecovery = 0;

    pthread_mutex_lock (&context->lock);
    BRAssertEnsureRecoveries (context);

    for (size_t index = 0; index < array_count(context->recoveries); index++)
        if (info == context->recoveries[index].info) {
            array_rm(context->recoveries, index);
            removedRecovery = 1;
            break; // for
        }
    pthread_mutex_unlock (&context->lock);
    return removedRecovery;
}
