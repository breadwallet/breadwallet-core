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

#define BCS_TRANSACTION_CHECK_STATUS_SECONDS   (3)

#define BCS_BLOCKS_INITIAL_CAPACITY (1024)
#define BCS_ORPHAN_BLOCKS_INITIAL_CAPACITY (10)
#define BCS_PENDING_TRANSACTION_INITIAL_CAPACITY  (10)
#define BCS_PENDING_LOGS_INITIAL_CAPACITY  (10)

#define BCS_TRANSACTIONS_INITIAL_CAPACITY (50)
#define BCS_LOGS_INITIAL_CAPACITY (50)

// Any orphan more then AGE_OFFSET blocks in the past will be purged.  That is, if the head block
// is N, then any orphan block we hold at (N - ARG_OFFSET) will no longer be chainable - unless
// there is some 'giant' fork developing...
#define BCS_ORPHAN_AGE_OFFSET  (10)

// We'll save every 500 blocks.  On restart we'll expect these blocks to be passed to bcsCreate()
// so as to initialize the chain.
#define BCS_SAVE_BLOCKS_COUNT  (500)

/* Forward Declarations */
static void
bcsPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);

static void
bcsExtendChain (BREthereumBCS bcs,
                BREthereumBlock block,
                const char *message);

static void
bcsSyncReportBlocksCallback (BREthereumBCS bcs,
                             BREthereumBCSSync sync,
                             BRArrayOf(BREthereumBCSSyncResult) blocks);

static void
bcsSyncReportProgressCallback (BREthereumBCS bcs,
                               BREthereumBCSSync sync,
                               uint64_t blockNumberBeg,
                               uint64_t blockNumberNow,
                               uint64_t blockNumberEnd);
/**
 */
static void
bcsCreateInitializeBlocks (BREthereumBCS bcs,
                            BRArrayOf(BREthereumBlock) blocks) {
    if (NULL == blocks) return;

    bcs->chain = bcs->chainTail = NULL;

    // THIS SHOULD DUPLICATE 'NORMAL HEADER PROCESSING'
    //    [Set chain to NULL; find the earliest; make all others orphans; handle the earliest]
    //    [Implies looking for transactions/logs - which we don't need.]

    // Iterate over `blocks` to recreate `chain`.  In general we cannot assume anything about
    // `blocks` - might have gaps (missing parent/child); might have duplicates.  Likely, we must
    // be willing to create orphans, to discard/ignore blocks, and what.
    //
    // We'll sort `blocks` ascending by {blockNumber, timestamp}. Then we'll interate and chain
    // them together while ignoring any duplicates/orphans.
    size_t sortedBlocksCount = array_count(blocks);
    BREthereumBlock *sortedBlocks;
    array_new(sortedBlocks, sortedBlocksCount);
    array_add_array(sortedBlocks, blocks, sortedBlocksCount);

    // TODO: Sort

    for (int i = 0; i < sortedBlocksCount; i++) {
        // Skip block `i` if its blockNumber equals the blockNumber of `i+1`.
        BREthereumBlock block = sortedBlocks[i];

        if (i + 1 < sortedBlocksCount &&
            blockGetNumber(block) == blockGetNumber(sortedBlocks[i+1]))
            continue;

        // TODO: Check for orpahns

        BRSetAdd(bcs->blocks, block);
        bcsExtendChain(bcs, block, "Chained (from Saved)");

        if (NULL == bcs->chainTail)
            bcs->chainTail = bcs->chain;
    }

    array_free(sortedBlocks);
    array_free(blocks);
}

static void
bcsCreateInitializeTransactions (BREthereumBCS bcs,
                                 BRArrayOf(BREthereumTransaction) transactions) {
    if (NULL == transactions) return;
    
    for (size_t index = 0; index < array_count(transactions); index++) {
        BREthereumTransaction transaction = transactions[index];
        BREthereumTransactionStatus status = transactionGetStatus(transaction);

        // For now, assume all provided transactions are in a 'final state'.
        assert (TRANSACTION_STATUS_INCLUDED == status.type ||
                TRANSACTION_STATUS_ERRORED  == status.type ||
                TRANSACTION_STATUS_PENDING  == status.type);

        bcsSignalTransaction(bcs, transaction);
    }
}

static void
bcsCreateInitializeLogs (BREthereumBCS bcs,
                         BRArrayOf(BREthereumLog) logs) {
    if (NULL == logs) return;

    for (size_t index = 0; index < array_count(logs); index++) {
        BREthereumLog log = logs[index];
        BREthereumTransactionStatus status = logGetStatus(log);

        // For now, assume all provided logs are in a 'final state'.
        assert (TRANSACTION_STATUS_INCLUDED == status.type ||
                TRANSACTION_STATUS_ERRORED  == status.type ||
                TRANSACTION_STATUS_PENDING  == status.type);

        bcsSignalLog(bcs, log);
    }
}

extern BREthereumBCS
bcsCreate (BREthereumNetwork network,
           BREthereumAddress address,
           BREthereumBCSListener listener,
           BRArrayOf(BREthereumPeerConfig) peers,
           BRArrayOf(BREthereumBlock) blocks,
           BRArrayOf(BREthereumTransaction) transactions,
           BRArrayOf(BREthereumLog) logs) {
           // peers

    BREthereumBCS bcs = (BREthereumBCS) calloc (1, sizeof(struct BREthereumBCSStruct));

    bcs->network = network;
    bcs->address = address;
    bcs->accountState = accountStateCreateEmpty ();
    bcs->filterForAddressOnTransactions = bloomFilterCreateAddress(bcs->address);
    bcs->filterForAddressOnLogs = logTopicGetBloomFilterAddress(bcs->address);

    bcs->listener = listener;

    //
    // Initialize the `headers`, `chain, and `orphans`
    //
    bcs->chain = NULL;
    bcs->chainTail = NULL;
    bcs->blocks = BRSetNew(blockHashValue,
                           blockHashEqual,
                           BCS_BLOCKS_INITIAL_CAPACITY);
    bcs->orphans = BRSetNew(blockHashValue,
                            blockHashEqual,
                            BCS_ORPHAN_BLOCKS_INITIAL_CAPACITY);

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
    array_new (bcs->pendingLogs, BCS_PENDING_LOGS_INITIAL_CAPACITY);

    // Our genesis block.
    bcs->genesis = networkGetGenesisBlock(network);
    BRSetAdd(bcs->blocks, bcs->genesis);

    // Create but don't start the event handler.  Ensure that a fast-acting lesCreate()
    // can signal events (by queuing; they won't be handled until the event queue is started).
    bcs->handler = eventHandlerCreate(bcsEventTypes, bcsEventTypesCount);
    eventHandlerSetTimeoutDispatcher(bcs->handler,
                                     1000 * BCS_TRANSACTION_CHECK_STATUS_SECONDS,
                                     (BREventDispatcher)bcsPeriodicDispatcher,
                                     (void*) bcs);

    // Initialize `chain` - will be modified based on `headers`
    bcs->chain = bcs->chainTail = bcs->genesis;

    bcsCreateInitializeBlocks(bcs, blocks);
    bcsCreateInitializeTransactions(bcs, transactions);
    bcsCreateInitializeLogs(bcs, logs);

    // peers - save for bcsStart; or, better, use lesCreate() here and lesStart() later.
    return bcs;
}

extern void
bcsStart (BREthereumBCS bcs) {
    BREthereumBlockHeader genesis = blockGetHeader(bcs->genesis);
    BREthereumBlockHeader header  = blockGetHeader(bcs->chain);

    eventHandlerStart(bcs->handler);
    bcs->les = lesCreate(bcs->network,
                         (BREthereumLESAnnounceContext) bcs,
                         (BREthereumLESAnnounceCallback) bcsSignalAnnounce,
                         blockHeaderGetHash(header),
                         blockHeaderGetNumber(header),
                         blockHeaderGetDifficulty(header),
                         blockHeaderGetHash(genesis));

    bcs->sync = bcsSyncCreate ((BREthereumBCSSyncContext) bcs,
                               (BREthereumBCSSyncReportBlocks) bcsSyncReportBlocksCallback,
                               (BREthereumBCSSyncReportProgress) bcsSyncReportProgressCallback,
                               bcs->address,
                               bcs->les,
                               bcs->handler);
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

    // Headers
    BRSetApply(bcs->blocks, NULL, blockReleaseForSet);
    BRSetFree(bcs->blocks);

    // Orphans (All are in 'headers')
    BRSetFree(bcs->orphans);

    // Transaction
    BRSetApply(bcs->transactions, NULL, transactionReleaseForSet);
    BRSetFree(bcs->transactions);

    // Logs
    BRSetApply(bcs->logs, NULL, logReleaseForSet);
    BRSetFree(bcs->logs);

    // pending transactions/logs are in bcs->transactions/logs; thus already released.
    array_free(bcs->pendingTransactions);
    array_free(bcs->pendingLogs);

    // Destroy the Event w/ queue
    eventHandlerDestroy(bcs->handler);
    free (bcs);
}

extern void
bcsSync (BREthereumBCS bcs,
         uint64_t blockNumber) {
    bcsSyncContinue(bcs->sync, blockGetNumber(bcs->chain), blockNumber);
}

extern BREthereumBoolean
bcsSyncInProgress (BREthereumBCS bcs) {
    return bcsSyncIsActive(bcs->sync);
}

extern void
bcsSendTransaction (BREthereumBCS bcs,
                    BREthereumTransaction transaction) {
    bcsSignalSubmitTransaction(bcs, transaction);
}

extern void
bcsSendTransactionRequest (BREthereumBCS bcs,
                           BREthereumHash transactionHash,
                           uint64_t blockNumber,
                           uint64_t blockTransactionIndex) {
    lesGetBlockHeaders(bcs->les,
                       (BREthereumLESBlockHeadersContext) bcs,
                       (BREthereumLESBlockHeadersCallback) bcsSignalBlockHeader,
                       blockNumber, 1, 0, ETHEREUM_BOOLEAN_FALSE);
    // How to know to get block bodies?
    //    Well, should find the transaction (otherwise we've big problems)
}

extern void
bcsSendLogRequest (BREthereumBCS bcs,
                   BREthereumHash transactionHash,
                   uint64_t blockNumber,
                   uint64_t blockTransactionIndex) {
    lesGetBlockHeaders(bcs->les,
                       (BREthereumLESBlockHeadersContext) bcs,
                       (BREthereumLESBlockHeadersCallback) bcsSignalBlockHeader,
                       blockNumber, 1, 0, ETHEREUM_BOOLEAN_FALSE);
    // How to know to get block bodies?
    //    Well, should find the log (otherwise we've big problems)
}

extern void
bcsHandleSubmitTransaction (BREthereumBCS bcs,
                            BREthereumTransaction transaction) {

    bcsSignalTransaction(bcs, transaction);

    // Use LES to submit the transaction; provide our transactionStatus callback.
    BREthereumLESStatus lesStatus =
    lesSubmitTransaction(bcs->les,
                         (BREthereumLESTransactionStatusContext) bcs,
                         (BREthereumLESTransactionStatusCallback) bcsSignalTransactionStatus,
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
                   UInt256 headTotalDifficulty,
                   uint64_t reorgDepth) {
    // Request the block.
    lesGetBlockHeaders(bcs->les,
                       (BREthereumLESBlockHeadersContext) bcs,
                       (BREthereumLESBlockHeadersCallback) bcsSignalBlockHeader,
                       headNumber,
                       1,
                       0,
                       ETHEREUM_BOOLEAN_FALSE);
}

///
/// MARK: - Chain
///
static void
bcsReclaimBlock (BREthereumBCS bcs,
                 BREthereumBlock block,
                 int useLog) {
    BRSetRemove(bcs->blocks, block);
    if (useLog) eth_log("BCS", "Block %llu Reclaimed", blockGetNumber(block));

    // TODO: Avoid dangling references

    blockRelease(block);
}

static void
bcsSaveBlocks (BREthereumBCS bcs) {
    // We'll save everything between bcs->chain->next and bcs->chainTail
    unsigned long long blockCount = blockGetNumber(bcs->chain) - blockGetNumber(bcs->chainTail);

    // We'll pass long the blocks directly, for now.  This will likely need to change because
    // this code will lose control of the block memory.
    //
    // For example, we quickly hit another 'ReclaimAndSaveBlocks' point prior the the
    // `saveBlocksCallback` completing - we'll then release memory needed by the callback handler.
    BRArrayOf(BREthereumBlock) blocks;
    array_new(blocks, blockCount);
    array_set_count(blocks, blockCount);

    BREthereumBlock next = blockGetNext (bcs->chain);
    BREthereumBlock last = bcs->chainTail;
    unsigned long long blockIndex = blockCount - 1;

    while (next != last) {
        blocks[blockIndex--] = next;
        next = blockGetNext(next);
    }
    assert (0 == blockIndex);
    blocks[blockIndex--] = next;

    bcs->listener.saveBlocksCallback (bcs->listener.context, blocks);
    
    eth_log("BCS", "Blocks {%llu, %llu} Saved",
            blockGetNumber(bcs->chainTail),
            blockGetNumber(blockGetNext(bcs->chain)));
}

static void
bcsReclaimAndSaveBlocksIfAppropriate (BREthereumBCS bcs) {
    uint64_t chainBlockNumber = blockGetNumber(bcs->chain);
    uint64_t chainBlockLength = chainBlockNumber - blockGetNumber(bcs->chainTail);

    // Note, we might have chained together a number of blocks.  Thus this method might be called
    // with bcs->chain not on a 'boundary' (currently: 0 == chainBlockNumber/BCS_SAVE_BLOCKS_COUNT)
    if (chainBlockLength >= 2 * BCS_SAVE_BLOCKS_COUNT) {
        uint64_t thisBlockNumber = blockGetNumber(bcs->chain);
        uint64_t reclaimFromBlockNumber = chainBlockNumber - BCS_SAVE_BLOCKS_COUNT;

        // Walk bcs->chain back to BCS_SAVE_BLOCKS_COUNT, then keep walking but start reclaiming.
        for (BREthereumBlock block = bcs->chain; NULL != block;) {
            // Save this before bcsReclaimBlock() zeros it.
            BREthereumBlock nextBlock = blockGetNext(block);

            thisBlockNumber = blockGetNumber(block);
            if (thisBlockNumber == reclaimFromBlockNumber)
                bcs->chainTail = block;
            else if (thisBlockNumber < reclaimFromBlockNumber)
                bcsReclaimBlock(bcs, block, 0);

            block = nextBlock;
        }
        blockClrNext(bcs->chainTail);

        eth_log("BCS", "Blocks {%llu, %llu} Reclaimed",
                thisBlockNumber,
                reclaimFromBlockNumber - 1);

        bcsSaveBlocks(bcs);
    }
}


/**
 * Extend `bcs->chain` with `block`. Announce the new chain with the `blockChainCallback`
 */
static void
bcsExtendChain (BREthereumBCS bcs,
                BREthereumBlock block,
                const char *message) {
    assert (NULL != block);

    blockSetNext(block, bcs->chain);
    bcs->chain = block;

    eth_log("BCS", "Block %llu %s", blockGetNumber(block), message);

    bcs->listener.blockChainCallback (bcs->listener.context,
                                      blockGetHash(block),
                                      blockGetNumber(block),
                                      blockGetTimestamp(block));
}

/**
 * Find the minumum block number amoung orphans. I think we can use this to identify when
 * syncing is done... except when the block is a true orphan.
 */
static uint64_t
bcsGetOrphanBlockNumberMinimum (BREthereumBCS bcs) {
    // TODO: Handle the 'true orphan' case
    uint64_t number = UINT64_MAX;
    FOR_SET(BREthereumBlock, orphan, bcs->orphans)
        if (blockGetNumber(orphan) < number)
            number = blockGetNumber(orphan);
    return number;
}

/**
 * Unceremoniously dump any orphans older then (`blockNumber` - AGE_OFFSET) - their time has past.
 * Expect `blockNumber` to be the blockNumber at the head of the chain.
 *
 * TODO: Shouldn't remove if still active!
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
        FOR_SET(BREthereumBlock, orphan, bcs->orphans)
            if (blockGetNumber(orphan) < blockNumber) {
                BRSetRemove(bcs->orphans, orphan);
                eth_log("BCS", "Block %llu Purged Orphan", blockGetNumber(orphan));
                blockRelease(orphan); // TODO: Pending Requests... crash?
                keepLooking = 1;
                break; // FOR_SET
            }
    }
}


/**
 * Select between `block1` and `block2` for extending the chain. (We assume the two blocks have
 * the same parent hash).  Selectin criteria include: totalDifficulty and timestamp.
 */
static BREthereumBlock
bcsSelectPreferredBlock (BREthereumBCS bcs,
                         BREthereumBlock block1,
                         BREthereumBlock block2) {
    if (NULL == block1) return block2;
    if (NULL == block2) return block1;

    // Gitter: "hi. when chain reorg occurs in Ethereum, the common way is first find common block, insert
    // the block with the more total difficuty, and keep the transaction within the old chain but
    // not within the new chain back to txpool."
    
    BREthereumBlockHeader header1 = blockGetHeader(block1);
    BREthereumBlockHeader header2 = blockGetHeader(block2);

    return (gtUInt256 (blockHeaderGetDifficulty(header1), blockHeaderGetDifficulty(header2))
            ? block1
            : (ltUInt256 (blockHeaderGetDifficulty(header1), blockHeaderGetDifficulty(header2))
               ? block2
               : (blockHeaderGetTimestamp(header1) <= blockHeaderGetTimestamp(header2)
                  ? block1
                  : block2)));
}

/**
 * Chain all possible orphans.  We'll look through `bcs-orphans` for any orphan with a parent hash
 * pointing to `bcs->chain`. If we find one, we'll extend the chain and then look again.  Result
 * will be `bcs->chain` being extended with N orpans (0 <= N).
 *
 * TODO: It is possible to have two orphans with the same parent.  Deal with it.
 * We'll select between two orphans sharing a parent
 */
static void
bcsChainOrphans (BREthereumBCS bcs) {
    int keepLooking = 1;
    while (keepLooking) {
        keepLooking = 0;

        BREthereumBlock block = NULL;

        // Select the preferred block to chain by looking through all orphans for ...
        FOR_SET(BREthereumBlock, orphan, bcs->orphans) {
            // ... an orphan with a parent hash that matches `bcs->chain`.
            if (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(blockGetHash (bcs->chain),
                                                   blockHeaderGetParentHash(blockGetHeader(orphan)))))
                block = bcsSelectPreferredBlock(bcs, block, orphan);
        }

        // If we found a block, then ...
        if (NULL != block)
        {
            // ... extend the chain, and ...
            bcsExtendChain(bcs, block, "Chained (Orphan)");

            // ... remove as an orphan, and ...
            BRSetRemove(bcs->orphans, block);

            // ... keep looking for another orphan.
            keepLooking = 1;
        }
    }
}


/**
 * Make `block` an orphan by adding `block` to `bcs->orphans` and by clearing `block->next`.  By
 * making `block` an orphan we might have made blocks pointing to `block` orphans too - we'll
 * have to deal with that later (we don't know what points to `block` in this function's context.)
 *
 * @return The value of `block->next`.
 */
static BREthereumBlock
bcsMakeOrphan (BREthereumBCS bcs,
               BREthereumBlock block) {
    BRSetAdd (bcs->orphans, block);
    eth_log ("BCS", "Block %llu Newly Orphaned", blockGetNumber(block));

    // With `block` as an orphan we might have orphaned some transactions or logs.  We'll
    // deal with that later.

    return blockClrNext(block);
}

/**
 * Chain all possible orphans, which may extend the chain, and then purge any orphans that are
 * now too old to be chained.
 */
static void
bcsChainThenPurgeOrphans (BREthereumBCS bcs) {
    bcsChainOrphans(bcs);
    bcsPurgeOrphans(bcs,  blockGetNumber(bcs->chain));
}

/**
 * Pend transactions and logs if they are `included` but reference an orphaned block.  Once
 * pending we'll periodically reexamine them to see of they are now chained.
 *
 * TODO: Shouldn't we announce this 'transition' (INCLUDED->PENDIN)?
 * Is it a real transition?  Transaction/Log status doesn't change; corresponding transfer should?
 */
static void
bcsPendOrphanedTransactionsAndLogs (BREthereumBCS bcs) {
    BREthereumTransactionStatus status;
    BREthereumHash blockHash;

    // Examine transactions to see if any are now orphaned; is so, make them PENDING
    FOR_SET(BREthereumTransaction, tx, bcs->transactions) {
        status = transactionGetStatus(tx);
        if (transactionStatusExtractIncluded(&status, NULL, &blockHash, NULL, NULL) &&
            NULL != BRSetGet (bcs->orphans, &blockHash)) {
            array_add (bcs->pendingTransactions, transactionGetHash(tx));
        }
    }

    // Examine logs to see if any are now orphaned.  Logs are seen if and only if they are
    // in a block; see if that block is now an orphan and if so make the log pending.
    FOR_SET(BREthereumLog, log, bcs->logs) {
        status = logGetStatus(log);
        if (transactionStatusExtractIncluded(&status, NULL, &blockHash, NULL, NULL) &&
            NULL != BRSetGet (bcs->orphans, &blockHash)) {
            array_add(bcs->pendingLogs, logGetHash(log));
        }
    }


}

#if defined UNUSED
/*!
 * Check if `blockHash` and `blockNumber` are in the chain.  They will be in the chain if:
 *   a) blockNumber is smaller than the chain's earliest maintained block number, or
 *   b1) blockNumber is not larger than the chain's latest maintained block number and
 *   b2) blockHash is not an orphan and
 *   b4) blockHash is known.
 */
static int
bcsChainHasBlock (BREthereumBCS bcs,
                  BREthereumHash blockHash,
                  uint64_t blockNumber) {
    return (blockNumber < blockGetNumber(bcs->chainTail) ||
            (blockNumber <= blockGetNumber(bcs->chain) &&
             NULL == BRSetGet(bcs->orphans, &blockHash) &&
             NULL != BRSetGet(bcs->blocks, &blockHash)));
}
#endif // UNUSED

/**
 * Check if `block` is in `bcs->chain` (which include `block` being so old as to have a block
 * number before `bcs->chainTail`.
 */
static int
bcsHasBlockInChain (BREthereumBCS bcs,
                   BREthereumBlock block) {
    return (ETHEREUM_BOOLEAN_IS_TRUE (blockHasNext(block)) ||
            blockGetNumber(block) <= blockGetNumber(bcs->chainTail));
}

/**
 * Extends `bcs->transactions` and `bcs->logs` with the tranactions and logs within `block`.
 * Requires `block` to be in 'complete' and in `bcs->chain`.
 */
static void
bcsExtendTransactionsAndLogsForBlock (BREthereumBCS bcs,
                                           BREthereumBlock block) {
    assert (bcsHasBlockInChain(bcs, block) && ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusComplete(block)));

    // `block` is chained and complete, we can process its status transactions and logs.
    BREthereumBlockStatus blockStatus = blockGetStatus(block);

    // Process each transaction...
    if (NULL != blockStatus.transactions)
        for (size_t ti = 0; ti < array_count(blockStatus.transactions); ti++)
            bcsHandleTransaction(bcs, blockStatus.transactions[ti]);

    // ... then process each log...
    if (NULL != blockStatus.logs)
        for (size_t li = 0; li < array_count(blockStatus.logs); li++)
            bcsHandleLog (bcs, blockStatus.logs[li]);

//    if (purgeOrphans) bcsPendOrphanedTransactionsAndLogs(bcs);

#if 0
    // Get the status explicitly; apparently this is the *only* way to get the gasUsed.
    lesGetTransactionStatusOne(bcs->les,
                               (BREthereumLESTransactionStatusContext) bcs,
                               (BREthereumLESTransactionStatusCallback) bcsSignalTransactionStatus,
                               transactionGetHash(tx));
#endif
}

/**
 * Extend transactions and logs for block if and only if block is 'complete' and in bcs->chain.
 */
static void
bcsExtendTransactionsAndLogsForBlockIfAppropriate (BREthereumBCS bcs,
                                           BREthereumBlock block) {
    if (bcsHasBlockInChain(bcs, block) && ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusComplete(block)))
        bcsExtendTransactionsAndLogsForBlock (bcs, block);
}

static void
bcsExtendChainIfPossible (BREthereumBCS bcs,
                          BREthereumBlock block,
                          int isFromSync) {
    // THIS WILL BE THE FIRST TIME WE'VE SEEN BLOCK.  EVEN IF COMPLETE, NONE OF ITS LOGS NOR
    // TRANSACTIONS ARE HELD.
    //
    // TODO: Handle case where block is well in the past, like from a sync.

    // Lookup `headerParent`
    BREthereumHash blockParentHash = blockHeaderGetParentHash(blockGetHeader(block));
    BREthereumBlock blockParent = BRSetGet(bcs->blocks, &blockParentHash);

    // If we have a parent, but `header` is inconsitent with its parent, then ignore `header`
    if (NULL != blockParent &&
        ETHEREUM_BOOLEAN_IS_FALSE(blockHeaderIsConsistent(blockGetHeader(block),
                                                          blockGetHeader(blockParent),
                                                          blockGetOmmersCount(blockParent),
                                                          blockGetHeader(bcs->genesis)))) {

        eth_log("BCS", "Block %llu Inconsistent", blockGetNumber(block));
        // TODO: Can we release `block`?
        return;
    }

    // Put `header` in the `chain` - HANDLE 3 CASES:

    // 1) If we do not have any chain, then adopt `block` directly, no questions asked.  This will
    // be used for SYNC_MODE_PRIME_WITH_ENDPOINT where we get all interesting transactions, logs,
    // etc from the ENDPOINT and just want to process new blocks as they are announced;
    if (NULL == bcs->chain) {
        assert (NULL == bcs->chainTail);
        bcsExtendChain(bcs, block, "Chained");
        bcs->chainTail = block;
    }

    // TODO: What about a non-linear sync?  Check if blockNumber < (CurrentBlockNumber - 10)?

    // 2) If there is no `block` parent or if  `block` parent is an orphan, then ...
    else if (NULL == blockParent || NULL != BRSetGet(bcs->orphans, blockParent)) {
        // ... if not a sync, then `block` is an orphan too.
        if (!isFromSync) {
            // Add it to the set of orphans and RETURN (non-local exit).
            bcsMakeOrphan(bcs, block);

            // If `block` is an orphan, then it's parent is not in bcs->chain.  That could be
            // because there is just a fork developing or that we've fallen behind.  Attempt a
            // sync to recover (might not actually perform a sync - just attempt).
            uint64_t orphanBlockNumberMinumum = bcsGetOrphanBlockNumberMinimum(bcs);
            if (UINT64_MAX != orphanBlockNumberMinumum)
                bcsSyncContinue(bcs->sync,
                                blockGetNumber(bcs->chain),
                                orphanBlockNumberMinumum);

            return;
        }

        // ... otherwise, if in a sync, then adopt block
        else if (blockGetNumber(block) > blockGetNumber(bcs->chain)) {
            BREthereumBlock oldChainHead = bcs->chain;
            BREthereumBlock oldChainTail = bcs->chainTail;

            bcs->chain = bcs->chainTail = block;
            blockClrNext(block);
            eth_log("BCS", "Block %llu Chained (Sync)", blockGetNumber(block));

            // clean up `oldChain`
        }
    }

    // 3) othewise, we have a new `block` that links to a parent that is somewhere in the
    // chain.  All headers from chain back to parent are now orphans.  In practice, there might
    // be only one (or two or three) orphans.
    //
    // Can we assert that `headerParent` is in `chain` if it is not an orphan?
    //
    // TODO: We need to be selective on adopting `block` - maybe those already in place are better?
    //
    else {
        // Every header between `chain` and `blockParent` is now an orphan.  Usually `chain` is
        // the `blockParent` and thus there is nothing to orphan.
        while (NULL != bcs->chain && blockParent != bcs->chain) {
            // Make an orphan from an existing chain element
            bcs->chain = bcsMakeOrphan(bcs, bcs->chain);
        }
        // TODO: Handle bcs->chainTail

        // Must be there; right?
        assert (NULL != bcs->chain);

        // Extend the chain
        bcsExtendChain(bcs, block, "Chained");
    }

    // Having extended the chain, see if we can chain some orphans.
    bcsChainThenPurgeOrphans (bcs);

    // We have now extended the chain from blockParent, with possibly multiple blocks, to
    // bcs->chain.  We also have updated bcs->orphans will all orphans (but we don't know which of
    // the orphans are newly orphaned).
    //
    // It is now time to extend `transactions` and `logs` based on the extended chain and orphans
    BCS_FOR_CHAIN(bcs, block) {
        if (block == blockParent) break; // done
        if (ETHEREUM_BOOLEAN_IS_FALSE(blockHasStatusComplete(block))) continue;

        // Block is 'complete' - can find transaction and logs of interest.
        bcsExtendTransactionsAndLogsForBlock(bcs, block);
    }

    // And finally purge any transactions and logs for orphaned blocks
    bcsPendOrphanedTransactionsAndLogs(bcs);

    // Periodically reclaim 'excessive' blocks and save the latest.
    bcsReclaimAndSaveBlocksIfAppropriate (bcs);
}

///
/// MARK: - Block Header
///

static BREthereumBoolean
bcsBlockHasMatchingTransactions (BREthereumBCS bcs,
                                 BREthereumBlock block) {
    // TODO: Massive Fix Required ('massive' in importance, not size)
    //    return AS_ETHEREUM_BOOLEAN(blockGetNumber(block) < 5795675);
    return ETHEREUM_BOOLEAN_TRUE;
    // return ETHEREUM_BOOLEAN_FALSE;
}

static BREthereumBoolean
bcsBlockNeedsAccountState (BREthereumBCS bcs,
                           BREthereumBlock block) {
    return ETHEREUM_BOOLEAN_FALSE;
}

static BREthereumBoolean
bcsBlockHasMatchingLogs (BREthereumBCS bcs,
                         BREthereumBlock block) {
    return blockHeaderMatch(blockGetHeader(block), bcs->filterForAddressOnLogs);
}

/**
 * Handle a (generally new) block header.
 */
static void
bcsHandleBlockHeaderInternal (BREthereumBCS bcs,
                              BREthereumBlockHeader header,
                              int isFromSync) {

    // Ignore the header if we have seen it before.  Given an identical hash, *nothing*, at any
    // level (transactions, receipts, logs), could have changed and thus no processing is needed.
    BREthereumHash blockHash = blockHeaderGetHash(header);
    if (NULL != BRSetGet(bcs->blocks, &blockHash)) {
        eth_log("BCS", "Block %llu Ignored", blockHeaderGetNumber(header));
        blockHeaderRelease(header);
        return;
    }

    // Ignore the header if it is not valid.
    if (ETHEREUM_BOOLEAN_IS_FALSE(blockHeaderIsValid (header))) {
        eth_log("BCS", "Block %llu Invalid", blockHeaderGetNumber(header));
        blockHeaderRelease(header);
        return;
    }

    // ?? Other checks ??

    // We have a header that appears consistent.  Create a block and work to fill it out.
    BREthereumBlock block = blockCreate(header);
    BRSetAdd(bcs->blocks, block);

    // Check if we need 'transaction receipts', 'block bodies' or 'account state'.  We'll use the
    // header's logsBloom for the recipts check; we've got nothing in the header to check for
    // bodies nor for account state.  We'll get block bodies by default and avoid account state
    // (getting account state might allow us to avoid getting block bodies; however, the client
    // cost to get the account state is ~2.5 times more then getting block bodies so we'll just
    // get block bodies)
    BREthereumBoolean needReceipts = bcsBlockHasMatchingLogs(bcs, block);
    BREthereumBoolean needBodies   = bcsBlockHasMatchingTransactions(bcs, block);
    BREthereumBoolean needAccount  = bcsBlockNeedsAccountState(bcs, block);

    // Request block bodied, if needed.
    if (ETHEREUM_BOOLEAN_IS_TRUE(needBodies)) {
        blockReportStatusTransactionsRequest(block, BLOCK_REQEUST_PENDING);
        lesGetBlockBodiesOne(bcs->les,
                             (BREthereumLESBlockBodiesContext) bcs,
                             (BREthereumLESBlockBodiesCallback) bcsSignalBlockBodies,
                             blockGetHash(block));
        eth_log("BCS", "Block %llu Needs Bodies", blockGetNumber(block));
    }

    // Request transaction receipts, if needed.
    if (ETHEREUM_BOOLEAN_IS_TRUE(needReceipts)) {
        blockReportStatusLogsRequest(block, BLOCK_REQEUST_PENDING);
        lesGetReceiptsOne(bcs->les,
                          (BREthereumLESReceiptsContext) bcs,
                          (BREthereumLESReceiptsCallback) bcsSignalTransactionReceipts,
                          blockGetHash(block));
        eth_log("BCS", "Block %llu Needs Receipts", blockGetNumber(block));
    }

    // Request account state, if needed.
    if (ETHEREUM_BOOLEAN_IS_TRUE(needAccount)) {
        blockReportStatusAccountStateRequest (block, BLOCK_REQEUST_PENDING);
        lesGetAccountState(bcs->les,
                           (BREthereumLESAccountStateContext) bcs,
                           (BREthereumLESAccountStateCallback) bcsSignalAccountState,
                           blockGetNumber(block),
                           blockGetHash(block),
                           bcs->address);
        eth_log("BCS", "Block %llu Needs AccountState", blockGetNumber(block));
    }

    // Chain 'block' - we'll do this before the block is fully constituted.  Once constituted,
    // we'll handle the block's interesting transactions and logs; including if the block is an
    // orphan.  Note: the `bcsExtendChainIfAppropriate()` function will handle a fully constituted
    // block; however, as the above suggests, the block might be complete but likely empty.
    //
    // TODO: What is the header is well into the past - like during a sync?
    bcsExtendChainIfPossible(bcs, block, isFromSync);
}

extern void
bcsHandleBlockHeader (BREthereumBCS bcs,
                      BREthereumBlockHeader header) {
    bcsHandleBlockHeaderInternal(bcs, header, 0);
}

///
/// MARK: - Account State
///

extern void
bcsHandleAccountState (BREthereumBCS bcs,
                       BREthereumLESAccountStateResult result) {
    // Ensure we have a Block
    BREthereumBlock block = BRSetGet(bcs->blocks, &result.u.success.block);
    if (NULL == block) {
        eth_log ("BCS", "Block %llu Missed (Account)", (NULL == block ? -1 : blockGetNumber(block)));
        return;
    }

    // If the status has somehow errored, skip out with nothing more to do.
    if (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusError(block))) {
        eth_log ("BCS", "Block %llu In Error (Account)", blockGetNumber(block));
        return;
    }

    // We must be in a 'account needed' status
    assert (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusAccountStateRequest(block, BLOCK_REQEUST_PENDING)));

    eth_log("BCS", "Account %llu Nonce %llu, Balance XX",
            blockGetNumber(block),
            accountStateGetNonce(result.u.success.accountState));

    // Report the block status - we'll flag as HAS_ACCOUNT_STATE.
    blockReportStatusAccountState(block, result.u.success.accountState);

//    // TODO: How to update the overall, bcs account state?
//    // If block is bcs->chain then block is the latest and we should update the bcs state;
//    // however, most often, at least during a sync, the chain will have moved on by the time
//    // this account state result is available - particularly during a sync
//    //
//    // Well, eventually, we'll catch up, so ...
//    if (block == bcs->chain) {
//        bcs->accountState = state;
//        bcs->listener.accountStateCallback (bcs->listener.context,
//                                            bcs->accountState);
//    }

    bcsExtendTransactionsAndLogsForBlockIfAppropriate (bcs, block);
}

///
/// MARK: - Block Bodies
///

/*!
 */
static void
bcsReleaseOmmersAndTransactionsFully (BREthereumBCS bcs,
                                      BREthereumTransaction *transactions,
                                      BREthereumBlockHeader *ommers) {
    for (size_t index = 0; index < array_count(transactions); index++)
        transactionRelease(transactions[index]);
    array_free(transactions);

    for (size_t index = 0; index < array_count(ommers); index++)
        blockHeaderRelease(ommers[index]);
    array_free(ommers);
}

extern void
bcsHandleBlockBodies (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transactions[],
                      BREthereumBlockHeader ommers[]) {
    // Ensure we have a Block
    BREthereumBlock block = BRSetGet(bcs->blocks, &blockHash);
    if (NULL == block) {
        eth_log ("BCS", "Block %llu Missed (Bodies)", (NULL == block ? -1 : blockGetNumber(block)));
        bcsReleaseOmmersAndTransactionsFully(bcs, transactions, ommers);
        return;
    }

    // Get the block status and begin filling it out.
//    BREthereumBlockStatus activeStatus = blockGetStatus(block);

    // If the status has some how in error, skip out with nothing more to do.
    if (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusError(block))) {
        eth_log ("BCS", "Block %llu In Error (Bodies)", blockGetNumber(block));
        bcsReleaseOmmersAndTransactionsFully(bcs, transactions, ommers);
        return;
    }

    // We must be in a 'bodies needed' status
    assert (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusTransactionsRequest(block, BLOCK_REQEUST_PENDING)));

    eth_log("BCS", "Bodies %llu Count %lu",
            blockGetNumber(block),
            array_count(transactions));

    // Update `block` with the reported ommers and transactions.  Note that generally
    // these are not used.... but we take full ownership of the memory for ommers and transactions.
    blockUpdateBody(block, ommers, transactions);

    // Having filled out `block`, ensure that it is valid.  If invalid, change the status
    // to 'error' and skip out.  Don't release anything as other handlers (receipts, account) may
    // still be pending.
    if (ETHEREUM_BOOLEAN_IS_FALSE(blockIsValid(block, ETHEREUM_BOOLEAN_TRUE))) {
        blockReportStatusError(block, ETHEREUM_BOOLEAN_TRUE);
        eth_log ("BCS", "Block %llu Invalid (Bodies)", blockGetNumber(block));
        return;
    }

    // Find transactions of interest.
    BREthereumTransaction *neededTransactions = NULL;

    // Check the transactions one-by-one.
    for (int i = 0; i < array_count(transactions); i++) {
        BREthereumTransaction tx = transactions[i];
        assert (NULL != tx);
        
        // If it is our transaction (as source or target), handle it.
        if (ETHEREUM_BOOLEAN_IS_TRUE(transactionHasAddress(tx, bcs->address))) {
            eth_log("BCS", "Bodies %llu Found Transaction at %d",
                    blockGetNumber(block), i);

            // We'll need a copy of the transation as this transaction will be added to the
            // transaction status - which must be distinct from the block transactions.
            tx = transactionCopy(tx);

            // Fill-out the status.  Note that gasUsed is zero.  When this block is chained
            // we'll request the TxStatus so we can get a valid gasUsed value.
            transactionSetStatus(tx, transactionStatusCreateIncluded (gasCreate(0),
                                                                      blockGetHash(block),
                                                                      blockGetNumber(block),
                                                                      i));

            if (NULL == neededTransactions) array_new (neededTransactions, 3);
            array_add(neededTransactions, tx);
        }
        // else - TODO: Handle if has a 'contract' address of interest?
    }

    // Report the block status.  Do so even if neededTransaction is NULL.
    blockReportStatusTransactions(block, neededTransactions);

    if (NULL != neededTransactions) {
        // TODO: Something interesting... Not sure what... AccountState {balance, nonce}
    }

    // If we requested Receipts (for Logs) and have them, then we can process the Logs.
    if (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusLogsRequest(block, BLOCK_REQUEST_COMPLETE)))
        blockLinkLogsWithTransactions (block);

    // In the following, 'if appropriate' means complete and chained.
    bcsExtendTransactionsAndLogsForBlockIfAppropriate (bcs, block);
}


///
/// MARK: - Transaction Receipts
///


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
static void
bcsReleaseReceiptsFully (BREthereumBCS bcs,
                         BREthereumTransactionReceipt *receipts) {
    for (size_t index = 0; index < array_count(receipts); index++)
        transactionReceiptRelease(receipts[index]);
    array_free (receipts);
}

extern void
bcsHandleTransactionReceipts (BREthereumBCS bcs,
                              BREthereumHash blockHash,
                              BREthereumTransactionReceipt *receipts) {
    // Ensure we have a Block
    BREthereumBlock block = BRSetGet(bcs->blocks, &blockHash);
    if (NULL == block) {
        eth_log ("BCS", "Block %llu Missed (Receipts)", (NULL == block ? -1 : blockGetNumber(block)));
        bcsReleaseReceiptsFully(bcs, receipts);
        return;
    }

    // If the status has some how errored, skip out with nothing more to do.
    if (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusError(block))) {
        eth_log ("BCS", "Block %llu In Error (Receipts)", blockGetNumber(block));
        bcsReleaseReceiptsFully(bcs, receipts);
        return;
    }

    // We must be in a 'receipts needed' status
    assert (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusLogsRequest(block, BLOCK_REQEUST_PENDING)));

    eth_log("BCS", "Receipts %llu Count %lu",
            blockGetNumber(block),
            array_count(receipts));

    BREthereumLog *neededLogs = NULL;
    BREthereumHash emptyHash = EMPTY_HASH_INIT;

    // We do not ever necessarily have transactions.
    size_t receiptsCount = array_count(receipts);
    for (size_t ti = 0; ti < receiptsCount; ti++) { // transactionIndex
        BREthereumTransactionReceipt receipt = receipts[ti];
        if (ETHEREUM_BOOLEAN_IS_TRUE (transactionReceiptMatch(receipt, bcs->filterForAddressOnLogs))) {
            size_t logsCount = transactionReceiptGetLogsCount(receipt);
            for (size_t li = 0; li < logsCount; li++) { // logIndex
                BREthereumLog log = transactionReceiptGetLog(receipt, li);

                // If `log` topics match our address....
                if (ETHEREUM_BOOLEAN_IS_TRUE (logMatchesAddress(log, bcs->address, ETHEREUM_BOOLEAN_TRUE))) {
                    eth_log("BCS", "Receipts %llu Found Log at (%lu, %lu)",
                            blockGetNumber(block), ti, li);

                    // We'll need a copy of the log as this log will be added to the
                    // transaction status - which must be distinct from the receipts.
                    log = logCopy(log);

                    // We must save `li`, it identifies this log amoung other logs in transaction.
                    // We won't have the transaction hash so we'll use an empty one.
                    logInitializeIdentifier(log, emptyHash, li);

                    logSetStatus(log, transactionStatusCreateIncluded(gasCreate(0),
                                                                      blockGetHash(block),
                                                                      blockGetNumber(block),
                                                                      ti));

                    if (NULL == neededLogs) array_new(neededLogs, 3);
                    array_add(neededLogs, log);
                }

                // else are we intereted in contract matches?  To 'estimate Gas'?  If so, check
                // logic elsewhere to avoid excluding logs.
            }
        }
    }
    bcsReleaseReceiptsFully(bcs, receipts);

    // Report the block status - we'll flat as HAS_LOGS (even if none of interest).
    blockReportStatusLogs(block, neededLogs);

    // If we have any logs, then we'll need transactions (block bodies).  We might have them
    // already - if so, use them; if not, request them.
    if (NULL != neededLogs) {
        if (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusTransactionsRequest(block, BLOCK_REQUEST_COMPLETE)))
            blockLinkLogsWithTransactions (block);
        else if (ETHEREUM_BOOLEAN_IS_TRUE (blockHasStatusTransactionsRequest(block, BLOCK_REQUEST_NOT_NEEDED))) {
            blockReportStatusTransactionsRequest(block, BLOCK_REQEUST_PENDING);
            lesGetBlockBodiesOne(bcs->les,
                                 (BREthereumLESBlockBodiesContext) bcs,
                                 (BREthereumLESBlockBodiesCallback) bcsSignalBlockBodies,
                                 blockGetHash(block));
            eth_log("BCS", "Block %llu Needs Bodies (for Logs)", blockGetNumber(block));
        }
    }

    // In the following, 'if appropriate' means complete and chained.
    bcsExtendTransactionsAndLogsForBlockIfAppropriate (bcs, block);
}

///
/// MARK: - Transaction Status
///

static int
bcsLookupPendingTransaction (BREthereumBCS bcs,
                             BREthereumHash hash) {
    for (int i = 0; i < array_count(bcs->pendingTransactions); i++)
        if (ETHEREUM_BOOLEAN_IS_TRUE (hashEqual(bcs->pendingTransactions[i], hash)))
            return i;
    return -1;
}

//
// We only obsserve transaction status for two cases:
//
//    a) transactions that we've submitted/originated
//    b) transactions that have been (newly) chained.
//
// In both cases, the transaction *must* be in the BRSet of bcs->transactions.
//
// In case 'a' the transaction can be in any state, PENDING, UKNONWN, etc and would generally
// be progressing to one of the final states of INCLUDED or ERRORRED.  We'll keep requesting
// the status (leave the hash in `bcs->pendingTransactions` list) unless the state is ERORRED.
// (If the new state is INCLUDED, we'll fall back to 'a' in a subsequent handler call.
//
// In case 'b' the transaction is INCLUDED in the chain but the BlockBodies tranaction data
// does not include `gasUsed`.  We want the 'gasUsed' value.
//
extern void
bcsHandleTransactionStatus (BREthereumBCS bcs,
                            BREthereumHash transactionHash,
                            BREthereumTransactionStatus status) {
    BREthereumTransaction transaction = BRSetGet(bcs->transactions, &transactionHash);
    if (NULL == transaction) return;

    int needSignal = 0;
    int pendingIndex = bcsLookupPendingTransaction(bcs, transactionHash);

    // Get the current (aka 'old') status.
    BREthereumTransactionStatus oldStatus = transactionGetStatus(transaction);
    BREthereumHash oldStatusBlockHash = EMPTY_HASH_INIT;

    switch (oldStatus.type) {
        case TRANSACTION_STATUS_INCLUDED:
            // Case 'b': the transaction is already in the chain.  We are only interested in
            // updating the gasUsed status.  We can only update gasUsed if `status` is also
            // included and the block hashes match.
            if (TRANSACTION_STATUS_INCLUDED == status.type) {
                BREthereumHash newStatusBlockHash;
                BREthereumGas newStatusGasUsed;
                transactionStatusExtractIncluded(&oldStatus, NULL, &oldStatusBlockHash, NULL, NULL);
                transactionStatusExtractIncluded(&status, &newStatusGasUsed, &newStatusBlockHash, NULL, NULL);

                if (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(oldStatusBlockHash, newStatusBlockHash))) {
                    oldStatus.u.included.gasUsed = newStatusGasUsed;
                    transactionSetStatus(transaction, oldStatus);
                    if (-1 != pendingIndex) array_rm(bcs->pendingTransactions, pendingIndex);
                    needSignal = 1;
                    pendingIndex = -1;
                }
            }
            break;

        case TRANSACTION_STATUS_ERRORED:
            // We should never be here... just remove the transaction from pending
            if (-1 != pendingIndex) array_rm(bcs->pendingTransactions, pendingIndex);
            pendingIndex = -1;
            break;

        default:
            // Case 'a': the transaction is in any other state.  We'll keep looking unless
            // the new state is ERRORRED.  The transaction might leave this state if the
            // transaction is included in another block - which will be handled above.
            if (TRANSACTION_STATUS_ERRORED == status.type) {
                transactionSetStatus(transaction, status);
                if (-1 != pendingIndex) array_rm(bcs->pendingTransactions, pendingIndex);
                needSignal = 1;
                pendingIndex = -1;
            }
            break;
    }

    if (needSignal) bcsSignalTransaction(bcs, transaction);

    BREthereumHashString hashString;
    hashFillString(transactionHash, hashString);

    eth_log("BCS", "Transaction: \"%s\", Status: %d, Pending: %d%s%s",
            hashString,
            status.type,
            -1 != pendingIndex,
            (TRANSACTION_STATUS_ERRORED == status.type ? ", Error: " : ""),
            (TRANSACTION_STATUS_ERRORED == status.type ? status.u.errored.reason : ""));
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

    // TODO: Avoid-ish a race condition on bcsRelease. This is the wrong approach.
    if (NULL == bcs->les) return;

    lesGetTransactionStatus (bcs->les,
                             (BREthereumLESTransactionStatusContext) bcs,
                             (BREthereumLESTransactionStatusCallback) bcsSignalTransactionStatus,
                             bcs->pendingTransactions);
}

///
/// MARK - Handle Transaction, Log, Peers and Account(?)
///

extern void
bcsHandleTransaction (BREthereumBCS bcs,
                      BREthereumTransaction transaction) {
    int needUpdated = 1;

    // See if we have an existing transaction...
    BREthereumTransaction oldTransaction = BRSetGet(bcs->transactions, transaction);

    // ... if we don't, then add it and announce it as `ADDED`
    if (NULL == oldTransaction) {
        BRSetAdd(bcs->transactions, transactionCopy(transaction));
        bcs->listener.transactionCallback (bcs->listener.context,
                                           BCS_CALLBACK_TRANSACTION_ADDED,
                                           transaction);
        needUpdated = 0;
    }
    // otherwise, it is already known and we'll update it's status
    else {
        transactionSetStatus(oldTransaction, transactionGetStatus(transaction));
    }

    BREthereumTransactionStatus status = transactionGetStatus(transaction);

    switch (status.type) {
        case TRANSACTION_STATUS_ERRORED:
            break;

        case TRANSACTION_STATUS_INCLUDED:
            if (BRSetContains(bcs->orphans, &status.u.included.blockHash)) {
                BRSetRemove(bcs->transactions, transaction);
                bcs->listener.transactionCallback (bcs->listener.context,
                                                   BCS_CALLBACK_TRANSACTION_DELETED,
                                                   transaction);
                transactionRelease(transaction);
                needUpdated = 0;
            }
            break;

        default:
            array_add (bcs->pendingTransactions, transactionGetHash(transaction));
            break;
    }

    // Announce as `UPDATED` (unless we announced ADDED)
    if (needUpdated)
        bcs->listener.transactionCallback (bcs->listener.context,
                                           BCS_CALLBACK_TRANSACTION_UPDATED,
                                           transaction);
}

/*!
 */
extern void
bcsHandleLog (BREthereumBCS bcs,
              BREthereumLog log) {
    int needUpdate = 1;

    BREthereumLog oldLog = BRSetGet(bcs->logs, log);

    if (NULL == oldLog) {
        BRSetAdd(bcs->logs, log);
        bcs->listener.logCallback (bcs->listener.context,
                                   BCS_CALLBACK_LOG_ADDED,
                                   log);
        needUpdate = 0;
    }
    else
        logSetStatus(oldLog, logGetStatus(log));

    BREthereumTransactionStatus status = logGetStatus(log);

    // Something based on 'state'
    switch (status.type) {
        case TRANSACTION_STATUS_ERRORED:
            break;

        case TRANSACTION_STATUS_INCLUDED:
            if (BRSetContains(bcs->orphans, &status.u.included.blockHash)) {
                BRSetRemove(bcs->logs, log);
                bcs->listener.logCallback (bcs->listener.context,
                                           BCS_CALLBACK_LOG_DELETED,
                                           log);
                logRelease(log);
                needUpdate = 0;
            }
            break;

        default:
            array_add (bcs->pendingLogs, logGetHash(log));
            break;
    }

    if (needUpdate)
        bcs->listener.logCallback (bcs->listener.context,
                                   BCS_CALLBACK_LOG_UPDATED,
                                   log);
}

extern void
bcsHandlePeers (BREthereumBCS bcs,
                BRArrayOf(BREthereumPeerConfig) peers) {
    size_t peersCount = array_count(peers);
    bcs->listener.savePeersCallback (bcs->listener.context, peers);
    eth_log("BCS", "Peers %zu Saved", peersCount);
}

static void
bcsSyncReportBlocksCallback (BREthereumBCS bcs,
                             BREthereumBCSSync sync,
                             BRArrayOf(BREthereumBCSSyncResult) results) {
    size_t resultsCount = (NULL != results ? array_count(results) : 0);
    for (size_t index = 0; index < resultsCount; index++)
        bcsHandleBlockHeaderInternal (bcs, results[index].header, 1);
}

static void
bcsSyncReportProgressCallback (BREthereumBCS bcs,
                               BREthereumBCSSync sync,
                               uint64_t blockNumberBeg,
                               uint64_t blockNumberNow,
                               uint64_t blockNumberEnd) {

    bcs->listener.syncCallback (bcs->listener.context,
                                (blockNumberNow == blockNumberBeg
                                 ? BCS_CALLBACK_SYNC_STARTED
                                 : (blockNumberNow == blockNumberEnd
                                    ? BCS_CALLBACK_SYNC_STOPPED
                                    : BCS_CALLBACK_SYNC_UPDATE)),
                                blockNumberBeg,
                                blockNumberNow,
                                blockNumberEnd);
}

////
//// Active Block
////
//extern BREthereumBlock
//bcsLookupActiveBlock (BREthereumBCS bcs,
//                      BREthereumHash hash) {
//    for (int index = 0; index < array_count (bcs->activeBlocks); index++)
//        if (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(hash, blockGetHash (bcs->activeBlocks[index]))))
//            return bcs->activeBlocks[index];
//    return NULL;
//}
//
//extern void
//bcsReleaseActiveBlock (BREthereumBCS bcs,
//                       BREthereumHash hash) {
//    for (int index = 0; index < array_count (bcs->activeBlocks); index++)
//        if (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(hash, blockGetHash (bcs->activeBlocks[index])))) {
//            array_rm (bcs->activeBlocks, index);
//            break;
//        }
//}
