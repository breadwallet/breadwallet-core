//
//  BREthereumEWMEvent.c
//  BRCore
//
//  Created by Ed Gamble on 5/7/18.
//  Copyright (c) 2018 breadwallet LLC
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

#include "BRArray.h"
#include "BREthereumEWMPrivate.h"

/*!
 * A EWM has Listeners 'subscribed' to Events - Wallet, Transaction, Block, Peer and
 * EWM Events.  The subscribed listeners should not run on the 'main' EWM queue;
 * instead we run listeners on their own queue.
 *
 * Herein we define Listener Events, the Dispatcher to handle Events (which turns around and
 * invokes the subscribed method, and a convenience function to queue an event on the
 * 'listener' queue.
 */

//
// Wallet Event
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumWalletId wid;
    BREthereumWalletEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumListenerWalletEvent;

#define LISTENER_WALLET_EVENT_INITIALIZER(ewm, wid, vent, status, desc)  \
{ { NULL, &listenerWalletEventType }, (ewm), (wid), (event), (status), (desc) }

static void
ewmListenerWalletEventDispatcher(BREventHandler ignore,
                                 BREthereumListenerWalletEvent *event) {
    ewmListenerHandleWalletEvent(event->ewm,
                                 event->wid,
                                 event->event,
                                 event->status,
                                 event->errorDescription);
}

static BREventType listenerWalletEventType = {
    "EWM: Listener Wallet Event",
    sizeof (BREthereumListenerWalletEvent),
    (BREventDispatcher) ewmListenerWalletEventDispatcher
};

/*!
 * Add a WalletEvent to the EWM's Listener Queue
 */
extern void
ewmListenerSignalWalletEvent(BREthereumEWM ewm,
                             BREthereumWalletId wid,
                             BREthereumWalletEvent event,
                             BREthereumStatus status,
                             const char *errorDescription) {
    BREthereumListenerWalletEvent message =
    LISTENER_WALLET_EVENT_INITIALIZER (ewm, wid, event, status, errorDescription);
    eventHandlerSignalEvent(ewm->handlerForListener, (BREvent*) &message);
}

//
// Block Event
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumBlockId bid;
    BREthereumBlockEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumListenerBlockEvent;

#define LISTENER_BLOCK_EVENT_INITIALIZER(ewm, bid, event, status, desc)  \
{ { NULL, &listenerBlockEventType }, (ewm), (bid), (event), (status), (desc) }

static void
ewmListenerBlockEventDispatcher(BREventHandler ignore,
                                BREthereumListenerBlockEvent *event) {
    ewmListenerHandleBlockEvent(event->ewm,
                                event->bid,
                                event->event,
                                event->status,
                                event->errorDescription);
}

static BREventType listenerBlockEventType = {
    "EWM: Listener Block Event",
    sizeof (BREthereumListenerBlockEvent),
    (BREventDispatcher) ewmListenerBlockEventDispatcher
};

/*!
 * Add a BlockEvent to the EWM's Listener Queue
 */
extern void
ewmListenerSignalBlockEvent(BREthereumEWM ewm,
                            BREthereumBlockId bid,
                            BREthereumBlockEvent event,
                            BREthereumStatus status,
                            const char *errorDescription) {
    BREthereumListenerBlockEvent message =
    LISTENER_BLOCK_EVENT_INITIALIZER (ewm, bid, event, status, errorDescription);
    eventHandlerSignalEvent(ewm->handlerForListener, (BREvent*) &message);
}

//
// Transaction Event
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumWalletId wid;
    BREthereumTransactionId tid;
    BREthereumTransactionEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumListenerTransactionEvent;

#define LISTENER_TRANSACTION_EVENT_INITIALIZER(ewm, wid, tid, event, status, desc)  \
{ { NULL, &listenerTransactionEventType }, (ewm), (wid), (tid), (event), (status), (desc) }

static void
ewmListenerTransactionEventDispatcher(BREventHandler ignore,
                                      BREthereumListenerTransactionEvent *event) {
    ewmListenerHandleTransactionEvent(event->ewm,
                                      event->wid,
                                      event->tid,
                                      event->event,
                                      event->status,
                                      event->errorDescription);
}

static BREventType listenerTransactionEventType = {
    "EWM: Listener Transaction Event",
    sizeof (BREthereumListenerTransactionEvent),
    (BREventDispatcher) ewmListenerTransactionEventDispatcher
};

/*!
 * Add a TransactionEvent to the EWM's Listener Queue
 */
extern void
ewmListenerSignalTransactionEvent(BREthereumEWM ewm,
                                  BREthereumWalletId wid,
                                  BREthereumTransactionId tid,
                                  BREthereumTransactionEvent event,
                                  BREthereumStatus status,
                                  const char *errorDescription) {
    BREthereumListenerTransactionEvent message =
    LISTENER_TRANSACTION_EVENT_INITIALIZER (ewm, wid, tid, event, status, errorDescription);
    eventHandlerSignalEvent(ewm->handlerForListener, (BREvent*) &message);
}

//
// Peer Event
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    // BREthereumWalletId wid;
    // BREthereumTransactionId tid;
    BREthereumPeerEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumListenerPeerEvent;

#define LISTENER_PEER_EVENT_INITIALIZER(ewm, /* wid, tid,*/ event, status, desc)  \
{ { NULL, &listenerPeerEventType }, (ewm), /*(wid), (tid),*/ (event), (status), (desc) }

static void
ewmListenerPeerEventDispatcher(BREventHandler ignore,
                               BREthereumListenerPeerEvent *event) {
    ewmListenerHandlePeerEvent(event->ewm, event->event, event->status, event->errorDescription);
}

static BREventType listenerPeerEventType = {
    "EWM: Listener Peer Event",
    sizeof (BREthereumListenerPeerEvent),
    (BREventDispatcher) ewmListenerPeerEventDispatcher
};

/*!
 * Add a PeerEvent to the EWM's Listener Queue
 */
extern void
ewmListenerSignalPeerEvent(BREthereumEWM ewm,
                           // BREthereumWalletId wid,
                           // BREthereumTransactionId tid,
                           BREthereumPeerEvent event,
                           BREthereumStatus status,
                           const char *errorDescription) {
    BREthereumListenerPeerEvent message =
    LISTENER_PEER_EVENT_INITIALIZER (ewm, /* wid, tid,*/ event, status, errorDescription);
    eventHandlerSignalEvent(ewm->handlerForListener, (BREvent*) &message);
}

//
// EWM Event
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    // BREthereumWalletId wid;
    // BREthereumTransactionId tid;
    BREthereumEWMEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumListenerEWMEvent;

#define LISTENER_EWM_EVENT_INITIALIZER(ewm, /* wid, tid,*/ event, status, desc)  \
{ { NULL, &listenerEWMEventType }, (ewm), /*(wid), (tid),*/ (event), (status), (desc) }

static void
ewmListenerEWMEventDispatcher(BREventHandler ignore,
                                    BREthereumListenerEWMEvent *event) {
    ewmListenerHandleEWMEvent(event->ewm, event->event, event->status, event->errorDescription);
}

static BREventType listenerEWMEventType = {
    "EWM: Listener EWM Event",
    sizeof (BREthereumListenerEWMEvent),
    (BREventDispatcher) ewmListenerEWMEventDispatcher
};

/*!
 * Add a EWMEvent to the EWM's Listener Queue
 */
extern void
ewmListenerSignalEWMEvent(BREthereumEWM ewm,
                                // BREthereumWalletId wid,
                                // BREthereumTransactionId tid,
                                BREthereumEWMEvent event,
                                BREthereumStatus status,
                                const char *errorDescription) {
    BREthereumListenerEWMEvent message =
    LISTENER_EWM_EVENT_INITIALIZER (ewm, /* wid, tid,*/ event, status, errorDescription);
    eventHandlerSignalEvent(ewm->handlerForListener, (BREvent*) &message);
}

//
// All Listener Event Types
//
const BREventType *listenerEventTypes[] = {
    &listenerWalletEventType,
    &listenerBlockEventType,
    &listenerTransactionEventType,
    &listenerPeerEventType,
    &listenerEWMEventType
};
const unsigned int listenerEventTypesCount = 5;

// ==============================================================================================
// ==============================================================================================

/*!
 * Define the Events handled on the EWM's Main queue.
 */

// ==============================================================================================
//
// Handle Block Chain
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumHash headBlockHash;
    uint64_t headBlockNumber;
    uint64_t headBlockTimestamp;
} BREthereumHandleBlockChainEvent;

static void
ewmHandleBlockChainEventDispatcher(BREventHandler ignore,
                                   BREthereumHandleBlockChainEvent *event) {
    ewmHandleBlockChain(event->ewm,
                        event->headBlockHash,
                        event->headBlockNumber,
                        event->headBlockTimestamp);
}

BREventType handleBlockChainEventType = {
    "EWM: Handle BlockChain Event",
    sizeof (BREthereumHandleBlockChainEvent),
    (BREventDispatcher) ewmHandleBlockChainEventDispatcher
};

extern void
ewmSignalBlockChain (BREthereumEWM ewm,
                     BREthereumHash headBlockHash,
                     uint64_t headBlockNumber,
                     uint64_t headBlockTimestamp) {

    BREthereumHandleBlockChainEvent event = { { NULL, &handleBlockChainEventType }, ewm,
        headBlockHash,
        headBlockNumber,
        headBlockTimestamp };
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &event);
}


// ==============================================================================================
//
// Handle AccountState
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumAccountState accountState;
} BREthereumHandleAccountStateEvent;

static void
ewmHandleAccountStateEventDispatcher(BREventHandler ignore,
                                BREthereumHandleAccountStateEvent *event) {
    ewmHandleAccountState(event->ewm, event->accountState);
}

BREventType handleAccountStateEventType = {
    "EWM: Handle AccountState Event",
    sizeof (BREthereumHandleAccountStateEvent),
    (BREventDispatcher) ewmHandleAccountStateEventDispatcher
};

extern void
ewmSignalAccountState (BREthereumEWM ewm,
                  BREthereumAccountState accountState) {
    BREthereumHandleAccountStateEvent event = { { NULL, &handleAccountStateEventType }, ewm, accountState };
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Balance
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumAmount amount;
} BREthereumHandleBalanceEvent;

static void
ewmHandleBalanceEventDispatcher(BREventHandler ignore,
                              BREthereumHandleBalanceEvent *event) {
    ewmHandleBalance(event->ewm, event->amount);
}

BREventType handleBalanceEventType = {
    "EWM: Handle Balance Event",
    sizeof (BREthereumHandleBalanceEvent),
    (BREventDispatcher) ewmHandleBalanceEventDispatcher
};

extern void
ewmSignalBalance (BREthereumEWM ewm,
                  BREthereumAmount amount) {
    BREthereumHandleBalanceEvent event = { { NULL, &handleBalanceEventType }, ewm, amount };
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Gas Price
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumWallet wallet;
    BREthereumGasPrice gasPrice;
} BREthereumHandleGasPriceEvent;

static void
ewmHandleGasPriceEventDispatcher(BREventHandler ignore,
                                 BREthereumHandleGasPriceEvent *event) {
    ewmHandleGasPrice(event->ewm, event->wallet, event->gasPrice);
}

BREventType handleGasPriceEventType = {
    "EWM: Handle GasPrice Event",
    sizeof (BREthereumHandleGasPriceEvent),
    (BREventDispatcher) ewmHandleGasPriceEventDispatcher
};

extern void
ewmSignalGasPrice (BREthereumEWM ewm,
                   BREthereumWallet wallet,
                   BREthereumGasPrice gasPrice) {
    BREthereumHandleGasPriceEvent event = { { NULL, &handleGasPriceEventType }, ewm, wallet, gasPrice };
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Gas Estimate
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumWallet wallet;
    BREthereumTransaction transaction;
    BREthereumGas gasEstimate;
} BREthereumHandleGasEstimateEvent;

static void
ewmHandleGasEstimateEventDispatcher(BREventHandler ignore,
                                    BREthereumHandleGasEstimateEvent *event) {
    ewmHandleGasEstimate(event->ewm, event->wallet, event->transaction, event->gasEstimate);
}

BREventType handleGasEstimateEventType = {
    "EWM: Handle GasEstimate Event",
    sizeof (BREthereumHandleGasEstimateEvent),
    (BREventDispatcher) ewmHandleGasEstimateEventDispatcher
};

extern void
ewmSignalGasEstimate (BREthereumEWM ewm,
                      BREthereumWallet wallet,
                      BREthereumTransaction transaction,
                      BREthereumGas gasEstimate) {
    BREthereumHandleGasEstimateEvent event = { { NULL, &handleGasEstimateEventType }, ewm, wallet, transaction, gasEstimate };
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Transaction
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumBCSCallbackTransactionType type;
    BREthereumTransaction transaction;
} BREthereumHandleTransactionEvent;

static void
ewmHandleTransactionEventDispatcher(BREventHandler ignore,
                                    BREthereumHandleTransactionEvent *event) {
    ewmHandleTransaction(event->ewm, event->type, event->transaction);
}

BREventType handleTransactionEventType = {
    "EWM: Handle Transaction Event",
    sizeof (BREthereumHandleTransactionEvent),
    (BREventDispatcher) ewmHandleTransactionEventDispatcher
};

extern void
ewmSignalTransaction (BREthereumEWM ewm,
                      BREthereumBCSCallbackTransactionType type,
                      BREthereumTransaction transaction) {
    BREthereumHandleTransactionEvent event = { { NULL, &handleTransactionEventType }, ewm, type, transaction };
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Log
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumBCSCallbackLogType type;
    BREthereumLog log;
} BREthereumHandleLogEvent;

static void
ewmHandleLogEventDispatcher(BREventHandler ignore,
                            BREthereumHandleLogEvent *event) {
    ewmHandleLog(event->ewm, event->type, event->log);
}

BREventType handleLogEventType = {
    "EWM: Handle Log Event",
    sizeof (BREthereumHandleLogEvent),
    (BREventDispatcher) ewmHandleLogEventDispatcher
};

extern void
ewmSignalLog (BREthereumEWM ewm,
              BREthereumBCSCallbackLogType type,
              BREthereumLog log) {
    BREthereumHandleLogEvent event = { { NULL, &handleLogEventType }, ewm, type, log };
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle SaveBlocks
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BRArrayOf(BREthereumBlock) blocks;
} BREthereumHandleSaveBlocksEvent;

static void
ewmHandleSaveBlocksEventDispatcher(BREventHandler ignore,
                             BREthereumHandleSaveBlocksEvent *event) {
    ewmHandleSaveBlocks(event->ewm, event->blocks);
}

static void
ewmHandleSaveBlocksEventDestroyer (BREthereumHandleSaveBlocksEvent *event) {
    if (NULL != event->blocks)
        array_free(event->blocks);
}

BREventType handleSaveBlocksEventType = {
    "EWM: Handle SaveBlocks Event",
    sizeof (BREthereumHandleSaveBlocksEvent),
    (BREventDispatcher) ewmHandleSaveBlocksEventDispatcher,
    (BREventDestroyer) ewmHandleSaveBlocksEventDestroyer
};

extern void
ewmSignalSaveBlocks (BREthereumEWM ewm,
                     BRArrayOf(BREthereumBlock) blocks) {
    BREthereumHandleSaveBlocksEvent event = { { NULL, &handleSaveBlocksEventType }, ewm, blocks };
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle SavePeers
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
} BREthereumHandleSavePeersEvent;

static void
ewmHandleSavePeersEventDispatcher(BREventHandler ignore,
                                   BREthereumHandleSavePeersEvent *event) {
    ewmHandleSavePeers(event->ewm);
}

BREventType handleSavePeersEventType = {
    "EWM: Handle SavePeers Event",
    sizeof (BREthereumHandleSavePeersEvent),
    (BREventDispatcher) ewmHandleSavePeersEventDispatcher
};

extern void
ewmSignalSavePeers (BREthereumEWM ewm) {
    BREthereumHandleSavePeersEvent event = { { NULL, &handleSavePeersEventType }, ewm };
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &event);
}



// ==============================================================================================
//
// Handle Sync
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumBCSCallbackSyncType type;
    uint64_t blockNumberStart;
    uint64_t blockNumberCurrent;
    uint64_t blockNumberStop;

} BREthereumHandleSyncEvent;

static void
ewmHandleSyncEventDispatcher(BREventHandler ignore,
                            BREthereumHandleSyncEvent *event) {
    ewmHandleSync(event->ewm, event->type,
                  event->blockNumberStart,
                  event->blockNumberCurrent,
                  event->blockNumberStop);
}

BREventType handleSyncEventType = {
    "EWM: Handle Sync Event",
    sizeof (BREthereumHandleSyncEvent),
    (BREventDispatcher) ewmHandleSyncEventDispatcher
};

extern void
ewmSignalSync (BREthereumEWM ewm,
               BREthereumBCSCallbackSyncType type,
               uint64_t blockNumberStart,
               uint64_t blockNumberCurrent,
               uint64_t blockNumberStop) {

    BREthereumHandleSyncEvent event = { { NULL, &handleSyncEventType }, ewm, type,
        blockNumberStart,
        blockNumberCurrent,
        blockNumberStop };
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// All Handler Event Types
//
const BREventType *handlerEventTypes[] = {
    &handleBlockChainEventType,
    &handleAccountStateEventType,
    &handleBalanceEventType,
    &handleGasPriceEventType,
    &handleGasEstimateEventType,
    &handleTransactionEventType,
    &handleLogEventType,
    &handleSaveBlocksEventType,
    &handleSavePeersEventType,
    &handleSyncEventType
};
const unsigned int handlerEventTypesCount = 10;
