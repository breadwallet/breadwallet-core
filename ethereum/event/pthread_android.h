//
//  pthread_android.h
//  BRCore
//
//  See: https://github.com/Hax4us/Hax4us.github.io/blob/master/Pthread_patches/Bpthread.h
//

#ifndef pthread_android_h
#define pthread_android_h

#if defined (__ANDROID__)

#include <pthread.h>
#include <signal.h>

#define SIG_CANCEL_SIGNAL SIGUSR1
#define PTHREAD_CANCEL_ENABLE 1
#define PTHREAD_CANCEL_DISABLE 0

static inline int
pthread_setcanceltype(int ignore1 , int *ignore2) {
    return 0
}

static int
pthread_setcancelstate(int state, int *oldstate) {
    sigset_t   new, old;
    int ret;
    sigemptyset (&new);
    sigaddset (&new, SIG_CANCEL_SIGNAL);

    ret = pthread_sigmask(state == PTHREAD_CANCEL_ENABLE ? SIG_BLOCK : SIG_UNBLOCK, &new , &old);
    if(oldstate != NULL)
    {
        *oldstate =sigismember(&old,SIG_CANCEL_SIGNAL) == 0 ? PTHREAD_CANCEL_DISABLE : PTHREAD_CANCEL_ENABLE;
    }
    return ret;
}


static inline int
pthread_cancel(pthread_t thread) {
    return pthread_kill(thread, SIG_CANCEL_SIGNAL);
}

static inline int
pthread_setname_np(const char *ignore) {
    return 0;
}

#endif /* defined (__ANDROID__) */
#endif /* pthread_android_h */
