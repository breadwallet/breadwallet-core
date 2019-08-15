//  BRSyncManager.c
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

struct BRSyncManagerStruct {
    BRSyncMode mode;
};

/// MARK: - Sync Manager Decls & Defs

struct BRClientSyncManagerStruct {
    // !!! must be first !!!
    struct BRSyncManagerStruct common;

    /// Mark: - Immutable Section

    /**
     *  Mutable state lock
     */
    pthread_mutex_t lock;

    /**
     * Wallet being synced
     */
    BRWallet *wallet;

    /**
     * Event callback info
     */
    BRSyncManagerEventContext eventContext;
    BRSyncManagerEventCallback eventCallback;

    /**
     * Client callback info
     */
    BRSyncManagerClientContext clientContext;
    BRSyncManagerClientCallbacks clientCallbacks;

    /**
     * The height of the earliest block of interest. Initialized based on the
     * earliest key time of the account being synced.
     */
    uint64_t initBlockHeight;

    /// Mark: - Mutable Sction

    /**
     * Contains the height that we have synced to. Initially, this will be the same
     * as `initBlockHeight`. As we download transactions, this moves forward. It
     * can be reset when a `Scan` has been initiated, in which case it reverts to
     * `initBlockHeight`.
     */
    uint64_t syncedBlockHeight;

    /**
     * The known height of the blockchain, as reported by the "network".
     */
    uint64_t networkBlockHeight;

    /**
     * Flag for whether or not we are connected to the "network".
     */
    uint8_t isConnected;

    /**
     * An identiifer generator for a BRD Request
     */
    int requestIdGenerator;

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
        uint8_t isFullScan;
    } scanState;
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
                       OwnershipKept const BRChainParams *params,
                       OwnershipKept BRWallet *wallet,
                       uint32_t earliestKeyTime,
                       uint64_t blockHeight,
                       OwnershipKept BRMerkleBlock *blocks[],
                       size_t blocksCount,
                       OwnershipKept const BRPeer peers[],
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
                          OwnershipGiven BRTransaction *transaction);

static void
BRClientSyncManagerTickTock(BRClientSyncManager manager);

static void
BRClientSyncManagerAnnounceGetBlockNumber(BRClientSyncManager manager,
                                          int rid,
                                          uint64_t blockHeight);

static void
BRClientSyncManagerAnnounceGetTransactionsItem (BRClientSyncManager manager,
                                                int rid,
                                                OwnershipGiven BRTransaction *transaction);

static void
BRClientSyncManagerAnnounceGetTransactionsDone (BRClientSyncManager manager,
                                                int rid,
                                                int success);

static void
BRClientSyncManagerAnnounceSubmitTransaction (BRClientSyncManager manager,
                                              int rid,
                                              OwnershipGiven BRTransaction *transaction,
                                              int error);

static void
BRClientSyncManagerUpdateBlockNumber (BRClientSyncManager manager);

static void
BRClientSyncManagerStartScanIfNeeded (BRClientSyncManager manager);

static int
BRClientSyncManagerGenerateRid (BRClientSyncManager manager);

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

    /// Mark: - Immutable Section

    /**
     *  Mutable state lock
     */
    pthread_mutex_t lock;

    /**
     * P2P syncing manager
     */
    BRPeerManager *peerManager;

    /**
     * Event callback info
     */
    BRSyncManagerEventContext eventContext;
    BRSyncManagerEventCallback eventCallback;

    /// Mark: - Mutable Sction

    /**
     * The known height of the blockchain, as reported by the P2P network.
     */
    uint64_t networkBlockHeight;

    /**
     * Flag for whether or not we are connected to the P2P network.
     */
    uint8_t isConnected;

    /**
     * Flag for whether or not a full sync is in progress, versus when we have
     * caught up to the blockchain and are receiving new blocks.
     */
    uint8_t isFullScan;

};

typedef struct BRPeerSyncManagerStruct * BRPeerSyncManager;

#define BRPeerSyncManagerAsSyncManager(x)     ((BRSyncManager) (x))

static BRPeerSyncManager
BRPeerSyncManagerNew(BRSyncManagerEventContext eventContext,
                     BRSyncManagerEventCallback eventCallback,
                     OwnershipKept const BRChainParams *params,
                     OwnershipKept BRWallet *wallet,
                     uint32_t earliestKeyTime,
                     uint64_t blockHeight,
                     OwnershipKept BRMerkleBlock *blocks[],
                     size_t blocksCount,
                     OwnershipKept const BRPeer peers[],
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
BRPeerSyncManagerSubmit(BRPeerSyncManager manager,
                        OwnershipGiven BRTransaction *transaction);

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
                        OwnershipKept const BRChainParams *params,
                        OwnershipKept BRWallet *wallet,
                        uint32_t earliestKeyTime,
                        uint64_t blockHeight,
                        OwnershipKept BRMerkleBlock *blocks[],
                        size_t blocksCount,
                        OwnershipKept const BRPeer peers[],
                        size_t peersCount) {
    assert (NULL != eventCallback);
    assert (NULL != params);
    assert (NULL != wallet);
    assert(blocks != NULL || blocksCount == 0);
    assert(peers != NULL || peersCount == 0);

    switch (mode) {
        case SYNC_MODE_BRD_ONLY:
        return BRClientSyncManagerAsSyncManager (BRClientSyncManagerNew (eventContext,
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
                                                                         peersCount));
        case SYNC_MODE_P2P_ONLY:
        return BRPeerSyncManagerAsSyncManager (BRPeerSyncManagerNew (eventContext,
                                                                     eventCallback,
                                                                     params,
                                                                     wallet,
                                                                     earliestKeyTime,
                                                                     blockHeight,
                                                                     blocks,
                                                                     blocksCount,
                                                                     peers,
                                                                     peersCount));
        default:
        assert (0);
        return NULL;
    }
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
                    OwnershipGiven BRTransaction *transaction) {
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
                                    uint64_t blockHeight) {
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
                                         OwnershipGiven BRTransaction *transaction) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
        BRClientSyncManagerAnnounceGetTransactionsItem (BRSyncManagerAsClientSyncManager (manager),
                                                        rid,
                                                        transaction);
        break;
        case SYNC_MODE_P2P_ONLY:
        // This case might arise if we swap a BRWalletManager's syncManager
        BRTransactionFree (transaction);
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
                                       OwnershipGiven BRTransaction *transaction,
                                       int error) {
    switch (manager->mode) {
        case SYNC_MODE_BRD_ONLY:
        BRClientSyncManagerAnnounceSubmitTransaction (BRSyncManagerAsClientSyncManager (manager),
                                                      rid,
                                                      transaction,
                                                      error);
        break;
        case SYNC_MODE_P2P_ONLY:
        // This case might arise if we swap a BRWalletManager's syncManager
        BRTransactionFree (transaction);
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
                       OwnershipKept const BRChainParams *params,
                       OwnershipKept BRWallet *wallet,
                       uint32_t earliestKeyTime,
                       uint64_t blockHeight,
                       OwnershipKept BRMerkleBlock *blocks[],
                       size_t blocksCount,
                       OwnershipKept const BRPeer peers[],
                       size_t peersCount) {
    BRClientSyncManager manager = (BRClientSyncManager) calloc (1, sizeof(struct BRClientSyncManagerStruct));
    manager->common.mode = SYNC_MODE_BRD_ONLY;

    manager->wallet = wallet;
    manager->eventContext = eventContext;
    manager->eventCallback = eventCallback;
    manager->clientContext = clientContext;
    manager->clientCallbacks = clientCallbacks;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&manager->lock, &attr);
    pthread_mutexattr_destroy(&attr);

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
    manager->isConnected        = 0;

    // The `scanState` struct is zeroed out by the calloc call

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
    pthread_mutex_destroy(&manager->lock);
    memset (manager, 0, sizeof(*manager));
    free (manager);
}

static void
BRClientSyncManagerConnect(BRClientSyncManager manager) {
    uint8_t needEvent = 0;

    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (!manager->isConnected) {
            manager->isConnected = 1;
            needEvent = 1;
        }

        // Send event while holding the state lock so that event
        // callbacks are ordered to reflect state transitions.

        if (needEvent) {
            manager->eventCallback (manager->eventContext,
                                    BRClientSyncManagerAsSyncManager (manager),
                                    (BRSyncManagerEvent) {
                                        SYNC_MANAGER_CONNECTED
                                    });
        }

        pthread_mutex_unlock (&manager->lock);
    } else {
        assert (0);
    }

    BRClientSyncManagerStartScanIfNeeded (manager);
}

static void
BRClientSyncManagerDisconnect(BRClientSyncManager manager) {
    uint8_t needConnectionEvent = 0;
    uint8_t needSyncEvent       = 0;

    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (manager->isConnected) {
            // We are connected. Check for a full scan in progress
            // and then wipe the current scan state so that a new one will be
            // triggered.
            manager->isConnected = 0;
            needConnectionEvent = 1;
            needSyncEvent = manager->scanState.isFullScan;
            memset(&manager->scanState, 0, sizeof(manager->scanState));
        }

        // Send event while holding the state lock so that event
        // callbacks are ordered to reflect state transitions.

        if (needSyncEvent) {
            // TODO(fix): What should the error code be?
            manager->eventCallback (manager->eventContext,
                                    BRClientSyncManagerAsSyncManager (manager),
                                    (BRSyncManagerEvent) {
                                        SYNC_MANAGER_SYNC_STOPPED,
                                        { .syncStopped = { -1 }}
                                    });
        }

        if (needConnectionEvent) {
            manager->eventCallback (manager->eventContext,
                                    BRClientSyncManagerAsSyncManager (manager),
                                    (BRSyncManagerEvent) {
                                        SYNC_MANAGER_DISCONNECTED
                                    });
        }

        pthread_mutex_unlock (&manager->lock);
    } else {
        assert (0);
    }
}

static void
BRClientSyncManagerScan(BRClientSyncManager manager) {
    uint8_t needConnectionEvent = 0;
    uint8_t needSyncEvent       = 0;

    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (!manager->isConnected) {
            manager->isConnected = 1;
            needConnectionEvent = 1;
        } else {
            // We are already connected. Checkf for a full scan in progress
            // and then wipe the current scan state so that a new one will be
            // triggered.
            needSyncEvent = manager->scanState.isFullScan;
            memset(&manager->scanState, 0, sizeof(manager->scanState));
        }

        // Reset the height that we've synced to to be the initial height.
        // This will trigger a full sync.
        manager->syncedBlockHeight = manager->initBlockHeight;

        // Send event while holding the state lock so that event
        // callbacks are ordered to reflect state transitions.

        if (needSyncEvent) {
            // TODO(fix): What should the error code be?
            manager->eventCallback (manager->eventContext,
                                    BRClientSyncManagerAsSyncManager (manager),
                                    (BRSyncManagerEvent) {
                                        SYNC_MANAGER_SYNC_STOPPED,
                                        { .syncStopped = { -1 }}
                                    });
        }

        if (needConnectionEvent) {
            manager->eventCallback (manager->eventContext,
                                    BRClientSyncManagerAsSyncManager (manager),
                                    (BRSyncManagerEvent) {
                                        SYNC_MANAGER_CONNECTED
                                    });
        }

        pthread_mutex_unlock (&manager->lock);
    } else {
        assert (0);
    }

    BRClientSyncManagerStartScanIfNeeded (manager);
}

static void
BRClientSyncManagerSubmit(BRClientSyncManager manager,
                          OwnershipGiven BRTransaction *transaction) {
    uint8_t needClientCall = 0;
    int rid                = -1;

    if (0 == pthread_mutex_lock (&manager->lock)) {
        needClientCall = manager->isConnected;
        rid = needClientCall ? BRClientSyncManagerGenerateRid (manager) : rid;

        pthread_mutex_unlock (&manager->lock);
    } else {
        assert (0);
    }

    // Send event WITHOUT holding the state lock as this type of event
    // has no impact on the sync manager's state.

    if (needClientCall) {
        manager->clientCallbacks.funcSubmitTransaction (manager->clientContext,
                                                        BRClientSyncManagerAsSyncManager (manager),
                                                        transaction,
                                                        rid);
    } else {
        // TODO(fix): What should the error code be?
        manager->eventCallback (manager->eventContext,
                                BRClientSyncManagerAsSyncManager (manager),
                                (BRSyncManagerEvent) {
                                    SYNC_MANAGER_TXN_SUBMITTED,
                                    { .submitted = {transaction, -1} },
                                });

        BRTransactionFree (transaction);
    }
}

static void
BRClientSyncManagerTickTock(BRClientSyncManager manager) {
    BRClientSyncManagerStartScanIfNeeded (manager);
    BRClientSyncManagerUpdateBlockNumber (manager);
}

static void
BRClientSyncManagerAnnounceGetBlockNumber(BRClientSyncManager manager,
                                          int rid,
                                          uint64_t blockHeight) {
    uint8_t needEvent = 0;

    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (blockHeight > manager->networkBlockHeight && manager->isConnected) {
            manager->networkBlockHeight = blockHeight;
            needEvent = 1;
        }

        // Send event while holding the state lock so that we
        // don't broadcast a height updated while disconnected,
        // for example.

        if (needEvent) {
            manager->eventCallback (manager->eventContext,
                                    BRClientSyncManagerAsSyncManager (manager),
                                    (BRSyncManagerEvent) {
                                        SYNC_MANAGER_BLOCK_HEIGHT_UPDATED,
                                        { .blockHeightUpdated = { blockHeight }}
                                    });
        }

        pthread_mutex_unlock (&manager->lock);
    } else {
        assert (0);
    }
}

static void
BRClientSyncManagerAnnounceSubmitTransaction (BRClientSyncManager manager,
                                              int rid,
                                              OwnershipGiven BRTransaction *transaction,
                                              int error) {
    manager->eventCallback (manager->eventContext,
                            BRClientSyncManagerAsSyncManager (manager),
                            (BRSyncManagerEvent) {
                                SYNC_MANAGER_TXN_SUBMITTED,
                                { .submitted = {transaction, error} },
                            });

    BRTransactionFree (transaction);
}

static void
BRClientSyncManagerAnnounceGetTransactionsItem (BRClientSyncManager manager,
                                                int rid,
                                                OwnershipGiven BRTransaction *transaction) {
    uint8_t needRegistration = BRTransactionIsSigned (transaction);
    if (needRegistration) {
        if (0 == pthread_mutex_lock (&manager->lock)) {
            // confirm completion is for in-progress sync
            needRegistration &= (rid == manager->scanState.requestId && manager->isConnected);
            pthread_mutex_unlock (&manager->lock);
        } else {
            assert (0);
        }
    }

    if (needRegistration) {
        BRWalletRegisterTransaction (manager->wallet, transaction);
        if (BRWalletTransactionForHash (manager->wallet, transaction->txHash) != transaction) {
            BRTransactionFree (transaction);
        }
    } else {
        BRTransactionFree (transaction);
    }
}

static void
BRClientSyncManagerAnnounceGetTransactionsDone (BRClientSyncManager manager,
                                                int rid,
                                                int success) {
    size_t addressCount          = 0;
    uint8_t needSyncEvent        = 0;
    uint8_t needClientCall       = 0;
    uint64_t begBlockNumber      = 0;
    uint64_t endBlockNumber      = 0;
    BRAddress *addressArray      = NULL;
    const char **addressStrings  = NULL;
    BRSyncManagerEvent syncEvent = {0};

    if (0 == pthread_mutex_lock (&manager->lock)) {
        // confirm completion is for in-progress sync
        if (rid == manager->scanState.requestId &&
            manager->isConnected) {
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
                if (!BRAddressEq (&externalAddress, &manager->scanState.lastExternalAddress) ||
                    !BRAddressEq (&internalAddress, &manager->scanState.lastInternalAddress)) {
                    // ... we've discovered a new address (i.e. there were transactions announce)
                    // so we need to requery the same range including the newly derived addresses

                    // store the first unused addresses for comparison in the next complete call
                    manager->scanState.lastExternalAddress = externalAddress;
                    manager->scanState.lastInternalAddress = internalAddress;

                    // get the addresses to query the BDB with
                    BRClientSyncManagerGetAllAddrsAsStrings (manager,
                                                             &addressCount,
                                                             &addressStrings,
                                                             &addressArray);

                    // don't need to alter the range (we haven't found all transactions yet)

                    // store sync data for callback outside of lock
                    begBlockNumber = manager->scanState.chunkBegBlockNumber;
                    endBlockNumber = manager->scanState.chunkEndBlockNumber;

                    // store control flow flags
                    needClientCall = 1;

                } else if (manager->scanState.chunkEndBlockNumber != manager->scanState.endBlockNumber) {
                    // .. we haven't discovered any new addresses but we haven't gone through the whole range yet

                    // don't need to store the first unused addresses (we just confirmed they are equal)

                    // get the addresses to query the BDB with
                    BRClientSyncManagerGetAllAddrsAsStrings (manager,
                                                             &addressCount,
                                                             &addressStrings,
                                                             &addressArray);

                    // store the new range
                    manager->scanState.chunkBegBlockNumber = manager->scanState.chunkEndBlockNumber;
                    manager->scanState.chunkEndBlockNumber = MIN (manager->scanState.chunkEndBlockNumber + manager->scanState.chunkSize,
                                                                  manager->scanState.endBlockNumber);

                    // store sync data for callback outside of lock
                    begBlockNumber = manager->scanState.chunkBegBlockNumber;
                    endBlockNumber = manager->scanState.chunkEndBlockNumber;

                    // store control flow flags
                    needClientCall = 1;
                    needSyncEvent = 1;
                    syncEvent = (BRSyncManagerEvent) {
                        SYNC_MANAGER_SYNC_PROGRESS,
                        { .syncProgress = { (uint32_t) (((manager->scanState.chunkBegBlockNumber - manager->scanState.begBlockNumber) * 100) /
                                                        (manager->scanState.endBlockNumber - manager->scanState.begBlockNumber)) }}
                    };

                } else {
                    // .. we haven't discovered any new addresses and we just finished the last chunk

                    // store synced block height
                    manager->syncedBlockHeight = manager->scanState.endBlockNumber - 1;

                    // store control flow flags
                    needSyncEvent = manager->scanState.isFullScan;
                    syncEvent = (BRSyncManagerEvent) {SYNC_MANAGER_SYNC_STOPPED, { .syncStopped = { 0 }}};

                    // reset sync state
                    memset(&manager->scanState, 0, sizeof(manager->scanState));

                }
            } else {
                // store control flow flags
                needSyncEvent = manager->scanState.isFullScan;

                // TODO(fix): What should the error code be?
                syncEvent = (BRSyncManagerEvent) {SYNC_MANAGER_SYNC_STOPPED, { .syncStopped = { -1 }}};

                // reset sync state on failure
                memset(&manager->scanState, 0, sizeof(manager->scanState));
            }
        }

        // Send event while holding the state lock so that event
        // callbacks are ordered to reflect state transitions.

        if (needSyncEvent) {
            manager->eventCallback (manager->eventContext,
                                    BRClientSyncManagerAsSyncManager (manager),
                                    syncEvent);
        }

        pthread_mutex_unlock (&manager->lock);
    } else {
        assert (0);
    }

    if (needClientCall) {
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
    } else if (addressStrings) {
        free (addressStrings);
    }

    if (addressArray) {
        free (addressArray);
    }
}

static void
BRClientSyncManagerStartScanIfNeeded (BRClientSyncManager manager) {
    int rid                     = 0;
    uint8_t needSyncEvent       = 0;
    uint8_t needClientCall      = 0;
    size_t addressCount         = 0;
    uint64_t begBlockNumber     = 0;
    uint64_t endBlockNumber     = 0;
    BRAddress *addressArray     = NULL;
    const char **addressStrings = NULL;

    if (0 == pthread_mutex_lock (&manager->lock)) {
        // check if we are connect and the prior sync has completed.
        if (0 == manager->scanState.requestId &&
            manager->isConnected) {
            // update the `endBlockNumber` to the current block height;
            // since this is exclusive on the end height, we need to increment by
            // one to make sure we get the last block
            manager->scanState.endBlockNumber = MAX (manager->syncedBlockHeight, manager->networkBlockHeight) + 1;

            // update the `startBlockNumber` to the last synced height;
            // provide a bit of buffer and request the last X blocks, regardless
            manager->scanState.begBlockNumber = MIN (manager->syncedBlockHeight, (manager->scanState.endBlockNumber >=  BWM_BRD_SYNC_START_BLOCK_OFFSET
                                                                                  ? manager->scanState.endBlockNumber - BWM_BRD_SYNC_START_BLOCK_OFFSET
                                                                                  : 0));

            // update the chunk range
            manager->scanState.chunkSize = BWM_BRD_SYNC_CHUNK_SIZE;
            manager->scanState.chunkBegBlockNumber = manager->scanState.begBlockNumber;
            manager->scanState.chunkEndBlockNumber = MIN (manager->scanState.begBlockNumber + manager->scanState.chunkSize,
                                                          manager->scanState.endBlockNumber);

            // generate addresses
            BRClientSyncManagerGenerateUnusedAddrs (manager);

            // save the last known external and internal addresses
            BRWalletUnusedAddrs(manager->wallet, &manager->scanState.lastExternalAddress, 1, 0);
            BRWalletUnusedAddrs(manager->wallet, &manager->scanState.lastInternalAddress, 1, 1);

            // get the addresses to query the BDB with
            BRClientSyncManagerGetAllAddrsAsStrings (manager,
                                                     &addressCount,
                                                     &addressStrings,
                                                     &addressArray);

            // save the current requestId
            manager->scanState.requestId = BRClientSyncManagerGenerateRid (manager);

            // mark as sync or not
            manager->scanState.isFullScan = (manager->scanState.endBlockNumber - manager->scanState.begBlockNumber) > BWM_BRD_SYNC_START_BLOCK_OFFSET;

            // store sync data for callback outside of lock
            begBlockNumber = manager->scanState.chunkBegBlockNumber;
            endBlockNumber = manager->scanState.chunkEndBlockNumber;
            rid = manager->scanState.requestId;

            // store control flow flags
            needSyncEvent = manager->scanState.isFullScan;
            needClientCall = 1;
        }

        // Send event while holding the state lock so that event
        // callbacks are ordered to reflect state transitions.

        if (needSyncEvent) {
            manager->eventCallback (manager->eventContext,
                                    BRClientSyncManagerAsSyncManager (manager),
                                    (BRSyncManagerEvent) {
                                        SYNC_MANAGER_SYNC_STARTED,
                                    });
        }

        pthread_mutex_unlock (&manager->lock);
    } else {
        assert (0);
    }

    if (needClientCall) {
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
    } else if (addressStrings) {
        free (addressStrings);
    }

    if (addressArray) {
        free (addressArray);
    }
}

static void
BRClientSyncManagerUpdateBlockNumber(BRClientSyncManager manager) {
    uint8_t needClientCall = 0;
    int rid                = -1;

    if (0 == pthread_mutex_lock (&manager->lock)) {
        needClientCall = manager->isConnected;
        rid = needClientCall ? BRClientSyncManagerGenerateRid (manager) : rid;

        pthread_mutex_unlock (&manager->lock);
    } else {
        assert (0);
    }

    if (needClientCall) {
        manager->clientCallbacks.funcGetBlockNumber (manager->clientContext,
                                                     BRClientSyncManagerAsSyncManager (manager),
                                                     rid);
    }
}

static int
BRClientSyncManagerGenerateRid (BRClientSyncManager manager) {
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
    size_t addrCount     = 0;
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
                     OwnershipKept const BRChainParams *params,
                     OwnershipKept BRWallet *wallet,
                     uint32_t earliestKeyTime,
                     uint64_t blockHeight,
                     OwnershipKept BRMerkleBlock *blocks[],
                     size_t blocksCount,
                     OwnershipKept const BRPeer peers[],
                     size_t peersCount) {
    BRPeerSyncManager manager = (BRPeerSyncManager) calloc (1, sizeof(struct BRPeerSyncManagerStruct));
    manager->common.mode = SYNC_MODE_P2P_ONLY;

    manager->eventContext = eventContext;
    manager->eventCallback = eventCallback;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&manager->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    manager->networkBlockHeight = 0;  // TODO(discuss): Should this be initialized to blockHeight?
    manager->isConnected = 0;

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

    pthread_mutex_destroy(&manager->lock);
    memset (manager, 0, sizeof(*manager));
    free (manager);
}

static void
BRPeerSyncManagerConnect(BRPeerSyncManager manager) {
    BRPeerManagerConnect (manager->peerManager);
}

static void
BRPeerSyncManagerDisconnect(BRPeerSyncManager manager) {
    BRPeerManagerDisconnect (manager->peerManager);

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
                        OwnershipGiven BRTransaction *transaction) {
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
    uint8_t needSyncEvent = progress > 0.0 && progress < 1.0;
    if (needSyncEvent) {
        if (0 == pthread_mutex_lock (&manager->lock)) {
            needSyncEvent &= manager->isConnected && manager->isFullScan;
            pthread_mutex_unlock (&manager->lock);
        } else {
            assert (0);
        }
    }

    if (needSyncEvent) {
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
                              OwnershipKept BRMerkleBlock **blocks,
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
                              OwnershipKept const BRPeer *peers,
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

    uint8_t isFullScan = 0;

    // can't call BRPeerManagerConnectStatus (manager->peerManager), grabs a non-reentrant lock
    uint8_t needConnectionEvent = 1;
    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (!manager->isConnected) {
            manager->isConnected = 1;
        } else {
            needConnectionEvent = 0;
        }

        isFullScan = manager->isFullScan;
        manager->isFullScan = 1;
        pthread_mutex_unlock (&manager->lock);
    } else {
        assert (0);
    }

    // a full sync was already in progress, so announce its conclusion before
    // announcing a new sync starting
    if (isFullScan) {
        // TODO(fix): What should the error code be?
        manager->eventCallback (manager->eventContext,
                                BRPeerSyncManagerAsSyncManager (manager),
                                (BRSyncManagerEvent) {
                                    SYNC_MANAGER_SYNC_STOPPED,
                                    { .syncStopped = { -1 }},
                                });
    }

    // if we weren't aware that we were connected, we are now, so announce it!
    if (needConnectionEvent) {
        manager->eventCallback (manager->eventContext,
                                BRPeerSyncManagerAsSyncManager (manager),
                                (BRSyncManagerEvent) {
                                    SYNC_MANAGER_CONNECTED,
                                });
    };

    // announce that a new sync has begun
    manager->eventCallback (manager->eventContext,
                            BRPeerSyncManagerAsSyncManager (manager),
                            (BRSyncManagerEvent) {
                                SYNC_MANAGER_SYNC_STARTED,
                            });
}

static void
_BRPeerSyncManagerSyncStopped (void *info, int reason) {
    BRPeerSyncManager manager = (BRPeerSyncManager) info;

    uint8_t isFullScan = 0;

    int8_t disconnected = BRPeerStatusDisconnected == BRPeerManagerConnectStatus (manager->peerManager);
    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (disconnected && manager->isConnected) {
            manager->isConnected = 0;
        } else {
            disconnected = 0;
        }

        isFullScan = manager->isFullScan;
        manager->isFullScan = 0;
        pthread_mutex_unlock (&manager->lock);
    } else {
        assert (0);
    }

    // if a full sync was in progress, announce its conclusion
    if (isFullScan) {
        manager->eventCallback (manager->eventContext,
                                BRPeerSyncManagerAsSyncManager (manager),
                                (BRSyncManagerEvent) {
                                    SYNC_MANAGER_SYNC_STOPPED,
                                    { .syncStopped = { reason }},
                                });
    }

    // if we weren't aware that we were disconnected, we are now, so announce it!
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

    uint8_t needBlockHeightEvent = 0;
    uint64_t blockHeight = BRPeerManagerLastBlockHeight (manager->peerManager);
    BRSyncManagerEvent blockHeightEvent = (BRSyncManagerEvent) { SYNC_MANAGER_BLOCK_HEIGHT_UPDATED, { .blockHeightUpdated = { blockHeight }} };

    if (0 == pthread_mutex_lock (&manager->lock)) {
        if (blockHeight != manager->networkBlockHeight) {
            manager->networkBlockHeight = blockHeight;
            needBlockHeightEvent = 1;
        }

        pthread_mutex_unlock (&manager->lock);
    } else {
        assert (0);
    }

    if (needBlockHeightEvent) {
        manager->eventCallback (manager->eventContext,
                                BRPeerSyncManagerAsSyncManager (manager),
                                blockHeightEvent);
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
