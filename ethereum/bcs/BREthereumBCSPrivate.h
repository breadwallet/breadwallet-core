//
//  BREthereumBCSPrivate.h
//  BRCore
//
//  Created by Ed Gamble on 5/24/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_BCS_Private_h
#define BR_Ethereum_BCS_Private_h

#include "ethereum/blockchain/BREthereumBlockChain.h"
#include "ethereum/event/BREvent.h"
#include "BREthereumBCS.h"

#ifdef __cplusplus
extern "C" {
#endif

/// MARK: - Sync Defines

#define LES_GET_HEADERS_MAXIMUM        (192)

/**
 * For a 'N_ARY sync' we'll split the range (of needed blockNumbers) into sub-ranges.  We'll find
 * the optimum number of subranges (such that the subranges exactly span the parent range). We'll
 * limit the number of subranges to between MINIMUM and MAXIMUM (below).  The maximum is determined
 * by the maximum in LES GetBlockHeaders; the minimum is arbitrary.
 *
 * Say we have 6,000,0xx headers and the request minimum/maximum are 100/200.  We'll find the
 * highest request count with the minimum remainder.  For 100 and 200 the remainder is 'xx'; we'd
 * choose 200 as it is the highest.  (The actual request count will depend on 'xx' - e.g for
 * headers of 6083022, the optimal count is 159).  For the remainder, we'll do a SYNC_LINEAR_SMALL.
 *
 * With 200, we'll issue GetAccountState at the 201 headers, each header 30,000 apart.  If the
 * AccountState change, we'll recurse.  The highest minimal remainder is again 200 with blocks
 * spaced 150 apart.
 */
#define SYNC_N_ARY_REQUEST_MINIMUM     (100)
#define SYNC_N_ARY_REQUEST_MAXIMUM     (LES_GET_HEADERS_MAXIMUM - 1)

/**
 * For a 'linear sync' we'll request at most MAXIMUM headers.  The maximum is determined by the
 * maximum in LES GetBlockHeaders.
 *
 * We'll favor a 'linear sync' over a 'N_ARY sync' if the range (of needed blockNumbers) is less
 * than LIMIT (below)
 */
#define SYNC_LINEAR_REQUEST_MAXIMUM     (LES_GET_HEADERS_MAXIMUM - 1)
#define SYNC_LINEAR_LIMIT               (10 * SYNC_LINEAR_REQUEST_MAXIMUM)
#define SYNC_LINEAR_LIMIT_IF_N_ARY      (100) // 3 * SYNC_LINEAR_REQUEST_MAXIMUM)

/**
 * As the sync find results (block headers, at least) we'll report them every PERIOD results.
 */
#define BCS_SYNC_RESULT_PERIOD  250

/**
 *
 */
typedef struct BREthereumBCSSyncStruct *BREthereumBCSSync;

/// MARK: - typedef BCS

//
// BCS
//
struct BREthereumBCSStruct {

    /**
     * The network
     */
    BREthereumNetwork network;

    /**
     * The address.  The 'slice' will focus on transactions and logs for this address.
     */
    BREthereumAddress address;

    /**
     * The sync mode
     */
    BREthereumMode mode;
    
    /**
     * A BloomFilter with address for application to transactions
     */
    BREthereumBloomFilter filterForAddressOnTransactions;

    /**
     * A BloomFilter with address for application to logs.  For logs, the bloom filter is based
     * on matching `LogTopic` data.
     */
    BREthereumBloomFilter filterForAddressOnLogs;

    /**
     * The listener interested in BCS events
     */
    BREthereumBCSListener listener;

    /**
     * The LES functionality - provides 'quality' resutls to LES/2 queries.  Manages
     * multiple peer nodes
     */
    BREthereumLES les;

    /**
     * Our event handler
     */
    BREventHandler handler;

    /**
     * The Genesis Block.
     */
    BREthereumBlock genesis;

    /**
     * A BRSet holding blocks.  This is *every* block that we are interested in which will
     * be a small subset of all Ethereum block.  This set contains:
     *    - the last N blocks in `chain`
     *    - block checkpoints (including the genesis block)
     *    - blocks containing transactions and logs of interest
     *    - orphaned block
     * Some of these block will be 'minimal' - headers not fully reconstituted and w/o ommers
     * nor transactions.
     *
     * Note: Why keep blocks rather then blockHeaders?  We need to associate Logs with the
     * block (or blockHeader) that generated the Log - we chose block.
     */
    BRSetOf(BREthereumBlock) blocks;

    /**
     * A chain of blocks.  These are 'chained' by the `parentHash` using the block's `next` field.
     */
    BREthereumBlock chain;

    /**
     * The block at the tail of `chain`.  We will not have a block for this header's parent and
     * the block's `next` field will be BLOCK_NEXT_NULL.
     */
    BREthereumBlock chainTail;

    /**
     * A BRSet of orphaned block headers.  These are block headers that 'conflict' with
     * chained headers.  An orphan is a previously chained header that was replaced by a
     * subsequently accounced header or, importantly, a header beyond `chain` such as when
     * a new header is announced but a sync is in progress.  Such 'beyond' headers will get chained
     * once the sync is up-to-date.
     */
    BRSetOf(BREthereumBlock) orphans;

    /**
     * A BRArray of hashes for pending transactions.  A transaction is 'pending' if it's
     * status is not 'INCLUDED' nor 'ERRORED'.  When pending, BCS will periodically (see
     * BCS_TRANSACTION_CHECK_STATUS_SECONDS) issue a batched lesGetTransactionStatus() call
     * to get a status update.
     *
     * TODO: Need to clarify how an 'INCLUDED' status interacts with block header chaining.  That
     * is, we might see 'INCLUDED' but not yet know about the block.  Presumably we do not
     * announe the transaction to the `listener`.  Similarly we could see the block, chained or
     * orphaned, but not have the status.
     *
     * This is an array of hashes rather then a set of transactions to faclitate the call to
     * lesGetTransactionStatus().
     *
     * I think we keep a transaction pending, even when INCLUDED, until its block is chained.  Thus
     * we continue asking for status.
     */
    BRArrayOf(BREthereumHash) pendingTransactions;
    BRArrayOf(BREthereumHash) pendingLogs;

    /**
     * A BRSet of transactions for account.  This includes any and all transactions that we've
     * ever identified/created for account no matter the transaction's status.  Specifically,
     * transactions that have ERRORRED, transactions that have been replaced, transactions that
     * have been created but never signed nor submitted - they are all on the list.  The User of
     * this set is responsible for filtering the transactions as needed.
     */
    BRSetOf(BREthereumTransaction) transactions;

    /**
     * A BRSet of logs for account.  This includes any and all logs that we've ever
     * identified/created for account no matter the log's status.  [See above for `transactions`]
     *
     * As this is a BRSet and a log's hash is the set identifier, every log in this set must be
     * included?
     */
    BRSetOf(BREtherumLog) logs;

    /**
     * The Account State
     */
    uint64_t accountStateBlockNumber;
    BREthereumAccountState accountState;

    /**
     * Sync state
     */
    BREthereumBCSSync sync;

    /**
     * Proof of Work
     */
    BREthereumProofOfWork pow;
};

extern const BREventType *bcsEventTypes[];
extern const unsigned int bcsEventTypesCount;

#define BCS_FOR_BLOCK(block)  FOR_SET(BREthereumBlock, block, bcs->blocks)

#define BCS_FOR_CHAIN(bcs, block)            \
    for (BREthereumBlock block = bcs->chain; \
         NULL != block;                      \
         block = blockGetNext(block))

//
// Submit Transaction
//
extern void
bcsHandleSubmitTransaction (BREthereumBCS bcs,
                            OwnershipGiven BREthereumTransaction transaction);

extern void
bcsSignalSubmitTransaction (BREthereumBCS bcs,
                            OwnershipGiven BREthereumTransaction transaction);

//
// Announce (New Chain Head)
//
extern void
bcsHandleAnnounce (BREthereumBCS bcs,
                   BREthereumNodeReference node,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   UInt256 headTotalDifficulty,
                   uint64_t reorgDepth);

extern void
bcsSignalAnnounce (BREthereumBCS bcs,
                   BREthereumNodeReference node,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   UInt256 headTotalDifficulty,
                   uint64_t reorgDepth);

//
// Status (of a LES Node)
//
extern void
bcsHandleStatus (BREthereumBCS bcs,
                 BREthereumNodeReference node,
                 BREthereumHash headHash,
                 uint64_t headNumber);

extern void
bcsSignalStatus (BREthereumBCS bcs,
                 BREthereumNodeReference node,
                 BREthereumHash headHash,
                 uint64_t headNumber);

//
// Provision (Result Data)
//
extern void
bcsHandleProvision (BREthereumBCS bcs,
                    BREthereumLES les,
                    BREthereumNodeReference node,
                    BREthereumProvisionResult result);

extern void
bcsSignalProvision (BREthereumBCS bcs,
                    BREthereumLES les,
                    BREthereumNodeReference node,
                    BREthereumProvisionResult result);

//
// Transactions
//
extern void
bcsHandleTransaction (BREthereumBCS bcs,
                      OwnershipGiven BREthereumTransaction transaction);

extern void
bcsSignalTransaction (BREthereumBCS bcs,
                      OwnershipGiven BREthereumTransaction transaction);

//
// Logs
//
extern void
bcsHandleLog (BREthereumBCS bcs,
              BREthereumLog log);

extern void
bcsSignalLog (BREthereumBCS bcs,
              BREthereumLog log);

//
// Peers
//
extern void
bcsHandleNodes (BREthereumBCS bcs,
                BRArrayOf(BREthereumNodeConfig) peers);

extern void
bcsSignalNodes (BREthereumBCS bcs,
                BRArrayOf(BREthereumNodeConfig) peers);

//
// Sync
//
typedef struct BREthereumBCSSyncRangeRecord *BREthereumBCSSyncRange;

extern BREventHandler
bcsSyncRangeGetHandler (BREthereumBCSSyncRange range);
    
typedef struct {
    BREthereumBlockHeader header;
    // account state
    // needBodies
    // needReceipts
} BREthereumBCSSyncResult;

typedef void* BREthereumBCSSyncContext;

typedef void
(*BREthereumBCSSyncReportBlocks) (BREthereumBCSSyncContext context,
                                  BREthereumBCSSync sync,
                                  BREthereumNodeReference node,
                                  BRArrayOf(BREthereumBCSSyncResult) blocks);

typedef void
(*BREthereumBCSSyncReportProgress) (BREthereumBCSSyncContext context,
                                    BREthereumBCSSync sync,
                                    BREthereumNodeReference node,
                                    uint64_t blockNumberBeg,
                                    uint64_t blockNumberNow,
                                    uint64_t blockNumberEnd);

extern BREthereumBCSSync
bcsSyncCreate (BREthereumBCSSyncContext context,
               BREthereumBCSSyncReportBlocks callbackBlocks,
               BREthereumBCSSyncReportProgress callbackProgress,
               BREthereumAddress address,
               BREthereumLES les,
               BREventHandler handler);

extern void
bcsSyncRelease (BREthereumBCSSync sync);

extern BREthereumBoolean
bcsSyncIsActive (BREthereumBCSSync sync);

extern void
bcsSyncStop (BREthereumBCSSync sync);

extern void
bcsSyncStart (BREthereumBCSSync sync,
              BREthereumNodeReference node,
              uint64_t thisBlockNumber,
              uint64_t needBlockNumber);

extern void
bcsSyncHandleProvision (BREthereumBCSSyncRange range,
                        BREthereumLES les,
                        BREthereumNodeReference node,
                        BREthereumProvisionResult result);

extern void
bcsSyncSignalProvision (BREthereumBCSSyncRange range,
                        BREthereumLES les,
                        BREthereumNodeReference node,
                        BREthereumProvisionResult result);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_BCS_Private_h */
