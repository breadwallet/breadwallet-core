//
//  BRCryptoWalletManagerPrivate.h
//  BRCrypto
//
//  Created by Michael Carrara on 6/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoWalletManagerPrivate_h
#define BRCryptoWalletManagerPrivate_h

#include <pthread.h>

#include "BRCryptoBase.h"
#include "BRCryptoNetwork.h"
#include "BRCryptoAccount.h"
#include "BRCryptoWallet.h"
#include "BRCryptoWalletManager.h"

#include "ethereum/BREthereum.h"
#include "bitcoin/BRWalletManager.h"
#include "generic/BRGeneric.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BRCryptoWalletManagerRecord {
    pthread_mutex_t lock;

    BRCryptoBlockChainType type;
    union {
        BRWalletManager btc;
        BREthereumEWM eth;
        BRGenericManager gen;
    } u;

    BRCryptoCWMListener listener;
    BRCryptoCWMClient client;
    BRCryptoNetwork network;
    BRCryptoAccount account;
    BRCryptoAddressScheme addressScheme;

    BRCryptoWalletManagerState state;

    /// The primary wallet
    BRCryptoWallet wallet;

    /// All wallets
    BRArrayOf(BRCryptoWallet) wallets;
    char *path;

    BRCryptoRef ref;
};

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoWalletManagerPrivate_h */
