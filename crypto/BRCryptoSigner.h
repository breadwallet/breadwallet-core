//
//  BRCryptoSigner.h
//  BRCore
//
//  Created by Michael Carrara on 9/23/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoSigner_h
#define BRCryptoSigner_h

#include "BRCryptoBase.h"
#include "BRCryptoKey.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        CRYPTO_SIGNER_BASIC_DER,
        CRYPTO_SIGNER_BASIC_JOSE,
        CRYPTO_SIGNER_COMPACT
    } BRCryptoSignerType;

    typedef struct BRCryptoSignerRecord *BRCryptoSigner;

    extern BRCryptoSigner
    cryptoSignerCreate(BRCryptoSignerType type);

    extern size_t
    cryptoSignerSignLength (BRCryptoSigner signer,
                            BRCryptoKey key,
                            const uint8_t *src,
                            size_t srcLen);

    extern BRCryptoBoolean
    cryptoSignerSign (BRCryptoSigner signer,
                      BRCryptoKey key,
                      uint8_t *dst,
                      size_t dstLen,
                      const uint8_t *src,
                      size_t srcLen);

    extern BRCryptoKey
    cryptoSignerRecover (BRCryptoSigner signer,
                         const uint8_t *digest,
                         size_t digestLen,
                         const uint8_t *signature,
                         size_t signatureLen);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoSigner, cryptoSigner);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoSigner_h */
