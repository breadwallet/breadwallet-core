//
//  test.c
//
//  Created by Michael Carrara on 8/18/19.
//  Copyright (c) 2019 breadwallet LLC
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bcash/BRBCashParams.h"
#include "bitcoin/BRChainParams.h"

#include "support/BRArray.h"
#include "support/BRBIP39Mnemonic.h"
#include "bitcoin/BRPeerManager.h"
#include "bitcoin/BRTransaction.h"
#include "bitcoin/BRWallet.h"
#include "bitcoin/BRWalletManager.h"

#ifdef __ANDROID__
#include <android/log.h>
#define fprintf(...) __android_log_print(ANDROID_LOG_ERROR, "testBwm", _va_rest(__VA_ARGS__, NULL))
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "testBwm", __VA_ARGS__)
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

    BRCryptoSyncMode mode = CRYPTO_SYNC_MODE_P2P_ONLY;

    BRWalletManager manager = BRWalletManagerNew (client, mpk, params, epoch, mode, storagePath, 0, 6);

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

static int
BRWalletManagerSyncEventEqual (BRWalletManagerSyncEvent *e1, BRWalletManagerSyncEvent *e2) {
    int success = 1;

    if (e1->type != e2->type) {
        success = 0;
    }

    if (success) {
        switch (e1->type) {
            case SYNC_EVENT_WALLET_MANAGER_TYPE: {
                if (e1->u.m.event.type != e2->u.m.event.type) {
                    success = 0;
                }
                break;
            }
            case SYNC_EVENT_WALLET_TYPE: {
                if (e1->u.w.event.type != e2->u.w.event.type) {
                    success = 0;
                }
                break;
            }
            case SYNC_EVENT_TXN_TYPE: {
                if (e1->u.t.event.type != e2->u.t.event.type) {
                    success = 0;
                }
                break;
            }
        }
    }

    return success;
}

static char *
BRWalletManagerSyncEventString (BRWalletManagerSyncEvent *e) {
    const char * subtypeString = NULL;
    const char * typeString = BRWalletManagerSyncEventTypeString (e->type);

    switch (e->type) {
        case SYNC_EVENT_WALLET_MANAGER_TYPE:
        subtypeString = BRWalletManagerEventTypeString (e->u.m.event.type);
        break;
        case SYNC_EVENT_WALLET_TYPE:
        subtypeString = BRWalletEventTypeString (e->u.w.event.type);
        break;
        case SYNC_EVENT_TXN_TYPE:
        subtypeString = BRTransactionEventTypeString (e->u.t.event.type);
        break;
    }

    const char * fmtString = "BRWalletManagerSyncEventString(%s -> %s)";
    size_t fmtStringLength = strlen (fmtString);

    size_t eventStringLength = fmtStringLength + strlen(typeString)  + strlen(subtypeString) + 1;
    char * eventString = calloc (eventStringLength, sizeof(char));

    snprintf (eventString, eventStringLength, fmtString, typeString, subtypeString);
    return eventString;
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
            case CRYPTO_SYNC_MODE_API_ONLY:
                BRWalletManagerSetMode (state->manager, CRYPTO_SYNC_MODE_P2P_ONLY);
                break;
            case CRYPTO_SYNC_MODE_P2P_ONLY:
                BRWalletManagerSetMode (state->manager, CRYPTO_SYNC_MODE_API_ONLY);
                break;
            default:
                break;
        }
    }
    return NULL;
}

static size_t
BRRunTestWalletManagerSyncTestGetEventByType(BRRunTestWalletManagerSyncState *state,
                                             size_t startIndex,
                                             BRWalletManagerSyncEventType type) {
    for (size_t index = startIndex; index < array_count(state->events); index++) {
        if (state->events[index]->type == type) {
            return index;
        }
    }
    return SIZE_MAX;
}


static void
BRRunTestWalletManagerSyncTestSetup (BRRunTestWalletManagerSyncState *state,
                                     uint64_t blockHeight,
                                     uint8_t isSilent) {
    state->blockHeight = blockHeight;
    state->silent = isSilent;
    array_new (state->events, 100);
    pthread_mutex_init (&state->lock, NULL);
}

static void
BRRunTestWalletManagerSyncTestSetupAsDefault (BRRunTestWalletManagerSyncState *state,
                                              uint64_t blockHeight) {
    BRRunTestWalletManagerSyncTestSetup (state, blockHeight, 0);
}

static int
BRRunTestWalletManagerSyncTestVerifyEventSequence (BRRunTestWalletManagerSyncState *state,
                                                   uint8_t isCompleteSequence,
                                                   BRWalletManagerSyncEvent *expected,
                                                   size_t expectedCount,
                                                   BRWalletManagerSyncEvent *ignored,
                                                   size_t ignoredCount) {
    int success = 1;
    pthread_mutex_lock (&state->lock);

    size_t index = 0;
    size_t count = array_count (state->events);
    size_t expectedIndex = 0;

    for (; success && index < count; index++) {
        if (!isCompleteSequence && expectedIndex == expectedCount) {
            break;

        } else if ((success = (expectedIndex < expectedCount &&
                               BRWalletManagerSyncEventEqual (state->events[index], &expected[expectedIndex])))) {
            expectedIndex++;

        } else {
            for (size_t ignoredIndex = 0; !success && ignoredIndex < ignoredCount; ignoredIndex++) {
                success = BRWalletManagerSyncEventEqual (state->events[index], &ignored[ignoredIndex]);
            }
        }

        if (!success) {
            printf("%s: failed due to mismatched event types (expected at idx %zu -> %s, received at idx %zu -> %s)\n",
                   __func__,
                   expectedIndex,
                   BRWalletManagerSyncEventString (&expected[expectedIndex]),
                   index,
                   BRWalletManagerSyncEventString (state->events[index]));
        }
    }

    pthread_mutex_unlock (&state->lock);

    if (success &&
        isCompleteSequence && index != count && expectedIndex == expectedCount) {
        success = 0;
        printf("%s: failed due to reaching the end of expected events but more occurred\n",
               __func__);
    }

    if (success &&
        index == count && expectedIndex != expectedCount) {
        success = 0;
        printf("%s: failed due to reaching the end of occurred events but more expected\n",
               __func__);
    }

    return success;
}

static int
BRRunTestWalletManagerSyncTestVerifyEventPairs (BRRunTestWalletManagerSyncState *state) {
    int success = 1;
    pthread_mutex_lock (&state->lock);

    // check that each manager event pair is allowed
    {
        size_t index = BRRunTestWalletManagerSyncTestGetEventByType (state, 0, SYNC_EVENT_WALLET_MANAGER_TYPE);
        size_t nextIndex = 0;
        while (SIZE_MAX != (nextIndex = BRRunTestWalletManagerSyncTestGetEventByType (state, index+1, SYNC_EVENT_WALLET_MANAGER_TYPE))) {
            if (!BRWalletManagerEventTypeIsValidPair (state->events[index]->u.m.event.type, state->events[nextIndex]->u.m.event.type)) {
                success = 0;
                printf("%s: failed due to invalid wallet manager event pair (%s, %s) test\n",
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
                printf("%s: failed due to invalid wallet event pair (%s, %s) test\n",
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
BRRunTestWalletManagerSyncBwmSetup (BRCryptoSyncMode mode,
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

    const BRChainParams *params = NULL;
    if (isBTC) {
        params = isMainnet ? BRMainNetParams : BRTestNetParams;
    } else {
        params = isMainnet ? BRBCashParams : BRBCashTestNetParams;
    }
    return BRWalletManagerNew (client, mpk, params, earliestKeyTime, mode, storagePath, blockHeight, 6);
}

static int
BRRunTestWalletManagerSyncForMode (const char *testName,
                                   BRCryptoSyncMode mode,
                                   const char *paperKey,
                                   const char *storagePath,
                                   uint32_t earliestKeyTime,
                                   uint64_t blockHeight,
                                   int isBTC,
                                   int isMainnet) {
    int success = 1;

    printf("%s testing BRWalletManager events for %s mode, %u epoch, %" PRIu64 " height on \"%s:%s\" with %s as storage...\n",
           testName,
           cryptoSyncModeString (mode),
           earliestKeyTime,
           blockHeight,
           isBTC ? "btc": "bch",
           isMainnet ? "mainnet" : "testnet",
           storagePath);

    printf("Testing BRWalletManager connect, disconnect...\n");
    {
        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // connect, disconnect cycle
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    1,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                    },
                                                                    6,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_PROGRESS),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED),
                                                                    },
                                                                    2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    printf("Testing BRWalletManager repeated connect attempts...\n");
    {
        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // repeated connect attempts
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    1,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                    },
                                                                    6,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_PROGRESS),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED),
                                                                    },
                                                                    2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    printf("Testing BRWalletManager repeated disconnect attempts...\n");
    {
        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // repeated disconnect attempts
        BRWalletManagerDisconnect (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    1,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED)
                                                                    },
                                                                    2,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_PROGRESS),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED),
                                                                    },
                                                                    2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    printf("Testing BRWalletManager connect, scan, disconnect...\n");
    {
        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // connect, scan, disconnect
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerScan (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        // Verification
        // multiple cases accepted as with P2P, depending on peer connections, could see:
        //      - a reconnect, if the BRWalletManagerScan was after the P2P connection and after receiving blocks (case 1)
        //      - a sync restart, if the BRWalletManagerScan was after the P2P connection but before the sync received blocks (case 2)
        //      - a sync, if the BRWalletManagerScan beat the P2P connections being established (case 3)
        success = (BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                     1,
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
                                                                     10,
                                                                     (BRWalletManagerSyncEvent []) {
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_PROGRESS),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED),
                                                                     },
                                                                     2) ||
                   BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                     1,
                                                                     (BRWalletManagerSyncEvent []) {
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                         BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                     },
                                                                     8,
                                                                     (BRWalletManagerSyncEvent []) {
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_PROGRESS),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED),
                                                                     },
                                                                     2) ||
                   BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                     1,
                                                                     (BRWalletManagerSyncEvent []) {
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                         BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                     },
                                                                     6,
                                                                     (BRWalletManagerSyncEvent []) {
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_PROGRESS),
                                                                         BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED),
                                                                     },
                                                                     2));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    printf("Testing BRWalletManager scan, connect, disconnect...\n");
    {
        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // scan, connect, disconnect
        BRWalletManagerScan (manager);
        sleep(1);
        BRWalletManagerConnect (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    1,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                    },
                                                                    6,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_PROGRESS),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED),
                                                                    },
                                                                    2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    printf("Testing BRWalletManager scan, disconnect...\n");
    {
        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (mode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // scan, disconnect
        BRWalletManagerScan (manager);
        sleep(1);
        BRWalletManagerDisconnect (manager);
        sleep(1);

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    1,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                    },
                                                                    2,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_PROGRESS),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED),
                                                                    },
                                                                    2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    printf("Testing BRWalletManager threading...\n");
    if (mode == CRYPTO_SYNC_MODE_P2P_ONLY) {
        // TODO(fix): There is a thread-related issue in BRPeerManager/BRPeer where we have a use after free; re-enable once that is fixed
        fprintf(stderr, "***WARNING*** %s:%d: BRWalletManager threading test is disabled for CRYPTO_SYNC_MODE_P2P_ONLY\n", testName, __LINE__);

    } else {
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
            fprintf(stderr, "***FAILED*** %s:%d: pthread_creates failed\n", testName, __LINE__);
            return success;
        }

        sleep (5);

        threadState.kill = 1;
        success = (0 == pthread_join (connectThread, NULL) &&
                   0 == pthread_join (disconnectThread, NULL) &&
                   0 == pthread_join (scanThread, NULL));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: pthread_joins failed\n", testName, __LINE__);
            return success;
        }

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    0,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED)},
                                                                    2,
                                                                    NULL,
                                                                    0);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    return success;
}

static int
BRRunTestWalletManagerSyncAllModes (const char *testName,
                                    BRCryptoSyncMode primaryMode,
                                    BRCryptoSyncMode secondaryMode,
                                    const char *paperKey,
                                    const char *storagePath,
                                    uint32_t earliestKeyTime,
                                    uint64_t blockHeight,
                                    int isBTC,
                                    int isMainnet) {
    int success = 1;

    printf("%s testing BRWalletManager events for %s -> %s modes, %u epoch, %" PRIu64 " height on \"%s:%s\" with %s as storage...\n",
           testName,
           cryptoSyncModeString (primaryMode),
           cryptoSyncModeString (secondaryMode),
           earliestKeyTime,
           blockHeight,
           isBTC ? "btc": "bch",
           isMainnet ? "mainnet" : "testnet",
           storagePath);

    printf("Testing BRWalletManager mode swap while disconnected...\n");
    {
        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (primaryMode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // swap modes while disconnected

        BRWalletManagerSetMode (manager, secondaryMode);
        success = BRWalletManagerGetMode (manager) == secondaryMode;
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRWalletManagerSetMode failed\n", testName, __LINE__);
            return success;
        }
        sleep (1);

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    1,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED)
                                                                    },
                                                                    2,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_PROGRESS),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED),
                                                                    },
                                                                    2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    printf("Testing BRWalletManager mode swap while connected...\n");
    {
        // Test setup
        BRRunTestWalletManagerSyncState state = {0};
        BRRunTestWalletManagerSyncTestSetupAsDefault (&state, blockHeight);

        BRWalletManager manager = BRRunTestWalletManagerSyncBwmSetup (primaryMode, &state, paperKey, storagePath, earliestKeyTime, blockHeight, isBTC, isMainnet);
        BRWalletManagerStart (manager);

        // swap modes while connected
        BRWalletManagerConnect (manager);
        sleep (1);

        BRWalletManagerSetMode (manager, secondaryMode);
        success = BRWalletManagerGetMode (manager) == secondaryMode;
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRWalletManagerSetMode failed\n", testName, __LINE__);
            return success;
        }
        sleep (1);

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    1,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CONNECTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STARTED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_STOPPED),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_DISCONNECTED),
                                                                    },
                                                                    6,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_SYNC_PROGRESS),
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED),
                                                                    },
                                                                    2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

        BRRunTestWalletManagerSyncTestTeardown (&state);
    }

    printf("Testing BRWalletManager mode swap threading...\n");
    if (primaryMode == CRYPTO_SYNC_MODE_P2P_ONLY || secondaryMode == CRYPTO_SYNC_MODE_P2P_ONLY) {
        // TODO(fix): There is a thread-related issue in BRPeerManager/BRPeer where we have a use after free; re-enable once that is fixed
        fprintf(stderr, "***WARNING*** %s:%d: BRWalletManager mode swap threading test is disabled\n", testName, __LINE__);

    } else {
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

        sleep (5);

        threadState.kill = 1;
        success = (0 == pthread_join (connectThread, NULL) &&
                   0 == pthread_join (disconnectThread, NULL) &&
                   0 == pthread_join (scanThread, NULL) &&
                   0 == pthread_join (swapThread, NULL));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: pthread_joins failed\n", testName, __LINE__);
            return success;
        }

        BRWalletManagerStop (manager);
        BRWalletManagerFree (manager);

        // Verification
        success = BRRunTestWalletManagerSyncTestVerifyEventSequence(&state,
                                                                    0,
                                                                    (BRWalletManagerSyncEvent []) {
                                                                        BRWalletManagerSyncEventForWalletManagerType (BITCOIN_WALLET_MANAGER_CREATED),
                                                                        BRWalletManagerSyncEventForWalletType(BITCOIN_WALLET_CREATED)},
                                                                    2,
                                                                    NULL,
                                                                    0);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", testName, __LINE__);
            return success;
        }

        success = BRRunTestWalletManagerSyncTestVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", testName, __LINE__);
            return success;
        }

        // Test teardown

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
                                                    CRYPTO_SYNC_MODE_P2P_ONLY,
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
                                                    CRYPTO_SYNC_MODE_API_ONLY,
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
                                                     CRYPTO_SYNC_MODE_P2P_ONLY,
                                                     CRYPTO_SYNC_MODE_API_ONLY,
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
                                                     CRYPTO_SYNC_MODE_API_ONLY,
                                                     CRYPTO_SYNC_MODE_P2P_ONLY,
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

/// MARK: File Service

typedef struct BRFileServiceTesterRecord {
    int ignore;
} *BRFileServiceTester;

static void
fileServiceErrorHandler (BRFileServiceContext context,
                         BRFileService fs,
                         BRFileServiceError error) {
    BRFileServiceTester fst = (BRFileServiceTester) context;
    // ...
}

extern BRFileServiceTypeSpecification fileServiceSpecifications[];
extern size_t fileServiceSpecificationsCount;

// Crib these from BRWalletManager.c
#define fileServiceTypeTransactions "transactions"
#define fileServiceTypeBlocks       "blocks"
#define fileServiceTypePeers        "peers"

static int
BRMerkleBlockEqual (const BRMerkleBlock *block1, const BRMerkleBlock *block2);

static int
BRTransactionEqual (BRTransaction *tx1, BRTransaction *tx2);

static int
BRPeerEqual (const BRPeer *p1, const BRPeer *p2);

static int
BRRunTestWalletManagerFileService (const char *storagePath) {
    BRFileServiceTester fst = calloc (1, sizeof (struct BRFileServiceTesterRecord));

    BRFileService fs = fileServiceCreateFromTypeSpecfications (storagePath, "btc", "mainnet",
                                                               fst,
                                                               fileServiceErrorHandler,
                                                               fileServiceSpecificationsCount,
                                                               fileServiceSpecifications);
    if (NULL == fs) return 0;

    ///
    /// Blocks
    ///
    BRSetOf(BRMerkleBlock*) blockSet = BRSetNew (BRMerkleBlockHash, BRMerkleBlockEq, 10);

    char block[] = // block 10001 filtered to include only transactions 0, 1, 2, and 6
    "\x01\x00\x00\x00\x06\xe5\x33\xfd\x1a\xda\x86\x39\x1f\x3f\x6c\x34\x32\x04\xb0\xd2\x78\xd4\xaa\xec\x1c"
    "\x0b\x20\xaa\x27\xba\x03\x00\x00\x00\x00\x00\x6a\xbb\xb3\xeb\x3d\x73\x3a\x9f\xe1\x89\x67\xfd\x7d\x4c\x11\x7e\x4c"
    "\xcb\xba\xc5\xbe\xc4\xd9\x10\xd9\x00\xb3\xae\x07\x93\xe7\x7f\x54\x24\x1b\x4d\x4c\x86\x04\x1b\x40\x89\xcc\x9b\x0c"
    "\x00\x00\x00\x08\x4c\x30\xb6\x3c\xfc\xdc\x2d\x35\xe3\x32\x94\x21\xb9\x80\x5e\xf0\xc6\x56\x5d\x35\x38\x1c\xa8\x57"
    "\x76\x2e\xa0\xb3\xa5\xa1\x28\xbb\xca\x50\x65\xff\x96\x17\xcb\xcb\xa4\x5e\xb2\x37\x26\xdf\x64\x98\xa9\xb9\xca\xfe"
    "\xd4\xf5\x4c\xba\xb9\xd2\x27\xb0\x03\x5d\xde\xfb\xbb\x15\xac\x1d\x57\xd0\x18\x2a\xae\xe6\x1c\x74\x74\x3a\x9c\x4f"
    "\x78\x58\x95\xe5\x63\x90\x9b\xaf\xec\x45\xc9\xa2\xb0\xff\x31\x81\xd7\x77\x06\xbe\x8b\x1d\xcc\x91\x11\x2e\xad\xa8"
    "\x6d\x42\x4e\x2d\x0a\x89\x07\xc3\x48\x8b\x6e\x44\xfd\xa5\xa7\x4a\x25\xcb\xc7\xd6\xbb\x4f\xa0\x42\x45\xf4\xac\x8a"
    "\x1a\x57\x1d\x55\x37\xea\xc2\x4a\xdc\xa1\x45\x4d\x65\xed\xa4\x46\x05\x54\x79\xaf\x6c\x6d\x4d\xd3\xc9\xab\x65\x84"
    "\x48\xc1\x0b\x69\x21\xb7\xa4\xce\x30\x21\xeb\x22\xed\x6b\xb6\xa7\xfd\xe1\xe5\xbc\xc4\xb1\xdb\x66\x15\xc6\xab\xc5"
    "\xca\x04\x21\x27\xbf\xaf\x9f\x44\xeb\xce\x29\xcb\x29\xc6\xdf\x9d\x05\xb4\x7f\x35\xb2\xed\xff\x4f\x00\x64\xb5\x78"
    "\xab\x74\x1f\xa7\x82\x76\x22\x26\x51\x20\x9f\xe1\xa2\xc4\xc0\xfa\x1c\x58\x51\x0a\xec\x8b\x09\x0d\xd1\xeb\x1f\x82"
    "\xf9\xd2\x61\xb8\x27\x3b\x52\x5b\x02\xff\x1a";

    // Confirm `block` is correct before checking FS
    BRMerkleBlock *b = BRMerkleBlockParse((uint8_t *)block, sizeof(block) - 1);
    if (NULL == b) return 0;

    if (1 != fileServiceSave (fs, fileServiceTypeBlocks, b)) return 0;

    BRSetClear(blockSet);
    if (1 != fileServiceLoad (fs, blockSet, fileServiceTypeBlocks, 1)) return 0;
    if (1 != BRSetCount(blockSet)) return 0;

    BRMerkleBlock *b2 = BRSetGet (blockSet, b);
    if (NULL == b2) return 0;

    if (1 != BRMerkleBlockEq (b, b2)) return 0;
    if (1 != BRMerkleBlockEqual (b, b2)) return 0;

    BRMerkleBlockFree(b);
    BRMerkleBlockFree(b2);
    BRSetFree (blockSet);
    ///
    /// Transaction
    ///
    BRSetOf(BRTransaction*) transactionSet = BRSetNew(BRTransactionHash, BRTransactionEq, 10);

    char transaction[] = "\x01\x00\x00\x00\x00\x01\x01\x7b\x03\x2f\x6a\x65\x1c\x7d\xcb\xcf\xb7\x8d\x81\x7b\x30\x3b\xe8\xd2\x0a"
     "\xfa\x22\x90\x16\x18\xb5\x17\xf2\x17\x55\xa7\xcd\x8d\x48\x01\x00\x00\x00\x23\x22\x00\x20\xe0\x62\x7b\x64\x74\x59"
     "\x05\x64\x6f\x27\x6f\x35\x55\x02\xa4\x05\x30\x58\xb6\x4e\xdb\xf2\x77\x11\x92\x49\x61\x1c\x98\xda\x41\x69\xff\xff"
     "\xff\xff\x02\x0c\xf9\x62\x01\x00\x00\x00\x00\x17\xa9\x14\x24\x31\x57\xd5\x78\xbd\x92\x8a\x92\xe0\x39\xe8\xd4\xdb"
     "\xbb\x29\x44\x16\x93\x5c\x87\xf3\xbe\x2a\x00\x00\x00\x00\x00\x19\x76\xa9\x14\x48\x38\x0b\xc7\x60\x5e\x91\xa3\x8f"
     "\x8d\x7b\xa0\x1a\x27\x95\x41\x6b\xf9\x2d\xde\x88\xac\x04\x00\x47\x30\x44\x02\x20\x5f\x5d\xe6\x88\x96\xca\x3e\xdf"
     "\x97\xe3\xea\x1f\xd3\x51\x39\x03\x53\x7f\xd5\xf2\xe0\xb3\x66\x1d\x6c\x61\x7b\x1c\x48\xfc\x69\xe1\x02\x20\x0e\x0f"
     "\x20\x59\x51\x3b\xe9\x31\x83\x92\x9c\x7d\x3e\x2d\xe0\xe9\xc7\x08\x57\x06\xa8\x8e\x8f\x74\x6e\x8f\x5a\xa7\x13\xd2"
     "\x7a\x52\x01\x47\x30\x44\x02\x20\x50\xd8\xec\xb9\xcd\x7f\xda\xcb\x6d\x63\x51\xde\xc2\xbc\x5b\x37\x16\x32\x8e\xf2"
     "\xc4\x46\x6d\xb4\x4b\xdd\x34\xa6\x57\x29\x2b\x8c\x02\x20\x68\x50\x1b\xf8\x18\x12\xad\x8e\x3e\xd9\xdf\x24\x35\x4c"
     "\x37\x19\x23\xa0\x7d\xc9\x66\xa6\xe4\x14\x63\x59\x47\x74\xd0\x09\x16\x9e\x01\x69\x52\x21\x03\xb8\xe1\x38\xed\x70"
     "\x23\x2c\x9c\xbd\x1b\x90\x28\x12\x10\x64\x23\x6a\xf1\x2d\xbe\x98\x64\x1c\x3f\x74\xfa\x13\x16\x6f\x27\x2f\x58\x21"
     "\x03\xf6\x6e\xe7\xc8\x78\x17\xd3\x24\x92\x1e\xdc\x3f\x7d\x77\x26\xde\x5a\x18\xcf\xed\x05\x7e\x5a\x50\xe7\xc7\x4e"
     "\x2a\xe7\xe0\x5a\xd7\x21\x02\xa7\xbf\x21\x58\x2d\x71\xe5\xda\x5c\x3b\xc4\x3e\x84\xc8\x8f\xdf\x32\x80\x3a\xa4\x72"
     "\x0e\x1c\x1a\x9d\x08\xaa\xb5\x41\xa4\xf3\x31\x53\xae\x00\x00\x00\x00";

    BRTransaction *tx = BRTransactionParse((uint8_t *)transaction, sizeof(transaction) - 1);
    if (NULL == tx) return 0;

    if (1 != fileServiceSave (fs, fileServiceTypeTransactions, tx)) return 0;

    BRSetClear(transactionSet);
    if (1 != fileServiceLoad (fs, transactionSet, fileServiceTypeTransactions, 1)) return 0;
    if (1 != BRSetCount(transactionSet)) return 0;

    BRTransaction *tx2 = BRSetGet (transactionSet, tx);
    if (NULL == tx2) return 0;

    if (1 != BRTransactionEq (tx, tx2)) return 0;
    if (1 != BRTransactionEqual (tx, tx2)) return 0;

    BRTransactionFree(tx);
    BRTransactionFree(tx2);
    BRSetFree (transactionSet);
    
    ///
    /// Peer
    ///
    BRSetOf(BRPeer*) peerSet = BRSetNew(BRPeerHash, BRPeerEq, 100);

    time_t now = time(NULL);

    BRPeer pFull = ((const BRPeer) { UINT128_ZERO, 1111, 0xdeadbeef, now, 3 });
    BRPeer *p = &pFull;
    if (NULL == p) return 0;

    if (1 != fileServiceSave (fs, fileServiceTypePeers, p)) return 0;

    BRSetClear (peerSet);
    if (1 != fileServiceLoad (fs, peerSet, fileServiceTypePeers, 1)) return 0;
    if (1 != BRSetCount(peerSet)) return 0;

    BRPeer *p2 = BRSetGet (peerSet, p);
    if (NULL == p2) return 0;

    if (1 != BRPeerEq (p, p2)) return 0;
    if (1 != BRPeerEqual (p, p2)) return 0;

    free(p2);
    BRSetFree(peerSet);
    
    fileServiceClose(fs);
    fileServiceRelease(fs);

    return 1;
}

static int
BRMerkleBlockEqual (const BRMerkleBlock *block1, const BRMerkleBlock *block2) {
    return 0 == memcmp(&block1->blockHash, &block2->blockHash, sizeof(UInt256))
           && block1->version == block2->version
           && 0 == memcmp(&block1->prevBlock, &block2->prevBlock, sizeof(UInt256))
           && 0 == memcmp(&block1->merkleRoot, &block2->merkleRoot, sizeof(UInt256))
           && block1->timestamp == block2->timestamp
           && block1->target == block2->target
           && block1->nonce == block2->nonce
           && block1->totalTx == block2->totalTx
           && block1->hashesCount == block2->hashesCount
           && 0 == memcmp(block1->hashes, block2->hashes, block1->hashesCount * sizeof(UInt256))
           && block1->flagsLen == block2->flagsLen
           && 0 == memcmp(block1->flags, block2->flags, block1->flagsLen * sizeof(uint8_t))
           && block1->height == block2->height;
}

static int BRTxOutputEqual(BRTxOutput *out1, BRTxOutput *out2) {
    return out1->amount == out2->amount
           && out1->scriptLen == out2->scriptLen
           && 0 == memcmp (out1->script, out2->script, out1->scriptLen * sizeof (uint8_t));
}


//
static int BRTxInputEqual(BRTxInput *in1, BRTxInput *in2) {
    return 0 == memcmp(&in1->txHash, &in2->txHash, sizeof(UInt256))
           && in1->index == in2->index
           && in1->amount == in2->amount
           && in1->scriptLen == in2->scriptLen
           && 0 == memcmp(in1->script, in2->script, in1->scriptLen * sizeof(uint8_t))
           && in1->sigLen == in2->sigLen
           && 0 == memcmp(in1->signature, in2->signature, in1->sigLen * sizeof(uint8_t))
           && in1->sequence == in2->sequence;
}

static int BRTransactionEqual (BRTransaction *tx1, BRTransaction *tx2) {
    if (memcmp (&tx1->txHash, &tx2->txHash, sizeof(UInt256))
        || tx1->version != tx2->version
        || tx1->lockTime != tx2->lockTime
        || tx1->blockHeight != tx2->blockHeight
        || tx1->timestamp != tx2->timestamp
        || tx1->inCount != tx2->inCount
        || tx1->outCount != tx2->outCount)
        return 0;

    // Inputs
    if (NULL != tx1->inputs)
        for (int i = 0; i < tx1->inCount; i++)
            if (!BRTxInputEqual(&tx1->inputs[i], &tx2->inputs[i]))
                return 0;
    // Outputs
    if (NULL != tx1->outputs)
        for (int i = 0; i < tx1->outCount; i++)
            if (!BRTxOutputEqual(&tx1->outputs[i], &tx2->outputs[i]))
                return 0;

    return 1;
}

static int BRPeerEqual (const BRPeer *p1, const BRPeer *p2) {
    return (1 == UInt128Eq (p1->address, p2->address) &&
            p1->port == p2->port &&
            p1->services == p2->services &&
            p1->timestamp == p2->timestamp &&
            p1->flags == p2->flags);
}

extern int BRRunTestsBWM (const char *paperKey,
                          const char *storagePath,
                          int isBTC,
                          int isMainnet) {
    int success = 1;

    success &= BRRunTestWalletManagerFileService (storagePath);

    return success;
}
