//
//  BRCryptoHash.c
//  Core
//
//  Created by Ed Gamble on 5/15/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//


#include "BRCryptoHash.h"
#include "ethereum/util/BRUtilHex.h"

static void
cryptoHashRelease (BRCryptoHash hash);

struct BRCryptoHashRecord {
    BRCryptoBlockChainType type;
    union {
        UInt256 btc;
        BREthereumHash eth;
    } u;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoHash, cryptoHash)

static BRCryptoHash
cryptoHashCreateInternal (BRCryptoBlockChainType type) {
    BRCryptoHash hash = malloc (sizeof (struct BRCryptoHashRecord));

    hash->type = type;
    hash->ref  = CRYPTO_REF_ASSIGN (cryptoHashRelease);

    return hash;
}

private_extern BRCryptoHash
cryptoHashCreateAsBTC (UInt256 btc) {
    BRCryptoHash hash = cryptoHashCreateInternal (BLOCK_CHAIN_TYPE_BTC);
    hash->u.btc = btc;

    return hash;
}

private_extern BRCryptoHash
cryptoHashCreateAsETH (BREthereumHash eth) {
    BRCryptoHash hash = cryptoHashCreateInternal (BLOCK_CHAIN_TYPE_BTC);
    hash->u.eth = eth;

    return hash;
}

static void
cryptoHashRelease (BRCryptoHash hash) {
    free (hash);
}

extern BRCryptoBoolean
cryptoHashEqual (BRCryptoHash h1, BRCryptoHash h2) {
    return (h1->type == h2->type &&
            ((BLOCK_CHAIN_TYPE_BTC == h1->type && UInt256Eq (h1->u.btc, h2->u.btc)) ||
             (BLOCK_CHAIN_TYPE_ETH == h1->type && ETHEREUM_BOOLEAN_TRUE ==  hashEqual (h1->u.eth, h2->u.eth)))
            ? CRYPTO_TRUE
            : CRYPTO_FALSE);
}

extern char *
cryptoHashString (BRCryptoHash hash) {
    switch (hash->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return encodeHexCreate(NULL, hash->u.btc.u8, sizeof(hash->u.btc.u8));

        case BLOCK_CHAIN_TYPE_ETH:
            return hashAsString (hash->u.eth);

        case BLOCK_CHAIN_TYPE_GEN:
            return strdup ("abc");
    }
}

extern int
cryptoHashGetHashValue (BRCryptoHash hash) {
    switch (hash->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return hash->u.btc.u32[0];

        case BLOCK_CHAIN_TYPE_ETH:
            return hashSetValue (&hash->u.eth);

        case BLOCK_CHAIN_TYPE_GEN:
            return 0;
    }
}
