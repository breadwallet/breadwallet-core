//
//  BREthereumEWMEvent.c
//  BRCore
//
//  Created by Ed Gamble on 5/7/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "support/BRArray.h"
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
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &event);
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
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &event);
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
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &event);
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
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Gas Estimate
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumWallet wallet;
    BREthereumCookie cookie;
    BREthereumStatus status;
    BREthereumGas gasEstimate;
    BREthereumGasPrice gasPrice;
} BREthereumHandleGasEstimateEvent;

static void
ewmHandleGasEstimateEventDispatcher(BREventHandler ignore,
                                    BREthereumHandleGasEstimateEvent *event) {
    if (SUCCESS == event->status) {
        ewmHandlGasEstimateSuccess (event->ewm, event->wallet, event->cookie, event->gasEstimate, event->gasPrice);
    } else {
        ewmHandlGasEstimateFailure (event->ewm, event->wallet, event->cookie, event->status);
    }
}

BREventType handleGasEstimateEventType = {
    "EWM: Handle GasEstimate Event",
    sizeof (BREthereumHandleGasEstimateEvent),
    (BREventDispatcher) ewmHandleGasEstimateEventDispatcher
};

extern void
ewmSignalGasEstimateSuccess(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumCookie cookie,
                            BREthereumGas gasEstimate,
                            BREthereumGasPrice gasPrice) {
    BREthereumHandleGasEstimateEvent event = { { NULL, &handleGasEstimateEventType }, ewm, wallet, cookie, SUCCESS, gasEstimate, gasPrice };
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &event);
}

extern void
ewmSignalGasEstimateFailure(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumCookie cookie,
                            BREthereumStatus status) {
    BREthereumHandleGasEstimateEvent event = { { NULL, &handleGasEstimateEventType }, ewm, wallet, cookie, status };
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &event);
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

static void
ewmHandleTransactionEventDestroyer (BREthereumHandleTransactionEvent *event) {
    transactionRelease(event->transaction);
}

BREventType handleTransactionEventType = {
    "EWM: Handle Transaction Event",
    sizeof (BREthereumHandleTransactionEvent),
    (BREventDispatcher) ewmHandleTransactionEventDispatcher,
    (BREventDestroyer) ewmHandleTransactionEventDestroyer
};

extern void
ewmSignalTransaction (BREthereumEWM ewm,
                      BREthereumBCSCallbackTransactionType type,
                      OwnershipGiven BREthereumTransaction transaction) {
    BREthereumHandleTransactionEvent event = { { NULL, &handleTransactionEventType }, ewm, type, transaction };
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &event);
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

static void
ewmHandleLogEventDestroyer (BREthereumHandleLogEvent *event) {
    logRelease(event->log);
}

BREventType handleLogEventType = {
    "EWM: Handle Log Event",
    sizeof (BREthereumHandleLogEvent),
    (BREventDispatcher) ewmHandleLogEventDispatcher,
    (BREventDestroyer) ewmHandleLogEventDestroyer
};

extern void
ewmSignalLog (BREthereumEWM ewm,
              BREthereumBCSCallbackLogType type,
              OwnershipGiven BREthereumLog log) {
    BREthereumHandleLogEvent event = { { NULL, &handleLogEventType }, ewm, type, log };
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &event);
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
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &event);
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

static void
ewmHandleSaveNodesEventDestroyer (BREthereumHandleSaveNodesEvent *event) {
    if (NULL != event->nodes) {
        for (size_t index = 0; index < array_count (event->nodes); index++)
            nodeConfigRelease (event->nodes[index]);
        array_free (event->nodes);
    }
}

BREventType handleSaveNodesEventType = {
    "EWM: Handle SaveNodes Event",
    sizeof (BREthereumHandleSaveNodesEvent),
    (BREventDispatcher) ewmHandleSaveNodesEventDispatcher,
    (BREventDestroyer)  ewmHandleSaveNodesEventDestroyer
};

extern void
ewmSignalSaveNodes (BREthereumEWM ewm,
                    OwnershipGiven BRArrayOf(BREthereumNodeConfig) nodes) {
    BREthereumHandleSaveNodesEvent event = { { NULL, &handleSaveNodesEventType }, ewm, nodes };
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &event);
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
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Sync API
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
} BREthereumHandleSyncAPIEvent;

static void
ewmHandleSyncAPIEventDispatcher(BREventHandler ignore,
                             BREthereumHandleSyncAPIEvent *event) {
    ewmHandleSyncAPI (event->ewm);
}

BREventType handleSyncAPIEventType = {
    "EWM: Handle Sync API Event",
    sizeof (BREthereumHandleSyncAPIEvent),
    (BREventDispatcher) ewmHandleSyncAPIEventDispatcher
};

extern void
ewmSignalSyncAPI (BREthereumEWM ewm,
                  BREthereumBoolean OOB) {
    BREthereumHandleSyncEvent event = { { NULL, &handleSyncAPIEventType }, ewm };
    if (ETHEREUM_BOOLEAN_IS_TRUE(OOB)) eventHandlerSignalEventOOB (ewm->handler, (BREvent*) &event);
    else eventHandlerSignalEvent (ewm->handler, (BREvent*) &event);
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
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &event);
}

/// MARK: - Client

//
// Wallet Event
//
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumWallet wid;
    BREthereumWalletEvent event;
} BREthereumEWMClientWalletEvent;

#define CLIENT_WALLET_EVENT_INITIALIZER(ewm, wid, event)  \
{ { NULL, &ewmClientWalletEventType }, (ewm), (wid), (event) }

static void
ewmClientWalletEventDispatcher(BREventHandler ignore,
                               BREthereumEWMClientWalletEvent *event) {
    ewmHandleWalletEvent(event->ewm,
                         event->wid,
                         event->event);
}

static BREventType ewmClientWalletEventType = {
    "EWM: Client Wallet Event",
    sizeof (BREthereumEWMClientWalletEvent),
    (BREventDispatcher) ewmClientWalletEventDispatcher
};

extern void
ewmSignalWalletEvent(BREthereumEWM ewm,
                     BREthereumWallet wid,
                     BREthereumWalletEvent event) {
    BREthereumEWMClientWalletEvent message =
    CLIENT_WALLET_EVENT_INITIALIZER (ewm, wid, event);
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &message);
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
} BREthereumEWMClientBlockEvent;

#define CLIENT_BLOCK_EVENT_INITIALIZER(ewm, bid, event)  \
{ { NULL, &ewmClientBlockEventType }, (ewm), (bid), (event) }

static void
ewmClientBlockEventDispatcher(BREventHandler ignore,
                              BREthereumEWMClientBlockEvent *event) {
    ewmHandleBlockEvent(event->ewm,
                        event->bid,
                        event->event);
}

static BREventType ewmClientBlockEventType = {
    "EWM: Client Block Event",
    sizeof (BREthereumEWMClientBlockEvent),
    (BREventDispatcher) ewmClientBlockEventDispatcher
};

extern void
ewmSignalBlockEvent(BREthereumEWM ewm,
                    BREthereumBlock bid,
                    BREthereumBlockEvent event) {
    BREthereumEWMClientBlockEvent message =
    CLIENT_BLOCK_EVENT_INITIALIZER (ewm, bid, event);
    eventHandlerSignalEvent(ewm->handlerForMain, (BREvent*) &message);
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
} BREthereumEWMClientTransactionEvent;

#define CLIENT_TRANSACTION_EVENT_INITIALIZER(ewm, wid, tid, event)  \
{ { NULL, &ewmClientTransactionEventType }, (ewm), (wid), (tid), (event) }

static void
ewmClientTransactionEventDispatcher(BREventHandler ignore,
                                    BREthereumEWMClientTransactionEvent *event) {
    ewmHandleTransferEvent(event->ewm,
                           event->wid,
                           event->tid,
                           event->event);
}

static BREventType ewmClientTransactionEventType = {
    "EWM: Client Transaction Event",
    sizeof (BREthereumEWMClientTransactionEvent),
    (BREventDispatcher) ewmClientTransactionEventDispatcher
};

extern void
ewmSignalTransferEvent(BREthereumEWM ewm,
                       BREthereumWallet wid,
                       BREthereumTransfer tid,
                       BREthereumTransferEvent event) {
    BREthereumEWMClientTransactionEvent message =
    CLIENT_TRANSACTION_EVENT_INITIALIZER (ewm, wid, tid, event);
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &message);
}

//
// Peer Event
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumPeerEvent event;

} BREthereumEWMClientPeerEvent;

#define CLIENT_PEER_EVENT_INITIALIZER(ewm, /* wid, tid,*/ event)  \
{ { NULL, &ewmClientPeerEventType }, (ewm), /*(wid), (tid),*/ (event) }

static void
ewmClientPeerEventDispatcher(BREventHandler ignore,
                             BREthereumEWMClientPeerEvent *event) {
    ewmHandlePeerEvent(event->ewm, event->event);
}

static BREventType ewmClientPeerEventType = {
    "EWM: Client Peer Event",
    sizeof (BREthereumEWMClientPeerEvent),
    (BREventDispatcher) ewmClientPeerEventDispatcher
};

extern void
ewmSignalPeerEvent(BREthereumEWM ewm,
                    BREthereumPeerEvent event) {
    BREthereumEWMClientPeerEvent message =
    CLIENT_PEER_EVENT_INITIALIZER (ewm, /* wid, tid,*/ event);
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &message);
}

//
// EWM Event
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumEWMEvent event;
} BREthereumEWMClientEWMEvent;

#define CLIENT_EWM_EVENT_INITIALIZER(ewm, /* wid, tid,*/ event)  \
{ { NULL, &ewmClientEWMEventType }, (ewm), /*(wid), (tid),*/ (event) }

static void
ewmClientEWMEventDispatcher(BREventHandler ignore,
                            BREthereumEWMClientEWMEvent *event) {
    ewmHandleEWMEvent(event->ewm, event->event);
}

static BREventType ewmClientEWMEventType = {
    "EWM: Client EWM Event",
    sizeof (BREthereumEWMClientEWMEvent),
    (BREventDispatcher) ewmClientEWMEventDispatcher
};

extern void
ewmSignalEWMEvent(BREthereumEWM ewm,
                  BREthereumEWMEvent event) {
    BREthereumEWMClientEWMEvent message =
    CLIENT_EWM_EVENT_INITIALIZER (ewm, /* wid, tid,*/ event);
    eventHandlerSignalEvent(ewm->handler, (BREvent*) &message);
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
ewmSignalAnnounceBlockNumberDispatcher (BREventHandler ignore,
                                        BREthereumEWMClientAnnounceBlockNumberEvent *event) {
    ewmHandleAnnounceBlockNumber(event->ewm, event->blockNumber, event->rid);
}

static BREventType ewmClientAnnounceBlockNumberEventType = {
    "EWM: Client Announce Block Number Event",
    sizeof (BREthereumEWMClientAnnounceBlockNumberEvent),
    (BREventDispatcher) ewmSignalAnnounceBlockNumberDispatcher
};

extern void
ewmSignalAnnounceBlockNumber (BREthereumEWM ewm,
                              uint64_t blockNumber,
                              int rid) {
    BREthereumEWMClientAnnounceBlockNumberEvent message =
    { { NULL, &ewmClientAnnounceBlockNumberEventType}, ewm, blockNumber, rid};
    eventHandlerSignalEvent (ewm->handler, (BREvent*) &message);
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
ewmSignalAnnounceNonceDispatcher (BREventHandler ignore,
                                  BREthereumEWMClientAnnounceNonceEvent *event) {
    ewmHandleAnnounceNonce(event->ewm, event->address, event->nonce, event->rid);
}

static BREventType ewmClientAnnounceNonceEventType = {
    "EWM: Client Announce Block Number Event",
    sizeof (BREthereumEWMClientAnnounceNonceEvent),
    (BREventDispatcher) ewmSignalAnnounceNonceDispatcher
};

extern void
ewmSignalAnnounceNonce (BREthereumEWM ewm,
                        BREthereumAddress address,
                        uint64_t nonce,
                        int rid) {
    BREthereumEWMClientAnnounceNonceEvent message =
    { { NULL, &ewmClientAnnounceNonceEventType}, ewm, address, nonce, rid};
    eventHandlerSignalEvent (ewm->handler, (BREvent*) &message);
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
ewmSignalAnnounceBalanceDispatcher (BREventHandler ignore,
                                    BREthereumEWMClientAnnounceBalanceEvent *event) {
    ewmHandleAnnounceBalance(event->ewm, event->wallet, event->value, event->rid);
}

static BREventType ewmClientAnnounceBalanceEventType = {
    "EWM: Client Announce Balance Event",
    sizeof (BREthereumEWMClientAnnounceBalanceEvent),
    (BREventDispatcher) ewmSignalAnnounceBalanceDispatcher
};

extern void
ewmSignalAnnounceBalance (BREthereumEWM ewm,
                          BREthereumWallet wallet,
                          UInt256 value,
                          int rid) {
    BREthereumEWMClientAnnounceBalanceEvent message =
    { { NULL, &ewmClientAnnounceBalanceEventType}, ewm, wallet, value, rid};
    eventHandlerSignalEvent (ewm->handler, (BREvent*) &message);
}

//
// Update Wallet Balances
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
} BREthereumEWMClientUpdateWalletBalancesEvent;

static void
ewmHandleUpdateWalletBalancesDispatcher (BREventHandler ignore,
                                         BREthereumEWMClientUpdateWalletBalancesEvent *event) {
    ewmHandleUpdateWalletBalances (event->ewm);
}

static BREventType ewmClientUpdateWalletBalancesEventType = {
    "EWM: Client Update Wallet Balances Event",
    sizeof (BREthereumEWMClientUpdateWalletBalancesEvent),
    (BREventDispatcher) ewmHandleUpdateWalletBalancesDispatcher
};

extern void
ewmSignalUpdateWalletBalances (BREthereumEWM ewm) {
    BREthereumEWMClientUpdateWalletBalancesEvent message =
    { { NULL, &ewmClientUpdateWalletBalancesEventType}, ewm };
    eventHandlerSignalEvent (ewm->handler, (BREvent*) &message);
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
ewmSignalAnnounceGasPriceDispatcher (BREventHandler ignore,
                                     BREthereumEWMClientAnnounceGasPriceEvent *event) {
    ewmHandleAnnounceGasPrice(event->ewm, event->wallet, event->amount, event->rid);
}

static BREventType ewmClientAnnounceGasPriceEventType = {
    "EWM: Client Announce GasPrice Event",
    sizeof (BREthereumEWMClientAnnounceGasPriceEvent),
    (BREventDispatcher) ewmSignalAnnounceGasPriceDispatcher
};

extern void
ewmSignalAnnounceGasPrice (BREthereumEWM ewm,
                           BREthereumWallet wallet,
                           UInt256 amount,
                           int rid) {
    BREthereumEWMClientAnnounceGasPriceEvent message =
    { { NULL, &ewmClientAnnounceGasPriceEventType}, ewm, wallet, amount, rid};
    eventHandlerSignalEvent (ewm->handler, (BREvent*) &message);
}

//
// Announce Submit Transaction
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumWallet wallet;
    BREthereumTransfer transfer;
    int errorCode;
    char *errorMessage;
    int rid;
} BREthereumEWMClientAnnounceSubmitTransferEvent;

static void
ewmSignalAnnounceSubmitTransferDispatcher (BREventHandler ignore,
                                           BREthereumEWMClientAnnounceSubmitTransferEvent *event) {
    ewmHandleAnnounceSubmitTransfer (event->ewm, event->wallet, event->transfer, event->errorCode, event->errorMessage, event->rid);
    if (NULL != event->errorMessage) free (event->errorMessage);
}

static void
ewmSignalAnnounceSubmitTransferDestroyer (BREthereumEWMClientAnnounceSubmitTransferEvent *event) {
    if (NULL != event->errorMessage) free (event->errorMessage);
}

static BREventType ewmClientAnnounceSubmitTransferEventType = {
    "EWM: Client Announce SubmitTransfer Event",
    sizeof (BREthereumEWMClientAnnounceSubmitTransferEvent),
    (BREventDispatcher) ewmSignalAnnounceSubmitTransferDispatcher,
    (BREventDestroyer) ewmSignalAnnounceSubmitTransferDestroyer
};

extern void
ewmSignalAnnounceSubmitTransfer (BREthereumEWM ewm,
                                 BREthereumWallet wallet,
                                 BREthereumTransfer transfer,
                                 int errorCode,
                                 const char *errorMessage,
                                 int rid) {
    BREthereumEWMClientAnnounceSubmitTransferEvent message =
    { { NULL, &ewmClientAnnounceSubmitTransferEventType}, ewm, wallet, transfer,
        errorCode,
        (NULL == errorMessage ? NULL : strdup (errorMessage)),
        rid};
    eventHandlerSignalEvent (ewm->handler, (BREvent*) &message);
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
ewmSignalAnnounceTransactionDispatcher (BREventHandler ignore,
                                        BREthereumEWMClientAnnounceTransactionEvent *event) {
    ewmHandleAnnounceTransaction(event->ewm, event->bundle, event->rid);
}

static void
ewmSignalAnnounceTransactionDestroyer (BREthereumEWMClientAnnounceTransactionEvent *event) {
    ewmClientAnnounceTransactionBundleRelease(event->bundle);
}

static BREventType ewmClientAnnounceTransactionEventType = {
    "EWM: Client Announce Transaction Event",
    sizeof (BREthereumEWMClientAnnounceTransactionEvent),
    (BREventDispatcher) ewmSignalAnnounceTransactionDispatcher,
    (BREventDestroyer) ewmSignalAnnounceTransactionDestroyer
};

extern void
ewmSignalAnnounceTransaction(BREthereumEWM ewm,
                             BREthereumEWMClientAnnounceTransactionBundle *bundle,
                             int rid) {
    BREthereumEWMClientAnnounceTransactionEvent message =
    { { NULL, &ewmClientAnnounceTransactionEventType}, ewm, bundle, rid};
    eventHandlerSignalEvent (ewm->handler, (BREvent*) &message);
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
ewmSignalAnnounceLogDispatcher (BREventHandler ignore,
                                BREthereumEWMClientAnnounceLogEvent *event) {
    ewmHandleAnnounceLog(event->ewm, event->bundle, event->rid);
}

static void
ewmSignalAnnounceLogDestroyer (BREthereumEWMClientAnnounceLogEvent *event) {
    ewmClientAnnounceLogBundleRelease(event->bundle);
}

static BREventType ewmClientAnnounceLogEventType = {
    "EWM: Client Announce Log Event",
    sizeof (BREthereumEWMClientAnnounceLogEvent),
    (BREventDispatcher) ewmSignalAnnounceLogDispatcher,
    (BREventDestroyer) ewmSignalAnnounceLogDestroyer
};

extern void
ewmSignalAnnounceLog (BREthereumEWM ewm,
                      BREthereumEWMClientAnnounceLogBundle *bundle,
                      int rid) {
    BREthereumEWMClientAnnounceLogEvent message =
    { { NULL, &ewmClientAnnounceLogEventType}, ewm, bundle, rid};
    eventHandlerSignalEvent (ewm->handler, (BREvent*) &message);
}

//
// Announce {Transaction, Log} Complete
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumBoolean isTransaction;
    BREthereumBoolean success;
    int rid;
} BREthereumEWMClientAnnounceCompleteEvent;

static void
ewmSignalAnnounceCompleteDispatcher (BREventHandler ignore,
                                     BREthereumEWMClientAnnounceCompleteEvent *event) {
    ewmHandleAnnounceComplete(event->ewm, event->isTransaction, event->success, event->rid);
}

static BREventType ewmClientAnnounceCompleteEventType = {
    "EWM: Client Announce Complete Event",
    sizeof (BREthereumEWMClientAnnounceCompleteEvent),
    (BREventDispatcher) ewmSignalAnnounceCompleteDispatcher
};

extern void
ewmSignalAnnounceComplete (BREthereumEWM ewm,
                           BREthereumBoolean isTransaction,
                           BREthereumBoolean success,
                           int rid) {
    BREthereumEWMClientAnnounceCompleteEvent message =
    { { NULL, &ewmClientAnnounceCompleteEventType}, ewm, isTransaction, success, rid };
    eventHandlerSignalEvent (ewm->handler, (BREvent*) &message);
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
ewmSignalAnnounceTokenDispatcher (BREventHandler ignore,
                                  BREthereumEWMClientAnnounceTokenEvent *event) {
    ewmHandleAnnounceToken(event->ewm, event->bundle, event->rid);
}

static void
ewmSignalAnnounceTokenDestroyer (BREthereumEWMClientAnnounceTokenEvent *event) {
    ewmClientAnnounceTokenBundleRelease(event->bundle);
}

static BREventType ewmClientAnnounceTokenEventType = {
    "EWM: Client Announce Token Event",
    sizeof (BREthereumEWMClientAnnounceTokenEvent),
    (BREventDispatcher) ewmSignalAnnounceTokenDispatcher,
    (BREventDestroyer) ewmSignalAnnounceTokenDestroyer
};

extern void
ewmSignalAnnounceToken (BREthereumEWM ewm,
                        BREthereumEWMClientAnnounceTokenBundle *bundle,
                        int rid) {
    BREthereumEWMClientAnnounceTokenEvent message =
    { { NULL, &ewmClientAnnounceTokenEventType}, ewm, bundle, rid};
    eventHandlerSignalEventOOB (ewm->handler, (BREvent*) &message);
}

//
// Announce Token Complete
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumBoolean success;
    int rid;
} BREthereumEWMClientAnnounceTokenCompleteEvent;

static void
ewmSignalAnnounceTokenCompleteDispatcher (BREventHandler ignore,
                                          BREthereumEWMClientAnnounceTokenCompleteEvent *event) {
    ewmHandleAnnounceTokenComplete(event->ewm, event->rid, event->success);
}

static BREventType ewmClientAnnounceTokenCompleteEventType = {
    "EWM: Client Announce Complete Token Event",
    sizeof (BREthereumEWMClientAnnounceTokenCompleteEvent),
    (BREventDispatcher) ewmSignalAnnounceTokenCompleteDispatcher
};

extern void
ewmSignalAnnounceTokenComplete (BREthereumEWM ewm,
                                int rid,
                                BREthereumBoolean success) {
    BREthereumEWMClientAnnounceTokenCompleteEvent message =
    { { NULL, &ewmClientAnnounceTokenCompleteEventType}, ewm, success, rid };
    eventHandlerSignalEvent (ewm->handler, (BREvent*) &message);
}

// ==============================================================================================
//
// All Event Types
//
const BREventType *ewmEventTypes[] = {
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
    &handleSyncAPIEventType,
    &handleGetBlocksEventType,

    &ewmClientWalletEventType,
    //    &ewmClientBlockEventType,
    &ewmClientTransactionEventType,
    &ewmClientPeerEventType,
    &ewmClientEWMEventType,
    &ewmClientAnnounceBlockNumberEventType,
    &ewmClientAnnounceNonceEventType,
    &ewmClientAnnounceBalanceEventType,
    &ewmClientUpdateWalletBalancesEventType,
    &ewmClientAnnounceGasPriceEventType,
    &ewmClientAnnounceSubmitTransferEventType,
    &ewmClientAnnounceTransactionEventType,
    &ewmClientAnnounceLogEventType,
    &ewmClientAnnounceCompleteEventType,
    &ewmClientAnnounceTokenEventType,
    &ewmClientAnnounceTokenCompleteEventType,
};
const unsigned int
ewmEventTypesCount = (sizeof (ewmEventTypes) / sizeof (BREventType*));

