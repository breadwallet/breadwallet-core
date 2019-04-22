//
//  BRWalletManager.c
//  BRCore
//
//  Created by Ed Gamble on 11/21/18.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "BRArray.h"
#include "BRSet.h"
#include "BRWalletManager.h"
#include "BRPeerManager.h"
#include "BRMerkleBlock.h"
#include "BRBase58.h"
#include "BRChainParams.h"
#include "bcash/BRBCashParams.h"

#include "support/BRFileService.h"
#include "ethereum/event/BREvent.h"
#include "ethereum/event/BREventAlarm.h"

#define BWM_SLEEP_SECONDS       (1 * 60)                  // 5 minutes

// When using a BRD sync, offset the start block by N days of Bitcoin blocks; the value of N is
// assumed to be 'the maximum number of days that the blockchain DB could be behind'
#define BWM_MINUTES_PER_BLOCK                   10              // assumed, bitcoin
#define BWM_BRD_SYNC_DAYS_OFFSET                 1
#define BWM_BRD_SYNC_START_BLOCK_OFFSET        ((BWM_BRD_SYNC_DAYS_OFFSET * 24 * 60) / BWM_MINUTES_PER_BLOCK)

/* Forward Declarations */
static void
bwmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);

extern const BREventType *bwmEventTypes[];
extern const unsigned int bwmEventTypesCount;

static void _BRWalletManagerBalanceChanged (void *info, uint64_t balanceInSatoshi);
static void _BRWalletManagerTxAdded   (void *info, BRTransaction *tx);
static void _BRWalletManagerTxUpdated (void *info, const UInt256 *hashes, size_t count, uint32_t blockHeight, uint32_t timestamp);
static void _BRWalletManagerTxDeleted (void *info, UInt256 hash, int notifyUser, int recommendRescan);

static void _BRWalletManagerSyncStarted (void *info);
static void _BRWalletManagerSyncStopped (void *info, int reason);
static void _BRWalletManagerTxStatusUpdate (void *info);
static void _BRWalletManagerSaveBlocks (void *info, int replace, BRMerkleBlock **blocks, size_t count);
static void _BRWalletManagerSavePeers  (void *info, int replace, const BRPeer *peers, size_t count);
static int  _BRWalletManagerNetworkIsReachabele (void *info);
static void _BRWalletManagerThreadCleanup (void *info);

static const char *
getNetworkName (const BRChainParams *params) {
    if (params->magicNumber == BRMainNetParams->magicNumber ||
        params->magicNumber == BRBCashParams->magicNumber)
        return "mainnet";

    if (params->magicNumber == BRTestNetParams->magicNumber ||
        params->magicNumber == BRBCashTestNetParams->magicNumber)
        return "testnet";

    return NULL;
}

static const char *
getCurrencyName (const BRChainParams *params) {
    if (params->magicNumber == BRMainNetParams->magicNumber ||
        params->magicNumber == BRTestNetParams->magicNumber)
        return "btc";

    if (params->magicNumber == BRBCashParams->magicNumber ||
        params->magicNumber == BRBCashTestNetParams->magicNumber)
        return "bch";

    return NULL;
}

static BRWalletForkId
getForkId (const BRChainParams *params) {
    if (params->magicNumber == BRMainNetParams->magicNumber ||
        params->magicNumber == BRTestNetParams->magicNumber)
        return WALLET_FORKID_BITCOIN;

    if (params->magicNumber == BRBCashParams->magicNumber ||
        params->magicNumber == BRBCashTestNetParams->magicNumber)
        return WALLET_FORKID_BITCASH;

    return (BRWalletForkId) -1;
}

/// MARK: - BRWalletManager

struct BRWalletManagerStruct {

    /** The mode */
    BRSyncMode mode;

    /** The wallet */
    BRWallet *wallet;

    /** The peer manager */
    BRPeerManager  *peerManager;

    /** The client */
    BRWalletManagerClient client;

    /** The file service */
    BRFileService fileService;

    /**
     * The BlockHeight is the largest block number seen
     */
    uint64_t blockHeight;

    /**
     * An identiifer for a BRD Request
     */
    unsigned int requestId;

    /**
     * An EventHandler for Main.  All 'announcements' (via PeerManager (or BRD) hit here.
     */
    BREventHandler handler;

    /**
     * The Lock ensuring single thread access to BWM state.
     */
    pthread_mutex_t lock;

    /**
     * If we are syncing with BRD, instead of as P2P with PeerManager, then we'll keep a record to
     * ensure we've successfully completed the getTransactions() callbacks to the client.
     */
    struct {
        uint64_t begBlockNumber;
        uint64_t endBlockNumber;

        int rid;

        int completed:1;
    } brdSync;
};

/// MARK: - Transaction File Service

static const char *fileServiceTypeTransactions = "transactions";

enum {
    WALLET_MANAGER_TRANSACTION_VERSION_1
};

static UInt256
fileServiceTypeTransactionV1Identifier (BRFileServiceContext context,
                                        BRFileService fs,
                                        const void *entity) {
    const BRTransaction *transaction = entity;
    return transaction->txHash;
}

static uint8_t *
fileServiceTypeTransactionV1Writer (BRFileServiceContext context,
                                    BRFileService fs,
                                    const void* entity,
                                    uint32_t *bytesCount) {
    const BRTransaction *transaction = entity;

    size_t txTimestampSize  = sizeof (uint32_t);
    size_t txBlockHeightSize = sizeof (uint32_t);
    size_t txSize = BRTransactionSerialize (transaction, NULL, 0);

    assert (txTimestampSize   == sizeof(transaction->timestamp));
    assert (txBlockHeightSize == sizeof(transaction->blockHeight));

    *bytesCount = (uint32_t) (txSize + txBlockHeightSize + txTimestampSize);

    uint8_t *bytes = calloc (*bytesCount, 1);

    size_t bytesOffset = 0;

    BRTransactionSerialize (transaction, &bytes[bytesOffset], txSize);
    bytesOffset += txSize;

    UInt32SetLE (&bytes[bytesOffset], transaction->blockHeight);
    bytesOffset += txBlockHeightSize;

    UInt32SetLE(&bytes[bytesOffset], transaction->timestamp);

    return bytes;
}

static void *
fileServiceTypeTransactionV1Reader (BRFileServiceContext context,
                                    BRFileService fs,
                                    uint8_t *bytes,
                                    uint32_t bytesCount) {
    size_t txTimestampSize  = sizeof (uint32_t);
    size_t txBlockHeightSize = sizeof (uint32_t);

    BRTransaction *transaction = BRTransactionParse (bytes, bytesCount);
    if (NULL == transaction) return NULL;

    transaction->blockHeight = UInt32GetLE (&bytes[bytesCount - txTimestampSize - txBlockHeightSize]);
    transaction->timestamp   = UInt32GetLE (&bytes[bytesCount - txTimestampSize]);

    return transaction;
}

static BRArrayOf(BRTransaction*)
initialTransactionsLoad (BRWalletManager manager) {
    BRSetOf(BRTransaction*) transactionSet = BRSetNew(BRTransactionHash, BRTransactionEq, 100);
    if (1 != fileServiceLoad (manager->fileService, transactionSet, fileServiceTypeTransactions, 1)) {
        BRSetFree(transactionSet);
        return NULL;
    }

    size_t transactionsCount = BRSetCount(transactionSet);

    BRArrayOf(BRTransaction*) transactions;
    array_new (transactions, transactionsCount);
    array_set_count(transactions, transactionsCount);

    BRSetAll(transactionSet, (void**) transactions, transactionsCount);
    BRSetFree(transactionSet);

    return transactions;
}

/// MARK: - Block File Service

static const char *fileServiceTypeBlocks = "blocks";
enum {
    WALLET_MANAGER_BLOCK_VERSION_1
};

static UInt256
fileServiceTypeBlockV1Identifier (BRFileServiceContext context,
                                  BRFileService fs,
                                  const void *entity) {
    const BRMerkleBlock *block = (BRMerkleBlock*) entity;
    return block->blockHash;
}

static uint8_t *
fileServiceTypeBlockV1Writer (BRFileServiceContext context,
                              BRFileService fs,
                              const void* entity,
                              uint32_t *bytesCount) {
    const BRMerkleBlock *block = entity;

    // The serialization of a block does not include the block height.  Thus, we'll need to
    // append the height.

    // These are serialization sizes
    size_t blockHeightSize = sizeof (uint32_t);
    size_t blockSize = BRMerkleBlockSerialize(block, NULL, 0);

    // Confirm.
    assert (blockHeightSize == sizeof (block->height));

    // Update bytesCound with the total of what is written.
    *bytesCount = (uint32_t) (blockSize + blockHeightSize);

    // Get our bytes
    uint8_t *bytes = calloc (*bytesCount, 1);

    // We'll serialize the block itself first
    BRMerkleBlockSerialize(block, bytes, blockSize);

    // And then the height.
    UInt32SetLE(&bytes[blockSize], block->height);

    return bytes;
}

static void *
fileServiceTypeBlockV1Reader (BRFileServiceContext context,
                              BRFileService fs,
                              uint8_t *bytes,
                              uint32_t bytesCount) {
    size_t blockHeightSize = sizeof (uint32_t);

    BRMerkleBlock *block = BRMerkleBlockParse (bytes, bytesCount);
    if (NULL == block) return NULL;
    
    block->height = UInt32GetLE(&bytes[bytesCount - blockHeightSize]);

    return block;
}

static BRArrayOf(BRMerkleBlock*)
initialBlocksLoad (BRWalletManager manager) {
    BRSetOf(BRTransaction*) blockSet = BRSetNew(BRMerkleBlockHash, BRMerkleBlockEq, 100);
    if (1 != fileServiceLoad (manager->fileService, blockSet, fileServiceTypeBlocks, 1)) {
        BRSetFree (blockSet);
        return NULL;
    }

    size_t blocksCount = BRSetCount(blockSet);

    BRArrayOf(BRMerkleBlock*) blocks;
    array_new (blocks, blocksCount);
    array_set_count(blocks, blocksCount);

    BRSetAll(blockSet, (void**) blocks, blocksCount);
    BRSetFree(blockSet);

    return blocks;
}

/// MARK: - Peer File Service

static const char *fileServiceTypePeers = "peers";
enum {
    WALLET_MANAGER_PEER_VERSION_1
};

static UInt256
fileServiceTypePeerV1Identifier (BRFileServiceContext context,
                                 BRFileService fs,
                                 const void *entity) {
    const BRPeer *peer = entity;

    UInt256 hash;
    BRSHA256 (&hash, peer, sizeof(BRPeer));

    return hash;
}

static uint8_t *
fileServiceTypePeerV1Writer (BRFileServiceContext context,
                             BRFileService fs,
                             const void* entity,
                             uint32_t *bytesCount) {
    const BRPeer *peer = entity;

    // long term, this is wrong
    *bytesCount = sizeof (BRPeer);
    uint8_t *bytes = malloc (*bytesCount);
    memcpy (bytes, peer, *bytesCount);

    return bytes;
}

static void *
fileServiceTypePeerV1Reader (BRFileServiceContext context,
                             BRFileService fs,
                             uint8_t *bytes,
                             uint32_t bytesCount) {
    assert (bytesCount = sizeof (BRPeer));

    BRPeer *peer = malloc (bytesCount);;
    memcpy (peer, bytes, bytesCount);

    return peer;
}

static BRArrayOf(BRPeer)
initialPeersLoad (BRWalletManager manager) {
    /// Load peers for the wallet manager.
    BRSetOf(BRPeer*) peerSet = BRSetNew(BRPeerHash, BRPeerEq, 100);
    if (1 != fileServiceLoad (manager->fileService, peerSet, fileServiceTypePeers, 1)) {
        BRSetFree(peerSet);
        return NULL;
    }

    size_t peersCount = BRSetCount(peerSet);
    BRPeer *peersRefs[peersCount];

    BRSetAll(peerSet, (void**) peersRefs, peersCount);
    BRSetClear(peerSet);
    BRSetFree(peerSet);

    BRArrayOf(BRPeer) peers;
    array_new (peers, peersCount);

    for (size_t index = 0; index < peersCount; index++)
        array_add (peers, *peersRefs[index]);

    return peers;
}

static void
bwmFileServiceErrorHandler (BRFileServiceContext context,
                            BRFileService fs,
                            BRFileServiceError error) {
    BRWalletManager bwm = (BRWalletManager) context;

    switch (error.type) {
        case FILE_SERVICE_IMPL:
            // This actually a FATAL - an unresolvable coding error.
            _peer_log ("bread: FileService Error: IMPL: %s", error.u.impl.reason);
            break;
        case FILE_SERVICE_UNIX:
            _peer_log ("bread: FileService Error: UNIX: %s", strerror(error.u.unix.error));
            break;
        case FILE_SERVICE_ENTITY:
            // This is likely a coding error too.
            _peer_log ("bread: FileService Error: ENTITY (%s); %s",
                     error.u.entity.type,
                     error.u.entity.reason);
            break;
    }
    _peer_log ("bread: FileService Error: FORCED SYNC%s", "");

    if (NULL != bwm->peerManager)
        BRPeerManagerRescan (bwm->peerManager);
}

/// MARK: - Wallet Manager

static BRWalletManager
bwmCreateErrorHandler (BRWalletManager bwm, int fileService, const char* reason) {
    if (NULL != bwm) free (bwm);
    if (fileService)
        _peer_log ("bread: on ewmCreate: FileService Error: %s", reason);
    else
        _peer_log ("bread: on ewmCreate: Error: %s", reason);

    return NULL;
}

extern BRWalletManager
BRWalletManagerNew (BRWalletManagerClient client,
                    BRMasterPubKey mpk,
                    const BRChainParams *params,
                    uint32_t earliestKeyTime,
                    BRSyncMode mode,
                    const char *baseStoragePath) {
    BRWalletManager bwm = malloc (sizeof (struct BRWalletManagerStruct));
    if (NULL == bwm) return bwmCreateErrorHandler (NULL, 0, "allocate");

    bwm->mode = mode;
    bwm->client = client;

    BRWalletForkId fork = getForkId (params);
    const char *networkName  = getNetworkName  (params);
    const char *currencyName = getCurrencyName (params);
    //    manager->walletForkId = fork;

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&bwm->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // Initialize the `brdSync` struct
    bwm->brdSync.rid = -1;
    bwm->brdSync.begBlockNumber = 0;
    bwm->brdSync.endBlockNumber = 0;
    bwm->brdSync.completed = 0;

    // Create the alarm clock, but don't start it.
    alarmClockCreateIfNecessary(0);

    const char *handlerName = (BRChainParamsIsBitcoin (params)
                              ? "Core Bitcoin BWM"
                              : (BRChainParamsIsBitcash (params)
                                 ? "Core Bitcash BWM"
                                 : "Core BWM"));
    
    // The `main` event handler has a periodic wake-up.  Used, perhaps, if the mode indicates
    // that we should/might query the BRD backend services.
    bwm->handler = eventHandlerCreate (handlerName,
                                       bwmEventTypes,
                                       bwmEventTypesCount,
                                       &bwm->lock);

    //
    // Create the File Service w/ associated types.
    //
    bwm->fileService = fileServiceCreate (baseStoragePath, currencyName, networkName, 
                                              bwm,
                                              bwmFileServiceErrorHandler);
    if (NULL == bwm->fileService) return bwmCreateErrorHandler (bwm, 1, "create");

    /// Transaction
    if (1 != fileServiceDefineType (bwm->fileService, fileServiceTypeTransactions, WALLET_MANAGER_TRANSACTION_VERSION_1,
                                    (BRFileServiceContext) bwm,
                                    fileServiceTypeTransactionV1Identifier,
                                    fileServiceTypeTransactionV1Reader,
                                    fileServiceTypeTransactionV1Writer) ||
        1 != fileServiceDefineCurrentVersion (bwm->fileService, fileServiceTypeTransactions,
                                              WALLET_MANAGER_TRANSACTION_VERSION_1))
        return bwmCreateErrorHandler (bwm, 1, fileServiceTypeTransactions);

    /// Block
    if (1 != fileServiceDefineType (bwm->fileService, fileServiceTypeBlocks, WALLET_MANAGER_BLOCK_VERSION_1,
                                    (BRFileServiceContext) bwm,
                                    fileServiceTypeBlockV1Identifier,
                                    fileServiceTypeBlockV1Reader,
                                    fileServiceTypeBlockV1Writer) ||
        1 != fileServiceDefineCurrentVersion (bwm->fileService, fileServiceTypeBlocks,
                                              WALLET_MANAGER_BLOCK_VERSION_1))
        return bwmCreateErrorHandler (bwm, 1, fileServiceTypeBlocks);

    /// Peer
    if (1 != fileServiceDefineType (bwm->fileService, fileServiceTypePeers, WALLET_MANAGER_PEER_VERSION_1,
                                    (BRFileServiceContext) bwm,
                                    fileServiceTypePeerV1Identifier,
                                    fileServiceTypePeerV1Reader,
                                    fileServiceTypePeerV1Writer) ||
        1 != fileServiceDefineCurrentVersion (bwm->fileService, fileServiceTypePeers,
                                              WALLET_MANAGER_PEER_VERSION_1))
        return bwmCreateErrorHandler (bwm, 1, fileServiceTypePeers);

    /// Load transactions for the wallet manager.
    BRArrayOf(BRTransaction*) transactions = initialTransactionsLoad(bwm);
    /// Load blocks and peers for the peer manager.
    BRArrayOf(BRMerkleBlock*) blocks = initialBlocksLoad(bwm);
    BRArrayOf(BRPeer) peers = initialPeersLoad(bwm);

    // If any of these are NULL, then there was a failure; on a failure they all need to be cleared
    // which will cause a *FULL SYNC*
    if (NULL == transactions || NULL == blocks || NULL == peers) {
        if (NULL == transactions) array_new (transactions, 1);
        else array_clear(transactions);

        if (NULL == blocks) array_new (blocks, 1);
        else array_clear(blocks);

        if (NULL == peers) array_new (peers, 1);
        else array_clear(peers);
    }

    bwm->wallet = BRWalletNew (transactions, array_count(transactions), mpk, fork);
    BRWalletSetCallbacks (bwm->wallet, bwm,
                          _BRWalletManagerBalanceChanged,
                          _BRWalletManagerTxAdded,
                          _BRWalletManagerTxUpdated,
                          _BRWalletManagerTxDeleted);
    

    bwm->peerManager = BRPeerManagerNew (params, bwm->wallet, earliestKeyTime,
                                         blocks, array_count(blocks),
                                         peers,  array_count(peers));
    BRPeerManagerSetCallbacks (bwm->peerManager, bwm,
                               _BRWalletManagerSyncStarted,
                               _BRWalletManagerSyncStopped,
                               _BRWalletManagerTxStatusUpdate,
                               _BRWalletManagerSaveBlocks,
                               _BRWalletManagerSavePeers,
                               _BRWalletManagerNetworkIsReachabele,
                               _BRWalletManagerThreadCleanup);


    // Support the requested mode.  We already created the BRPeerManager.  But we might consider
    // only creating the BRPeerManager for modes where it is used (all modes besides BRD_ONLY).
    // And, we might consider not populating the BRPeerMangaer with blocks for modes where they
    // are not used (BRD_ONLY and BRD_WITH_P2P_SEND).
    switch (bwm->mode) {
        case SYNC_MODE_BRD_ONLY:
        case SYNC_MODE_BRD_WITH_P2P_SEND: {
            // Create a 'special' peer manager.
            // <code removed>
            
            // Announce all the provided transactions...
            // <code removed>
            
            // ... and then the latest block.
            //            BREthereumBlock lastBlock = NULL;
            //            FOR_SET (BREthereumBlock, block, blocks)
            //            if (NULL == lastBlock || blockGetNumber(lastBlock) < blockGetNumber(block))
            //                lastBlock = block;
            //            ewmSignalBlockChain (ewm,
            //                                 blockGetHash( lastBlock),
            //                                 blockGetNumber (lastBlock),
            //                                 blockGetTimestamp (lastBlock));
            
            // ... and then just ignore nodes
            // <code removed>
            
            // Free sets... BUT DO NOT free 'nodes' as those had 'OwnershipGiven' in bcsCreate()
            // <code removed>
            
            // Add ewmPeriodicDispatcher to handlerForMain.  Note that a 'timeout' is handled by
            // an OOB (out-of-band) event whereby the event is pushed to the front of the queue.
            // This may not be the right thing to do.  Imagine that EWM is blocked somehow (doing
            // a time consuming calculation) and two 'timeout events' occur - the events will be
            // queued in the wrong order (second before first).
            //
            // The function `ewmPeriodcDispatcher()` will be installed as a periodic alarm
            // on the event handler.  It will only trigger when the event handler is running (
            // the time between `eventHandlerStart()` and `eventHandlerStop()`)
            
            eventHandlerSetTimeoutDispatcher (bwm->handler,
                                              1000 * BWM_SLEEP_SECONDS,
                                              (BREventDispatcher) bwmPeriodicDispatcher,
                                              (void*) bwm);
            
            break;
        }
            
        case SYNC_MODE_P2P_WITH_BRD_SYNC:  //
        case SYNC_MODE_P2P_ONLY: {
            // Create a 'special' peer manager
            // <code removed>
            break;
        }
    }
    
    array_free(transactions); array_free(blocks); array_free(peers);
    
    bwm->client.funcWalletManagerEvent (bwm->client.context,
                                        bwm,
                                        (BRWalletManagerEvent) {
                                            BITCOIN_WALLET_MANAGER_CREATED
                                        });
    
    bwm->client.funcWalletEvent (bwm->client.context,
                                 bwm,
                                 bwm->wallet,
                                 (BRWalletEvent) {
                                     BITCOIN_WALLET_CREATED
                                 });
    
    return bwm;
}

extern void
BRWalletManagerFree (BRWalletManager manager) {
    fileServiceRelease(manager->fileService);
    BRPeerManagerFree(manager->peerManager);
    BRWalletFree(manager->wallet);
    free (manager);
}

extern BRWallet *
BRWalletManagerGetWallet (BRWalletManager manager) {
    return manager->wallet;
}

extern BRPeerManager *
BRWalletManagerGetPeerManager (BRWalletManager manager) {
    return manager->peerManager;
}

extern void
BRWalletManagerConnect (BRWalletManager manager) {
    switch (manager->mode) {
            case SYNC_MODE_BRD_ONLY:
            break;

            case SYNC_MODE_BRD_WITH_P2P_SEND:
            case SYNC_MODE_P2P_WITH_BRD_SYNC:
            case SYNC_MODE_P2P_ONLY:
            BRPeerManagerConnect(manager->peerManager);
            break;
    }

    eventHandlerStart (manager->handler);

    manager->client.funcWalletManagerEvent (manager->client.context,
                                            manager,
                                            (BRWalletManagerEvent) {
                                                BITCOIN_WALLET_MANAGER_CONNECTED
                                            });
}

extern void
BRWalletManagerDisconnect (BRWalletManager manager) {
    switch (manager->mode) {
            case SYNC_MODE_BRD_ONLY:
            break;

            case SYNC_MODE_BRD_WITH_P2P_SEND:
            case SYNC_MODE_P2P_WITH_BRD_SYNC:
            case SYNC_MODE_P2P_ONLY:
            BRPeerManagerDisconnect(manager->peerManager);
            break;
    }

    eventHandlerStop(manager->handler);

    manager->client.funcWalletManagerEvent (manager->client.context,
                                            manager,
                                            (BRWalletManagerEvent) {
                                                BITCOIN_WALLET_MANAGER_DISCONNECTED
                                            });
}

extern void
BRWalletManagerScan (BRWalletManager manager) {
    BRPeerManagerRescan(manager->peerManager);
    manager->client.funcWalletManagerEvent (manager->client.context,
                                            manager,
                                            (BRWalletManagerEvent) {
                                                BITCOIN_WALLET_MANAGER_SYNC_STARTED
                                            });
}

extern BRAddress *
BRWalletManagerGetUnusedAddrs (BRWalletManager manager,
                               uint32_t limit) {
    //    assert (sizeof (size_t) == sizeof (uint32_t));

    BRAddress *addresses = calloc (limit, sizeof (BRAddress));
    BRWalletUnusedAddrs (manager->wallet, addresses, (uint32_t) limit, 0);
    return addresses;
}

/// MARK: Wallet Callbacks

static void
_BRWalletManagerBalanceChanged (void *info, uint64_t balanceInSatoshi) {
    BRWalletManager manager = (BRWalletManager) info;
    manager->client.funcWalletEvent (manager->client.context,
                                     manager,
                                     manager->wallet,
                                     (BRWalletEvent) {
                                         BITCOIN_WALLET_BALANCE_UPDATED,
                                         { .balance = { balanceInSatoshi }}
                                     });
}

static void
_BRWalletManagerTxAdded   (void *info, BRTransaction *tx) {
    BRWalletManager manager = (BRWalletManager) info;
    fileServiceSave(manager->fileService, fileServiceTypeTransactions, tx);
    manager->client.funcTransactionEvent (manager->client.context,
                                          manager,
                                          manager->wallet,
                                          tx,
                                          (BRTransactionEvent) {
                                              BITCOIN_TRANSACTION_ADDED
                                          });
}

static void
_BRWalletManagerTxUpdated (void *info, const UInt256 *hashes, size_t count, uint32_t blockHeight, uint32_t timestamp) {
    BRWalletManager manager = (BRWalletManager) info;

    for (size_t index = 0; index < count; index++) {
        UInt256 hash = hashes[index];
        BRTransaction *transaction = BRWalletTransactionForHash(manager->wallet, hash);

        // assert timestamp and blockHeight in transaction
        fileServiceSave (manager->fileService, fileServiceTypeTransactions, transaction);

        manager->client.funcTransactionEvent (manager->client.context,
                                              manager,
                                              manager->wallet,
                                              transaction,
                                              (BRTransactionEvent) {
                                                  BITCOIN_TRANSACTION_UPDATED,
                                                  { .updated = { blockHeight, timestamp }}
                                              });
    }
}

static void
_BRWalletManagerTxDeleted (void *info, UInt256 hash, int notifyUser, int recommendRescan) {
    BRWalletManager manager = (BRWalletManager) info;
    fileServiceRemove(manager->fileService, fileServiceTypeTransactions, hash);

    BRTransaction *transaction = BRWalletTransactionForHash(manager->wallet, hash);
    manager->client.funcTransactionEvent (manager->client.context,
                                          manager,
                                          manager->wallet,
                                          transaction,
                                          (BRTransactionEvent) {
                                              BITCOIN_TRANSACTION_DELETED
                                          });
}

/// MARK: - Peer Manager Callbacks

static void
_BRWalletManagerSaveBlocks (void *info, int replace, BRMerkleBlock **blocks, size_t count) {
    BRWalletManager manager = (BRWalletManager) info;

    if (replace) fileServiceClear(manager->fileService, fileServiceTypeBlocks);
    for (size_t index = 0; index < count; index++)
        fileServiceSave (manager->fileService, fileServiceTypeBlocks, blocks[index]);
}

static void
_BRWalletManagerSavePeers  (void *info, int replace, const BRPeer *peers, size_t count) {
    BRWalletManager manager = (BRWalletManager) info;

    if (replace) fileServiceClear(manager->fileService, fileServiceTypePeers);
    for (size_t index = 0; index < count; index++)
        fileServiceSave (manager->fileService, fileServiceTypePeers, &peers[index]);
}

static void
_BRWalletManagerSyncStarted (void *info) {
    BRWalletManager manager = (BRWalletManager) info;
    manager->client.funcWalletManagerEvent (manager->client.context,
                                            manager,
                                            (BRWalletManagerEvent) {
                                                BITCOIN_WALLET_MANAGER_SYNC_STARTED
                                            });
}

static void
_BRWalletManagerSyncStopped (void *info, int reason) {
    BRWalletManager manager = (BRWalletManager) info;
    manager->client.funcWalletManagerEvent (manager->client.context,
                                            manager,
                                            (BRWalletManagerEvent) {
                                                BITCOIN_WALLET_MANAGER_SYNC_STOPPED,
                                                { .syncStopped = { reason }}
                                            });
}

static void
_BRWalletManagerTxStatusUpdate (void *info) {
//    BRWalletManager manager = (BRWalletManager) info;

    // event

}

static int
_BRWalletManagerNetworkIsReachabele (void *info) {
//    BRWalletManager manager = (BRWalletManager) info;

    // event
   return 1;
}

static void
_BRWalletManagerThreadCleanup (void *info) {
//    BRWalletManager manager = (BRWalletManager) info;

    // event
}

///
/// MARK: Events
//

static void
bwmUpdateBlockNumber (BRWalletManager bwm) {
    switch (bwm->mode) {
        case SYNC_MODE_BRD_ONLY:
        case SYNC_MODE_BRD_WITH_P2P_SEND:
        case SYNC_MODE_P2P_WITH_BRD_SYNC:
            bwm->client.funcGetBlockNumber (bwm->client.context,
                                            bwm,
                                            bwm->requestId++);
            break;

        case SYNC_MODE_P2P_ONLY:
            assert (0);
            break;
    }
}

static void
bwmUpdateTransactions (BRWalletManager bwm) {
    switch (bwm->mode) {
        case SYNC_MODE_BRD_ONLY:
        case SYNC_MODE_BRD_WITH_P2P_SEND:
        case SYNC_MODE_P2P_WITH_BRD_SYNC:
            // Callback to 'client' to get all transactions (for all wallet addresses) between
            // a {beg,end}BlockNumber.  The client will gather the transactions and then call
            // bwmAnnounceTransaction()  (for each one or with all of them).
            if (bwm->brdSync.begBlockNumber != bwm->brdSync.endBlockNumber)
                bwm->client.funcGetTransactions (bwm->client.context,
                                                 bwm,
                                                 bwm->brdSync.begBlockNumber,
                                                 bwm->brdSync.endBlockNumber,
                                                 bwm->requestId++);
            break;

        case SYNC_MODE_P2P_ONLY:
            // Never here
            assert (0);
            break;
    }
}

const BREventType *bwmEventTypes[] = {
//    &handleBlockChainEventType,
//    &handleAccountStateEventType,
//    &handleBalanceEventType,
//    &handleGasPriceEventType,
//    &handleGasEstimateEventType,
//    &handleTransactionEventType,
//    &handleLogEventType,
//    &handleSaveBlocksEventType,
//    &handleSaveNodesEventType,
//    &handleSyncEventType,
//    &handleGetBlocksEventType,
//
//    &ewmClientWalletEventType,
//    //    &ewmClientBlockEventType,
//    &ewmClientTransactionEventType,
//    &ewmClientPeerEventType,
//    &ewmClientEWMEventType,
//    &ewmClientAnnounceBlockNumberEventType,
//    &ewmClientAnnounceNonceEventType,
//    &ewmClientAnnounceBalanceEventType,
//    &ewmClientAnnounceGasPriceEventType,
//    &ewmClientAnnounceGasEstimateEventType,
//    &ewmClientAnnounceSubmitTransferEventType,
//    &ewmClientAnnounceTransactionEventType,
//    &ewmClientAnnounceLogEventType,
//    &ewmClientAnnounceCompleteEventType,
//    &ewmClientAnnounceTokenEventType,
//    &ewmClientAnnounceTokenCompleteEventType,
};

const unsigned int
bwmEventTypesCount = (sizeof (bwmEventTypes) / sizeof (BREventType*));

//
// Periodicaly query the BRD backend to get current status (block number, nonce, balances,
// transactions and logs) The event will be NULL (as specified for a 'period dispatcher' - See
// `eventHandlerSetTimeoutDispatcher()`)
//
static void
bwmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event) {
    BRWalletManager bwm = (BRWalletManager) event->context;

    if (SYNC_MODE_P2P_ONLY == bwm->mode || SYNC_MODE_P2P_WITH_BRD_SYNC == bwm->mode) return;

    bwmUpdateBlockNumber(bwm);

    // Handle a BRD Sync:

    // 1) check if the prior sync has completed.
    if (bwm->brdSync.completed) {
        // 1a) if so, advance the sync range by updating `begBlockNumber`
        bwm->brdSync.begBlockNumber = (bwm->brdSync.endBlockNumber >=  BWM_BRD_SYNC_START_BLOCK_OFFSET
                                       ? bwm->brdSync.endBlockNumber - BWM_BRD_SYNC_START_BLOCK_OFFSET
                                       : 0);
    }
    // 2) completed or not, update the `endBlockNumber` to the current block height.
    bwm->brdSync.endBlockNumber = bwm->blockHeight;

    // 3) We'll query all transactions for this bwm's account.  We'll process all the transactions
    // into the wallet.
    bwmUpdateTransactions(bwm);

    // Note: we don't do the following  `ewmUpdateTransactions` because that is `extern`
    bwm->brdSync.rid = bwm->requestId;
    bwm->brdSync.completed = 0;

    // End handling a BRD Sync
}

extern int // success - data is valid
bwmAnnounceTransaction (BRWalletManager manager,
                        int id,
                        BRTransaction *transaction) {
    pthread_mutex_lock (&manager->lock);
    BRWalletRegisterTransaction (manager->wallet, transaction);
    pthread_mutex_unlock (&manager->lock);
    return 1;
}

extern void
bwmAnnounceTransactionComplete (BRWalletManager manager,
                                int rid,
                                int success) {
    pthread_mutex_lock (&manager->lock);
    if (rid == manager->brdSync.rid)
        manager->brdSync.completed = success;
    pthread_mutex_unlock (&manager->lock);
}

extern int
bwmAnnounceBlockNumber (BRWalletManager manager,
                        int rid,
                        uint64_t blockNumber) {
    pthread_mutex_lock (&manager->lock);
    manager->blockHeight = blockNumber;
    pthread_mutex_unlock (&manager->lock);
    return 1;
}

