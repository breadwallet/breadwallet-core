//
//  BREthereumBCSEvent.c
//  Core
//
//  Created by Ed Gamble on 5/24/18.
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
#include "BREthereumBCSPrivate.h"

// ==============================================================================================
//
// Signal/Handle Transaction
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BREthereumTransaction transaction;
} BREthereumHandleSubmitTransactionEvent;

static void
bcsHandleSubmitTransactionDispatcher (BREventHandler ignore,
                                      BREthereumHandleSubmitTransactionEvent *event) {
    bcsHandleSubmitTransaction(event->bcs,
                               event->transaction);
}

BREventType handleSubmitTransactionEventType = {
    "BCS: Handle Submit Transaction Event",
    sizeof (BREthereumHandleSubmitTransactionEvent),
    (BREventDispatcher) bcsHandleSubmitTransactionDispatcher
};

extern void
bcsSignalSubmitTransaction (BREthereumBCS bcs,
                            BREthereumTransaction transaction) {
    BREthereumHandleSubmitTransactionEvent event =
    { { NULL, &handleSubmitTransactionEventType}, bcs, transaction };
    eventHandlerSignalEvent(bcs->handler, (BREvent *) &event);
}


// ==============================================================================================
//
// Signal/Handle Announce
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BREthereumHash headHash;
    uint64_t headNumber;
    uint64_t headTotalDifficulty;
} BREthereumHandleAnnounceEvent;

static void
bcsHandleAnnounceDispatcher (BREventHandler ignore,
                             BREthereumHandleAnnounceEvent *event) {
    bcsHandleAnnounce(event->bcs, event->headHash, event->headNumber, event->headTotalDifficulty);
}

BREventType handleAnnounceEventType = {
    "BCS: Handle Announce Event",
    sizeof (BREthereumHandleAnnounceEvent),
    (BREventDispatcher) bcsHandleAnnounceDispatcher
};

extern void
bcsSignalAnnounce (BREthereumBCS bcs,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   uint64_t headTotalDifficulty) {
    BREthereumHandleAnnounceEvent event =
    { { NULL, &handleAnnounceEventType}, bcs, headHash, headNumber, headTotalDifficulty};
    eventHandlerSignalEvent(bcs->handler, (BREvent *) &event);
}

// ==============================================================================================
//
// Signal/Handle Block Header
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BREthereumBlockHeader header;
} BREthereumHandleBlockHeaderEvent;

static void
bcsHandleBlockHeaderDispatcher (BREventHandler ignore,
                                BREthereumHandleBlockHeaderEvent *event) {
    bcsHandleBlockHeader(event->bcs, event->header);
}

BREventType handleBlockHeaderEventType = {
    "BCS: Handle Block Header Event",
    sizeof (BREthereumHandleBlockHeaderEvent),
    (BREventDispatcher) bcsHandleBlockHeaderDispatcher
};

extern void
bcsSignalBlockHeader (BREthereumBCS bcs,
                      BREthereumBlockHeader header) {
    BREthereumHandleBlockHeaderEvent event =
    { { NULL, &handleBlockHeaderEventType}, bcs, header };
    eventHandlerSignalEvent(bcs->handler, (BREvent *) &event);
}

// ==============================================================================================
//
// Signal/Handle Block Bodies
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BREthereumHash blockHash;
    BREthereumTransaction *transactions;
    BREthereumBlockHeader *ommers;
} BREthereumHandleBlockBodiesEvent;

static void
bcsHandleBlockBodiesDispatcher (BREventHandler ignore,
                                BREthereumHandleBlockBodiesEvent *event) {
    bcsHandleBlockBodies(event->bcs,
                         event->blockHash,
                         event->transactions,
                         event->ommers);
}

BREventType handleBlockBodiesEventType = {
    "BCS: Handle Block Bodies Event",
    sizeof (BREthereumHandleBlockBodiesEvent),
    (BREventDispatcher) bcsHandleBlockBodiesDispatcher
};

extern void
bcsSignalBlockBodies (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transactions[],
                      BREthereumBlockHeader ommers[]) {
    BREthereumHandleBlockBodiesEvent event =
    { { NULL, &handleBlockBodiesEventType}, bcs, blockHash, transactions, ommers };
    eventHandlerSignalEvent(bcs->handler, (BREvent *) &event);
}

// ==============================================================================================
//
// Signal/Handle Transaction
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BREthereumHash blockHash;
    BREthereumTransaction transaction;
} BREthereumHandleTransactionEvent;

static void
bcsHandleTransactionDispatcher (BREventHandler ignore,
                                BREthereumHandleTransactionEvent *event) {
    bcsHandleTransaction(event->bcs,
                         event->blockHash,
                         event->transaction);
}

BREventType handleTransactionEventType = {
    "BCS: Handle Transaction Event",
    sizeof (BREthereumHandleTransactionEvent),
    (BREventDispatcher) bcsHandleTransactionDispatcher
};

extern void
bcsSignalTransaction (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transaction) {
    BREthereumHandleTransactionEvent event =
    { { NULL, &handleTransactionEventType}, bcs, blockHash, transaction };
    eventHandlerSignalEvent(bcs->handler, (BREvent *) &event);
}

// ==============================================================================================
//
// Handle Transaction Status
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BREthereumHash transactionHash;
    BREthereumTransactionStatus status;
} BREthereumHandleTransactionStatusEvent;

static void
bcsHandleTransactionStatusDispatcher(BREventHandler ignore,
                                     BREthereumHandleTransactionStatusEvent *event) {
    bcsHandleTransactionStatus(event->bcs, event->transactionHash, event->status);
}

BREventType handleTransactionStatusEventType = {
    "BCS: Handle TransactionStatus Event",
    sizeof (BREthereumHandleTransactionStatusEvent),
    (BREventDispatcher) bcsHandleTransactionStatusDispatcher
};

extern void
bcsSignalTransactionStatus (BREthereumBCS bcs,
                            BREthereumHash transactionHash,
                            BREthereumTransactionStatus status) {
    BREthereumHandleTransactionStatusEvent event =
    { { NULL, &handleTransactionStatusEventType }, bcs, transactionHash, status };
    eventHandlerSignalEvent(bcs->handler, (BREvent*) &event);
}


// ==============================================================================================
//
// Handle Transaction Receipt
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BREthereumHash blockHash;
    BREthereumTransactionReceipt *receipts;
} BREthereumHandleTransactionReceiptEvent;

static void
bcsHandleTransactionReceiptsDispatcher(BREventHandler ignore,
                                       BREthereumHandleTransactionReceiptEvent *event) {
    bcsHandleTransactionReceipts(event->bcs, event->blockHash, event->receipts);
}

BREventType handleTransactionReceiptEventType = {
    "BCS: Handle TransactionReceipt Event",
    sizeof (BREthereumHandleTransactionReceiptEvent),
    (BREventDispatcher) bcsHandleTransactionReceiptsDispatcher
};

extern void
bcsSignalTransactionReceipts (BREthereumBCS bcs,
                              BREthereumHash blockHash,
                              BREthereumTransactionReceipt *receipts) {
    BREthereumHandleTransactionReceiptEvent event =
    { { NULL, &handleTransactionReceiptEventType }, bcs, blockHash, receipts };
    eventHandlerSignalEvent(bcs->handler, (BREvent*) &event);
}

// ==============================================================================================
//
// Signal/Handle Log
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BREthereumHash blockHash;
    BREthereumHash transactionHash;
    BREthereumLog log;
} BREthereumHandleLogEvent;

static void
bcsHandleLogDispatcher (BREventHandler ignore,
                        BREthereumHandleLogEvent *event) {
    bcsHandleLog(event->bcs,
                 event->blockHash,
                 event->transactionHash,
                 event->log);
}

BREventType handleLogEventType = {
    "BCS: Handle Log Event",
    sizeof (BREthereumHandleLogEvent),
    (BREventDispatcher) bcsHandleLogDispatcher
};

extern void
bcsSignalLog (BREthereumBCS bcs,
              BREthereumHash blockHash,
              BREthereumHash transactionHash,
              BREthereumLog log) {
    BREthereumHandleLogEvent event =
    { { NULL, &handleLogEventType}, bcs, blockHash, transactionHash, log };
    eventHandlerSignalEvent(bcs->handler, (BREvent *) &event);
}


// ==============================================================================================
//
// BCS Event Types
//
const BREventType *bcsEventTypes[] = {
    &handleAnnounceEventType,
    &handleBlockHeaderEventType,
    &handleBlockBodiesEventType,
    &handleTransactionEventType,
    &handleTransactionStatusEventType,
    &handleTransactionReceiptEventType,
    &handleLogEventType
};
const unsigned int bcsEventTypesCount = 7;
