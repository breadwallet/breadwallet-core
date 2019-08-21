//
//  BRCryptoAddress.c
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

#include "BRCryptoAddress.h"
#include "BRCryptoBase.h"

#include "bcash/BRBCashAddr.h"
#include "support/BRAddress.h"
#include "ethereum/BREthereum.h"
#include "generic/BRGeneric.h"

static void
cryptoAddressRelease (BRCryptoAddress address);

struct BRCryptoAddressRecord {
    BRCryptoBlockChainType type;
    union {
        struct {
            int isBitcoinCashAddr;
            BRAddress addr;
        } btc;
        BREthereumAddress eth;
        struct {
            BRGenericWalletManager gwm;
            BRGenericAddress aid;
        } gen;
    } u;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoAddress, cryptoAddress);

static void
cryptoAddressRelease (BRCryptoAddress address) {
    //    printf ("Address: Release\n");

    // TODO: btc, eth, gen ?
    free (address);
}

static BRCryptoAddress
cryptoAddressCreate (BRCryptoBlockChainType type) {
    BRCryptoAddress address = malloc (sizeof (struct BRCryptoAddressRecord));

    address->type = type;
    address->ref = CRYPTO_REF_ASSIGN(cryptoAddressRelease);

    return address;
}

private_extern BRCryptoAddress
cryptoAddressCreateAsETH (BREthereumAddress eth) {
    BRCryptoAddress address = cryptoAddressCreate (BLOCK_CHAIN_TYPE_ETH);
    address->u.eth = eth;
    return address;
}

private_extern BRCryptoAddress
cryptoAddressCreateAsBTC (BRAddress btc, int isBitcoinCashAddr) {
    BRCryptoAddress address = cryptoAddressCreate (BLOCK_CHAIN_TYPE_BTC);
    address->u.btc.isBitcoinCashAddr = isBitcoinCashAddr;
    address->u.btc.addr = btc;
    return address;
}

private_extern BRCryptoAddress
cryptoAddressCreateAsGEN (BRGenericWalletManager gwm,
    BRGenericAddress aid) {
    BRCryptoAddress address = cryptoAddressCreate (BLOCK_CHAIN_TYPE_GEN);
    address->u.gen.gwm = gwm;
    address->u.gen.aid = aid;
    return address;
}

private_extern BREthereumAddress
cryptoAddressAsETH (BRCryptoAddress address) {
    assert (BLOCK_CHAIN_TYPE_ETH == address->type);
    return address->u.eth;
}

private_extern BRGenericAddress
cryptoAddressAsGEN (BRCryptoAddress address) {
    assert (BLOCK_CHAIN_TYPE_GEN == address->type);
    return address->u.gen.aid;
}


extern BRCryptoAddress
cryptoAddressCreateFromStringAsBTC (BRAddressParams params, const char *btcAddress) {
    assert (btcAddress);

    return (BRAddressIsValid (params, btcAddress)
            ? cryptoAddressCreateAsBTC (BRAddressFill(params, btcAddress), 0)
            : NULL);
}

extern BRCryptoAddress
cryptoAddressCreateFromStringAsBCH (BRAddressParams params, const char *bchAddress) {
    assert (bchAddress);

    char btcAddr[36];
    return (0 != BRBCashAddrDecode(btcAddr, bchAddress) && !BRAddressIsValid(params, bchAddress)
            ? cryptoAddressCreateAsBTC (BRAddressFill(params, btcAddr), 1)
            : NULL);
}

extern BRCryptoAddress
cryptoAddressCreateFromStringAsETH (const char *ethAddress) {
    assert (ethAddress);
    BRCryptoAddress address = NULL;
    if (ETHEREUM_BOOLEAN_TRUE == addressValidateString (ethAddress)) {
        address = cryptoAddressCreate (BLOCK_CHAIN_TYPE_ETH);
        address->u.eth = addressCreate (ethAddress);
    }
    return address;
}

extern BRCryptoAddress
cryptoAddressCreateFromStringAsGEN (const char *ethAddress) {
    assert (0);
    return NULL;
}

extern char *
cryptoAddressAsString (BRCryptoAddress address) {
    switch (address->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            if (!address->u.btc.isBitcoinCashAddr)
                return strdup (address->u.btc.addr.s);
            else {
                char *result = malloc (55);
                BRBCashAddrEncode(result, address->u.btc.addr.s);
                return result;
            }
        case BLOCK_CHAIN_TYPE_ETH:
            return addressGetEncodedString(address->u.eth, 1);
        case BLOCK_CHAIN_TYPE_GEN:
            return gwmAddressAsString (address->u.gen.gwm, address->u.gen.aid);
    }
}

extern BRCryptoBoolean
cryptoAddressIsIdentical (BRCryptoAddress a1,
                          BRCryptoAddress a2) {
    return AS_CRYPTO_BOOLEAN (a1 == a2 ||
                              (a1->type == a2->type &&
                               (a1->type == BLOCK_CHAIN_TYPE_BTC
                                ? (0 == strcmp (a1->u.btc.addr.s, a2->u.btc.addr.s) && a1->u.btc.isBitcoinCashAddr == a2->u.btc.isBitcoinCashAddr)
                                : ( a1->type == BLOCK_CHAIN_TYPE_ETH
                                   ? ETHEREUM_BOOLEAN_IS_TRUE (addressEqual (a1->u.eth, a2->u.eth))
                                   : gwmAddressEqual (a1->u.gen.gwm, a1->u.gen.aid, a2->u.gen.aid)))));
}

