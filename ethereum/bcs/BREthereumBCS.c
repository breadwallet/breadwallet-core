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

// Any orphan more then AGE_OFFSET blocks in the past will be purged.
#define BCS_ORPHAN_AGE_OFFSET  (10)

#define BCS_SAVE_BLOCKS_COUNT  (100)

/* Forward Declarations */
static void
bcsPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);

extern BREthereumBCS
bcsCreate (BREthereumNetwork network,
           BREthereumAccount account,
           BREthereumBlockHeader *headers,
           BREthereumBCSListener listener) {

    // Stubbed, for now.
    BREthereumTransaction *transactions = NULL;
    array_new(transactions, 0);

    BREthereumLog *logs = NULL;
    array_new(logs, 0);

    BREthereumBCS bcs = (BREthereumBCS) calloc (1, sizeof(struct BREthereumBCSStruct));

    bcs->network = network;
    bcs->account = account;
    bcs->accountState = accountStateCreateEmpty ();
    bcs->address = accountGetPrimaryAddress(account);
    bcs->filterForAddressOnTransactions = bloomFilterCreateAddress(bcs->address);
    bcs->filterForAddressOnLogs = logTopicGetBloomFilterAddress(bcs->address);


    bcs->listener = listener;

    //
    // Initialize the `headers`, `chain, and `orphans`
    //
    bcs->chain = NULL;
    bcs->chainTail = NULL;
    bcs->headers = BRSetNew(blockHeaderHashValue,
                            blockHeaderHashEqual,
                            BCS_HEADERS_INITIAL_CAPACITY);
    bcs->orphans = BRSetNew(blockHeaderHashValue,
                           blockHeaderHashEqual,
                           BCS_ORPHAN_HEADERS_INITIAL_CAPACITY);

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
    BRSetAdd(bcs->headers, genesis);

    //
    // Initialize `chain` - hackily.
    //
    bcs->chain = bcs->chainTail = genesis;

    if (NULL != headers) {
        // THIS SHOULD DUPLICATE 'NORMAL HEADER PROCESSING'
        //    [Set chain to NULL; find the earliest; make all others orphans; handle the earliest]
        //    [Implies looking for transactions/logs - which we don't need.]

        // Iterate over `headers` to recreate `chain`.  In general we cannot assume anything about
        // `headers` - might have gaps (missing parent/child); might have duplicates.  Likely, we must
        // be willing to create orphans, to discard/ignore headers, and what.
        //
        // We'll sort `headers` ascending by {blockNumber, timestamp}. Then we'll interate and chain
        // them together while ignoring any duplicates/orphans.
        size_t sortedHeadersCount = array_count(headers);
        BREthereumBlockHeader *sortedHeaders;
        array_new(sortedHeaders, sortedHeadersCount);
        array_add_array(sortedHeaders, headers, sortedHeadersCount);

        // TODO: Sort

        bcs->chainTail = NULL;
        for (int i = 0; i < sortedHeadersCount; i++) {
            // Skip header `i` if its blockNumber equals the blockNumber of `i+1`.
            if (i + 1 < sortedHeadersCount &&
                blockHeaderGetNumber(sortedHeaders[i]) == blockHeaderGetNumber(sortedHeaders[i+1]))
                continue;

            // TODO: Check for orpahns

            BRSetAdd(bcs->headers, sortedHeaders[i]);
            bcs->chain = sortedHeaders[i];

            if (NULL == bcs->chainTail)
                bcs->chainTail = bcs->chain;
        }
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
    // Ensure we are stopped and no longer handling events (anything submitted will pile up).
    if (ETHEREUM_BOOLEAN_IS_TRUE(bcsIsStarted(bcs)))
        bcsStop (bcs);

    // TODO: We'll need to announce things to our `listener`

    // Free internal state.
    BRSetFree(bcs->headers);
    BRSetFree(bcs->orphans);
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
    BRSetAdd(bcs->transactions, transaction);

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
            bcsSignalTransactionStatus(bcs,
                                       transactionGetHash(transaction),
                                       transactionStatusCreateErrored("LES Submit Failed"));
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
    return ETHEREUM_BOOLEAN_FALSE;
}

static BREthereumBoolean
bcsBlockHeaderHasMatchingLogs (BREthereumBCS bcs,
                               BREthereumBlockHeader header) {
    return blockHeaderMatch(header, bcs->filterForAddressOnLogs);
}

static void
bcsReclaimBlock (BREthereumBCS bcs,
                 BREthereumBlockHeader header) {
    BRSetRemove(bcs->headers, header);
    eth_log("BCS", "Header %llu Reclaimed", blockHeaderGetNumber(header));

    // TODO: Avoid dangling references

    // TODO: Can't release genesis nor checkpoints
    if (0 != blockHeaderGetNumber(header))
        blockHeaderRelease(header);
}

static void
bcsReclaimAndSaveBlocksIfAppropriate (BREthereumBCS bcs) {
    uint64_t chainBlockNumber = blockHeaderGetNumber(bcs->chain);
    uint64_t chainBlockLength = chainBlockNumber - blockHeaderGetNumber(bcs->chainTail);

    // Note, we might have chained together a number of blocks.  Thus this method might be called
    // with bcs->chain not on a 'boundary' (currently: 0 == chainBlockNumber/BCS_SAVE_BLOCKS_COUNT)
    if (chainBlockLength > 2 * BCS_SAVE_BLOCKS_COUNT) {
        BREthereumBlockHeader header = bcs->chain;
        uint64_t thisBlockNumber, reclaimFromBlockNumber = chainBlockNumber - BCS_SAVE_BLOCKS_COUNT;

        // Walk bcs->chain back to BCS_SAVE_BLOCKS_COUNT, then start reclaiming.
        while (NULL != header) {
            BREthereumHash parentHash = blockHeaderGetParentHash(header);

            thisBlockNumber = blockHeaderGetNumber(header);
            if (thisBlockNumber == reclaimFromBlockNumber)
                bcs->chainTail = header;
            else if (thisBlockNumber < reclaimFromBlockNumber)
                bcsReclaimBlock(bcs, header);

            header = BRSetGet(bcs->headers, &parentHash);
        }

        header = bcs->chain;
        while (header != NULL && header != bcs->chainTail) {
            BREthereumHash parentHash = blockHeaderGetParentHash(header);
            eth_log("BCS", "Header %llu Saved", blockHeaderGetNumber(header));
            header = BRSetGet(bcs->headers, &parentHash);
        }
        eth_log("BCS", "Header %llu Saved", blockHeaderGetNumber(header));
    }
}


/**
 * Find the minumum block number amoung orphans. I think we can use this to identify when
 * syncing is done... except when the block is a true orphan.
 */
static uint64_t
bcsGetOrphanBlockNumberMinimum (BREthereumBCS bcs) {
    // TODO: Handle the 'true orphan' case
    uint64_t number = UINT64_MAX;
    FOR_SET(BREthereumBlockHeader, orphan, bcs->orphans)
        if (blockHeaderGetNumber(orphan) < number)
            number = blockHeaderGetNumber(orphan);
    return number;
}

/**
 * Unceremoniously dump any orphans older then (`blockNumber` - AGE_OFFSET) - their time has past.
 * Expect `blockNumber` to be the blockNumber at the head of the chain.
 */
static void
bcsPurgeOrphans (BREthereumBCS bcs,
                 uint64_t blockNumber) {
    // If blockNumber is below AGE_OFFSET, then there is nothing to do.  Said another way,
    // don't orphans when we are syncing from the genesis block.
    if (blockNumber <= BCS_ORPHAN_AGE_OFFSET) return;

    // Modify blockNumber for comparision with orphans
    blockNumber -= BCS_ORPHAN_AGE_OFFSET;

    // Look through all the orphans; remove those with old/small block numbers
    int keepLooking = 1;
    while (keepLooking) {
        keepLooking = 0;
        FOR_SET(BREthereumBlockHeader, orphan, bcs->orphans)
            if (blockHeaderGetNumber(orphan) < blockNumber) {
                BRSetRemove(bcs->orphans, orphan);
                eth_log("BCS", "Header %llu Purged Orphan", blockHeaderGetNumber(orphan));
                keepLooking = 1;
                break; // FOR_SET
            }
    }
}

static void
bcsChainOrphans (BREthereumBCS bcs) {
    // Examine bcs->orphans looking for any with a parent that is bcs->chain.
    // TODO: Can we have two orphans with the same parent - deal with it.
    int keepLooking = 1;
    while (keepLooking) {
        keepLooking = 0;
        // We should look up bcs->orphans based on parentHash, see Aaron's Core code.
        FOR_SET(BREthereumBlockHeader, orphan, bcs->orphans) {
            if (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(blockHeaderGetHash (bcs->chain),
                                                   blockHeaderGetParentHash(orphan)))) {
                // Extend the chain.
                bcs->chain = orphan;
                eth_log("BCS", "Header %llu Chained Orphan", blockHeaderGetNumber(orphan));

                // No longer an orphan
                BRSetRemove(bcs->orphans, orphan);
                // Our FOR_SET iteration is now broken, so ...

                // ... skip out (of `for`) but keep looking.
                keepLooking = 1;
                break;
            }
        }
    }
}

static void
bcsMakeOrphan (BREthereumBCS bcs,
               BREthereumBlockHeader header) {
    BRSetAdd (bcs->orphans, header);
    eth_log ("BCS", "Header %llu Newly Orphaned", blockHeaderGetNumber(header));

    // TODO: Is this the proper place to examine transactions and logs now orphaned?
}

static void
bcsChainThenPurgeOrphans (BREthereumBCS bcs) {
    bcsChainOrphans(bcs);
    bcsPurgeOrphans(bcs,  blockHeaderGetNumber(bcs->chain));
}

static void
bcsSyncFrom (BREthereumBCS bcs,
             uint64_t chainBlockNumber) {
    // Sync up, if need be.  We'll need to sync if the minimum orphan header is larger then
    // the chain header (by more than just one).
#define BCS_HEADER_REQUEST_MAXIMUM  (1)
    uint64_t orphanBlockNumberMinumum = bcsGetOrphanBlockNumberMinimum(bcs);
    if (UINT64_MAX != orphanBlockNumberMinumum && orphanBlockNumberMinumum > chainBlockNumber + 1) {
        uint64_t needHeadersCount = orphanBlockNumberMinumum - (chainBlockNumber + 1);
        if (needHeadersCount > BCS_HEADER_REQUEST_MAXIMUM)
            needHeadersCount = BCS_HEADER_REQUEST_MAXIMUM;

        // Make the request
        eth_log("BCS", "Header Sync {%llu, %llu}",
                chainBlockNumber + 1,
                chainBlockNumber + 1 + needHeadersCount);

        lesGetBlockHeaders(bcs->les,
                           (BREthereumLESBlockHeadersContext) bcs,
                           (BREthereumLESBlockHeadersCallback) bcsSignalBlockHeader,
                           chainBlockNumber + 1,
                           needHeadersCount,
                           0,
                           ETHEREUM_BOOLEAN_FALSE);
    }
}

/*!
 */
extern void
bcsHandleBlockHeader (BREthereumBCS bcs,
                      BREthereumBlockHeader header) {

    // Ignore the header if we have seen it before.  Given an identical hash, *nothing*, at any
    // level (transactions, receipts, logs), could have changed and thus no processing is needed.
    if (NULL != BRSetGet(bcs->headers, header)) {
        eth_log("BCS", "Header %llu Ignored", blockHeaderGetNumber(header));
        return;
    }

    // Ignore the header if it is not valid.
    if (ETHEREUM_BOOLEAN_IS_FALSE(blockHeaderIsValid (header))) {
        eth_log("BCS", "Header %llu Invald", blockHeaderGetNumber(header));
        return;
    }

    // Lookup `headerParent`
    BREthereumHash headerParentHash = blockHeaderGetParentHash(header);
    BREthereumBlockHeader headerParent = BRSetGet(bcs->headers, &headerParentHash);

    // If we have a parent, but the block numbers are not consistent, then ignore `header`
    if (NULL != headerParent && blockHeaderGetNumber(header) != 1 + blockHeaderGetNumber(headerParent)) {
        eth_log("BCS", "Header %llu Inconsistent", blockHeaderGetNumber(header));
        return;
    }

    // Other checks.

    // Add `header` to the set of headers
    BRSetAdd(bcs->headers, header);

    // Put `header` in the `chain` - HANDLE 3 CASES:

    // 1) If we do not have any chain, then adopt `header` directly, no questions asked.  This will
    // be used for SYNC_MODE_PRIME_WITH_ENDPOINT where we get all interesting transactions, logs,
    // etc from the ENDPOINT and just want to process new blocks as they are announced;
    if (NULL == bcs->chain) {
        bcs->chain = bcs->chainTail = header;
    }

    // 2) If there is no `header` parent or if  `header` parent is an orphan, then `header` is
    // an orphan too.  Add it to the set of orphans and RETURN (non-local exit);
    else if (NULL == headerParent || NULL != BRSetGet(bcs->orphans, headerParent)) {
        bcsMakeOrphan(bcs, header);

        // If `header` is an orphan, then it's parent is not in bcs->chain.  That could be
        // because there is just a fork developing or that we've fallen behind.  Attempt a
        // sync to recover (might not actually perform a sync - attempt).
        bcsSyncFrom(bcs, blockHeaderGetNumber(bcs->chain));
        return;
    }

    // 3) othewise, we have a new `header` that links to a parent that is somewhere in the
    // chain.  All headers from chain back to parent are now orphans.  In practice, there will
    // be only one (or two or three) orphans.
    //
    // Can we assert that `headerParent` is in `chain` if it is not an orphan?
    else {
        // Every header between `chain` and `headerParent` is now an orphan
        while (NULL != bcs->chain && headerParent != bcs->chain) {
            BREthereumHash chainParentHash = blockHeaderGetParentHash(bcs->chain);

            // Make an orphan from an existing chain element
            bcsMakeOrphan(bcs, bcs->chain);

            // continue back.
            bcs->chain = BRSetGet(bcs->headers, &chainParentHash);
        }
        // TODO: Handle bcs->chainTail

        // Must be there; right?
        assert (NULL != bcs->chain);

        // Extend the chain
        bcs->chain = header;
    }
    eth_log("BCS", "Header %llu Chained", blockHeaderGetNumber(header));

    bcsChainThenPurgeOrphans (bcs);

    // Examine transactions/logs to see if any are not chained or orphaned.
    //   Change their 'confirmation status' accordingly.
    FOR_SET(BREthereumTransaction, tx, bcs->transactions) {
        BREthereumHash blockHash;
        if (transactionExtractIncluded(tx, NULL, &blockHash, NULL, NULL)) {
            if (NULL != BRSetGet (bcs->orphans, &blockHash)) {
                // TODO: Make TX an orphan
            }
            else if (NULL != BRSetGet (bcs->headers, &blockHash)) {
                // TODO: Make TX included - wasn't it already
            }
        }
    }

    FOR_SET(BREthereumLog, log, bcs->logs) {
        BREthereumHash hash = logGetHash(log);
        BREthereumTransaction transaction = BRSetGet (bcs->transactions, &hash);
        // TODO: Mirror 'log' status as 'transaction'
    }

    // We need block bodies and transactions for every matching header between bcs->chain
    // and headerParent - because we added orphans - or might have.
    BREthereumHash *headerHashesTransactions, *headerHashesLogs;
    array_new (headerHashesTransactions, 5);
    array_new (headerHashesLogs, 5);
    header = bcs->chain;
    while (NULL != header && header != headerParent) {
        // If `header` has matching transactions, then we'll get the block body.
        if (ETHEREUM_BOOLEAN_IS_TRUE(bcsBlockHeaderHasMatchingTransactions(bcs, header))) {
            eth_log("BCS", "Header %llu Need Transactions", blockHeaderGetNumber(header));
            array_insert (headerHashesTransactions, 0, blockHeaderGetHash(header));
        }

        if (ETHEREUM_BOOLEAN_IS_TRUE(bcsBlockHeaderHasMatchingLogs(bcs, header))) {
            eth_log("BCS", "Header %llu Need Logs", blockHeaderGetNumber(header));
            array_insert (headerHashesLogs, 0, blockHeaderGetHash(header));
        }
        // Next header...
        headerParentHash = blockHeaderGetParentHash(header);
        header = BRSetGet(bcs->headers, &headerParentHash);
    }

    // If we have matching transactions, then we'll request blockBodies.
    if (array_count(headerHashesTransactions) > 0) {
        // Get the blockbody
        lesGetBlockBodies(bcs->les,
                          (BREthereumLESBlockBodiesContext) bcs,
                          (BREthereumLESBlockBodiesCallback) bcsSignalBlockBodies,
                          headerHashesTransactions);
    }
    else array_free(headerHashesTransactions);

    // If we have matchines logs, then we'll request transactionReceipts
    if (array_count(headerHashesLogs) > 0) {
        // Get the transaction receipts.
        lesGetReceipts(bcs->les,
                       (BREthereumLESReceiptsContext) bcs,
                       (BREthereumLESReceiptsCallback) bcsSignalTransactionReceipts,
                       headerHashesLogs);
    }
    else array_free(headerHashesLogs);

    // Periodically reclaim 'excessive' blocks and save the latest.
    bcsReclaimAndSaveBlocksIfAppropriate (bcs);

    // TODO: Not here, likely.
    bcsSyncFrom (bcs, blockHeaderGetNumber(bcs->chain));
}

/*!
 */
extern void
bcsHandleAccountState (BREthereumBCS bcs,
                       BREthereumHash blockHash,
                       BREthereumAddress address,
                       BREthereumAccountState state) {
    // If the AccountState differs, then this blockHash has transactions of interest.
    // Otherwise, we'll skip out immediately.
    if (ETHEREUM_BOOLEAN_IS_TRUE(accountStateEqual(bcs->accountState, state))) return;



}

/*!
 */
extern void
bcsHandleBlockBodies (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transactions[],
                      BREthereumBlockHeader ommers[]) {

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
    for (int i = 0; i < array_count(transactions); i++) {
        BREthereumTransaction tx = transactions[i];
        // If it is our transaction (as source or target), handle it.
        if (ETHEREUM_BOOLEAN_IS_TRUE(transactionHasAddress(tx, bcs->address))) {
            hasTransactionOfInterest = 1;

            // Save the transaction
            BRSetAdd(bcs->transactions, tx);

            // Get the status explicitly; apparently this is the *only* way to get the gasUsed.
            lesGetTransactionStatusOne(bcs->les,
                                       (BREthereumLESTransactionStatusContext) bcs,
                                       (BREthereumLESTransactionStatusCallback) bcsSignalTransactionStatus,
                                       transactionGetHash(tx));
        }
        else /* release transaction */ ;

        // TODO: Handle if has a 'contract' address of interest?
    }

    if (hasTransactionOfInterest) {
        // TODO: Something interesting for AccountState {balance, nonce}
    }

    blockRelease(block);
}

/*!
 * Check if `blockHash` and `blockNumber` are in the chain.  They will be in the chain if:
 *   a) blockNumber is smaller than the chain's earliest maintained block number, or
 *   b1) blockNumber is not larger than the chain's latest maintained block number and
 *   b2) blockHash is not an orphan and
 *   b4) blockHash is known.
 */
static BREthereumBoolean
bcsChainHasBlock (BREthereumBCS bcs,
                  BREthereumHash blockHash,
                  uint64_t blockNumber) {
    return AS_ETHEREUM_BOOLEAN(blockNumber < blockHeaderGetNumber(bcs->chainTail) ||
                               (blockNumber <= blockHeaderGetNumber(bcs->chain) &&
                                NULL == BRSetGet(bcs->orphans, &blockHash) &&
                                NULL != BRSetGet(bcs->headers, &blockHash)));
}

static int
bcsLookupPendingTransaction (BREthereumBCS bcs,
                             BREthereumHash hash) {
    for (int i = 0; i < array_count(bcs->pendingTransactions); i++)
        if (ETHEREUM_BOOLEAN_IS_TRUE (hashEqual(bcs->pendingTransactions[i], hash)))
            return i;
    return -1;
}

extern void
bcsHandleTransactionStatus (BREthereumBCS bcs,
                            BREthereumHash transactionHash,
                            BREthereumTransactionStatus status) {
    // We only observe transaction status for transactions that we've originated.  Therefore
    // we simply *must* have a transaction for transactionHash.  Perhaps this is untrue -
    // if we get a transaction sent to use but it's block is orphaned, then we will pend the
    // transaction and start requesting status updates.
    BREthereumTransaction transaction = BRSetGet(bcs->transactions, &transactionHash);
    if (NULL == transaction) return;  // And yet...

    // Get the current (aka 'old') status.
    BREthereumTransactionStatus oldStatus = transactionGetStatus(transaction);

    // We'll assume we are pending.  We generated a transaction status request from pending
    // transactions so assuming pending is reasonable.  However, while processing a block this
    // transaction may have been included and removed as pending.
    int isInChain = 0;

    // A transation in error is a 'terminal state' and the transaction won't ever be in the chain;
    // we'll remove the transaction from pending.  Note: transaction can be resubmitted.
    int isAnError = 0;

    // We have seen back-to-back status messages on a submit transaction.  The first is
    // an error: 'transaction underpriced'; the second is type 'unknown'.  Must be that the first
    // status is the result of 'submit' and the second is a result of a pend update.
    //
    // Conclusion: if the transaction is in error, then it ain't coming back.  Change `status`
    // to be `oldStatus` and then continue on.
    //
    // TODO: Confirm this on 'resubmitted' transactions.
    if (TRANSACTION_STATUS_ERRORED == oldStatus.type)
        status = oldStatus;

    // We have also seen a type of 'pending' (felt joy) and then surprisingly, in a subsequent
    // status result, a type of 'unknown'.  It is as if the GETH node passd on the transaction,
    // had nothing in its 'mempool' and thus declared 'unknown'.  We'll try to handle this
    // reasonably, if we can.
    if (TRANSACTION_STATUS_UNKNOWN == status.type)
        status = oldStatus;

    // Check based on the reported status.type...
    switch (status.type) {
        case TRANSACTION_STATUS_UNKNOWN:
            // It appears that we can get to `unknown` from any old type.  We've seen
            // 'pending' -> 'unknown'.  So, if status.type is 'unknown' simply adop

            status.type = TRANSACTION_STATUS_SIGNED; // Surely not just CREATED.
            break;

            // Awkward... (why have 'submitted' if 'queued/pending' indicates the same?)
        case TRANSACTION_STATUS_QUEUED:
        case TRANSACTION_STATUS_PENDING:
            status.type = TRANSACTION_STATUS_SUBMITTED;
            break;

        case TRANSACTION_STATUS_INCLUDED:
            // With status of `included` this transaction is in a block.  However, we *will not*
            // consider this transaction as included *until and unless* we have the transaction's
            // included block in the chain.  At worst, this leaves the transaction pending and
            // we'll need another couple status requests before we conclude that the transaction
            // is in the chain.
            isInChain = ETHEREUM_BOOLEAN_IS_TRUE (bcsChainHasBlock (bcs,
                                                                    status.u.included.blockHash,
                                                                    status.u.included.blockNumber));

            // Even if included we'll revert to 'submitted' if not in the chain.
            if (!isInChain) status.type = TRANSACTION_STATUS_SUBMITTED;
            break;

        case TRANSACTION_STATUS_ERRORED:
            isInChain = 0;
            isAnError = 1;
            break;

        case TRANSACTION_STATUS_CREATED:
        case TRANSACTION_STATUS_SIGNED:
        case TRANSACTION_STATUS_SUBMITTED:
            assert (0); // LES *cannot* report these.
            break;
    }

    // If in the chain or on an error, then remove from pending
    if (isInChain || isAnError) {
        int index = bcsLookupPendingTransaction(bcs, transactionHash);
        if (-1 != index) {
            array_rm(bcs->pendingTransactions, index);
            eth_log("BCS", "Transaction: \"0x%c%c%c%c...\", Pending: 0",
                    _hexc (transactionHash.bytes[0] >> 4), _hexc(transactionHash.bytes[0]),
                    _hexc (transactionHash.bytes[1] >> 4), _hexc(transactionHash.bytes[1]));
        }
    }
    // ... but if not in chain and not an error, then add to pending.  Presumably this could occur
    // if while processing a block we mark the transaction as included *but* owing to a fork we
    // get a non-included status.  Just make it pending again and wait for the fork to resolve.
    else if (!isInChain && !isAnError && -1 == bcsLookupPendingTransaction(bcs, transactionHash)) {
        array_add (bcs->pendingTransactions, transactionHash);
        eth_log("BCS", "Transaction: \"0x%c%c%c%c...\", Pending: 1",
                _hexc (transactionHash.bytes[0] >> 4), _hexc(transactionHash.bytes[0]),
                _hexc (transactionHash.bytes[1] >> 4), _hexc(transactionHash.bytes[1]));
    }

    // If the status has changed, then report
    if (ETHEREUM_BOOLEAN_IS_FALSE(transactionStatusEqual(status, oldStatus))) {
        transactionSetStatus(transaction, status);
        eth_log("BCS", "Transaction: \"0x%c%c%c%c...\", Status: %d, Included: %d, Pending: %d%s%s",
                _hexc (transactionHash.bytes[0] >> 4), _hexc(transactionHash.bytes[0]),
                _hexc (transactionHash.bytes[1] >> 4), _hexc(transactionHash.bytes[1]),
                status.type,
                isInChain,
                -1 != bcsLookupPendingTransaction(bcs, transactionHash),
                (TRANSACTION_STATUS_ERRORED == status.type ? ", Error: " : ""),
                (TRANSACTION_STATUS_ERRORED == status.type ? status.u.errored.reason : ""));
        bcs->listener.transactionCallback (bcs->listener.context, transaction);
    }
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
                    transactionSetStatus(logTransfer,
                                         transactionStatusCreateIncluded(gasCreate(0),
                                                                         blockHeaderGetHash(header),
                                                                         blockHeaderGetNumber(header),
                                                                         i));
                    bcs->listener.transactionCallback (bcs->listener.context, logTransfer);
                }
            }
        }
    }
}

//
// Periodicaly get the transaction status for all pending transaction
//
static void
bcsPeriodicDispatcher (BREventHandler handler,
                             BREventTimeout *event) {
    BREthereumBCS bcs = (BREthereumBCS) event->context;

    // If nothing to do; simply skip out.
    if (NULL == bcs->pendingTransactions || 0 == array_count(bcs->pendingTransactions))
        return;

    lesGetTransactionStatus (bcs->les,
                             (BREthereumLESTransactionStatusContext) bcs,
                             (BREthereumLESTransactionStatusCallback) bcsSignalTransactionStatus,
                             bcs->pendingTransactions);
}


//
// Unneeded
//

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
bcsHandleLog (BREthereumBCS bcs,
              BREthereumHash blockHash,
              BREthereumHash transactionHash, // transaction?
              BREthereumLog log) {
}

