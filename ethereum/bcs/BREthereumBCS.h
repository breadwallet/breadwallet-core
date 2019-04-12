//
//  BREthereumBCS.h
//  Core
//
//  Created by Ed Gamble on 5/24/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_BCS_h
#define BR_Ethereum_BCS_h

#include "ethereum/base/BREthereumBase.h"
#include "ethereum/les/BREthereumLES.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A Block Chain Slice
 *
 * A Block Chain Slice (BCS) represents a user account-centric view of the Ethereum
 * block chain.  A BCS maintains the User's transfers (both transactions and logs) for the User as
 * either the source or target of the transfer.  The maintained transfers must be valid; the
 * validation is performed by validating blocks in the block chain.
 *
 * BCS validates blocks in one or two ways.  First, if the block is relatively old, then Geth and
 * Parity nodes provide a 'Header Proof' message that returns a proof and the headers total
 * difficulty.  For Parity 'old' is 2048 block numbers in the past; for Geth 'old' is <what 1024>.
 * Second, if the block is relatively new, then BCS chains together individual blocks - to form a
 * block chain and to propagate the total difficulty - once each block has itself been validated.
 *
 * In the end, a transfer is valid of its containing block is valid; that holding no matter how
 * the block is validated - using a 'Header Proof' or a block chain.
 *
 * BCS uses a unique 'nary-search on account state changes w/ augmentation' to identify transfers.
 */
typedef struct BREthereumBCSStruct *BREthereumBCS;

//
// BCS Listener
//
typedef void* BREthereumBCSCallbackContext;

/**
 * The BCS block chain has been extended.
 */
typedef void
(*BREthereumBCSCallbackBlockchain) (BREthereumBCSCallbackContext context,
                                    BREthereumHash headBlockHash,
                                    uint64_t headBlockNumber,
                                    uint64_t headBlockTimestamp);


/**
 * The BCS account state has been updated.
 */
typedef void
(*BREthereumBCSCallbackAccountState) (BREthereumBCSCallbackContext context,
                                      BREthereumAccountState accountState);

/**
 * A BCS Transaction (for `account`) have been updated.
 */
typedef enum {
    BCS_CALLBACK_TRANSACTION_ADDED,
    BCS_CALLBACK_TRANSACTION_UPDATED,
    BCS_CALLBACK_TRANSACTION_DELETED
} BREthereumBCSCallbackTransactionType;

#define BCS_CALLBACK_TRANSACTION_TYPE_NAME( type ) \
  ((type) == BCS_CALLBACK_TRANSACTION_UPDATED \
    ? "Updated" \
    : ((type) == BCS_CALLBACK_TRANSACTION_ADDED \
        ? "Added" \
        : "Deleted"))

typedef void
(*BREthereumBCSCallbackTransaction) (BREthereumBCSCallbackContext context,
                                     BREthereumBCSCallbackTransactionType event,
                                     OwnershipGiven BREthereumTransaction transaction);

/**
 * A BCS Log (for `account`) has been updated.
 */
typedef enum {
    BCS_CALLBACK_LOG_ADDED,
    BCS_CALLBACK_LOG_UPDATED,
    BCS_CALLBACK_LOG_DELETED,
} BREthereumBCSCallbackLogType;

typedef void
(*BREthereumBCSCallbackLog) (BREthereumBCSCallbackContext context,
                             BREthereumBCSCallbackLogType event,
                             OwnershipGiven BREthereumLog log);

/**
 * Save Blocks
 */
typedef void
(*BREthereumBCSCallbackSaveBlocks) (BREthereumBCSCallbackContext context,
                                    OwnershipGiven BRArrayOf(BREthereumBlock) blocks);

/**
 * Save Peers
 */
typedef void
(*BREthereumBCSCallbackSavePeers) (BREthereumBCSCallbackContext context,
                                   OwnershipGiven BRArrayOf(BREthereumNodeConfig) peers);

/**
 * Sync
 */
typedef enum {
    BCS_CALLBACK_SYNC_STARTED,
    BCS_CALLBACK_SYNC_UPDATE,
    BCS_CALLBACK_SYNC_STOPPED
} BREthereumBCSCallbackSyncType;

typedef void
(*BREthereumBCSCallbackSync) (BREthereumBCSCallbackContext context,
                              BREthereumBCSCallbackSyncType event,
                              uint64_t blockNumberStart,
                              uint64_t blockNumberCurrent,
                              uint64_t blockNumberStop);

/**
 * Get (Interesting) Blocks.  Depending on the sync-mode we may ask the BRD endpoint for blocks
 * that contain transactions or logs of interest.
 */
typedef void
(*BREthereumBCSCallbackGetBlocks) (BREthereumBCSCallbackContext context,
                                   BREthereumAddress address,
                                   BREthereumSyncInterestSet interests,
                                   uint64_t blockStart,
                                   uint64_t blockStop);

typedef struct {
    BREthereumBCSCallbackContext context;
    BREthereumBCSCallbackBlockchain blockChainCallback;
    BREthereumBCSCallbackAccountState accountStateCallback;
    BREthereumBCSCallbackTransaction transactionCallback;
    BREthereumBCSCallbackLog logCallback;
    BREthereumBCSCallbackSaveBlocks saveBlocksCallback;
    BREthereumBCSCallbackSavePeers savePeersCallback;
    BREthereumBCSCallbackSync syncCallback;
    BREthereumBCSCallbackGetBlocks getBlocksCallback;
} BREthereumBCSListener;

/**
 * Create BCS (a 'BlockChain Slice`) providing a view of the Ethereum blockchain for `network`
 * focused on the `account` primary address.  Initialize the synchronization with the previously
 * saved `headers`.  Provide `listener` to anounce BCS 'events'.
 *
 * @parameters
 * @parameter headers - is this a BRArray; assume so for now.
 */
extern BREthereumBCS
bcsCreate (BREthereumNetwork network,
           BREthereumAddress address,
           BREthereumBCSListener listener,
           BREthereumMode syncMode,
           BRSetOf(BREthereumNodeConfig) peers,
           BRSetOf(BREthereumBlock) blocks,
           BRSetOf(BREthereumTransaction) transactions,
           BRSetOf(BREthereumLog) logs);

extern void
bcsStart (BREthereumBCS bcs);

extern void
bcsStop (BREthereumBCS bcs);

extern BREthereumBoolean
bcsIsStarted (BREthereumBCS bcs);

extern void
bcsDestroy (BREthereumBCS bcs);

/**
 * Scavenge BCS memory
 */
extern void
bcsClean (BREthereumBCS bcs);


/**
 * Start a sync from block number.  If a sync is in progress, then it is stopped.  This function
 * is typically invoked due to a) a User initiated re-sync, or b) the addition of a token - both
 * invocations are through ewmSync().
 *
 * Any sync that is started with a range that contains an included transfer (transaction or log)
 * will force transfer to 'pending'.  Then, as the sync progresses, the transfer will, presumably,
 * get re-included.
 *
 * @param bcs
 * @param blockNumber the block number (>= 0) to sync from
 */
extern void
bcsSync (BREthereumBCS bcs,
         uint64_t blockNumber);

extern BREthereumBoolean
bcsSyncInProgress (BREthereumBCS bcs);

/**
 * Submit `transaction` to the Ethereum Network
 *
 * @param bcs
 * @param transaction
 */
extern void
bcsSendTransaction (BREthereumBCS bcs,
                    BREthereumTransaction transaction);

/**
 * Request the status of `transaction` for the given block from the Ethereum Network
 *
 * @param bcs
 * @param transactionHash
 * @param blockNumber
 *@param blockTransactionIndex
 */
extern void
bcsSendTransactionRequest (BREthereumBCS bcs,
                           BREthereumHash transactionHash,
                           uint64_t blockNumber,
                           uint64_t blockTransactionIndex);

/**
 * Request the status of `log` for the given block from the Ethereum Network
 *
 * @param bcs
 * @param transactionHash
 * @param blockNumber
 * @param blockTransactionIndex 
 */
extern void
bcsSendLogRequest (BREthereumBCS bcs,
                   BREthereumHash transactionHash,
                   uint64_t blockNumber,
                   uint64_t blockTransactionIndex);

extern void
bcsReportInterestingBlocks (BREthereumBCS bcs,
                            // interest
                            // request id
                            BRArrayOf(uint64_t) blockNumbers);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_BCS_h */
