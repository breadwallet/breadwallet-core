//
//  BREthereumEWMClientEvent.c
//  Core
//
//  Created by Ed Gamble on 7/19/18.
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
#include "BREthereumEWMPrivate.h"

// We use private BCS interfaces to 'inject' our JSON_RPC 'announced' results as if from LES
#include "../bcs/BREthereumBCSPrivate.h"

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
                           BREthereumWalletId wid,
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
typedef struct {
    BREvent base;
    BREthereumEWM ewm;
    BREthereumBlockId bid;
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
                          BREthereumBlockId bid,
                          BREthereumBlockEvent event,
                          BREthereumStatus status,
                          const char *errorDescription) {
    BREthereumEWMClientBlockEvent message =
    CLIENT_BLOCK_EVENT_INITIALIZER (ewm, bid, event, status, errorDescription);
    eventHandlerSignalEvent(ewm->handlerForClient, (BREvent*) &message);
}

//
// Transaction Event
//
typedef struct {
    struct BREventRecord base;
    BREthereumEWM ewm;
    BREthereumWalletId wid;
    BREthereumTransferId tid;
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
                             BREthereumWalletId wid,
                             BREthereumTransferId tid,
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
    // BREthereumWalletId wid;
    // BREthereumTransactionId tid;
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
                         // BREthereumWalletId wid,
                         // BREthereumTransactionId tid,
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
    // BREthereumWalletId wid;
    // BREthereumTransactionId tid;
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
                        // BREthereumWalletId wid,
                        // BREthereumTransactionId tid,
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

static BREventType ewmClientAnnounceTransactionEventType = {
    "EWM: Client Announce Transaction Event",
    sizeof (BREthereumEWMClientAnnounceTransactionEvent),
    (BREventDispatcher) ewmClientSignalAnnounceTransactionDispatcher
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

static BREventType ewmClientAnnounceLogEventType = {
    "EWM: Client Announce Log Event",
    sizeof (BREthereumEWMClientAnnounceLogEvent),
    (BREventDispatcher) ewmClientSignalAnnounceLogDispatcher
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

static BREventType ewmClientAnnounceTokenEventType = {
    "EWM: Client Announce Token Event",
    sizeof (BREthereumEWMClientAnnounceTokenEvent),
    (BREventDispatcher) ewmClientSignalAnnounceTokenDispatcher
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
    &ewmClientBlockEventType,
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

