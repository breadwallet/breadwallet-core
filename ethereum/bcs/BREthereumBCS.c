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

    // From headers: extract headhash, headNumber, headTotalDifficulty
    // From network: extract genesisHash

    bcs->account = account;
    bcs->address = addressGetRawAddress(accountGetPrimaryAddress(account));
    bcs->filter = bloomFilterCreateAddress(bcs->address);

    bcs->listener = listener;

    //
    // Initialize the `headers`, `chain, and `orphans`
    //
    bcs->headers = BRSetNew(blockHeaderHashValue,
                            blockHeaderHashEqual,
                            BCS_HEADERS_INITIAL_CAPACITY);
    bcs->chain = NULL;
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

    //
    // Chain `headers` into bcs->headers, bcs->chain.
    //

    // Create but don't start the event handler.  Ensure that a fast-acting lesCreate()
    // can signal events (by queuing; they won't be handled until the event queue is started).
    bcs->handler = eventHandlerCreate(bcsEventTypes, bcsEventTypesCount);
    eventHandlerSetTimeoutDispatcher(bcs->handler,
                                     1000 * BCS_TRANSACTION_CHECK_STATUS_SECONDS,
                                     (BREventDispatcher)bcsPeriodicDispatcher,
                                     (void*) bcs);

    bcs->les = lesCreate(network,
                         (BREthereumLESAnnounceContext) bcs,
                         (BREthereumLESAnnounceCallback) bcsSignalAnnounce,
                         hashCreateEmpty(),
                         0, 0,
                         hashCreateEmpty());

    // Start handling events.
    eventHandlerStart(bcs->handler);

    return bcs;
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

    // Mark `transaction` as pending; we'll perodically request status until finalized.
    array_add(bcs->pendingTransactions, transactionGetHash(transaction));

    // Use LES to submit the transaction; provide our transactionStatus callback.
    lesSubmitTransaction(bcs->les,
                         (BREthereumLESTransactionStatusContext) bcs,
                         (BREthereumLESTransactionStatusCallback) bcsSignalTransactionStatus,
                         transaction);
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

    // Add `header` to the set of headers
    // Put `header` in the chain
    //   first, move 'conflicting' headers to `orphans`
    //   then, extend as the new head

    // Handle transactions (and logs) that now are included but with an orphaned block
    //   Move to pending?
    //

    // check the bloomFilter for a match - note this *must* match any of a) transaction
    // source/target addresses and b) log source/target addresses.  If the header's bloom filter
    // does not match both transactions and logs then we *must* get the block bodies and the
    // transactions receipts to check for matches one-by-one.
    //
    // If the header's bloom filter only matches addresses in transactions then alternatively
    // we can look for a match with any contract address that we are interested in.  If a
    // contract matches, then we must get the transaction receipts to check for any log with
    // an address match.
    if (ETHEREUM_BOOLEAN_IS_TRUE (blockHeaderMatch(header, bcs->filter))) {
        // Get the blockbody
        lesGetBlockBodiesOne(bcs->les,
                             (BREthereumLESBlockBodiesContext) bcs,
                             (BREthereumLESBlockBodiesCallback) bcsSignalBlockBodies,
                             blockHeaderGetHash(header));
    }

    // Did the bloom filter match from/to and contract from/to?
    else if (ETHEREUM_BOOLEAN_IS_TRUE(ETHEREUM_BOOLEAN_FALSE)) {

    }

    // Save blocks
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

