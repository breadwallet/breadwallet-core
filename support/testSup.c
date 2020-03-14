//
//  testSup.c
//  CoreTests
//
//  Created by Ed Gamble on 1/25/19.
//  Copyright © 2019 breadwallet LLC
//

#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ftw.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

typedef void* (*ThreadRoutine) (void*);         // pthread_create

#include "support/BRFileService.h"
#include "support/BRAssert.h"

#define PTHREAD_NULL            ((pthread_t) NULL)

/// MARK: - Helpers

static int _pthread_cond_timedwait_relative (pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *reltime) {
#if defined(__ANDROID__)
        struct timeval t;
        gettimeofday(&t, NULL);

        struct timespec timeout;
        timeout.tv_sec  = reltime->tv_sec + t.tv_sec;
        timeout.tv_nsec = reltime->tv_nsec +t.tv_usec*1000;

        return pthread_cond_timedwait (cond, mutex, &timeout);
#else
        return pthread_cond_timedwait_relative_np (cond, mutex, reltime);
#endif
}

/// MARK: - File Service Tests

static int
_remove(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);
    if (rv)
        perror(fpath);
    return rv;
}

static int _rmdir (char *path)
{
    return nftw (path, _remove, 64, FTW_DEPTH | FTW_PHYS);
}

static int
fileServiceTestDone (char *path, int success) {
    struct stat dirStat;

    if (0 == stat  (path, &dirStat)) _rmdir (path);
    return success;
}

static BRFileService
fileServiceSetupError (const char *path, BRFileService fs) {
    struct stat dirStat;

    if (NULL != fs) fileServiceRelease(fs);
    if (0 == stat  (path, &dirStat)) _rmdir ((char *) path);
    return NULL;
}

static void
fileServiceErrorHandler (BRFileServiceContext context,
                            BRFileService fs,
                            BRFileServiceError error) {
    //BREthereumEWM ewm = (BREthereumEWM) context;

    switch (error.type) {
        case FILE_SERVICE_IMPL:
            // This actually a FATAL - an unresolvable coding error.
            printf ("  supFileServiceThread: FileService Error: IMPL: %s\n", error.u.impl.reason);
            break;
        case FILE_SERVICE_UNIX:
            printf ("  supFileServiceThread: FileService Error: UNIX: %s\n", strerror(error.u.unix.error));
            break;
        case FILE_SERVICE_ENTITY:
            // This is likely a coding error too.
            printf ("  supFileServiceThread: FileService Error: ENTITY (%s): %s\n",
                     error.u.entity.type,
                     error.u.entity.reason);
            break;
        case FILE_SERVICE_SDB:
            printf ("  supFileServiceThread: FileService Error: SDB: (%d): %s\n",
                     error.u.sdb.code,
                     error.u.sdb.reason);
            break;
    }
}

static BRFileService
fileServiceSetup (const char *path, const char *currency, const char *network, const char *type1) {
    BRFileService fs = fileServiceCreate(path, currency, network, NULL, fileServiceErrorHandler);
    if (NULL == fs) return fileServiceSetupError (path, fs);

    if (1 != fileServiceDefineType(fs, type1, 0, NULL, NULL, NULL, NULL))
        return fileServiceSetupError (path, fs);

    if (1 != fileServiceDefineCurrentVersion(fs, type1, 0))
        return fileServiceSetupError (path, fs);


    return fs;
}
/// MARK: - File Service Tests

static int runSupFileServiceTests (void) {
    printf ("==== SUP:FileService\n");

    struct stat dirStat;

    BRFileService fs;
    char *path;
    char *currency = "btc", *network = "mainnet";
    char *type1 = "foo";
    //
    // Try to create a directory that is not writable; expect fileServiceCreate() to fail.
    //
    path = "private";

    if (0 == stat  (path, &dirStat)) _rmdir (path);
    if (0 != mkdir (path, 0000)) return 0;

    fs = fileServiceCreate(path, currency, network, NULL, NULL);
    if (NULL != fs) return fileServiceTestDone(path, 0);

    //
    // Create a directory that is writeable; expect fileServiceCreate to succeed.
    //
    if (0 == stat  (path, &dirStat)) _rmdir (path);
    if (0 != mkdir (path, 0700)) return 0;

    fs = fileServiceCreate(path, currency, network, NULL, NULL);
    if (NULL == fs) return fileServiceTestDone(path, 0);

    // Confirm the full path exists.
    char dbpath[1024];
    sprintf (dbpath, "%s/%s-%s-entities.db", path,  currency, network);
    if (0 != stat (dbpath, &dirStat)) return fileServiceTestDone (path, 0);

    if (1 != fileServiceDefineType(fs, type1, 0, NULL, NULL, NULL, NULL))
        return fileServiceTestDone (path, 0);

    if (1 != fileServiceDefineCurrentVersion(fs, type1, 0))
        return fileServiceTestDone (path, 0);

    // Good, finally.
    return fileServiceTestDone(path, 1);
}

typedef struct {
    pthread_t thread;
    BRFileService fs;
    int done;
    int success;
} BRFileServiceHelper;

static void *
supFileServiceThread (BRFileServiceHelper *fsh) {
    for (size_t i = 0; i < 100; i++) {
        int success = fileServiceReplace (fsh->fs, "foo", NULL, 0); // 1 on success
        if (!success) {
            printf ("  supFileServiceThread: Replace Error\n");
            fsh->success = 0;
            break;
        }
    }
    fsh->done = 1;
    return NULL;
}

static void
supFileServiceConnect (BRFileServiceHelper *fsh) {
    fsh->done    = 0;
    fsh->success = 1;
    {
        pthread_attr_t attr;
        pthread_attr_init (&attr);
        pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setstacksize (&attr, 1024 * 1024);
        pthread_create(&fsh->thread, &attr, (ThreadRoutine) supFileServiceThread, fsh);
        pthread_attr_destroy(&attr);
    }
}

static int runSupFileServiceMultiTests (void) {
    printf ("==== SUP:FileServiceMulti\n");

    struct stat dirStat;

    BRFileService fs1, fs2;
    char *path;
    char *currency = "btc", *network = "mainnet";
    char *type1 = "foo";
    //
    // Try to create a directory that is not writable; expect fileServiceCreate() to fail.
    //
    path = "private";

    if (0 == stat  (path, &dirStat)) _rmdir (path);
    if (0 != mkdir (path, 0700)) return 0;

    fs1 = fileServiceSetup (path, currency, network, type1);
    fs2 = fileServiceSetup (path, currency, network, type1);
    if (NULL == fs1 || NULL == fs2) return fileServiceTestDone(path, 0);

#define FS_HELPER_COUNT         10

    BRFileServiceHelper fsh[FS_HELPER_COUNT];
    for (size_t i = 0; i < FS_HELPER_COUNT; i++) {
        fsh[i].fs = fs1; // (0 == i % 2 ? fs1 : fs2);
        fsh[i].done = 0;
        supFileServiceConnect (&fsh[i]);
    }

    struct timespec ts = { 0, 1e8 };
    int done = 0;
    while (FS_HELPER_COUNT != done) {
        done = 0;
        for (size_t i = 0; i < FS_HELPER_COUNT; i++) {
            done += fsh[i].done;
            nanosleep(&ts, NULL);
        }
    }

    int success = 1;

    for (size_t i = 0; i < FS_HELPER_COUNT; i++)
        success &= fsh[i].success;

    fileServiceRelease(fs1);
    fileServiceRelease(fs2);
    return fileServiceTestDone(path, success);
}
/// MARK: - Assert Tests


#define DEFAULT_WORKERS     (5)
#define SUP_MAIN_COUNT      (3)

///
/// Worker
///
/// This represents an arbitary computation.  It runs in its own thread and will randomly fail.
/// It must be disconnected if an error occurs.
///
/// Multiple workers are owned by 'main'; on an error 'main' is responsible for disconnecting all
/// of main's worker.
///
typedef struct SupWorkerRecord {
    pthread_t thread;
    pthread_cond_t cond;
    pthread_mutex_t lock;
} *SupWorker;

static SupWorker
supWorkerCreate () {
    return calloc (1, sizeof (struct SupWorkerRecord));
}

/// Do the disconnect.
static void
supWorkerDisconnect (SupWorker worker, int report) {
    if (report) printf ("Work (%p): Disconnect\n", worker);
    pthread_cond_signal (&worker->cond);
    pthread_mutex_lock(&worker->lock);
    pthread_t thread = worker->thread;
    worker->thread = PTHREAD_NULL;
    pthread_mutex_unlock(&worker->lock);
    pthread_join(thread, NULL);
}

/// Confirm a disconnect
static int
supWorkerIsConnected (SupWorker worker) {
    int connected = 0;
    pthread_mutex_lock(&worker->lock);
    connected = PTHREAD_NULL != worker->thread;
    pthread_mutex_unlock(&worker->lock);
    return connected;
}

/// Release by disconnecting, then freeing
static void
supWorkerRelease (SupWorker worker) {
    printf ("Work (%p): Release\n", worker);
    supWorkerDisconnect(worker, 0);
    free (worker);
}

static void *
supWorkerThread (SupWorker worker) {
    struct timespec timeout = { 1, 0 };   // 1 second
    printf ("Work (%p): Run\n", worker);

    pthread_mutex_lock(&worker->lock);
    while (1) {
        switch (_pthread_cond_timedwait_relative (&worker->cond, &worker->lock, &timeout)) {
            case ETIMEDOUT:
                if (0 == arc4random_uniform (10 * DEFAULT_WORKERS)) {
                    printf ("Work (%p): Fail\n", worker);
                    pthread_mutex_unlock(&worker->lock);
                    BRFail();
                }
                break;
            default:
                pthread_mutex_unlock(&worker->lock);
                pthread_exit(NULL);
       }
    }
}

/// Connet by spawning the thread.
static void
supWorkerConnect (SupWorker worker) {
    worker->cond = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    worker->lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    {
        pthread_attr_t attr;
        pthread_attr_init (&attr);
        pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setstacksize (&attr, 1024 * 1024);
        pthread_create(&worker->thread, &attr, (ThreadRoutine) supWorkerThread, worker);
        pthread_attr_destroy(&attr);
    }
}

///
/// Main
///
/// Manages a number of workers and does its own computation.  This is will have an 'Assert
/// Recovery' which will a) disconnect the workers and b) disconnect itself.
///
typedef struct SupMainRecord {
    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    SupWorker workers[DEFAULT_WORKERS];
} *SupMain;

/// Create itself and workers
static SupMain
supMainCreate (void) {
    SupMain main = calloc (1, sizeof (struct SupMainRecord));

    main->cond = (pthread_cond_t)  PTHREAD_COND_INITIALIZER;
    main->lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    // Create workers
    for (size_t index = 0; index < DEFAULT_WORKERS; index++)
        main->workers[index] = supWorkerCreate();

    return main;
}

/// Release each worker, then itself
static void
supMainRelease (SupMain main) {
    printf ("Main (%p): Release Workers\n", main);
    for (size_t index = 0; index < DEFAULT_WORKERS; index++) {
        supWorkerRelease(main->workers[index]);
        main->workers[index] = NULL;
    }
    printf ("Main (%p): Release Self\n", main);
    free (main);
}

static void
supMainConnect (SupMain main);

/// Disconnect the workers (waiting for each) and then disconect and wait on itself.
static void
supMainDisconnect (SupMain main) {
    pthread_mutex_lock (&main->lock);
    printf ("Main (%p): Disconnect Workers\n", main);
   for (size_t index = 0; index < DEFAULT_WORKERS; index++)
        supWorkerDisconnect (main->workers[index], 1);
    
    printf ("Main (%p): Disconnect Self\n", main);
    if (PTHREAD_NULL != main->thread) {
        pthread_cond_signal(&main->cond);
        pthread_mutex_unlock (&main->lock);
        pthread_join(main->thread, NULL);
        main->thread = PTHREAD_NULL;
    }
    else pthread_mutex_unlock (&main->lock);
}

static int
supMainIsConnected (SupMain main) {
    int connected;
    pthread_mutex_lock (&main->lock);
    connected = PTHREAD_NULL != main->thread;
    pthread_mutex_unlock (&main->lock);
    return connected;
}

/// Start the workers; do our own work
static void
supMainDoWork (SupMain main) {
    printf ("Main (%p): Workers\n", main);
    for (size_t index = 0; index < DEFAULT_WORKERS; index++)
        supWorkerConnect (main->workers[index]);

    // Do our own work.
    pthread_mutex_lock(&main->lock);
    pthread_cond_wait(&main->cond, &main->lock);
    pthread_mutex_unlock(&main->lock);
}

/// Recover by disconnecting ourself and our workers
static void
supMainRecovery (SupMain main) {
    printf ("Main (%p): Recover\n", main);
    supMainDisconnect(main);
}

/// Install the recovery; then do work.
static void *
supMainThread (SupMain main) {
    BRAssertDefineRecovery (main, (BRAssertRecoveryHandler) supMainRecovery);

    printf ("Main (%p): Job\n", main);
    supMainDoWork(main);

//    BRAssertRemoveRecovery(main);
    pthread_exit(NULL);
}

/// Spawn the thread to do work.
static void
supMainConnect (SupMain main) {
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize (&attr, 1024 * 1024);
    pthread_create(&main->thread, &attr, (ThreadRoutine) supMainThread, main);
    pthread_attr_destroy(&attr);
}

pthread_cond_t  done_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t done_lock = PTHREAD_MUTEX_INITIALIZER;

/// Test is complete if all workers and each main is no longer connected
static int
supConfirmComplete (SupMain *mains) {
    // Recovery is complete.
    for (size_t m = 0; m < SUP_MAIN_COUNT; m++) {
        for (size_t w = 0; w < DEFAULT_WORKERS; w++)
            if (supWorkerIsConnected (mains[m]->workers[w])) return 0;
        if (supMainIsConnected(mains[m])) return 0;
    }
    return 1;
}

/// Handle an assert by confirming that all mains are completed and then signal done
static void
supAssertHandler (SupMain *mains) {
    supConfirmComplete(mains);
    pthread_cond_signal (&done_cond);
}

/// connect all main threads and then wait until they finish.  Return success if completed.
static int
supRunOnce (SupMain *mains) {
    int success = 0;
    struct timespec timeout = { 30, 0 };

    for (size_t index = 0; index < SUP_MAIN_COUNT; index++)
        supMainConnect (mains[index]);

    // We wait for supAssertHandler to signal that we've handled a worker BRFail.

    pthread_mutex_lock (&done_lock);
    switch (_pthread_cond_timedwait_relative (&done_cond, &done_lock, &timeout)) {
        case ETIMEDOUT: break;
        default: success = 1; break;
    }
    pthread_mutex_unlock (&done_lock);

    return success && supConfirmComplete(mains);
}

static int
runSupAssertTests (void) {
    printf ("==== SUP: Assert\n");

    SupMain mains[SUP_MAIN_COUNT];
    int success = 1;

    if (0 != BRAssertIsInstalled())
        return 0;
    BRAssertInstall(mains, (BRAssertHandler) supAssertHandler);
    if (1 != BRAssertIsInstalled())
        return 0;
    BRAssertUninstall();
    if (0 != BRAssertIsInstalled())
        return 0;

    if (0 != BRAssertRemoveRecovery((BRAssertRecoveryInfo) 1)) return 0;
    BRAssertDefineRecovery((BRAssertRecoveryInfo) 1, NULL);
    if (1 != BRAssertRemoveRecovery((BRAssertRecoveryInfo) 1)) return 0;
    if (0 != BRAssertRemoveRecovery((BRAssertRecoveryInfo) 1)) return 0;

    // Try once...
   BRAssertInstall(mains, (BRAssertHandler) supAssertHandler);
    for (size_t index = 0; index < SUP_MAIN_COUNT; index++)
        mains[index] = supMainCreate();

    printf ("==== SUP: Assert Run Once\n");
    success = supRunOnce(mains);
    // We have fully recovered

    // Try again...
    printf ("==== SUP:Assert Run Twice\n");
   success &= supRunOnce(mains);
    for (size_t index = 0; index < SUP_MAIN_COUNT; index++)
        supMainRelease (mains[index]);

    BRAssertUninstall();
    printf ("==== SUP:Assert Done");
    return success;
}

///
/// Support Tests
///
int BRRunSupTests (void) {
    printf ("==== SUP\n");
    int success = 1;

    success &= runSupFileServiceTests();
    success &= runSupFileServiceMultiTests ();
    success &= runSupAssertTests();

    return success;
}
