//
//  BRGenericWalletManager.h
//  BRCore
//
//  Created by Ed Gamble on 6/20/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRGenericWalletManager_h
#define BRGenericWalletManager_h

#include "BRGenericBase.h"
#include "BRGenericHandlers.h"

#ifdef __cplusplus
extern "C" {
#endif

    // MARK: - Generic Wallet Manager
    typedef struct BRGenericWalletManagerRecord *BRGenericWalletManager;

    typedef void *BRGenericClientContext;

    typedef void
    (*BRGenericGetBlockNumberCallback) (BRGenericClientContext context,
                                        BRGenericWalletManager manager,
                                        int rid);

    extern int
    gwmAnnounceBlockNumber (BRGenericWalletManager manager,
                            int rid,
                            uint64_t blockNumber);


    typedef void
    (*BRGenericGetTransactionsCallback) (BRGenericClientContext context,
                                         BRGenericWalletManager manager,
                                         const char *address,
                                         uint64_t begBlockNumber,
                                         uint64_t endBlockNumber,
                                         int rid);


    extern int // success - data is valid
    gwmAnnounceTransfer (BRGenericWalletManager manager,
                         int rid,
                         BRGenericTransfer transfer);

    extern void
    gwmAnnounceTransferComplete (BRGenericWalletManager manager,
                                 int rid,
                                 int success);

    typedef void
    (*BRGenericSubmitTransactionCallback) (BRGenericClientContext context,
                                           BRGenericWalletManager manager,
                                           BRGenericWallet wallet,
                                           BRGenericTransfer transfer,
                                           int rid);

    extern void
    gwmAnnounceSubmit (BRGenericWalletManager manager,
                       int rid,
                       BRGenericTransfer transfer,
                       int error);

    typedef struct {
        BRGenericClientContext context;
        BRGenericGetBlockNumberCallback getBlockNumber;
        BRGenericGetTransactionsCallback getTransactions;
        BRGenericSubmitTransactionCallback submitTransaction;
    } BRGenericClient;

    // Add: NAME (as Currency Code) for pthread name
    // Add: period for query rate.
    extern BRGenericWalletManager
    gwmCreate (BRGenericClient client,
               const char *type,
               BRGenericAccount account,
               uint64_t accountTimestamp,
               const char *storagePath,
               uint32_t syncPeriodInSeconds,
               uint64_t blockHeight);

    extern void
    gwmRelease (BRGenericWalletManager gwm);

    extern void
    gwmStop (BRGenericWalletManager gwm);

    extern void
    gwmConnect (BRGenericWalletManager gwm);

    extern void
    gwmDisconnect (BRGenericWalletManager gwm);

    extern void
    gwmSync (BRGenericWalletManager gwm);

#if 0
    extern BRArrayOf (BRGenericTransfer) // BRSetOf
    gwmRestorePersistentTransfers (BRGenericWalletManager gwm);

    extern void
    gwmPersistTransfer (BRGenericWalletManager gwm,
                        BRGenericTransfer  tid);
#endif
    extern BRGenericAddress
    gwmGetAccountAddress (BRGenericWalletManager gwm);

    extern BRGenericWallet
    gwmCreatePrimaryWallet (BRGenericWalletManager gwm);

    extern BRGenericHandlers
    gwmGetHandlers (BRGenericWalletManager gwm);

    extern BRGenericAccount
    gwmGetAccount (BRGenericWalletManager gwm);

    extern BRGenericTransfer
    gwmRecoverTransfer (BRGenericWalletManager gwm,
                        uint8_t *bytes,
                        size_t   bytesCount);

    extern void
    gwmWipe (const char *type,
             const char *storagePath);

#ifdef __cplusplus
}
#endif

#endif /* BRGenericWalletManager_h */
