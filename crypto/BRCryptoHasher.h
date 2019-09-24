//
//  BRCryptoHasher.h
//  BRCore
//
//  Created by Michael Carrara on 9/23/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoHasher_h
#define BRCryptoHasher_h

#include "BRCryptoBase.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        CRYPTO_HASHER_SHA1,
        CRYPTO_HASHER_SHA224,
        CRYPTO_HASHER_SHA256,
        CRYPTO_HASHER_SHA256_2,
        CRYPTO_HASHER_SHA384,
        CRYPTO_HASHER_SHA512,
        CRYPTO_HASHER_SHA3,
        CRYPTO_HASHER_RMD160,
        CRYPTO_HASHER_HASH160,
        CRYPTO_HASHER_KECCAK256,
        CRYPTO_HASHER_MD5
    } BRCryptoHasherType;

    typedef struct BRCryptoHasherRecord *BRCryptoHasher;

    extern BRCryptoHasher
    cryptoHasherCreate(BRCryptoHasherType type);

    extern size_t
    cryptoHasherLength (BRCryptoHasher hasher);

    extern BRCryptoBoolean
    cryptoHasherHash (BRCryptoHasher hasher,
                      uint8_t *dst,
                      size_t dstLen,
                      const uint8_t *src,
                      size_t srcLen);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoHasher, cryptoHasher);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoHasher_h */
