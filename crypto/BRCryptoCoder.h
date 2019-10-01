//
//  BRCryptoCoder.h
//  BRCore
//
//  Created by Michael Carrara on 9/23/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoCoder_h
#define BRCryptoCoder_h

#include "BRCryptoBase.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        CRYPTO_CODER_HEX,
        CRYPTO_CODER_BASE58,
        CRYPTO_CODER_BASE58CHECK
    } BRCryptoCoderType;

    typedef struct BRCryptoCoderRecord *BRCryptoCoder;

    extern BRCryptoCoder
    cryptoCoderCreate(BRCryptoCoderType type);

    extern size_t
    cryptoCoderEncodeLength (BRCryptoCoder coder,
                             const uint8_t *src,
                             size_t srcLen);

    extern BRCryptoBoolean
    cryptoCoderEncode (BRCryptoCoder coder,
                       char *dst,
                       size_t dstLen,
                       const uint8_t *src,
                       size_t srcLen);

    extern size_t
    cryptoCoderDecodeLength (BRCryptoCoder coder,
                             const char *src);

    extern BRCryptoBoolean
    cryptoCoderDecode (BRCryptoCoder coder,
                       uint8_t *dst,
                       size_t dstLen,
                       const char *src);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoCoder, cryptoCoder);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoCoder_h */
