//  BRSyncManager.h
//
//  Created by Michael Carrara on 12/08/19.
//  Copyright (c) 2019 breadwallet LLC.
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

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>

#include "support/BRArray.h"
#include "BRSyncManager.h"
#include "BRPeerManager.h"

/// MARK: - Common Decls & Defs

#if !defined (MAX)
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#if !defined (MIN)
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

typedef enum {
    SYNC_MANAGER_CSTATE_CONNECTED,
    SYNC_MANAGER_CSTATE_DISCONNECTED,
} BRSyncManagerConnectionState;

struct BRSyncManagerStruct {
    BRSyncMode mode;
};

/// MARK: - Sync Manager Decls & Defs

struct BRClientSyncManagerStruct {
    // !!! must be first !!!
    struct BRSyncManagerStruct common;

    // immutables
    BRWallet *wallet;

    BRSyncManagerEventContext eventContext;
    BRSyncManagerEventCallback eventCallback;

    BRSyncManagerClientContext clientContext;
    BRSyncManagerClientCallbacks clientCallbacks;

    uint32_t initBlockHeight;

    pthread_mutex_t lock;

    // mutables
    uint32_t syncedBlockHeight;
    uint32_t networkBlockHeight;
    BRSyncManagerConnectionState connectionState;

    /**
     * An identiifer generator for a BRD Request
     */
    unsigned int requestIdGenerator;

    /**
     * If we are syncing with BRD, instead of as P2P with PeerManager, then we'll keep a record to
     * ensure we've successfully completed the getTransactions() callbacks to the client.
     *
     * We sync, using chunks, through the total range being synced on rather than using the range
     * in its entirety.
     *
     *  The reasons for this are:
     *
     * 1) As transactions are announced, the set of addresses that need to be queried for transactions
     *    will grow. By splitting the range into smaller chunks, we will pick up addresses as we move
     *    through the range.
     *
     * 2) Chunking the sync range allows us to measure progress organically. If the whole range was
     *    requested, we would need to enhance the client/announceCallback relationship to provide
     *    additional data points on its progress and add complexity as a byproduct.
     *
     * 3) For naive implemenations that announce transactions once all of them have been discovered,
     *    the use of chunking forces them to announce transactions more frequently. This should
     *    ultimately lead to a more responsive user experience.
     */
    struct {
        int requestId;
        BRAddress lastExternalAddress;
        BRAddress lastInternalAddress;
        uint64_t begBlockNumber;
        uint64_t endBlockNumber;
        uint64_t chunkSize;
        uint64_t chunkBegBlockNumber;
        uint64_t chunkEndBlockNumber;
        int isFullSync:1;
    } brdSync;
};

typedef struct BRClientSyncManagerStruct * BRClientSyncManager;

// When using a BRD sync, offset the start block by N days of Bitcoin blocks; the value of N is
// assumed to be 'the maximum number of days that the blockchain DB could be behind'
#define BWM_MINUTES_PER_BLOCK                   10              // assumed, bitcoin
#define BWM_BRD_SYNC_DAYS_OFFSET                 1
#define BWM_BRD_SYNC_START_BLOCK_OFFSET        ((BWM_BRD_SYNC_DAYS_OFFSET * 24 * 60) / BWM_MINUTES_PER_BLOCK)
#define BWM_BRD_SYNC_CHUNK_SIZE                 50000

#define BRClientSyncManagerAsSyncManager(x)     ((BRSyncManager) (x))

static BRClientSyncManager
BRClientSyncManagerNew(BRSyncManagerEventContext eventContext,
                       BRSyncManagerEventCallback eventCallback,
                       BRSyncManagerClientContext clientContext,
                       BRSyncManagerClientCallbacks clientCallbacks,
                       const BRChainParams *params,
                       BRWallet *wallet,
                       uint32_t earliestKeyTime,
                       uint32_t blockHeight,
                       BRMerkleBlock *blocks[],
                       size_t blocksCount,
                       const BRPeer peers[],
                       size_t peersCount);

static BRClientSyncManager
BRSyncManagerAsClientSyncManager(BRSyncManager manager);

static void
BRClientSyncManagerFree(BRClientSyncManager manager);

static void
BRClientSyncManagerConnect(BRClientSyncManager manager);

static void
BRClientSyncManagerDisconnect(BRClientSyncManager manager);

static void
BRClientSyncManagerScan(BRClientSyncManager manager);

static void
BRClientSyncManagerSubmit(BRClientSyncManager manager,
                          BRTransaction *transaction);

static void
BRClientSyncManagerTickTock(BRClientSyncManager manager);

static void
BRClientSyncManagerAnnounceGetBlockNumber(BRClientSyncManager manager,
                                          int rid,
                                          uint32_t blockHeight);

static void
BRClientSyncManagerAnnounceGetTransactionsItem (BRClientSyncManager manager,
                                                int rid,
                                                BRTransaction *transaction);

static void
BRClientSyncManagerAnnounceGetTransactionsDone (BRClientSyncManager manager,
                                                int rid,
                                                int success);

static void
BRClientSyncManagerAnnounceSubmitTransaction (BRClientSyncManager manager,
                                              int rid,
                                              BRTransaction *transaction,
                                              int error);

static void
BRClientSyncManagerUpdateBlockNumber (BRClientSyncManager manager);

static void
BRClientSyncManagerStartSyncIfNeeded (BRClientSyncManager manager);

static unsigned int
BRClientSyncManagerGenerateRid (BRClientSyncManager manager);

static unsigned int
BRClientSyncManagerGenerateRidLocked (BRClientSyncManager manager);

static void
BRClientSyncManagerAddressToLegacy (BRClientSyncManager manager,
                                    BRAddress *addr);

static void
BRClientSyncManagerGenerateUnusedAddrs (BRClientSyncManager manager);

static BRAddress *
BRClientSyncManagerGetAllAddrs (BRClientSyncManager manager,
                                size_t *addressCount);

static void
BRClientSyncManagerGetAllAddrsAsStrings (BRClientSyncManager manager,
                                         size_t *addressCount,
                                         const char ***addressStrings,
                                         BRAddress **addressArray);

/// MARK: - Peer Sync Manager Decls & Defs

struct BRPeerSyncManagerStruct {
    // !!! must be first !!!
    struct BRSyncManagerStruct common;

    // immutables
    BRWallet *wallet;
    BRPeerManager *peerManager;

    BRSyncManagerEventContext eventContext;
    BRSyncManagerEventCallback eventCallback;

    pthread_mutex_t lock;

    // mutables
    uint32_t networkBlockHeight;
    BRSyncManagerConnectionState connectionState;
};

typedef struct BRPeerSyncManagerStruct * BRPeerSyncManager;

#define BRPeerSyncManagerAsSyncManager(x)     ((BRSyncManager) (x))

static BRPeerSyncManager
BRPeerSyncManagerNew(BRSyncManagerEventContext eventContext,
                     BRSyncManagerEventCallback eventCallback,
                     const BRChainParams *params,
                     BRWallet *wallet,
                     uint32_t earliestKeyTime,
                     uint32_t blockHeight,
                     BRMerkleBlock *blocks[],
                     size_t blocksCount,
                     const BRPeer peers[],
                     size_t peersCount);

static BRPeerSyncManager
BRSyncManagerAsPeerSyncManager(BRSyncManager manager);

static void
BRPeerSyncManagerFree(BRPeerSyncManager);

static void
BRPeerSyncManagerConnect(BRPeerSyncManager manager);

static void
BRPeerSyncManagerDisconnect(BRPeerSyncManager manager);

static void
BRPeerSyncManagerScan(BRPeerSyncManager manager);

static void
BRPeerSyncManagerSubmit(BRPeerSyncManager manager, BRTransaction *transaction);

static void
BRPeerSyncManagerTickTock(BRPeerSyncManager manager);

// PeerManager callbacks
static void _BRPeerSyncManagerSyncStarted (void *info);
static void _BRPeerSyncManagerSyncStopped (void *info, int reason);
static void _BRPeerSyncManagerTxStatusUpdate (void *info);
static void _BRPeerSyncManagerSaveBlocks (void *info, int replace, BRMerkleBlock **blocks, size_t count);
static void _BRPeerSyncManagerSavePeers  (void *info, int replace, const BRPeer *peers, size_t count);
static int  _BRPeerSyncManagerNetworkIsReachabele (void *info);
static void _BRPeerSyncManagerThreadCleanup (void *info);
static void _BRPeerSyncManagerTxPublished (void *info, int error);

/// MARK: - Sync Manager Implementation

extern BRSyncManager
BRSyncManagerNewForMode(BRSyncMode mode,
                        BRSyncManagerEventContext eventContext,
                        BRSyncManagerEventCallback eventCallback,
                        BRSyncManagerClientContext clientContext,
                        BRSyncManagerClientCallbacks clientCallbacks,
                        const BRChainParams *params,
                        BRWallet *wallet,
                        uint32_t earliestKeyTime,
                        uint32_t blockHeight,
                        BRMerkleBlock *blocks[],
                        size_t blocksCount,
                        const BRPeer peers[],
                        size_t peersCount) {
    switch (mode) {
        case SYNC_MODE_BRD_ONLY:
        return BRSyncManagerNewForClient (eventContext,
                                          eventCallback,
                                          clientContext,
                                          clientCallbacks,
                                          params,
                                          wallet,
                                          earliestKeyTime,
                                          blockHeight,
                                          blocks,
                                          blocksCount,
                                          peers,
                                          peersCount);
        case SYNC_MODE_P2P_ONLY:
        return BRSyncManagerNewForP2P (eventContext,
                                       eventCallback,
                                       params,
                                       wallet,
                                       earliestKeyTime,
                                       blockHeight,
                                       blocks,
                                       blocksCount,
                                       peers,
                                       peersCount);
        default:
        assert (0);
        return NULL;
    }
}

extern BRSyncManager
BRSyncManagerNewForClient(BRSyncManagerEventContext eventContext,
                          BRSyncManagerEventCallback eventCallback,
                          BRSyncManagerClientContext clientContext,
                          BRSyncManagerClientCallbacks clientCallbacks,
                          const BRChainParams *params,
                          BRWallet *wallet,
                          uint32_t earliestKeyTime,
                          uint32_t blockHeight,
                          BRMerkleBlock *blocks[],
                          size_t blocksCount,
                          const BRPeer peers[],
                          size_t peersCount) {
    assert (NULL != eventCallback);
    assert (NULL != params);
    assert (NULL != wallet);
    assert(blocks != NULL || blocksCount == 0);
    assert(peers != NULL || peersCount == 0);

    BRClientSyncManager clientManager = BRClientSyncManagerNew(eventContext,
                                                               eventCallback,
                                                               clientContext,
                                                               clientCallbacks,
                                                               params,
                                                               wallet,
                                                               earliestKeyTime,
                                                               blockHeight,
                                                               blocks,
                                                               blocksCount,
                                                               peers,
                                                               peersCount);
    return BRClientSyncManagerAsSyncManager (clientManager);
}

extern BRSyncManager
BRSyncManagerNewForP2P(BRSyncManagerEventContext eventContext,
                       BRSyncManagerEventCallback eventCallback,
                       const BRChainParams *params,
                       BRWallet *wallet,
                       uint32_t earliestKeyTime,
                       uint32_t blockHeight,
                       BRMerkleBlock *blocks[],
                       size_t blocksCount,
                       const BRPeer peers[],
                       size_t peersCount) {
    assert (NULL != eventCallback);
    assert (NULL != params);
    assert (NULL != wallet);
    assert(blocks != NULL || blocksCount == 0);
    assert(peers != NULL || peersCount == 0);

    BRPeerSyncManager peerManager = BRPeerSyncManagerNew(eventContext,
                                                         eventCallback,
                                                         params,
                                                         wallet,
                                                         earliestKeyTime,
                                                         blockHeight,
                                                         blocks,
                                                         blocksCount,
                                                         peers,
                                                         peersCount);
    return BRPeerSyncManagerAsSyncManager (peerManager);
}

extern void
BRSyncManagerFree(BRSyncManager manager) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
        BRClientSyncManagerFree (BRSyncManagerAsClientSyncManager (manager));
        break;
        case SYNC_MODE_P2P_ONLY:
        BRPeerSyncManagerFree (BRSyncManagerAsPeerSyncManager (manager));
        break;
        default:
        assert (0);
        break;
    }
}

extern void
BRSyncManagerConnect(BRSyncManager manager) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
        BRClientSyncManagerConnect (BRSyncManagerAsClientSyncManager (manager));
        break;
        case SYNC_MODE_P2P_ONLY:
        BRPeerSyncManagerConnect (BRSyncManagerAsPeerSyncManager (manager));
        break;
        default:
        assert (0);
        break;
    }
}

extern void
BRSyncManagerDisconnect(BRSyncManager manager) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
        BRClientSyncManagerDisconnect (BRSyncManagerAsClientSyncManager (manager));
        break;
        case SYNC_MODE_P2P_ONLY:
        BRPeerSyncManagerDisconnect (BRSyncManagerAsPeerSyncManager (manager));
        break;
        default:
        assert (0);
        break;
    }
}

extern void
BRSyncManagerScan(BRSyncManager manager) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
        BRClientSyncManagerScan (BRSyncManagerAsClientSyncManager (manager));
        break;
        case SYNC_MODE_P2P_ONLY:
        BRPeerSyncManagerScan (BRSyncManagerAsPeerSyncManager (manager));
        break;
        default:
        assert (0);
        break;
    }
}

extern void
BRSyncManagerSubmit(BRSyncManager manager,
                    BRTransaction *transaction) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
        BRClientSyncManagerSubmit (BRSyncManagerAsClientSyncManager (manager),
                                   transaction);
        break;
        case SYNC_MODE_P2P_ONLY:
        BRPeerSyncManagerSubmit (BRSyncManagerAsPeerSyncManager (manager),
                                 transaction);
        break;
        default:
        assert (0);
        break;
    }
}

extern void
BRSyncManagerTickTock(BRSyncManager manager) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
        BRClientSyncManagerTickTock (BRSyncManagerAsClientSyncManager (manager));
        break;
        case SYNC_MODE_P2P_ONLY:
        BRPeerSyncManagerTickTock (BRSyncManagerAsPeerSyncManager (manager));
        break;
        default:
        assert (0);
        break;
    }
}

extern void
BRSyncManagerAnnounceGetBlockNumber(BRSyncManager manager,
                                    int rid,
                                    uint32_t blockHeight) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
        BRClientSyncManagerAnnounceGetBlockNumber (BRSyncManagerAsClientSyncManager (manager),
                                                   rid,
                                                   blockHeight);
        break;
        case SYNC_MODE_P2P_ONLY:
        // do nothing
        break;
        default:
        assert (0);
        break;
    }
}

extern void
BRSyncManagerAnnounceGetTransactionsItem(BRSyncManager manager,
                                         int rid,
                                         BRTransaction *transaction) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
        BRClientSyncManagerAnnounceGetTransactionsItem (BRSyncManagerAsClientSyncManager (manager),
                                                        rid,
                                                        transaction);
        break;
        case SYNC_MODE_P2P_ONLY:
        // do nothing
        break;
        default:
        assert (0);
        break;
    }
}

extern void
BRSyncManagerAnnounceGetTransactionsDone(BRSyncManager manager,
                                         int rid,
                                         int success) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
        BRClientSyncManagerAnnounceGetTransactionsDone (BRSyncManagerAsClientSyncManager (manager),
                                                        rid,
                                                        success);
        break;
        case SYNC_MODE_P2P_ONLY:
        // do nothing
        break;
        default:
        assert (0);
        break;
    }
}

extern void
BRSyncManagerAnnounceSubmitTransaction(BRSyncManager manager,
                                       int rid,
                                       BRTransaction *transaction,
                                       int error) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
        BRClientSyncManagerAnnounceSubmitTransaction (BRSyncManagerAsClientSyncManager (manager),
                                                      rid,
                                                      transaction,
                                                      error);
        break;
        case SYNC_MODE_P2P_ONLY:
        // do nothing
        break;
        default:
        assert (0);
        break;
    }
}

/// MARK: - Client Sync Manager Implementation

static BRClientSyncManager
BRClientSyncManagerNew(BRSyncManagerEventContext eventContext,
                       BRSyncManagerEventCallback eventCallback,
                       BRSyncManagerClientContext clientContext,
                       BRSyncManagerClientCallbacks clientCallbacks,
                       const BRChainParams *params,
                       BRWallet *wallet,
                       uint32_t earliestKeyTime,
                       uint32_t blockHeight,
                       BRMerkleBlock *blocks[],
                       size_t blocksCount,
                       const BRPeer peers[],
                       size_t peersCount) {
    BRClientSyncManager manager = (BRClientSyncManager) calloc (1, sizeof(struct BRClientSyncManagerStruct));
    manager->common.mode = SYNC_MODE_BRD_ONLY;

    manager->wallet = wallet;
    manager->eventContext = eventContext;
    manager->eventCallback = eventCallback;
    manager->clientContext = clientContext;
    manager->clientCallbacks = clientCallbacks;

    pthread_mutex_init(&manager->lock, NULL);

    // Find the BRCheckpoint that is at least one week before earliestKeyTime.
#define ONE_WEEK_IN_SECONDS      (7*24*60*60)
    const BRCheckPoint *earliestCheckPoint = BRChainParamsGetCheckpointBefore (params, earliestKeyTime - ONE_WEEK_IN_SECONDS);
    assert (NULL != earliestCheckPoint);

    // Initialize this instance's blockHeight.  This might be out-of-sync with a) the P2P block
    // height which will be derived from the persistently restored blocks and then from the sync()
    // process or b) from the API-based Blockchain DB reported block height which will be updated
    // preriodically when in API sync modes.
    //
    // So, we'll start with the best block height we have and expect it to change. Doing this allows
    // an API-based sync to start immediately rather than waiting for a BRClientSyncManagerUpdateBlockNumber()
    // result in period '1' and then starting the sync in period '2' - where each period is
    // BWM_SLEEP_SECONDS and at least 1 minute.
    manager->initBlockHeight    = MIN (earliestCheckPoint->height, blockHeight);
    manager->syncedBlockHeight  = manager->initBlockHeight;
    manager->networkBlockHeight = MAX (earliestCheckPoint->height, blockHeight);
    manager->connectionState    = SYNC_MANAGER_CSTATE_DISCONNECTED;

    // The `brdSync` struct is zeroed out by the calloc call

    return manager;
}

static BRClientSyncManager
BRSyncManagerAsClientSyncManager(BRSyncManager manager) {
    if (manager->mode == SYNC_MODE_BRD_ONLY) {
        return (BRClientSyncManager) manager;
    }
    assert (0);
    return NULL;
}

static void
BRClientSyncManagerFree(BRClientSyncManager manager) {
    // TODO(fix): Create a centralized version of this
    pthread_mutex_destroy(&manager->lock);
    memset (manager, 0, sizeof(*manager));
    free (manager);
}

static void
BRClientSyncManagerConnect(BRClientSyncManager manager) {
    uint8_t needConnectionEvent = 0;
    BRSyncManagerEvent connectionEvent = (BRSyncManagerEvent) { SYNC_MANAGER_CONNECTED };

    pthread_mutex_lock (&manager->lock);
    if (SYNC_MANAGER_CSTATE_DISCONNECTED == manager->connectionState) {
        manager->connectionState = SYNC_MANAGER_CSTATE_CONNECTED;
        needConnectionEvent = 1;
    }
    pthread_mutex_unlock (&manager->lock);

    if (needConnectionEvent) {
        manager->eventCallback (manager->eventContext,
                                BRClientSyncManagerAsSyncManager (manager),
                                connectionEvent);
    }

    BRClientSyncManagerStartSyncIfNeeded (manager);
}

static void
BRClientSyncManagerDisconnect(BRClientSyncManager manager) {
    uint8_t needConnectionEvent = 0;
    BRSyncManagerEvent connectionEvent = (BRSyncManagerEvent) { SYNC_MANAGER_DISCONNECTED };

    uint8_t needSyncingEvent = 0;
    BRSyncManagerEvent syncingEvent = (BRSyncManagerEvent) { SYNC_MANAGER_SYNC_STOPPED, { .syncStopped = { -1 }} };

    pthread_mutex_lock (&manager->lock);
    if (SYNC_MANAGER_CSTATE_CONNECTED == manager->connectionState) {
        manager->connectionState = SYNC_MANAGER_CSTATE_DISCONNECTED;
        needConnectionEvent = 1;
        needSyncingEvent = manager->brdSync.isFullSync;
        memset(&manager->brdSync, 0, sizeof(manager->brdSync));
    }
    pthread_mutex_unlock (&manager->lock);

    if (needConnectionEvent) {
        manager->eventCallback (manager->eventContext,
                                BRClientSyncManagerAsSyncManager (manager),
                                connectionEvent);
    }

    if (needSyncingEvent) {
        manager->eventCallback (manager->eventContext,
                                BRClientSyncManagerAsSyncManager (manager),
                                syncingEvent);
    }
}

static void
BRClientSyncManagerScan(BRClientSyncManager manager) {
    uint8_t needConnectionEvent = 0;
    BRSyncManagerEvent connectionEvent = (BRSyncManagerEvent) { SYNC_MANAGER_CONNECTED };

    uint8_t needSyncingEvent = 0;
    BRSyncManagerEvent syncingEvent = (BRSyncManagerEvent) { SYNC_MANAGER_SYNC_STOPPED, { .syncStopped = { -1 }} };

    pthread_mutex_lock (&manager->lock);
    if (SYNC_MANAGER_CSTATE_DISCONNECTED == manager->connectionState) {
        manager->connectionState = SYNC_MANAGER_CSTATE_CONNECTED;
        needConnectionEvent = 1;
    } else {
        needSyncingEvent = manager->brdSync.isFullSync;
        memset(&manager->brdSync, 0, sizeof(manager->brdSync));
    }

    manager->syncedBlockHeight = manager->initBlockHeight;
    pthread_mutex_unlock (&manager->lock);

    if (needConnectionEvent) {
        manager->eventCallback (manager->eventContext,
                                BRClientSyncManagerAsSyncManager (manager),
                                connectionEvent);
    }

    if (needSyncingEvent) {
        manager->eventCallback (manager->eventContext,
                                BRClientSyncManagerAsSyncManager (manager),
                                syncingEvent);
    }

    BRClientSyncManagerStartSyncIfNeeded (manager);
}

static void
BRClientSyncManagerSubmit(BRClientSyncManager manager,
                          BRTransaction *transaction) {
    manager->clientCallbacks.funcSubmitTransaction (manager->clientContext,
                                                    BRClientSyncManagerAsSyncManager (manager),
                                                    transaction,
                                                    BRClientSyncManagerGenerateRid (manager));
}

static void
BRClientSyncManagerTickTock(BRClientSyncManager manager) {
    BRClientSyncManagerUpdateBlockNumber (manager);
    BRClientSyncManagerStartSyncIfNeeded (manager);
}

static void
BRClientSyncManagerAnnounceGetBlockNumber(BRClientSyncManager manager,
                                          int rid,
                                          uint32_t blockHeight) {
    uint8_t needBlockHeightEvent = 0;
    BRSyncManagerEvent blockHeightEvent = {0};

    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (blockHeight > manager->networkBlockHeight) {
            manager->networkBlockHeight = blockHeight;
            needBlockHeightEvent = 1;
            blockHeightEvent = (BRSyncManagerEvent) { SYNC_MANAGER_BLOCK_HEIGHT_UPDATED, { .blockHeightUpdated = { blockHeight }} };
        }

        pthread_mutex_unlock (&manager->lock);
    }

    if (needBlockHeightEvent) {
        manager->eventCallback (manager->eventContext,
                                BRClientSyncManagerAsSyncManager (manager),
                                blockHeightEvent);
    }
}

static void
BRClientSyncManagerAnnounceSubmitTransaction (BRClientSyncManager manager,
                                              int rid,
                                              BRTransaction *transaction,
                                              int error) {
    manager->eventCallback (manager->eventContext,
                            BRClientSyncManagerAsSyncManager (manager),
                            (BRSyncManagerEvent) {
                                SYNC_MANAGER_TXN_SUBMITTED,
                                { .submitted = {transaction, error} },
                            });
}

static void
BRClientSyncManagerUpdateBlockNumber(BRClientSyncManager manager) {
    manager->clientCallbacks.funcGetBlockNumber (manager->clientContext,
                                                 BRClientSyncManagerAsSyncManager (manager),
                                                 BRClientSyncManagerGenerateRid (manager));
}

static void
BRClientSyncManagerStartSyncIfNeeded (BRClientSyncManager manager) {
    int rid                     = 0;
    uint8_t needSyncEvent       = 0;
    uint8_t needGetRequest      = 0;
    size_t addressCount         = 0;
    uint64_t begBlockNumber     = 0;
    uint64_t endBlockNumber     = 0;
    BRAddress *addressArray     = NULL;
    const char **addressStrings = NULL;

    pthread_mutex_lock (&manager->lock);
    // check if we are connect and the prior sync has completed.
    if (0 == manager->brdSync.requestId &&
        SYNC_MANAGER_CSTATE_CONNECTED == manager->connectionState) {
        // update the `endBlockNumber` to the current block height;
        // since this is exclusive on the end height, we need to increment by
        // one to make sure we get the last block
        manager->brdSync.endBlockNumber = MAX (manager->syncedBlockHeight, manager->networkBlockHeight) + 1;

        // update the `startBlockNumber` to the last synced height;
        // provide a bit of buffer and request the last X blocks, regardless
        manager->brdSync.begBlockNumber = MIN (manager->syncedBlockHeight, (manager->brdSync.endBlockNumber >=  BWM_BRD_SYNC_START_BLOCK_OFFSET
                                                                            ? manager->brdSync.endBlockNumber - BWM_BRD_SYNC_START_BLOCK_OFFSET
                                                                            : 0));

        // update the chunk range
        manager->brdSync.chunkSize = BWM_BRD_SYNC_CHUNK_SIZE;
        manager->brdSync.chunkBegBlockNumber = manager->brdSync.begBlockNumber;
        manager->brdSync.chunkEndBlockNumber = MIN (manager->brdSync.begBlockNumber + manager->brdSync.chunkSize,
                                                    manager->brdSync.endBlockNumber);

        // generate addresses
        BRClientSyncManagerGenerateUnusedAddrs (manager);

        // save the last known external and internal addresses
        BRWalletUnusedAddrs(manager->wallet, &manager->brdSync.lastExternalAddress, 1, 0);
        BRWalletUnusedAddrs(manager->wallet, &manager->brdSync.lastInternalAddress, 1, 1);

        // get the addresses to query the BDB with
        BRClientSyncManagerGetAllAddrsAsStrings (manager,
                                                 &addressCount,
                                                 &addressStrings,
                                                 &addressArray);

        // save the current requestId
        manager->brdSync.requestId = BRClientSyncManagerGenerateRidLocked (manager);

        // mark as sync or not
        manager->brdSync.isFullSync = (manager->brdSync.endBlockNumber - manager->brdSync.begBlockNumber) > BWM_BRD_SYNC_START_BLOCK_OFFSET;

        // store sync data for callback outside of lock
        begBlockNumber = manager->brdSync.chunkBegBlockNumber;
        endBlockNumber = manager->brdSync.chunkEndBlockNumber;
        rid = manager->brdSync.requestId;

        // store control flow flags
        needSyncEvent = manager->brdSync.isFullSync;
        needGetRequest = 1;
    }
    pthread_mutex_unlock (&manager->lock);

    if (needSyncEvent) {
        manager->eventCallback (manager->eventContext,
                                BRClientSyncManagerAsSyncManager (manager),
                                (BRSyncManagerEvent) {
                                    SYNC_MANAGER_SYNC_STARTED,
                                });
    }

    if (needGetRequest) {
        // Callback to 'client' to get all transactions (for all wallet addresses) between
        // a {beg,end}BlockNumber.  The client will gather the transactions and then call
        // bwmAnnounceTransaction()  (for each one or with all of them).
        manager->clientCallbacks.funcGetTransactions (manager->clientContext,
                                                      BRClientSyncManagerAsSyncManager (manager),
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
BRClientSyncManagerAnnounceGetTransactionsItem (BRClientSyncManager manager,
                                                int rid,
                                                BRTransaction *transaction) {
    pthread_mutex_lock (&manager->lock);

    // confirm completion is for in-progress sync
    if (rid == manager->brdSync.requestId &&
        SYNC_MANAGER_CSTATE_CONNECTED == manager->connectionState &&
        BRTransactionIsSigned (transaction)) {

        BRWalletRegisterTransaction (manager->wallet, transaction);
        if (BRWalletTransactionForHash (manager->wallet, transaction->txHash) != transaction) {
            BRTransactionFree (transaction);
        }

    } else {
        BRTransactionFree (transaction);
    }

    pthread_mutex_unlock (&manager->lock);
}

static void
BRClientSyncManagerAnnounceGetTransactionsDone (BRClientSyncManager manager,
                                                int rid,
                                                int success) {
    size_t addressCount          = 0;
    uint8_t needSyncEvent        = 0;
    uint8_t needGetRequest       = 0;
    uint64_t begBlockNumber      = 0;
    uint64_t endBlockNumber      = 0;
    BRAddress *addressArray      = NULL;
    const char **addressStrings  = NULL;
    BRSyncManagerEvent syncEvent = {0};

    pthread_mutex_lock (&manager->lock);

    // confirm completion is for in-progress sync
    if (rid == manager->brdSync.requestId &&
        SYNC_MANAGER_CSTATE_CONNECTED == manager->connectionState) {
        // check for a successful completion
        if (success) {
            BRAddress externalAddress = BR_ADDRESS_NONE;
            BRAddress internalAddress = BR_ADDRESS_NONE;

            // generate addresses
            BRClientSyncManagerGenerateUnusedAddrs (manager);

            // get the first unused address
            BRWalletUnusedAddrs (manager->wallet, &externalAddress, 1, 0);
            BRWalletUnusedAddrs (manager->wallet, &internalAddress, 1, 1);

            // check if the first unused addresses have changed since last completion
            if (!BRAddressEq (&externalAddress, &manager->brdSync.lastExternalAddress) ||
                !BRAddressEq (&internalAddress, &manager->brdSync.lastInternalAddress)) {
                // ... we've discovered a new address (i.e. there were transactions announce)
                // so we need to requery the same range including the newly derived addresses

                // store the first unused addresses for comparison in the next complete call
                manager->brdSync.lastExternalAddress = externalAddress;
                manager->brdSync.lastInternalAddress = internalAddress;

                // get the addresses to query the BDB with
                BRClientSyncManagerGetAllAddrsAsStrings (manager,
                                                         &addressCount,
                                                         &addressStrings,
                                                         &addressArray);

                // don't need to alter the range (we haven't found all transactions yet)

                // store sync data for callback outside of lock
                begBlockNumber = manager->brdSync.chunkBegBlockNumber;
                endBlockNumber = manager->brdSync.chunkEndBlockNumber;

                // store control flow flags
                needGetRequest = 1;

            } else if (manager->brdSync.chunkEndBlockNumber != manager->brdSync.endBlockNumber) {
                // .. we haven't discovered any new addresses but we haven't gone through the whole range yet

                // don't need to store the first unused addresses (we just confirmed they are equal)

                // get the addresses to query the BDB with
                BRClientSyncManagerGetAllAddrsAsStrings (manager,
                                                         &addressCount,
                                                         &addressStrings,
                                                         &addressArray);

                // store the new range
                manager->brdSync.chunkBegBlockNumber = manager->brdSync.chunkEndBlockNumber;
                manager->brdSync.chunkEndBlockNumber = MIN (manager->brdSync.chunkEndBlockNumber + manager->brdSync.chunkSize,
                                                            manager->brdSync.endBlockNumber);

                // store sync data for callback outside of lock
                begBlockNumber = manager->brdSync.chunkBegBlockNumber;
                endBlockNumber = manager->brdSync.chunkEndBlockNumber;

                // store control flow flags
                needGetRequest = 1;
                needSyncEvent = 1;
                syncEvent = (BRSyncManagerEvent) {
                    SYNC_MANAGER_SYNC_PROGRESS,
                    { .syncProgress = { (((manager->brdSync.chunkBegBlockNumber - manager->brdSync.begBlockNumber) * 100) /
                                         (manager->brdSync.endBlockNumber - manager->brdSync.begBlockNumber)) }}
                };

            } else {
                // .. we haven't discovered any new addresses and we just finished the last chunk

                // store synced block height
                manager->syncedBlockHeight = manager->brdSync.endBlockNumber - 1;

                // store control flow flags
                needSyncEvent = manager->brdSync.isFullSync;
                syncEvent = (BRSyncManagerEvent) {SYNC_MANAGER_SYNC_STOPPED, { .syncStopped = { 0 }}};

                // reset sync state
                memset(&manager->brdSync, 0, sizeof(manager->brdSync));

            }
        } else {
            // store control flow flags
            needSyncEvent = manager->brdSync.isFullSync;
            syncEvent = (BRSyncManagerEvent) {SYNC_MANAGER_SYNC_STOPPED, { .syncStopped = { -1 }}};

            // reset sync state on failure
            memset(&manager->brdSync, 0, sizeof(manager->brdSync));
        }
    }

    pthread_mutex_unlock (&manager->lock);

    if (needSyncEvent) {
        manager->eventCallback (manager->eventContext,
                                BRClientSyncManagerAsSyncManager (manager),
                                syncEvent);
    }

    if (needGetRequest) {
        // Callback to 'client' to get all transactions (for all wallet addresses) between
        // a {beg,end}BlockNumber.  The client will gather the transactions and then call
        // bwmAnnounceTransaction()  (for each one or with all of them).
        manager->clientCallbacks.funcGetTransactions (manager->clientContext,
                                                      BRClientSyncManagerAsSyncManager (manager),
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

static unsigned int
BRClientSyncManagerGenerateRid (BRClientSyncManager manager) {
    unsigned int requestId;
    pthread_mutex_lock (&manager->lock);
    requestId = BRClientSyncManagerGenerateRidLocked (manager);
    pthread_mutex_unlock (&manager->lock);
    return requestId;
}

static unsigned int
BRClientSyncManagerGenerateRidLocked (BRClientSyncManager manager) {
    return ++manager->requestIdGenerator;
}

static void
BRClientSyncManagerAddressToLegacy (BRClientSyncManager manager,
                                    BRAddress *addr) {
    *addr = BRWalletAddressToLegacy (manager->wallet, addr);
}

static void
BRClientSyncManagerGenerateUnusedAddrs (BRClientSyncManager manager) {
    BRWalletUnusedAddrs (manager->wallet, NULL, SEQUENCE_GAP_LIMIT_EXTERNAL, 0);
    BRWalletUnusedAddrs (manager->wallet, NULL, SEQUENCE_GAP_LIMIT_INTERNAL, 1);
}

static BRAddress *
BRClientSyncManagerGetAllAddrs (BRClientSyncManager manager,
                                size_t *addressCount) {
    assert (addressCount);

    size_t addrCount = BRWalletAllAddrs (manager->wallet, NULL, 0);

    BRAddress *addrs = (BRAddress *) calloc (2 * addrCount, sizeof (BRAddress));
    BRWalletAllAddrs (manager->wallet, addrs, addrCount);

    memcpy (addrs + addrCount, addrs, addrCount * sizeof(BRAddress));
    for (size_t index = 0; index < addrCount; index++)
        BRClientSyncManagerAddressToLegacy (manager, &addrs[addrCount + index]);

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
BRClientSyncManagerGetAllAddrsAsStrings (BRClientSyncManager manager,
                                         size_t *addressCount,
                                         const char ***addressStrings,
                                         BRAddress **addressArray) {
    size_t addrCount = 0;
    BRAddress *addrArray = BRClientSyncManagerGetAllAddrs (manager, &addrCount);

    const char **addrsStrings = calloc (addrCount, sizeof(char *));
    for (size_t index = 0; index < addrCount; index ++)
        addrsStrings[index] = (char *) &addrArray[index];

    *addressCount = addrCount;
    *addressStrings = addrsStrings;
    *addressArray = addrArray;
}

/// MARK: - Peer Sync Manager Implementation

static BRPeerSyncManager
BRPeerSyncManagerNew(BRSyncManagerEventContext eventContext,
                     BRSyncManagerEventCallback eventCallback,
                     const BRChainParams *params,
                     BRWallet *wallet,
                     uint32_t earliestKeyTime,
                     uint32_t blockHeight,
                     BRMerkleBlock *blocks[],
                     size_t blocksCount,
                     const BRPeer peers[],
                     size_t peersCount) {
    BRPeerSyncManager manager = (BRPeerSyncManager) calloc (1, sizeof(struct BRPeerSyncManagerStruct));
    manager->common.mode = SYNC_MODE_P2P_ONLY;

    manager->wallet = wallet;
    manager->eventContext = eventContext;
    manager->eventCallback = eventCallback;

    pthread_mutex_init(&manager->lock, NULL);
    manager->networkBlockHeight = 0;  // TODO(fix): Should this be initialized to blockHeight?
    manager->connectionState = SYNC_MANAGER_CSTATE_DISCONNECTED;

    manager->peerManager = BRPeerManagerNew (params,
                                             wallet,
                                             earliestKeyTime,
                                             blocks, array_count(blocks),
                                             peers,  array_count(peers));

    BRPeerManagerSetCallbacks (manager->peerManager,
                               manager,
                               _BRPeerSyncManagerSyncStarted,
                               _BRPeerSyncManagerSyncStopped,
                               _BRPeerSyncManagerTxStatusUpdate,
                               _BRPeerSyncManagerSaveBlocks,
                               _BRPeerSyncManagerSavePeers,
                               _BRPeerSyncManagerNetworkIsReachabele,
                               _BRPeerSyncManagerThreadCleanup);

    return manager;
}

static BRPeerSyncManager
BRSyncManagerAsPeerSyncManager(BRSyncManager manager) {
    if (manager->mode == SYNC_MODE_P2P_ONLY) {
        return (BRPeerSyncManager) manager;
    }
    assert (0);
    return NULL;
}

static void
BRPeerSyncManagerFree(BRPeerSyncManager manager) {
    BRPeerManagerDisconnect (manager->peerManager);

    // TODO(fix): Create a centralized version of this
    pthread_mutex_destroy(&manager->lock);
    memset (manager, 0, sizeof(*manager));
    free (manager);
}

static void
BRPeerSyncManagerConnect(BRPeerSyncManager manager) {
    BRPeerManagerConnect (manager->peerManager);
    int8_t connected = BRPeerStatusDisconnected != BRPeerManagerConnectStatus (manager->peerManager);
    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (connected && manager->connectionState == SYNC_MANAGER_CSTATE_DISCONNECTED) {
            manager->connectionState = SYNC_MANAGER_CSTATE_CONNECTED;
        } else {
            connected = 0;
        }
        pthread_mutex_unlock (&manager->lock);
    }

    // send event on transition to connected state
    if (connected) {
        manager->eventCallback (manager->eventContext,
                                BRPeerSyncManagerAsSyncManager (manager),
                                (BRSyncManagerEvent) {
                                    SYNC_MANAGER_CONNECTED,
                                });
    };
}

static void
BRPeerSyncManagerDisconnect(BRPeerSyncManager manager) {
    BRPeerManagerDisconnect (manager->peerManager);
    int8_t disconnected = BRPeerStatusDisconnected == BRPeerManagerConnectStatus (manager->peerManager);
    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (disconnected && manager->connectionState == SYNC_MANAGER_CSTATE_CONNECTED) {
            manager->connectionState = SYNC_MANAGER_CSTATE_DISCONNECTED;
        } else {
            disconnected = 0;
        }
        pthread_mutex_unlock (&manager->lock);
    }

    // send event on transition to disconnected state
    if (disconnected) {
        manager->eventCallback (manager->eventContext,
                                BRPeerSyncManagerAsSyncManager (manager),
                                (BRSyncManagerEvent) {
                                    SYNC_MANAGER_DISCONNECTED,
                                });
    };
}

static void
BRPeerSyncManagerScan(BRPeerSyncManager manager) {
    BRPeerManagerRescan (manager->peerManager);
}

typedef struct {
    BRPeerSyncManager manager;
    BRTransaction *transaction;
} SubmitTransactionInfo;

static void
BRPeerSyncManagerSubmit(BRPeerSyncManager manager,
                        BRTransaction *transaction) {
    SubmitTransactionInfo *info = malloc (sizeof (SubmitTransactionInfo));
    info->manager = manager;
    info->transaction = transaction;

    BRPeerManagerPublishTx (manager->peerManager,
                            transaction,
                            info,
                            _BRPeerSyncManagerTxPublished);
}

static void
BRPeerSyncManagerTickTock(BRPeerSyncManager manager) {
    double progress = BRPeerManagerSyncProgress (manager->peerManager, 0);
    if (progress > 0.0 && progress < 1.0) {
        manager->eventCallback (manager->eventContext,
                                BRPeerSyncManagerAsSyncManager (manager),
                                (BRSyncManagerEvent) {
                                    SYNC_MANAGER_SYNC_PROGRESS,
                                    { .syncProgress = { progress * 100 }}
                                });
    }
}

/// MARK: - Peer Manager Callbacks

static void
_BRPeerSyncManagerSaveBlocks (void *info,
                              int replace,
                              BRMerkleBlock **blocks,
                              size_t count) {
    BRPeerSyncManager manager = (BRPeerSyncManager) info;
    manager->eventCallback (manager->eventContext,
                            BRPeerSyncManagerAsSyncManager (manager),
                            (BRSyncManagerEvent) {
                                replace ? SYNC_MANAGER_SET_BLOCKS : SYNC_MANAGER_ADD_BLOCKS,
                                {
                                    .blocks = { blocks, count }
                                }
                            });
}

static void
_BRPeerSyncManagerSavePeers  (void *info,
                              int replace,
                              const BRPeer *peers,
                              size_t count) {
    BRPeerSyncManager manager = (BRPeerSyncManager) info;
    manager->eventCallback (manager->eventContext,
                            BRPeerSyncManagerAsSyncManager (manager),
                            (BRSyncManagerEvent) {
                                replace ? SYNC_MANAGER_SET_PEERS : SYNC_MANAGER_ADD_PEERS,
                                {
                                    .peers = { (BRPeer *) peers, count }
                                }
                            });
}

static void
_BRPeerSyncManagerSyncStarted (void *info) {
    BRPeerSyncManager manager = (BRPeerSyncManager) info;

    // can't call BRPeerManagerConnectStatus (manager->peerManager), grabs a non-reentrant lock
    uint8_t connected = 1;
    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (manager->connectionState == SYNC_MANAGER_CSTATE_DISCONNECTED) {
            manager->connectionState = SYNC_MANAGER_CSTATE_CONNECTED;
        } else {
            connected = 0;
        }
        pthread_mutex_unlock (&manager->lock);
    }

    // send event on transition to connected state
    if (connected) {
        manager->eventCallback (manager->eventContext,
                                BRPeerSyncManagerAsSyncManager (manager),
                                (BRSyncManagerEvent) {
                                    SYNC_MANAGER_CONNECTED,
                                });
    };

    manager->eventCallback (manager->eventContext,
                            BRPeerSyncManagerAsSyncManager (manager),
                            (BRSyncManagerEvent) {
                                SYNC_MANAGER_SYNC_STARTED,
                            });
}

static void
_BRPeerSyncManagerSyncStopped (void *info, int reason) {
    BRPeerSyncManager manager = (BRPeerSyncManager) info;

    int8_t disconnected = BRPeerStatusDisconnected == BRPeerManagerConnectStatus (manager->peerManager);
    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (disconnected && manager->connectionState == SYNC_MANAGER_CSTATE_CONNECTED) {
            manager->connectionState = SYNC_MANAGER_CSTATE_DISCONNECTED;
        } else {
            disconnected = 0;
        }
        pthread_mutex_unlock (&manager->lock);
    }

    manager->eventCallback (manager->eventContext,
                            BRPeerSyncManagerAsSyncManager (manager),
                            (BRSyncManagerEvent) {
                                SYNC_MANAGER_SYNC_STOPPED,
                                { .syncStopped = { reason }},
                            });

    // send event on transition to disconnected state
    if (disconnected) {
        manager->eventCallback (manager->eventContext,
                                BRPeerSyncManagerAsSyncManager (manager),
                                (BRSyncManagerEvent) {
                                    SYNC_MANAGER_DISCONNECTED,
                                });
    };
}

static void
_BRPeerSyncManagerTxStatusUpdate (void *info) {
    BRPeerSyncManager manager = (BRPeerSyncManager) info;

    uint8_t needConnectionEvent = 0;
    BRSyncManagerEvent connectionEvent = {0};
    BRPeerStatus connectStatus = BRPeerManagerConnectStatus (manager->peerManager);

    uint8_t needBlockHeightEvent = 0;
    BRSyncManagerEvent blockHeightEvent = {0};
    uint32_t blockHeight = BRPeerManagerLastBlockHeight (manager->peerManager);

    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (connectStatus != BRPeerStatusDisconnected && manager->connectionState == SYNC_MANAGER_CSTATE_DISCONNECTED) {
            manager->connectionState = SYNC_MANAGER_CSTATE_CONNECTED;
            needConnectionEvent = 1;
            connectionEvent = (BRSyncManagerEvent) { SYNC_MANAGER_CONNECTED };

        } else if (connectStatus == BRPeerStatusDisconnected && manager->connectionState != SYNC_MANAGER_CSTATE_DISCONNECTED) {
            manager->connectionState = SYNC_MANAGER_CSTATE_DISCONNECTED;
            needConnectionEvent = 1;
            connectionEvent = (BRSyncManagerEvent) { SYNC_MANAGER_DISCONNECTED };
        }

        if (blockHeight != manager->networkBlockHeight) {
            manager->networkBlockHeight = blockHeight;
            needBlockHeightEvent = 1;
            blockHeightEvent = (BRSyncManagerEvent) { SYNC_MANAGER_BLOCK_HEIGHT_UPDATED, { .blockHeightUpdated = { blockHeight }} };
        }

        pthread_mutex_unlock (&manager->lock);
    }

    if (needBlockHeightEvent) {
        manager->eventCallback (manager->eventContext,
                                BRPeerSyncManagerAsSyncManager (manager),
                                blockHeightEvent);
    }

    if (needConnectionEvent) {
        manager->eventCallback (manager->eventContext,
                                BRPeerSyncManagerAsSyncManager (manager),
                                connectionEvent);
    }

    manager->eventCallback (manager->eventContext,
                            BRPeerSyncManagerAsSyncManager (manager),
                            (BRSyncManagerEvent) {
                                SYNC_MANAGER_TXNS_UPDATED
                            });
}

static int
_BRPeerSyncManagerNetworkIsReachabele (void *info) {
    return 1;
}

static void
_BRPeerSyncManagerThreadCleanup (void *info) {
}

static void
_BRPeerSyncManagerTxPublished (void *info,
                             int error) {
    BRPeerSyncManager manager    = ((SubmitTransactionInfo*) info)->manager;
    BRTransaction *transaction = ((SubmitTransactionInfo*) info)->transaction;
    free (info);

    manager->eventCallback (manager->eventContext,
                            BRPeerSyncManagerAsSyncManager (manager),
                            (BRSyncManagerEvent) {
                                SYNC_MANAGER_TXN_SUBMITTED,
                                { .submitted = {transaction, error} },
                            });
}
