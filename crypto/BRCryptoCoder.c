//
//  BRCryptoCoder.c
//  BRCore
//
//  Created by Michael Carrara on 9/23/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "BRCryptoCoder.h"
#include "ethereum/util/BRUtilHex.h"
#include "support/BRBase58.h"

struct BRCryptoCoderRecord {
    BRCryptoCoderType type;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoCoder, cryptoCoder);

extern BRCryptoCoder
cryptoCoderCreate(BRCryptoCoderType type) {
    BRCryptoCoder coder = NULL;

    switch (type) {
        case CRYPTO_CODER_HEX:
        case CRYPTO_CODER_BASE58:
        case CRYPTO_CODER_BASE58CHECK: {
            coder = calloc (1, sizeof(struct BRCryptoCoderRecord));
            coder->type = type;
            coder->ref = CRYPTO_REF_ASSIGN(cryptoCoderRelease);
            break;
        }
        default: {
            assert (0);
            break;
        }
    }

    return coder;
}

static void
cryptoCoderRelease (BRCryptoCoder coder) {
    memset (coder, 0, sizeof(*coder));
    free (coder);
}

extern size_t
cryptoCoderEncodeLength (BRCryptoCoder coder,
                         const uint8_t *src,
                         size_t srcLen) {
    // - src CANNOT be NULL (see: BRBase58Encode), even with srcLen of 0
    if (NULL == src) {
        assert (0);
        return 0;
    }

    size_t length = 0;

    switch (coder->type) {
        case CRYPTO_CODER_HEX: {
            length = encodeHexLength (srcLen);
            break;
        }
        case CRYPTO_CODER_BASE58: {
            length = BRBase58Encode (NULL, 0, src, srcLen);
            break;
        }
        case CRYPTO_CODER_BASE58CHECK: {
            length = BRBase58CheckEncode (NULL, 0, src, srcLen);
            break;
        }
        default: {
            assert (0);
            break;
        }
    }

    return length;
}

extern BRCryptoBoolean
cryptoCoderEncode (BRCryptoCoder coder,
                   char *dst,
                   size_t dstLen,
                   const uint8_t *src,
                   size_t srcLen) {
    // - src CANNOT be NULL (see: BRBase58Encode), even with srcLen of 0
    // - dst MUST be non-NULL and sufficiently sized
    if (NULL == src ||
        NULL == dst || dstLen < cryptoCoderEncodeLength (coder, src, srcLen)) {
        assert (0);
        return CRYPTO_FALSE;
    }

    BRCryptoBoolean result = CRYPTO_FALSE;

    switch (coder->type) {
        case CRYPTO_CODER_HEX: {
            encodeHex (dst, dstLen, src, srcLen);
            result = CRYPTO_TRUE;
            break;
        }
        case CRYPTO_CODER_BASE58: {
            result = AS_CRYPTO_BOOLEAN (BRBase58Encode (dst, dstLen, src, srcLen));
            break;
        }
        case CRYPTO_CODER_BASE58CHECK: {
            result = AS_CRYPTO_BOOLEAN (BRBase58CheckEncode (dst, dstLen, src, srcLen));
            break;
        }
        default: {
            assert (0);
            break;
        }
    }

    return result;
}

extern size_t
cryptoCoderDecodeLength (BRCryptoCoder coder,
                         const char *src) {
    // - src CANNOT be NULL (see: BRBase58Decode), even with srcLen of 0
    if (NULL == src) {
        assert (0);
        return 0;
    }

    size_t length = 0;

    switch (coder->type) {
        case CRYPTO_CODER_HEX: {
            size_t strLen = strlen (src);
            length = (0 == strLen % 2) ? decodeHexLength (strLen) : 0;
            break;
        }
        case CRYPTO_CODER_BASE58: {
            length = BRBase58Decode (NULL, 0, src);
            break;
        }
        case CRYPTO_CODER_BASE58CHECK: {
            length = BRBase58CheckDecode (NULL, 0, src);
            break;
        }
        default: {
            // for an unsupported algorithm, assert
            assert (0);
            break;
        }
    }

    return length;
}

extern BRCryptoBoolean
cryptoCoderDecode (BRCryptoCoder coder,
                   uint8_t *dst,
                   size_t dstLen,
                   const char *src) {
    // - src CANNOT be NULL (see: BRBase58Decode), even with srcLen of 0
    // - dst MUST be non-NULL and sufficiently sized
    if (NULL == src ||
        NULL == dst || dstLen < cryptoCoderDecodeLength (coder, src)) {
        assert (0);
        return CRYPTO_FALSE;
    }

    BRCryptoBoolean result = CRYPTO_FALSE;

    switch (coder->type) {
        case CRYPTO_CODER_HEX: {
            decodeHex (dst, dstLen, src, strlen (src));
            result = CRYPTO_TRUE;
            break;
        }
        case CRYPTO_CODER_BASE58: {
            result = AS_CRYPTO_BOOLEAN (BRBase58Decode (dst, dstLen, src));
            break;
        }
        case CRYPTO_CODER_BASE58CHECK: {
            result = AS_CRYPTO_BOOLEAN (BRBase58CheckDecode (dst, dstLen, src));
            break;
        }
        default: {
            // for an unsupported algorithm, assert
            assert (0);
            break;
        }
    }

    return result;
}
