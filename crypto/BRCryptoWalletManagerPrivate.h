//
//  BRCryptoWalletManagerPrivate.h
//  BRCrypto
//
//  Created by Michael Carrara on 6/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
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
#include "generic/BRGenericWalletManager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BRCryptoWalletManagerRecord {
    pthread_mutex_t lock;

    BRCryptoBlockChainType type;
    union {
        BRWalletManager btc;
        BREthereumEWM eth;
        BRGenericWalletManager gen;
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
