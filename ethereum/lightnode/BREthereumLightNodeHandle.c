//
//  BREthereumLightNodeHandle.c
//  BRCore
//
//  Created by Ed Gamble on 5/16/18.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "BRArray.h"
#include "BREthereumPrivate.h"
#include "BREthereumLightNodePrivate.h"

/*!
 * Define the Events handled on the LightNode's Main queue.
 */

// ==============================================================================================
//
// Handle Balance
//
typedef struct {
    BREvent base;
    BREthereumLightNode node;
    BREthereumAmount amount;
} BREthereumHandleBalanceEvent;

static void
lightNodeHandleBalanceEventDispatcher(BREventHandler ignore,
                                      BREthereumHandleBalanceEvent *event) {
    BREthereumLightNode node = event->node;
    pthread_mutex_lock(&node->lock);

    BREthereumWalletId wid = (AMOUNT_ETHER == amountGetType(event->amount)
                              ? lightNodeGetWallet(node)
                              : lightNodeGetWalletHoldingToken(node, amountGetToken (event->amount)));

    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);

    walletSetBalance(wallet, event->amount);

    lightNodeListenerAnnounceWalletEvent(node, wid, WALLET_EVENT_BALANCE_UPDATED,
                                         SUCCESS,
                                         NULL);

    pthread_mutex_unlock(&node->lock);
}

BREventType handleBalanceEventType = {
    "Handle Balance Event",
    sizeof (BREthereumHandleBalanceEvent),
    (BREventDispatcher) lightNodeHandleBalanceEventDispatcher
};

extern void
lightNodeHandleBalance (BREthereumLightNode node,
                        BREthereumAmount amount) {
    BREthereumHandleBalanceEvent event = { { NULL, &handleBalanceEventType }, node, amount };
    eventHandlerSignalEvent(node->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Nonce
//
typedef struct {
    BREvent base;
    BREthereumLightNode node;
    uint64_t nonce;
} BREthereumHandleNonceEvent;

static void
lightNodeHandleNonceEventDispatcher(BREventHandler ignore,
                                    BREthereumHandleNonceEvent *event) {
    BREthereumLightNode node = event->node;
    pthread_mutex_lock(&node->lock);

    BREthereumEncodedAddress address = accountGetPrimaryAddress(lightNodeGetAccount(node));

    addressSetNonce(address, event->nonce);

    // lightNodeListenerAnnounce ...
    pthread_mutex_unlock(&node->lock);
}

BREventType handleNonceEventType = {
    "Handle Nonce Event",
    sizeof (BREthereumHandleNonceEvent),
    (BREventDispatcher) lightNodeHandleNonceEventDispatcher
};

extern void
lightNodeHandleNonce (BREthereumLightNode node,
                      uint64_t nonce) {
    BREthereumHandleNonceEvent event = { { NULL, &handleNonceEventType }, node, nonce };
    eventHandlerSignalEvent(node->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Gas Price
//
typedef struct {
    BREvent base;
    BREthereumLightNode node;
    BREthereumWallet wallet;
    BREthereumGasPrice gasPrice;
} BREthereumHandleGasPriceEvent;

static void
lightNodeHandleGasPriceEventDispatcher(BREventHandler ignore,
                                       BREthereumHandleGasPriceEvent *event) {
    BREthereumLightNode node = event->node;
    pthread_mutex_lock(&node->lock);

    walletSetDefaultGasPrice(event->wallet, event->gasPrice);

    lightNodeListenerAnnounceWalletEvent(node,
                                         lightNodeLookupWalletId(node, event->wallet),
                                         WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
                                         SUCCESS, NULL);

    pthread_mutex_unlock(&node->lock);
}

BREventType handleGasPriceEventType = {
    "Handle GasPrice Event",
    sizeof (BREthereumHandleGasPriceEvent),
    (BREventDispatcher) lightNodeHandleGasPriceEventDispatcher
};

extern void
lightNodeHandleGasPrice (BREthereumLightNode node,
                         BREthereumWallet wallet,
                         BREthereumGasPrice gasPrice) {
    BREthereumHandleGasPriceEvent event = { { NULL, &handleGasPriceEventType }, node, wallet, gasPrice };
    eventHandlerSignalEvent(node->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Gas Estimate
//
typedef struct {
    BREvent base;
    BREthereumLightNode node;
    BREthereumWallet wallet;
    BREthereumTransaction transaction;
    BREthereumGas gasEstimate;
} BREthereumHandleGasEstimateEvent;

static void
lightNodeHandleGasEstimateEventDispatcher(BREventHandler ignore,
                                       BREthereumHandleGasEstimateEvent *event) {
    BREthereumLightNode node = event->node;
    pthread_mutex_lock(&node->lock);

    transactionSetGasEstimate(event->transaction, event->gasEstimate);

    lightNodeListenerAnnounceTransactionEvent(node,
                                         lightNodeLookupWalletId(node, event->wallet),
                                         lightNodeLookupTransactionId (node, event->transaction),
                                         TRANSACTION_EVENT_GAS_ESTIMATE_UPDATED,
                                         SUCCESS, NULL);

    pthread_mutex_unlock(&node->lock);
}

BREventType handleGasEstimateEventType = {
    "Handle GasEstimate Event",
    sizeof (BREthereumHandleGasEstimateEvent),
    (BREventDispatcher) lightNodeHandleGasEstimateEventDispatcher
};

extern void
lightNodeHandleGasEstimate (BREthereumLightNode node,
                            BREthereumWallet wallet,
                            BREthereumTransaction transaction,
                            BREthereumGas gasEstimate) {
    BREthereumHandleGasEstimateEvent event = { { NULL, &handleGasEstimateEventType }, node, wallet, transaction, gasEstimate };
    eventHandlerSignalEvent(node->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Transaction Status
//
typedef struct {
    BREvent base;
    BREthereumLightNode node;
    BREthereumHash transactionHash;
    BREthereumTransactionStatusLES status;
} BREthereumHandleTransactionStatusEvent;

static void
lightNodeHandleTransactionStatusEventDispatcher(BREventHandler ignore,
                                                BREthereumHandleTransactionStatusEvent *event) {
    BREthereumLightNode node = event->node;
    pthread_mutex_lock(&node->lock);

    BREthereumTransaction transaction = lightNodeLookupTransactionByHash(node, event->transactionHash);
    BREthereumWallet wallet = (NULL == transaction ? NULL : lightNodeLookupWalletByTransaction(node, transaction));

    // Strict, for now.
    assert (NULL != transaction);
    assert (NULL != wallet);

    switch (event->status.type) {
        case TRANSACTION_STATUS_UNKNOWN:
            break;
        case TRANSACTION_STATUS_QUEUED:
            break;
        case TRANSACTION_STATUS_PENDING:
            walletTransactionSubmitted(wallet, transaction, event->transactionHash);
            lightNodeListenerAnnounceTransactionEvent(node,
                                                      lightNodeLookupWalletId(node, wallet),
                                                      lightNodeLookupTransactionId (node, transaction),
                                                      TRANSACTION_EVENT_SUBMITTED,
                                                      SUCCESS, NULL);
            break;

        case TRANSACTION_STATUS_INCLUDED: {
            // TODO: DO NOTHING - should get a TransactionReceipt message with *all* needed data.
            BREthereumBlock block = lightNodeLookupBlockByHash(node, event->status.u.included.blockHash);
            assert (NULL != block);

            walletTransactionBlocked(wallet, transaction,
                                     gasCreate(0),
                                     event->status.u.included.blockNumber,
                                     0,
                                     event->status.u.included.transactionIndex);
            lightNodeListenerAnnounceTransactionEvent(node,
                                                      lightNodeLookupWalletId(node, wallet),
                                                      lightNodeLookupTransactionId (node, transaction),
                                                      TRANSACTION_EVENT_BLOCKED,
                                                      SUCCESS, NULL);
            break;
        }

        case TRANSACTION_STATUS_ERROR:
            lightNodeListenerAnnounceTransactionEvent(node,
                                                      lightNodeLookupWalletId(node, wallet),
                                                      lightNodeLookupTransactionId (node, transaction),
                                                      TRANSACTION_EVENT_SUBMITTED,
                                                      ERROR_TRANSACTION_SUBMISSION,
                                                      event->status.u.error.message);
            break;
    }
    pthread_mutex_unlock(&node->lock);
}

BREventType handleTransactionStatusEventType = {
    "Handle TransactionStatus Event",
    sizeof (BREthereumHandleTransactionStatusEvent),
    (BREventDispatcher) lightNodeHandleTransactionStatusEventDispatcher
};

extern void
lightNodeHandleTransactionStatus (BREthereumLightNode node,
                                  BREthereumHash transactionHash,
                                  BREthereumTransactionStatusLES status) {
    BREthereumHandleTransactionStatusEvent event =
    { { NULL, &handleTransactionStatusEventType }, node, transactionHash, status };
    eventHandlerSignalEvent(node->handlerForMain, (BREvent*) &event);
}


// ==============================================================================================
//
// Handle Transaction Receipt
//
typedef struct {
    BREvent base;
    BREthereumLightNode node;
    BREthereumHash blockHash;
    BREthereumTransactionReceipt receipt;
    unsigned int receiptIndex;
} BREthereumHandleTransactionReceiptEvent;

static void
lightNodeHandleTransactionReceiptEventDispatcher(BREventHandler ignore,
                                                 BREthereumHandleTransactionReceiptEvent *event) {
    BREthereumLightNode node = event->node;
    pthread_mutex_lock(&node->lock);

    BREthereumBlock block = lightNodeLookupBlockByHash(node, event->blockHash);
    assert (NULL != block);

    BREthereumTransaction transaction = blockGetTransaction(block, event->receiptIndex);
    assert (NULL != transaction);

    BREthereumWallet wallet = lightNodeLookupWalletByTransaction(node, transaction);
    assert (NULL != wallet);

    walletTransactionBlocked(wallet, transaction,
                             gasCreate (transactionReceiptGetGasUsed(event->receipt)),
                             blockGetNumber(block),
                             blockGetTimestamp(block),
                             event->receiptIndex);
    
    lightNodeListenerAnnounceTransactionEvent(node,
                                              lightNodeLookupWalletId(node, wallet),
                                              lightNodeLookupTransactionId (node, transaction),
                                              TRANSACTION_EVENT_BLOCKED,
                                              SUCCESS, NULL);

    // TODO: Check if the bloom filter matches; if so, handle Logs.

    pthread_mutex_unlock(&node->lock);
}

BREventType handleTransactionReceiptEventType = {
    "Handle TransactionReceipt Event",
    sizeof (BREthereumHandleTransactionReceiptEvent),
    (BREventDispatcher) lightNodeHandleTransactionReceiptEventDispatcher
};

extern void
lightNodeHandleTransactionReceipt (BREthereumLightNode node,
                                   BREthereumHash blockHash,
                                   BREthereumTransactionReceipt receipt,
                                   unsigned int receiptIndex) {
    BREthereumHandleTransactionReceiptEvent event =
        { { NULL, &handleTransactionReceiptEventType }, node, blockHash, receipt, receiptIndex };
    eventHandlerSignalEvent(node->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Announce
//
typedef struct {
    BREvent base;
    BREthereumLightNode node;
    BREthereumHash headHash;
    uint64_t headNumber;
    uint64_t headTotalDifficulty;
} BREthereumHandleAnnounceEvent;

static void
lightNodeHandleAnnounceEventDispatcher (BREventHandler ignore,
                                        BREthereumHandleAnnounceEvent *event) {
    BREthereumLightNode node = event->node;
    pthread_mutex_lock(&node->lock);

    // Request the block.

    pthread_mutex_unlock(&node->lock);
}

BREventType handleAnnounceEventType = {
    "Handle Announce Event",
    sizeof (BREthereumHandleAnnounceEvent),
    (BREventDispatcher) lightNodeHandleAnnounceEventDispatcher
};

extern void
lightNodeHandleAnnounce (BREthereumLightNode node,
                         BREthereumHash headHash,
                         uint64_t headNumber,
                         uint64_t headTotalDifficulty) {
    BREthereumHandleAnnounceEvent event =
        { { NULL, &handleAnnounceEventType}, node, headHash, headNumber, headTotalDifficulty};
    eventHandlerSignalEvent(node->handlerForMain, (BREvent *) &event);
}

// ==============================================================================================
//
// Handle Block Header
//
typedef struct {
    BREvent base;
    BREthereumLightNode node;
    BREthereumBlockHeader header;
} BREthereumHandleBlockHeaderEvent;

static void
lightNodeHandleBlockHeaderDispatcher (BREventHandler ignore,
                                      BREthereumHandleBlockHeaderEvent *event) {
    BREthereumLightNode node = event->node;
    pthread_mutex_lock(&node->lock);

    // Request the block.

    pthread_mutex_unlock(&node->lock);

}

BREventType handleBlockHeaderEventType = {
    "Handle Block Header Event",
    sizeof (BREthereumHandleBlockHeaderEvent),
    (BREventDispatcher) lightNodeHandleBlockHeaderDispatcher
};

extern void
lightNodeHandleBlockHeader (BREthereumLightNode node,
                            BREthereumBlockHeader header) {
    BREthereumHandleBlockHeaderEvent event =
    { { NULL, &handleBlockHeaderEventType}, node, header };
    eventHandlerSignalEvent(node->handlerForMain, (BREvent *) &event);
}

// ==============================================================================================
//
// Handle Block Bodies
//
typedef struct {
    BREvent base;
    BREthereumLightNode node;
    BREthereumHash blockHash;
    BREthereumTransaction *transactions;
    BREthereumHash *ommer;
} BREthereumHandleBlockBodiesEvent;

static void
lightNodeHandleBlockBodiesDispatcher (BREventHandler ignore,
                                      BREthereumHandleBlockBodiesEvent *event) {
    BREthereumLightNode node = event->node;
    pthread_mutex_lock(&node->lock);

    // Request the block.

    pthread_mutex_unlock(&node->lock);

}

BREventType handleBlockBodiesEventType = {
    "Handle Block Bodies Event",
    sizeof (BREthereumHandleBlockBodiesEvent),
    (BREventDispatcher) lightNodeHandleBlockBodiesDispatcher
};

extern void
lightNodeHandleBlockBodies (BREthereumLightNode node,
                            BREthereumHash blockHash,
                            BREthereumTransaction transactions[],
                            BREthereumHash ommers[]) {
    BREthereumHandleBlockBodiesEvent event =
    { { NULL, &handleBlockBodiesEventType}, node, blockHash, transactions, ommers };
    eventHandlerSignalEvent(node->handlerForMain, (BREvent *) &event);
}


// ==============================================================================================
//
// All Handler Event Types
//
const BREventType *handlerEventTypes[] = {
    &handleBalanceEventType,
    &handleNonceEventType,
    &handleGasPriceEventType,
    &handleGasEstimateEventType,
    &handleTransactionStatusEventType,
    &handleTransactionReceiptEventType,
    &handleAnnounceEventType,
    &handleBlockHeaderEventType,
    &handleBlockBodiesEventType
};
const unsigned int handlerEventTypesCount = 9;

///////////

/*
extern void
lightNodeHandleBlock (BREthereumLightNode node,
                      BREthereumBlock block);


extern void
lightNodeHandleTransaction (BREthereumLightNode node,
                            BREthereumBlock block,      // hash
                            BREthereumTransaction transaction);

extern void
ligthNodeHandleLog (BREthereumLightNode node,
                    BREthereumBlock block,          // hash
                    BREthereumTransaction transaction, // hash
                    BREthereumLog log);
*/
