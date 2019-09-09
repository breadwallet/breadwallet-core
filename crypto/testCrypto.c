//
//  testCrypto.c
//  CoreTests
//
//  Created by Ed Gamble on 3/28/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "BRCryptoAmount.h"
#include "BRCryptoPrivate.h"
#include "BRCryptoWalletManager.h"
#include "BRCryptoWallet.h"
#include "BRCryptoTransfer.h"

#ifdef __ANDROID__
#include <android/log.h>
#define fprintf(...) __android_log_print(ANDROID_LOG_ERROR, "testCrypto", _va_rest(__VA_ARGS__, NULL))
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "testCrypto", __VA_ARGS__)
#define _va_first(first, ...) first
#define _va_rest(first, ...) __VA_ARGS__
#endif

///
/// Mark: BRCryptoAmount Tests
///

static void
runCryptoAmountTests (void) {
    BRCryptoBoolean overflow;

    BRCryptoCurrency currency =
        cryptoCurrencyCreate ("Cuids",
                              "Cname",
                              "Ccode",
                              "Ctype",
                              NULL);

    BRCryptoUnit unitBase =
        cryptoUnitCreateAsBase (currency,
                                "UuidsBase",
                                "UnameBase",
                                "UsymbBase");

    BRCryptoUnit unitDef =
        cryptoUnitCreate (currency,
                          "UuidsDef",
                          "UnameDef",
                          "UsymbDef",
                          unitBase,
                          18);

    double value = 25.25434525155732538797258871;

    BRCryptoAmount amountInBase = cryptoAmountCreateDouble (value, unitBase);
    assert (NULL != amountInBase);

    double valueFromBase = cryptoAmountGetDouble (amountInBase, unitBase, &overflow);
    assert (CRYPTO_FALSE == overflow);
    assert (valueFromBase == 25.0);  // In base truncated fraction
    cryptoAmountGive(amountInBase);

    BRCryptoAmount amountInDef  = cryptoAmountCreateDouble (value, unitDef);
    assert (NULL != amountInDef);

    double valueFromDef = cryptoAmountGetDouble (amountInDef, unitDef, &overflow);
    assert (CRYPTO_FALSE == overflow);
    assert (fabs (valueFromDef - value) / value < 1e-10);
    cryptoAmountGive(amountInDef);

    value = 1e50;
    amountInBase = cryptoAmountCreateDouble (value, unitBase);
    assert (NULL != amountInBase);
    valueFromBase = cryptoAmountGetDouble (amountInBase, unitBase, &overflow);
    assert (CRYPTO_FALSE == overflow);
    assert (fabs (valueFromBase - value) / value < 1e-10);
    cryptoAmountGive(amountInBase);

    value = 1e100;
    amountInBase = cryptoAmountCreateDouble (value, unitBase);
    assert (NULL == amountInBase);

    cryptoUnitGive(unitDef);
    cryptoUnitGive(unitBase);
    cryptoCurrencyGive(currency);
}

///
/// Mark: BRCryptoWalletManager Tests
///

// BRCryptoWalletManager Abuse Thread Routines

typedef struct {
    uint8_t kill;
    BRCryptoWalletManager manager;
    BRSyncMode primaryMode;
    BRSyncMode secondaryMode;
} CWMAbuseThreadState;

static void *
_CWMAbuseConnectThread (void *context) {
    CWMAbuseThreadState *state = (CWMAbuseThreadState *) context;
    while (!state->kill) {
        cryptoWalletManagerConnect (state->manager);
    }
    return NULL;
}

static void *
_CWMAbuseDisconnectThread (void *context) {
    CWMAbuseThreadState *state = (CWMAbuseThreadState *) context;
    while (!state->kill) {
        cryptoWalletManagerDisconnect (state->manager);
    }
    return NULL;
}

static void *
_CWMAbuseSyncThread (void *context) {
    CWMAbuseThreadState *state = (CWMAbuseThreadState *) context;
    while (!state->kill) {
        cryptoWalletManagerSync (state->manager);
    }
    return NULL;
}

static void *
_CWMAbuseSwapThread (void *context) {
    CWMAbuseThreadState *state = (CWMAbuseThreadState *) context;
    while (!state->kill) {
        if (state->primaryMode == cryptoWalletManagerGetMode (state->manager)) {
            cryptoWalletManagerSetMode (state->manager, state->secondaryMode);
        } else {
            cryptoWalletManagerSetMode (state->manager, state->primaryMode);
        }
    }
    return NULL;
}

// BRCryptoCWMClient NOP Callbacks

// TODO(fix): The below callbacks leak state

static void
_CWMNopGetBlockNumberBtcCallback (BRCryptoCWMClientContext context,
                                  OwnershipGiven BRCryptoWalletManager manager,
                                  OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopGetTransactionsBtcCallback (BRCryptoCWMClientContext context,
                                   OwnershipGiven BRCryptoWalletManager manager,
                                   OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                   OwnershipKept const char **addresses,
                                   size_t addressCount,
                                   uint64_t begBlockNumber,
                                   uint64_t endBlockNumber) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopSubmitTransactionBtcCallback (BRCryptoCWMClientContext context,
                                     OwnershipGiven BRCryptoWalletManager manager,
                                     OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                     OwnershipKept uint8_t *transaction,
                                     size_t transactionLength,
                                     OwnershipKept const char *hashAsHex) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopGetEtherBalanceEthCallback (BRCryptoCWMClientContext context,
                                            OwnershipGiven BRCryptoWalletManager manager,
                                            OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                            OwnershipKept const char *network,
                                            OwnershipKept const char *address) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopGetTokenBalanceEthCallback (BRCryptoCWMClientContext context,
                                            OwnershipGiven BRCryptoWalletManager manager,
                                            OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                            OwnershipKept const char *network,
                                            OwnershipKept const char *address,
                                            OwnershipKept const char *tokenAddress) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopGetGasPriceEthCallback (BRCryptoCWMClientContext context,
                                        OwnershipGiven BRCryptoWalletManager manager,
                                        OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                        OwnershipKept const char *network) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopEstimateGasEthCallback (BRCryptoCWMClientContext context,
                                        OwnershipGiven BRCryptoWalletManager manager,
                                        OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                        OwnershipKept const char *network,
                                        OwnershipKept const char *from,
                                        OwnershipKept const char *to,
                                        OwnershipKept const char *amount,
                                        OwnershipKept const char *price,
                                        OwnershipKept const char *data) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopSubmitTransactionEthCallback (BRCryptoCWMClientContext context,
                                            OwnershipGiven BRCryptoWalletManager manager,
                                            OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                            OwnershipKept const char *network,
                                            OwnershipKept const char *transaction) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopGetTransactionsEthCallback (BRCryptoCWMClientContext context,
                                            OwnershipGiven BRCryptoWalletManager manager,
                                            OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                            OwnershipKept const char *network,
                                            OwnershipKept const char *address,
                                            uint64_t begBlockNumber,
                                            uint64_t endBlockNumber) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopGetLogsEthCallback (BRCryptoCWMClientContext context,
                                    OwnershipGiven BRCryptoWalletManager manager,
                                    OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                    OwnershipKept const char *network,
                                    OwnershipKept const char *contract,
                                    OwnershipKept const char *address,
                                    OwnershipKept const char *event,
                                    uint64_t begBlockNumber,
                                    uint64_t endBlockNumber) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopGetBlocksEthCallback (BRCryptoCWMClientContext context,
                                    OwnershipGiven BRCryptoWalletManager manager,
                                    OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                    OwnershipKept const char *network,
                                    OwnershipKept const char *address, // disappears immediately
                                    BREthereumSyncInterestSet interests,
                                    uint64_t blockNumberStart,
                                    uint64_t blockNumberStop) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopGetTokensEthCallback (BRCryptoCWMClientContext context,
                                    OwnershipGiven BRCryptoWalletManager manager,
                                    OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopGetBlockNumberEthCallback (BRCryptoCWMClientContext context,
                                            OwnershipGiven BRCryptoWalletManager manager,
                                            OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                            OwnershipKept const char *network) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopGetNonceEthCallback (BRCryptoCWMClientContext context,
                                    OwnershipGiven BRCryptoWalletManager manager,
                                    OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                    OwnershipKept const char *network,
                                    OwnershipKept const char *address) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopGetBlockNumberGenCallback (BRCryptoCWMClientContext context,
                                            OwnershipGiven BRCryptoWalletManager manager,
                                            OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopGetTransactionsGenCallback (BRCryptoCWMClientContext context,
                                            OwnershipGiven BRCryptoWalletManager manager,
                                            OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                            OwnershipKept const char *address,
                                            uint64_t begBlockNumber,
                                            uint64_t endBlockNumber) {
    cryptoWalletManagerGive (manager);
}

static void
_CWMNopSubmitTransactionGenCallback (BRCryptoCWMClientContext context,
                                            OwnershipGiven BRCryptoWalletManager manager,
                                            OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                            OwnershipKept uint8_t *transaction,
                                            size_t transactionLength,
                                            OwnershipKept const char *hashAsHex) {
    cryptoWalletManagerGive (manager);
}


// BRCryptoCWMListener Event Wrapper

typedef enum {
    SYNC_EVENT_WALLET_MANAGER_TYPE,
    SYNC_EVENT_WALLET_TYPE,
    SYNC_EVENT_TXN_TYPE,
} CWMEventType;

typedef struct CWMEventRecord {
    CWMEventType type;
    union {
        struct {
            BRCryptoWalletManager manager;
            BRCryptoWalletManagerEvent event;
        } m;
        struct {
            BRCryptoWalletManager manager;
            BRCryptoWallet wallet;
            BRCryptoWalletEvent event;
        } w;
        struct {
            BRCryptoWalletManager manager;
            BRCryptoWallet wallet;
            BRCryptoTransfer transfer;
            BRCryptoTransferEvent event;
        } t;
    } u;
} CWMEvent;

const char *
CWMEventTypeString (CWMEventType type) {
    switch (type) {
        case SYNC_EVENT_WALLET_MANAGER_TYPE:
        return "SYNC_EVENT_WALLET_MANAGER_TYPE";
        case SYNC_EVENT_WALLET_TYPE:
        return "SYNC_EVENT_WALLET_TYPE";
        case SYNC_EVENT_TXN_TYPE:
        return "SYNC_EVENT_TXN_TYPE";
    }
}

static CWMEvent
CWMEventForWalletManagerType(BRCryptoWalletManagerEventType type) {
    return (CWMEvent) {
        SYNC_EVENT_WALLET_MANAGER_TYPE,
        {
            .m = {
                NULL,
                (BRCryptoWalletManagerEvent) {
                    type
                }
            }
        }
    };
}

static CWMEvent
CWMEventForWalletManagerStateType(BRCryptoWalletManagerEventType type,
                                  BRCryptoWalletManagerState oldState,
                                  BRCryptoWalletManagerState newState) {
    return (CWMEvent) {
        SYNC_EVENT_WALLET_MANAGER_TYPE,
        {
            .m = {
                NULL,
                (BRCryptoWalletManagerEvent) {
                    type,
                    {
                        .state = { oldState, newState }
                    }
                }
            }
        }
    };
}

static CWMEvent
CWMEventForWalletManagerWalletType(BRCryptoWalletManagerEventType type,
                                   BRCryptoWallet wallet) {
    return (CWMEvent) {
        SYNC_EVENT_WALLET_MANAGER_TYPE,
        {
            .m = {
                NULL,
                (BRCryptoWalletManagerEvent) {
                    type,
                    {
                        .wallet = { wallet }
                    }
                }
            }
        }
    };
}

static CWMEvent
CWMEventForWalletType(BRCryptoWalletEventType type) {
    return (CWMEvent) {
        SYNC_EVENT_WALLET_TYPE,
        {
            .w = {
                NULL,
                NULL,
                (BRCryptoWalletEvent) {
                    type
                }
            }
        }
    };
}

static int
CWMEventEqual (CWMEvent *e1, CWMEvent *e2) {
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

                switch (e1->u.m.event.type) {
                    case CRYPTO_WALLET_MANAGER_EVENT_CHANGED:
                        success = 0 == memcmp(&e1->u.m.event.u.state, &e2->u.m.event.u.state, sizeof(e1->u.m.event.u.state));
                        break;
                    case CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED:
                    case CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED:
                    case CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED:
                        success = CRYPTO_TRUE == cryptoWalletEqual (e1->u.m.event.u.wallet.value, e2->u.m.event.u.wallet.value);
                        break;
                    case CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES:
                        // Do we want to check for this?
                    case CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED:
                        // Do we want to check for this?
                    default:
                        break;
                }
                break;
            }
            case SYNC_EVENT_WALLET_TYPE: {
                if (e1->u.w.event.type != e2->u.w.event.type) {
                    success = 0;
                }

                switch (e1->u.w.event.type) {
                    case CRYPTO_WALLET_EVENT_CHANGED:
                        success = 0 == memcmp(&e1->u.w.event.u.state, &e2->u.w.event.u.state, sizeof(e1->u.w.event.u.state));
                        break;
                    case CRYPTO_WALLET_EVENT_BALANCE_UPDATED:
                        success = CRYPTO_COMPARE_EQ == cryptoAmountCompare (e1->u.w.event.u.balanceUpdated.amount, e2->u.w.event.u.balanceUpdated.amount);
                        break;
                    case CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED:
                        success = CRYPTO_TRUE == cryptoFeeBasisIsIdentical (e1->u.w.event.u.feeBasisUpdated.basis, e2->u.w.event.u.feeBasisUpdated.basis);
                        break;
                    case CRYPTO_WALLET_EVENT_TRANSFER_ADDED:
                    case CRYPTO_WALLET_EVENT_TRANSFER_CHANGED:
                    case CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED:
                    case CRYPTO_WALLET_EVENT_TRANSFER_DELETED:
                        success = CRYPTO_TRUE == cryptoTransferEqual (e1->u.w.event.u.transfer.value, e2->u.w.event.u.transfer.value);
                        break;
                    case CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED:
                        success = (e1->u.w.event.u.feeBasisEstimated.cookie == e2->u.w.event.u.feeBasisEstimated.cookie &&
                                   e1->u.w.event.u.feeBasisEstimated.status == e2->u.w.event.u.feeBasisEstimated.status &&
                                   CRYPTO_TRUE == cryptoFeeBasisIsIdentical (e1->u.w.event.u.feeBasisEstimated.basis, e2->u.w.event.u.feeBasisEstimated.basis));
                        break;
                    default:
                        break;
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
CWMEventString (CWMEvent *e) {
    const char * subtypeString = NULL;
    const char * typeString = CWMEventTypeString (e->type);

    switch (e->type) {
        case SYNC_EVENT_WALLET_MANAGER_TYPE:
        subtypeString = BRCryptoWalletManagerEventTypeString (e->u.m.event.type);
        break;
        case SYNC_EVENT_WALLET_TYPE:
        subtypeString = BRCryptoWalletEventTypeString (e->u.w.event.type);
        break;
        case SYNC_EVENT_TXN_TYPE:
        subtypeString = BRCryptoTransferEventTypeString (e->u.t.event.type);
        break;
    }

    const char * fmtString = "CWMEventString(%s -> %s)";
    size_t fmtStringLength = strlen (fmtString);

    size_t eventStringLength = fmtStringLength + strlen(typeString)  + strlen(subtypeString) + 1;
    char * eventString = calloc (eventStringLength, sizeof(char));

    snprintf (eventString, eventStringLength, fmtString, typeString, subtypeString);
    return eventString;
}

// BRCryptoCWMListener Event Recording

typedef struct {
    BRCryptoBoolean silent;
    BRArrayOf(CWMEvent *) events;
    pthread_mutex_t lock;
} CWMEventRecordingState;

static void
CWMEventRecordingStateNew (CWMEventRecordingState *state,
                           BRCryptoBoolean isSilent) {
    state->silent = isSilent;
    array_new (state->events, 100);
    pthread_mutex_init (&state->lock, NULL);
}

static void
CWMEventRecordingStateNewDefault (CWMEventRecordingState *state) {
    CWMEventRecordingStateNew (state, CRYPTO_FALSE);
}

static void
CWMEventRecordingStateFree (CWMEventRecordingState *state) {
    for (size_t index = 0; index < array_count(state->events); index++)
        free (state->events[index]);
    array_free (state->events);

    pthread_mutex_destroy (&state->lock);
}

// TODO(fix): The below callbacks leak managers/wallets/transfers, as well as any ref counted event fields

static void
_CWMEventRecordingManagerCallback (BRCryptoCWMListenerContext context,
                                   BRCryptoWalletManager manager,
                                   BRCryptoWalletManagerEvent event) {
    CWMEventRecordingState *state = (CWMEventRecordingState*) context;
    CWMEvent *cwmEvent = calloc (1, sizeof (CWMEvent));
    cwmEvent->type = SYNC_EVENT_WALLET_MANAGER_TYPE;
    cwmEvent->u.m.manager= manager;
    cwmEvent->u.m.event = event;

    pthread_mutex_lock (&state->lock);
    array_add (state->events, cwmEvent);
    if (!state->silent) printf ("Added MANAGER event: %s (%zu total)\n", BRCryptoWalletManagerEventTypeString (event.type), array_count (state->events));
    pthread_mutex_unlock (&state->lock);
}

static void
_CWMEventRecordingWalletCallback (BRCryptoCWMListenerContext context,
                                  BRCryptoWalletManager manager,
                                  BRCryptoWallet wallet,
                                  BRCryptoWalletEvent event) {
    CWMEventRecordingState *state = (CWMEventRecordingState*) context;
    CWMEvent *cwmEvent = calloc (1, sizeof (CWMEvent));
    cwmEvent->type = SYNC_EVENT_WALLET_TYPE;
    cwmEvent->u.w.manager= manager;
    cwmEvent->u.w.wallet= wallet;
    cwmEvent->u.w.event = event;

    pthread_mutex_lock (&state->lock);
    array_add (state->events, cwmEvent);
    if (!state->silent) printf ("Added WALLET event: %s (%zu total)\n", BRCryptoWalletEventTypeString (event.type), array_count (state->events));
    pthread_mutex_unlock (&state->lock);
}

static void
_CWMEventRecordingTransferCallback (BRCryptoCWMListenerContext context,
                                    BRCryptoWalletManager manager,
                                    BRCryptoWallet wallet,
                                    BRCryptoTransfer transfer,
                                    BRCryptoTransferEvent event) {
    CWMEventRecordingState *state = (CWMEventRecordingState*) context;
    CWMEvent *cwmEvent = calloc (1, sizeof (CWMEvent));
    cwmEvent->type = SYNC_EVENT_TXN_TYPE;
    cwmEvent->u.t.manager= manager;
    cwmEvent->u.t.wallet= wallet;
    cwmEvent->u.t.transfer = transfer;
    cwmEvent->u.t.event = event;

    pthread_mutex_lock (&state->lock);
    array_add (state->events, cwmEvent);
    if (!state->silent) printf ("Added TXN event: %s (%zu total)\n", BRCryptoTransferEventTypeString (event.type), array_count (state->events));
    pthread_mutex_unlock (&state->lock);
}

static size_t
CWMEventRecordingGetNextEventIndexByType(CWMEventRecordingState *state,
                                         size_t startIndex,
                                         CWMEventType type) {
    for (size_t index = startIndex; index < array_count(state->events); index++) {
        if (state->events[index]->type == type) {
            return index;
        }
    }
    return SIZE_MAX;
}

static size_t
CWMEventRecordingGetNextWalletManagerEventIndexForWallet(CWMEventRecordingState *state,
                                                         size_t startIndex,
                                                         BRCryptoWallet wallet) {
    for (size_t index = startIndex; index < array_count(state->events); index++) {
        if (state->events[index]->type == SYNC_EVENT_WALLET_MANAGER_TYPE) {
            switch (state->events[index]->u.m.event.type) {
                case CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED:
                case CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED:
                case CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED:
                    if (state->events[index]->u.m.event.u.wallet.value == wallet) {
                        return index;
                    }
                default:
                    continue;
            }
        }
    }
    return SIZE_MAX;
}

static size_t
CWMEventRecordingGetNextWalletManagerEventIndexForState(CWMEventRecordingState *state,
                                                        size_t startIndex) {
    for (size_t index = startIndex; index < array_count(state->events); index++) {
        if (state->events[index]->type == SYNC_EVENT_WALLET_MANAGER_TYPE) {
            switch (state->events[index]->u.m.event.type) {
                case CRYPTO_WALLET_MANAGER_EVENT_CREATED:
                case CRYPTO_WALLET_MANAGER_EVENT_CHANGED:
                case CRYPTO_WALLET_MANAGER_EVENT_DELETED:
                case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED:
                case CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES:
                case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED:
                    return index;
                default:
                    break;
            }
        }
    }
    return SIZE_MAX;
}

static int
CWMEventRecordingVerifyEventPairs (CWMEventRecordingState *state) {
    int success = 1;
    pthread_mutex_lock (&state->lock);

    // TODO(fix): Implement this; verifying valid event pairs of logically connected events such as:
    //              - CRYPTO_WALLET_MANAGER_EVENT_ created/changed/deleted/sync
    //              - CRYPTO_WALLET_MANAGER_EVENT_WALLET_ added/changed/deleted

    pthread_mutex_unlock (&state->lock);
    return success;
}

static int
CWMEventRecordingVerifyEventSequence (CWMEventRecordingState *state,
                                      BRCryptoBoolean isCompleteSequence,
                                      CWMEvent *expected,
                                      size_t expectedCount,
                                      CWMEvent *ignored,
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
                               CWMEventEqual (state->events[index], &expected[expectedIndex])))) {
            expectedIndex++;

        } else {
            for (size_t ignoredIndex = 0; !success && ignoredIndex < ignoredCount; ignoredIndex++) {
                success = CWMEventEqual (state->events[index], &ignored[ignoredIndex]);
            }
        }

        if (!success) {
            printf("%s: failed due to mismatched event types (expected at idx %zu -> %s, received at idx %zu -> %s)\n",
                   __func__,
                   expectedIndex,
                   CWMEventString (&expected[expectedIndex]),
                   index,
                   CWMEventString (state->events[index]));
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

///
/// Mark: Lifecycle Tests
///

static BRCryptoWalletManager
BRCryptoWalletManagerSetupForLifecycleTest (CWMEventRecordingState *state,
                                            BRCryptoAccount account,
                                            BRCryptoNetwork network,
                                            BRSyncMode mode,
                                            BRCryptoAddressScheme scheme,
                                            const char *storagePath)
{
    BRCryptoCWMListener listener = (BRCryptoCWMListener) {
        state,
        _CWMEventRecordingManagerCallback,
        _CWMEventRecordingWalletCallback,
        _CWMEventRecordingTransferCallback,
    };

    BRCryptoCWMClientBTC btcClient = (BRCryptoCWMClientBTC) {
        _CWMNopGetBlockNumberBtcCallback,
        _CWMNopGetTransactionsBtcCallback,
        _CWMNopSubmitTransactionBtcCallback,
    };

    BRCryptoCWMClientETH ethClient = (BRCryptoCWMClientETH) {
        _CWMNopGetEtherBalanceEthCallback,
        _CWMNopGetTokenBalanceEthCallback,
        _CWMNopGetGasPriceEthCallback,
        _CWMNopEstimateGasEthCallback,
        _CWMNopSubmitTransactionEthCallback,
        _CWMNopGetTransactionsEthCallback,
        _CWMNopGetLogsEthCallback,
        _CWMNopGetBlocksEthCallback,
        _CWMNopGetTokensEthCallback,
        _CWMNopGetBlockNumberEthCallback,
        _CWMNopGetNonceEthCallback,
    };

    BRCryptoCWMClientGEN genClient = (BRCryptoCWMClientGEN) {
        _CWMNopGetBlockNumberGenCallback,
        _CWMNopGetTransactionsGenCallback,
        _CWMNopSubmitTransactionGenCallback,
    };

    BRCryptoCWMClient client = (BRCryptoCWMClient) {
        state,
        btcClient,
        ethClient,
        genClient,
    };

    return cryptoWalletManagerCreate (listener, client, account, network, mode, scheme, storagePath);
}

static int
runCryptoWalletManagerLifecycleTest (BRCryptoAccount account,
                                     BRCryptoNetwork network,
                                     BRSyncMode mode,
                                     BRCryptoAddressScheme scheme,
                                     const char *storagePath) {
    int success = 1;

    // HACK: Managers set the height; we need to be able to restore it between tests
    BRCryptoBlockChainHeight originalNetworkHeight = cryptoNetworkGetHeight (network);

    printf("Testing BRCryptoWalletManager events for mode=\"%s\", network=\"%s (%s)\" and path=\"%s\"...\n",
           BRSyncModeString (mode),
           cryptoNetworkGetName (network),
           cryptoNetworkIsMainnet (network) ? "mainnet" : "testnet",
           storagePath);

   printf("Testing BRCryptoWalletManager connect, disconnect...\n");
   {
       // Test setup
       CWMEventRecordingState state = {0};
       CWMEventRecordingStateNewDefault (&state);

       BRCryptoWalletManager manager = BRCryptoWalletManagerSetupForLifecycleTest(&state, account, network, mode, scheme, storagePath);
       BRCryptoWallet wallet = cryptoWalletManagerGetWallet (manager);

       // connect, disconnect
       cryptoWalletManagerConnect (manager);
       sleep(1);
       cryptoWalletManagerDisconnect (manager);
       sleep(1);

       // Verification
       success = CWMEventRecordingVerifyEventSequence(&state,
                                                      CRYPTO_TRUE,
                                                      (CWMEvent []) {
                                                          // cryptoWalletManagerCreate()
                                                          CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_CREATED),
                                                          CWMEventForWalletType                (CRYPTO_WALLET_EVENT_CREATED),
                                                          CWMEventForWalletManagerWalletType   (CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                                                                    wallet),
                                                           // cryptoWalletManagerConnect()
                                                          CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_CREATED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_CONNECTED),
                                                          CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED),
                                                          CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_CONNECTED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_SYNCING),
                                                           // cryptoWalletManagerDisconnect()
                                                          CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED),
                                                          CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_SYNCING,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_CONNECTED),
                                                          CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_CONNECTED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED),
                                                      },
                                                      9,
                                                      (CWMEvent []) {
                                                          CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES),
                                                          CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED),
                                                      },
                                                      2);
       if (!success) {
           fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", __func__, __LINE__);
           return success;
       }

       success = CWMEventRecordingVerifyEventPairs (&state);
       if (!success) {
           fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", __func__, __LINE__);
           return success;
       }

       // Test teardown
       cryptoNetworkSetHeight (network, originalNetworkHeight);
       cryptoWalletGive (wallet);
       cryptoWalletManagerGive (manager);
       CWMEventRecordingStateFree (&state);
   }

    printf("Testing BRCryptoWalletManager repeated connect attempts...\n");
    {
        // Test setup
        CWMEventRecordingState state = {0};
        CWMEventRecordingStateNewDefault (&state);

        BRCryptoWalletManager manager = BRCryptoWalletManagerSetupForLifecycleTest(&state, account, network, mode, scheme, storagePath);
        BRCryptoWallet wallet = cryptoWalletManagerGetWallet (manager);

        // repeated connect attempts
        cryptoWalletManagerConnect (manager);
        sleep(1);
        cryptoWalletManagerConnect (manager);
        sleep(1);
        cryptoWalletManagerConnect (manager);
        sleep(1);
        cryptoWalletManagerDisconnect (manager);
        sleep(1);

        // Verification
        success = CWMEventRecordingVerifyEventSequence(&state,
                                                       CRYPTO_TRUE,
                                                       (CWMEvent []) {
                                                           // cryptoWalletManagerCreate()
                                                           CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_CREATED),
                                                           CWMEventForWalletType                (CRYPTO_WALLET_EVENT_CREATED),
                                                           CWMEventForWalletManagerWalletType   (CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                                                                     wallet),
                                                            // cryptoWalletManagerConnect() - first, second and third do nothing
                                                           CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_CREATED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_CONNECTED),
                                                           CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED),
                                                           CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_CONNECTED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_SYNCING),
                                                            // cryptoWalletManagerDisconnect()
                                                           CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED),
                                                           CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_SYNCING,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_CONNECTED),
                                                           CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_CONNECTED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED),
                                                       },
                                                       9,
                                                       (CWMEvent []) {
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES),
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED),
                                                       },
                                                       2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", __func__, __LINE__);
            return success;
        }

        success = CWMEventRecordingVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", __func__, __LINE__);
            return success;
        }

        // Test teardown
        cryptoNetworkSetHeight (network, originalNetworkHeight);
        cryptoWalletGive (wallet);
        cryptoWalletManagerGive (manager);
        CWMEventRecordingStateFree (&state);
    }

    printf("Testing BRWalletManager connect, scan, disconnect...\n");
    {
        // TODO(fix): Add this test; issue here is the variance in P2P, namely that there are multiple possible
        //            cases depending on peer connections:
        //      - a reconnect, if the BRWalletManagerScan was after the P2P connection and after receiving blocks (case 1)
        //      - a sync restart, if the BRWalletManagerScan was after the P2P connection but before the sync received blocks (case 2)
        //      - a sync, if the BRWalletManagerScan beat the P2P connections being established (case 3)
    }

    printf("Testing BRCryptoWalletManager repeated disconnect attempts...\n");
    {
        // Test setup
        CWMEventRecordingState state = {0};
        CWMEventRecordingStateNewDefault (&state);

        BRCryptoWalletManager manager = BRCryptoWalletManagerSetupForLifecycleTest(&state, account, network, mode, scheme, storagePath);
        BRCryptoWallet wallet = cryptoWalletManagerGetWallet (manager);

        // repeated disconnect attempts
        cryptoWalletManagerDisconnect (manager);
        sleep(1);
        cryptoWalletManagerDisconnect (manager);
        sleep(1);
        cryptoWalletManagerDisconnect (manager);
        sleep(1);

        // Verification
        success = CWMEventRecordingVerifyEventSequence(&state,
                                                       CRYPTO_TRUE,
                                                       (CWMEvent []) {
                                                           // cryptoWalletManagerCreate()
                                                           CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_CREATED),
                                                           CWMEventForWalletType                (CRYPTO_WALLET_EVENT_CREATED),
                                                           CWMEventForWalletManagerWalletType   (CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                                                                     wallet),
                                                       },
                                                       3,
                                                       (CWMEvent []) {
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES),
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED),
                                                       },
                                                       2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", __func__, __LINE__);
            return success;
        }

        success = CWMEventRecordingVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", __func__, __LINE__);
            return success;
        }

        // Test teardown
        cryptoNetworkSetHeight (network, originalNetworkHeight);
        cryptoWalletGive (wallet);
        cryptoWalletManagerGive (manager);
        CWMEventRecordingStateFree (&state);
    }

    printf("Testing BRCryptoWalletManager sync, connect, disconnect...\n");
    {
        // Test setup
        CWMEventRecordingState state = {0};
        CWMEventRecordingStateNewDefault (&state);

        BRCryptoWalletManager manager = BRCryptoWalletManagerSetupForLifecycleTest(&state, account, network, mode, scheme, storagePath);
        BRCryptoWallet wallet = cryptoWalletManagerGetWallet (manager);

        // sync, connect, disconnect
        cryptoWalletManagerSync (manager);
        sleep(1);
        cryptoWalletManagerConnect (manager);
        sleep(1);
        cryptoWalletManagerDisconnect (manager);
        sleep(1);

        // Verification
        success = CWMEventRecordingVerifyEventSequence(&state,
                                                       CRYPTO_TRUE,
                                                       (CWMEvent []) {
                                                           // cryptoWalletManagerCreate()
                                                           CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_CREATED),
                                                           CWMEventForWalletType                (CRYPTO_WALLET_EVENT_CREATED),
                                                           CWMEventForWalletManagerWalletType   (CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                                                                     wallet),
                                                            // cryptoWalletManagerConnect()
                                                           CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_CREATED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_CONNECTED),
                                                           CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED),
                                                           CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_CONNECTED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_SYNCING),
                                                            // cryptoWalletManagerDisconnect()
                                                           CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED),
                                                           CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_SYNCING,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_CONNECTED),
                                                           CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_CONNECTED,
                                                                                                     CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED),
                                                       },
                                                       9,
                                                       (CWMEvent []) {
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES),
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED),
                                                       },
                                                       2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", __func__, __LINE__);
            return success;
        }

        success = CWMEventRecordingVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", __func__, __LINE__);
            return success;
        }

        // Test teardown
        cryptoNetworkSetHeight (network, originalNetworkHeight);
        cryptoWalletGive (wallet);
        cryptoWalletManagerGive (manager);
        CWMEventRecordingStateFree (&state);
    }

    printf("Testing BRCryptoWalletManager sync, disconnect...\n");
    {
        // Test setup
        CWMEventRecordingState state = {0};
        CWMEventRecordingStateNewDefault (&state);

        BRCryptoWalletManager manager = BRCryptoWalletManagerSetupForLifecycleTest(&state, account, network, mode, scheme, storagePath);
        BRCryptoWallet wallet = cryptoWalletManagerGetWallet (manager);

        // sync, disconnect
        cryptoWalletManagerSync (manager);
        sleep(1);
        cryptoWalletManagerDisconnect (manager);
        sleep(1);

        // Verification
        success = CWMEventRecordingVerifyEventSequence(&state,
                                                       CRYPTO_TRUE,
                                                       (CWMEvent []) {
                                                           // cryptoWalletManagerCreate()
                                                           CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_CREATED),
                                                           CWMEventForWalletType                (CRYPTO_WALLET_EVENT_CREATED),
                                                           CWMEventForWalletManagerWalletType   (CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                                                                     wallet),
                                                       },
                                                       3,
                                                       (CWMEvent []) {
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES),
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED),
                                                       },
                                                       2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", __func__, __LINE__);
            return success;
        }

        success = CWMEventRecordingVerifyEventPairs (&state);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", __func__, __LINE__);
            return success;
        }

        // Test teardown
        cryptoNetworkSetHeight (network, originalNetworkHeight);
        cryptoWalletGive (wallet);
        cryptoWalletManagerGive (manager);
        CWMEventRecordingStateFree (&state);
    }

    printf("Testing BRCryptoWalletManager threading...\n");
    if (mode == SYNC_MODE_P2P_ONLY && BLOCK_CHAIN_TYPE_BTC == cryptoNetworkGetType (network)) {
        // TODO(fix): There is a thread-related issue in BRPeerManager/BRPeer where we have a use after free; re-enable once that is fixed
        fprintf(stderr, "***WARNING*** %s:%d: BRCryptoWalletManager threading test is disabled for SYNC_MODE_P2P_ONLY and BLOCK_CHAIN_TYPE_BTC\n", __func__, __LINE__);

    } else {
        // Test setup
        CWMEventRecordingState state = {0};
        CWMEventRecordingStateNew (&state, CRYPTO_TRUE);

        BRCryptoWalletManager manager = BRCryptoWalletManagerSetupForLifecycleTest(&state, account, network, mode, scheme, storagePath);
        BRCryptoWallet wallet = cryptoWalletManagerGetWallet (manager);

        // release the hounds
        CWMAbuseThreadState threadState = {0, manager};
        pthread_t connectThread = (pthread_t) NULL, disconnectThread = (pthread_t) NULL, scanThread = (pthread_t) NULL;

        success = (0 == pthread_create (&connectThread, NULL, _CWMAbuseConnectThread, (void*) &threadState) &&
                   0 == pthread_create (&disconnectThread, NULL, _CWMAbuseDisconnectThread, (void*) &threadState) &&
                   0 == pthread_create (&scanThread, NULL, _CWMAbuseSyncThread, (void*) &threadState));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: pthread_creates failed\n", __func__, __LINE__);
            return success;
        }

        sleep (1);

        threadState.kill = 1;
        success = (0 == pthread_join (connectThread, NULL) &&
                   0 == pthread_join (disconnectThread, NULL) &&
                   0 == pthread_join (scanThread, NULL));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: pthread_joins failed\n", __func__, __LINE__);
            return success;
        }

        // Verification
        success = CWMEventRecordingVerifyEventSequence(&state,
                                                       CRYPTO_FALSE,
                                                       (CWMEvent []) {
                                                           // cryptoWalletManagerCreate()
                                                           CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_CREATED),
                                                           CWMEventForWalletType                (CRYPTO_WALLET_EVENT_CREATED),
                                                           CWMEventForWalletManagerWalletType   (CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                                                                     wallet),
                                                       },
                                                       3,
                                                       (CWMEvent []) {
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES),
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED),
                                                       },
                                                       2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", __func__, __LINE__);
            return success;
        }

        // Test teardown
        cryptoNetworkSetHeight (network, originalNetworkHeight);
        cryptoWalletGive (wallet);
        cryptoWalletManagerGive (manager);
        CWMEventRecordingStateFree (&state);
    }

    return success;
}

static int
runCryptoWalletManagerLifecycleWithSetModeTest (BRCryptoAccount account,
                                                BRCryptoNetwork network,
                                                BRSyncMode primaryMode,
                                                BRSyncMode secondaryMode,
                                                BRCryptoAddressScheme scheme,
                                                const char *storagePath) {
    int success = 1;

    // HACK: Managers set the height; we need to be able to restore it between tests
    BRCryptoBlockChainHeight originalNetworkHeight = cryptoNetworkGetHeight (network);

    printf("Testing BRCryptoWalletManager events for mode=\"%s/%s\", network=\"%s (%s)\" and path=\"%s\"...\n",
           BRSyncModeString (primaryMode),
           BRSyncModeString (secondaryMode),
           cryptoNetworkGetName (network),
           cryptoNetworkIsMainnet (network) ? "mainnet" : "testnet",
           storagePath);

   printf("Testing BRCryptoWalletManager mode swap while disconnected...\n");
   {
       // Test setup
       CWMEventRecordingState state = {0};
       CWMEventRecordingStateNewDefault (&state);

       BRCryptoWalletManager manager = BRCryptoWalletManagerSetupForLifecycleTest(&state, account, network, primaryMode, scheme, storagePath);
       BRCryptoWallet wallet = cryptoWalletManagerGetWallet (manager);

        // swap modes while disconnected
        cryptoWalletManagerSetMode (manager, secondaryMode);
        success = cryptoWalletManagerGetMode (manager) == secondaryMode;
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRWalletManagerSetMode failed\n", __func__, __LINE__);
            return success;
        }
        sleep (1);

       // Verification
        success = CWMEventRecordingVerifyEventSequence(&state,
                                                       CRYPTO_TRUE,
                                                       (CWMEvent []) {
                                                           // cryptoWalletManagerCreate()
                                                           CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_CREATED),
                                                           CWMEventForWalletType                (CRYPTO_WALLET_EVENT_CREATED),
                                                           CWMEventForWalletManagerWalletType   (CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                                                                     wallet),
                                                       },
                                                       3,
                                                       (CWMEvent []) {
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES),
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED),
                                                       },
                                                       2);
       if (!success) {
           fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", __func__, __LINE__);
           return success;
       }

       success = CWMEventRecordingVerifyEventPairs (&state);
       if (!success) {
           fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", __func__, __LINE__);
           return success;
       }

       // Test teardown
       cryptoNetworkSetHeight (network, originalNetworkHeight);
       cryptoWalletGive (wallet);
       cryptoWalletManagerGive (manager);
       CWMEventRecordingStateFree (&state);
   }

   printf("Testing BRCryptoWalletManager mode swap while connected...\n");
   {
       // Test setup
       CWMEventRecordingState state = {0};
       CWMEventRecordingStateNewDefault (&state);

       BRCryptoWalletManager manager = BRCryptoWalletManagerSetupForLifecycleTest(&state, account, network, primaryMode, scheme, storagePath);
       BRCryptoWallet wallet = cryptoWalletManagerGetWallet (manager);

        // swap modes while connected
        cryptoWalletManagerConnect (manager);
        sleep(1);

        cryptoWalletManagerSetMode (manager, secondaryMode);
        success = cryptoWalletManagerGetMode (manager) == secondaryMode;
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRWalletManagerSetMode failed\n", __func__, __LINE__);
            return success;
        }
        sleep (1);

       // Verification
       success = CWMEventRecordingVerifyEventSequence(&state,
                                                      CRYPTO_TRUE,
                                                      (CWMEvent []) {
                                                          // cryptoWalletManagerCreate()
                                                          CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_CREATED),
                                                          CWMEventForWalletType                (CRYPTO_WALLET_EVENT_CREATED),
                                                          CWMEventForWalletManagerWalletType   (CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                                                                    wallet),
                                                           // cryptoWalletManagerConnect()
                                                          CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_CREATED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_CONNECTED),
                                                          CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED),
                                                          CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_CONNECTED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_SYNCING),
                                                           // cryptoWalletManagerSetMode()
                                                          CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED),
                                                          CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_SYNCING,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_CONNECTED),
                                                          CWMEventForWalletManagerStateType    (CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_CONNECTED,
                                                                                                    CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED),
                                                      },
                                                      9,
                                                      (CWMEvent []) {
                                                          CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES),
                                                          CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED),
                                                      },
                                                      2);
       if (!success) {
           fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", __func__, __LINE__);
           return success;
       }

       success = CWMEventRecordingVerifyEventPairs (&state);
       if (!success) {
           fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventPairs failed\n", __func__, __LINE__);
           return success;
       }

       // Test teardown
       cryptoNetworkSetHeight (network, originalNetworkHeight);
       cryptoWalletGive (wallet);
       cryptoWalletManagerGive (manager);
       CWMEventRecordingStateFree (&state);
   }

    printf("Testing BRCryptoWalletManager mode swap threading...\n");
    if (BLOCK_CHAIN_TYPE_BTC == cryptoNetworkGetType (network) &&
        (primaryMode == SYNC_MODE_P2P_ONLY || secondaryMode == SYNC_MODE_P2P_ONLY)) {
        // TODO(fix): There is a thread-related issue in BRPeerManager/BRPeer where we have a use after free; re-enable once that is fixed
        fprintf(stderr, "***WARNING*** %s:%d: BRCryptoWalletManager threading test is disabled for SYNC_MODE_P2P_ONLY and BLOCK_CHAIN_TYPE_BTC\n", __func__, __LINE__);

    } else {
        // Test setup
        CWMEventRecordingState state = {0};
        CWMEventRecordingStateNew (&state, CRYPTO_TRUE);

        BRCryptoWalletManager manager = BRCryptoWalletManagerSetupForLifecycleTest(&state, account, network, primaryMode, scheme, storagePath);
        BRCryptoWallet wallet = cryptoWalletManagerGetWallet (manager);

        // release the hounds
        CWMAbuseThreadState threadState = {0, manager, primaryMode, secondaryMode};
        pthread_t connectThread = (pthread_t) NULL, disconnectThread = (pthread_t) NULL, scanThread = (pthread_t) NULL, swapThread = (pthread_t) NULL;

        success = (0 == pthread_create (&connectThread, NULL, _CWMAbuseConnectThread, (void*) &threadState) &&
                   0 == pthread_create (&disconnectThread, NULL, _CWMAbuseDisconnectThread, (void*) &threadState) &&
                   0 == pthread_create (&scanThread, NULL, _CWMAbuseSyncThread, (void*) &threadState) &&
                   0 == pthread_create (&swapThread, NULL, _CWMAbuseSwapThread, (void*) &threadState));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: pthread_creates failed\n", __func__, __LINE__);
            return success;
        }

        sleep (1);

        threadState.kill = 1;
        success = (0 == pthread_join (connectThread, NULL) &&
                   0 == pthread_join (disconnectThread, NULL) &&
                   0 == pthread_join (scanThread, NULL) &&
                   0 == pthread_join (swapThread, NULL));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: pthread_joins failed\n", __func__, __LINE__);
            return success;
        }

        // Verification
        success = CWMEventRecordingVerifyEventSequence(&state,
                                                       CRYPTO_FALSE,
                                                       (CWMEvent []) {
                                                           // cryptoWalletManagerCreate()
                                                           CWMEventForWalletManagerType         (CRYPTO_WALLET_MANAGER_EVENT_CREATED),
                                                           CWMEventForWalletType                (CRYPTO_WALLET_EVENT_CREATED),
                                                           CWMEventForWalletManagerWalletType   (CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                                                                     wallet),
                                                       },
                                                       3,
                                                       (CWMEvent []) {
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES),
                                                           CWMEventForWalletManagerType (CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED),
                                                       },
                                                       2);
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: BRRunTestWalletManagerSyncTestVerifyEventSequence failed\n", __func__, __LINE__);
            return success;
        }

        // Test teardown
        cryptoNetworkSetHeight (network, originalNetworkHeight);
        cryptoWalletGive (wallet);
        cryptoWalletManagerGive (manager);
        CWMEventRecordingStateFree (&state);
    }

    return success;
}

///
/// Mark: Entrypoints
///

extern BRCryptoBoolean
runCryptoTestsWithAccountAndNetwork (BRCryptoAccount account,
                                     BRCryptoNetwork network,
                                     const char *storagePath) {
    BRCryptoBoolean success = CRYPTO_TRUE;

    BRCryptoBlockChainType chainType = cryptoNetworkGetType (network);

    BRCryptoBoolean isGen = AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_GEN == chainType);
    BRCryptoBoolean isEth = AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_ETH == chainType);
    BRCryptoBoolean isBtc = (AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_BTC == chainType)
                             && (cryptoNetworkAsBTC (network) == BRMainNetParams || cryptoNetworkAsBTC (network) == BRTestNetParams));
    BRCryptoBoolean isBch = (AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_BTC == chainType)
                             && (cryptoNetworkAsBTC (network) == BRBCashParams || cryptoNetworkAsBTC (network) == BRBCashTestNetParams));

    BRCryptoAddressScheme scheme = ((isBtc || isBch) ?
                                    CRYPTO_ADDRESS_SCHEME_BTC_LEGACY :
                                    (isEth ?
                                     CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT :
                                     CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT));

    if (isBtc || isEth || isGen) {
        success = AS_CRYPTO_BOOLEAN(runCryptoWalletManagerLifecycleTest (account,
                                                                         network,
                                                                         SYNC_MODE_BRD_ONLY,
                                                                         scheme,
                                                                         storagePath));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: failed\n", __func__, __LINE__);
            return success;
        }
    }

    if (isBtc || isBch || isEth) {
        success = AS_CRYPTO_BOOLEAN(runCryptoWalletManagerLifecycleTest (account,
                                                                         network,
                                                                         SYNC_MODE_P2P_ONLY,
                                                                         scheme,
                                                                         storagePath));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: failed\n", __func__, __LINE__);
            return success;
        }
    }

    if (isEth) {
        success = AS_CRYPTO_BOOLEAN(runCryptoWalletManagerLifecycleTest (account,
                                                                         network,
                                                                         SYNC_MODE_BRD_WITH_P2P_SEND,
                                                                         scheme,
                                                                         storagePath));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: failed\n", __func__, __LINE__);
            return success;
        }
    }

    if (isBtc) {
        success = AS_CRYPTO_BOOLEAN(runCryptoWalletManagerLifecycleWithSetModeTest (account,
                                                                                    network,
                                                                                    SYNC_MODE_P2P_ONLY,
                                                                                    SYNC_MODE_BRD_ONLY,
                                                                                    scheme,
                                                                                    storagePath));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: failed\n", __func__, __LINE__);
            return success;
        }

        success = AS_CRYPTO_BOOLEAN(runCryptoWalletManagerLifecycleWithSetModeTest (account,
                                                                                    network,
                                                                                    SYNC_MODE_BRD_ONLY,
                                                                                    SYNC_MODE_P2P_ONLY,
                                                                                    scheme,
                                                                                    storagePath));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: failed\n", __func__, __LINE__);
            return success;
        }
    }

    if (isEth) {
        success = AS_CRYPTO_BOOLEAN(runCryptoWalletManagerLifecycleWithSetModeTest (account,
                                                                                    network,
                                                                                    SYNC_MODE_P2P_ONLY,
                                                                                    SYNC_MODE_BRD_WITH_P2P_SEND,
                                                                                    scheme,
                                                                                    storagePath));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: failed\n", __func__, __LINE__);
            return success;
        }

        success = AS_CRYPTO_BOOLEAN(runCryptoWalletManagerLifecycleWithSetModeTest (account,
                                                                                    network,
                                                                                    SYNC_MODE_BRD_WITH_P2P_SEND,
                                                                                    SYNC_MODE_P2P_ONLY,
                                                                                    scheme,
                                                                                    storagePath));
        if (!success) {
            fprintf(stderr, "***FAILED*** %s:%d: failed\n", __func__, __LINE__);
            return success;
        }
    }

    return success;
}

extern void
runCryptoTests (void) {
    runCryptoAmountTests ();
    return;
}
