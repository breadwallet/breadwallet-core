//
//  BRCryptoWalletP.h
//  BRCore
//
//  Created by Ed Gamble on 11/22/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoWalletP_h
#define BRCryptoWalletP_h

#include <pthread.h>

#include "BRCryptoWallet.h"
#include "BRCryptoBaseP.h"

#include "bitcoin/BRWallet.h"
#include "bitcoin/BRWalletManager.h"

#include "ethereum/BREthereum.h"
#include "generic/BRGeneric.h"

#ifdef __cplusplus
extern "C" {
#endif


struct BRCryptoWalletRecord {
    pthread_mutex_t lock;

    BRCryptoBlockChainType type;
    union {
        struct {
            BRWalletManager bwm;
            BRWallet *wid;
        } btc;

        struct {
            BREthereumEWM ewm;
            BREthereumWallet wid;
        } eth;

        // The GEN wallet is owned by the GEN Manager!
        BRGenericWallet gen;
    } u;

    BRCryptoWalletState state;

    BRCryptoUnit unit;
    BRCryptoUnit unitForFee;

    //
    // Do we hold transfers here?  The BRWallet and the BREthereumWallet already hold transfers.
    // Shouldn't we defer to those to get transfers (and then wrap them in BRCryptoTransfer)?
    // Then we avoid caching trouble (in part).  For a newly created transaction (not yet signed),
    // the BRWallet will not hold a BRTransaction however, BREthereumWallet will hold a new
    // BREthereumTransaction. From BRWalet: `assert(tx != NULL && BRTransactionIsSigned(tx));`
    //
    // We are going to have the same
    //
    BRArrayOf (BRCryptoTransfer) transfers;

    BRCryptoRef ref;
};

/// MARK: - Wallet

private_extern BRCryptoBlockChainType
cryptoWalletGetType (BRCryptoWallet wallet);

private_extern void
cryptoWalletSetState (BRCryptoWallet wallet,
                      BRCryptoWalletState state);

private_extern BRWallet *
cryptoWalletAsBTC (BRCryptoWallet wallet);

private_extern BREthereumWallet
cryptoWalletAsETH (BRCryptoWallet wallet);

private_extern BRGenericWallet
cryptoWalletAsGEN (BRCryptoWallet wallet);

private_extern BRCryptoWallet
cryptoWalletCreateAsBTC (BRCryptoUnit unit,
                         BRCryptoUnit unitForFee,
                         BRWalletManager bwm,
                         BRWallet *wid);

private_extern BRCryptoWallet
cryptoWalletCreateAsETH (BRCryptoUnit unit,
                         BRCryptoUnit unitForFee,
                         BREthereumEWM ewm,
                         BREthereumWallet wid);

private_extern BRCryptoWallet
cryptoWalletCreateAsGEN (BRCryptoUnit unit,
                         BRCryptoUnit unitForFee,
                         BRGenericWallet wid);

private_extern BRCryptoTransfer
cryptoWalletFindTransferAsBTC (BRCryptoWallet wallet,
                               BRTransaction *btc);

private_extern BRCryptoTransfer
cryptoWalletFindTransferAsETH (BRCryptoWallet wallet,
                               BREthereumTransfer eth);

private_extern BRCryptoTransfer
cryptoWalletFindTransferAsGEN (BRCryptoWallet wallet,
                               BRGenericTransfer gen);

private_extern void
cryptoWalletAddTransfer (BRCryptoWallet wallet, BRCryptoTransfer transfer);

private_extern void
cryptoWalletRemTransfer (BRCryptoWallet wallet, BRCryptoTransfer transfer);

/// MARK: - Wallet Sweeper

private_extern BRWalletSweeper
cryptoWalletSweeperAsBTC (BRCryptoWalletSweeper sweeper);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoWalletP_h */
