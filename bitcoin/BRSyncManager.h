//  BRSyncManager.h
//
//  Created by Michael Carrara on 12/08/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#ifndef BRSyncManager_h
#define BRSyncManager_h

#include <stddef.h>
#include <inttypes.h>

#include "BRChainParams.h"
#include "BRMerkleBlock.h"
#include "BRPeer.h"
#include "BRWallet.h"
#include "support/BRBase.h"

#include "BRCryptoTransfer.h"
#include "BRCryptoWalletManager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BRSyncManagerStruct *BRSyncManager;

typedef void *BRSyncManagerClientContext;

typedef void
(*BRSyncManagerGetBlockNumberCallback) (BRSyncManagerClientContext context,
                                        BRSyncManager manager,
                                        int rid);

typedef void
(*BRSyncManagerGetTransactionsCallback) (BRSyncManagerClientContext context,
                                         BRSyncManager manager,
                                         OwnershipKept const char **addresses,
                                         size_t addressCount,
                                         uint64_t begBlockNumber,
                                         uint64_t endBlockNumber,
                                         int rid);

typedef void
(*BRSyncManagerSubmitTransactionCallback) (BRSyncManagerClientContext context,
                                           BRSyncManager manager,
                                           OwnershipKept uint8_t *transaction,
                                           size_t transactionLength,
                                           UInt256 transactionHash,
                                           int rid);

typedef struct {
    BRSyncManagerGetBlockNumberCallback  funcGetBlockNumber;
    BRSyncManagerGetTransactionsCallback funcGetTransactions;
    BRSyncManagerSubmitTransactionCallback funcSubmitTransaction;
} BRSyncManagerClientCallbacks;

typedef enum {
    SYNC_MANAGER_ADD_BLOCKS,
    SYNC_MANAGER_SET_BLOCKS,

    SYNC_MANAGER_ADD_PEERS,
    SYNC_MANAGER_SET_PEERS,

    SYNC_MANAGER_CONNECTED,
    SYNC_MANAGER_DISCONNECTED,

    SYNC_MANAGER_SYNC_STARTED,
    SYNC_MANAGER_SYNC_PROGRESS,
    SYNC_MANAGER_SYNC_STOPPED,

    SYNC_MANAGER_TXN_SUBMIT_SUCCEEDED,
    SYNC_MANAGER_TXN_SUBMIT_FAILED,

    SYNC_MANAGER_TXNS_UPDATED,
    SYNC_MANAGER_BLOCK_HEIGHT_UPDATED
} BRSyncManagerEventType;

typedef struct {
    BRSyncManagerEventType type;
    union {
        struct {
            BRMerkleBlock **blocks;
            size_t count;
        } blocks;
        struct {
            BRPeer *peers;
            size_t count;
        } peers;
        struct {
            BRCryptoSyncTimestamp timestamp;
            BRCryptoSyncPercentComplete percentComplete;
        } syncProgress;
        struct {
            BRCryptoSyncStoppedReason reason;
        } syncStopped;
        struct {
            BRCryptoWalletManagerDisconnectReason reason;
        } disconnected;
        struct {
            uint64_t value;
        } blockHeightUpdated;
        struct {
            BRTransaction *transaction;
        } submitSucceeded;
        struct {
            BRTransaction *transaction;
            BRCryptoTransferSubmitError error;
        } submitFailed;
    } u;
} BRSyncManagerEvent;

typedef void *BRSyncManagerEventContext;

typedef void
(*BRSyncManagerEventCallback) (void * context,
                               BRSyncManager manager,
                               OwnershipKept BRSyncManagerEvent event);

/**
 * Create a new BRSyncManager for the desired mode.
 *
 * A note on the threading model:
 *  - No explicitly managed thread or event handler contained in BRSyncManager
 *  - Defers threading to external components (i.e. owning BRWalletManager
 *    and/or owned BRPeerManager)
 *  - Locking does occur both internally in the BRSyncManager as well as
 *    its subordinate objects (ex: BRPeerManager).
 *  - Callbacks (both `eventCallback` and `clientCallbacks`) MAY be
 *    called with non-reentrant locks held. As such, calls back
 *    into the BRSyncManager MUST NOT occur in a callback.
 */
extern BRSyncManager
BRSyncManagerNewForMode(BRCryptoSyncMode mode,
                        BRSyncManagerEventContext eventContext,
                        BRSyncManagerEventCallback eventCallback,
                        BRSyncManagerClientContext clientContext,
                        BRSyncManagerClientCallbacks clientCallbacks,
                        OwnershipKept const BRChainParams *params,
                        OwnershipKept BRWallet *wallet,
                        uint32_t earliestKeyTime,
                        uint64_t blockHeight,
                        uint64_t confirmationUntilFinal,
                        int isNetworkReachable,
                        OwnershipKept BRMerkleBlock *blocks[],
                        size_t blocksCount,
                        OwnershipKept const BRPeer peers[],
                        size_t peersCount);

extern void
BRSyncManagerFree(BRSyncManager manager);

extern uint64_t
BRSyncManagerGetBlockHeight (BRSyncManager manager);

extern uint64_t
BRSyncManagerGetConfirmationsUntilFinal (BRSyncManager manager);

extern int
BRSyncManagerGetNetworkReachable (BRSyncManager manager);

extern void
BRSyncManagerSetNetworkReachable (BRSyncManager manager,
                                  int isNetworkReachable);

extern void
BRSyncManagerSetFixedPeer (BRSyncManager manager,
                           UInt128 address,
                           uint16_t port);

extern void
BRSyncManagerConnect(BRSyncManager manager);

extern void
BRSyncManagerDisconnect(BRSyncManager manager);

extern void
BRSyncManagerScanToDepth(BRSyncManager manager,
                         BRCryptoSyncDepth depth,
                         OwnershipKept BRTransaction *lastConfirmedSend);

extern void
BRSyncManagerSubmit(BRSyncManager manager,
                    OwnershipKept BRTransaction *transaction);

extern void
BRSyncManagerTickTock(BRSyncManager manager);

extern void
BRSyncManagerP2PFullScanReport(BRSyncManager manager);

extern void
BRSyncManagerAnnounceGetBlockNumber(BRSyncManager manager,
                                    int rid,
                                    uint64_t blockHeight);

extern void
BRSyncManagerAnnounceGetTransactionsItem(BRSyncManager manager,
                                         int rid,
                                         OwnershipKept uint8_t *transaction,
                                         size_t transactionLength,
                                         uint64_t timestamp,
                                         uint64_t blockHeight);

extern void
BRSyncManagerAnnounceGetTransactionsDone(BRSyncManager manager,
                                         int rid,
                                         int success);

extern void
BRSyncManagerAnnounceSubmitTransaction(BRSyncManager manager,
                                       int rid,
                                       OwnershipKept BRTransaction *transaction,
                                       int error);

#ifdef __cplusplus
}
#endif

#endif // BRSyncManager_h
