//
//  BRCryptoCoder.c
//  BRCore
//
//  Created by Michael Carrara on 9/23/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRCryptoCoder.h"
#include "ethereum/util/BRUtilHex.h"
#include "support/BRBase58.h"

static void
cryptoCoderRelease (BRCryptoCoder coder);

struct BRCryptoCoderRecord {
    BRCryptoCoderType type;
    BRCryptoRef ref;
};

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
            // for an unsupported algorithm, assert and return 0
            assert (0);
            length = 0;
            break;
        }
    }

    return length;
}

extern size_t
cryptoCoderEncode (BRCryptoCoder coder,
                   char *dst,
                   size_t dstLen,
                   const uint8_t *src,
                   size_t srcLen) {
    size_t length = cryptoCoderEncodeLength (coder,
                                             src,
                                             srcLen);

    // for NULL or insufficient length buffer, return the length required
    if (NULL == dst || dstLen < length) {
        return length;
    // ... for non-NULL dst buffer but NULL src, assert and return 0
    } else if (NULL == src) {
        assert (0);
        return 0;
    }

    switch (coder->type) {
        case CRYPTO_CODER_HEX: {
            encodeHex (dst, dstLen, src, srcLen);
            break;
        }
        case CRYPTO_CODER_BASE58: {
            BRBase58Encode (dst, dstLen, src, srcLen);
            break;
        }
        case CRYPTO_CODER_BASE58CHECK: {
            BRBase58CheckEncode (dst, dstLen, src, srcLen);
            break;
        }
        default: {
            assert (0);
            break;
        }
    }

    return length;
}

extern size_t
cryptoCoderDecodeLength (BRCryptoCoder coder,
                         const char *src,
                         size_t srcLen) {
    size_t length = 0;

    switch (coder->type) {
        case CRYPTO_CODER_HEX: {
            length = (0 == srcLen % 2) ? decodeHexLength (srcLen) : 0;
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
            // for an unsupported algorithm, assert and return 0
            assert (0);
            length = 0;
            break;
        }
    }

    return length;
}

extern size_t
cryptoCoderDecode (BRCryptoCoder coder,
                   uint8_t *dst,
                   size_t dstLen,
                   const char *src,
                   size_t srcLen) {
    size_t length = cryptoCoderDecodeLength (coder,
                                             src,
                                             srcLen);

    // for NULL or insufficient length buffer, return the length required
    if (NULL == dst || dstLen < length) {
        return length;
    // ... for non-NULL dst buffer but NULL src, assert and return 0
    } else if (NULL == src) {
        assert (0);
        return 0;
    }

    switch (coder->type) {
        case CRYPTO_CODER_HEX: {
            decodeHex (dst, dstLen, src, srcLen);
            break;
        }
        case CRYPTO_CODER_BASE58: {
            BRBase58Decode (dst, dstLen, src);
            break;
        }
        case CRYPTO_CODER_BASE58CHECK: {
            BRBase58CheckDecode (dst, dstLen, src);
            break;
        }
        default: {
            assert (0);
            break;
        }
    }

    return length;
}

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoCoder, cryptoCoder);
