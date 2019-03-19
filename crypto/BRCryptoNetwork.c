//
//  BRCryptoNetwork.c
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

#include "BRCryptoNetwork.h"
#include "BRCryptoBase.h"

#include "bitcoin/BRChainParams.h"
#include "ethereum/BREthereum.h"


BRArrayOf(BRCryptoNetwork) networks = NULL;
static BRCryptoNetworkListener listener = { NULL, NULL };

extern void
cryptoNetworkDeclareListener (BRCryptoNetworkListener l) {
    if (NULL == networks) array_new (networks, 10);
    listener = l;
}

struct BRCryptoNetworkRecord {
    char *name;
    BRCryptoCurrency currency;

    BRCryptoBlockChainType type;
    union {
        struct {
            uint8_t forkId;
            BRChainParams *params;
        } btc;

        struct {
            uint32_t chainId;
            BREthereumNetwork net;
        } eth;

        struct {

        } gen;
    } u;
};

static BRCryptoNetwork
cryptoNetworkCreate (char *name,
                     BRCryptoCurrency currenncy /*, more */) {
    BRCryptoNetwork network = malloc (sizeof (struct BRCryptoNetworkRecord));

    network->name = strdup (name);
    network->currency = currenncy;

    array_add (networks, network);
    if (NULL != listener.announce)
        listener.announce (listener.context, network);

    return network;
}

extern const char *
cryptoNetworkGetName (BRCryptoNetwork network) {
    return network->name;
}

extern BRCryptoBlockChainHeight
cryptoNetworkGetHeight (BRCryptoNetwork network) {
    return 0;
}

extern BRCryptoCurrency
cryptoNetworkGetCurrency (BRCryptoNetwork network) {
    return network->currency;
}

extern BRArrayOf (BRCryptoCurrency)
cryptoNetworkGetCurrencies (BRCryptoNetwork network) {
    return NULL;
}

extern BRCryptoUnit
cryptoNetworkGetUnitAsBase (BRCryptoNetwork network,
                            BRCryptoCurrency currency) {
    return NULL;
}

extern BRCryptoUnit
cryptoNetworkGetUnitAsDefault (BRCryptoNetwork network,
                               BRCryptoCurrency currency) {
    return NULL;
}

extern BRArrayOf (BRCryptoUnit)
cryptoNetworkGetUnits (BRCryptoNetwork network,
                       BRCryptoCurrency currency) {
    return NULL;
}

/* private */ extern BREthereumNetwork
cryptoNetworkAsETH (BRCryptoNetwork network) {
    assert (BLOCK_CHAIN_TYPE_ETH == network->type);
    return network->u.eth.net;
}

/* private */ extern BRChainParams *
cryptoNetworkAsBTC (BRCryptoNetwork network) {
    assert (BLOCK_CHAIN_TYPE_BTC == network->type);
    return network->u.btc.params;
}
