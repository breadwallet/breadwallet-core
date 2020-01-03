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
#include "BRCryptoBaseP.h"

#include "support/BRInt.h"
#include "ethereum/base/BREthereumHash.h"
#include "ethereum/util/BRUtilHex.h"
#include "generic/BRGeneric.h"

struct BRCryptoHashRecord {
    BRCryptoBlockChainType type;
    union {
        UInt256 btc;
        BREthereumHash eth;
        BRGenericHash gen;
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
    BRCryptoHash hash = cryptoHashCreateInternal (BLOCK_CHAIN_TYPE_ETH);
    hash->u.eth = eth;

    return hash;
}

private_extern BRCryptoHash
cryptoHashCreateAsGEN (BRGenericHash gen) {
    BRCryptoHash hash = cryptoHashCreateInternal (BLOCK_CHAIN_TYPE_GEN);
    hash->u.gen = gen;

    return hash;
}

static void
cryptoHashRelease (BRCryptoHash hash) {
    memset (hash, 0, sizeof(*hash));
    free (hash);
}

extern BRCryptoBoolean
cryptoHashEqual (BRCryptoHash h1, BRCryptoHash h2) {
    return (h1->type == h2->type &&
            ((BLOCK_CHAIN_TYPE_BTC == h1->type && UInt256Eq (h1->u.btc, h2->u.btc)) ||
             (BLOCK_CHAIN_TYPE_ETH == h1->type && ETHEREUM_BOOLEAN_TRUE ==  hashEqual (h1->u.eth, h2->u.eth)) ||
             (BLOCK_CHAIN_TYPE_GEN == h1->type && genericHashEqual (h1->u.gen, h2->u.gen)))
            ? CRYPTO_TRUE
            : CRYPTO_FALSE);
}

static char *
_cryptoHashAddPrefix (char *hash, int freeHash) {
    char *result = malloc (2 + strlen (hash) + 1);
    strcpy (result, "0x");
    strcat (result, hash);
    if (freeHash) free (hash);
    return result;
}

extern char *
cryptoHashString (BRCryptoHash hash) {
    switch (hash->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            UInt256 reversedHash = UInt256Reverse (hash->u.btc);
            return _cryptoHashAddPrefix (encodeHexCreate(NULL, reversedHash.u8, sizeof(reversedHash.u8)), 1);
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            return hashAsString (hash->u.eth);
        }
        case BLOCK_CHAIN_TYPE_GEN: {
            return _cryptoHashAddPrefix (genericHashAsString(hash->u.gen), 1);
        }
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
            return genericHashSetValue (hash->u.gen);
    }
}
