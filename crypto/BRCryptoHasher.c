//
//  BRCryptoHasher.c
//  BRCore
//
//  Created by Michael Carrara on 9/23/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRCryptoHasher.h"
#include "support/BRCrypto.h"

static void
cryptoHasherRelease (BRCryptoHasher hasher);

struct BRCryptoHasherRecord {
    BRCryptoHasherType type;
    BRCryptoRef ref;
};

extern BRCryptoHasher
cryptoHasherCreate(BRCryptoHasherType type) {
    BRCryptoHasher hasher = NULL;

    switch (type) {
        case CRYPTO_HASHER_SHA1:
        case CRYPTO_HASHER_SHA224:
        case CRYPTO_HASHER_SHA256:
        case CRYPTO_HASHER_SHA256_2:
        case CRYPTO_HASHER_SHA384:
        case CRYPTO_HASHER_SHA512:
        case CRYPTO_HASHER_SHA3:
        case CRYPTO_HASHER_RMD160:
        case CRYPTO_HASHER_HASH160:
        case CRYPTO_HASHER_KECCAK256:
        case CRYPTO_HASHER_MD5: {
            hasher = calloc (1, sizeof(struct BRCryptoHasherRecord));
            hasher->type = type;
            hasher->ref = CRYPTO_REF_ASSIGN(cryptoHasherRelease);
            break;
        }
        default: {
            assert (0);
            break;
        }
    }

    return hasher;
}

static void
cryptoHasherRelease (BRCryptoHasher hasher) {
    memset (hasher, 0, sizeof(*hasher));
    free (hasher);
}

extern size_t
cryptoHasherLength (BRCryptoHasher hasher) {
    size_t length = 0;

    switch (hasher->type) {
        case CRYPTO_HASHER_SHA1: {
            length = 20;
            break;
        }
        case CRYPTO_HASHER_SHA224: {
            length = 28;
            break;
        }
        case CRYPTO_HASHER_SHA256: {
            length = 32;
            break;
        }
        case CRYPTO_HASHER_SHA256_2: {
            length = 32;
            break;
        }
        case CRYPTO_HASHER_SHA384: {
            length = 48;
            break;
        }
        case CRYPTO_HASHER_SHA512: {
            length = 64;
            break;
        }
        case CRYPTO_HASHER_SHA3: {
            length = 32;
            break;
        }
        case CRYPTO_HASHER_RMD160: {
            length = 20;
            break;
        }
        case CRYPTO_HASHER_HASH160: {
            length = 20;
            break;
        }
        case CRYPTO_HASHER_KECCAK256: {
            length = 32;
            break;
        }
        case CRYPTO_HASHER_MD5: {
            length = 16;
            break;
        }
        default: {
            // for an unsupported algorithm, assert and return 0
            assert (0);
            length = 0;
            break;
        }
    }

    return length;
}

extern size_t
cryptoHasherHash (BRCryptoHasher hasher,
                  void *dst,
                  const void *src,
                  size_t srcLen) {
    size_t length = cryptoHasherLength (hasher);

    // for NULL dst buffer, return the length required
    if (NULL == dst) {
        return length;
    // ... for non-NULL dst buffer but NULL src, assert and return 0
    } else if (NULL == src) {
        assert (0);
        return 0;
    }

    switch (hasher->type) {
        case CRYPTO_HASHER_SHA1: {
            BRSHA1 (dst, src, srcLen);
            break;
        }
        case CRYPTO_HASHER_SHA224: {
            BRSHA224 (dst, src, srcLen);
            break;
        }
        case CRYPTO_HASHER_SHA256: {
            BRSHA256 (dst, src, srcLen);
            break;
        }
        case CRYPTO_HASHER_SHA256_2: {
            BRSHA256_2 (dst, src, srcLen);
            break;
        }
        case CRYPTO_HASHER_SHA384: {
            BRSHA384 (dst, src, srcLen);
            break;
        }
        case CRYPTO_HASHER_SHA512: {
            BRSHA512 (dst, src, srcLen);
            break;
        }
        case CRYPTO_HASHER_SHA3: {
            BRSHA3_256 (dst, src, srcLen);
            break;
        }
        case CRYPTO_HASHER_RMD160: {
            BRRMD160 (dst, src, srcLen);
            break;
        }
        case CRYPTO_HASHER_HASH160: {
            BRHash160 (dst, src, srcLen);
            break;
        }
        case CRYPTO_HASHER_KECCAK256: {
            BRKeccak256 (dst, src, srcLen);
            break;
        }
        case CRYPTO_HASHER_MD5: {
            BRMD5 (dst, src, srcLen);
            break;
        }
        default: {
            assert (0);
            break;
        }
    }

    return length;
}

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoHasher, cryptoHasher);
