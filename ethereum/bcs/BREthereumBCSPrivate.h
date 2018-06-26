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
#include "../event/BREvent.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ACTIVE_BLOCK_PENDING_BODIES,
    ACTIVE_BLOCK_PENDING_RECEIPTS,
    ACTIVE_BLOCK_PENDING_ACCOUNT
} BREthereumBCSActiveBlockState;

typedef struct {
    BREthereumHash hash;
    BREthereumBlock block;
    BREthereumLog *logs;
    BREthereumAccountState accountState;
    BREthereumBCSActiveBlockState state;
} BREthereumBCSActiveBlock;

/**
 *
 */
struct BREthereumBCSStruct {

    /**
     * The network
     */
    BREthereumNetwork network;

    /**
     * The account
     */
    BREthereumAccount account;

    /**
     * The address is the account's primary address.  The 'slice' will focus on transactions
     * and logs for this address.
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
     * A BRSet holding block headers.  This is *every* header that we are interested in which will
     * be a small subset of all Ethereum headers.  This set contains:
     *    - the last N headers in `chain`
     *    - header checkpoints (including the genesis block)
     *    - headers containings transactions and logs of interest
     *    - orphaned headers
     */
    BRSet *headers;

    /**
     * A chain of block headers.  These are 'chained' by the `parentHash`.
     */
    BREthereumBlockHeader chain;

    /**
     * The block header at the tail of `chain`.  We will not have a block header for this
     * header's parent.
     */
    BREthereumBlockHeader chainTail;

    /**
     * A BRSet of orphaned block headers.  These are block headers that 'conflict' with
     * chained headers.  Typically (Exclusively) an orphan is a previously chained header
     * that was replaced by a subsequently accounced header.
     */
    BRSet *orphans;

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
    BREthereumHash *pendingTransactions;
    // TODO: pendingLogs

    /**
     * A BRSet of transactions for account.
     */
    BRSet *transactions;

    /**
     * A BRSet of logs for account.
     */
    BRSet *logs;

    /**
     * The Account State
     */
    BREthereumAccountState accountState;

    /**
     * The Array of Active Blocks
     */
    BREthereumBCSActiveBlock *activeBlocks;

    int syncActive;
    uint64_t syncTail;
    uint64_t syncNext;
    uint64_t syncHead;
};

extern const BREventType *bcsEventTypes[];
extern const unsigned int bcsEventTypesCount;

#define FOR_SET(type,var,set) \
    for (type var = BRSetIterate(set, NULL); \
         NULL != var; \
         var = BRSetIterate(set, var))

#define BCS_FOR_HEADERS(header)  FOR_SET(BREthereumBlockHeader, header, bcs->headers)
//    for (BREthereumBlockHeader header = BRSetIterate (bcs->headers, NULL); \
//         NULL != header; \
//         header = BRSetIterate (bcs->headers, header))

//
// Submit Transaction
//
extern void
bcsSignalSubmitTransaction (BREthereumBCS bcs,
                            BREthereumTransaction transaction);

extern void
bcsHandleSubmitTransaction (BREthereumBCS bcs,
                            BREthereumTransaction transaction);

//
// Announce
//
extern void
bcsSignalAnnounce (BREthereumBCS bcs,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   uint64_t headTotalDifficulty);

extern void
bcsHandleAnnounce (BREthereumBCS bcs,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   uint64_t headTotalDifficulty);

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
// Transactions
//
extern void
bcsHandleTransaction (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transaction);

extern void
bcsSignalTransaction (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transaction);

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
                       BREthereumHash blockHash,
                       BREthereumAddress address,
                       BREthereumAccountState state);
    
extern void
bcsSignalAccountState (BREthereumBCS bcs,
                       BREthereumHash blockHash,
                       BREthereumAddress address,
                       BREthereumAccountState state);

//
// Logs
//
extern void
bcsHandleLog (BREthereumBCS bcs,
              BREthereumHash blockHash,
              BREthereumHash transactionHash,
              BREthereumLog log);

extern void
bcsSignalLog (BREthereumBCS bcs,
              BREthereumHash blockHash,
              BREthereumHash transactionHash,
              BREthereumLog log);

//
// Active Block
//
extern BREthereumBCSActiveBlock *
bcsLookupActiveBlock (BREthereumBCS bcs,
                      BREthereumHash hash);

extern void
bcsReleaseActiveBlock (BREthereumBCS bcs,
                       BREthereumHash hash);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_BCS_Private_h */
