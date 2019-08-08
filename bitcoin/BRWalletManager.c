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
#include "BRBase.h"
#include "BRSet.h"
#include "BRWalletManager.h"
#include "BRWalletManagerPrivate.h"
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
#define BWM_BRD_SYNC_CHUNK_SIZE                 50000

#if !defined (MAX)
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#if !defined (MIN)
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

/* Forward Declarations */
static void
bwmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);

static void
bwmSyncReset (BRWalletManager bwm);

static void
bwmSyncStart (BRWalletManager bwm);

static void
bwmSyncTransaction (BRWalletManager bwm,
                    int rid,
                    BRTransaction *transaction);

static void
bwmSyncComplete (BRWalletManager bwm,
                 int rid,
                 int success);

static void _BRWalletManagerBalanceChanged (void *info, uint64_t balanceInSatoshi);
static void _BRWalletManagerTxAdded   (void *info, BRTransaction *tx);
static void _BRWalletManagerTxUpdated (void *info, const UInt256 *hashes, size_t count, uint32_t blockHeight, uint32_t timestamp);
static void _BRWalletManagerTxDeleted (void *info, UInt256 hash, int notifyUser, int recommendRescan);
static void _BRWalletManagerTxPublished (void *info, int error);

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
    assert (bytesCount == sizeof (BRPeer));

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
                    const char *baseStoragePath,
                    uint64_t blockHeight) {
    BRWalletManager bwm = calloc (1, sizeof (struct BRWalletManagerStruct));
    if (NULL == bwm) return bwmCreateErrorHandler (NULL, 0, "allocate");

    bwm->mode = mode;
    bwm->client = client;
    bwm->requestId = 0;

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

    bwm->wallet = BRWalletNew (params->addrParams, transactions, array_count(transactions), mpk);
    BRWalletSetCallbacks (bwm->wallet, bwm,
                          _BRWalletManagerBalanceChanged,
                          _BRWalletManagerTxAdded,
                          _BRWalletManagerTxUpdated,
                          _BRWalletManagerTxDeleted);

    // Find the BRCheckpoint that is at least one week before earliestKeyTime.
#define ONE_WEEK_IN_SECONDS      (7*24*60*60)
    const BRCheckPoint *earliestCheckPoint = BRChainParamsGetCheckpointBefore (params, earliestKeyTime - ONE_WEEK_IN_SECONDS);
    assert (NULL != earliestCheckPoint);

    // If in a SYNC_MODE using BRD, explicitly set the BRPeerManager's `earliestKeyTime` to
    // 'infinity'.  Normally that time is defined as that time when the User's `paperKey` was
    // created.  It informs the Bitcoin peers of the point beyond which BTC/BCH transactions might
    // occur.  But, we'll set time impossibly into the future for the case where we want to disable
    // P2P 'syncing' yet still maintain a P2P connection.

    if (SYNC_MODE_BRD_ONLY == bwm->mode || SYNC_MODE_BRD_WITH_P2P_SEND == bwm->mode)
        // There might be BTC calculations based on adding to earliestKeyTime.  Try to avoid having
        // those computations overflow, hackily.  Back up one year before the end of time.
        earliestKeyTime = UINT32_MAX - 365 * 24 * 60 * 60;

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


    // Initialize this instance's blockHeight.  This might be out-of-sync with a) the P2P block
    // height which will be derived from the persistently restored blocks and then from the sync()
    // process or b) from the API-based Blockchain DB reported block height which will be updated
    // preriodically when in API sync modes.
    //
    // So, we'll start with the best block height we have and expect it to change. Doing this allows
    // an API-based sync to start immediately rather than waiting for a bwmUpdateBlockNumber()
    // result in period '1' and then starting the sync in period '2' - where each period is
    // BWM_SLEEP_SECONDS and at least 1 minute.
    bwm->blockHeight = (uint32_t) blockHeight;

    // Initialize the `brdSync` struct
    bwm->brdSync.rid = -1;
    bwm->brdSync.lastInternalAddress = BR_ADDRESS_NONE;
    bwm->brdSync.lastExternalAddress = BR_ADDRESS_NONE;
    bwm->brdSync.begBlockNumber = earliestCheckPoint->height;
    bwm->brdSync.endBlockNumber = MAX (bwm->brdSync.begBlockNumber, bwm->blockHeight);
    bwm->brdSync.chunkBegBlockNumber = bwm->brdSync.begBlockNumber;
    bwm->brdSync.chunkEndBlockNumber = bwm->brdSync.endBlockNumber;
    bwm->brdSync.completed = 0;


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

    assert (NULL != bwm->client.funcWalletManagerEvent);
    bwm->client.funcWalletManagerEvent (bwm->client.context,
                                        bwm,
                                        (BRWalletManagerEvent) {
                                            BITCOIN_WALLET_MANAGER_CREATED
                                        });

    assert (NULL != bwm->client.funcWalletEvent);
    bwm->client.funcWalletEvent (bwm->client.context,
                                 bwm,
                                 bwm->wallet,
                                 (BRWalletEvent) {
                                     BITCOIN_WALLET_CREATED
                                 });

    for (size_t i = 0; transactions && i < array_count(transactions); i++) {
        bwm->client.funcTransactionEvent (bwm->client.context,
                                          bwm,
                                          bwm->wallet,
                                          transactions[i],
                                          (BRTransactionEvent) {
                                              BITCOIN_TRANSACTION_ADDED
                                          });

        bwm->client.funcTransactionEvent (bwm->client.context,
                                          bwm,
                                          bwm->wallet,
                                          transactions[i],
                                          (BRTransactionEvent) {
                                          BITCOIN_TRANSACTION_UPDATED,
                                              { .updated = { transactions[i]->blockHeight, transactions[i]->timestamp }}
                                          });
    }

    array_free(transactions); array_free(blocks); array_free(peers);

    return bwm;
}

extern void
BRWalletManagerFree (BRWalletManager manager) {
    BRPeerManagerDisconnect (manager->peerManager);
    BRPeerManagerFree (manager->peerManager);

    BRWalletFree (manager->wallet);

    fileServiceRelease (manager->fileService);

    eventHandlerDestroy (manager->handler);

    pthread_mutex_destroy (&manager->lock);

    memset (manager, 0, sizeof(*manager));
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
            bwmSyncReset (manager);
            break;

        case SYNC_MODE_BRD_WITH_P2P_SEND:
        case SYNC_MODE_P2P_WITH_BRD_SYNC:
            bwmSyncReset (manager);
            // no break;

        case SYNC_MODE_P2P_ONLY:
            BRPeerManagerConnect(manager->peerManager);
            break;
    }

    eventHandlerStart (manager->handler);

    assert (NULL != manager->client.funcWalletManagerEvent);
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

    assert (NULL != manager->client.funcWalletManagerEvent);
    manager->client.funcWalletManagerEvent (manager->client.context,
                                            manager,
                                            (BRWalletManagerEvent) {
                                                BITCOIN_WALLET_MANAGER_DISCONNECTED
                                            });
}

extern void
BRWalletManagerScan (BRWalletManager manager) {
    BRPeerManagerRescan(manager->peerManager);

    assert (NULL != manager->client.funcWalletManagerEvent);
    manager->client.funcWalletManagerEvent (manager->client.context,
                                            manager,
                                            (BRWalletManagerEvent) {
                                                BITCOIN_WALLET_MANAGER_SYNC_STARTED
                                            });
}

extern unsigned int
BRWalletManagerGetThenIncrRequestId (BRWalletManager manager) {
    unsigned int requestId;
    pthread_mutex_lock (&manager->lock);
    requestId = manager->requestId++;
    pthread_mutex_unlock (&manager->lock);
    return requestId;

}
extern BRTransaction *
BRWalletManagerCreateTransaction (BRWalletManager manager,
                                  BRWallet *wallet,
                                  uint64_t amount,
                                  const char *addr) {
    BRTransaction *transaction = BRWalletCreateTransaction (wallet, amount, addr);
    if (NULL != transaction) {
        assert (NULL != manager->client.funcTransactionEvent);
        manager->client.funcTransactionEvent (manager->client.context,
                                              manager,
                                              manager->wallet,
                                              transaction,
                                              (BRTransactionEvent) {
                                                  BITCOIN_TRANSACTION_CREATED
                                              });
    }

    return transaction;
}

extern int
BRWalletManagerSignTransaction (BRWalletManager manager,
                                OwnershipKept BRTransaction *transaction,
                                const void *seed,
                                size_t seedLen) {
    int forkId = BRPeerManagerChainParams(manager->peerManager)->forkId;

    int r = (1 == BRWalletSignTransaction (manager->wallet, transaction, forkId, seed, seedLen) ? 1 : 0);
    if (r) {
        assert (NULL != manager->client.funcTransactionEvent);
        manager->client.funcTransactionEvent (manager->client.context,
                                              manager,
                                              manager->wallet,
                                              transaction,
                                              (BRTransactionEvent) {
                                                  BITCOIN_TRANSACTION_SIGNED
                                              });
    }

    return r;
}

typedef struct {
    BRWalletManager manager;
    BRTransaction *transaction;
} SubmitTransactionInfo;

extern void
BRWalletManagerSubmitTransaction (BRWalletManager manager,
                                  OwnershipGiven BRTransaction *transaction) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
            assert (NULL != manager->client.funcSubmitTransaction);
            manager->client.funcSubmitTransaction (manager->client.context,
                                                   manager,
                                                   manager->wallet,
                                                   transaction,
                                                   BRWalletManagerGetThenIncrRequestId (manager));
            break;

        case SYNC_MODE_BRD_WITH_P2P_SEND:
        case SYNC_MODE_P2P_WITH_BRD_SYNC:
        case SYNC_MODE_P2P_ONLY: {
            SubmitTransactionInfo *info = malloc (sizeof (SubmitTransactionInfo));
            info->manager = manager;
            info->transaction = transaction;

            BRPeerManagerPublishTx (manager->peerManager, transaction, info,
                                    _BRWalletManagerTxPublished);
            break;
        }
    }
}

extern void
BRWalletManagerUpdateFeePerKB (BRWalletManager manager,
                               BRWallet *wallet,
                               uint64_t feePerKb) {
    BRWalletSetFeePerKb (wallet, feePerKb);
    manager->client.funcWalletEvent (manager->client.context,
                                     manager,
                                     wallet,
                                     (BRWalletEvent) {
                                         BITCOIN_WALLET_FEE_PER_KB_UPDATED,
                                         { .feePerKb = { feePerKb }}
                                     });
}

extern void
BRWalletManagerEstimateFeeForTransfer (BRWalletManager manager,
                                       BRWallet *wallet,
                                       BRCookie cookie,
                                       uint64_t transferAmount,
                                       uint64_t feePerKb) {
    uint64_t feePerKBSaved = BRWalletFeePerKb (wallet);
    BRWalletSetFeePerKb (wallet, feePerKb);
    uint64_t fee  = (0 == transferAmount ? 0 : BRWalletFeeForTxAmount (wallet, transferAmount));
    uint32_t sizeInByte = (uint32_t) ((1000 * fee)/ feePerKb);
    BRWalletSetFeePerKb (wallet, feePerKBSaved);

    // TODO(fix): this should be on a thread
    manager->client.funcWalletEvent (manager->client.context,
                                     manager,
                                     wallet,
                                     (BRWalletEvent) {
                                         BITCOIN_WALLET_FEE_ESTIMATED,
                                         { .feeEstimated = { cookie, feePerKb, sizeInByte }}
                                     });
}

static void
BRWalletManagerAddressToLegacy (BRWalletManager manager, BRAddress *addr) {
    *addr = BRWalletAddressToLegacy (manager->wallet, addr);
}

extern void
BRWalletManagerGenerateUnusedAddrs (BRWalletManager manager) {
    BRWalletUnusedAddrs (manager->wallet, NULL, SEQUENCE_GAP_LIMIT_EXTERNAL, 0);
    BRWalletUnusedAddrs (manager->wallet, NULL, SEQUENCE_GAP_LIMIT_INTERNAL, 1);
}

extern BRAddress *
BRWalletManagerGetUnusedAddrs (BRWalletManager manager,
                               size_t *addressCount) {
    assert (addressCount);

    BRWalletUnusedAddrs (manager->wallet, NULL, SEQUENCE_GAP_LIMIT_EXTERNAL, 0);
    BRWalletUnusedAddrs (manager->wallet, NULL, SEQUENCE_GAP_LIMIT_INTERNAL, 1);
    size_t addrCount = SEQUENCE_GAP_LIMIT_EXTERNAL + SEQUENCE_GAP_LIMIT_INTERNAL;

    BRAddress *addrs = (BRAddress *) calloc (2 * addrCount, sizeof (BRAddress));
    BRWalletUnusedAddrs (manager->wallet, addrs, SEQUENCE_GAP_LIMIT_EXTERNAL, 0);
    BRWalletUnusedAddrs (manager->wallet, &addrs[SEQUENCE_GAP_LIMIT_EXTERNAL], SEQUENCE_GAP_LIMIT_INTERNAL, 1);

    memcpy (addrs + addrCount, addrs, addrCount * sizeof(BRAddress));
    for (size_t index = 0; index < addrCount; index++)
        BRWalletManagerAddressToLegacy (manager, &addrs[addrCount + index]);

    *addressCount = 2 * addrCount;
    return addrs;
}

/**
 * Return all unused addresses tracked by the wallet. The addresses
 * are both 'internal' and 'external' ones.
 *
 * The addresses are returned as both a sequential array of BRAddress data, as well
 * as an array of pointers to each address.
 *
 * Note: Both the addressStrings and addressArray arrays must be freed.
 *
 * Note: The addressStrings array contains pointers to data in the addressArray. As such,
 *       elements in addressStrings should not be accessed once addressArray has been freed.
 */
// static void
// BRWalletManagerUnusedAddrsAsStrings (BRWalletManager bwm, size_t *addressCount, const char ***addressStrings, BRAddress **addressArray) {
//     size_t addrCount = 0;
//     BRAddress *addrArray = BRWalletManagerGetUnusedAddrs (bwm, &addrCount);
//
//     const char **addrsStrings = calloc (addrCount, sizeof(char *));
//     for (size_t index = 0; index < addrCount; index ++)
//         addrsStrings[index] = (char *) &addrArray[index];
//
//     *addressCount = addrCount;
//     *addressStrings = addrsStrings;
//     *addressArray = addrArray;
// }

extern BRAddress *
BRWalletManagerGetAllAddrs (BRWalletManager manager,
                            size_t *addressCount) {
    assert (addressCount);

    size_t addrCount = BRWalletAllAddrs (manager->wallet, NULL, 0);

    BRAddress *addrs = (BRAddress *) calloc (2 * addrCount, sizeof (BRAddress));
    BRWalletAllAddrs (manager->wallet, addrs, addrCount);

    memcpy (addrs + addrCount, addrs, addrCount * sizeof(BRAddress));
    for (size_t index = 0; index < addrCount; index++)
        BRWalletManagerAddressToLegacy (manager, &addrs[addrCount + index]);

    *addressCount = 2 * addrCount;
    return addrs;
}

/**
 * Return all addresses, used and unused, tracked by the wallet. The addresses
 * are both 'internal' and 'external' ones.
 *
 * The addresses are returned as both a sequential array of BRAddress data, as well
 * as an array of pointers to each address.
 *
 * Note: Both the addressStrings and addressArray arrays must be freed.
 *
 * Note: The addressStrings array contains pointers to data in the addressArray. As such,
 *       elements in addressStrings should not be accessed once addressArray has been freed.
 */
static void
BRWalletManagerGetAllAddrsAsStrings (BRWalletManager bwm, size_t *addressCount, const char ***addressStrings, BRAddress **addressArray) {
    size_t addrCount = 0;
    BRAddress *addrArray = BRWalletManagerGetAllAddrs (bwm, &addrCount);

    const char **addrsStrings = calloc (addrCount, sizeof(char *));
    for (size_t index = 0; index < addrCount; index ++)
        addrsStrings[index] = (char *) &addrArray[index];

    *addressCount = addrCount;
    *addressStrings = addrsStrings;
    *addressArray = addrArray;
}

static void
BRWalletManagerUpdateHeightIfAppropriate (BRWalletManager manager,
                                          uint32_t height) {
    pthread_mutex_lock (&manager->lock);
    if (height != manager->blockHeight) {
        assert (NULL != manager->client.funcWalletManagerEvent);

        manager->blockHeight = height;
        manager->client.funcWalletManagerEvent (manager->client.context,
                                                manager,
                                                (BRWalletManagerEvent) {
                                                    BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED,
                                                    { .blockHeightUpdated = { manager->blockHeight }}
                                                });
    }
    pthread_mutex_unlock (&manager->lock);
}

static void
BRWalletManagerCheckHeight (BRWalletManager manager) {
    BRWalletManagerUpdateHeightIfAppropriate (manager, BRPeerManagerLastBlockHeight (manager->peerManager));
}

/// MARK: Wallet Callbacks

static void
_BRWalletManagerBalanceChanged (void *info,
                                uint64_t balanceInSatoshi) {
    BRWalletManager manager = (BRWalletManager) info;

    assert (NULL != manager->client.funcWalletEvent);
    manager->client.funcWalletEvent (manager->client.context,
                                     manager,
                                     manager->wallet,
                                     (BRWalletEvent) {
                                         BITCOIN_WALLET_BALANCE_UPDATED,
                                         { .balance = { balanceInSatoshi }}
                                     });
}

static void
_BRWalletManagerTxAdded   (void *info,
                           OwnershipKept BRTransaction *tx) {
    BRWalletManager manager = (BRWalletManager) info;
    fileServiceSave(manager->fileService, fileServiceTypeTransactions, tx);

    assert (NULL != manager->client.funcTransactionEvent);
    manager->client.funcTransactionEvent (manager->client.context,
                                          manager,
                                          manager->wallet,
                                          tx,
                                          (BRTransactionEvent) {
                                              BITCOIN_TRANSACTION_ADDED
                                          });
}

static void
_BRWalletManagerTxUpdated (void *info,
                           OwnershipKept const UInt256 *hashes,
                           size_t count,
                           uint32_t blockHeight,
                           uint32_t timestamp) {
    BRWalletManager manager = (BRWalletManager) info;

    for (size_t index = 0; index < count; index++) {
        UInt256 hash = hashes[index];
        BRTransaction *transaction = BRWalletTransactionForHash(manager->wallet, hash);

        // assert timestamp and blockHeight in transaction
        fileServiceSave (manager->fileService, fileServiceTypeTransactions, transaction);

        assert (NULL != manager->client.funcTransactionEvent);
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
_BRWalletManagerTxDeleted (void *info,
                           UInt256 hash,
                           int notifyUser,
                           int recommendRescan) {
    BRWalletManager manager = (BRWalletManager) info;
    fileServiceRemove(manager->fileService, fileServiceTypeTransactions, hash);

    BRTransaction *transaction = BRWalletTransactionForHash(manager->wallet, hash);

    assert (NULL != manager->client.funcTransactionEvent);
    manager->client.funcTransactionEvent (manager->client.context,
                                          manager,
                                          manager->wallet,
                                          transaction,
                                          (BRTransactionEvent) {
                                              BITCOIN_TRANSACTION_DELETED
                                          });
}

static void
_BRWalletManagerTxPublished (void *info,
                             int error) {
    BRWalletManager manager    = ((SubmitTransactionInfo*) info)->manager;
    BRTransaction *transaction = ((SubmitTransactionInfo*) info)->transaction;
    free (info);

    assert  (NULL != manager->client.funcWalletEvent);
    manager->client.funcWalletEvent (manager->client.context,
                                     manager,
                                     manager->wallet,
                                     (BRWalletEvent) {
                                         BITCOIN_WALLET_TRANSACTION_SUBMITTED,
                                         { .submitted = { transaction, error }}
                                     });
}

/// MARK: - Peer Manager Callbacks

static void
_BRWalletManagerSaveBlocks (void *info,
                            int replace,
                            OwnershipKept BRMerkleBlock **blocks,
                            size_t count) {
    BRWalletManager manager = (BRWalletManager) info;

    if (replace) fileServiceClear(manager->fileService, fileServiceTypeBlocks);
    for (size_t index = 0; index < count; index++)
        fileServiceSave (manager->fileService, fileServiceTypeBlocks, blocks[index]);
}

static void
_BRWalletManagerSavePeers  (void *info,
                            int replace,
                            OwnershipKept const BRPeer *peers,
                            size_t count) {
    BRWalletManager manager = (BRWalletManager) info;

    if (replace) fileServiceClear(manager->fileService, fileServiceTypePeers);
    for (size_t index = 0; index < count; index++)
        fileServiceSave (manager->fileService, fileServiceTypePeers, &peers[index]);
}

static void
_BRWalletManagerSyncStarted (void *info) {
    BRWalletManager manager = (BRWalletManager) info;

    assert (NULL != manager->client.funcWalletManagerEvent);
    manager->client.funcWalletManagerEvent (manager->client.context,
                                            manager,
                                            (BRWalletManagerEvent) {
                                                BITCOIN_WALLET_MANAGER_SYNC_STARTED
                                            });
}

static void
_BRWalletManagerSyncStopped (void *info, int reason) {
    BRWalletManager manager = (BRWalletManager) info;

    assert (NULL != manager->client.funcWalletManagerEvent);
    manager->client.funcWalletManagerEvent (manager->client.context,
                                            manager,
                                            (BRWalletManagerEvent) {
                                                BITCOIN_WALLET_MANAGER_SYNC_STOPPED,
                                                { .syncStopped = { reason }}
                                            });
}

static void
_BRWalletManagerTxStatusUpdate (void *info) {
    BRWalletManager manager = (BRWalletManager) info;

    // It is safe to call this here.  This function will call BRPeerManagerLastBlockTimestamp()
    // to get the current block height but that function attempts to take the BRPeerManager
    // mutex.  That mutex IS NOT RECURSIVE and thus a deadlock can occur.  Only this
    // BRPeerManager callback occurs outside of that mutex being held.  Once could call this
    // function in the BRWallet callbacks; but those callbacks are not redundent with this
    // callback as regards a block height change.
    BRWalletManagerCheckHeight (manager);
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
            assert (NULL != bwm->client.funcGetBlockNumber);
            bwm->client.funcGetBlockNumber (bwm->client.context,
                                            bwm,
                                            BRWalletManagerGetThenIncrRequestId (bwm));
            break;

        case SYNC_MODE_P2P_ONLY:
            assert (0);
            break;
    }
}

extern int
bwmAnnounceBlockNumber (BRWalletManager manager,
                        int rid,
                        uint64_t blockNumber) {
    bwmSignalAnnounceBlockNumber (manager, rid, blockNumber);
    return 1;
}

extern int
bwmHandleAnnounceBlockNumber (BRWalletManager manager,
                              int rid,
                              uint64_t blockNumber) {
    BRWalletManagerUpdateHeightIfAppropriate(manager, (int32_t) blockNumber);
    return 1;
}

extern int
bwmAnnounceTransaction (BRWalletManager manager,
                        int id,
                        OwnershipGiven BRTransaction *transaction) {
    bwmSignalAnnounceTransaction (manager, id, transaction);
    return 1;
}

extern int
bwmHandleAnnounceTransaction (BRWalletManager manager,
                              int id,
                              OwnershipGiven BRTransaction *transaction) {
    bwmSyncTransaction (manager, id, transaction);
    return 1;
}

extern void
bwmAnnounceTransactionComplete (BRWalletManager manager,
                                int rid,
                                int success) {
    bwmSignalAnnounceTransactionComplete (manager, rid, success);
}

extern void
bwmHandleAnnounceTransactionComplete (BRWalletManager manager,
                                      int rid,
                                      int success) {
    bwmSyncComplete(manager, rid, success);
}

//
// Periodicaly query the BRD backend to get current status (block number, nonce, balances,
// transactions and logs) The event will be NULL (as specified for a 'period dispatcher' - See
// `eventHandlerSetTimeoutDispatcher()`)
//
static void
bwmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event) {
    BRWalletManager bwm = (BRWalletManager) event->context;

    assert(SYNC_MODE_P2P_ONLY != bwm->mode);

    bwmUpdateBlockNumber(bwm);

    bwmSyncStart(bwm);
}

extern void
bwmAnnounceSubmit (BRWalletManager manager,
                   int rid,
                   OwnershipGiven BRTransaction *transaction,
                   int error) {
    bwmSignalAnnounceSubmit (manager, rid, transaction, error);
}

extern void
bwmHandleAnnounceSubmit (BRWalletManager manager,
                         int rid,
                         OwnershipGiven BRTransaction *transaction,
                         int error) {
    assert (NULL != manager->client.funcWalletEvent);
    manager->client.funcWalletEvent (manager->client.context,
                                     manager,
                                     manager->wallet,
                                     (BRWalletEvent) {
                                         BITCOIN_WALLET_TRANSACTION_SUBMITTED,
                                         { .submitted = { transaction, error }}
                                     });
}

///
/// MARK: Sync
//

static void
bwmSyncReset (BRWalletManager bwm) {
    assert (SYNC_MODE_P2P_ONLY != bwm->mode);
    pthread_mutex_lock (&bwm->lock);
    bwm->brdSync.chunkBegBlockNumber = bwm->brdSync.begBlockNumber;
    bwm->brdSync.chunkEndBlockNumber = bwm->brdSync.endBlockNumber;
    bwm->brdSync.lastInternalAddress = BR_ADDRESS_NONE;
    bwm->brdSync.lastExternalAddress = BR_ADDRESS_NONE;
    bwm->brdSync.completed           = 1;
    pthread_mutex_unlock (&bwm->lock);
}

static void
bwmSyncStart (BRWalletManager bwm) {
    int rid                     = 0;
    size_t addressCount         = 0;
    uint64_t begBlockNumber     = 0;
    uint64_t endBlockNumber     = 0;
    BRAddress *addressArray     = NULL;
    const char **addressStrings = NULL;

    assert (SYNC_MODE_P2P_ONLY != bwm->mode);
    pthread_mutex_lock (&bwm->lock);
    // check if the prior sync has completed.
    if (bwm->brdSync.completed) {
        // update the `endBlockNumber` to the current block height;
        // since this is exclusive on the end height, we need to increment by
        // one to make sure we get the last block
        bwm->brdSync.endBlockNumber = MAX (bwm->blockHeight == 0 ? 0 : bwm->blockHeight + 1,
                                           bwm->brdSync.begBlockNumber);

        // we'll update transactions if there are more blocks to examine
        if (bwm->brdSync.begBlockNumber != bwm->brdSync.endBlockNumber) {

            // save the current requestId
            bwm->brdSync.rid = bwm->requestId++;

            // generate addresses
            BRWalletManagerGenerateUnusedAddrs (bwm);

            // save the last known external and internal addresses
            BRWalletUnusedAddrs(bwm->wallet, &bwm->brdSync.lastExternalAddress, 1, 0);
            BRWalletUnusedAddrs(bwm->wallet, &bwm->brdSync.lastInternalAddress, 1, 1);

            // get the addresses to query the BDB with
            BRWalletManagerGetAllAddrsAsStrings (bwm,
                                                 &addressCount,
                                                 &addressStrings,
                                                 &addressArray);

            // update the chunk range
            bwm->brdSync.chunkBegBlockNumber = bwm->brdSync.begBlockNumber;
            bwm->brdSync.chunkEndBlockNumber = MIN (bwm->brdSync.begBlockNumber + BWM_BRD_SYNC_CHUNK_SIZE,
                                                    bwm->brdSync.endBlockNumber);

            // mark as not completed
            bwm->brdSync.completed = 0;

            // store sync data for callback outside of lock
            begBlockNumber = bwm->brdSync.chunkBegBlockNumber;
            endBlockNumber = bwm->brdSync.chunkEndBlockNumber;
            rid = bwm->brdSync.rid;
        }
    }
    pthread_mutex_unlock (&bwm->lock);

    // addressCount is only set if a) we are querying a chunk for the first time or b) we need to
    // re-query a chunk, as new addresses have been discovered due to to announced transactions.
    // In either case, call back to the client, outside of the wallet manager's lock, asking for
    // transactions.
    if (addressCount) {
        assert (NULL != bwm->client.funcGetTransactions);
        // Callback to 'client' to get all transactions (for all wallet addresses) between
        // a {beg,end}BlockNumber.  The client will gather the transactions and then call
        // bwmAnnounceTransaction()  (for each one or with all of them).
        bwm->client.funcGetTransactions (bwm->client.context,
                                         bwm,
                                         addressStrings,
                                         addressCount,
                                         begBlockNumber,
                                         endBlockNumber,
                                         rid);
    }

    if (addressStrings) {
        free (addressStrings);
    }

    if (addressArray) {
        free (addressArray);
    }
}

static void
bwmSyncTransaction (BRWalletManager bwm,
                    int rid,
                    OwnershipGiven BRTransaction *transaction) {
    assert (SYNC_MODE_P2P_ONLY != bwm->mode);

    pthread_mutex_lock (&bwm->lock);

    // confirm completion is for in-progress sync
    if (rid == bwm->brdSync.rid &&
        !bwm->brdSync.completed &&
        BRTransactionIsSigned (transaction)) {

        BRWalletRegisterTransaction (bwm->wallet, transaction);
        if (BRWalletTransactionForHash (bwm->wallet, transaction->txHash) != transaction) {
            BRTransactionFree (transaction);
        }

    } else {
        BRTransactionFree (transaction);
    }

    pthread_mutex_unlock (&bwm->lock);
}

static void
bwmSyncComplete (BRWalletManager bwm,
                 int rid,
                 int success) {
    size_t addressCount         = 0;
    uint64_t begBlockNumber     = 0;
    uint64_t endBlockNumber     = 0;
    BRAddress *addressArray     = NULL;
    const char **addressStrings = NULL;

    assert (SYNC_MODE_P2P_ONLY != bwm->mode);

    pthread_mutex_lock (&bwm->lock);

    // confirm completion is for in-progress sync
    if (rid == bwm->brdSync.rid &&
        !bwm->brdSync.completed) {
        // check for a successful completion
        if (success) {
            BRAddress externalAddress = BR_ADDRESS_NONE;
            BRAddress internalAddress = BR_ADDRESS_NONE;

            // generate addresses
            BRWalletManagerGenerateUnusedAddrs (bwm);

            // get the first unused address
            BRWalletUnusedAddrs (bwm->wallet, &externalAddress, 1, 0);
            BRWalletUnusedAddrs (bwm->wallet, &internalAddress, 1, 1);

            // check if the first unused addresses have changed since last completion
            if (!BRAddressEq (&externalAddress, &bwm->brdSync.lastExternalAddress) ||
                !BRAddressEq (&internalAddress, &bwm->brdSync.lastInternalAddress)) {
                // ... we've discovered a new address (i.e. there were transactions announce)
                // so we need to requery the same range including the newly derived addresses

                // store the first unused addresses for comparison in the next complete call
                bwm->brdSync.lastExternalAddress = externalAddress;
                bwm->brdSync.lastInternalAddress = internalAddress;

                // get the addresses to query the BDB with
                BRWalletManagerGetAllAddrsAsStrings (bwm,
                                                    &addressCount,
                                                    &addressStrings,
                                                    &addressArray);

                // don't need to alter the range (we haven't found all transactions yet)

                // store sync data for callback outside of lock
                begBlockNumber = bwm->brdSync.chunkBegBlockNumber;
                endBlockNumber = bwm->brdSync.chunkEndBlockNumber;

            } else if (bwm->brdSync.chunkEndBlockNumber != bwm->brdSync.endBlockNumber) {
                // .. we haven't discovered any new addresses but we haven't gone through the whole range yet

                // don't need to store the first unused addresses (we just confirmed they are equal)

                // get the addresses to query the BDB with
                BRWalletManagerGetAllAddrsAsStrings (bwm,
                                                    &addressCount,
                                                    &addressStrings,
                                                    &addressArray);

                // store the new range
                bwm->brdSync.chunkBegBlockNumber = bwm->brdSync.chunkEndBlockNumber;
                bwm->brdSync.chunkEndBlockNumber = MIN (bwm->brdSync.chunkEndBlockNumber + BWM_BRD_SYNC_CHUNK_SIZE,
                                                        bwm->brdSync.endBlockNumber);

                // store sync data for callback outside of lock
                begBlockNumber = bwm->brdSync.chunkBegBlockNumber;
                endBlockNumber = bwm->brdSync.chunkEndBlockNumber;

            } else {
                // .. we haven't discovered any new addresses and we just finished the last chunk

                // reset sync state and advance the sync range by updating `begBlockNumber`
                bwm->brdSync.completed           = 1;
                bwm->brdSync.lastInternalAddress = BR_ADDRESS_NONE;
                bwm->brdSync.lastExternalAddress = BR_ADDRESS_NONE;
                bwm->brdSync.begBlockNumber      = (bwm->brdSync.endBlockNumber >=  BWM_BRD_SYNC_START_BLOCK_OFFSET
                                                    ? bwm->brdSync.endBlockNumber - BWM_BRD_SYNC_START_BLOCK_OFFSET
                                                    : 0);
            }
        } else {
            // reset sync state on failure
            bwm->brdSync.lastInternalAddress = BR_ADDRESS_NONE;
            bwm->brdSync.lastExternalAddress = BR_ADDRESS_NONE;
            bwm->brdSync.completed           = 1;
        }
    }

    pthread_mutex_unlock (&bwm->lock);

    // addressCount is only set if a) we are querying a chunk for the first time or b) we need to
    // re-query a chunk, as new addresses have been discovered due to to announced transactions.
    // In either case, call back to the client, outside of the wallet manager's lock, asking for
    // transactions.
    if (addressCount) {
        assert (NULL != bwm->client.funcGetTransactions);
        // Callback to 'client' to get all transactions (for all wallet addresses) between
        // a {beg,end}BlockNumber.  The client will gather the transactions and then call
        // bwmAnnounceTransaction()  (for each one or with all of them).
        bwm->client.funcGetTransactions (bwm->client.context,
                                         bwm,
                                         addressStrings,
                                         addressCount,
                                         begBlockNumber,
                                         endBlockNumber,
                                         rid);
    }

    if (addressStrings) {
        free (addressStrings);
    }

    if (addressArray) {
        free (addressArray);
    }
}
