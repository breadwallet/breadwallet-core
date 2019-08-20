//
//  test.c
//
//  Created by Michael Carrara on 8/18/19.
//  Copyright (c) 2019 breadwallet LLC
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

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bcash/BRBCashParams.h"
#include "bitcoin/BRChainParams.h"

#include "BRArray.h"
#include "BRBIP39Mnemonic.h"
#include "BRPeerManager.h"
#include "BRTransaction.h"
#include "BRWallet.h"
#include "BRWalletManager.h"

#ifdef __ANDROID__
#include <android/log.h>
#define fprintf(...) __android_log_print(ANDROID_LOG_ERROR, "bread", _va_rest(__VA_ARGS__, NULL))
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "bread", __VA_ARGS__)
#define _va_first(first, ...) first
#define _va_rest(first, ...) __VA_ARGS__
#endif

///
///
///

static void
_testTransactionEventCallback (BRWalletManagerClientContext context,
                               BRWalletManager manager,
                               BRWallet *wallet,
                               BRTransaction *transaction,
                               BRTransactionEvent event) {
    printf ("TST: TransactionEvent: %d\n", event.type);
}

static void
_testWalletEventCallback (BRWalletManagerClientContext context,
                          BRWalletManager manager,
                          BRWallet *wallet,
                          BRWalletEvent event) {
    printf ("TST: WalletEvent: %d\n", event.type);
}

static int syncDone = 0;

static void
_testWalletManagerEventCallback (BRWalletManagerClientContext context,
                                 BRWalletManager manager,
                                 BRWalletManagerEvent event) {
    printf ("TST: WalletManagerEvent: %d\n", event.type);
    switch (event.type) {

        case BITCOIN_WALLET_MANAGER_CONNECTED:
            break;
        case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
            break;
        case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
            syncDone = 1;
            break;
        default:
            break;
    }
}

extern int BRRunTestWalletManagerSync (const char *paperKey,
                                       const char *storagePath,
                                       int isBTC,
                                       int isMainnet) {
    const BRChainParams *params = (isBTC & isMainnet ? BRMainNetParams
                                   : (isBTC & !isMainnet ? BRTestNetParams
                                      : (isMainnet ? BRBCashParams : BRBCashTestNetParams)));

    uint32_t epoch = 1483228800; // 1/1/17
    epoch += (365 + 365/2) * 24 * 60 * 60;

    printf ("***\n***\nPaperKey (Start): \"%s\"\n***\n***\n", paperKey);
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey (seed.u8, paperKey, NULL);
    BRMasterPubKey mpk = BRBIP32MasterPubKey(&seed, sizeof (seed));

    BRWalletManagerClient client = {
        NULL,
        (BRGetBlockNumberCallback) NULL,
        (BRGetTransactionsCallback) NULL,
        (BRSubmitTransactionCallback) NULL,

        _testTransactionEventCallback,
        _testWalletEventCallback,
        _testWalletManagerEventCallback
    };

    BRSyncMode mode = SYNC_MODE_P2P_ONLY;

    BRWalletManager manager = BRWalletManagerNew (client, mpk, params, epoch, mode, storagePath, 0);

    BRWalletManagerStart (manager);

    syncDone = 0;
    BRWalletManagerConnect (manager);

    int err = 0;
    while (err == 0 && !syncDone) {
        err = sleep(1);
    }
    err = 0;

    int seconds = 120;
    while (err == 0 && seconds-- > 0) {
        err = sleep(1);
    }

    printf ("***\n***\nPaperKey (Done): \"%s\"\n***\n***\n", paperKey);
    BRWalletManagerDisconnect (manager);
    sleep (2);
    BRWalletManagerStop (manager);
    sleep (2);
    BRWalletManagerFree (manager);
    return 1;
}

///
/// MARK: BRWalletManager Connect/Disconnect/Scan Tests
///

typedef enum {
    SYNC_EVENT_WALLET_MANAGER_TYPE,
    SYNC_EVENT_WALLET_TYPE,
    SYNC_EVENT_TXN_TYPE,
} BRWalletManagerSyncEventType;

typedef struct BRWalletManagerSyncEventRecord {
    BRWalletManagerSyncEventType type;
    union {
        struct {
            BRWalletManager manager;
            BRWalletManagerEvent event;
        } m;
        struct {
            BRWalletManager manager;
            BRWallet *wallet;
            BRWalletEvent event;
        } w;
        struct {
            BRWalletManager manager;
            BRWallet *wallet;
            BRTransaction *transaction;
            BRTransactionEvent event;
        } t;
    } u;
} BRWalletManagerSyncEvent;

const char *
BRWalletManagerSyncEventTypeString (BRWalletManagerSyncEventType type) {
    switch (type) {
        case SYNC_EVENT_WALLET_MANAGER_TYPE:
        return "SYNC_EVENT_WALLET_MANAGER_TYPE";
        case SYNC_EVENT_WALLET_TYPE:
        return "SYNC_EVENT_WALLET_TYPE";
        case SYNC_EVENT_TXN_TYPE:
        return "SYNC_EVENT_TXN_TYPE";
    }
}

static BRWalletManagerSyncEvent
BRWalletManagerSyncEventForWalletManagerType(BRWalletManagerEventType type) {
    return (BRWalletManagerSyncEvent) {
        SYNC_EVENT_WALLET_MANAGER_TYPE,
        {
            .m = {
                NULL,
                (BRWalletManagerEvent) {
                    type
                }
            }
        }
    };
}

static BRWalletManagerSyncEvent
BRWalletManagerSyncEventForWalletType(BRWalletEventType type) {
    return (BRWalletManagerSyncEvent) {
        SYNC_EVENT_WALLET_TYPE,
        {
            .w = {
                NULL,
                NULL,
                (BRWalletEvent) {
                    type
                }
            }
        }
    };
}

typedef struct {
    uint64_t blockHeight;
    uint8_t silent;
    BRArrayOf(BRWalletManagerSyncEvent *) events;
    pthread_mutex_t lock;
} BRRunTestWalletManagerSyncState;

static void
_testGetBlockNumberNopCallback (BRWalletManagerClientContext context,
                                BRWalletManager manager,
                                int rid) {
    // do nothing
}

static void
_testGetTransactionsNopCallback (BRWalletManagerClientContext context,
                                 BRWalletManager manager,
                                 OwnershipKept const char **addresses,
                                 size_t addressCount,
                                 uint64_t begBlockNumber,
                                 uint64_t endBlockNumber,
                                 int rid) {
    // do nothing
}

static void
_testSubmitTransactionNopCallback (BRWalletManagerClientContext context,
                                   BRWalletManager manager,
                                   BRWallet *wallet,
                                   OwnershipKept uint8_t *transaction,
                                   size_t transactionLength,
                                   UInt256 transactionHash,
                                   int rid) {
    // do nothing
}

static void
_testTransactionEventRecordingCallback (BRWalletManagerClientContext context,
                                        BRWalletManager manager,
                                        BRWallet *wallet,
                                        BRTransaction *transaction,
                                        BRTransactionEvent event) {
    BRRunTestWalletManagerSyncState *state = (BRRunTestWalletManagerSyncState*) context;
    BRWalletManagerSyncEvent *record = calloc (1, sizeof (BRWalletManagerSyncEvent));
    record->type = SYNC_EVENT_TXN_TYPE;
    record->u.t.manager= manager;
    record->u.t.wallet= wallet;
    record->u.t.transaction = transaction;
    record->u.t.event = event;

    pthread_mutex_lock (&state->lock);
    array_add (state->events, record);
    if (!state->silent) printf ("Added TXN event: %s (%zu total)\n", BRTransactionEventTypeString (event.type), array_count (state->events));
    pthread_mutex_unlock (&state->lock);
}

static void
_testWalletEventRecordingCallback (BRWalletManagerClientContext context,
                                   BRWalletManager manager,
                                   BRWallet *wallet,
                                   BRWalletEvent event) {
    BRRunTestWalletManagerSyncState *state = (BRRunTestWalletManagerSyncState*) context;
    BRWalletManagerSyncEvent *record = calloc (1, sizeof (BRWalletManagerSyncEvent));
    record->type = SYNC_EVENT_WALLET_TYPE;
    record->u.w.manager= manager;
    record->u.w.wallet= wallet;
    record->u.w.event = event;

    pthread_mutex_lock (&state->lock);
    array_add (state->events, record);
    if (!state->silent) printf ("Added WALLET event: %s (%zu total)\n", BRWalletEventTypeString (event.type), array_count (state->events));
    pthread_mutex_unlock (&state->lock);
}

static void
_testWalletManagerEventRecordingCallback (BRWalletManagerClientContext context,
                                          BRWalletManager manager,
                                          BRWalletManagerEvent event) {
    BRRunTestWalletManagerSyncState *state = (BRRunTestWalletManagerSyncState*) context;
    BRWalletManagerSyncEvent *record = calloc (1, sizeof (BRWalletManagerSyncEvent));
    record->type = SYNC_EVENT_WALLET_MANAGER_TYPE;
    record->u.m.manager= manager;
    record->u.m.event = event;

    pthread_mutex_lock (&state->lock);
    array_add (state->events, record);
    if (!state->silent) printf ("Added MANAGER event: %s (%zu total)\n", BRWalletManagerEventTypeString (event.type), array_count (state->events));
    pthread_mutex_unlock (&state->lock);
}

typedef struct {
    uint8_t kill;
    BRWalletManager manager;
} BRRunTestWalletManagerSyncThreadState;

static void *
_testBRWalletManagerConnectThread (void *context) {
    BRRunTestWalletManagerSyncThreadState *state = (BRRunTestWalletManagerSyncThreadState *) context;
    while (!state->kill) {
        BRWalletManagerConnect (state->manager);
    }
    return NULL;
}

static void *
_testBRWalletManagerDisconnectThread (void *context) {
    BRRunTestWalletManagerSyncThreadState *state = (BRRunTestWalletManagerSyncThreadState *) context;
    while (!state->kill) {
        BRWalletManagerDisconnect (state->manager);
    }
    return NULL;
}

static void *
_testBRWalletManagerScanThread (void *context) {
    BRRunTestWalletManagerSyncThreadState *state = (BRRunTestWalletManagerSyncThreadState *) context;
    while (!state->kill) {
        BRWalletManagerScan (state->manager);
    }
    return NULL;
}

static void *
_testBRWalletManagerSwapThread (void *context) {
    BRRunTestWalletManagerSyncThreadState *state = (BRRunTestWalletManagerSyncThreadState *) context;
    while (!state->kill) {
        switch (BRWalletManagerGetMode (state->manager)) {
            case SYNC_MODE_BRD_ONLY:
                BRWalletManagerSetMode (state->manager, SYNC_MODE_P2P_ONLY);
                break;
            case SYNC_MODE_P2P_ONLY:
                BRWalletManagerSetMode (state->manager, SYNC_MODE_BRD_ONLY);
                break;
            default:
                break;
        }
    }
    return NULL;
}

static size_t
BRRunTestWalletManagerSyncTestGetEventByType(BRRunTestWalletManagerSyncState *state, size_t startIndex, BRWalletManagerSyncEventType type) {
    for (size_t index = startIndex; index < array_count(state->events); index++) {
        if (state->events[index]->type == type) {
            return index;
        }
    }
    return SIZE_MAX;
}

static void
BRRunTestWalletManagerSyncTestSetup (BRRunTestWalletManagerSyncState *state, uint64_t blockHeight, uint8_t isSilent) {
    state->blockHeight = blockHeight;
    state->silent = isSilent;
    array_new (state->events, 100);
    pthread_mutex_init (&state->lock, NULL);
}

static void
BRRunTestWalletManagerSyncTestSetupAsDefault (BRRunTestWalletManagerSyncState *state, uint64_t blockHeight) {
    BRRunTestWalletManagerSyncTestSetup (state, blockHeight, 0);
}

static int
BRRunTestWalletManagerSyncTestVerifyEventSequenceAtIndex (BRRunTestWalletManagerSyncState *state, size_t index, BRWalletManagerSyncEvent *expected, size_t count) {
    int success = 1;
    pthread_mutex_lock (&state->lock);

    for (; success && index < array_count (state->events) && index < count; index++) {
        if (expected[index].type != state->events[index]->type) {
            success = 0;
            fprintf(stderr,
                    "***FAILED*** %s: mismatched event types (expected %s, received %s)\n",
                    __func__,
                    BRWalletManagerSyncEventTypeString (expected[index].type),
                    BRWalletManagerSyncEventTypeString (state->events[index]->type));
            continue;
        }

        switch (expected[index].type) {
            case SYNC_EVENT_WALLET_MANAGER_TYPE: {
                if (expected[index].u.m.event.type != state->events[index]->u.m.event.type) {
                    success = 0;
                    fprintf(stderr,
                            "***FAILED*** %s: mismatched managers types (expected %s, received %s)\n",
                            __func__,
                            BRWalletManagerEventTypeString (expected[index].u.m.event.type),
                            BRWalletManagerEventTypeString (state->events[index]->u.m.event.type));
                }
                break;
            }
            case SYNC_EVENT_WALLET_TYPE: {
                if (expected[index].u.w.event.type != state->events[index]->u.w.event.type) {
                    success = 0;
                    fprintf(stderr,
                            "***FAILED*** %s: mismatched managers types (expected %s, received %s)\n",
                            __func__,
                            BRWalletEventTypeString (expected[index].u.w.event.type),
                            BRWalletEventTypeString (state->events[index]->u.w.event.type));
                }
                break;
            }
            case SYNC_EVENT_TXN_TYPE: {
                if (expected[index].u.t.event.type != state->events[index]->u.t.event.type) {
                    success = 0;
                    fprintf(stderr,
                            "***FAILED*** %s: mismatched managers types (expected %s, received %s)\n",
                            __func__,
                            BRTransactionEventTypeString (expected[index].u.t.event.type),
                            BRTransactionEventTypeString (state->events[index]->u.t.event.type));
                }
                break;
            }
        }
    }

    pthread_mutex_unlock (&state->lock);
    return success;
}

static int
BRRunTestWalletManagerSyncTestVerifyEventSequence (BRRunTestWalletManagerSyncState *state, BRWalletManagerSyncEvent *expected, size_t count) {
    return BRRunTestWalletManagerSyncTestVerifyEventSequenceAtIndex (state, 0, expected, count);
}

static int
BRRunTestWalletManagerSyncTestVerifyEventSequenceExact (BRRunTestWalletManagerSyncState *state, BRWalletManagerSyncEvent *expected, size_t count) {
    int success = 1;

    pthread_mutex_lock (&state->lock);
    if (array_count (state->events) != count) {
        success = 0;
        fprintf(stderr,
                    "***FAILED*** %s: expected %zu but received %zu events\n",
                    __func__,
                    count,
                    array_count (state->events));
    }
    pthread_mutex_unlock (&state->lock);

    return success && BRRunTestWalletManagerSyncTestVerifyEventSequenceAtIndex (state, 0, expected, count);
}

static int
BRRunTestWalletManagerSyncTestVerification (BRRunTestWalletManagerSyncState *state) {
    int success = 1;
    pthread_mutex_lock (&state->lock);

    // check that each manager event pair is allowed
    {
        size_t index = BRRunTestWalletManagerSyncTestGetEventByType (state, 0, SYNC_EVENT_WALLET_MANAGER_TYPE);
        size_t nextIndex = 0;
        while (SIZE_MAX != (nextIndex = BRRunTestWalletManagerSyncTestGetEventByType (state, index+1, SYNC_EVENT_WALLET_MANAGER_TYPE))) {
            if (!BRWalletManagerEventTypeIsValidPair (state->events[index]->u.m.event.type, state->events[nextIndex]->u.m.event.type)) {
                success = 0;
                fprintf(stderr,
                        "***FAILED*** %s: BRWalletManagerEventTypeIsValidPair(%s, %s) test\n",
                        __func__,
                        BRWalletManagerEventTypeString (state->events[index]->u.m.event.type),
                        BRWalletManagerEventTypeString (state->events[nextIndex]->u.m.event.type));
                break;
            }
            index = nextIndex;
        }
    }

    // check that each wallet event pair is allowed
    {
        size_t index = BRRunTestWalletManagerSyncTestGetEventByType (state, 0, SYNC_EVENT_WALLET_TYPE);
        size_t nextIndex = 0;
        while (SIZE_MAX != (nextIndex = BRRunTestWalletManagerSyncTestGetEventByType (state, index+1, SYNC_EVENT_WALLET_TYPE))) {
            if (!BRWalletEventTypeIsValidPair (state->events[index]->u.w.event.type, state->events[nextIndex]->u.w.event.type)) {
                success = 0;
                fprintf(stderr,
                        "***FAILED*** %s: BRWalletEventTypeIsValidPair(%s, %s) test\n",
                        __func__,
                        BRWalletEventTypeString (state->events[index]->u.w.event.type),
                        BRWalletEventTypeString (state->events[nextIndex]->u.w.event.type));
                break;
            }
            index = nextIndex;
        }
    }

    pthread_mutex_unlock (&state->lock);
    return success;
}

static void
BRRunTestWalletManagerSyncTestTeardown (BRRunTestWalletManagerSyncState *state) {
    for (size_t index = 0; index < array_count(state->events); index++)
        free (state->events[index]);
    array_free (state->events);

    pthread_mutex_destroy (&state->lock);
}

static BRWalletManager
BRRunTestWalletManagerSyncBwmSetup (BRSyncMode mode,
                                    BRWalletManagerClientContext context,
                                    const char *paperKey,
                                    const char *storagePath,
                                    uint32_t earliestKeyTime,
                                    uint64_t blockHeight,
                                    int isBTC,
                                    int isMainnet)
{
    BRWalletManagerClient client = {
        context,
        _testGetBlockNumberNopCallback, _testGetTransactionsNopCallback, _testSubmitTransactionNopCallback,
        _testTransactionEventRecordingCallback, _testWalletEventRecordingCallback, _testWalletManagerEventRecordingCallback};

    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey (seed.u8, paperKey, NULL);
    BRMasterPubKey mpk = BRBIP32MasterPubKey(&seed, sizeof (seed));

    const BRChainParams *params = (isBTC & isMainnet ? BRMainNetParams
                                   : (isBTC & !isMainnet ? BRTestNetParams
                                      : (isMainnet ? BRBCashParams : BRBCashTestNetParams)));

    return BRWalletManagerNew (client, mpk, params, earliestKeyTime, mode, storagePath, blockHeight);
}

static int
BRRunTestWalletManagerSyncForMode (const char *funcName,
                                   BRSyncMode mode,
                                   const char *paperKey,
                                   const char *storagePath,
                                   uint32_t earliestKeyTime,
                                   uint64_t blockHeight,
                                   int isBTC,
                                   int isMainnet) {
    int success = 1;

    printf("%s testing BRWalletManager events for %s mode, %u epoch, %" PRIu64 " height on \"%s:%s\" with %s as storage...\n",
           funcName,
           BRSyncModeString (mode),
           earliestKeyTime,
           blockHeight,
           isBTC ? "btc": "bch",
           isMainnet ? "mainnet" : "testnet",
           storagePath);

    // Test
    {
        printf("Testing BRWalletManager connect/disconnect cycle...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // fast connect/disconnect cycle
        BRWalletManagerConnect (manager);
        BRWalletManagerDisconnect (manager);

        // slow connect/disconnect cycle
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequenceExact(&state,
                                                                         (BRWalletManagerSyncEvent []) {
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                             BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                             // fast event sequence
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                             // slow event sequence
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                         },
                                                                         10);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequenceExact failed\n", funcName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", funcName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    {
        printf("Testing BRWalletManager repeated connect attempts...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // fast repeated connect attempts
        BRWalletManagerConnect (manager);
        BRWalletManagerConnect (manager);
        BRWalletManagerConnect (manager);

        // slow repeated connect attempts
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        // Verification
        // TODO(fix): Inconsistency between P2P and BRD modes; is that OK?
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                    },
                                                                    6);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", funcName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", funcName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    {
        printf("Testing BRWalletManager repeated disconnect attempts...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // fast repeated disconnect attempts
        BRWalletManagerDisconnect (manager);
        BRWalletManagerDisconnect (manager);
        BRWalletManagerDisconnect (manager);

        // slow repeated disconnect attempts
        BRWalletManagerDisconnect (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequenceExact(&state,
                                                                         (BRWalletManagerSyncEvent []) {
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                             BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED)
                                                                         },
                                                                         2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequenceExact failed\n", funcName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", funcName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    {
        printf("Testing BRWalletManager connect, scan, disconnect...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // fast connect, scan, disconnect
        BRWalletManagerConnect (manager);
        BRWalletManagerScan (manager);
        BRWalletManagerDisconnect (manager);

        // slow connect, scan, disconnect
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerScan (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        // Verification
        // TODO(fix): Inconsistency between P2P and BRD modes; is that OK?
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                    },
                                                                    6);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", funcName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", funcName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    {
        printf("Testing BRWalletManager connect, scan (repeated), disconnect...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // fast connect, scan (repeated), disconnect
        BRWalletManagerConnect (manager);
        BRWalletManagerScan (manager);
        BRWalletManagerScan (manager);
        BRWalletManagerScan (manager);
        BRWalletManagerDisconnect (manager);

        // slow connect, scan (repeated), disconnect
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerScan (manager);
        sleep(1);
        BRWalletManagerScan (manager);
        sleep(1);
        BRWalletManagerScan (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        // Verification
        // TODO(fix): Inconsistency between P2P and BRD modes; is that OK?
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                    },
                                                                    6);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", funcName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", funcName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    {
        printf("Testing BRWalletManager scan, connect, disconnect...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // fast scan, connect, disconnect
        BRWalletManagerScan (manager);
        BRWalletManagerConnect (manager);
        BRWalletManagerDisconnect (manager);

        // slow scan, connect, disconnect
        BRWalletManagerScan (manager);
        sleep(1);
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        // Verification
        // TODO(fix): Inconsistency between P2P and BRD modes; is that OK?
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                    },
                                                                    6);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", funcName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", funcName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    {
        printf("Testing BRWalletManager connect, scan, connect, disconnect...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // fast connect, scan, connect, disconnect
        BRWalletManagerConnect (manager);
        BRWalletManagerScan (manager);
        BRWalletManagerConnect (manager);
        BRWalletManagerDisconnect (manager);

        // slow connect, scan, connect, disconnect
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerScan (manager);
        sleep(1);
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        // Verification
        // TODO(fix): Inconsistency between P2P and BRD modes; is that OK?
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                    },
                                                                    6);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", funcName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", funcName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    {
        printf("Testing BRWalletManager scan, disconnect...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // fast scan, disconnect
        BRWalletManagerScan (manager);
        BRWalletManagerDisconnect (manager);

        // slow scan, disconnect
        BRWalletManagerScan (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequenceExact(&state,
                                                                         (BRWalletManagerSyncEvent []) {
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                             BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                         },
                                                                         2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", funcName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", funcName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    {
        printf("Testing BRWalletManager threading...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetup (&state, blockHeight, 1);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        BRRunTestWalletManagerSyncThreadState threadState = {0, manager};
        pthread_t connectThread = (pthread_t) NULL, disconnectThread = (pthread_t) NULL, scanThread = (pthread_t) NULL;

        success = (0 == pthread_create (&connectThread, NULL, _testBRWalletManagerConnectThread, (void*) &threadState) &&
                   0 == pthread_create (&disconnectThread, NULL, _testBRWalletManagerDisconnectThread, (void*) &threadState) &&
                   0 == pthread_create (&scanThread, NULL, _testBRWalletManagerScanThread, (void*) &threadState));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: pthread_creates failed\n", funcName, __LINE__);
            return success;
        }

        sleep (1);

        threadState.kill = 1;
        success = (0 == pthread_join (connectThread, NULL) &&
                   0 == pthread_join (disconnectThread, NULL) &&
                   0 == pthread_join (scanThread, NULL));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: pthread_joins failed\n", funcName, __LINE__);
            return success;
        }

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED)},
                                                                    2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", funcName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", funcName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    return success;
}

static int
BRRunTestWalletManagerSyncAllModes (const char *testName,
                                    BRSyncMode primaryMode,
                                    BRSyncMode secondaryMode,
                                    const char *paperKey,
                                    const char *storagePath,
                                    uint32_t earliestKeyTime,
                                    uint64_t blockHeight,
                                    int isBTC,
                                    int isMainnet) {
    int success = 1;

    printf("%s testing BRWalletManager events for %s -> %s modes, %u epoch, %" PRIu64 " height on \"%s:%s\" with %s as storage...\n",
           testName,
           BRSyncModeString (primaryMode),
           BRSyncModeString (secondaryMode),
           earliestKeyTime,
           blockHeight,
           isBTC ? "btc": "bch",
           isMainnet ? "mainnet" : "testnet",
           storagePath);

    // Test
    {
        printf("Testing BRWalletManager mode swap while disconnected...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (primaryMode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // swap modes while disconnected
        BRWalletManagerSetMode (manager, primaryMode);
        sleep (1);
        BRWalletManagerSetMode (manager, secondaryMode);
        sleep (1);
        BRWalletManagerSetMode (manager, primaryMode);
        sleep (1);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequenceExact(&state,
                                                                         (BRWalletManagerSyncEvent []) {
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                             BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED)
                                                                         },
                                                                         2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequenceExact failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    // Test
    {
        printf("Testing BRWalletManager mode swap while connected...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (primaryMode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // swap modes while connected
        BRWalletManagerConnect (manager);
        sleep (1);
        BRWalletManagerSetMode (manager, secondaryMode);
        BRWalletManagerConnect (manager);
        sleep (1);
        BRWalletManagerDisconnect (manager);
        sleep (1);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequenceExact(&state,
                                                                         (BRWalletManagerSyncEvent []) {
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                             BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                         },
                                                                         10);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequenceExact failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    // Test
    {
        printf("Testing BRWalletManager mode swap while scanning...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (primaryMode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // swap modes while scanning
        BRWalletManagerScan (manager);
        sleep (1);
        BRWalletManagerSetMode (manager, secondaryMode);
        BRWalletManagerScan (manager);
        sleep (1);
        BRWalletManagerDisconnect (manager);
        sleep (1);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequenceExact(&state,
                                                                         (BRWalletManagerSyncEvent []) {
                                                                             BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                             BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                         },
                                                                         2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequenceExact failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    {
        printf("Testing BRWalletManager mode swap threading...\n");

        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetup (&state, blockHeight, 1);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (primaryMode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        BRRunTestWalletManagerSyncThreadState threadState = {0, manager};
        pthread_t connectThread = (pthread_t) NULL, disconnectThread = (pthread_t) NULL, scanThread = (pthread_t) NULL, swapThread = (pthread_t) NULL;

        success = (0 == pthread_create (&connectThread, NULL, _testBRWalletManagerConnectThread, (void*) &threadState) &&
                   0 == pthread_create (&disconnectThread, NULL, _testBRWalletManagerDisconnectThread, (void*) &threadState) &&
                   0 == pthread_create (&scanThread, NULL, _testBRWalletManagerScanThread, (void*) &threadState) &&
                   0 == pthread_create (&swapThread, NULL, _testBRWalletManagerSwapThread, (void*) &threadState));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: pthread_creates failed\n", testName, __LINE__);
            return success;
        }

        sleep (1);

        threadState.kill = 1;
        success = (0 == pthread_join (connectThread, NULL) &&
                   0 == pthread_join (disconnectThread, NULL) &&
                   0 == pthread_join (scanThread, NULL) &&
                   0 == pthread_join (swapThread, NULL));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: pthread_joins failed\n", testName, __LINE__);
            return success;
        }

        // Verification
        success = BRRunTestWalletManagerSyncTestVerification (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerification failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    return success;
}

extern int BRRunTestWalletManagerSyncStress (const char *paperKey,
                                             const char *storagePath,
                                             uint32_t earliestKeyTime,
                                             uint64_t blockHeight,
                                             int isBTC,
                                             int isMainnet) {
    int success = 1;

    {
        success = BRRunTestWalletManagerSyncForMode("BRRunTestWalletManagerSyncP2P",
                                                    SYNC_MODE_P2P_ONLY,
                                                    paperKey,
                                                    storagePath,
                                                    earliestKeyTime,
                                                    blockHeight,
                                                    isBTC,
                                                    isMainnet);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: P2P sync failed\n", __func__, __LINE__);
            return success;
        }
    }

    {
        success = BRRunTestWalletManagerSyncForMode("BRRunTestWalletManagerSyncBRD",
                                                    SYNC_MODE_BRD_ONLY,
                                                    paperKey,
                                                    storagePath,
                                                    earliestKeyTime,
                                                    blockHeight,
                                                    isBTC,
                                                    isMainnet);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: API sync failed\n", __func__, __LINE__);
            return success;
        }
    }

    {
        success = BRRunTestWalletManagerSyncAllModes("BRRunTestWalletManagerSyncP2PtoBRD",
                                                     SYNC_MODE_P2P_ONLY,
                                                     SYNC_MODE_BRD_ONLY,
                                                     paperKey,
                                                     storagePath,
                                                     earliestKeyTime,
                                                     blockHeight,
                                                     isBTC,
                                                     isMainnet);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: swapping modes sync failed\n", __func__, __LINE__);
            return success;
        }
    }

    {
        success = BRRunTestWalletManagerSyncAllModes("BRRunTestWalletManagerSyncBRDtoP2P",
                                                     SYNC_MODE_BRD_ONLY,
                                                     SYNC_MODE_P2P_ONLY,
                                                     paperKey,
                                                     storagePath,
                                                     earliestKeyTime,
                                                     blockHeight,
                                                     isBTC,
                                                     isMainnet);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: swapping modes sync failed\n", __func__, __LINE__);
            return success;
        }
    }

    return success;
}
