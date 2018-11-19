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
    BREthereumTransfer transfer;
    BREthereumGas gasEstimate;
} BREthereumHandleGasEstimateEvent;

static void
ewmHandleGasEstimateEventDispatcher(BREventHandler ignore,
                                    BREthereumHandleGasEstimateEvent *event) {
    ewmHandleGasEstimate(event->ewm, event->wallet, event->transfer, event->gasEstimate);
}

BREventType handleGasEstimateEventType = {
    "EWM: Handle GasEstimate Event",
    sizeof (BREthereumHandleGasEstimateEvent),
    (BREventDispatcher) ewmHandleGasEstimateEventDispatcher
};

extern void
ewmSignalGasEstimate (BREthereumEWM ewm,
                      BREthereumWallet wallet,
                      BREthereumTransfer transfer,
                      BREthereumGas gasEstimate) {
    BREthereumHandleGasEstimateEvent event = { { NULL, &handleGasEstimateEventType }, ewm, wallet, transfer, gasEstimate };
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
                      OwnershipGiven BREthereumTransaction transaction) {
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
              OwnershipGiven BREthereumLog log) {
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
                     OwnershipGiven BRArrayOf(BREthereumBlock) blocks) {
    BREthereumHandleSaveBlocksEvent event = { { NULL, &handleSaveBlocksEventType }, ewm, blocks };
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle SaveNodes
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BRArrayOf(BREthereumNodeConfig) nodes;
} BREthereumHandleSaveNodesEvent;

static void
ewmHandleSaveNodesEventDispatcher(BREventHandler ignore,
                                   BREthereumHandleSaveNodesEvent *event) {
    ewmHandleSaveNodes(event->ewm, event->nodes);
}

BREventType handleSaveNodesEventType = {
    "EWM: Handle SaveNodes Event",
    sizeof (BREthereumHandleSaveNodesEvent),
    (BREventDispatcher) ewmHandleSaveNodesEventDispatcher
};

extern void
ewmSignalSaveNodes (BREthereumEWM ewm,
                    OwnershipGiven BRArrayOf(BREthereumNodeConfig) nodes) {
    BREthereumHandleSaveNodesEvent event = { { NULL, &handleSaveNodesEventType }, ewm, nodes };
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
// Handle GetBlocks
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumAddress address;
    BREthereumSyncInterestSet interests;
    uint64_t blockStart;
    uint64_t blockStop;
} BREthereumHandleGetBlocksEvent;

static void
ewmHandleGetBlocksEventDispatcher(BREventHandler ignore,
                                  BREthereumHandleGetBlocksEvent *event) {
    ewmHandleGetBlocks(event->ewm, event->address, event->interests, event->blockStart, event->blockStop);
}

BREventType handleGetBlocksEventType = {
    "EWM: Handle GetBlocks Event",
    sizeof (BREthereumHandleGetBlocksEvent),
    (BREventDispatcher) ewmHandleGetBlocksEventDispatcher
};

extern void
ewmSignalGetBlocks (BREthereumEWM ewm,
                    BREthereumAddress address,
                    BREthereumSyncInterestSet interests,
                    uint64_t blockStart,
                    uint64_t blockStop) {
    BREthereumHandleGetBlocksEvent event = { { NULL, &handleGetBlocksEventType }, ewm, address, interests, blockStart, blockStop };
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// All Handler Event Types
//
const BREventType *handlerForMainEventTypes[] = {
    &handleBlockChainEventType,
    &handleAccountStateEventType,
    &handleBalanceEventType,
    &handleGasPriceEventType,
    &handleGasEstimateEventType,
    &handleTransactionEventType,
    &handleLogEventType,
    &handleSaveBlocksEventType,
    &handleSaveNodesEventType,
    &handleSyncEventType,
    &handleGetBlocksEventType,

};
const unsigned int handlerForMainEventTypesCount = 10;

///
/// MARK: - Client
///

//
// Wallet Event
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumWallet wid;
    BREthereumWalletEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumEWMClientWalletEvent;

#define CLIENT_WALLET_EVENT_INITIALIZER(ewm, wid, vent, status, desc)  \
{ { NULL, &ewmClientWalletEventType }, (ewm), (wid), (event), (status), (desc) }

static void
ewmClientWalletEventDispatcher(BREventHandler ignore,
                               BREthereumEWMClientWalletEvent *event) {
    ewmClientHandleWalletEvent(event->ewm,
                               event->wid,
                               event->event,
                               event->status,
                               event->errorDescription);
}

static BREventType ewmClientWalletEventType = {
    "EMW: Client Wallet Event",
    sizeof (BREthereumEWMClientWalletEvent),
    (BREventDispatcher) ewmClientWalletEventDispatcher
};

extern void
ewmClientSignalWalletEvent(BREthereumEWM ewm,
                           BREthereumWallet wid,
                           BREthereumWalletEvent event,
                           BREthereumStatus status,
                           const char *errorDescription) {
    BREthereumEWMClientWalletEvent message =
    CLIENT_WALLET_EVENT_INITIALIZER (ewm, wid, event, status, errorDescription);
    eventHandlerSignalEvent(ewm->handlerForClient, (BREvent*) &message);
}

//
// Block Event
//
#if defined (NEVER_DEFINED)
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumBlock bid;
    BREthereumBlockEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumEWMClientBlockEvent;

#define CLIENT_BLOCK_EVENT_INITIALIZER(ewm, bid, event, status, desc)  \
{ { NULL, &ewmClientBlockEventType }, (ewm), (bid), (event), (status), (desc) }

static void
ewmClientBlockEventDispatcher(BREventHandler ignore,
                              BREthereumEWMClientBlockEvent *event) {
    ewmClientHandleBlockEvent(event->ewm,
                              event->bid,
                              event->event,
                              event->status,
                              event->errorDescription);
}

static BREventType ewmClientBlockEventType = {
    "EMW: Client Block Event",
    sizeof (BREthereumEWMClientBlockEvent),
    (BREventDispatcher) ewmClientBlockEventDispatcher
};

extern void
ewmClientSignalBlockEvent(BREthereumEWM ewm,
                          BREthereumBlock bid,
                          BREthereumBlockEvent event,
                          BREthereumStatus status,
                          const char *errorDescription) {
    BREthereumEWMClientBlockEvent message =
    CLIENT_BLOCK_EVENT_INITIALIZER (ewm, bid, event, status, errorDescription);
    eventHandlerSignalEvent(ewm->handlerForClient, (BREvent*) &message);
}
#endif

//
// Transaction Event
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumWallet wid;
    BREthereumTransfer tid;
    BREthereumTransferEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumEWMClientTransactionEvent;

#define CLIENT_TRANSACTION_EVENT_INITIALIZER(ewm, wid, tid, event, status, desc)  \
{ { NULL, &ewmClientTransactionEventType }, (ewm), (wid), (tid), (event), (status), (desc) }

static void
ewmClientTransactionEventDispatcher(BREventHandler ignore,
                                    BREthereumEWMClientTransactionEvent *event) {
    ewmClientHandleTransferEvent(event->ewm,
                                 event->wid,
                                 event->tid,
                                 event->event,
                                 event->status,
                                 event->errorDescription);
}

static BREventType ewmClientTransactionEventType = {
    "EMW: Client Transaction Event",
    sizeof (BREthereumEWMClientTransactionEvent),
    (BREventDispatcher) ewmClientTransactionEventDispatcher
};

extern void
ewmClientSignalTransferEvent(BREthereumEWM ewm,
                             BREthereumWallet wid,
                             BREthereumTransfer tid,
                             BREthereumTransferEvent event,
                             BREthereumStatus status,
                             const char *errorDescription) {
    BREthereumEWMClientTransactionEvent message =
    CLIENT_TRANSACTION_EVENT_INITIALIZER (ewm, wid, tid, event, status, errorDescription);
    eventHandlerSignalEvent(ewm->handlerForClient, (BREvent*) &message);
}

//
// Peer Event
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    // BREthereumWallet wid;
    // BREthereumTransaction tid;
    BREthereumPeerEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumEWMClientPeerEvent;

#define CLIENT_PEER_EVENT_INITIALIZER(ewm, /* wid, tid,*/ event, status, desc)  \
{ { NULL, &ewmClientPeerEventType }, (ewm), /*(wid), (tid),*/ (event), (status), (desc) }

static void
ewmClientPeerEventDispatcher(BREventHandler ignore,
                             BREthereumEWMClientPeerEvent *event) {
    ewmClientHandlePeerEvent(event->ewm, event->event, event->status, event->errorDescription);
}

static BREventType ewmClientPeerEventType = {
    "EMW: Client Peer Event",
    sizeof (BREthereumEWMClientPeerEvent),
    (BREventDispatcher) ewmClientPeerEventDispatcher
};

extern void
ewmClientSignalPeerEvent(BREthereumEWM ewm,
                         // BREthereumWallet wid,
                         // BREthereumTransaction tid,
                         BREthereumPeerEvent event,
                         BREthereumStatus status,
                         const char *errorDescription) {
    BREthereumEWMClientPeerEvent message =
    CLIENT_PEER_EVENT_INITIALIZER (ewm, /* wid, tid,*/ event, status, errorDescription);
    eventHandlerSignalEvent(ewm->handlerForClient, (BREvent*) &message);
}

//
// EWM Event
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    // BREthereumWallet wid;
    // BREthereumTransaction tid;
    BREthereumEWMEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumEWMClientEWMEvent;

#define CLIENT_EWM_EVENT_INITIALIZER(ewm, /* wid, tid,*/ event, status, desc)  \
{ { NULL, &ewmClientEWMEventType }, (ewm), /*(wid), (tid),*/ (event), (status), (desc) }

static void
ewmClientEWMEventDispatcher(BREventHandler ignore,
                            BREthereumEWMClientEWMEvent *event) {
    ewmClientHandleEWMEvent(event->ewm, event->event, event->status, event->errorDescription);
}

static BREventType ewmClientEWMEventType = {
    "EMW: Client EWM Event",
    sizeof (BREthereumEWMClientEWMEvent),
    (BREventDispatcher) ewmClientEWMEventDispatcher
};

extern void
ewmClientSignalEWMEvent(BREthereumEWM ewm,
                        // BREthereumWallet wid,
                        // BREthereumTransaction tid,
                        BREthereumEWMEvent event,
                        BREthereumStatus status,
                        const char *errorDescription) {
    BREthereumEWMClientEWMEvent message =
    CLIENT_EWM_EVENT_INITIALIZER (ewm, /* wid, tid,*/ event, status, errorDescription);
    eventHandlerSignalEvent(ewm->handlerForClient, (BREvent*) &message);
}

//
// Announce Block Number
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    uint64_t blockNumber;
    int rid;
} BREthereumEWMClientAnnounceBlockNumberEvent;

static void
ewmClientSignalAnnounceBlockNumberDispatcher (BREventHandler ignore,
                                              BREthereumEWMClientAnnounceBlockNumberEvent *event) {
    ewmClientHandleAnnounceBlockNumber(event->ewm, event->blockNumber, event->rid);
}

static BREventType ewmClientAnnounceBlockNumberEventType = {
    "EWM: Client Announce Block Number Event",
    sizeof (BREthereumEWMClientAnnounceBlockNumberEvent),
    (BREventDispatcher) ewmClientSignalAnnounceBlockNumberDispatcher
};

extern void
ewmClientSignalAnnounceBlockNumber (BREthereumEWM ewm,
                                    uint64_t blockNumber,
                                    int rid) {
    BREthereumEWMClientAnnounceBlockNumberEvent message =
    { { NULL, &ewmClientAnnounceBlockNumberEventType}, ewm, blockNumber, rid};
    eventHandlerSignalEvent (ewm->handlerForClient, (BREvent*) &message);
}

//
// Announce Nonce
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumAddress address;
    uint64_t nonce;
    int rid;
} BREthereumEWMClientAnnounceNonceEvent;

static void
ewmClientSignalAnnounceNonceDispatcher (BREventHandler ignore,
                                        BREthereumEWMClientAnnounceNonceEvent *event) {
    ewmClientHandleAnnounceNonce(event->ewm, event->address, event->nonce, event->rid);
}

static BREventType ewmClientAnnounceNonceEventType = {
    "EWM: Client Announce Block Number Event",
    sizeof (BREthereumEWMClientAnnounceNonceEvent),
    (BREventDispatcher) ewmClientSignalAnnounceNonceDispatcher
};

extern void
ewmClientSignalAnnounceNonce (BREthereumEWM ewm,
                              BREthereumAddress address,
                              uint64_t nonce,
                              int rid) {
    BREthereumEWMClientAnnounceNonceEvent message =
    { { NULL, &ewmClientAnnounceNonceEventType}, ewm, address, nonce, rid};
    eventHandlerSignalEvent (ewm->handlerForClient, (BREvent*) &message);
}

//
// Announce Balance
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumWallet wallet;
    UInt256 value;
    int rid;
} BREthereumEWMClientAnnounceBalanceEvent;

static void
ewmClientSignalAnnounceBalanceDispatcher (BREventHandler ignore,
                                          BREthereumEWMClientAnnounceBalanceEvent *event) {
    ewmClientHandleAnnounceBalance(event->ewm, event->wallet, event->value, event->rid);
}

static BREventType ewmClientAnnounceBalanceEventType = {
    "EWM: Client Announce Balance Event",
    sizeof (BREthereumEWMClientAnnounceBalanceEvent),
    (BREventDispatcher) ewmClientSignalAnnounceBalanceDispatcher
};

extern void
ewmClientSignalAnnounceBalance (BREthereumEWM ewm,
                                BREthereumWallet wallet,
                                UInt256 value,
                                int rid) {
    BREthereumEWMClientAnnounceBalanceEvent message =
    { { NULL, &ewmClientAnnounceBalanceEventType}, ewm, wallet, value, rid};
    eventHandlerSignalEvent (ewm->handlerForClient, (BREvent*) &message);
}

//
// Announce Gas Price
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumWallet wallet;
    UInt256 amount;
    int rid;
} BREthereumEWMClientAnnounceGasPriceEvent;

static void
ewmClientSignalAnnounceGasPriceDispatcher (BREventHandler ignore,
                                           BREthereumEWMClientAnnounceGasPriceEvent *event) {
    ewmClientHandleAnnounceGasPrice(event->ewm, event->wallet, event->amount, event->rid);
}

static BREventType ewmClientAnnounceGasPriceEventType = {
    "EWM: Client Announce GasPrice Event",
    sizeof (BREthereumEWMClientAnnounceGasPriceEvent),
    (BREventDispatcher) ewmClientSignalAnnounceGasPriceDispatcher
};

extern void
ewmClientSignalAnnounceGasPrice (BREthereumEWM ewm,
                                 BREthereumWallet wallet,
                                 UInt256 amount,
                                 int rid) {
    BREthereumEWMClientAnnounceGasPriceEvent message =
    { { NULL, &ewmClientAnnounceGasPriceEventType}, ewm, wallet, amount, rid};
    eventHandlerSignalEvent (ewm->handlerForClient, (BREvent*) &message);
}

//
// Announce Gas Estimate
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumWallet wallet;
    BREthereumTransfer transfer;
    UInt256 value;
    int rid;
} BREthereumEWMClientAnnounceGasEstimateEvent;

static void
ewmClientSignalAnnounceGasEstimateDispatcher (BREventHandler ignore,
                                              BREthereumEWMClientAnnounceGasEstimateEvent *event) {
    ewmClientHandleAnnounceGasEstimate(event->ewm, event->wallet, event->transfer, event->value, event->rid);
}

static BREventType ewmClientAnnounceGasEstimateEventType = {
    "EWM: Client Announce GasEstimate Event",
    sizeof (BREthereumEWMClientAnnounceGasEstimateEvent),
    (BREventDispatcher) ewmClientSignalAnnounceGasEstimateDispatcher
};

extern void
ewmClientSignalAnnounceGasEstimate (BREthereumEWM ewm,
                                    BREthereumWallet wallet,
                                    BREthereumTransfer transfer,
                                    UInt256 value,
                                    int rid) {
    BREthereumEWMClientAnnounceGasEstimateEvent message =
    { { NULL, &ewmClientAnnounceGasEstimateEventType}, ewm, wallet, transfer, value, rid};
    eventHandlerSignalEvent (ewm->handlerForClient, (BREvent*) &message);
}

//
// Announce Submit Transaction
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumWallet wallet;
    BREthereumTransfer transfer;
    int rid;
} BREthereumEWMClientAnnounceSubmitTransferEvent;

static void
ewmClientSignalAnnounceSubmitTransferDispatcher (BREventHandler ignore,
                                                 BREthereumEWMClientAnnounceSubmitTransferEvent *event) {
    ewmClientHandleAnnounceSubmitTransfer(event->ewm, event->wallet, event->transfer, event->rid);
}

static BREventType ewmClientAnnounceSubmitTransferEventType = {
    "EWM: Client Announce SubmitTransfer Event",
    sizeof (BREthereumEWMClientAnnounceSubmitTransferEvent),
    (BREventDispatcher) ewmClientSignalAnnounceSubmitTransferDispatcher
};

extern void
ewmClientSignalAnnounceSubmitTransfer (BREthereumEWM ewm,
                                       BREthereumWallet wallet,
                                       BREthereumTransfer transfer,
                                       int rid) {
    BREthereumEWMClientAnnounceSubmitTransferEvent message =
    { { NULL, &ewmClientAnnounceSubmitTransferEventType}, ewm, wallet, transfer, rid};
    eventHandlerSignalEvent (ewm->handlerForClient, (BREvent*) &message);
}

//
// Announce Transaction
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumEWMClientAnnounceTransactionBundle *bundle;
    int rid;
} BREthereumEWMClientAnnounceTransactionEvent;

static void
ewmClientSignalAnnounceTransactionDispatcher (BREventHandler ignore,
                                              BREthereumEWMClientAnnounceTransactionEvent *event) {
    ewmClientHandleAnnounceTransaction(event->ewm, event->bundle, event->rid);
}

static void
ewmClientSignalAnnounceTransactionDestroyer (BREthereumEWMClientAnnounceTransactionEvent *event) {
    ewmClientAnnounceTransactionBundleRelease(event->bundle);
}

static BREventType ewmClientAnnounceTransactionEventType = {
    "EWM: Client Announce Transaction Event",
    sizeof (BREthereumEWMClientAnnounceTransactionEvent),
    (BREventDispatcher) ewmClientSignalAnnounceTransactionDispatcher,
    (BREventDestroyer) ewmClientSignalAnnounceTransactionDestroyer
};

extern void
ewmClientSignalAnnounceTransaction(BREthereumEWM ewm,
                                   BREthereumEWMClientAnnounceTransactionBundle *bundle,
                                   int rid) {
    BREthereumEWMClientAnnounceTransactionEvent message =
    { { NULL, &ewmClientAnnounceTransactionEventType}, ewm, bundle, rid};
    eventHandlerSignalEvent (ewm->handlerForClient, (BREvent*) &message);
}

//
// Announce Log
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumEWMClientAnnounceLogBundle *bundle;
    int rid;
} BREthereumEWMClientAnnounceLogEvent;

static void
ewmClientSignalAnnounceLogDispatcher (BREventHandler ignore,
                                      BREthereumEWMClientAnnounceLogEvent *event) {
    ewmClientHandleAnnounceLog(event->ewm, event->bundle, event->rid);
}

static void
ewmClientSignalAnnounceLogDestroyer (BREthereumEWMClientAnnounceLogEvent *event) {
    ewmClientAnnounceLogBundleRelease(event->bundle);
}

static BREventType ewmClientAnnounceLogEventType = {
    "EWM: Client Announce Log Event",
    sizeof (BREthereumEWMClientAnnounceLogEvent),
    (BREventDispatcher) ewmClientSignalAnnounceLogDispatcher,
    (BREventDestroyer) ewmClientSignalAnnounceLogDestroyer
};

extern void
ewmClientSignalAnnounceLog (BREthereumEWM ewm,
                            BREthereumEWMClientAnnounceLogBundle *bundle,
                            int rid) {
    BREthereumEWMClientAnnounceLogEvent message =
    { { NULL, &ewmClientAnnounceLogEventType}, ewm, bundle, rid};
    eventHandlerSignalEvent (ewm->handlerForClient, (BREvent*) &message);
}

//
// Announce Token
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumEWMClientAnnounceTokenBundle *bundle;
    int rid;
} BREthereumEWMClientAnnounceTokenEvent;

static void
ewmClientSignalAnnounceTokenDispatcher (BREventHandler ignore,
                                        BREthereumEWMClientAnnounceTokenEvent *event) {
    ewmClientHandleAnnounceToken(event->ewm, event->bundle, event->rid);
}

static void
ewmClientSignalAnnounceTokenDestroyer (BREthereumEWMClientAnnounceTokenEvent *event) {
    ewmClientAnnounceTokenBundleRelease(event->bundle);
}

static BREventType ewmClientAnnounceTokenEventType = {
    "EWM: Client Announce Token Event",
    sizeof (BREthereumEWMClientAnnounceTokenEvent),
    (BREventDispatcher) ewmClientSignalAnnounceTokenDispatcher,
    (BREventDestroyer) ewmClientSignalAnnounceTokenDestroyer
};

extern void
ewmClientSignalAnnounceToken (BREthereumEWM ewm,
                              BREthereumEWMClientAnnounceTokenBundle *bundle,
                              int rid) {
    BREthereumEWMClientAnnounceTokenEvent message =
    { { NULL, &ewmClientAnnounceTokenEventType}, ewm, bundle, rid};
    eventHandlerSignalEvent (ewm->handlerForClient, (BREvent*) &message);
}

//
// All Listener Event Types
//
const BREventType *
handlerForClientEventTypes[] = {
    &ewmClientWalletEventType,
    //    &ewmClientBlockEventType,
    &ewmClientTransactionEventType,
    &ewmClientPeerEventType,
    &ewmClientEWMEventType,
    &ewmClientAnnounceBlockNumberEventType,
    &ewmClientAnnounceNonceEventType,
    &ewmClientAnnounceBalanceEventType,
    &ewmClientAnnounceGasPriceEventType,
    &ewmClientAnnounceGasEstimateEventType,
    &ewmClientAnnounceSubmitTransferEventType,
    &ewmClientAnnounceTransactionEventType,
    &ewmClientAnnounceLogEventType,
    &ewmClientAnnounceTokenEventType,
};
const unsigned int
handlerForClientEventTypesCount = (sizeof (handlerForClientEventTypes) / sizeof (BREventType*)); // 14;

