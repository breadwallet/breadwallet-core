//
//  BRWalletManagerPrivate.h
//  BRCore
//
//  Created by Michael Carrara on 3/19/19.
//  Copyright (c) 2019 breadwallet LLC
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
    BRSyncMode mode;

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

    /**
     * The Lock ensuring single thread access to the collection of transfers..
     */
    pthread_mutex_t transactionLock;

    /*
     * The collection of all transfers, including those that have been "deleted",
     * associated with the `wallet`.
     */
    BRArrayOf(BRTransactionWithState) transactions;
};

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
