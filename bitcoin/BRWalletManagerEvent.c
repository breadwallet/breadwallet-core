//
//  BRWalletManagerEvent.c
//  BRCore
//
//  Created by Michael Carrara on 3/19/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include "BRBase.h"
#include "BRWalletManager.h"
#include "BRWalletManagerPrivate.h"
#include "BRTransaction.h"

//
// Wallet Callbacks
//

typedef struct {
    struct BREventRecord base;
    BRWalletManager manager;
    BRTransaction *ownedTransaction;
    BRTransaction *refedTransaction;
} BRWalletManagerWalletTxAddedEvent;

static void
bwmSignalTxAddedDispatcher (BREventHandler ignore,
                            BRWalletManagerWalletTxAddedEvent *event) {
    bwmHandleTxAdded(event->manager, event->ownedTransaction, event->refedTransaction);
}

static void
bwmSignalTxAddedDestroyer (BRWalletManagerWalletTxAddedEvent *event) {
    BRTransactionFree (event->ownedTransaction);
}

static BREventType bwmSignalTxAddedEventType = {
    "BTC: Wallet TX Added Event",
    sizeof (BRWalletManagerWalletTxAddedEvent),
    (BREventDispatcher) bwmSignalTxAddedDispatcher,
    (BREventDestroyer) bwmSignalTxAddedDestroyer
};

extern void
bwmSignalTxAdded (BRWalletManager manager,
                  OwnershipGiven BRTransaction *ownedTransaction,
                  OwnershipKept BRTransaction *refedTransaction) {
    BRWalletManagerWalletTxAddedEvent message =
    { { NULL, &bwmSignalTxAddedEventType}, manager, ownedTransaction, refedTransaction};
    eventHandlerSignalEvent (manager->handler, (BREvent*) &message);
}

typedef struct {
    struct BREventRecord base;
    BRWalletManager manager;
    UInt256 hash;
    uint32_t blockHeight;
    uint32_t timestamp;
} BRWalletManagerWalletTxUpdatedEvent;

static void
bwmSignalTxUpdatedDispatcher (BREventHandler ignore,
                              BRWalletManagerWalletTxUpdatedEvent *event) {
    bwmHandleTxUpdated(event->manager, event->hash, event->blockHeight, event->timestamp);
}

static BREventType bwmSignalTxUpdatedEventType = {
    "BTC: Wallet TX Updated Event",
    sizeof (BRWalletManagerWalletTxUpdatedEvent),
    (BREventDispatcher) bwmSignalTxUpdatedDispatcher
};

extern void
bwmSignalTxUpdated (BRWalletManager manager,
                    UInt256 hash,
                    uint32_t blockHeight,
                    uint32_t timestamp) {
    BRWalletManagerWalletTxUpdatedEvent message =
    { { NULL, &bwmSignalTxUpdatedEventType}, manager, hash, blockHeight, timestamp};
    eventHandlerSignalEvent (manager->handler, (BREvent*) &message);
}

typedef struct {
    struct BREventRecord base;
    BRWalletManager manager;
    UInt256 hash;
    int recommendRescan;
} BRWalletManagerWalletTxDeletedEvent;

static void
bwmSignalTxDeletedDispatcher (BREventHandler ignore,
                              BRWalletManagerWalletTxDeletedEvent *event) {
    bwmHandleTxDeleted(event->manager, event->hash,  event->recommendRescan);
}

static BREventType bwmSignalTxDeletedEventType = {
    "BTC: Wallet TX Deleted Event",
    sizeof (BRWalletManagerWalletTxDeletedEvent),
    (BREventDispatcher) bwmSignalTxDeletedDispatcher
};

extern void
bwmSignalTxDeleted (BRWalletManager manager,
                    UInt256 hash,
                    int recommendRescan) {
    BRWalletManagerWalletTxDeletedEvent message =
    { { NULL, &bwmSignalTxDeletedEventType}, manager, hash, recommendRescan};
    eventHandlerSignalEvent (manager->handler, (BREvent*) &message);
}

//
// WalletManager Event
//

typedef struct {
    struct BREventRecord base;
    BRWalletManager manager;
    BRWalletManagerEvent event;
} BRWalletManagerWMEvent;

static void
bwmSignalWalletManagerWMEventDispatcher (BREventHandler ignore,
                                         BRWalletManagerWMEvent *event) {
    bwmHandleWalletManagerEvent(event->manager, event->event);
}

static BREventType bwmWalletManagerEventType = {
    "BTC: WalletManager Event",
    sizeof (BRWalletManagerWMEvent),
    (BREventDispatcher) bwmSignalWalletManagerWMEventDispatcher
};

extern void
bwmSignalWalletManagerEvent (BRWalletManager manager,
                             BRWalletManagerEvent event) {
    BRWalletManagerWMEvent message =
    { { NULL, &bwmWalletManagerEventType}, manager, event};
    eventHandlerSignalEvent (manager->handler, (BREvent*) &message);
}

//
// Wallet Event
//

typedef struct {
    struct BREventRecord base;
    BRWalletManager manager;
    BRWallet *wallet;
    BRWalletEvent event;
} BRWalletManagerWalletEvent;

static void
bwmSignalWalletEventDispatcher (BREventHandler ignore,
                                BRWalletManagerWalletEvent *event) {
    bwmHandleWalletEvent(event->manager, event->wallet, event->event);
}

static BREventType bwmWalletEventType = {
    "BTC: Wallet Event",
    sizeof (BRWalletManagerWalletEvent),
    (BREventDispatcher) bwmSignalWalletEventDispatcher
};

extern void
bwmSignalWalletEvent (BRWalletManager manager,
                      BRWallet *wallet,
                      BRWalletEvent event) {
    BRWalletManagerWalletEvent message =
    { { NULL, &bwmWalletEventType}, manager, wallet, event};
    eventHandlerSignalEvent (manager->handler, (BREvent*) &message);
}

//
// Wallet Event
//

typedef struct {
    struct BREventRecord base;
    BRWalletManager manager;
    BRWallet *wallet;
    BRTransaction *transaction;
    BRTransactionEvent event;
} BRWalletManagerTransactionEvent;

static void
bwmSignalTransactionEventDispatcher (BREventHandler ignore,
                                     BRWalletManagerTransactionEvent *event) {
    bwmHandleTransactionEvent(event->manager, event->wallet, event->transaction, event->event);
}

static BREventType bwmTransactionEventType = {
    "BTC: Transaction Event",
    sizeof (BRWalletManagerTransactionEvent),
    (BREventDispatcher) bwmSignalTransactionEventDispatcher
};

extern void
bwmSignalTransactionEvent (BRWalletManager manager,
                           BRWallet *wallet,
                           BRTransaction *transaction,
                           BRTransactionEvent event) {
    BRWalletManagerTransactionEvent message =
    { { NULL, &bwmTransactionEventType}, manager, wallet, transaction, event};
    eventHandlerSignalEvent (manager->handler, (BREvent*) &message);
}

//
// Announce Block Number
//

typedef struct {
    struct BREventRecord base;
    BRWalletManager manager;
    int rid;
    uint64_t blockNumber;
} BRWalletManagerClientAnnounceBlockNumberEvent;

static void
bwmSignalAnnounceBlockNumberDispatcher (BREventHandler ignore,
                                        BRWalletManagerClientAnnounceBlockNumberEvent *event) {
    bwmHandleAnnounceBlockNumber(event->manager, event->rid, event->blockNumber);
}

static BREventType bwmClientAnnounceBlockNumberEventType = {
    "BTC: Client Announce Block Number Event",
    sizeof (BRWalletManagerClientAnnounceBlockNumberEvent),
    (BREventDispatcher) bwmSignalAnnounceBlockNumberDispatcher
};

extern void
bwmSignalAnnounceBlockNumber (BRWalletManager manager,
                              int rid,
                              uint64_t blockNumber) {
    BRWalletManagerClientAnnounceBlockNumberEvent message =
    { { NULL, &bwmClientAnnounceBlockNumberEventType}, manager, rid, blockNumber};
    eventHandlerSignalEvent (manager->handler, (BREvent*) &message);
}

//
// Announce Transaction
//

typedef struct {
    struct BREventRecord base;
    BRWalletManager manager;
    int rid;
    uint8_t *transaction;
    size_t transactionLength;
    uint64_t timestamp;
    uint64_t blockHeight;
} BRWalletManagerClientAnnounceTransactionEvent;

static void
bwmSignalAnnounceTransactionDispatcher (BREventHandler ignore,
                                        BRWalletManagerClientAnnounceTransactionEvent *event) {
    bwmHandleAnnounceTransaction(event->manager,
                                 event->rid,
                                 event->transaction,
                                 event->transactionLength,
                                 event->timestamp,
                                 event->blockHeight);
    free (event->transaction);
}

static void
bwmSignalAnnounceTransactionDestroyer (BRWalletManagerClientAnnounceTransactionEvent *event) {
    free (event->transaction);
}

static BREventType bwmClientAnnounceTransactionEventType = {
    "BWM: Client Announce Transaction Event",
    sizeof (BRWalletManagerClientAnnounceTransactionEvent),
    (BREventDispatcher) bwmSignalAnnounceTransactionDispatcher,
    (BREventDestroyer) bwmSignalAnnounceTransactionDestroyer
};

extern void
bwmSignalAnnounceTransaction(BRWalletManager manager,
                             int rid,
                             OwnershipKept uint8_t *transaction,
                             size_t transactionLength,
                             uint64_t timestamp,
                             uint64_t blockHeight) {
    uint8_t *transactionCopy = malloc (transactionLength);
    memcpy (transactionCopy, transaction, transactionLength);

    BRWalletManagerClientAnnounceTransactionEvent message =
    { { NULL, &bwmClientAnnounceTransactionEventType}, manager, rid, transactionCopy, transactionLength, timestamp, blockHeight};
    eventHandlerSignalEvent (manager->handler, (BREvent*) &message);
}

//
// Announce Transaction Complete
//

typedef struct {
    struct BREventRecord base;
    BRWalletManager manager;
    int rid;
    int success;
} BRWalletManagerClientAnnounceTransactionCompleteEvent;

static void
bwmSignalAnnounceTransactionCompleteDispatcher (BREventHandler ignore,
                                     BRWalletManagerClientAnnounceTransactionCompleteEvent *event) {
    bwmHandleAnnounceTransactionComplete(event->manager, event->rid, event->success);
}

static BREventType bwmClientAnnounceTransactionCompleteEventType = {
    "BWM: Client Announce Get Transactions Complete Event",
    sizeof (BRWalletManagerClientAnnounceTransactionCompleteEvent),
    (BREventDispatcher) bwmSignalAnnounceTransactionCompleteDispatcher
};

extern void
bwmSignalAnnounceTransactionComplete (BRWalletManager manager,
                                      int rid,
                                      int success) {
    BRWalletManagerClientAnnounceTransactionCompleteEvent message =
    { { NULL, &bwmClientAnnounceTransactionCompleteEventType}, manager, rid, success };
    eventHandlerSignalEvent (manager->handler, (BREvent*) &message);
}

// Announce Submit

typedef struct {
    struct BREventRecord base;
    BRWalletManager manager;
    int rid;
    UInt256 txHash;
    int error;
} BRWalletManagerClientAnnounceSubmitEvent;

static void
bwmSignalAnnounceSubmitDispatcher (BREventHandler ignore,
                                   BRWalletManagerClientAnnounceSubmitEvent *event) {
    bwmHandleAnnounceSubmit(event->manager, event->rid, event->txHash, event->error);
}

static BREventType bwmClientAnnounceSubmitEventType = {
    "BTC: Client Announce Submit Event",
    sizeof (BRWalletManagerClientAnnounceSubmitEvent),
    (BREventDispatcher) bwmSignalAnnounceSubmitDispatcher
};

extern void
bwmSignalAnnounceSubmit (BRWalletManager manager,
                         int rid,
                         UInt256 txHash,
                         int error) {
    BRWalletManagerClientAnnounceSubmitEvent message =
    { { NULL, &bwmClientAnnounceSubmitEventType}, manager, rid, txHash, error};
    eventHandlerSignalEvent (manager->handler, (BREvent*) &message);
}

// ==============================================================================================
//
// All Event Types
//
const BREventType *bwmEventTypes[] = {
    &bwmSignalTxAddedEventType,
    &bwmSignalTxUpdatedEventType,
    &bwmSignalTxDeletedEventType,

    &bwmWalletManagerEventType,
    &bwmWalletEventType,
    &bwmTransactionEventType,

    &bwmClientAnnounceBlockNumberEventType,
    &bwmClientAnnounceTransactionEventType,
    &bwmClientAnnounceTransactionCompleteEventType,
    &bwmClientAnnounceSubmitEventType,
};

const unsigned int
bwmEventTypesCount = (sizeof (bwmEventTypes) / sizeof (BREventType*));
