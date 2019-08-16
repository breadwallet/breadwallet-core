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

//
// Helper Routine
//

static const char *
_BRSyncModeString (BRSyncMode m) {
    switch (m) {
        case SYNC_MODE_BRD_ONLY:
        return "SYNC_MODE_BRD_ONLY";
        case SYNC_MODE_BRD_WITH_P2P_SEND:
        return "SYNC_MODE_BRD_WITH_P2P_SEND";
        case SYNC_MODE_P2P_WITH_BRD_SYNC:
        return "SYNC_MODE_P2P_WITH_BRD_SYNC";
        case SYNC_MODE_P2P_ONLY:
        return "SYNC_MODE_P2P_ONLY";
    }
}

static int
_isValidWalletManagerEventPair (BRWalletManagerEvent *e1, BRWalletManagerEvent *e2) {
    int isValid = 0;
    switch (e1->type) {
        case BITCOIN_WALLET_MANAGER_CREATED:
            switch (e2->type) {
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_MANAGER_CONNECTED:
            switch (e2->type) {
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_MANAGER_DISCONNECTED:
            switch (e2->type) {
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
            switch (e2->type) {
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
            switch (e2->type) {
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
            switch (e2->type) {
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
            switch (e2->type) {
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                isValid = 0;
                break;
            }
        break;
    }
    return isValid;
}

static int
_isValidWalletEventPair (BRWalletEvent *e1, BRWalletEvent *e2) {
    int isValid = 0;
    switch (e1->type) {
        case BITCOIN_WALLET_CREATED:
            switch (e2->type) {
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
                case BITCOIN_WALLET_FEE_ESTIMATED:
                case BITCOIN_WALLET_DELETED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_CREATED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_BALANCE_UPDATED:
            switch (e2->type) {
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
                case BITCOIN_WALLET_FEE_ESTIMATED:
                case BITCOIN_WALLET_DELETED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_CREATED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
            switch (e2->type) {
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
                case BITCOIN_WALLET_FEE_ESTIMATED:
                case BITCOIN_WALLET_DELETED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_CREATED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
            switch (e2->type) {
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
                case BITCOIN_WALLET_FEE_ESTIMATED:
                case BITCOIN_WALLET_DELETED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_CREATED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_FEE_ESTIMATED:
            switch (e2->type) {
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
                case BITCOIN_WALLET_FEE_ESTIMATED:
                case BITCOIN_WALLET_DELETED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_CREATED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_DELETED:
            switch (e2->type) {
                case BITCOIN_WALLET_CREATED:
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
                case BITCOIN_WALLET_FEE_ESTIMATED:
                case BITCOIN_WALLET_DELETED:
                isValid = 0;
                break;
            }
        break;
    }
    return isValid;
}

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

struct BRWalletManagerEventRecord {
    size_t sequenceId;
    BRWalletManager manager;
    BRWalletManagerEvent event;
};

struct BRWalletEventRecord {
    size_t sequenceId;
    BRWalletManager manager;
    BRWallet *wallet;
    BRWalletEvent event;
};

struct BRTransactionEventRecord {
    size_t sequenceId;
    BRWalletManager manager;
    BRWallet *wallet;
    BRTransaction *transaction;
    BRTransactionEvent event;
} ;

typedef struct {
    uint64_t blockHeight;
    size_t sequenceIdGenerator;
    BRArrayOf(struct BRWalletManagerEventRecord *) managerEvents;
    BRArrayOf(struct BRWalletEventRecord *) walletEvents;
    BRArrayOf(struct BRTransactionEventRecord *) transactionEvents;
    pthread_mutex_t lock;
} BRRunTestWalletManagerSyncState;

static void
_testGetBlockNumberNopCallback (BRWalletManagerClientContext context,
                                BRWalletManager manager,
                                int rid) {
    BRRunTestWalletManagerSyncState *state = (BRRunTestWalletManagerSyncState*) context;
    bwmAnnounceBlockNumber (manager, rid, state->blockHeight);
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
    struct BRTransactionEventRecord *record = calloc (1, sizeof (struct BRTransactionEventRecord));
    record->manager= manager;
    record->wallet= wallet;
    record->transaction = transaction;
    record->event = event;

    pthread_mutex_lock (&state->lock);
    record->sequenceId = state->sequenceIdGenerator++;
    array_add (state->transactionEvents, record);
    pthread_mutex_unlock (&state->lock);
}

static void
_testWalletEventRecordingCallback (BRWalletManagerClientContext context,
                                   BRWalletManager manager,
                                   BRWallet *wallet,
                                   BRWalletEvent event) {
    BRRunTestWalletManagerSyncState *state = (BRRunTestWalletManagerSyncState*) context;
    struct BRWalletEventRecord *record = calloc (1, sizeof (struct BRWalletEventRecord));
    record->manager= manager;
    record->wallet= wallet;
    record->event = event;

    pthread_mutex_lock (&state->lock);
    record->sequenceId = state->sequenceIdGenerator++;
    array_add (state->walletEvents, record);
    pthread_mutex_unlock (&state->lock);
}

static void
_testWalletManagerEventRecordingCallback (BRWalletManagerClientContext context,
                                          BRWalletManager manager,
                                          BRWalletManagerEvent event) {
    BRRunTestWalletManagerSyncState *state = (BRRunTestWalletManagerSyncState*) context;
    struct BRWalletManagerEventRecord *record = calloc (1, sizeof (struct BRWalletManagerEventRecord));
    record->manager= manager;
    record->event = event;

    pthread_mutex_lock (&state->lock);
    record->sequenceId = state->sequenceIdGenerator++;
    array_add (state->managerEvents, record);
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

static void
BRRunTestWalletManagerSyncTestSetup (BRRunTestWalletManagerSyncState *state, uint64_t blockHeight) {
    state->blockHeight = blockHeight;
    state->sequenceIdGenerator = 0;
    array_new (state->managerEvents, 100);
    array_new (state->walletEvents, 100);
    array_new (state->transactionEvents, 100);
    pthread_mutex_init (&state->lock, NULL);
}

static int
BRRunTestWalletManagerSyncTestVerification (BRRunTestWalletManagerSyncState *state) {
    int success = 1;
    pthread_mutex_lock (&state->lock);

    // WalletManagerEvent checks

    // check for presence of wallet events
    if (success &&
        array_count(state->managerEvents) == 0) {
        success = 0;
        fprintf(stderr,
                "***FAILED*** %s: no manager events\n",
                __func__);
    }

    // check that each event pair is allowed
    for (size_t index = 0; success && array_count(state->managerEvents) > 1 && index < array_count(state->managerEvents) - 1; index++) {
        if (!_isValidWalletManagerEventPair (&state->managerEvents[index]->event, &state->managerEvents[index+1]->event)) {
            success = 0;
            fprintf(stderr,
                    "***FAILED*** %s: _isValidWalletManagerEventPair(%s, %s) test\n",
                    __func__,
                    BRWalletManagerEventTypeString (state->managerEvents[index]->event.type),
                    BRWalletManagerEventTypeString (state->managerEvents[index+1]->event.type));
            break;
        }
    }

    // WalletEvent checks

    // check for presence of wallet events
    if (success &&
        array_count(state->walletEvents) == 0) {
        success = 0;
        fprintf(stderr,
                "***FAILED*** %s: no wallet events\n",
                __func__);
    }

    // check that each event pair is allowed
    for (size_t index = 0; success && array_count(state->walletEvents) > 1 && index < array_count(state->walletEvents) - 1; index++) {
        if (!_isValidWalletEventPair (&state->walletEvents[index]->event, &state->walletEvents[index+1]->event)) {
            success = 0;
            fprintf(stderr,
                    "***FAILED*** %s: _isValidWalletEventPair(%s, %s) test\n",
                    __func__,
                    BRWalletEventTypeString (state->walletEvents[index]->event.type),
                    BRWalletEventTypeString (state->walletEvents[index+1]->event.type));
            break;
        }
    }

    // Sequencing checks

    // check that second event received is BITCOIN_WALLET_MANAGER_CREATED
    {
        if (success &&
            state->managerEvents[0]->sequenceId != 0) {
            success = 0;
            fprintf(stderr,
                    "***FAILED*** %s: first event is not manager event\n",
                    __func__);
        }

        if (success &&
            state->managerEvents[0]->event.type != BITCOIN_WALLET_MANAGER_CREATED) {
            success = 0;
            fprintf(stderr,
                    "***FAILED*** %s: first manager event is not BITCOIN_WALLET_MANAGER_CREATED\n",
                    __func__);
        }
    }

    // check that second event received is BITCOIN_WALLET_CREATED
    {
        if (success &&
            state->walletEvents[0]->sequenceId != 1) {
            success = 0;
            fprintf(stderr,
                    "***FAILED*** %s: second event is not wallet event\n",
                    __func__);
        }

        if (success &&
            state->walletEvents[0]->event.type != BITCOIN_WALLET_CREATED) {
            success = 0;
            fprintf(stderr,
                    "***FAILED*** %s: first wallet event is not BITCOIN_WALLET_CREATED\n",
                    __func__);
        }
    }

    pthread_mutex_unlock (&state->lock);
    return success;
}

static void
BRRunTestWalletManagerSyncTestTeardown (BRRunTestWalletManagerSyncState *state) {
    for (size_t index = 0; index < array_count(state->managerEvents); index++)
        free (state->managerEvents[index]);
    array_free (state->managerEvents);

    for (size_t index = 0; index < array_count(state->walletEvents); index++)
        free (state->walletEvents[index]);
    array_free (state->walletEvents);

    for (size_t index = 0; index < array_count(state->transactionEvents); index++)
        free (state->transactionEvents[index]);
    array_free (state->transactionEvents);

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
           _BRSyncModeString (mode),
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
        BRRunTestWalletManagerSyncTestSetup (&state, blockHeight);

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
        BRRunTestWalletManagerSyncTestSetup (&state, blockHeight);

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
        BRRunTestWalletManagerSyncTestSetup (&state, blockHeight);

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
        BRRunTestWalletManagerSyncTestSetup (&state, blockHeight);

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
        BRRunTestWalletManagerSyncTestSetup (&state, blockHeight);

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
        BRRunTestWalletManagerSyncTestSetup (&state, blockHeight);

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
        BRRunTestWalletManagerSyncTestSetup (&state, blockHeight);

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
        BRRunTestWalletManagerSyncTestSetup (&state, blockHeight);

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
        BRRunTestWalletManagerSyncTestSetup (&state, blockHeight);

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

extern int BRRunTestWalletManagerSyncP2P (const char *paperKey,
                                          const char *storagePath,
                                          uint32_t earliestKeyTime,
                                          uint64_t blockHeight,
                                          int isBTC,
                                          int isMainnet) {
  return BRRunTestWalletManagerSyncForMode("BRRunTestWalletManagerSyncP2P",
                                           SYNC_MODE_P2P_ONLY,
                                           paperKey,
                                           storagePath,
                                           earliestKeyTime,
                                           blockHeight,
                                           isBTC,
                                           isMainnet);
}

extern int BRRunTestWalletManagerSyncBRD (const char *paperKey,
                                          const char *storagePath,
                                          uint32_t earliestKeyTime,
                                          uint64_t blockHeight,
                                          int isBTC,
                                          int isMainnet) {
  return BRRunTestWalletManagerSyncForMode("BRRunTestWalletManagerSyncBRD",
                                           SYNC_MODE_BRD_ONLY,
                                           paperKey,
                                           storagePath,
                                           earliestKeyTime,
                                           blockHeight,
                                           isBTC,
                                           isMainnet);
}
