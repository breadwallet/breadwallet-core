//
//  BRCryptoAddress.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRCryptoAddressP.h"

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoAddress, cryptoAddress);

static void
cryptoAddressRelease (BRCryptoAddress address) {
    // TODO: btc, eth, gen ?
    memset (address, 0, sizeof(*address));
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
cryptoAddressCreateAsBTC (BRAddress btc, BRCryptoBoolean isBitcoinAddr) {
    BRCryptoAddress address = cryptoAddressCreate (BLOCK_CHAIN_TYPE_BTC);
    address->u.btc.isBitcoinAddr = isBitcoinAddr;
    address->u.btc.addr = btc;
    return address;
}

private_extern BRCryptoAddress
cryptoAddressCreateAsGEN (BRGenericWalletManager gwm,
                          BRGenericAddress aid) { // TODO: BRGenericAddress - ownership given?
    BRCryptoAddress address = cryptoAddressCreate (BLOCK_CHAIN_TYPE_GEN);
    address->u.gen.gwm = gwm;
    address->u.gen.aid = aid;
    return address;
}

private_extern BRCryptoBlockChainType
cryptoAddressGetType (BRCryptoAddress address) {
    return address->type;
}

private_extern BRAddress
cryptoAddressAsBTC (BRCryptoAddress address,
                    BRCryptoBoolean *isBitcoinAddr) {
    assert (BLOCK_CHAIN_TYPE_BTC == address->type);
    assert (NULL != isBitcoinAddr);
    *isBitcoinAddr = address->u.btc.isBitcoinAddr;
    return address->u.btc.addr;

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


private_extern BRCryptoAddress
cryptoAddressCreateFromStringAsBTC (BRAddressParams params, const char *btcAddress) {
    assert (btcAddress);

    return (BRAddressIsValid (params, btcAddress)
            ? cryptoAddressCreateAsBTC (BRAddressFill(params, btcAddress), CRYPTO_TRUE)
            : NULL);
}

private_extern BRCryptoAddress
cryptoAddressCreateFromStringAsBCH (BRAddressParams params, const char *bchAddress) {
    assert (bchAddress);

    char btcAddr[36];
    return (0 != BRBCashAddrDecode(btcAddr, bchAddress) && !BRAddressIsValid(params, bchAddress)
            ? cryptoAddressCreateAsBTC (BRAddressFill(params, btcAddr), CRYPTO_FALSE)
            : NULL);
}

private_extern BRCryptoAddress
cryptoAddressCreateFromStringAsETH (const char *ethAddress) {
    assert (ethAddress);
    BRCryptoAddress address = NULL;
    if (ETHEREUM_BOOLEAN_TRUE == addressValidateString (ethAddress)) {
        address = cryptoAddressCreate (BLOCK_CHAIN_TYPE_ETH);
        address->u.eth = addressCreate (ethAddress);
    }
    return address;
}

private_extern BRCryptoAddress
cryptoAddressCreateFromStringAsGEN (const char *ethAddress) {
    assert (0);
    return NULL;
}

extern char *
cryptoAddressAsString (BRCryptoAddress address) {
    switch (address->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            if (CRYPTO_TRUE == address->u.btc.isBitcoinAddr)
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
                                ? (0 == strcmp (a1->u.btc.addr.s, a2->u.btc.addr.s) && a1->u.btc.isBitcoinAddr == a2->u.btc.isBitcoinAddr)
                                : ( a1->type == BLOCK_CHAIN_TYPE_ETH
                                   ? ETHEREUM_BOOLEAN_IS_TRUE (addressEqual (a1->u.eth, a2->u.eth))
                                   : gwmAddressEqual (a1->u.gen.gwm, a1->u.gen.aid, a2->u.gen.aid)))));
}
