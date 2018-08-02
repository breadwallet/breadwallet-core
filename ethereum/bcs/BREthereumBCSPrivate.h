//
//  BREthereumBCSPrivate.h
//  BRCore
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

#ifndef BR_Ethereum_BCS_Private_h
#define BR_Ethereum_BCS_Private_h

#include "BRSet.h"
#include "BRArray.h"
#include "BREthereumBCS.h"
#include "../blockchain/BREthereumBlockChain.h"
#include "../les/BREthereumNode.h"
#include "../event/BREvent.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREthereumBCSSyncStruct *BREthereumBCSSync;

/**
 *
 */
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
     * A BloomFilter with address for application to transactions
     */
    BREthereumBloomFilter filterForAddressOnTransactions;

    /**
     * A BlookFilter with address for application to logs.  For logs, the bloom filter is based
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
     * chained headers.  Typically (Exclusively) an orphan is a previously chained header
     * that was replaced by a subsequently accounced header.
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
     * A BRSet of transactions for account.
     */
    BRSetOf(BREthereumTransaction) transactions;

    /**
     * A BRSet of logs for account.
     */
    BRSetOf(BREtherumLog) logs;

    /**
     * The Account State
     */
    BREthereumAccountState accountState;

    /**
     * The Array of Active Blocks
     */
    BRArrayOf(BREthereumBlock) activeBlocks;

    /**
     * Sync state
     */
    BREthereumBCSSync sync;
//    int syncActive;
//    uint64_t syncTail;
//    uint64_t syncNext;
//    uint64_t syncHead;
};

extern const BREventType *bcsEventTypes[];
extern const unsigned int bcsEventTypesCount;

#define FOR_SET(type,var,set) \
    for (type var = BRSetIterate(set, NULL); \
         NULL != var; \
         var = BRSetIterate(set, var))

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
                                BREthereumTransaction transaction);

extern void
bcsSignalSubmitTransaction (BREthereumBCS bcs,
                            BREthereumTransaction transaction);

//
// Announce (New Chain Head)
//
extern void
bcsHandleAnnounce (BREthereumBCS bcs,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   UInt256 headTotalDifficulty,
                   uint64_t reorgDepth);

extern void
bcsSignalAnnounce (BREthereumBCS bcs,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   UInt256 headTotalDifficulty,
                   uint64_t reorgDepth);

//
// Block Headers
//
extern void
bcsHandleBlockHeader (BREthereumBCS bcs,
                      BREthereumBlockHeader header);

extern void
bcsSignalBlockHeader (BREthereumBCS bcs,
                      BREthereumBlockHeader header);

//
// Block Bodies
//
extern void
bcsHandleBlockBodies (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transactions[],
                      BREthereumBlockHeader ommers[]);

extern void
bcsSignalBlockBodies (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transactions[],
                      BREthereumBlockHeader ommers[]);

//
// Transaction Status
//
extern void
bcsHandleTransactionStatus (BREthereumBCS bcs,
                            BREthereumHash transactionHash,
                            BREthereumTransactionStatus status);

extern void
bcsSignalTransactionStatus (BREthereumBCS bcs,
                            BREthereumHash transactionHash,
                            BREthereumTransactionStatus status);

//
// Transaction Receipt
//
extern void
bcsHandleTransactionReceipts (BREthereumBCS bcs,
                              BREthereumHash blockHash,
                              BREthereumTransactionReceipt receipts[]);

extern void
bcsSignalTransactionReceipts (BREthereumBCS bcs,
                              BREthereumHash blockHash,
                              BREthereumTransactionReceipt receipts[]);

//
// Account State
//
extern void
bcsHandleAccountState (BREthereumBCS bcs,
                       BREthereumLESAccountStateResult result);

extern void
bcsSignalAccountState (BREthereumBCS bcs,
                   BREthereumLESAccountStateResult result);

//
// Transactions
//
extern void
bcsHandleTransaction (BREthereumBCS bcs,
                      BREthereumTransaction transaction);

extern void
bcsSignalTransaction (BREthereumBCS bcs,
                      BREthereumTransaction transaction);

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
bcsHandlePeers (BREthereumBCS bcs,
                BRArrayOf(BREthereumPeerConfig) peers);

extern void
bcsSignalPeers (BREthereumBCS bcs,
                BRArrayOf(BREthereumPeerConfig) peers);

////
//// Active Block
////
//extern BREthereumBlock
//bcsLookupActiveBlock (BREthereumBCS bcs,
//                      BREthereumHash hash);
//
//extern void
//bcsReleaseActiveBlock (BREthereumBCS bcs,
//                       BREthereumHash hash);


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
                                  BRArrayOf(BREthereumBCSSyncResult) blocks);

typedef void
(*BREthereumBCSSyncReportProgress) (BREthereumBCSSyncContext context,
                                    BREthereumBCSSync sync,
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

extern BREventHandler
bcsSyncHandler (BREthereumBCSSync sync);

extern BREthereumBoolean
bcsSyncIsActive (BREthereumBCSSync sync);

extern int
bcsSyncExtractRange (BREthereumBCSSync sync,
                     uint64_t *blockNumberBeg,
                     uint64_t *blockNumberEnd);

extern void
bcsSyncStart (BREthereumBCSSync sync);

extern void
bcsSyncStop (BREthereumBCSSync sync);

extern void
bcsSyncContinue (BREthereumBCSSync sync,
                 uint64_t thisBlockNumber,
                 uint64_t needBlockNumber);

extern void
bcsSyncHandleBlockHeader (BREthereumBCSSyncRange request,
                          BREthereumBlockHeader header);

extern void
bcsSyncSignalBlockHeader (BREthereumBCSSyncRange request,
                          BREthereumBlockHeader header);


extern void
bcsSyncHandleAccountState (BREthereumBCSSyncRange request,
                           BREthereumLESAccountStateResult result);

extern void
bcsSyncSignalAccountState (BREthereumBCSSyncRange request,
                           BREthereumLESAccountStateResult result);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_BCS_Private_h */
