//
//  BRCryptoWalletManager.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
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

#include "BRCryptoWalletManager.h"
#include "BRCryptoBase.h"
#include "BRCryptoPrivate.h"

#include "bitcoin/BRWalletManager.h"
#include "ethereum/BREthereum.h"

struct BRCryptoWalletManagerRecord {
    BRCryptoBlockChainType type;
    union {
        BRWalletManager btc;
        BREthereumEWM eth;
    } u;

    BRCryptoNetwork network;
    BRCryptoAccount account;
    BRCryptoSyncMode mode;
    char *path;
};

static BRCryptoWalletManager
cryptoWalletManagerCreateInternal (BRCryptoBlockChainType type,
                                   BRCryptoNetwork network,
                                   BRCryptoAccount account,
                                   BRCryptoSyncMode mode,
                                   const char *path) {
    BRCryptoWalletManager cwm = malloc (sizeof (struct BRCryptoWalletManagerRecord));

    cwm->type = type;
    cwm->network = network;
    cwm->account = account;
    cwm->mode = mode;
    cwm->path = strdup (path);

    return cwm;
}

extern BRCryptoWalletManager
cryptoWalletManagerCreateAsETH (BRCryptoNetwork network,
                                BRCryptoAccount account,
                                BRCryptoSyncMode mode,
                                const char *path) {
    BRCryptoWalletManager cwm =
    cryptoWalletManagerCreateInternal (BLOCK_CHAIN_TYPE_ETH, network, account, mode, path);

    BREthereumClient client = {

    };
    cwm->u.eth = ewmCreate (cryptoNetworkAsETH(network),
                            cryptoAccountAsETH(account),
                            (BREthereumTimestamp) cryptoAccountGetTimestamp(account),
                            (BREthereumMode) mode,
                            client,
                            path);

    return cwm;
}

extern BRCryptoWalletManager
cryptoWalletManagerCreateAsBTC (BRCryptoNetwork network,
                                BRCryptoAccount account,
                                BRCryptoSyncMode mode,
                                const char *path) {
    BRCryptoWalletManager cwm =
    cryptoWalletManagerCreateInternal (BLOCK_CHAIN_TYPE_ETH, network, account, mode, path);

    BRWalletManagerClient client = {
        NULL,
        NULL,
        NULL
    };

    cwm->u.btc = BRWalletManagerNew (client,
                                     cryptoAccountAsBTC (account),
                                     cryptoNetworkAsBTC (network),
                                     (uint32_t) cryptoAccountGetTimestamp(account),
                                     path);

    return cwm;
}

extern BRCryptoNetwork
cryptoWalletManagerGetNetwork (BRCryptoWalletManager cwm) {
    return cwm->network;
}

extern BRCryptoAccount
cryptoWalletMangerGetAccount (BRCryptoWalletManager cwm) {
    return cwm->account;
}

extern BRCryptoSyncMode
cryptoWalletManagerGetMode (BRCryptoWalletManager cwm) {
    return cwm->mode;
}

extern const char *
cryptoWalletManagerGetPath (BRCryptoWalletManager cwm) {
    return cwm->path;
}

// primary Wallet
// wallets

extern void
cryptoWalletManagerConnect (BRCryptoWalletManager cwm) {
    // Mode
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            break;
    }
}

extern void
cryptoWalletManagerDisconnect (BRCryptoWalletManager cwm) {
    // Mode
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            break;
    }
}

extern void
cryptoWalletManagerSync (BRCryptoWalletManager cwm) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            break;
    }
}


static BRCryptoCWMListener listener = { NULL, NULL };

extern void
cryptoWalletManagerDeclareListener (BRCryptoCWMListener l) {
    listener = l;
}

