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
    bcs->filterForAddressOnTransactions = bloomFilterCreateAddress(bcs->address);
    bcs->filterForAddressOnLogs = logTopicGetBloomFilterAddress(bcs->address);

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

static BREthereumBoolean
bcsBlockHeaderHasMatchingTransactions (BREthereumBCS bcs,
                                       BREthereumBlockHeader header) {
    return ETHEREUM_BOOLEAN_TRUE;
}

static BREthereumBoolean
bcsBlockHeaderHasMatchingLogs (BREthereumBCS bcs,
                               BREthereumBlockHeader header) {
    return blockHeaderMatch(header, bcs->filterForAddressOnLogs);
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

    // Examine transactions/logs to see if any are not chained or orphaned.
    //   Change their 'confirmation status' accordingly.

    // We need block bodies and transactions for every matcjhing header between bcs->chain
    // and headerParent - because we added orphans - or might have.
    BREthereumHash *headerHashesTransactions, *headerHashesLogs;
    array_new (headerHashesTransactions, 5);
    array_new (headerHashesLogs, 5);
    header = bcs->chain;
    while (NULL != header && header != headerParent) {
        // If `header` has matching transactions, then we'll get the block body.
        if (ETHEREUM_BOOLEAN_IS_TRUE(bcsBlockHeaderHasMatchingTransactions(bcs, header))) {
            array_insert (headerHashesTransactions, 0, blockHeaderGetHash(header));
        }

        if (ETHEREUM_BOOLEAN_IS_TRUE(bcsBlockHeaderHasMatchingLogs(bcs, header))) {
            array_insert (headerHashesLogs, 0, blockHeaderGetHash(header));
        }
        // Next header...
        headerParentHash = blockHeaderGetParentHash(header);
        header = BRSetGet(bcs->headers, &headerParentHash);
    }

    // If he have matching headers, then we'll request blockBodies.
    if (array_count(headerHashesTransactions) > 0) {
        // Get the blockbody
        lesGetBlockBodies(bcs->les,
                          (BREthereumLESBlockBodiesContext) bcs,
                          (BREthereumLESBlockBodiesCallback) bcsSignalBlockBodies,
                          headerHashesTransactions);
    }
    else array_free(headerHashesTransactions);

    if (array_count(headerHashesLogs) > 0) {
        // Get the transaction receipts.
        lesGetReceipts(bcs->les,
                       (BREthereumLESReceiptsContext) bcs,
                       (BREthereumLESReceiptsCallback) bcsSignalTransactionReceipts,
                       headerHashesLogs);
    }
    else array_free(headerHashesLogs);

    // Save blocks - on a difficulty boundry?
}

/*!
 */
extern void
bcsHandleBlockBodies (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transactions[],
                      BREthereumHash ommers[]) {

    BREthereumBlockHeader header = BRSetGet(bcs->headers, &blockHash);
    if (NULL == header) return;

    BREthereumBlock block = createBlock(header,
                                        ommers, array_count(ommers),
                                        transactions, array_count(transactions));

    if (ETHEREUM_BOOLEAN_IS_FALSE(blockIsValid(block, ETHEREUM_BOOLEAN_TRUE))) {
        blockRelease(block);
        return;
    }

    // When there is a transaction of interest, we'll want to get account balance and nonce info.
    int hasTransactionOfInterest = 0;

    // Check the transactions one-by-one.
    for (BREthereumTransaction *txs = transactions; NULL != *txs; txs++) {
        // If it is our transaction (as source or target), handle it.
        if (ETHEREUM_BOOLEAN_IS_TRUE(transactionHasAddress(*txs, bcs->address))) {
            hasTransactionOfInterest = 1;

            // Save the transaction
            BRSetAdd(bcs->transactions, *txs);

            // Get the status explicitly; apparently this is the *only* way to get the gasUsed.
            lesGetTransactionStatusOne(bcs->les,
                                       (BREthereumLESTransactionStatusContext) bcs,
                                       (BREthereumLESTransactionStatusCallback) bcsSignalTransactionStatus,
                                       transactionGetHash(*txs));
        }
        else /* release transaction */ ;

        // TODO: Handle if has a 'contract' address of interest?
    }

    if (hasTransactionOfInterest) {
        // TODO: Something interesting for AccountState {balance, nonce}
    }

    blockRelease(block);
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
                                       status.u.included.gasUsed,
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

//
// MARK: - Transaction Receipts
//

static BREthereumTransaction
bcsHandleLogCreateTransaction (BREthereumBCS bcs,
                               BREthereumLog log,
                               BREthereumToken token) {

    BREthereumAddress sourceAddr = logTopicAsAddress(logGetTopic(log, 1));
    BREthereumAddress targetAddr = logTopicAsAddress(logGetTopic(log, 2));

    // TODO: No Way
    BRRlpData valueData = logGetData(log);
    UInt256 *value = (UInt256 *) &valueData.bytes[valueData.bytesCount - sizeof(UInt256)];
    
    BREthereumAmount amount = amountCreateToken(createTokenQuantity(token, *value));

    // TODO: No Bueno
    BREthereumGasPrice gasPrice = tokenGetGasPrice(token);
    BREthereumGas gasLimit = tokenGetGasLimit(token);

    return transactionCreate(sourceAddr,
                      targetAddr,
                      amount,
                      gasPrice,
                      gasLimit,
                      0);

}

static BREthereumBoolean
bcsHandleLogExtractInterest (BREthereumBCS bcs,
                             BREthereumLog log,
                             BREthereumToken *token,
                             BREthereumContractEvent *tokenEvent) {
    assert (NULL != token && NULL != tokenEvent);

    *token = NULL;
    *tokenEvent = NULL;

    if (ETHEREUM_BOOLEAN_IS_FALSE (logMatchesAddress(log, bcs->address, ETHEREUM_BOOLEAN_TRUE)))
        return ETHEREUM_BOOLEAN_FALSE;

    *token = tokenLookupByAddress(logGetAddress(log));
    if (NULL == *token) return ETHEREUM_BOOLEAN_FALSE;

    BREthereumLogTopicString topicString = logTopicAsString(logGetTopic(log, 0));
    *tokenEvent = contractLookupEventForTopic(contractERC20,  topicString.chars);
    if (NULL == *tokenEvent) return ETHEREUM_BOOLEAN_FALSE;

    return ETHEREUM_BOOLEAN_TRUE;
}

/*!
 */
extern void
bcsHandleTransactionReceipts (BREthereumBCS bcs,
                              BREthereumHash blockHash,
                              BREthereumTransactionReceipt *receipts) {
    BREthereumBlockHeader header = BRSetGet(bcs->headers, &blockHash);
    BREthereumBlock block = NULL; //blockHash

    size_t receiptsCount = array_count(receipts);
    for (int i = 0; i < receiptsCount; i++) {
        BREthereumTransactionReceipt receipt = receipts[i];
        if (transactionReceiptMatch(receipt, bcs->filterForAddressOnLogs)) {
            BREthereumTransaction transaction = blockGetTransaction(block, i);

            size_t logsCount = transactionReceiptGetLogsCount(receipt);
            for (size_t index = 0; index < logsCount; index++) {
                BREthereumLog log = transactionReceiptGetLog(receipt, index);
                logSetHash(log, transactionGetHash(transaction));

                BREthereumToken token;
                BREthereumContractEvent tokenEvent;

                // If `log` is of interest...
                if (ETHEREUM_BOOLEAN_IS_TRUE
                    (bcsHandleLogExtractInterest(bcs, log, &token,&tokenEvent))) {

                    BRSetAdd(bcs->logs, log);

                    // Create a private transaction
                    BREthereumTransaction logTransfer = bcsHandleLogCreateTransaction(bcs, log, token);

                    // Announce the transaction
                    transactionAnnounceBlocked(logTransfer,
                                               gasCreate(0),
                                               blockHeaderGetHash(header),
                                               blockHeaderGetNumber(header),
                                               i);

                    bcs->listener.transactionCallback (bcs->listener.context, logTransfer);
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

