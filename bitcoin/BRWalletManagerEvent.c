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
    BRTransaction *transaction;
} BRWalletManagerClientAnnounceTransactionEvent;

static void
bwmSignalAnnounceTransactionDispatcher (BREventHandler ignore,
                                        BRWalletManagerClientAnnounceTransactionEvent *event) {
    bwmHandleAnnounceTransaction(event->manager, event->rid, event->transaction);
}

static void
bwmSignalAnnounceTransactionDestroyer (BRWalletManagerClientAnnounceTransactionEvent *event) {
    assert (event->transaction);
    BRTransactionFree (event->transaction);
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
                             OwnershipGiven BRTransaction *transaction) {
    BRWalletManagerClientAnnounceTransactionEvent message =
    { { NULL, &bwmClientAnnounceTransactionEventType}, manager, rid, transaction};
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
    BRTransaction *transaction;
    int error;
} BRWalletManagerClientAnnounceSubmitEvent;

static void
bwmSignalAnnounceSubmitDispatcher (BREventHandler ignore,
                                   BRWalletManagerClientAnnounceSubmitEvent *event) {
    bwmHandleAnnounceSubmit(event->manager, event->rid, event->transaction, event->error);
}

static void
bwmSignalAnnounceSubmitDestroyer (BRWalletManagerClientAnnounceSubmitEvent *event) {
    assert (event->transaction);
    BRTransactionFree (event->transaction);
}

static BREventType bwmClientAnnounceSubmitEventType = {
    "BTC: Client Announce Submit Event",
    sizeof (BRWalletManagerClientAnnounceSubmitEvent),
    (BREventDispatcher) bwmSignalAnnounceSubmitDispatcher,
    (BREventDestroyer) bwmSignalAnnounceSubmitDestroyer
};

extern void
bwmSignalAnnounceSubmit (BRWalletManager manager,
                         int rid,
                         OwnershipGiven BRTransaction *transaction,
                         int error) {
    BRWalletManagerClientAnnounceSubmitEvent message =
    { { NULL, &bwmClientAnnounceSubmitEventType}, manager, rid, transaction, error};
    eventHandlerSignalEvent (manager->handler, (BREvent*) &message);
}

// ==============================================================================================
//
// All Event Types
//
const BREventType *bwmEventTypes[] = {
    &bwmClientAnnounceBlockNumberEventType,
    &bwmClientAnnounceTransactionEventType,
    &bwmClientAnnounceTransactionCompleteEventType,
    &bwmClientAnnounceSubmitEventType,
};

const unsigned int
bwmEventTypesCount = (sizeof (bwmEventTypes) / sizeof (BREventType*));
