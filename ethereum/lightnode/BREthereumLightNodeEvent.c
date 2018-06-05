//
//  BREthereumLightNodeListener.c
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
#include "BREthereumLightNodePrivate.h"

/*!
 * A LightNode has Listeners 'subscribed' to Events - Wallet, Transaction, Block, Peer and
 * LightNode Events.  The subscribed listeners should not run on the 'main' LightNode queue;
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
    BREthereumLightNode node;
    BREthereumWalletId wid;
    BREthereumWalletEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumListenerWalletEvent;

#define LISTENER_WALLET_EVENT_INITIALIZER(node, wid, vent, status, desc)  \
{ { NULL, &listenerWalletEventType }, (node), (wid), (event), (status), (desc) }

static void
lightNodeListenerWalletEventDispatcher(BREventHandler ignore,
                                       BREthereumListenerWalletEvent *event) {
    lightNodeListenerHandleWalletEvent(event->node,
                                       event->wid,
                                       event->event,
                                       event->status,
                                       event->errorDescription);
}

BREventType listenerWalletEventType = {
    "LN: Listener Wallet Event",
    sizeof (BREthereumListenerWalletEvent),
    (BREventDispatcher) lightNodeListenerWalletEventDispatcher
};

/*!
 * Add a WalletEvent to the LightNode's Listener Queue
 */
extern void
lightNodeListenerSignalWalletEvent(BREthereumLightNode node,
                                     BREthereumWalletId wid,
                                     BREthereumWalletEvent event,
                                     BREthereumStatus status,
                                     const char *errorDescription) {
    BREthereumListenerWalletEvent message =
    LISTENER_WALLET_EVENT_INITIALIZER (node, wid, event, status, errorDescription);
    eventHandlerSignalEvent(node->handlerForListener, (BREvent*) &message);
}

//
// Block Event
//
typedef struct {
    BREvent base;
    BREthereumLightNode node;
    BREthereumBlockId bid;
    BREthereumBlockEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumListenerBlockEvent;

#define LISTENER_BLOCK_EVENT_INITIALIZER(node, bid, event, status, desc)  \
{ { NULL, &listenerBlockEventType }, (node), (bid), (event), (status), (desc) }

static void
lightNodeListenerBlockEventDispatcher(BREventHandler ignore,
                                      BREthereumListenerBlockEvent *event) {
    lightNodeListenerHandleBlockEvent(event->node,
                                      event->bid,
                                      event->event,
                                      event->status,
                                      event->errorDescription);
}

BREventType listenerBlockEventType = {
    "LN: Listener Block Event",
    sizeof (BREthereumListenerBlockEvent),
    (BREventDispatcher) lightNodeListenerBlockEventDispatcher
};

/*!
 * Add a BlockEvent to the LightNode's Listener Queue
 */
extern void
lightNodeListenerSignalBlockEvent(BREthereumLightNode node,
                                    BREthereumBlockId bid,
                                    BREthereumBlockEvent event,
                                    BREthereumStatus status,
                                    const char *errorDescription) {
    BREthereumListenerBlockEvent message =
    LISTENER_BLOCK_EVENT_INITIALIZER (node, bid, event, status, errorDescription);
    eventHandlerSignalEvent(node->handlerForListener, (BREvent*) &message);
}

//
// Transaction Event
//
typedef struct {
    struct BREventRecord base;
    BREthereumLightNode node;
    BREthereumWalletId wid;
    BREthereumTransactionId tid;
    BREthereumTransactionEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumListenerTransactionEvent;

#define LISTENER_TRANSACTION_EVENT_INITIALIZER(node, wid, tid, event, status, desc)  \
{ { NULL, &listenerTransactionEventType }, (node), (wid), (tid), (event), (status), (desc) }

static void
lightNodeListenerTransactionEventDispatcher(BREventHandler ignore,
                                            BREthereumListenerTransactionEvent *event) {
    lightNodeListenerHandleTransactionEvent(event->node,
                                            event->wid,
                                            event->tid,
                                            event->event,
                                            event->status,
                                            event->errorDescription);
}

BREventType listenerTransactionEventType = {
    "LN: Listener Transaction Event",
    sizeof (BREthereumListenerTransactionEvent),
    (BREventDispatcher) lightNodeListenerTransactionEventDispatcher
};

/*!
 * Add a TransactionEvent to the LightNode's Listener Queue
 */
extern void
lightNodeListenerSignalTransactionEvent(BREthereumLightNode node,
                                          BREthereumWalletId wid,
                                          BREthereumTransactionId tid,
                                          BREthereumTransactionEvent event,
                                          BREthereumStatus status,
                                          const char *errorDescription) {
    BREthereumListenerTransactionEvent message =
    LISTENER_TRANSACTION_EVENT_INITIALIZER (node, wid, tid, event, status, errorDescription);
    eventHandlerSignalEvent(node->handlerForListener, (BREvent*) &message);
}

//
// Peer Event
//
typedef struct {
    struct BREventRecord base;
    BREthereumLightNode node;
    // BREthereumWalletId wid;
    // BREthereumTransactionId tid;
    BREthereumPeerEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumListenerPeerEvent;

#define LISTENER_PEER_EVENT_INITIALIZER(node, /* wid, tid,*/ event, status, desc)  \
{ { NULL, &listenerPeerEventType }, (node), /*(wid), (tid),*/ (event), (status), (desc) }

static void
lightNodeListenerPeerEventDispatcher(BREventHandler ignore,
                                     BREthereumListenerPeerEvent *event) {
    lightNodeListenerHandlePeerEvent(event->node, event->event, event->status, event->errorDescription);
}

BREventType listenerPeerEventType = {
    "LN: Listener Peer Event",
    sizeof (BREthereumListenerPeerEvent),
    (BREventDispatcher) lightNodeListenerPeerEventDispatcher
};

/*!
 * Add a PeerEvent to the LightNode's Listener Queue
 */
extern void
lightNodeListenerSignalPeerEvent(BREthereumLightNode node,
                                   // BREthereumWalletId wid,
                                   // BREthereumTransactionId tid,
                                   BREthereumPeerEvent event,
                                   BREthereumStatus status,
                                   const char *errorDescription) {
    BREthereumListenerPeerEvent message =
    LISTENER_PEER_EVENT_INITIALIZER (node, /* wid, tid,*/ event, status, errorDescription);
    eventHandlerSignalEvent(node->handlerForListener, (BREvent*) &message);
}

//
// LightNode Event
//
typedef struct {
    struct BREventRecord base;
    BREthereumLightNode node;
    // BREthereumWalletId wid;
    // BREthereumTransactionId tid;
    BREthereumLightNodeEvent event;
    BREthereumStatus status;
    const char *errorDescription;
} BREthereumListenerLightNodeEvent;

#define LISTENER_LIGHT_NODE_EVENT_INITIALIZER(node, /* wid, tid,*/ event, status, desc)  \
{ { NULL, &listenerLightNodeEventType }, (node), /*(wid), (tid),*/ (event), (status), (desc) }

static void
lightNodeListenerLightNodeEventDispatcher(BREventHandler ignore,
                                          BREthereumListenerLightNodeEvent *event) {
    lightNodeListenerHandleLightNodeEvent(event->node, event->event, event->status, event->errorDescription);
}

BREventType listenerLightNodeEventType = {
    "LN: Listener LightNode Event",
    sizeof (BREthereumListenerLightNodeEvent),
    (BREventDispatcher) lightNodeListenerLightNodeEventDispatcher
};

/*!
 * Add a LightNodeEvent to the LightNode's Listener Queue
 */
extern void
lightNodeListenerSignalLightNodeEvent(BREthereumLightNode node,
                                        // BREthereumWalletId wid,
                                        // BREthereumTransactionId tid,
                                        BREthereumLightNodeEvent event,
                                        BREthereumStatus status,
                                        const char *errorDescription) {
    BREthereumListenerLightNodeEvent message =
    LISTENER_LIGHT_NODE_EVENT_INITIALIZER (node, /* wid, tid,*/ event, status, errorDescription);
    eventHandlerSignalEvent(node->handlerForListener, (BREvent*) &message);
}

//
// All Listener Event Types
//
const BREventType *listenerEventTypes[] = {
    &listenerWalletEventType,
    &listenerBlockEventType,
    &listenerTransactionEventType,
    &listenerPeerEventType,
    &listenerLightNodeEventType
};
const unsigned int listenerEventTypesCount = 5;

// ==============================================================================================
// ==============================================================================================

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
    lightNodeHandleBalance(event->node, event->amount);
}

BREventType handleBalanceEventType = {
    "LN: Handle Balance Event",
    sizeof (BREthereumHandleBalanceEvent),
    (BREventDispatcher) lightNodeHandleBalanceEventDispatcher
};

extern void
lightNodeSignalBalance (BREthereumLightNode node,
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
    lightNodeHandleNonce(event->node, event->nonce);
}

BREventType handleNonceEventType = {
    "LN: Handle Nonce Event",
    sizeof (BREthereumHandleNonceEvent),
    (BREventDispatcher) lightNodeHandleNonceEventDispatcher
};

extern void
lightNodeSignalNonce (BREthereumLightNode node,
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
    lightNodeHandleGasPrice(event->node, event->wallet, event->gasPrice);
}

BREventType handleGasPriceEventType = {
    "LN: Handle GasPrice Event",
    sizeof (BREthereumHandleGasPriceEvent),
    (BREventDispatcher) lightNodeHandleGasPriceEventDispatcher
};

extern void
lightNodeSignalGasPrice (BREthereumLightNode node,
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
    lightNodeHandleGasEstimate(event->node, event->wallet, event->transaction, event->gasEstimate);
}

BREventType handleGasEstimateEventType = {
    "LN: Handle GasEstimate Event",
    sizeof (BREthereumHandleGasEstimateEvent),
    (BREventDispatcher) lightNodeHandleGasEstimateEventDispatcher
};

extern void
lightNodeSignalGasEstimate (BREthereumLightNode node,
                            BREthereumWallet wallet,
                            BREthereumTransaction transaction,
                            BREthereumGas gasEstimate) {
    BREthereumHandleGasEstimateEvent event = { { NULL, &handleGasEstimateEventType }, node, wallet, transaction, gasEstimate };
    eventHandlerSignalEvent(node->handlerForMain, (BREvent*) &event);
}

// ==============================================================================================
//
// Handle Transaction
//
typedef struct {
    BREvent base;
    BREthereumLightNode node;
    BREthereumTransaction transaction;
} BREthereumHandleTransactionEvent;

static void
lightNodeHandleTransactionEventDispatcher(BREventHandler ignore,
                                          BREthereumHandleTransactionEvent *event) {
    lightNodeHandleTransaction(event->node, event->transaction);
}

BREventType handleTransactionEventType = {
    "LN: Handle Transaction Event",
    sizeof (BREthereumHandleTransactionEvent),
    (BREventDispatcher) lightNodeHandleTransactionEventDispatcher
};

extern void
lightNodeSignalTransaction (BREthereumLightNode node,
                            BREthereumTransaction transaction) {
    BREthereumHandleTransactionEvent event = { { NULL, &handleTransactionEventType }, node, transaction };
    eventHandlerSignalEvent(node->handlerForMain, (BREvent*) &event);
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
    &handleTransactionEventType
};
const unsigned int handlerEventTypesCount = 5;

