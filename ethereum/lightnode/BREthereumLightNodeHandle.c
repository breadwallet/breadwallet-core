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
    BREthereumWalletId wid = (AMOUNT_ETHER == amountGetType(event->amount)
                              ? lightNodeGetWallet(node)
                              : lightNodeGetWalletHoldingToken(node, amountGetToken (event->amount)));

    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);

    walletSetBalance(wallet, event->amount);

    lightNodeListenerAnnounceWalletEvent(node, wid, WALLET_EVENT_BALANCE_UPDATED,
                                         SUCCESS,
                                         NULL);
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
    BREthereumEncodedAddress address = accountGetPrimaryAddress(lightNodeGetAccount(node));

    addressSetNonce(address, event->nonce);

    // lightNodeListenerAnnounce ...
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
    BREthereumTransaction transaction = lightNodeLookupTransactionByHash(node, event->transactionHash);

    if (NULL == transaction) return;

    switch (event->status.type) {
        case TRANSACTION_STATUS_ERROR:
        case TRANSACTION_STATUS_QUEUED:
        case TRANSACTION_STATUS_PENDING:
        case TRANSACTION_STATUS_UNKNOWN:
            break;
        case TRANSACTION_STATUS_INCLUDED: {
            BREthereumBlock block = lightNodeLookupBlockByHash(node, event->status.u.included.blockHash);
            if (NULL == block) return;

            
        }

    }
    //    BREthereumAddress address = accountGetPrimaryAddress(lightNodeGetAccount(node));
    //    addressSetNonce(address, event->nonce);

    //lightNodeListenerAnnounceTransactionEvent(node, wid, tid, TRANSACTION_EVENT_BLOCKED, SUCCESS, NULL));
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

//
// All Handler Event Types
//
const BREventType *handlerEventTypes[] = {
    &handleBalanceEventType,
    &handleNonceEventType,
    &handleTransactionStatusEventType
};
const unsigned int handlerEventTypesCount = 3;

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
