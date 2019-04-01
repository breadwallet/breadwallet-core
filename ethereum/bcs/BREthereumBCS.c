//
//  BREthereumBCS.c
//  Core
//
//  Created by Ed Gamble on 5/24/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <stdarg.h>
#include "support/BRArray.h"
#include "support/BRSet.h"
#include "BREthereumBCSPrivate.h"

#define BCS_TRANSACTION_CHECK_STATUS_SECONDS   (7)

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

// We really can't set this limit; we've seen 15 before.  But, what about a rogue node?
#define BCS_REORG_LIMIT    (10)

#undef BCS_SHOW_ORPHANS

#undef BCS_REPORT_IGNORED_ANNOUNCE

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
static inline uint64_t maximum (uint64_t a, uint64_t b) { return a > b ? a : b; }
#pragma clang diagnostic pop

/* Forward Declarations */
static void
bcsPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);

static void
bcsExtendChain (BREthereumBCS bcs,
                BREthereumBlock block,
                const char *message);

static void
bcsUnwindChain (BREthereumBCS bcs,
                uint64_t depth,
                uint64_t headNumber);

static void
bcsSyncReportBlocksCallback (BREthereumBCS bcs,
                             BREthereumBCSSync sync,
                             BREthereumNodeReference node,
                             BRArrayOf(BREthereumBCSSyncResult) blocks);

static void
bcsSyncReportProgressCallback (BREthereumBCS bcs,
                               BREthereumBCSSync sync,
                               BREthereumNodeReference node,
                               uint64_t blockNumberBeg,
                               uint64_t blockNumberNow,
                               uint64_t blockNumberEnd);

static inline BREthereumSyncInterestSet
syncInterestsCreate (int count, /* BREthereumSyncInterest*/ ...) {
    BREthereumSyncInterestSet interests = 0;;

    va_list args;
    va_start (args, count);
    for (int i = 0; i < count; i++)
        interests |= va_arg (args, BREthereumSyncInterest);
    va_end(args);

    return interests;
}


/**
 */
static void
bcsCreateInitializeBlocks (BREthereumBCS bcs,
                           OwnershipGiven BRSetOf(BREthereumBlock) blocks) {
    if (NULL == blocks) return;
    else if (0 == BRSetCount(blocks)) { BRSetFree(blocks); return; }

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
    size_t sortedBlocksCount = BRSetCount(blocks);
    BREthereumBlock sortedBlocks[sortedBlocksCount];
    BRSetAll(blocks, (void**) sortedBlocks, sortedBlocksCount);

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

    BRSetFree (blocks);
}

static void
bcsCreateInitializeTransactions (BREthereumBCS bcs,
                                 BRSetOf(BREthereumTransaction) transactions) {
    if (NULL == transactions) return;
    else if (0 == BRSetCount(transactions)) { BRSetFree (transactions); return; }

    FOR_SET (BREthereumTransaction, transaction, transactions) {
        BREthereumTransactionStatus status = transactionGetStatus(transaction);

        // For now, assume all provided transactions are not in CREATED - because there
        // won't be a HASH until 'signed'
        assert (TRANSFER_STATUS_CREATED != status.type);

        bcsSignalTransaction (bcs, transaction);
    }

    BRSetFree (transactions);
}

static void
bcsCreateInitializeLogs (BREthereumBCS bcs,
                         BRSetOf(BREthereumLog) logs) {
    if (NULL == logs) return;
    else if (0 == BRSetCount(logs)) { BRSetFree (logs); return; }

    FOR_SET (BREthereumLog, log, logs) {
        BREthereumTransactionStatus status = logGetStatus(log);

        // For now, assume all provided logs are not in CREATED - because there
        // won't be a HASH until 'signed'
        assert (TRANSFER_STATUS_CREATED != status.type);

        bcsSignalLog (bcs, log);
    }

    BRSetFree (logs);
}

extern BREthereumBCS
bcsCreate (BREthereumNetwork network,
           BREthereumAddress address,
           BREthereumBCSListener listener,
           BREthereumMode mode,
           OwnershipGiven BRSetOf(BREthereumNodeConfig) peers,
           OwnershipGiven BRSetOf(BREthereumBlock) blocks,
           OwnershipGiven BRSetOf(BREthereumTransaction) transactions,
           OwnershipGiven BRSetOf(BREthereumLog) logs) {

    BREthereumBCS bcs = (BREthereumBCS) calloc (1, sizeof(struct BREthereumBCSStruct));

    bcs->network = network;
    bcs->address = address;
    bcs->accountStateBlockNumber = 0;
    bcs->accountState = accountStateCreateEmpty ();
    bcs->mode = mode;
    bcs->filterForAddressOnTransactions = bloomFilterCreateAddress(bcs->address);
    bcs->filterForAddressOnLogs = logTopicGetBloomFilterAddress(bcs->address);

    bcs->listener = listener;

    //
    // Initialize the `headers`, `chain, and `orphans`
    //
    bcs->chain = NULL;
    bcs->chainTail = NULL;
    bcs->blocks = BRSetNew (blockHashValue,
                            blockHashEqual,
                            BCS_BLOCKS_INITIAL_CAPACITY);
    bcs->orphans = BRSetNew (blockHashValue,
                             blockHashEqual,
                             BCS_ORPHAN_BLOCKS_INITIAL_CAPACITY);

    //
    // Initialize `transactions` and `logs` sets
    //
    bcs->transactions = BRSetNew (transactionHashValue,
                                  transactionHashEqual,
                                  BCS_TRANSACTIONS_INITIAL_CAPACITY);

    bcs->logs = BRSetNew (logHashValue,
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
    bcs->handler = eventHandlerCreate ("Core Ethereum BCS",
                                       bcsEventTypes,
                                       bcsEventTypesCount,
                                       NULL);

    // For the event Handler install a periodic alarm; when the alarm triggers, BCS will check
    // on the status of any pending transactions.  This event will only trigger when the
    // event handler is running (the time between `eventHandlerStart()` and `eventHandlerStop()`)
    eventHandlerSetTimeoutDispatcher (bcs->handler,
                                      1000 * BCS_TRANSACTION_CHECK_STATUS_SECONDS,
                                      (BREventDispatcher)bcsPeriodicDispatcher,
                                      (void*) bcs);

    // Initialize `chain` - will be modified based on `blocks`
    bcs->chain = bcs->chainTail = bcs->genesis;

    // Initialize blocks, transactions and logs from saved state.
    bcsCreateInitializeBlocks(bcs, blocks);
    bcsCreateInitializeTransactions(bcs, transactions);
    bcsCreateInitializeLogs(bcs, logs);

    // Initialize LES and SYNC - we must create LES from a block where the totalDifficulty is
    // computed.  In practice, we need all the blocks from bcs->chain back to a checkpoint - and
    // that is unlikely.  We'll at least try.
    UInt256 totalDifficulty = blockRecursivelyPropagateTotalDifficulty (bcs->chain);
    BREthereumBlockHeader chainHeader = blockGetHeader (bcs->chain);

    // Okay, we tried to get totalDifficulty - if it failed, fallback to a checkpoint.
    if (ETHEREUM_BOOLEAN_IS_FALSE (blockHasTotalDifficulty(bcs->chain))) {
        const BREthereumBlockCheckpoint *checkpoint =
        blockCheckpointLookupByNumber (bcs->network, blockGetNumber(bcs->chain));

        totalDifficulty = checkpoint->u.td;
        chainHeader = blockCheckpointCreatePartialBlockHeader(checkpoint);
    }

    // There is no need to discover nodes if we are in BRD_ONLY mode.
    BREthereumBoolean discoverNodes = AS_ETHEREUM_BOOLEAN (mode != BRD_ONLY);
#if defined (LES_DISABLE_DISCOVERY)
    discoverNodes = ETHEREUM_BOOLEAN_FALSE;
#endif

    BREthereumBoolean handleSync = AS_ETHEREUM_BOOLEAN (P2P_ONLY == mode ||
                                                        P2P_WITH_BRD_SYNC == mode);

    bcs->les = lesCreate (bcs->network,
                          (BREthereumLESCallbackContext) bcs,
                          (BREthereumLESCallbackAnnounce) bcsSignalAnnounce,
                          (BREthereumLESCallbackStatus) bcsSignalStatus,
                          (BREthereumLESCallbackSaveNodes) bcsSignalNodes,
                          blockHeaderGetHash(chainHeader),
                          blockHeaderGetNumber(chainHeader),
                          totalDifficulty,
                          blockGetHash (bcs->genesis),
                          peers,
                          discoverNodes,
                          handleSync);

    if (chainHeader != blockGetHeader(bcs->chain))
        blockHeaderRelease(chainHeader);

    bcs->sync = bcsSyncCreate ((BREthereumBCSSyncContext) bcs,
                               (BREthereumBCSSyncReportBlocks) bcsSyncReportBlocksCallback,
                               (BREthereumBCSSyncReportProgress) bcsSyncReportProgressCallback,
                               bcs->address,
                               bcs->les,
                               bcs->handler);

    bcs->pow = proofOfWorkCreate();

    return bcs;
}

extern void
bcsStart (BREthereumBCS bcs) {
    eventHandlerStart(bcs->handler);
    lesStart (bcs->les);
}

extern void
bcsStop (BREthereumBCS bcs) {
    //
    // What order for these stop functions?
    //   a) If we stop LES first, then the BCS thread might add an event to LES
    //   b) If we stop BCS first, then the LES thread might add an event to BCS, as a callback.
    //
    lesStop (bcs->les);
    eventHandlerStop (bcs->handler);
}

extern BREthereumBoolean
bcsIsStarted (BREthereumBCS bcs) {
    return AS_ETHEREUM_BOOLEAN (eventHandlerIsRunning(bcs->handler));
}

extern void
bcsDestroy (BREthereumBCS bcs) {
    // Ensure we are stopped and no longer handling events (anything submitted will pile up).
    if (ETHEREUM_BOOLEAN_IS_TRUE(bcsIsStarted(bcs)))
        bcsStop (bcs);

    lesRelease (bcs->les);
    bcsSyncRelease(bcs->sync);
    proofOfWorkRelease(bcs->pow);

    // TODO: We'll need to announce things to our `listener`

    // Headers
    BRSetFreeAll (bcs->blocks, (void (*) (void*)) blockRelease);

    // Orphans (All are in 'blocks') so don't release the block.
    BRSetFree (bcs->orphans);

    // Transaction
    BRSetFreeAll (bcs->transactions, (void (*) (void*)) transactionRelease);

    // Logs
    BRSetFreeAll (bcs->logs, (void (*) (void*)) logRelease);
    
    // pending transactions/logs are in bcs->transactions/logs; thus already released.
    array_free (bcs->pendingTransactions);
    array_free (bcs->pendingLogs);

    bcs->genesis = NULL;
    
    // Destroy the Event w/ queue
    eventHandlerDestroy(bcs->handler);
    free (bcs);
}

extern void
bcsClean (BREthereumBCS bcs) {
    if (NULL != bcs->les) lesClean (bcs->les);
}

static void
bcsSyncRange (BREthereumBCS bcs,
              BREthereumNodeReference node,
              uint64_t blockNumberStart,
              uint64_t blockNumberStop) {
    // If we are in a sync already, skip out.
    if (ETHEREUM_BOOLEAN_IS_TRUE (bcsSyncIsActive(bcs->sync))) return;

    // If we don't have a valid range then skip out as well.
    if (blockNumberStop <= blockNumberStart) return;

    BREthereumSyncInterestSet interests;
    uint64_t blockNumberStartAdjusted;

    switch (bcs->mode) {
        case BRD_ONLY:
        case BRD_WITH_P2P_SEND:
            assert (0);

        case P2P_WITH_BRD_SYNC:
            //
            // For a PRIME_WITH_ENDPOINT sync we rely 100% on the BRD backend to provide any and
            // all blocks of interest - which is any block involving `address` in a transaction
            // or a log.  But, we do need recent blocks anyway - not for transaction and logs -
            // but for block chain validity.
            //
            interests = syncInterestsCreate (4,
                                             CLIENT_GET_BLOCKS_LOGS_AS_SOURCE,
                                             CLIENT_GET_BLOCKS_LOGS_AS_TARGET,
                                             CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE,
                                             CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET);
            blockNumberStartAdjusted = maximum (blockNumberStart, blockNumberStop - SYNC_LINEAR_LIMIT + 1);
            break;

        case P2P_ONLY:
            //
            // For a FULL_BLOCKCHAIN sync we run our 'N-Ary Search on Account Changes' algorithm
            // which has a (current) weakness on 'ERC20 transfers w/ address as target'.  So, we
            // exploit the BRD backend, view the `getBlocksCallback()`, to get interesting blocks.
            //
            interests = syncInterestsCreate(1, CLIENT_GET_BLOCKS_LOGS_AS_TARGET);
            blockNumberStartAdjusted = blockNumberStart;
            break;
    }

    bcs->listener.getBlocksCallback (bcs->listener.context,
                                     bcs->address,
                                     interests,
                                     blockNumberStart,
                                     blockNumberStop);

    // Run the 'Search' algorithm -
    bcsSyncStart (bcs->sync, node, blockNumberStartAdjusted, blockNumberStop);
}

extern void
bcsSync (BREthereumBCS bcs,
         uint64_t blockNumber) {
    assert (P2P_ONLY == bcs->mode || P2P_WITH_BRD_SYNC == bcs->mode);

    // Stop a sync that is currently in progress.
    if (ETHEREUM_BOOLEAN_IS_TRUE(bcsSyncInProgress(bcs)))
        bcsSyncStop(bcs->sync);

    // Start a new sync from the provided `blockNumber` up to the block at the head.
    bcsSyncRange (bcs,
                  NODE_REFERENCE_ANY,
                  blockNumber,
                  blockGetNumber(bcs->chain));
}

extern BREthereumBoolean
bcsSyncInProgress (BREthereumBCS bcs) {
    return bcsSyncIsActive (bcs->sync);
}

extern void
bcsSendTransaction (BREthereumBCS bcs,
                    BREthereumTransaction transaction) {
    assert (BRD_ONLY != bcs->mode);
    bcsSignalSubmitTransaction (bcs, transactionCopy (transaction));
}

extern void
bcsSendTransactionRequest (BREthereumBCS bcs,
                           BREthereumHash transactionHash,
                           uint64_t blockNumber,
                           uint64_t blockTransactionIndex) {
    assert (P2P_ONLY == bcs->mode || P2P_WITH_BRD_SYNC == bcs->mode);
    // There is a transaction in `blockNumber` - get the block header and 'flow through' the logic
    // to find the suspected transaction.
    lesProvideBlockHeaders (bcs->les,
                            NODE_REFERENCE_ANY,
                            (BREthereumLESProvisionContext) bcs,
                            (BREthereumLESProvisionCallback) bcsSignalProvision,
                            blockNumber, 1, 0, ETHEREUM_BOOLEAN_FALSE);
}

extern void
bcsSendLogRequest (BREthereumBCS bcs,
                   BREthereumHash transactionHash,
                   uint64_t blockNumber,
                   uint64_t blockTransactionIndex) {
    assert (P2P_ONLY == bcs->mode || P2P_WITH_BRD_SYNC == bcs->mode);
    // There is a log in `blockNumber` - get the block header and 'flow through' the logic to find
    // the suspected log.
    lesProvideBlockHeaders (bcs->les,
                            NODE_REFERENCE_ANY,
                            (BREthereumLESProvisionContext) bcs,
                            (BREthereumLESProvisionCallback) bcsSignalProvision,
                            blockNumber, 1, 0, ETHEREUM_BOOLEAN_FALSE);
}

extern void
bcsReportInterestingBlocks (BREthereumBCS bcs,
                            // interest
                            // request id
                            BRArrayOf(uint64_t) blockNumbers) {
    assert (P2P_ONLY == bcs->mode || P2P_WITH_BRD_SYNC == bcs->mode);
    eth_log ("BCS", "Report Interesting Blocks: %zu", array_count(blockNumbers));
    for (size_t index = 0; index < array_count(blockNumbers); index++)
        lesProvideBlockHeaders (bcs->les,
                                NODE_REFERENCE_ANY,
                                (BREthereumLESProvisionContext) bcs,
                                (BREthereumLESProvisionCallback) bcsSignalProvision,
                                blockNumbers[index], 1, 0, ETHEREUM_BOOLEAN_FALSE);
    array_free (blockNumbers);
}

static int
bcsLookupPendingTransaction (BREthereumBCS bcs,
                             BREthereumHash hash) {
    for (int i = 0; i < array_count(bcs->pendingTransactions); i++)
        if (ETHEREUM_BOOLEAN_IS_TRUE (hashEqual(bcs->pendingTransactions[i], hash)))
            return i;
    return -1;
}

static void
bcsPendTransaction (BREthereumBCS bcs,
                    OwnershipKept BREthereumTransaction transaction) {
    BREthereumHash hash = transactionGetHash (transaction);
    if (-1 == bcsLookupPendingTransaction(bcs, hash))
        array_add (bcs->pendingTransactions, hash);
}

static void
bcsUnpendTransaction (BREthereumBCS bcs,
                      OwnershipKept BREthereumTransaction transaction) {
    int index = bcsLookupPendingTransaction (bcs, transactionGetHash (transaction));
    if (-1 != index)
        array_rm (bcs->pendingTransactions, index);
}

static int
bcsLookupPendingLog (BREthereumBCS bcs,
                     BREthereumHash hash) {
    for (int i = 0; i < array_count(bcs->pendingLogs); i++)
        if (ETHEREUM_BOOLEAN_IS_TRUE (hashEqual(bcs->pendingLogs[i], hash)))
            return i;
    return -1;
}

static void
bcsPendLog (BREthereumBCS bcs,
            OwnershipKept BREthereumLog log) {
    BREthereumHash hash = logGetHash (log);
    if (-1 == bcsLookupPendingLog(bcs, hash))
        array_add (bcs->pendingLogs, hash);
}

#if defined (INCLUDE_UNUSED_FUNCTION)
static void
bcsUnpendLog (BREthereumBCS bcs,
              OwnershipKept BREthereumLog log) {
    int index = bcsLookupPendingLog (bcs, logGetHash (log));
    if (-1 != index)
        array_rm (bcs->pendingLogs, index);
}

static BREthereumLog
bcsPendFindLogByLogHash (BREthereumBCS bcs,
                         BREthereumHash hash) {
    return (-1 != bcsLookupPendingLog(bcs, hash)
            ? BRSetGet (bcs->logs, &hash)
            : NULL);
}
#endif

static BRArrayOf(BREthereumLog)
bcsPendFindLogsByTransactionHash (BREthereumBCS bcs,
                                  BREthereumHash hash) {
    BRArrayOf(BREthereumLog) logs = NULL;
    for (int i = 0; i < array_count(bcs->pendingLogs); i++) {
        BREthereumHash logHash = bcs->pendingLogs[i];
        BREthereumLog  log     = BRSetGet (bcs->logs, &logHash);
        if (NULL != log) {
            BREthereumHash txHash;
            if (ETHEREUM_BOOLEAN_IS_TRUE (logExtractIdentifier (log, &txHash, NULL)) &&
                ETHEREUM_BOOLEAN_IS_TRUE (hashEqual(txHash, hash))) {
                if (NULL == logs) array_new (logs, 1);
                array_add (logs, log);
            }
        }
    }
    return logs;
}

/**
 * Submit a new transaction to the Ethereum network.  The transaction will be submitted to all
 * connected nodes and once submitted the nodes will be repeatedly queried for the transaction's
 * status.
 *
 * NOTE: For implementaton, this is one of two places where a transaction becomes `pending`.  The
 * other place is when an included transaction finds its block orphaned.  The `pending` transaction
 * has it's status queried for states like: QUEUED, PENDING, INCLUDED or ERROR.  [A transaction
 * that is discovered during a sync, in a chained block, is not pended - it goes immediately to
 * INCLUDED.].
 *
 * @param bcs bcs
 * @param transaction the signed transaction
 *
 */
extern void
bcsHandleSubmitTransaction (BREthereumBCS bcs,
                            OwnershipGiven BREthereumTransaction transaction) {
    // By now, surely signed
    assert (ETHEREUM_BOOLEAN_IS_TRUE (transactionIsSigned(transaction)));

    // ... and thus with a valid hash.
    BREthereumHash hash = transactionGetHash (transaction);

    // Check if the transaction is already pending; this on the slight chance of a resubmission.
    int pendingIndex = bcsLookupPendingTransaction (bcs, hash);
    if (-1 != pendingIndex) return;  // already pending, so skip out.

    // Make the transaction pending.
    bcsPendTransaction(bcs, transaction);

    // Signal a create/signed/submitted transaction.  This ultimately will callback to bcs
    // clients to announce a new transfer.
    bcsSignalTransaction(bcs, transactionCopy (transaction));

    // Actually submit... which will get the transaction status.
    lesSubmitTransaction (bcs->les,
                          NODE_REFERENCE_ALL,
                          (BREthereumLESProvisionContext) bcs,
                          (BREthereumLESProvisionCallback) bcsSignalProvision,
                          transaction);
}

extern void
bcsHandleStatus (BREthereumBCS bcs,
                 BREthereumNodeReference node,
                 BREthereumHash headHash,
                 uint64_t headNumber) {
    // If we are not a P2P_* node, we won't handle announcements
    if (BRD_ONLY == bcs->mode || BRD_WITH_P2P_SEND == bcs->mode) {
#if defined (BCS_REPORT_IGNORED_ANNOUNCE)
        eth_log ("BCS", "Status %" PRIu64 " Ignored (not P2P) <== %s",
                 headNumber,
                 lesGetNodeHostname (bcs->les, node));
#endif
        return;
    }

    bcsSyncRange (bcs, node, blockGetNumber(bcs->chain), headNumber);
}

/*!
 * @function bcsHandleAnnounce
 *
 * @abstract
 * Handle a LES 'announce' result.
 */
extern void
bcsHandleAnnounce (BREthereumBCS bcs,
                   BREthereumNodeReference node,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   UInt256 headTotalDifficulty,
                   uint64_t reorgDepth) {
    // If we are not a P2P_* node, we won't handle announcements
    if (BRD_ONLY == bcs->mode || BRD_WITH_P2P_SEND == bcs->mode) {
#if defined (BCS_REPORT_IGNORED_ANNOUNCE)
        eth_log ("BCS", "Block %" PRIu64 " Ignored (not P2P) <== %s",
                 headNumber,
                 lesGetNodeHostname (bcs->les, node));
#endif
        return;
    }

    // If we are in the middle of a sync, we won't be reorganizing anything.
    if (ETHEREUM_BOOLEAN_IS_TRUE (bcsSyncIsActive(bcs->sync)) && 0 != reorgDepth) {
        reorgDepth = 0;
        eth_log ("BCS", "ReorgDepth: %" PRIu64 " @ %" PRIu64 ": Ignored, in Sync", reorgDepth, headNumber);
    }

    // Reorg depth suggests that N blocks are wrong. We'll orphan all of them, request the next
    // block headers back in history by reorgDepth, and likely perform a sync to fill in the
    // missing headers.
    if (0 != reorgDepth) {
        eth_log ("BCS", "ReorgDepth: %" PRIu64 " @ %" PRIu64, reorgDepth, headNumber);
        if (reorgDepth < BCS_REORG_LIMIT)
            bcsUnwindChain (bcs, reorgDepth, headNumber);
    }

    // Request the block - backup a bit if we need to reorg.  Figure it will sort itself out
    // as old blocks arrive.
    lesProvideBlockHeaders (bcs->les,
                            node,
                            (BREthereumLESProvisionContext) bcs,
                            (BREthereumLESProvisionCallback) bcsSignalProvision,
                            headNumber - reorgDepth,
                            (uint32_t) (1 + reorgDepth),
                            0,
                            ETHEREUM_BOOLEAN_FALSE);
}

/// MARK: - Chain

static void
bcsReclaimBlock (BREthereumBCS bcs,
                 BREthereumBlock block,
                 int useLog) {
    BRSetRemove (bcs->orphans, block);  // needed, or overly cautious?
    BRSetRemove (bcs->blocks,  block);
    if (useLog) eth_log("BCS", "Block %" PRIu64 " Reclaimed", blockGetNumber(block));

    // TODO: Avoid dangling references - need to identify one/some first.

    blockRelease(block);
}

static void
bcsSaveBlocks (BREthereumBCS bcs) {
    // We'll save everything between bcs->chain->next and bcs->chainTail
    uint64_t blockCountRaw = blockGetNumber(bcs->chain) - blockGetNumber(bcs->chainTail);
    assert (blockCountRaw <= (uint64_t) SIZE_MAX);
    size_t blockCount = (size_t) blockCountRaw;

    // We'll pass long the blocks directly, for now.  This will likely need to change because
    // this code will lose control of the block memory.
    //
    // For example, we quickly hit another 'ReclaimAndSaveBlocks' point prior the the
    // `saveBlocksCallback` completing - we'll then release memory needed by the callback handler.
    BRArrayOf(BREthereumBlock) blocks;
    array_new (blocks, blockCount);
    array_set_count (blocks, blockCount);

    BREthereumBlock next = blockGetNext (bcs->chain);
    BREthereumBlock last = bcs->chainTail;
    size_t blockIndex = blockCount - 1;

    while (next != last) {
        blocks[blockIndex--] = next;
        next = blockGetNext(next);
    }
    assert (0 == blockIndex);
    blocks[blockIndex--] = next;

    bcs->listener.saveBlocksCallback (bcs->listener.context, blocks);
    
    eth_log("BCS", "Blocks {%" PRIu64 ", %" PRIu64 "} Saved",
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

        eth_log("BCS", "Blocks {%" PRIu64 ", %" PRIu64 "} Reclaimed",
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

    eth_log("BCS", "Block %" PRIu64 " %s", blockGetNumber(block), message);

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
    // don't orphan when we are syncing from the genesis block.
    if (blockNumber <= BCS_ORPHAN_AGE_OFFSET) return;

    // Modify blockNumber for comparision with orphans
    blockNumber -= BCS_ORPHAN_AGE_OFFSET;

    // Look through all the orphans; remove those with old/small block numbers.  But, don't purge
    // any block this is pending blocks/receipts.
    int keepLooking = 1;
    while (keepLooking) {
        keepLooking = 0;
        FOR_SET (BREthereumBlock, orphan, bcs->orphans)
            if (blockGetNumber(orphan) < blockNumber &&
                ETHEREUM_BOOLEAN_IS_TRUE (blockHasStatusComplete(orphan))) {
                BRSetRemove(bcs->orphans, orphan);
                eth_log("BCS", "Block %" PRIu64 " Purged Orphan", blockGetNumber(orphan));

                // TODO: Don't release `orphan` if it is in `bcs->blocks`
//                blockRelease(orphan);
                keepLooking = 1;
                break; // FOR_SET
            }
    }
}


/**
 * Select between `block1` and `block2` for extending the chain. (We assume the two blocks have
 * the same parent hash).  Selection criteria include: totalDifficulty and timestamp.
 */
static BREthereumBlock
bcsSelectPreferredBlock (BREthereumBCS bcs,
                         BREthereumBlock block1,
                         BREthereumBlock block2) {
    if (NULL == block1) return block2;
    if (NULL == block2) return block1;

    // Gitter: "hi. when chain reorg occurs in Ethereum, the common way is first find common block,
    // insert the block with the more total difficuty, and keep the transaction within the old
    // chain but not within the new chain back to txpool."
    
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
    eth_log ("BCS", "Block %" PRIu64 " Newly Orphaned", blockGetNumber(block));

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

    // Examine transactions to see if any are now orphaned; is so, make them PENDING.  We'll start
    // requesting status and expect some node to offer up a different block.
    FOR_SET(BREthereumTransaction, transaction, bcs->transactions) {
        status = transactionGetStatus(transaction);
        if (transactionStatusExtractIncluded(&status, &blockHash, NULL, NULL, NULL, NULL) &&
            NULL != BRSetGet (bcs->orphans, &blockHash)) {
            bcsPendTransaction(bcs, transaction);
        }
    }

    // Examine logs to see if any are now orphaned.  Logs are seen if and only if they are
    // in a block; see if that block is now an orphan and if so make the log pending.
    FOR_SET(BREthereumLog, log, bcs->logs) {
        status = logGetStatus(log);
        if (transactionStatusExtractIncluded(&status, &blockHash, NULL, NULL, NULL, NULL) &&
            NULL != BRSetGet (bcs->orphans, &blockHash)) {
            bcsPendLog (bcs, log);
        }
    }
}

static void
bcsUnwindChain (BREthereumBCS bcs,
                uint64_t depth,
                uint64_t headNumber) {
    // If nothing to unwind, return
    if (NULL == bcs->chain || depth > headNumber) return;

    // Any chained block with a number at or over `badHeadNumber` needs to be orphaned
    uint64_t badHeadNumber = headNumber - depth;

    // Unwind the chain, making orphans as we go.
    while (depth-- > 0 && bcs->chainTail != bcs->chain)
        if (blockGetNumber (bcs->chain) >= badHeadNumber) {
            BREthereumBlock next = blockGetNext (bcs->chain);
            bcsMakeOrphan (bcs, bcs->chain);
            bcs->chain = next;
        }

    // Until bcsMakeOrphan() pends transactions and logs, we'll do it here.
    bcsPendOrphanedTransactionsAndLogs (bcs);
}

static void
bcsShowBlockForChain (BREthereumBCS bcs,
                      BREthereumBlock block,
                      const char *preface) {
    BREthereumHashString parent, hash;
    hashFillString (blockGetHash(block), hash);
    hashFillString (blockHeaderGetParentHash(blockGetHeader(block)), parent);
    eth_log ("BCS", "%s: %" PRIu64 ", Hash: %s, Parent: %s",
             preface,
             blockGetNumber (block),
             hash,
             parent);
}
extern void
bcsOrphansShow (BREthereumBCS bcs,
                BREthereumBoolean showChain) {
//    BRArrayOf(BREthereumBlock) orphans;
//    array_new (orphans, BRSetCount(bcs->orphans));

    eth_log ("BCS", "Orphans%s", "");
    if (ETHEREUM_BOOLEAN_IS_TRUE (showChain) && NULL != bcs->chain) {
        BREthereumBlock next = blockGetNext(bcs->chain);
        if (NULL != next)
            bcsShowBlockForChain(bcs, next, "block1");
        bcsShowBlockForChain(bcs, bcs->chain, "block0");
    }

    FOR_SET(BREthereumBlock, orphan, bcs->orphans)
        bcsShowBlockForChain (bcs, orphan, "Orphan");
}

#if defined (INCLUDE_UNUSED_FUNCTION)
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
#endif

static int
bcsIsBlockValid (BREthereumBCS bcs,
                 BREthereumBlock block) {
    return ETHEREUM_BOOLEAN_IS_TRUE (blockIsValid (block));
}

/**
 * Extends `bcs->transactions` and `bcs->logs` with the tranactions and logs within `block`.
 * Requires `block` to be in 'complete' and in `bcs->chain`.
 */
static void
bcsExtendTransactionsAndLogsForBlock (BREthereumBCS bcs,
                                      BREthereumBlock block) {
    assert (bcsIsBlockValid (bcs, block) && ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusComplete(block)));

    // `block` is valid and complete, we can process its status transactions and logs.
    BREthereumBlockStatus blockStatus = blockGetStatus(block);

    // Process each transaction...
    if (NULL != blockStatus.transactions)
        for (size_t ti = 0; ti < array_count(blockStatus.transactions); ti++)
            bcsHandleTransaction(bcs, blockStatus.transactions[ti]);

    // ... then process each log...
    if (NULL != blockStatus.logs)
        for (size_t li = 0; li < array_count(blockStatus.logs); li++)
            bcsHandleLog (bcs, blockStatus.logs[li]);

    // Clear the status but don't touch {Transactions,Logs} (they have OwnershipGiven by the
    // above calls to bcsHandle{Transaction,Log}()).
    blockReleaseStatus (block, ETHEREUM_BOOLEAN_FALSE, ETHEREUM_BOOLEAN_FALSE);

    // If not in chain and not an orphan, then reclaim
    if (bcs->chainTail != block && NULL == blockGetNext(block) && NULL == BRSetGet(bcs->orphans, block))
        bcsReclaimBlock(bcs, block, 0);
}

static int
bcsWantToHandleTransactionsAndLogs (BREthereumBCS bcs,
                                    BREthereumBlock block) {
    BREthereumBlockStatus blockStatus = blockGetStatus(block);
    return (NULL != blockStatus.transactions || NULL != blockStatus.logs);
}

/**
 * Extend transactions and logs for block if and only if block is 'complete' and in bcs->chain.
 */
static void
bcsExtendTransactionsAndLogsForBlockIfAppropriate (BREthereumBCS bcs,
                                                   BREthereumBlock block) {
    if (ETHEREUM_BOOLEAN_IS_TRUE (blockHasStatusComplete(block))) {
        if (bcsIsBlockValid(bcs, block)) bcsExtendTransactionsAndLogsForBlock (bcs, block);
        else if (bcsWantToHandleTransactionsAndLogs (bcs, block))
            eth_log ("BCS", "Block %" PRIu64 " completed, not valid", blockGetNumber(block));
    }
}

static void
bcsExtendChainIfPossible (BREthereumBCS bcs,
                          BREthereumNodeReference node,
                          BREthereumBlock block,
                          int isFromSync) {
    // THIS WILL BE THE FIRST TIME WE'VE SEEN BLOCK.  EVEN IF COMPLETE, NONE OF ITS LOGS NOR
    // TRANSACTIONS ARE HELD.
    //
    // TODO: Handle case where block is well in the past, like from a sync.

    // Lookup `headerParent`
    BREthereumHash blockParentHash = blockHeaderGetParentHash(blockGetHeader(block));
    BREthereumBlock blockParent = BRSetGet(bcs->blocks, &blockParentHash);

    // If we have a parent, but `header` is inconsistent with its parent, then ignore `header`
    if (NULL != blockParent &&
        ETHEREUM_BOOLEAN_IS_FALSE (blockHeaderIsValid (blockGetHeader(block),
                                                       blockGetHeader(blockParent),
                                                       blockGetOmmersCount(blockParent),
                                                       blockGetHeader(bcs->genesis),
                                                       bcs->pow))) {

        eth_log("BCS", "Block %" PRIu64 " Inconsistent", blockGetNumber(block));
        // TODO: Can we release `block`?
        return;
    }

    // Put `header` in the `chain` - HANDLE 3 CASES:

    // 1) If we do not have any chain, then adopt `block` directly, no questions asked.  This will
    // be used for P2P_WITH_BRD_SYNC where we get all interesting transactions, logs,
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
            // try to chain.
            eth_log ("BCS", "Chain Try%s", "");
            bcsChainThenPurgeOrphans (bcs);

            // Add it to the set of orphans and RETURN (non-local exit).
            bcsMakeOrphan(bcs, block);

            // If `block` is an orphan, then it's parent is not in bcs->chain.  That could be
            // because there is just a fork developing or that we've fallen behind.  Attempt a
            // sync to recover (might not actually perform a sync - just attempt).
            uint64_t orphanBlockNumberMinumum = bcsGetOrphanBlockNumberMinimum(bcs);
            if (UINT64_MAX != orphanBlockNumberMinumum)
#if defined (BCS_SHOW_ORPHANS)
                if (ETHEREUM_BOOLEAN_IS_FALSE (bcsSyncIsActive(bcs->sync)))
                    bcsOrphansShow (bcs, ETHEREUM_BOOLEAN_TRUE);
#endif
                // Note: This can be an invalid range.  Say we have a old orphan that hasn't
                // been purged yet.. might be that orphanBlockNumberMinumum is in the past.
                // In `bcsSyncRange()` we'll check for a valid range.
                bcsSyncRange (bcs, node,
                              blockGetNumber(bcs->chain),
                              orphanBlockNumberMinumum);

            return;
        }

        // ... otherwise, if in a sync, then adopt block
        else if (blockGetNumber(block) > blockGetNumber(bcs->chain)) {
            BREthereumBlock oldChainHead = bcs->chain;
            BREthereumBlock oldChainTail = bcs->chainTail;
            BREthereumBlock oldChainStop = (NULL == oldChainTail ? oldChainTail : blockGetNext(oldChainTail));

            // Reclaim the old `chain`
            while (oldChainHead != oldChainStop) {
                BREthereumBlock oldChainNext = blockGetNext(oldChainHead);
                // Never reclaim if `oldChainhead` is not complete
                if (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusComplete(oldChainHead)))
                    bcsReclaimBlock(bcs, oldChainHead, 0);
                oldChainHead = oldChainNext;
            }

            // Adopt `block` as `chain`
            bcs->chain = bcs->chainTail = block;
            blockClrNext(block);
            eth_log("BCS", "Block %" PRIu64 " Chained (Sync)", blockGetNumber(block));
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
    if (ETHEREUM_BOOLEAN_IS_FALSE (bcsSyncIsActive(bcs->sync)))
        bcsOrphansShow (bcs, ETHEREUM_BOOLEAN_TRUE);

    // We have now extended the chain from blockParent, with possibly multiple blocks, to
    // bcs->chain.  We also have updated bcs->orphans with all orphans (but we don't know which of
    // the orphans are newly orphaned).
    //
    // It is now time to extend `transactions` and `logs` based on the extended chain and orphans
    BCS_FOR_CHAIN(bcs, block) {
        if (block == blockParent) break; // done
        bcsExtendTransactionsAndLogsForBlockIfAppropriate (bcs, block);
    }

    // And finally purge any transactions and logs for orphaned blocks
    bcsPendOrphanedTransactionsAndLogs (bcs);

    // Periodically reclaim 'excessive' blocks and save the latest.
    bcsReclaimAndSaveBlocksIfAppropriate (bcs);
}

/// MARK: - Block Header

static BREthereumBoolean
bcsBlockHasMatchingTransactions (BREthereumBCS bcs,
                                 BREthereumBlock block) {
    return ETHEREUM_BOOLEAN_TRUE;
    // return ETHEREUM_BOOLEAN_FALSE;
}

static BREthereumBoolean
bcsBlockHasMatchingLogs (BREthereumBCS bcs,
                         BREthereumBlock block) {
    return blockHeaderMatch (blockGetHeader (block), bcs->filterForAddressOnLogs);
}

static BREthereumBoolean
bcsBlockNeedsAccountState (BREthereumBCS bcs,
                           BREthereumBlock block) {
    return ETHEREUM_BOOLEAN_FALSE;
}

static BREthereumBoolean
bcsBlockNeedsHeaderProof (BREthereumBCS bcs,
                          BREthereumBlock block) {
    return blockHeaderIsCHTRoot (blockGetHeader (block));
}

/**
 * Handle a (generally new) block header.
 */
static void
bcsHandleBlockHeaderInternal (BREthereumBCS bcs,
                              BREthereumNodeReference node,
                              OwnershipGiven BREthereumBlockHeader header,
                              int isFromSync,
                              BRArrayOf(BREthereumHash) *bodiesHashes,
                              BRArrayOf(BREthereumHash) *receiptsHashes,
                              BRArrayOf(BREthereumHash) *accountsHashes,
                              BRArrayOf(uint64_t) *proofNumbers) {

    // Ignore the header if we have seen it before.  Given an identical hash, *nothing*, at any
    // level (transactions, receipts, logs), could have changed and thus no processing is needed.
    BREthereumHash blockHash = blockHeaderGetHash(header);
    if (NULL != BRSetGet(bcs->blocks, &blockHash)) {
        eth_log("BCS", "Block %" PRIu64 " Ignored", blockHeaderGetNumber(header));
        blockHeaderRelease(header);
        return;
    }

    // Ignore the header if it is not valid.
    if (ETHEREUM_BOOLEAN_IS_FALSE(blockHeaderIsInternallyValid (header))) {
        eth_log("BCS", "Block %" PRIu64 " Invalid", blockHeaderGetNumber(header));
        blockHeaderRelease(header);
        return;
    }

    // ?? Other checks ??

    // We have a header that appears consistent.  Create a block and work to fill it out. Note
    // that `header` memory ownership is transferred to `block`
    BREthereumBlock block = blockCreate(header);
    BRSetAdd(bcs->blocks, block);

    // Check if we need 'transaction receipts', 'block bodies', 'account state' or a 'header proof'.
    // We'll use the header's logsBloom for the recipts check; we've got nothing in the header to
    // check for needing bodies nor for needing account state.  We'll get block bodies by default
    // and avoid account state (getting account state might allow us to avoid getting block bodies;
    // however, the client cost to get the account state is ~2.5 times more then getting block
    // bodies so we'll just get block bodies and compute the account state).  We'll need the 'header
    // proof' occassionally so that we can build on the block chain's total difficulty and
    // ultimately our Proof-of-Work validations.
    BREthereumBoolean needBodies   = bcsBlockHasMatchingTransactions(bcs, block);
    BREthereumBoolean needReceipts = bcsBlockHasMatchingLogs(bcs, block);
    BREthereumBoolean needAccount  = bcsBlockNeedsAccountState(bcs, block);
    BREthereumBoolean needProof    = bcsBlockNeedsHeaderProof(bcs, block);

    // Request block bodies, if needed.
    if (ETHEREUM_BOOLEAN_IS_TRUE(needBodies)) {
        blockReportStatusTransactionsRequest(block, BLOCK_REQUEST_PENDING);
        if (NULL == *bodiesHashes) array_new (*bodiesHashes, 200);
        array_add (*bodiesHashes, blockGetHash(block));
        eth_log("BCS", "Block %" PRIu64 " Needs Bodies", blockGetNumber(block));
    }

    // Request transaction receipts, if needed.
    if (ETHEREUM_BOOLEAN_IS_TRUE(needReceipts)) {
        blockReportStatusLogsRequest(block, BLOCK_REQUEST_PENDING);
        if (NULL == *receiptsHashes) array_new (*receiptsHashes, 200);
        array_add (*receiptsHashes, blockGetHash(block));
        eth_log("BCS", "Block %" PRIu64 " Needs Receipts", blockGetNumber(block));
    }

    // Request account state, if needed.
    if (ETHEREUM_BOOLEAN_IS_TRUE(needAccount)) {
        blockReportStatusAccountStateRequest (block, BLOCK_REQUEST_PENDING);
        if (NULL == *accountsHashes ) array_new (*accountsHashes,  200);
        array_add (*accountsHashes, blockGetHash(block));
        eth_log("BCS", "Block %" PRIu64 " Needs AccountState", blockGetNumber(block));
    }

    // Request header proof, if needed.
    if (ETHEREUM_BOOLEAN_IS_TRUE (needProof)) {
        blockReportStatusHeaderProofRequest (block, BLOCK_REQUEST_PENDING);
        if (NULL == *proofNumbers) array_new (*proofNumbers, 20);
        array_add (*proofNumbers, blockGetNumber(block));
        eth_log("BCS", "Blook %" PRIu64 " Needs HeaderProof", blockGetNumber(block));
    }

    // Chain 'block' - we'll do this before the block is fully constituted.  Once constituted,
    // we'll handle the block's interesting transactions and logs; including if the block is an
    // orphan.  Note: the `bcsExtendChainIfAppropriate()` function will handle a fully constituted
    // block; however, as the above suggests, the block might be complete but likely empty.
    //
    // TODO: What if the header is well into the past - like during a sync?
    bcsExtendChainIfPossible(bcs, node, block, isFromSync);
}

static void
bcsHandleBlockHeaders (BREthereumBCS bcs,
                       BREthereumNodeReference node,
                       OwnershipGiven BRArrayOf(BREthereumBlockHeader) headers,
                       int isFromSync) {
    BRArrayOf(BREthereumHash) bodiesHashes = NULL;
    BRArrayOf(BREthereumHash) receiptsHashes = NULL;
    BRArrayOf(BREthereumHash) accountsHashes = NULL;
    BRArrayOf(uint64_t) proofNumbers = NULL;

    for (size_t index = 0; index < array_count(headers); index++)
        // Each `headers[index]` has 'OwnershipGiven'
        bcsHandleBlockHeaderInternal (bcs, node,
                                      headers[index],
                                      isFromSync,
                                      &bodiesHashes,
                                      &receiptsHashes,
                                      &accountsHashes,
                                      &proofNumbers);

    array_free(headers);

    if (NULL != bodiesHashes && array_count(bodiesHashes) > 0)
        lesProvideBlockBodies (bcs->les, node,
                               (BREthereumLESProvisionContext) bcs,
                               (BREthereumLESProvisionCallback) bcsSignalProvision,
                               bodiesHashes);

    if (NULL != receiptsHashes && array_count(receiptsHashes) > 0)
        lesProvideReceipts (bcs->les, node,
                            (BREthereumLESProvisionContext) bcs,
                            (BREthereumLESProvisionCallback) bcsSignalProvision,
                            receiptsHashes);

    if (NULL != accountsHashes && array_count(accountsHashes) > 0)
        lesProvideAccountStates (bcs->les, node,
                                 (BREthereumLESProvisionContext) bcs,
                                 (BREthereumLESProvisionCallback) bcsSignalProvision,
                                 bcs->address,
                                 accountsHashes);

    if (NULL != proofNumbers && array_count(proofNumbers) > 0)
        lesProvideBlockProofs (bcs->les, node,
                               (BREthereumLESProvisionContext) bcs,
                               (BREthereumLESProvisionCallback) bcsSignalProvision,
                               proofNumbers);
}

/// MARK: - Account State

static void
bcsHandleAccountState (BREthereumBCS bcs,
                       BREthereumNodeReference node,
                       BREthereumAddress address,
                       BREthereumHash blockHash,
                       BREthereumAccountState account) {
    // Ensure we have a Block
    BREthereumBlock block = BRSetGet(bcs->blocks, &blockHash);
    if (NULL == block) {
        eth_log ("BCS", "Block %" PRIu64 " Missed (Account)", (NULL == block ? -1 : blockGetNumber(block)));
        return;
    }

    // If the status has somehow errored, skip out with nothing more to do.
    if (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusError(block))) {
        eth_log ("BCS", "Block %" PRIu64 " In Error (Account)", blockGetNumber(block));
        return;
    }

    // We must be in a 'account needed' status
    assert (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusAccountStateRequest(block, BLOCK_REQUEST_PENDING)));

    eth_log("BCS", "Account %" PRIu64 " Nonce %" PRIu64 ", Balance XX",
            blockGetNumber(block),
            accountStateGetNonce(account));

    // Report the block status - we'll flag as HAS_ACCOUNT_STATE.
    blockReportStatusAccountState(block, account);

    // TODO: On a sync this needs to be cleared, I think
    //
    // Think about if this should be handled one and only one time when the full chain is synced.
    // Until then no data (=> never a wrong, intermediate nonce).  On once at the very beginning
    // using the 'announced block head'. and then there after if a log or transaction is found.
    // Sort of: don't announce here if in a sync?
    if (blockGetNumber(block) > bcs->accountStateBlockNumber) {
        bcs->accountStateBlockNumber = blockGetNumber(block);
        bcs->accountState = account;

        // Can we do this right here?
        bcs->listener.accountStateCallback (bcs->listener.context,
                                            bcs->accountState);
    }

    bcsExtendTransactionsAndLogsForBlockIfAppropriate (bcs, block);
}

static void
bcsHandleAccountStates (BREthereumBCS bcs,
                        BREthereumNodeReference node,
                        BREthereumAddress address,
                        OwnershipGiven BRArrayOf(BREthereumHash) hashes,
                        OwnershipGiven BRArrayOf(BREthereumAccountState) states) {
    for (size_t index = 0; index < array_count(hashes); index++)
        bcsHandleAccountState (bcs, node, address, hashes[index], states[index]);
    array_free (hashes);
    array_free (states);
}

/// MARK: - Block Bodies

/*!
 */
static void
bcsReleaseOmmersAndTransactionsFully (BREthereumBCS bcs,
                                      OwnershipGiven BRArrayOf(BREthereumTransaction) transactions,
                                      OwnershipGiven BRArrayOf(BREthereumBlockHeader) ommers) {
    transactionsRelease(transactions);
    blockHeadersRelease(ommers);
}

static void
bcsHandleBlockBody (BREthereumBCS bcs,
                    BREthereumNodeReference node,
                    BREthereumHash blockHash,
                    OwnershipGiven BRArrayOf(BREthereumTransaction) transactions,
                    OwnershipGiven BRArrayOf(BREthereumBlockHeader) ommers) {
    // Ensure we have a Block
    BREthereumBlock block = BRSetGet(bcs->blocks, &blockHash);
    if (NULL == block) {
        eth_log ("BCS", "Block %" PRIu64 " Missed (Bodies)", (NULL == block ? -1 : blockGetNumber(block)));
        bcsReleaseOmmersAndTransactionsFully(bcs, transactions, ommers);
        return;
    }

    // Get the block status and begin filling it out.
//    BREthereumBlockStatus activeStatus = blockGetStatus(block);

    // If the status is some how in error, skip out with nothing more to do.
    if (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusError(block))) {
        eth_log ("BCS", "Block %" PRIu64 " In Error (Bodies)", blockGetNumber(block));
        bcsReleaseOmmersAndTransactionsFully(bcs, transactions, ommers);
        return;
    }

    // We must be in a 'bodies needed' status
    assert (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusTransactionsRequest(block, BLOCK_REQUEST_PENDING)));

    eth_log("BCS", "Bodies %" PRIu64 " O:%2zu, T:%3zu",
            blockGetNumber(block),
            array_count(ommers),
            array_count(transactions));

    // Update `block` with the reported ommers and transactions.  Note that generally
    // these are not used.... but we take full ownership of the memory for ommers and transactions.
    blockUpdateBody (block, ommers, transactions);

    // Having filled out `block`, we'll now look for transactions.  The block might be invalid but
    // we won't know that until we attempt a 'header proof' or to extend the chain.  If this block
    // proves to be invalid, any transactions we find will be dumped at that point.  [In the
    // meantime we won't be reporting the transactions (nor logs) to anybody.]

    // Find transactions of interest.
    BREthereumTransaction *neededTransactions = NULL;

    // Check the transactions one-by-one.
    for (int i = 0; i < array_count(transactions); i++) {
        BREthereumTransaction tx = transactions[i];
        assert (NULL != tx);
        
        // If it is our transaction (as source or target), handle it.
        if (ETHEREUM_BOOLEAN_IS_TRUE(transactionHasAddress(tx, bcs->address))) {
            eth_log("BCS", "Bodies %" PRIu64 " Found Transaction at %d",
                    blockGetNumber(block), i);

            // We'll need a copy of the transaction as the orginal transaction is held in `block`
            // and this transaction (the copy) will go into `needTransactions` for 'block status'
            // reporting.
            tx = transactionCopy(tx);

            // Fill-out the status.  Note that gasUsed is zero.  When this block is chained
            // we'll request the TxStatus so we can get a valid gasUsed value.
            transactionSetStatus(tx, transactionStatusCreateIncluded (blockGetHash(block),
                                                                      blockGetNumber(block),
                                                                      i,
                                                                      blockGetTimestamp(block),
                                                                      transactionGetGasLimit(tx)));

            if (NULL == neededTransactions) array_new (neededTransactions, 3);
            array_add(neededTransactions, tx);
        }
        // else - TODO: Handle if has a 'contract' address of interest?
    }

    // Report the block status.  Do so even if neededTransaction is NULL.
    blockReportStatusTransactions(block, neededTransactions);

    if (NULL != neededTransactions) {
        // Once we've identified a transaction we must get additional information:

        // 1) Get the receipts - because the receipts hold the cummulative gasUsed which will use
        //    to compute the gasUsed by each transaction.
        if (ETHEREUM_BOOLEAN_IS_TRUE (blockHasStatusLogsRequest (block, BLOCK_REQUEST_NOT_NEEDED))) {
            blockReportStatusLogsRequest (block, BLOCK_REQUEST_PENDING);
            lesProvideReceiptsOne (bcs->les, node,
                                   (BREthereumLESProvisionContext) bcs,
                                   (BREthereumLESProvisionCallback) bcsSignalProvision,
                                   blockGetHash(block));
        }

        // 2) We want a header proof too; to ensure a valid block w/ transaction.
        if (ETHEREUM_BOOLEAN_IS_TRUE (blockHasStatusHeaderProofRequest(block, BLOCK_REQUEST_NOT_NEEDED))) {
            blockReportStatusHeaderProofRequest (block, BLOCK_REQUEST_PENDING);
            lesProvideBlockProofsOne (bcs->les, node,
                                      (BREthereumLESProvisionContext) bcs,
                                      (BREthereumLESProvisionCallback) bcsSignalProvision,
                                      blockGetNumber(block));
        }

        // 3) We want the account state too - because we've found a transaction for bcs->address
        // and the account changed (instead of computing the account, we'll query definitively).
        if (ETHEREUM_BOOLEAN_IS_TRUE (blockHasStatusAccountStateRequest (block, BLOCK_REQUEST_NOT_NEEDED))) {
            blockReportStatusAccountStateRequest(block, BLOCK_REQUEST_PENDING);
            lesProvideAccountStatesOne (bcs->les, node,
                                        (BREthereumLESProvisionContext) bcs,
                                        (BREthereumLESProvisionCallback) bcsSignalProvision,
                                        bcs->address,
                                        blockGetHash(block));
        }
    }

    // If we requested Receipts (for Logs) and have them, then we can process the Logs.
    if (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusLogsRequest(block, BLOCK_REQUEST_COMPLETE)))
        blockLinkLogsWithTransactions (block);

    // In the following, 'if appropriate' means complete and chained.
    bcsExtendTransactionsAndLogsForBlockIfAppropriate (bcs, block);
}

static void
bcsHandleBlockBodies (BREthereumBCS bcs,
                      BREthereumNodeReference node,
                      OwnershipGiven BRArrayOf(BREthereumHash) hashes,
                      OwnershipGiven BRArrayOf(BREthereumBlockBodyPair) pairs) {
    for (size_t index = 0; index < array_count(hashes); index++)
        // Transactions and Uncles have 'OwnershipGiven'
        bcsHandleBlockBody (bcs, node, hashes[index], pairs[index].transactions, pairs[index].uncles);
    array_free (hashes);
    array_free (pairs);
}

/// MARK: - Header Proofs

static void
bcsHandleBlockProof (BREthereumBCS bcs,
                     BREthereumNodeReference node,
                     uint64_t number,
                     BREthereumBlockHeaderProof proof) {
    // Header Proofs *do not exist* for recent blocks; not in Parity nor in Geth:
    // https://github.com/paritytech/parity-ethereum/issues/9829

    // If the proof failed, we've nothing to do.  Worse, we've no way to lookup the block
    // from number.  Probably need to add the hash - as we should have it.  However, if we get to
    // the point of CHT handling, maybe we won't
    //
    // We really, really need to find the block and mark the block.  Can be multiple blocks...
    if (1 == eqUInt256 (proof.totalDifficulty, UINT256_ZERO)) {
        eth_log ("BCS", "Block %" PRIu64 " Proof Failed", number);
        return;
    }

    BREthereumHash blockHash = proof.hash;

    BREthereumBlock block = BRSetGet(bcs->blocks, &blockHash);
    if (NULL == block) {
        eth_log ("BCS", "Block %" PRIu64 " Missed (Proof)", number);
        return;
    }

    // Report the block status - we'll flag as HAS_HEADER_PROOF.
    blockReportStatusHeaderProof (block, proof);

    // We might not need to do the following here... wait until all the 'statuses' (transactions,
    // logs, etc) are complete and then process

    // If block has no totalDifficulty, assign it...
    if (1 == eqUInt256 (blockGetTotalDifficulty (block), UINT256_ZERO))
        blockSetTotalDifficulty (block, proof.totalDifficulty);

    // ... otherwise, if the difficulties do not match
    else if (0 == eqUInt256 (blockGetTotalDifficulty (block), proof.totalDifficulty)) {
        // TODO: This SHOULD indicate a problem...
        eth_log ("BCS", "Block %" PRIu64 " Overwrite Difficulty (Proof)", number);
        blockSetTotalDifficulty (block, proof.totalDifficulty);
    }

    // In the following, 'if appropriate' means complete and chained.
    bcsExtendTransactionsAndLogsForBlockIfAppropriate (bcs, block);

}

static void
bcsHandleBlockProofs (BREthereumBCS bcs,
                      BREthereumNodeReference node,
                      OwnershipGiven BRArrayOf(uint64_t) numbers,
                      OwnershipGiven BRArrayOf(BREthereumBlockHeaderProof) proofs) {
    for (size_t index = 0; index < array_count(numbers); index++)
        bcsHandleBlockProof (bcs, node, numbers[index], proofs[index]);
    array_free (numbers);
    array_free (proofs);
}

/// MARK: - Transaction Receipts

#if defined (INCLUDE_UNUSED_FUNCTION)
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
#endif

/*!
 */
static void
bcsReleaseReceiptsFully (BREthereumBCS bcs,
                         OwnershipGiven BRArrayOf(BREthereumTransactionReceipt) receipts) {
    transactionReceiptsRelease(receipts);
}

static void
bcsHandleTransactionReceipts (BREthereumBCS bcs,
                              BREthereumNodeReference node,
                              BREthereumHash blockHash,
                              OwnershipGiven BRArrayOf(BREthereumTransactionReceipt) receipts) {
    // Ensure we have a Block
    BREthereumBlock block = BRSetGet(bcs->blocks, &blockHash);
    if (NULL == block) {
        eth_log ("BCS", "Block %" PRIu64 " Missed (Receipts)", (NULL == block ? -1 : blockGetNumber(block)));
        bcsReleaseReceiptsFully(bcs, receipts);
        return;
    }

    // If the status has some how errored, skip out with nothing more to do.
    if (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusError(block))) {
        eth_log ("BCS", "Block %" PRIu64 " In Error (Receipts)", blockGetNumber(block));
        bcsReleaseReceiptsFully(bcs, receipts);
        return;
    }

    // We must be in a 'receipts needed' status
    assert (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusLogsRequest(block, BLOCK_REQUEST_PENDING)));

    eth_log("BCS", "Receipts %" PRIu64 " Count %zu",
            blockGetNumber(block),
            array_count(receipts));

    BREthereumLog *neededLogs = NULL;
    BREthereumHash emptyHash = EMPTY_HASH_INIT;

    // We do not ever necessarily have corresponding transactions at this point.  We'll process
    // the logs as best we can and then, in blockLinkLogsWithTransactions(), complete processing.
    size_t logIndexInBlock = 0;
    size_t receiptsCount = array_count(receipts);
    for (size_t ti = 0; ti < receiptsCount; ti++) { // transactionIndex
        BREthereumTransactionReceipt receipt = receipts[ti];
        if (ETHEREUM_BOOLEAN_IS_TRUE (transactionReceiptMatch(receipt, bcs->filterForAddressOnLogs))) {
            size_t logsCount = transactionReceiptGetLogsCount(receipt);
            for (size_t li = 0; li < logsCount; li++) { // logIndex
                BREthereumLog log = transactionReceiptGetLog(receipt, li);

                // If `log` topics match our address....
                if (ETHEREUM_BOOLEAN_IS_TRUE (logMatchesAddress(log, bcs->address, ETHEREUM_BOOLEAN_TRUE))) {
                    eth_log("BCS", "Receipts %" PRIu64 " Found Log at (%zu, %zu)",
                            blockGetNumber(block), ti, li);

                    // We'll need a copy of the log as this log will be added to the
                    // transaction status - which must be distinct from the receipts.
                    log = logCopy(log);

                    // We must save `li`, it identifies this log amoung other logs in transaction.
                    // We won't have the transaction hash so we'll use an empty one.
                    logInitializeIdentifier(log, emptyHash, logIndexInBlock);

                    logSetStatus (log, transactionStatusCreateIncluded (blockGetHash(block),
                                                                        blockGetNumber(block),
                                                                        ti,
                                                                        blockGetTimestamp(block),
                                                                        gasCreate(0)));
                    
                    if (NULL == neededLogs) array_new(neededLogs, 3);
                    array_add(neededLogs, log);
                }

                logIndexInBlock += 1;

                // else are we intereted in contract matches?  To 'estimate Gas'?  If so, check
                // logic elsewhere to avoid excluding logs.
            }
        }
        else logIndexInBlock += transactionReceiptGetLogsCount (receipt);
    }

    // Use the cummulative gasUsed, in each receipt, to compute the gasUsed for each transaction.
    // Note that we compute gasUsed for each and every transaction, even if the transaction is not
    // one of ours - simply because we can't know our transactions here.  (We do know transactions
    // for our logs here, but only those transactions).
    BRArrayOf(BREthereumGas) gasUsedByTransaction;
    array_new (gasUsedByTransaction, receiptsCount);
    for (size_t ti = 0; ti < receiptsCount; ti++) {
        uint64_t gasUsed = (transactionReceiptGetGasUsed(receipts[ti]) -
                            (ti == 0 ? 0 : transactionReceiptGetGasUsed(receipts[ti-1])));
        array_add (gasUsedByTransaction, gasCreate (gasUsed));
    }

    bcsReleaseReceiptsFully(bcs, receipts);

    // Report the block status - we'll flag as BLOCK_REQUEST_COMPLETE (even if none of interest).
    // The `block` now owners `neededLogs`
    blockReportStatusLogs(block, neededLogs);
    // And report the gasUsed per transaction
    blockReportStatusGasUsed(block, gasUsedByTransaction);

    if (NULL != neededLogs) {
        // If we have any logs, then we'll need additional data:

        // 1) get transactions (block bodies).  We might have them already - if so, use them; if
        //    not, request them.
        if (ETHEREUM_BOOLEAN_IS_TRUE (blockHasStatusTransactionsRequest(block, BLOCK_REQUEST_NOT_NEEDED))) {
            blockReportStatusTransactionsRequest (block, BLOCK_REQUEST_PENDING);
            lesProvideBlockBodiesOne (bcs->les, node,
                                      (BREthereumLESProvisionContext) bcs,
                                      (BREthereumLESProvisionCallback) bcsSignalProvision,
                                      blockGetHash(block));
            // eth_log("BCS", "Block %" PRIu64 " Needs Bodies (for Logs)", blockGetNumber(block));
        }

        // 2) We want a header proof too; to ensure a valid block w/ transactions + logs.
        if (ETHEREUM_BOOLEAN_IS_TRUE (blockHasStatusHeaderProofRequest(block, BLOCK_REQUEST_NOT_NEEDED))) {
            blockReportStatusHeaderProofRequest (block, BLOCK_REQUEST_PENDING);
            lesProvideBlockProofsOne (bcs->les, node,
                                      (BREthereumLESProvisionContext) bcs,
                                      (BREthereumLESProvisionCallback) bcsSignalProvision,
                                      blockGetNumber(block));
        }
    }

    if (ETHEREUM_BOOLEAN_IS_TRUE(blockHasStatusTransactionsRequest(block, BLOCK_REQUEST_COMPLETE)))
        blockLinkLogsWithTransactions (block); // gasUsedByTransaction has been used; still in status

    // In the following, 'if appropriate' means complete and chained.
    bcsExtendTransactionsAndLogsForBlockIfAppropriate (bcs, block);
}

static void
bcsHandleTransactionReceiptsMultiple (BREthereumBCS bcs,
                                      BREthereumNodeReference node,
                                      OwnershipGiven BRArrayOf(BREthereumHash) hashes,
                                      OwnershipGiven BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) arrayOfReceipts) {
    for (size_t index = 0; index < array_count(hashes); index++)
        // Each `arrayOfReceipts[index]` has 'OwnershipGiven'
        bcsHandleTransactionReceipts(bcs, node, hashes[index], arrayOfReceipts[index]);
    array_free (hashes);
    array_free (arrayOfReceipts);
}

/// MARK: - Transaction Status

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


/**
 *
 * TL;DR: We probably should use a 'consensus' algorithm; but we'll excuse ourselves.
 *
 * Handle a transaction's status.  We only request a status when a transaction (perhaps log) is
 * 'pending'; a transaction (perhaps log) becomes pending in two cases: a) the transaction is
 * submitted and b) the transaction when once 'included' becomes orphaned.
 *
 * A status is requested from *all* connected nodes.  Those nodes have been observed to report
 * nearly anything.  Specifically, some apparently busy nodes transition to QUEUED or PENDING
 * back to UNKNONWN, mysteriously.  Another node might progress to INCLUDED while the other nodes
 * are still in PENDING.  We also anticipate that some nodes may be broken (or worse malicious) and
 * report something randomish (always ERROR, always UNKNOWN, etc).  How to resolve the conflicts?
 *
 * Thankfully, the status *is not* the primary way to include a transaction.  The definitive
 * transaction status is determined solely by the chaining of blocks.  See the two functions:
 * `bcsHandleTransactionReceipts()` and `bcsHandleBlockBody()` and note that only with the
 * transaction receipts function can the 'gasUsed' be computed).
 *
 * So, we'll be a little lazy and not work too hard to resolve conflicts between multiple node.
 *
 * Still, we don't want the status bouncing around, scaring the User.  And, we do need a way to
 * move the transaction off of 'pending' so we stop requesting status updates.  To that end:
 *
 *  a) if a node reports a transactin as included, we'll remove the transaction from pending.
 *     Note: we'll only remove from pending if two back-to-back nodes report the same status.
 *
 *  b) if a node reports a transaction as errored (other than 'dropped'), we'll remove the
 *     transaction from pending and mark the transaction as ERRORED.  If is posslble that the
 *     transaction does get included in a subsequently announced block.  Having the tranaction
 *     subsequently included implies a race condition between nodes, I think.  Note: we'll only
 *     remote from pending if two back-to-back statuses report the same error.
 *
 *  c) if a node reports 'dropped' (the transaction was not UNKNOWN but is now UNKNOWN - farcical
 *     on its face) we'll ignore the UNKNOWN status.  The transaction stays pending, future status
 *     requests occur and we'll hopefully get better status info.  Or, the transaction will be
 *     part of an announced block.
 *
 * What does this imply from a User's perspective.  They might see a transaction in error that
 * comes back to life as INCLUDED.  Worse, they might see a transaction in error and try to
 * cancel or resubmit that transaction - hard to say if the cancel/resubmit would succeed as once
 * resubmitted some nodes would reject it.
 *
 * @param bcs bcs
 * @param node the node providing the status
 * @param transactionHash the hash of the transaction
 * @param status the transaction's status.
 */
static void
bcsHandleTransactionStatus (BREthereumBCS bcs,
                            BREthereumNodeReference node,
                            BREthereumHash transactionHash,
                            BREthereumTransactionStatus status) {
    // BCS owns `transaction`; be sure to copy if passed to any OwnershipGiven function.
    BREthereumTransaction transaction = BRSetGet(bcs->transactions, &transactionHash);
    if (NULL == transaction) return;

    // Get the hash string - soley for eth_log() output.
    BREthereumHashString hashString;
    hashFillString(transactionHash, hashString);

    // Boolean to indicate if we need to update the transaction's status;
    int needStatus = 1;

    // Get the current (aka 'old') status.
    BREthereumTransactionStatus oldStatus = transactionGetStatus(transaction);

    // Process the current status; compare to `oldStatus` as appropriate.
    switch (status.type) {

        case TRANSACTION_STATUS_UNKNOWN:
            // If the status is unknown, then `node` has *nothing* to offer; skip out.
            eth_log("BCS", "Transaction: \"%s\", Status: %d, Ignored", hashString, status.type);
            return;

        case TRANSACTION_STATUS_QUEUED:
            needStatus = TRANSACTION_STATUS_UNKNOWN == oldStatus.type;
            break;

        case TRANSACTION_STATUS_PENDING:
            needStatus = (TRANSACTION_STATUS_UNKNOWN == oldStatus.type ||
                          TRANSACTION_STATUS_QUEUED  == oldStatus.type);
            break;

        case TRANSACTION_STATUS_INCLUDED:
            // One node reported INCLUDED.  We'll immediately remove the node from 'pending' so
            // that no more queries are made.  We'll plan to update the transaction's status to
            // PENDING so that INCLUDED only occurs when a block w/ transaction is announced.
            //
            // We'll do this even if some other node reported ERROR.
            if (TRANSACTION_STATUS_INCLUDED == oldStatus.type) {
                needStatus = 0;
                bcsUnpendTransaction (bcs, transaction);
            }
            break;

        case TRANSACTION_STATUS_ERRORED:
            // If we got two errors back-to-back, then unpend the transaction.
            if (TRANSACTION_STATUS_ERRORED == oldStatus.type) {
                needStatus = 0;  // already in ERROR; don't report again.
                bcsUnpendTransaction (bcs, transaction);
            }
            break;
    }

    if (needStatus) {
        // Update the transaction status and signal
        transactionSetStatus (transaction, status);
        eth_log("BCS", "Transaction: \"%s\", Status: %d, Pending: %s%s%s",
                hashString,
                status.type,
                (-1 != bcsLookupPendingTransaction (bcs, transactionHash) ? "Yes" : "No"),
                (TRANSACTION_STATUS_ERRORED == status.type ? ", Error: " : ""),
                (TRANSACTION_STATUS_ERRORED == status.type ? transactionGetErrorName(status.u.errored.type) : ""));

        bcsSignalTransaction(bcs, transactionCopy(transaction));

        // Update any logs depending on transaction
        BRArrayOf(BREthereumLog) logs = bcsPendFindLogsByTransactionHash (bcs, transactionHash);
        if (NULL != logs) {
            for (size_t index = 0; index < array_count(logs); index++) {
                logSetStatus (logs[index], status);
                hashFillString (logGetHash(logs[index]), hashString);
                eth_log("BCS", "Log: \"%s\", Status: %d, Pending: %s%s%s",
                        hashString,
                        status.type,
                        (-1 != bcsLookupPendingTransaction (bcs, transactionHash) ? "Yes" : "No"),
                        (TRANSACTION_STATUS_ERRORED == status.type ? ", Error: " : ""),
                        (TRANSACTION_STATUS_ERRORED == status.type ? transactionGetErrorName(status.u.errored.type) : ""));
                bcsSignalLog (bcs, logs[index]);
            }
            array_free (logs);
        }
    }
}

static void
bcsHandleTransactionStatuses (BREthereumBCS bcs,
                              BREthereumNodeReference node,
                              OwnershipGiven BRArrayOf(BREthereumHash) hashes,
                              OwnershipGiven BRArrayOf(BREthereumTransactionStatus) statuses) {
    for (size_t index = 0; index < array_count(hashes); index++)
        bcsHandleTransactionStatus (bcs, node, hashes[index], statuses[index]);
    array_free (hashes);
    array_free (statuses);
}

//
// Periodicaly get the transaction status for all pending transaction.  The event will be NULL
// (as specified for a 'period dispatcher' - See `eventHandlerSetTimeoutDispatcher()`)
//
static void
bcsPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event) {
    BREthereumBCS bcs = (BREthereumBCS) event->context;

    // TODO: Avoid-ish a race condition on bcsRelease. This is the wrong approach.
    if (NULL == bcs->les) return;

    // If nothing to do; simply skip out.
    if ((NULL == bcs->pendingTransactions || 0 == array_count (bcs->pendingTransactions)) &&
        (NULL == bcs->pendingLogs         || 0 == array_count (bcs->pendingLogs)))
        return;

    // We'll request status for each `pendingTransaction`.
    BRArrayOf(BREthereumHash) hashes;
    array_new (hashes, array_count(bcs->pendingTransactions) + array_count(bcs->pendingLogs));
    array_add_array (hashes, bcs->pendingTransactions, array_count(bcs->pendingTransactions));

    // Add in hashes for each transaction referenced by `pendingLogs`
    for (size_t index = 0; index < array_count(bcs->pendingLogs); index++) {
        BREthereumHash logHash = bcs->pendingLogs[index];
        BREthereumLog  log     = BRSetGet (bcs->logs, &logHash);
        if (NULL != log) {
            BREthereumHash hash;
            if (ETHEREUM_BOOLEAN_IS_TRUE (logExtractIdentifier (log, &hash, NULL)) &&
                -1 == hashesIndex(hashes, hash))
                array_add (hashes, hash);
        }
    }

    // OwnershipGiven for `hashes` (hence, above, `hashes` is a new array).
    lesProvideTransactionStatus (bcs->les,
                                 NODE_REFERENCE_ALL,
                                 (BREthereumLESProvisionContext) bcs,
                                 (BREthereumLESProvisionCallback) bcsSignalProvision,
                                 hashes);
}

///
/// MARK - Handle Transaction, Log, Peers and Account(?)
///


/**
 * Handle a transaction by adding it to the set of BCS transactions and then announcing the
 * transaction with the listener's `transactionCallback`.
 *
 * NOTE: For implementation, this is the *only place* where `bcs->transactions` is extented w/ the
 * transaction and also the *only place* where the callback is invoked.  This method in invoked
 * when a tranaction: a) is submitted, b) is found in a block, c) is provided during BCS
 * initialization and d) has its status updated.  For a, b & c, the transaction will be added to
 * bcs->transactions.
 *
 * NOTE: ... also, because this in involded during BCS initialization, we might reconstitute a
 * transaction that is not yet included/errorred - that is, the transaction was saved while pending.
 * In such a case, we add `transaction` to `pendingTransactions`
 *
 * @param bcs bcs
 * @param transaction transaction
 */
extern void
bcsHandleTransaction (BREthereumBCS bcs,
                      OwnershipGiven BREthereumTransaction transaction) {
    int needUpdate = 1;
    int needRelease = 1;

    // We own transaction... but we pass it to transactionCallback. And we might pass it
    // up to three full times (actually at most two times, based on the detailed logic).  Since
    // transactionCallback is `OwnershipGiven` we'll need to copy transaction on each callback.

    // See if we have an existing transaction...
    BREthereumTransaction oldTransaction = BRSetGet(bcs->transactions, transaction);

    // ... if we don't, then add it and announce it as `ADDED`
    if (NULL == oldTransaction) {
        // BCS takes ownership of `transaction`; it must not be released; all subsequent uses copy.
        BRSetAdd(bcs->transactions, transaction);
        needRelease = 0;

        bcs->listener.transactionCallback (bcs->listener.context,
                                           BCS_CALLBACK_TRANSACTION_ADDED,
                                           transactionCopy(transaction));
        needUpdate = 0;
    }

    // ... otherwise, it is already known and we'll update it's status
    else transactionSetStatus (oldTransaction, transactionGetStatus(transaction));

    // Get the transaction status and handle appropriately.
    BREthereumTransactionStatus status = transactionGetStatus(transaction);

    switch (status.type) {
        case TRANSACTION_STATUS_ERRORED:
            // This is a 'final state' - no need for further processing
            break;

        case TRANSACTION_STATUS_INCLUDED:
            // This is nearly a 'final state' - except in the case where the transaction's block
            // has been orphaned.  We'll let the 'orphaning process' change the transaction's
            // status if need be - no need for further processing.
            break;

        default: {
            // When not in a final state (ERRORED, INCLUDED) we must query for the transaction's
            // status.  Thus we'll pend the transaction.  In most cases, the transaction is already
            // pending but, in the rare case where we are initializing BCS with transactions that
            // are not final, we'll actually need to pend them.
            bcsPendTransaction(bcs, transaction);
            break;
        }
    }

    // Announce as `UPDATED` (unless we announced ADDED).
    if (needUpdate)
        bcs->listener.transactionCallback (bcs->listener.context,
                                           BCS_CALLBACK_TRANSACTION_UPDATED,
                                           transactionCopy(transaction));

    if (needRelease)
        transactionRelease (transaction);
}

/**
 * Handle a log by adding it to the set of BCS logs and then announcing the log with the
 * listener's `logCallback`
 *
 * NOTE: For implementation, ...
 *
 * @param bcs bcs
 * @param log log
 */
extern void
bcsHandleLog (BREthereumBCS bcs,
              OwnershipGiven BREthereumLog log) {
    int needUpdate  = 1;
    int needRelease = 1;

    // See if we have an existing log...
    BREthereumLog oldLog = BRSetGet(bcs->logs, log);

    // ... if we don't, then add it and announce it as `ADDED`
    if (NULL == oldLog) {
        // BCS takes ownership of `log`; it must not be released; all subsequent uses copy.
        BRSetAdd(bcs->logs, log);
        needRelease = 0;

        bcs->listener.logCallback (bcs->listener.context,
                                   BCS_CALLBACK_LOG_ADDED,
                                   logCopy(log));
        needUpdate = 0;
    }

    // ... otherwise, it is already known and we'll update it's status
    else  logSetStatus(oldLog, logGetStatus(log));

    // Get the log status and handle appropriately.
    BREthereumTransactionStatus status = logGetStatus(log);

    switch (status.type) {
        case TRANSACTION_STATUS_ERRORED:
            // [See above in bcsHandleTransaction()]
            break;

        case TRANSACTION_STATUS_INCLUDED:
            // [See above in bcsHandleTransaction()]
            break;

        default: {
            // [See above in bcsHandleTransaction()]
            bcsPendLog (bcs, log);
            break;
        }
    }

    if (needUpdate)
        bcs->listener.logCallback (bcs->listener.context,
                                   BCS_CALLBACK_LOG_UPDATED,
                                   logCopy(log));

    if (needRelease)
        logRelease(log);
}

extern void
bcsHandleNodes (BREthereumBCS bcs,
                BRArrayOf(BREthereumNodeConfig) peers) {
    size_t peersCount = array_count(peers);
    bcs->listener.savePeersCallback (bcs->listener.context, peers);
    eth_log("BCS", "Peers %zu Saved", peersCount);
}

static void
bcsSyncReportBlocksCallback (BREthereumBCS bcs,
                             BREthereumBCSSync sync,
                             BREthereumNodeReference node,
                             OwnershipGiven BRArrayOf(BREthereumBCSSyncResult) results) {
    if (NULL == results) return;

    // Extract the result's header.
    size_t count = array_count(results);
    BRArrayOf(BREthereumBlockHeader) headers;
    array_new (headers, count);

    for (size_t index = 0; index < count; index++)
        array_add (headers, results[index].header);

    array_free(results);
    
    bcsHandleBlockHeaders (bcs, node, headers, 1);
}

static void
bcsSyncReportProgressCallback (BREthereumBCS bcs,
                               BREthereumBCSSync sync,
                               BREthereumNodeReference node,
                               uint64_t blockNumberBeg,
                               uint64_t blockNumberNow,
                               uint64_t blockNumberEnd) {

    BREthereumBCSCallbackSyncType type = (blockNumberNow == blockNumberBeg
                                          ? BCS_CALLBACK_SYNC_STARTED
                                          : (blockNumberNow == blockNumberEnd
                                             ? BCS_CALLBACK_SYNC_STOPPED
                                             : BCS_CALLBACK_SYNC_UPDATE));
    bcs->listener.syncCallback (bcs->listener.context,
                                type,
                                blockNumberBeg,
                                blockNumberNow,
                                blockNumberEnd);

    switch (type) {
        case BCS_CALLBACK_SYNC_UPDATE:
            break;
        case BCS_CALLBACK_SYNC_STARTED:
        case BCS_CALLBACK_SYNC_STOPPED:
            lesClean (bcs->les);
            break;
    }
}

extern void
bcsHandleProvision (BREthereumBCS bcs,
                    BREthereumLES les,
                    BREthereumNodeReference node,
                    OwnershipGiven BREthereumProvisionResult result) {
    assert (bcs->les == les);

    int needProvisionRelease = 1;
    BREthereumProvision *provision = &result.provision;
    switch (result.status) {
        case PROVISION_ERROR:

            // Try to handle the specific error.
            switch (result.u.error.reason) {
                case PROVISION_ERROR_NODE_INACTIVE:
                    // The node went inactive, we'll submit again.
                    //
                    // This can cause problems... as in, we are queryng a node based on a recent
                    // block but upon resubmission the other node is behind.
                    lesRetryProvision (bcs->les,
                                       NODE_REFERENCE_ANY,
                                       (BREthereumLESProvisionContext) bcs,
                                       (BREthereumLESProvisionCallback) bcsSignalProvision,
                                       provision);
                    needProvisionRelease = 0;

                    eth_log ("BCS", "Resubmitted Provision: %zu: %s",
                             provision->identifier,
                             provisionGetTypeName(provision->type));
                    break;

                default: break;
            }
            break;

        case PROVISION_SUCCESS: {
            assert (result.type == provision->type);
            switch (result.type) {
                case PROVISION_BLOCK_HEADERS: {
                    BRArrayOf(BREthereumBlockHeader) headers;
                    provisionHeadersConsume (&provision->u.headers, &headers);
                    bcsHandleBlockHeaders (bcs, node, headers, 0);
                    break;
                }

                case PROVISION_BLOCK_PROOFS: {
                    BRArrayOf(uint64_t) numbers;
                    BRArrayOf(BREthereumBlockHeaderProof) proofs;
                    provisionProofsConsume (&provision->u.proofs, &numbers, &proofs);
                    bcsHandleBlockProofs (bcs, node, numbers, proofs);
                    break;
                }
                    
                case PROVISION_BLOCK_BODIES: {
                    BRArrayOf(BREthereumHash) hashes;
                    BRArrayOf(BREthereumBlockBodyPair) pairs;
                    provisionBodiesConsume (&provision->u.bodies, &hashes, &pairs);
                    bcsHandleBlockBodies (bcs, node, hashes, pairs);
                    break;
                }

                case PROVISION_TRANSACTION_RECEIPTS: {
                    BRArrayOf(BREthereumHash) hashes;
                    BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) receipts;
                    provisionReceiptsConsume(&provision->u.receipts, &hashes, &receipts);
                    bcsHandleTransactionReceiptsMultiple (bcs, node, hashes, receipts);
                    break;
                }

                case PROVISION_ACCOUNTS: {
                    BRArrayOf(BREthereumHash) hashes;
                    BRArrayOf(BREthereumAccountState) accounts;
                    provisionAccountsConsume (&provision->u.accounts, &hashes, &accounts);
                    bcsHandleAccountStates (bcs,
                                            node,
                                            provision->u.accounts.address,
                                            hashes,
                                            accounts);
                    break;
                }

                case PROVISION_TRANSACTION_STATUSES: {
                    BRArrayOf(BREthereumHash) hashes;
                    BRArrayOf(BREthereumTransactionStatus) statuses;
                    provisionStatusesConsume (&provision->u.statuses, &hashes, &statuses);
                    bcsHandleTransactionStatuses (bcs, node, hashes, statuses);
                    break;
                }

                case PROVISION_SUBMIT_TRANSACTION: {
                    BREthereumTransaction transaction;
                    BREthereumTransactionStatus status;
                    provisionSubmissionConsume (&provision->u.submission, &transaction, &status);
                    bcsHandleTransactionStatus (bcs, node,
                                                transactionGetHash(transaction),
                                                status);
                    transactionRelease(transaction);
                    break;
                }
            }
            break;
        }
    }

    // We have one result and have finished with it.  Must release - which will release the
    // included provision.
    if (needProvisionRelease)
        provisionResultRelease (&result);
}

