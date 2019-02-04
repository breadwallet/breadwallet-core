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

// https://stackoverflow.com/questions/6326290/about-the-ambiguous-description-of-sigwait
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>         // sleep
#include <limits.h>         // UINT_MAX
#include <signal.h>
#include "BRAssert.h"
#include "../BRArray.h"

typedef void* (*ThreadRoutine) (void*);         // pthread_create

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

#define ASSERT_THREAD_NAME      "Core Signal Handler"
#define ASSERT_DEFAULT_SIGNUM   SIGUSR1

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
 * The Context - our full signal handler context.
 */
typedef struct {
    int signum;

    BRAssertInfo info;
    BRAssertHandler handler;
    BRArrayOf(BRAssertRecoveryContext) recoveries;

    pthread_t thread;
    pthread_mutex_t lock;

    int timeToQuit;
    int isAssert;
} BRAssertContext;

static BRAssertContext context_record = {
    ASSERT_DEFAULT_SIGNUM,
    NULL, NULL, NULL,
    NULL, PTHREAD_MUTEX_INITIALIZER,
    0, 0
};
static BRAssertContext *context = &context_record;


extern void
BRAssertInstall (int signum, BRAssertInfo info, BRAssertHandler handler) {
    context->signum = signum;
    context->info = info;
    context->handler = handler;
    context->thread = NULL;

    if (NULL == context->recoveries) array_new(context->recoveries, 5);

    context->timeToQuit = 0;
    context->isAssert = 0;
}

static void
BRAssertInvokeRecoveries (BRAssertContext *context) {
    size_t count = array_count(context->recoveries);
    for (size_t index = 0; index < count; index++)
        BRAssertRecoveryInvoke (&context->recoveries[index]);
}

static void
BRAssertInvokeHandler (BRAssertContext *context) {
    if (NULL != context->handler)
        context->handler (context->info);
}

static void *
BRAssertThread (BRAssertContext *context) {
#if defined (__ANDROID__)
    pthread_setname_np (assert_thread, ASSERT_THREAD_NAME);
#else
    pthread_setname_np (ASSERT_THREAD_NAME);
#endif

    int caughtSignum = 0;
    sigset_t caughtSignalSet;

    // "The signals specified by set should be blocked, but not ignored, at the time
    // of the call to sigwait()."
    sigemptyset(&caughtSignalSet);
    sigaddset(&caughtSignalSet, context->signum);
    pthread_sigmask(SIG_BLOCK, &caughtSignalSet, NULL);

    while (0 == sigwait (&caughtSignalSet, &caughtSignum)) {
        // There is a race here... an unlikely one on BRAssertInstall changing the
        // context->signum.
        pthread_mutex_lock(&context->lock);
        if (caughtSignum == context->signum && (context->isAssert || context->timeToQuit)) {
            context->isAssert = 0;

            if (context->timeToQuit) break;

            assert_log ("AssertThread Caught: %d\n", caughtSignum);

            // Invoke recovery methods
            BRAssertInvokeRecoveries (context);

            // Invoke (top-level) handler
            BRAssertInvokeHandler(context);
        }

        pthread_mutex_unlock(&context->lock);
    }

    pthread_mutex_unlock(&context->lock);
    pthread_exit(0);
}

extern void
BRAssertConnect (void) {
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize (&attr, 1024 * 1024);
    pthread_create (&context->thread, &attr, (ThreadRoutine) BRAssertThread, context);
    pthread_attr_destroy(&attr);
}

extern void
BRAssertDisconnect (void) {
    pthread_mutex_lock (&context->lock);
    if (NULL != context->thread) {
        pthread_t thread = context->thread;
        int signum = context->signum;

        context->timeToQuit = 1;
        context->thread = NULL;

        pthread_mutex_unlock(&context->lock);

        // We might prefer pthread_cancel() but that is not supported on Android

        pthread_kill(thread, signum);
        pthread_join(thread, NULL);
    }
    else pthread_mutex_unlock(&context->lock);
}

extern int
BRAassertIsConnected (void) {
    int isConnected = 0;

    pthread_mutex_lock (&context->lock);
    isConnected = NULL != context->thread;
    pthread_mutex_lock (&context->lock);

    return isConnected;
}

extern void
BRFail (void) {
    pthread_mutex_lock (&context->lock);
    if (NULL != context->thread) {
        pthread_t thread = context->thread;
        int signum = context->signum;

        context->isAssert = 1;

        pthread_mutex_unlock(&context->lock);

        // Send the signal directly to the context->header
        pthread_kill (thread, signum);

    }
    else pthread_mutex_unlock(&context->lock);

    // Current thread has failed.
    pthread_exit(NULL);
}

extern void
BRAssertDefineRecovery (BRAssertRecoveryInfo info,
                        BRAssertRecoveryHandler handler) {
    if (NULL == context->recoveries) array_new(context->recoveries, 5);

    BRAssertRecoveryContext recovery = { info, handler };
    array_add (context->recoveries, recovery);
}
