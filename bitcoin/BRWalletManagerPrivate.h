//
//  BRWalletManagerPrivate.h
//  BRCore
//
//  Created by Michael Carrara on 3/19/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#ifndef BRWalletManagerPrivate_h
#define BRWalletManagerPrivate_h

#include "BRSyncManager.h"
#include "ethereum/event/BREvent.h"
#include "support/BRBase.h"
#include "support/BRArray.h"
#include "support/BRFileService.h"

#ifdef __cplusplus
extern "C" {
#endif

/// MARK: - BRWalletManager

typedef struct BRTransactionWithStateStruct *BRTransactionWithState;

struct BRWalletManagerStruct {

    /** The mode */
    BRCryptoSyncMode mode;

    /** The wallet */
    BRWallet *wallet;

    BRSyncManager syncManager;

    /** The client */
    BRWalletManagerClient client;

    /** The file service */
    BRFileService fileService;

    /**
     * The chain parameters associated with the wallet
     */
    const BRChainParams * chainParams;

    /**
     * The time, in seconds since UNIX epoch, at which the wallet was first created
     */
    uint32_t earliestKeyTime;

    /**
     * An EventHandler for Main.  All 'announcements' (via PeerManager (or BRD) hit here.
     */
    BREventHandler handler;

    /**
     * Number of wakeups since the last BRSyncManagerTickTock
     */
    uint32_t sleepWakeupsForSyncTickTock;

    /**
     * The Lock ensuring single thread access to BWM state.
     */
    pthread_mutex_t lock;

    /*
     * The collection of all transfers, including those that have been "deleted",
     * associated with the `wallet`.
     */
    BRArrayOf(BRTransactionWithState) transactions;
};

/// Mark: - Wallet Callbacks

extern void
bwmHandleTxAdded (BRWalletManager manager,
                  OwnershipGiven BRTransaction *ownedTransaction,
                  OwnershipKept BRTransaction *refedTransaction);

extern void
bwmSignalTxAdded (BRWalletManager manager,
                  OwnershipGiven BRTransaction *ownedTransaction,
                  OwnershipKept BRTransaction *refedTransaction);

extern void
bwmHandleTxUpdated (BRWalletManager manager,
                    UInt256 hash,
                    uint32_t blockHeight,
                    uint32_t timestamp);

extern void
bwmSignalTxUpdated (BRWalletManager manager,
                    UInt256 hash,
                    uint32_t blockHeight,
                    uint32_t timestamp);

extern void
bwmHandleTxDeleted (BRWalletManager manager,
                    UInt256 hash,
                    int recommendRescan);

extern void
bwmSignalTxDeleted (BRWalletManager manager,
                    UInt256 hash,
                    int recommendRescan);

/// Mark: - WalletManager Events

extern void
bwmHandleWalletManagerEvent(BRWalletManager bwm,
                            BRWalletManagerEvent event);

extern void
bwmSignalWalletManagerEvent (BRWalletManager manager,
                             BRWalletManagerEvent event);

extern void
bwmHandleWalletEvent(BRWalletManager bwm,
                     BRWallet *wallet,
                     BRWalletEvent event);

extern void
bwmSignalWalletEvent (BRWalletManager manager,
                      BRWallet *wallet,
                      BRWalletEvent event);

extern void
bwmHandleTransactionEvent(BRWalletManager bwm,
                          BRWallet *wallet,
                          BRTransaction *transaction,
                          BRTransactionEvent event);

extern void
bwmSignalTransactionEvent (BRWalletManager manager,
                           BRWallet *wallet,
                           BRTransaction *transaction,
                           BRTransactionEvent event);

/// MARK: - BlockNumber

extern int
bwmHandleAnnounceBlockNumber (BRWalletManager manager,
                              int rid,
                              uint64_t blockNumber);

extern void
bwmSignalAnnounceBlockNumber (BRWalletManager manager,
                              int rid,
                              uint64_t blockNumber);

/// MARK: - Transaction

extern int
bwmHandleAnnounceTransaction (BRWalletManager manager,
                              int id,
                              OwnershipKept uint8_t *transaction,
                              size_t transactionLength,
                              uint64_t timestamp,
                              uint64_t blockHeight);

extern void
bwmSignalAnnounceTransaction (BRWalletManager manager,
                              int id,
                              OwnershipKept uint8_t *transaction,
                              size_t transactionLength,
                              uint64_t timestamp,
                              uint64_t blockHeight);

extern void
bwmHandleAnnounceTransactionComplete (BRWalletManager manager,
                                      int rid,
                                      int success);

extern void
bwmSignalAnnounceTransactionComplete (BRWalletManager manager,
                                      int rid,
                                      int success);

/// MARK: - Submit

extern void
bwmHandleAnnounceSubmit (BRWalletManager manager,
                         int rid,
                         UInt256 txHash,
                         int error);

extern void
bwmSignalAnnounceSubmit (BRWalletManager manager,
                        int rid,
                        UInt256 txHash,
                        int error);

extern const BREventType *bwmEventTypes[];
extern const unsigned int bwmEventTypesCount;

#ifdef __cplusplus
}
#endif


#endif /* BRWalletManagerPrivate_h */
