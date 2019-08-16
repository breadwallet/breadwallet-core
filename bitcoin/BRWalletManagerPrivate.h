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
     * The forkId of the chain parameters associated with the wallet
     */
    uint8_t forkId;

    /**
     * An EventHandler for Main.  All 'announcements' (via PeerManager (or BRD) hit here.
     */
    BREventHandler handler;

    /**
     * The Lock ensuring single thread access to BWM state.
     */
    pthread_mutex_t lock;

    /*
     * Transfers - these are sorted from oldest [index 0] to newest.  As transfers are added
     * we'll maintain the ordering using an 'insertion sort' - while starting at the end and
     * working backwards.
     *
     * We are often faced with looking up a transfer based on a hash.  For example, BCS found
     * a transaction for our address and we need to find the corresponding transfer.  Or, instead
     * of BCS, the BRD endpoint reports a transaction/log of interest.  How do we lookup based
     * on a hash?
     *
     * Further complicating the lookup are:
     *  a) a transfer is only guaranteed to have a hash if we originated the transfer.  In this
     *     case we have an 'originating transaction' and can compare its hash.
     *  b) a log doesn't have a transaction hash until it has been included.
     *  c) one hash can produce multiple logs.  The logs will have a unique identifier, as
     *     {hash,indexInBlock} when included, but the hash itself need not be unique.  Note: this
     *     does not apply to ERC20 transfers, which have one hash and one log.
     *
     * FOR NOW, WE'LL ASSUME: ONE HASH <==> ONE TRANSFER (transaction or log)
     *
     * Given a hash, to find a corresponding transfers we'll iterate through `transfers` and
     * compare: a) the hash for the originating transaction, if it exists, b) the hash for the
     * basis, if it exists.  If the basis is a log, we'll extranct the transaction hash and compare
     * that.
     *
     * We might consider:
     *   BRSetOf (BRArrayOf ({hash, transfer}) transfersByHashPairs;
     * which would speed lookup of a transfer.
     *
     */
    BRTransactionWithState* transactions;
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
