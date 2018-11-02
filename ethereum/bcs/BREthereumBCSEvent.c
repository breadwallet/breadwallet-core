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

static BREventType handleSubmitTransactionEventType = {
    "BCS: Handle Submit Transaction Event",
    sizeof (BREthereumHandleSubmitTransactionEvent),
    (BREventDispatcher) bcsHandleSubmitTransactionDispatcher
};

extern void
bcsSignalSubmitTransaction (BREthereumBCS bcs,
                            OwnershipGiven BREthereumTransaction transaction) {
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
    BREthereumNodeReference node;
    BREthereumHash headHash;
    uint64_t headNumber;
    UInt256  headTotalDifficulty;
    uint64_t reorgDepth;
} BREthereumHandleAnnounceEvent;

static void
bcsHandleAnnounceDispatcher (BREventHandler ignore,
                             BREthereumHandleAnnounceEvent *event) {
    bcsHandleAnnounce(event->bcs,
                      event->node,
                      event->headHash,
                      event->headNumber,
                      event->headTotalDifficulty,
                      event->reorgDepth);
}

static BREventType handleAnnounceEventType = {
    "BCS: Handle Announce Event",
    sizeof (BREthereumHandleAnnounceEvent),
    (BREventDispatcher) bcsHandleAnnounceDispatcher
};

extern void
bcsSignalAnnounce (BREthereumBCS bcs,
                   BREthereumNodeReference node,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   UInt256 headTotalDifficulty,
                   uint64_t reorgDepth) {
    BREthereumHandleAnnounceEvent event =
    { { NULL, &handleAnnounceEventType}, bcs, node, headHash, headNumber, headTotalDifficulty, reorgDepth };
    eventHandlerSignalEvent(bcs->handler, (BREvent *) &event);
}

// ==============================================================================================
//
// Signal/Handle Status
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BREthereumNodeReference node;
    BREthereumHash headHash;
    uint64_t headNumber;
} BREthereumHandleStatusEvent;

static void
bcsHandleStatusDispatcher (BREventHandler ignore,
                           BREthereumHandleStatusEvent *event) {
    bcsHandleStatus (event->bcs,
                     event->node,
                     event->headHash,
                     event->headNumber);
}

static BREventType handleStatusEventType = {
    "BCS: Handle Status Event",
    sizeof (BREthereumHandleStatusEvent),
    (BREventDispatcher) bcsHandleStatusDispatcher
};

extern void
bcsSignalStatus (BREthereumBCS bcs,
                 BREthereumNodeReference node,
                 BREthereumHash headHash,
                 uint64_t headNumber) {
    BREthereumHandleStatusEvent event =
    { { NULL, &handleStatusEventType}, bcs, node, headHash, headNumber };
    eventHandlerSignalEvent(bcs->handler, (BREvent *) &event);
}

// ==============================================================================================
//
// Signal/Handle Provision
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BREthereumLES les;
    BREthereumNodeReference node;
    BREthereumProvisionResult result;
} BREthereumHandleProvisionEvent;

static void
bcsHandleProvisionDispatcher (BREventHandler ignore,
                                BREthereumHandleProvisionEvent *event) {
    bcsHandleProvision(event->bcs, event->les, event->node, event->result);
}

static void
bcsHandleProvisionDestroyer (BREthereumHandleProvisionEvent *event) {
    // provisionResultRelease(event->result);
}

static BREventType handleProvisionEventType = {
    "BCS: Handle Provision Event",
    sizeof (BREthereumHandleProvisionEvent),
    (BREventDispatcher) bcsHandleProvisionDispatcher,
    (BREventDestroyer) bcsHandleProvisionDestroyer
};

extern void
bcsSignalProvision (BREthereumBCS bcs,
                    BREthereumLES les,
                    BREthereumNodeReference node,
                    OwnershipGiven BREthereumProvisionResult result) {
    BREthereumHandleProvisionEvent event =
    { { NULL, &handleProvisionEventType}, bcs, les, node, result };
    eventHandlerSignalEvent(bcs->handler, (BREvent *) &event);
}

// ==============================================================================================
//
// Signal/Handle Transaction
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BREthereumTransaction transaction;
} BREthereumHandleTransactionEvent;

static void
bcsHandleTransactionDispatcher (BREventHandler ignore,
                                BREthereumHandleTransactionEvent *event) {
    bcsHandleTransaction(event->bcs, event->transaction);
}

static void
bcsHandleTransactionDestroyer (BREthereumHandleTransactionEvent *event) {
    transactionRelease(event->transaction);
}

static BREventType handleTransactionEventType = {
    "BCS: Handle Transaction Event",
    sizeof (BREthereumHandleTransactionEvent),
    (BREventDispatcher) bcsHandleTransactionDispatcher,
    (BREventDestroyer) bcsHandleTransactionDestroyer
};

extern void
bcsSignalTransaction (BREthereumBCS bcs,
                      OwnershipGiven BREthereumTransaction transaction) {
    BREthereumHandleTransactionEvent event =
    { { NULL, &handleTransactionEventType}, bcs, transaction };
    eventHandlerSignalEvent(bcs->handler, (BREvent *) &event);
}

// ==============================================================================================
//
// Signal/Handle Log
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BREthereumLog log;
} BREthereumHandleLogEvent;

static void
bcsHandleLogDispatcher (BREventHandler ignore,
                        BREthereumHandleLogEvent *event) {
    bcsHandleLog(event->bcs,
                 event->log);
}

static void
bcsHandleLogDestroyer (BREthereumHandleLogEvent *event) {
    logRelease(event->log);
}

static BREventType handleLogEventType = {
    "BCS: Handle Log Event",
    sizeof (BREthereumHandleLogEvent),
    (BREventDispatcher) bcsHandleLogDispatcher,
    (BREventDestroyer) bcsHandleLogDestroyer
};

extern void
bcsSignalLog (BREthereumBCS bcs,
              BREthereumLog log) {
    BREthereumHandleLogEvent event =
    { { NULL, &handleLogEventType}, bcs, log };
    eventHandlerSignalEvent(bcs->handler, (BREvent *) &event);
}

// ==============================================================================================
//
// Signal/Handle Peers
//
typedef struct {
    BREvent base;
    BREthereumBCS bcs;
    BRArrayOf(BREthereumNodeConfig) nodes;
} BREthereumHandleNodesEvent;

static void
bcsHandleNodesDispatcher (BREventHandler ignore,
                          BREthereumHandleNodesEvent *event) {
    bcsHandleNodes(event->bcs, event->nodes);
}

static void
bcsHandleNodesDestroyer (BREthereumHandleNodesEvent *event) {
    if (NULL != event->nodes) {
        for (size_t index = 0; index < array_count(event->nodes); index++)
            nodeConfigRelease(event->nodes[index]);
        array_free(event->nodes);
    }
}

static BREventType handleNodesEventType = {
    "BCS: Handle Nodes Event",
    sizeof (BREthereumHandleNodesEvent),
    (BREventDispatcher) bcsHandleNodesDispatcher,
    (BREventDestroyer) bcsHandleNodesDestroyer
};

extern void
bcsSignalNodes (BREthereumBCS bcs,
                BRArrayOf(BREthereumNodeConfig) nodes) {
    BREthereumHandleNodesEvent event =
    { { NULL, &handleNodesEventType}, bcs, nodes};
    eventHandlerSignalEvent (bcs->handler, (BREvent *) &event);
}

//
// MARK: - SyncProgress
//

/// ==============================================================================================
//
// Sync Signal/Handle Provision
//
typedef struct {
    BREvent base;
    BREthereumBCSSyncRange range;
    BREthereumLES les;
    BREthereumNodeReference node;
    BREthereumProvisionResult result;
} BREthereumBCSSyncHandleProvisionEvent;

static void
bcsSyncHandleProvisionDispatcher (BREventHandler ignore,
                                  BREthereumBCSSyncHandleProvisionEvent *event) {
    bcsSyncHandleProvision(event->range, event->les, event->node, event->result);
}

static void
bcsSyncHandleProvisionDestroyer (BREthereumHandleProvisionEvent *event) {
    // syncRangeRelease (event->range);
    // provisionResultRelease(event->result);
}

static BREventType handleSyncProvisionEventType = {
    "BCS: Sync Handle Provision Event",
    sizeof (BREthereumBCSSyncHandleProvisionEvent),
    (BREventDispatcher) bcsSyncHandleProvisionDispatcher,
    (BREventDestroyer) bcsSyncHandleProvisionDestroyer
};

extern void
bcsSyncSignalProvision (BREthereumBCSSyncRange range,
                    BREthereumLES les,
                    BREthereumNodeReference node,
                    BREthereumProvisionResult result) {
    BREthereumBCSSyncHandleProvisionEvent event =
    { { NULL, &handleSyncProvisionEventType }, range, les, node, result };
    eventHandlerSignalEvent(bcsSyncRangeGetHandler(range), (BREvent *) &event);
}

// ==============================================================================================
//
// BCS Event Types
//
const BREventType *
bcsEventTypes[] = {
    &handleSubmitTransactionEventType,
    &handleAnnounceEventType,
    &handleStatusEventType,
    &handleProvisionEventType,
    &handleTransactionEventType,
    &handleLogEventType,
    &handleNodesEventType,
    &handleSyncProvisionEventType
};

const unsigned int
bcsEventTypesCount = (sizeof (bcsEventTypes) / sizeof(BREventType*)); //  11
