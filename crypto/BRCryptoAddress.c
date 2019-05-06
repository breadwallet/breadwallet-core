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

#include "support/BRAddress.h"
#include "ethereum/BREthereum.h"
#include "ripple/BRRipple.h"

static void
cryptoAddressRelease (BRCryptoAddress address);

struct BRCryptoAddressRecord {
    BRCryptoBlockChainType type;
    union {
        BRAddress btc;
        BREthereumAddress eth;
        BRRippleAddress xrp;
    } u;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoAddress, cryptoAddress);

static void
cryptoAddressRelease (BRCryptoAddress address) {
//    printf ("Address: Release\n");
    free (address);
}

/* private */ extern BRCryptoAddress
cryptoAddressCreate (BRCryptoBlockChainType type) {
    BRCryptoAddress address = malloc (sizeof (struct BRCryptoAddressRecord));

    address->type = type;
    address->ref = CRYPTO_REF_ASSIGN(cryptoAddressRelease);

    return address;
}

/* private */ extern BRCryptoAddress
cryptoAddressCreateAsETH (BREthereumAddress eth) {
    BRCryptoAddress address = cryptoAddressCreate (BLOCK_CHAIN_TYPE_ETH);
    address->u.eth = eth;
    return address;
}

/* private */ extern BRCryptoAddress
cryptoAddressCreateAsBTC (BRAddress btc) {
    BRCryptoAddress address = cryptoAddressCreate (BLOCK_CHAIN_TYPE_BTC);
    address->u.btc = btc;
    return address;
}

private_extern BRCryptoAddress
cryptoAddressCreateAsXRP (BRRippleAddress xrp) {
    BRCryptoAddress address = cryptoAddressCreate (BLOCK_CHAIN_TYPE_XRP);
    address->u.xrp = xrp;
    return address;
}

private_extern BRRippleAddress
cryptoAddressAsXRP (BRCryptoAddress address) {
    return address->u.xrp;
}

extern char *
cryptoAddressAsString (BRCryptoAddress address) {
    switch (address->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return strdup (address->u.btc.s);
        case BLOCK_CHAIN_TYPE_ETH:
            return addressGetEncodedString(address->u.eth, 1);
        case BLOCK_CHAIN_TYPE_XRP:
            return "xrp-not-yet";
        case BLOCK_CHAIN_TYPE_GEN:
            return "none";
    }
}

extern BRCryptoBoolean
cryptoAddressIsIdentical (BRCryptoAddress a1,
                          BRCryptoAddress a2) {
    if (a1 == a2) return CRYPTO_TRUE;
    if (a1->type != a2->type) return CRYPTO_FALSE;

    switch (a1->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return AS_CRYPTO_BOOLEAN (0 == strcmp (a1->u.btc.s, a2->u.btc.s));
        case BLOCK_CHAIN_TYPE_ETH:
            return AS_CRYPTO_BOOLEAN (ETHEREUM_BOOLEAN_IS_TRUE (addressEqual (a1->u.eth, a2->u.eth)));
        case BLOCK_CHAIN_TYPE_XRP:
            return AS_CRYPTO_BOOLEAN (0 == memcmp (a1->u.xrp.bytes, a2->u.xrp.bytes, 20));
        case BLOCK_CHAIN_TYPE_GEN:
            return CRYPTO_FALSE;
    }
}

