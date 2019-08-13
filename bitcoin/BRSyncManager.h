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

#ifndef BRSyncManager_h
#define BRSyncManager_h

#include <stddef.h>
#include <inttypes.h>

#include "BRChainParams.h"
#include "BRMerkleBlock.h"
#include "BRPeer.h"
#include "BRWallet.h"
#include "support/BRSyncMode.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Thread model:
 *  - No explicitly managed thread or event handler contained in BRSyncManager
 *  - Defers threading to external components (i.e. parent, BRWalletManager,
 *    and children, BRPeerManager)
 */

typedef struct BRSyncManagerStruct *BRSyncManager;

typedef void *BRSyncManagerClientContext;

typedef void
(*BRSyncManagerGetBlockNumberCallback) (BRSyncManagerClientContext context,
                                        BRSyncManager manager,
                                        int rid);

typedef void
(*BRSyncManagerGetTransactionsCallback) (BRSyncManagerClientContext context,
                                         BRSyncManager manager,
                                         const char **addresses,
                                         size_t addressCount,
                                         uint64_t begBlockNumber,
                                         uint64_t endBlockNumber,
                                         int rid);

typedef void
(*BRSyncManagerSubmitTransactionCallback) (BRSyncManagerClientContext context,
                                           BRSyncManager manager,
                                           BRTransaction *transaction,
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
    SYNC_MANAGER_SYNC_STOPPED,

    SYNC_MANAGER_TXN_SUBMITTED,

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
            int reason;
        } syncStopped;
        struct {
            uint32_t value;
        } blockHeightUpdated;
        struct {
            BRTransaction *transaction;
            int error;
        } submitted;
    } u;
} BRSyncManagerEvent;

typedef void *BRSyncManagerEventContext;

typedef void
(*BRSyncManagerEventCallback) (void * context,
                               BRSyncManager manager,
                               BRSyncManagerEvent event);

extern BRSyncManager
BRSyncManagerNewForMode(BRSyncMode mode,
                        BRSyncManagerEventContext eventContext,
                        BRSyncManagerEventCallback eventCallback,
                        BRSyncManagerClientContext clientContext,
                        BRSyncManagerClientCallbacks client,
                        const BRChainParams *params,
                        BRWallet *wallet,
                        uint32_t earliestKeyTime,
                        uint32_t blockHeight,
                        BRMerkleBlock *blocks[],
                        size_t blocksCount,
                        const BRPeer peers[],
                        size_t peersCount);

extern BRSyncManager
BRSyncManagerNewForClient(BRSyncManagerEventContext eventContext,
                          BRSyncManagerEventCallback eventCallback,
                          BRSyncManagerClientContext clientContext,
                          BRSyncManagerClientCallbacks client,
                          const BRChainParams *params,
                          BRWallet *wallet,
                          uint32_t earliestKeyTime,
                          uint32_t blockHeight,
                          BRMerkleBlock *blocks[],
                          size_t blocksCount,
                          const BRPeer peers[],
                          size_t peersCount);

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
                       size_t peersCount);

extern void
BRSyncManagerFree(BRSyncManager manager);

extern void
BRSyncManagerConnect(BRSyncManager manager);

extern void
BRSyncManagerDisconnect(BRSyncManager manager);

extern void
BRSyncManagerScan(BRSyncManager manager);

extern void
BRSyncManagerSubmit(BRSyncManager manager,
                    BRTransaction *transaction);

extern void
BRSyncManagerTickTock(BRSyncManager manager);

extern void
BRSyncManagerAnnounceGetBlockNumber(BRSyncManager manager,
                                    int rid,
                                    uint32_t blockHeight);

extern void
BRSyncManagerAnnounceGetTransactionsItem(BRSyncManager manager,
                                         int rid,
                                         BRTransaction *transaction);

extern void
BRSyncManagerAnnounceGetTransactionsDone(BRSyncManager manager,
                                         int rid,
                                         int success);

extern void
BRSyncManagerAnnounceSubmitTransaction(BRSyncManager manager,
                                       int rid,
                                       BRTransaction *transaction,
                                       int error);

#ifdef __cplusplus
}
#endif

#endif // BRSyncManager_h
