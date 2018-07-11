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
#define BCS_ACTIVE_BLOCKS_INITIAL_CAPACITY  (5)

// Any orphan more then AGE_OFFSET blocks in the past will be purged.
#define BCS_ORPHAN_AGE_OFFSET  (10)

#define BCS_SAVE_BLOCKS_COUNT  (500)

/**
 * When syncing, we'll request a batch of headers
 */
#define BCS_SYNC_BLOCKS_COUNT  (150)

/* Forward Declarations */
static void
bcsPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);

static void
bcsSyncFrom (BREthereumBCS bcs,
             uint64_t chainBlockNumber);

static int
bcsLookupPendingTransaction (BREthereumBCS bcs,
                             BREthereumHash hash);

static void
bcsExtendChain (BREthereumBCS bcs,
                BREthereumBlock block,
                const char *message);
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

        BRSetAdd(bcs->transactions, transaction);

        if (TRANSACTION_STATUS_PENDING == status.type)
            array_add (bcs->pendingTransactions, transactionGetHash(transaction));

        bcs->listener.transactionCallback (bcs->listener.context,
                                           BCS_CALLBACK_TRANSACTION_ADDED,
                                           transaction);
    }
}

static void
bcsCreateInitializeLogs (BREthereumBCS bcs,
                         BRArrayOf(BREthereumLog) logs) {
    if (NULL == logs) return;

    for (size_t index = 0; index < array_count(logs); index++) {
        BREthereumLog log = logs[index];
        BREthereumLogStatus status = logGetStatus(log);

        // For now, assume all provided logs are in a 'final state'.
        assert (LOG_STATUS_INCLUDED == status.type ||
                LOG_STATUS_ERRORED  == status.type ||
                LOG_STATUS_PENDING  == status.type);

        BRSetAdd (bcs->logs, log);

        if (LOG_STATUS_PENDING == status.type)
            array_add (bcs->pendingLogs, logGetHash(log));

        bcs->listener.logCallback (bcs->listener.context,
                                   BCS_CALLBACK_LOG_ADDED,
                                   log);
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

    bcs->syncActive = 0;
    bcs->syncHead = bcs->syncNext = bcs->syncTail = 0;

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
    BREthereumBlock genesis = networkGetGenesisBlock(network);
    BRSetAdd(bcs->blocks, genesis);

    //
    // Initialize `activeBlocks`
    array_new (bcs->activeBlocks, BCS_ACTIVE_BLOCKS_INITIAL_CAPACITY);

    // Create but don't start the event handler.  Ensure that a fast-acting lesCreate()
    // can signal events (by queuing; they won't be handled until the event queue is started).
    bcs->handler = eventHandlerCreate(bcsEventTypes, bcsEventTypesCount);
    eventHandlerSetTimeoutDispatcher(bcs->handler,
                                     1000 * BCS_TRANSACTION_CHECK_STATUS_SECONDS,
                                     (BREventDispatcher)bcsPeriodicDispatcher,
                                     (void*) bcs);

    // Initialize `chain` - will be modified based on `headers`
    bcs->chain = bcs->chainTail = genesis;

    bcsCreateInitializeBlocks(bcs, blocks);
    bcsCreateInitializeTransactions(bcs, transactions);
    bcsCreateInitializeLogs(bcs, logs);

    // peers - save for bcsStart; or, better, use lesCreate() here and lesStart() later.
    return bcs;
}

extern void
bcsStart (BREthereumBCS bcs) {
    BREthereumBlockHeader genesis = networkGetGenesisBlockHeader(bcs->network);
    BREthereumBlockHeader header = blockGetHeader(bcs->chain);

    eventHandlerStart(bcs->handler);
    bcs->les = lesCreate(bcs->network,
                         (BREthereumLESAnnounceContext) bcs,
                         (BREthereumLESAnnounceCallback) bcsSignalAnnounce,
                         blockHeaderGetHash(header),
                         blockHeaderGetNumber(header),
                         blockHeaderGetDifficulty(header),
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

    // Active Block
    for (size_t index = 0; index < array_count(bcs->activeBlocks); index++)
        bcsReleaseActiveBlock(bcs, blockGetHash (bcs->activeBlocks[index]));
    array_free(bcs->activeBlocks);
    
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

extern BREthereumLES
bcsGetLES (BREthereumBCS bcs) {
    return bcs->les;
}

extern void
bcsSync (BREthereumBCS bcs,
         uint64_t blockNumber) {
    bcsSyncFrom(bcs, blockNumber);
}

extern BREthereumBoolean
bcsSyncInProgress (BREthereumBCS bcs) {
    return AS_ETHEREUM_BOOLEAN(bcs->syncActive);
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

/**
 * The wording is wrong here - if we are matching a block, then we should look at the transactions.
 */
static BREthereumBoolean
bcsBlockHasMatchingTransactions (BREthereumBCS bcs,
                                 BREthereumBlock block) {
//    return ETHEREUM_BOOLEAN_TRUE;
    return ETHEREUM_BOOLEAN_FALSE;
}

/**
 * The working is wrong here - if we are matching a block, then we should look at the logs.
 */
static BREthereumBoolean
bcsBlockHasMatchingLogs (BREthereumBCS bcs,
                         BREthereumBlock block) {
    return blockHeaderMatch(blockGetHeader(block), bcs->filterForAddressOnLogs);
}

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
    // We'll save everything between bcs->chain and bcs->chainTail
    unsigned long long blockCount = blockGetNumber(bcs->chain) - blockGetNumber(bcs->chainTail) + 1;

    // We'll pass long the blocks directly, for now.  This will likely need to change because
    // this code will lose control of the block memory.
    //
    // For example, we quickly hit another 'ReclaimAndSaveBlocks' point prior the the
    // `saveBlocksCallback` completing - we'll then release memory needed by the callback handler.
    BRArrayOf(BREthereumBlock) blocks;
    array_new(blocks, blockCount);
    array_set_count(blocks, blockCount);

    BREthereumBlock next = bcs->chain;
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
            blockGetNumber(bcs->chain));
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

    // Look through pendingLogs and see if any are now included.
    int keepLooking = 1;
    while (keepLooking) {
        keepLooking = 0;
        for (size_t index = 0; index < array_count(bcs->pendingLogs); index++) {
            BREthereumHash logHash = bcs->pendingLogs[index];
            BREthereumLog log = BRSetGet (bcs->logs, &logHash);
            assert (NULL != log);
            
            // TODO: Are we sure `block` has a filled status?
            if (ETHEREUM_BOOLEAN_IS_TRUE (blockHasStatusLog(block, log))) {
                BREthereumLogStatus status = logGetStatus(log);
                logStatusUpdateIncluded(&status, blockGetHash(block), blockGetNumber(block));
                bcs->listener.logCallback (bcs->listener.context,
                                           BCS_CALLBACK_LOG_UPDATED,
                                           log);
                array_rm (bcs->pendingLogs, index);
                keepLooking = 1;
            }
        }
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
    FOR_SET(BREthereumBlock, orphan, bcs->orphans)
        if (blockGetNumber(orphan) < number)
            number = blockGetNumber(orphan);
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
        FOR_SET(BREthereumBlock, orphan, bcs->orphans)
            if (blockGetNumber(orphan) < blockNumber) {
                BRSetRemove(bcs->orphans, orphan);
                eth_log("BCS", "Block %llu Purged Orphan", blockGetNumber(orphan));
                blockRelease(orphan);
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
        FOR_SET(BREthereumBlock, orphan, bcs->orphans) {
            if (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(blockGetHash (bcs->chain),
                                                   blockHeaderGetParentHash(blockGetHeader(orphan))))) {
                // Extend the chain.
                bcsExtendChain(bcs, orphan, "Chained Orphan");

                // No longer an orphan
                BRSetRemove(bcs->orphans, orphan);

                // With remove, our FOR_SET iteration is now broken, so ...
                // ... skip out (of `for`) but keep looking.
                keepLooking = 1;
                break;
            }
        }
    }
}

static BREthereumBlock
bcsMakeOrphan (BREthereumBCS bcs,
               BREthereumBlock block) {
    BRSetAdd (bcs->orphans, block);
    eth_log ("BCS", "Block %llu Newly Orphaned", blockGetNumber(block));
    // With `header` as an orphan we might have orphaned some transaction or log.  We'll
    // deal with that later as: a) there maybe be others declared orphans, b) once declared
    // some may be purched and c) some may be chained - all of which impact what transactions
    // or logs are declared orphans themselves, or not.

    return blockClrNext(block);
}

static void
bcsChainThenPurgeOrphans (BREthereumBCS bcs) {
    bcsChainOrphans(bcs);
    bcsPurgeOrphans(bcs,  blockGetNumber(bcs->chain));
}

static void
bcsSyncSubmit (BREthereumBCS bcs,
               uint64_t blockStart,
               uint64_t blockCount) {
    // Make the request
    eth_log("BCS", "Block Sync {%llu, %llu}",
            blockStart,
            blockStart + blockCount);

    lesGetBlockHeaders(bcs->les,
                       (BREthereumLESBlockHeadersContext) bcs,
                       (BREthereumLESBlockHeadersCallback) bcsSignalBlockHeader,
                       blockStart,
                       blockCount,
                       0,
                       ETHEREUM_BOOLEAN_FALSE);
}

static void
bcsSyncContinue (BREthereumBCS bcs,
                 uint64_t chainBlockNumber) {
    // Continue a sync if a) we are syncing and b) there is more to sync.
    bcs->syncActive &= (chainBlockNumber < bcs->syncHead);

    // Reqeust the next batch when the prior batch is complete
    if (bcs->syncActive && chainBlockNumber >= bcs->syncNext) {

        // Progress as [Tail, Next, Head]
        bcs->listener.syncCallback (bcs->listener.context,
                                    BCS_CALLBACK_SYNC_UPDATE,
                                    bcs->syncTail,
                                    bcs->syncNext,
                                    bcs->syncHead);

        uint64_t needHeadersCount = bcs->syncHead - (chainBlockNumber + 1);
        if (needHeadersCount > BCS_SYNC_BLOCKS_COUNT)
            needHeadersCount = BCS_SYNC_BLOCKS_COUNT;

        bcs->syncNext += needHeadersCount;
        bcsSyncSubmit(bcs, chainBlockNumber + 1, needHeadersCount);
    }

    // but if the sync is no longer active, then ...
    else if (chainBlockNumber >= bcs->syncHead && 0 != bcs->syncHead) {
        // ... announce that the sync is complete/stopped
        bcs->listener.syncCallback (bcs->listener.context,
                                    BCS_CALLBACK_SYNC_STOPPED,
                                    bcs->syncTail,
                                    bcs->syncHead,
                                    bcs->syncHead);
        // ... and clear the sync state.
        bcs->syncTail = bcs->syncNext = bcs->syncHead = 0;
    }
}

static void
bcsSyncFrom (BREthereumBCS bcs,
             uint64_t chainBlockNumber) {
    // If we are already syncing, then continue until that completes
    if (bcs->syncActive) {
        bcsSyncContinue(bcs, chainBlockNumber);
        return;
    }

    // We'll need to sync if the minimum orphan header is larger then the chain header (by
    // more than just one).

    uint64_t orphanBlockNumberMinumum = bcsGetOrphanBlockNumberMinimum(bcs);
    if (UINT64_MAX != orphanBlockNumberMinumum && orphanBlockNumberMinumum > chainBlockNumber + 1) {
        uint64_t needHeadersCount = orphanBlockNumberMinumum - (chainBlockNumber + 1);
        if (needHeadersCount > BCS_SYNC_BLOCKS_COUNT)
            needHeadersCount = BCS_SYNC_BLOCKS_COUNT;

        bcs->syncTail = chainBlockNumber + 1;
        bcs->syncHead = orphanBlockNumberMinumum;
        bcs->syncNext  = chainBlockNumber + needHeadersCount;
        bcs->syncActive = 1;

        bcs->listener.syncCallback (bcs->listener.context,
                                    BCS_CALLBACK_SYNC_STARTED,
                                    bcs->syncTail,
                                    bcs->syncTail,
                                    bcs->syncHead);

        bcsSyncSubmit(bcs, chainBlockNumber + 1, needHeadersCount);
    }
}

/*!
 */
extern void
bcsHandleBlockHeader (BREthereumBCS bcs,
                      BREthereumBlockHeader header) {

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
        eth_log("BCS", "Block %llu Invald", blockHeaderGetNumber(header));
        blockHeaderRelease(header);
        return;
    }

    // Lookup `headerParent`
    BREthereumHash blockParentHash = blockHeaderGetParentHash(header);
    BREthereumBlock blockParent = BRSetGet(bcs->blocks, &blockParentHash);

    // If we have a parent, but the block numbers are not consistent, then ignore `header`
    if (NULL != blockParent && blockHeaderGetNumber(header) != 1 + blockGetNumber(blockParent)) {
        eth_log("BCS", "Block %llu Inconsistent", blockHeaderGetNumber(header));
        blockHeaderRelease(header);
        return;
    }

    // ?? Other checks ??

    // We have a header that appears consistent.  Create a block and work to chain it.
    BREthereumBlock block = blockCreate(header);
    BRSetAdd(bcs->blocks, block);

    // Put `header` in the `chain` - HANDLE 3 CASES:

    // 1) If we do not have any chain, then adopt `block` directly, no questions asked.  This will
    // be used for SYNC_MODE_PRIME_WITH_ENDPOINT where we get all interesting transactions, logs,
    // etc from the ENDPOINT and just want to process new blocks as they are announced;
    if (NULL == bcs->chain) {
        assert (NULL == bcs->chainTail);
        bcsExtendChain(bcs, block, "Chained");
        bcs->chainTail = block;
    }

    // 2) If there is no `block` parent or if  `block` parent is an orphan, then `block` is
    // an orphan too.  Add it to the set of orphans and RETURN (non-local exit);
    else if (NULL == blockParent || NULL != BRSetGet(bcs->orphans, blockParent)) {
        bcsMakeOrphan(bcs, block);

        // If `block` is an orphan, then it's parent is not in bcs->chain.  That could be
        // because there is just a fork developing or that we've fallen behind.  Attempt a
        // sync to recover (might not actually perform a sync - just attempt).
        bcsSyncFrom(bcs, blockGetNumber(bcs->chain));
        return;
    }

    // 3) othewise, we have a new `block` that links to a parent that is somewhere in the
    // chain.  All headers from chain back to parent are now orphans.  In practice, there might
    // be only one (or two or three) orphans.
    //
    // Can we assert that `headerParent` is in `chain` if it is not an orphan?
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

    // Examine transactions to see if any are now orphaned; is so, make them PENDING
    FOR_SET(BREthereumTransaction, tx, bcs->transactions) {
        BREthereumHash blockHash;
        if (transactionExtractIncluded(tx, NULL, &blockHash, NULL, NULL)) {
            // If the transaction's blockHash is an orphan...
            if (NULL != BRSetGet (bcs->orphans, &blockHash)) {
                // .... then return the transaction to PENDING; we'll start requesting status again.
                bcsHandleTransactionStatus(bcs,
                                           transactionGetHash(tx),
                                           transactionStatusCreate(TRANSACTION_STATUS_PENDING));
            }

            // but if the transaction's blockHash is not an orphan and instead included...
            else if (NULL != BRSetGet (bcs->blocks, &blockHash)) {
                // ... then is there anything to do?   The transaction's `blockHash` cannot
                // reference a block in `headers` that was just now chained from orphans, can it?
                // More likely the tranaction is pending; we'll get that status and see the
                // chain now includes `blockHash` - but that isn't handled here.
                ;
            }
        }
    }

    // Examine logs to see if any are now orphaned.  Logs are seen if and only if they are
    // in a block; see if that block is now an orphan and if so make the log pending.
    FOR_SET(BREthereumLog, log, bcs->logs) {
        BREthereumHash blockHash;
        if (logExtractIncluded(log, &blockHash, NULL) &&
            NULL != BRSetGet (bcs->orphans, &blockHash)) {
                array_add(bcs->pendingLogs, logGetHash(log));
        }
    }

    // We need block bodies and transaction receipts for every matching block between bcs->chain
    // and blockParent - multiple blocks because we added orphans, or might have.  However,
    // because 99% of the time it is but one needed block, we'll use the LES interface to
    // request block bodies one-by-one.
    BCS_FOR_CHAIN(bcs, block) {
        if (block == blockParent) break;
        // If `block` has matching transactions or logs, then we'll get the block body.
        if (ETHEREUM_BOOLEAN_IS_TRUE(bcsBlockHasMatchingTransactions(bcs, block)) ||
            ETHEREUM_BOOLEAN_IS_TRUE(bcsBlockHasMatchingLogs(bcs, block))) {
            eth_log("BCS", "Block %llu Content Needed", blockGetNumber(block));

            // This is now an active block.  Marking `block` as `active` is critical to the
            // asynchronous calls for block bodies and transaction receipts.  Once we've collected
            // everything we need for `block` will go inactive.
            array_add (bcs->activeBlocks, block);

            lesGetBlockBodiesOne(bcs->les,
                                 (BREthereumLESBlockBodiesContext) bcs,
                                 (BREthereumLESBlockBodiesCallback) bcsSignalBlockBodies,
                                 blockGetHash(block));
        }
    }

    // Periodically reclaim 'excessive' blocks and save the latest.
    bcsReclaimAndSaveBlocksIfAppropriate (bcs);

    // If appropriate, continue a in-process sync.
    bcsSyncContinue (bcs, blockGetNumber(bcs->chain));
}

/*!
 */
extern void
bcsHandleAccountState (BREthereumBCS bcs,
                       BREthereumHash blockHash,
                       BREthereumAddress address,
                       BREthereumAccountState state) {
    BREthereumBlock block = bcsLookupActiveBlock(bcs, blockHash);
    if (NULL == block) {
        block = BRSetGet(bcs->blocks, &blockHash);
        eth_log ("BCS", "Active Block %llu Missed (AccountState)", (NULL == block ? -1 : blockGetNumber(block)));
        return;
    }
    assert (NULL != BRSetGet (bcs->blocks, &blockHash));

    BREthereumBlockStatus activeStatus = blockGetStatus(block);
    assert (ETHEREUM_BOOLEAN_IS_FALSE(blockStatusHasFlag(&activeStatus, BLOCK_STATUS_HAS_ACCOUNT_STATE)));

    blockReportStatusAccountState(block, state);

    // TODO: How to update the overall, bcs account state?
    // If block is bcs->chain then block is the latest and we should update the bcs state;
    // however, most often, at least during a sync, the chain will have moved on by the time
    // this account state result is available - particularly during a sync
    //
    // Well, eventually, we'll catch up, so ...
    if (block == bcs->chain) {
        bcs->accountState = state;
        bcs->listener.accountStateCallback (bcs->listener.context,
                                            bcs->accountState);
    }
}

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
    // Ensure we have an active Block
    BREthereumBlock block = bcsLookupActiveBlock(bcs, blockHash);
    if (NULL == block) {
        block = BRSetGet(bcs->blocks, &blockHash);
        eth_log ("BCS", "Active Block %llu Missed (BlockBodies)", (NULL == block ? -1 : blockGetNumber(block)));

        bcsReleaseOmmersAndTransactionsFully(bcs, transactions, ommers);
        return;
    }
    assert (NULL != BRSetGet (bcs->blocks, &blockHash));

    // Start filling in the block status.
    BREthereumBlockStatus activeStatus = blockGetStatus(block);
    assert (ETHEREUM_BOOLEAN_IS_FALSE(blockStatusHasFlag(&activeStatus, BLOCK_STATUS_HAS_TRANSACTIONS)));

    eth_log("BCS", "Bodies %llu Count %lu",
            blockGetNumber(block),
            array_count(transactions));

    // Update `block` with the reported ommers and transactions.  Note that generally
    // these are not used.... but we take full ownership of the memory for ommers and transactions.
    blockUpdateBody(block, ommers, transactions);

    // Having filled out `block`, ensure that it is valid.
    if (ETHEREUM_BOOLEAN_IS_FALSE(blockIsValid(block, ETHEREUM_BOOLEAN_TRUE))) {
        bcsReleaseActiveBlock(bcs, blockHash);

        // TODO: Not actually safe?
        blockRelease(block);
        return;
    }

    // We might optionally GetReceipts first?  Then the Active Block will be fully constituted
    // and we can process it uniformly for both transactions and logs.
    
    // When there is a transaction of interest, we'll want to keep all interresting transactions
    // and get account balance and nonce info.
    BREthereumTransaction *neededTransactions = NULL;

    // Check the transactions one-by-one.
    for (int i = 0; i < array_count(transactions); i++) {
        BREthereumTransaction tx = transactions[i];
        assert (NULL != tx);
        
        // If it is our transaction (as source or target), handle it.
        if (ETHEREUM_BOOLEAN_IS_TRUE(transactionHasAddress(tx, bcs->address))) {
            eth_log("BCS", "Bodies   %llu Found Transaction at (%d)",
                    blockGetNumber(block), i);

            if (NULL == neededTransactions) array_new (neededTransactions, 3);

            // We copy the transaction because `transactions` are owned by `block`.
            tx = transactionCopy(tx);
            array_add(neededTransactions, tx);

            // TODO: THIS COULD BE A DUPLICATE TRANSACTION (already in another block)

            // Save the transaction; this set now owns `tx`.
            BRSetAdd(bcs->transactions, tx);

            // TODO: Extending 'status transactions'

            // Get the status explicitly; apparently this is the *only* way to get the gasUsed.
            lesGetTransactionStatusOne(bcs->les,
                                       (BREthereumLESTransactionStatusContext) bcs,
                                       (BREthereumLESTransactionStatusCallback) bcsSignalTransactionStatus,
                                       transactionGetHash(tx));
        }
        // else - TODO: Handle if has a 'contract' address of interest?
    }

    // Report the block status - we'll flag as HAS_TRANSACTION but their could be none.
    blockReportStatusTransactions(block, neededTransactions);

    if (NULL != neededTransactions && array_count(neededTransactions) > 0) {
        // TODO: Something interesting for AccountState {balance, nonce}
    }

    // If there are logs of interest, get the transactions receipts...
    if (ETHEREUM_BOOLEAN_IS_TRUE(bcsBlockHasMatchingLogs(bcs, block))) {
        lesGetReceiptsOne(bcs->les,
                          (BREthereumLESReceiptsContext) bcs,
                          (BREthereumLESReceiptsCallback) bcsSignalTransactionReceipts,
                          blockHash);
    }
    // ... otherwise report `activeBlock` as having zero logs and then release it.
    else {
        blockReportStatusLogs(block, NULL);
        bcsReleaseActiveBlock(bcs, blockHash);
    }
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
    return AS_ETHEREUM_BOOLEAN(blockNumber < blockGetNumber(bcs->chainTail) ||
                               (blockNumber <= blockGetNumber(bcs->chain) &&
                                NULL == BRSetGet(bcs->orphans, &blockHash) &&
                                NULL != BRSetGet(bcs->blocks, &blockHash)));
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
//            assert (0); // LES *cannot* report these.
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
        bcs->listener.transactionCallback (bcs->listener.context,
                                           BCS_CALLBACK_TRANSACTION_UPDATED,
                                           transaction);
    }
}

//
// MARK: - Transaction Receipts
//

/*
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
 */

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

}
extern void
bcsHandleTransactionReceipts (BREthereumBCS bcs,
                              BREthereumHash blockHash,
                              BREthereumTransactionReceipt *receipts) {
    // Ensure we have an active block
    BREthereumBlock block = bcsLookupActiveBlock(bcs, blockHash);
    if (NULL == block) {
        block = BRSetGet (bcs->blocks, &blockHash);
        eth_log ("BCS", "Active Block %llu Missed (Transaction Receipts)",
                 (NULL == block ? -1 : blockGetNumber(block)));

        bcsReleaseReceiptsFully(bcs, receipts);
        return;
    }
        // assert (ACTIVE_BLOCK_PENDING_RECEIPTS == activeBlock->state);

    eth_log("BCS", "Receipts %llu Count %lu",
            blockGetNumber(block),
            array_count(receipts));

    BREthereumLog *neededLogs = NULL;

    size_t receiptsCount = array_count(receipts);
    for (size_t i = 0; i < receiptsCount; i++) {
        BREthereumTransactionReceipt receipt = receipts[i];
        if (ETHEREUM_BOOLEAN_IS_TRUE (transactionReceiptMatch(receipt, bcs->filterForAddressOnLogs))) {
            BREthereumTransaction transaction = blockGetTransaction(block, i);
            assert (NULL != transaction);

            size_t logsCount = transactionReceiptGetLogsCount(receipt);
            for (size_t index = 0; index < logsCount; index++) {
                BREthereumLog log = transactionReceiptGetLog(receipt, index);

                // If `log` topics match our address....
                if (ETHEREUM_BOOLEAN_IS_TRUE (logMatchesAddress(log, bcs->address, ETHEREUM_BOOLEAN_TRUE))) {
                    eth_log("BCS", "Receipts %llu Found Log at (%lu, %lu)",
                            blockGetNumber(block), i, index);

                    log = logCopy(log);
                    logInitializeStatus (log, transactionGetHash(transaction), index);

                    if (NULL == neededLogs) array_new(neededLogs, 3);

                    // TODO: THIS COULD BE A DUPLICATE LOG (already in another block)

                    BRSetAdd(bcs->logs, log);

                    array_add(neededLogs, log);

                    bcs->listener.logCallback (bcs->listener.context,
                                               BCS_CALLBACK_LOG_UPDATED,
                                               log);
                }

                // else are we intereted in contract matches?  To 'estimate Gas'?  If so, check
                // logic elsewhere to avoid excluding logs.
            }
        }
        transactionReceiptRelease(receipt);
    }
    array_free(receipts);

    blockReportStatusLogs(block, neededLogs);
    bcsReleaseActiveBlock(bcs, blockHash);
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

    bcs->listener.transactionCallback (bcs->listener.context,
                                       BCS_CALLBACK_TRANSACTION_UPDATED,
                                       transaction);

}

/*!
 */
extern void
bcsHandleLog (BREthereumBCS bcs,
              BREthereumHash blockHash,
              BREthereumHash transactionHash, // transaction?
              BREthereumLog log) {

}

extern void
bcsHandlePeers (BREthereumBCS bcs,
                BRArrayOf(BREthereumPeerConfig) peers) {
    size_t peersCount = array_count(peers);
    bcs->listener.savePeersCallback (bcs->listener.context, peers);
    eth_log("BCS", "Peers %zu Saved", peersCount);
}

//
// Active Block
//
extern BREthereumBlock
bcsLookupActiveBlock (BREthereumBCS bcs,
                      BREthereumHash hash) {
    for (int index = 0; index < array_count (bcs->activeBlocks); index++)
        if (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(hash, blockGetHash (bcs->activeBlocks[index]))))
            return bcs->activeBlocks[index];
    return NULL;
}

extern void
bcsReleaseActiveBlock (BREthereumBCS bcs,
                       BREthereumHash hash) {
    for (int index = 0; index < array_count (bcs->activeBlocks); index++)
        if (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(hash, blockGetHash (bcs->activeBlocks[index])))) {
            array_rm (bcs->activeBlocks, index);
            break;
        }
}
