//
//  BREthereumBCS.c
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
#include "BRArray.h"
#include "BRSet.h"
#include "BREthereumBCSPrivate.h"

#define BCS_SYNC_BLOCK_NUMBER_COUNT  (100)
#define BCS_TRANSACTION_CHECK_STATUS_SECONDS   (3)

#define BCS_HEADERS_INITIAL_CAPACITY (1024)
#define BCS_ORPHAN_HEADERS_INITIAL_CAPACITY (10)
#define BCS_PENDING_TRANSACTION_INITIAL_CAPACITY  (10)

#define BCS_TRANSACTIONS_INITIAL_CAPACITY (50)
#define BCS_LOGS_INITIAL_CAPACITY (50)

/* Forward Declarations */
static void
bcsPeriodicDispatcher (BREventHandler handler,
                       BRTimeoutEvent *event);

extern BREthereumBCS
bcsCreate (BREthereumNetwork network,
           BREthereumAccount account,
           BREthereumBlockHeader *headers,
           BREthereumBCSListener listener) {
    BREthereumBCS bcs = (BREthereumBCS) calloc (1, sizeof(struct BREthereumBCSStruct));

    bcs->network = network;
    bcs->account = account;
    bcs->address = addressGetRawAddress(accountGetPrimaryAddress(account));
    bcs->filter = bloomFilterCreateAddress(bcs->address);

    bcs->listener = listener;

    //
    // Initialize the `headers`, `chain, and `orphans`
    //
    bcs->chain = NULL;
    bcs->headers = BRSetNew(blockHeaderHashValue,
                            blockHeaderHashEqual,
                            BCS_HEADERS_INITIAL_CAPACITY);
    array_new (bcs->orphans, BCS_ORPHAN_HEADERS_INITIAL_CAPACITY);

    //
    // Initialize `transactions` and `logs` sets
    //
    bcs->transactions = BRSetNew(transactionHashValue,
                                 transactionHashEqual,
                                 BCS_TRANSACTIONS_INITIAL_CAPACITY);

    bcs->logs = BRSetNew(logHashValue,
                         logHashEqual,
                         BCS_LOGS_INITIAL_CAPACITY);

    //
    // Initialize `pendingTransactions`
    //
    array_new (bcs->pendingTransactions, BCS_PENDING_TRANSACTION_INITIAL_CAPACITY);

    // Our genesis block (header).
    BREthereumBlockHeader genesis = networkGetGenesisBlockHeader(network);

    //
    // Chain `headers` into bcs->headers, bcs->chain.
    //
    bcs->chain = genesis;
    if (NULL != headers) {
    }


    // Create but don't start the event handler.  Ensure that a fast-acting lesCreate()
    // can signal events (by queuing; they won't be handled until the event queue is started).
    bcs->handler = eventHandlerCreate(bcsEventTypes, bcsEventTypesCount);
    eventHandlerSetTimeoutDispatcher(bcs->handler,
                                     1000 * BCS_TRANSACTION_CHECK_STATUS_SECONDS,
                                     (BREventDispatcher)bcsPeriodicDispatcher,
                                     (void*) bcs);

    return bcs;
}

extern void
bcsStart (BREthereumBCS bcs) {
    BREthereumBlockHeader genesis = networkGetGenesisBlockHeader(bcs->network);

    eventHandlerStart(bcs->handler);
    bcs->les = lesCreate(bcs->network,
                         (BREthereumLESAnnounceContext) bcs,
                         (BREthereumLESAnnounceCallback) bcsSignalAnnounce,
                         blockHeaderGetHash(bcs->chain),
                         blockHeaderGetNumber(bcs->chain),
                         blockHeaderGetDifficulty(bcs->chain),
                         blockHeaderGetHash(genesis));
}

extern void
bcsStop (BREthereumBCS bcs) {
    eventHandlerStop (bcs->handler);
    if (NULL != bcs->les) {
        lesRelease(bcs->les);
        bcs->les = NULL;
    }
}

extern BREthereumBoolean
bcsIsStarted (BREthereumBCS bcs) {
    return AS_ETHEREUM_BOOLEAN(NULL != bcs->les);
}

extern void
bcsDestroy (BREthereumBCS bcs) {
    // Stop LES - avoiding any more callbacks.
//    lesRelease (bcs->les);

    // Stop the event handler - allows events to pile up.
    eventHandlerStop(bcs->handler);

    // Free internal state.
    BRSetFree(bcs->headers);
    array_free(bcs->orphans);
    BRSetFree(bcs->transactions);
    BRSetFree(bcs->logs);
    array_free(bcs->pendingTransactions);

    // Destroy the Event w/ queue
    eventHandlerDestroy(bcs->handler);
    free (bcs);
}

extern BREthereumLES
bcsGetLES (BREthereumBCS bcs) {
    return bcs->les;
}

extern void
bcsSync (BREthereumBCS bcs,
         uint64_t blockNumber) {
    lesGetBlockHeaders(bcs->les,
                       (BREthereumLESBlockHeadersContext) bcs,
                       (BREthereumLESBlockHeadersCallback) bcsSignalBlockHeader, // bcsSync...
                       blockNumber,
                       1,
                       0,
                       ETHEREUM_BOOLEAN_FALSE);
}

extern void
bcsSendTransaction (BREthereumBCS bcs,
                    BREthereumTransaction transaction) {
    bcsSignalSubmitTransaction(bcs, transaction);
}

extern void
bcsHandleSubmitTransaction (BREthereumBCS bcs,
                            BREthereumTransaction transaction) {

    // Mark `transaction` as pending; we'll perodically request status until finalized.
    array_add(bcs->pendingTransactions, transactionGetHash(transaction));

    // Use LES to submit the transaction; provide our transactionStatus callback.

    BREthereumLESStatus lesStatus =
    lesSubmitTransaction(bcs->les,
                         (BREthereumLESTransactionStatusContext) bcs,
                         (BREthereumLESTransactionStatusCallback) bcsSignalTransactionStatus,
                         TRANSACTION_RLP_SIGNED,
                         transaction);

    switch (lesStatus) {
        case LES_SUCCESS:
            break;
        case LES_UNKNOWN_ERROR:
        case LES_NETWORK_UNREACHABLE: {
            BREthereumTransactionStatus status;
            status.type = TRANSACTION_STATUS_ERROR;
            status.u.error.message = "LES Submit Failed";
            bcsSignalTransactionStatus(bcs, transactionGetHash(transaction), status);
            break;
        }
    }
}

/*!
 * @function bcsHandleAnnounce
 *
 * @abstract
 * Handle a LES 'announce' result.
 */
extern void
bcsHandleAnnounce (BREthereumBCS bcs,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   uint64_t headTotalDifficulty) {
    // Request the block.
    lesGetBlockHeaders(bcs->les,
                       (BREthereumLESBlockHeadersContext) bcs,
                       (BREthereumLESBlockHeadersCallback) bcsSignalBlockHeader,
                       headNumber,
                       1,
                       0,
                       ETHEREUM_BOOLEAN_FALSE);
}

/*!
 */
extern void
bcsHandleBlockHeader (BREthereumBCS bcs,
                      BREthereumBlockHeader header) {

    // Ignore the header if we have seen it before.  Given an identical hash, *nothing*, at any
    // level (transactions, receipts, logs), could have changed and thus no processing is needed.
    if (NULL != BRSetGet(bcs->headers, header)) return;

    // Ignore the header if it is not valid.  We might 'ping' the PeerManager or better
    // maybe the PeerManager does this test so as to only send 'textually' valid header.
    if (ETHEREUM_BOOLEAN_IS_FALSE(blockHeaderIsValid (header))) return;

    // Lookup `headerParent`
    BREthereumHash headerParentHash = blockHeaderGetParentHash(header);
    BREthereumBlockHeader headerParent = BRSetGet(bcs->headers, &headerParentHash);

    // If we have a parent, but the block numbers are not consistent, then ignore `header`
    if (NULL != headerParent && blockHeaderGetNumber(header) != 1 + blockHeaderGetNumber(headerParent))
        return;

    // Other checks.

    // Add `header` to the set of headers
    BRSetAdd(bcs->headers, header);

    // Put `header` in the `chain`

    // If we don't have a parent for `header` then `header` is an orphan.  Skip out.
    if (NULL == headerParent) {
        array_add(bcs->orphans, header);
        return;
    }

    // If the parent is an orphan then `header` is an orphan.  Skip out.
    for (int i = 0; i < array_count(bcs->orphans); i++)
        if (headerParent == bcs->orphans[i]) {
            array_add (bcs->orphans, header);
            return;
        }

    // Can we assert that `headerParent` is in `chain`

    // TODO: He we actually have a linked-list of blockHeader?  Or just parent hashes?
    //   Well, yeah - figure that out.

    // Every header between `chain` and `headerParent` is now an orphan
    while (NULL != bcs->chain && headerParent != bcs->chain) {
        BREthereumHash chainParentHash = blockHeaderGetParentHash(bcs->chain);
        // Make an orphan from an existing chain element
        // TODO: Handle unchained transactions, logs
        array_add (bcs->orphans, bcs->chain);

        // continue back.
        bcs->chain = BRSetGet(bcs->headers, &chainParentHash);
    }

    // Must be there; right?
    assert (NULL != bcs->chain);

    // Extend the chain
    bcs->chain = header;

    // Orphans may now be chained - one-by-one if their parent matches bcs->chain
    int keepLooking = 1;
    while (keepLooking) {
        keepLooking = 0;
        for (int i = 0; i <array_count (bcs->orphans); i++) {
            if (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(blockHeaderGetHash (bcs->chain),
                                                   blockHeaderGetParentHash(bcs->orphans[i])))) {
                // Extend the chain.
                bcs->chain = bcs->orphans[i];

                // No longer an orphan
                array_rm(bcs->orphans, i);

                // Skip out (of `for`) but keep looking.
                keepLooking = 1;
                break;
            }
        }
    }

    // We need block info for every header between bcs->chain and headerParent - because we
    // added orphans - or might have
    BREthereumHash *headerHashes;
    array_new (headerHashes, 5);
    header = bcs->chain;
    while (NULL != header && header != headerParent) {
        // check the bloomFilter for a match - note this *must* match any of a) transaction
        // source/target addresses and b) log source/target addresses.  If the header's bloom filter
        // does not match both transactions and logs then we *must* get the block bodies and the
        // transactions receipts to check for matches one-by-one.
        //
        // If the header's bloom filter only matches addresses in transactions then alternatively
        // we can look for a match with any contract address that we are interested in.  If a
        // contract matches, then we must get the transaction receipts to check for any log with
        // an address match.
        if (ETHEREUM_BOOLEAN_IS_TRUE(blockHeaderMatch(header, bcs->filter))) {
            array_insert (headerHashes, 0, blockHeaderGetHash(header));
        }
        // Did the bloom filter match from/to and contract from/to?
        else if (ETHEREUM_BOOLEAN_IS_TRUE(ETHEREUM_BOOLEAN_FALSE)) {

        }

        // Next header...
        headerParentHash = blockHeaderGetParentHash(header);
        header = BRSetGet(bcs->headers, &headerParentHash);
    }

    // If he have matching headers, then we'll request blockBodies.
    if (array_count(headerHashes) > 0) {
        // Get the blockbody
        lesGetBlockBodies(bcs->les,
                          (BREthereumLESBlockBodiesContext) bcs,
                          (BREthereumLESBlockBodiesCallback) bcsSignalBlockBodies,
                          headerHashes);
    }
    else array_free(headerHashes);

    // Save blocks ??
}

/*!
 */
extern void
bcsHandleBlockBodies (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transactions[],
                      BREthereumHash ommers[]) {

    // Only request receipts if any one transaction is of interest.
    int needReceipts = 0;

    // Check the transactions one-by-one.
    for (BREthereumTransaction *txs = transactions; NULL != *txs; txs++) {
        // If it is our transaction (as source or target), handle it.
        if (ETHEREUM_BOOLEAN_IS_TRUE(transactionHasAddress(*txs, bcs->address))) {
            needReceipts = 1;

            // Save the transaction -
            //   a) in a BRSet and/or
            //   b) In a Block

            // bcsHandleTransaction (bcs, blockHash, *txs);

            // get the account state
            // announce: nonce, balance
        }
        // TODO: Handle if has a 'contract' address of interest?
    }

    if (needReceipts)
        lesGetReceiptsOne(bcs->les,
                          (BREthereumLESReceiptsContext) bcs,
                          (BREthereumLESReceiptsCallback) bcsSignalTransactionReceipts,
                          blockHash);
}

// TODO: Add transactionIndex
extern void
bcsHandleTransaction (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transaction) {
    // TODO: Get Block
    BREthereumBlockHeader header = NULL;

    BREthereumTransactionStatus status;
    status.type = TRANSACTION_STATUS_INCLUDED;
    status.u.included.gasUsed = gasCreate(0);
    status.u.included.blockHash = blockHeaderGetHash(header);
    status.u.included.blockNumber = blockHeaderGetNonce(header);
    // TODO: Get transactionIndex
    status.u.included.transactionIndex = 0;

    bcs->listener.transactionCallback (bcs->listener.context, transaction);

}

/*!
 */
extern void
bcsHandleTransactionStatus (BREthereumBCS bcs,
                            BREthereumHash transactionHash,
                            BREthereumTransactionStatus status) {

    // TODO: Get Transaction
    BREthereumTransaction transaction = NULL;

    //
    // TODO: Do we get 'included' before or after we see transaction, in the block body?
    //  If we get 'included' we should ignore because a) we'll see the transaction
    //  later and b) we don't have any block information to provide in transaction anyway.
    //
    switch (status.type) {
        case TRANSACTION_STATUS_UNKNOWN:
        case TRANSACTION_STATUS_QUEUED:
            break;
        case TRANSACTION_STATUS_PENDING:
            transactionAnnounceSubmitted(transaction, transactionHash);
            break;

        case TRANSACTION_STATUS_INCLUDED:
            transactionAnnounceBlocked(transaction,
                                       gasCreate(0),
                                       status.u.included.blockHash,
                                       status.u.included.blockNumber,
                                       status.u.included.transactionIndex);
            break;

        case TRANSACTION_STATUS_ERROR:
            transactionAnnounceDropped(transaction, 0);
            // TRANSACTION_EVENT_SUBMITTED, ERROR_TRANSACTION_SUBMISSION, event->status.u.error.message
            break;

        case TRANSACTION_STATUS_CREATED:
        case TRANSACTION_STATUS_SIGNED:
        case TRANSACTION_STATUS_SUBMITTED:
            // TODO: DO
            break;
    }
    bcs->listener.transactionCallback (bcs->listener.context, transaction);
}

/*!
 */
extern void
bcsHandleTransactionReceipts (BREthereumBCS bcs,
                             BREthereumHash blockHash,
                              BREthereumTransactionReceipt *receipts) {
    size_t receiptsCount = array_count(receipts);
    for (int i = 0; i < receiptsCount; i++) {
        BREthereumTransactionReceipt receipt = receipts[i];
        if (transactionReceiptMatch(receipt, bcs->filter)) {
            BREthereumBlock block = NULL; //blockHash
            BREthereumTransaction transaction = blockGetTransaction(block, i);

            size_t logsCount = transactionReceiptGetLogsCount(receipt);
            for (size_t index = 0; index < logsCount; index++) {
                BREthereumLog log = transactionReceiptGetLog(receipt, index);

                // If `log` is of interest
                if (ETHEREUM_BOOLEAN_IS_TRUE (logMatchesAddress(log, bcs->address, ETHEREUM_BOOLEAN_TRUE))) {
                    // create a 'private' transaction

                    // Lookup token...
                    //   Confirm 'transfer topic'

                    // something related to transaction+log maybe TRANSACTION_EVENT_BLOCKED and then
                    // gasCreate (transactionReceiptGetGasUsed(event->receipt))
                    // bcs->listener.transactionCallback (bcs->listener.context, bcs, transaction);
                }
            }
        }
    }
}

/*!
 */
extern void
bcsHandleLog (BREthereumBCS bcs,
              BREthereumHash blockHash,
              BREthereumHash transactionHash, // transaction?
              BREthereumLog log) {
}

static void
bcsPeriodicDispatcher (BREventHandler handler,
                             BRTimeoutEvent *event) {
    BREthereumBCS bcs = (BREthereumBCS) event->context;

    // If nothing to do; simply skip out.
    if (NULL == bcs->pendingTransactions || 0 == array_count(bcs->pendingTransactions))
        return;

    lesGetTransactionStatus (bcs->les,
                             (BREthereumLESTransactionStatusContext) bcs,
                             (BREthereumLESTransactionStatusCallback) bcsSignalTransactionStatus,
                             bcs->pendingTransactions);
}

