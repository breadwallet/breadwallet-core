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
