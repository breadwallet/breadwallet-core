//
//  BRCryptoSigner.c
//  BRCore
//
//  Created by Michael Carrara on 9/23/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <assert.h>
#include <stdlib.h>

#include "BRCryptoSigner.h"
#include "BRCryptoKeyP.h"
#include "support/BRCrypto.h"

struct BRCryptoSignerRecord {
    BRCryptoSignerType type;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoSigner, cryptoSigner);

extern BRCryptoSigner
cryptoSignerCreate(BRCryptoSignerType type) {
    BRCryptoSigner signer = NULL;

    switch (type) {
        case CRYPTO_SIGNER_BASIC_DER:
        case CRYPTO_SIGNER_BASIC_JOSE:
        case CRYPTO_SIGNER_COMPACT: {
            signer = calloc (1, sizeof(struct BRCryptoSignerRecord));
            signer->type = type;
            signer->ref = CRYPTO_REF_ASSIGN(cryptoSignerRelease);
            break;
        }
        default: {
            // for an unsupported algorithm, assert
            assert (0);
            break;
        }
    }

    return signer;
}

static void
cryptoSignerRelease (BRCryptoSigner signer) {
    memset (signer, 0, sizeof(*signer));
    free (signer);
}

extern size_t
cryptoSignerSignLength (BRCryptoSigner signer,
                        BRCryptoKey key,
                        const uint8_t *src,
                        size_t srcLen) {
    // - key CANNOT be NULL
    // - src CANNOT be NULL and must be 32 bytes long (i.e. a UINT256)
    if (NULL == key ||
        NULL == src || 32 != srcLen) {
        assert (0);
        return 0;
    }

    size_t length = 0;

    switch (signer->type) {
        case CRYPTO_SIGNER_BASIC_DER: {
            length = BRKeySign (cryptoKeyGetCore (key), NULL, 0, UInt256Get (src));
            break;
        }
        case CRYPTO_SIGNER_BASIC_JOSE: {
            length = BRKeySignJOSE (cryptoKeyGetCore (key), NULL, 0, UInt256Get (src));
            break;
        }
        case CRYPTO_SIGNER_COMPACT: {
            length = BRKeyCompactSign (cryptoKeyGetCore (key), NULL, 0, UInt256Get (src));
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
cryptoSignerSign (BRCryptoSigner signer,
                  BRCryptoKey key,
                  uint8_t *dst,
                  size_t dstLen,
                  const uint8_t *src,
                  size_t srcLen) {
    // - key CANNOT be NULL
    // - src CANNOT be NULL and must be 32 bytes long (i.e. a UINT256)
    // - dst MUST be non-NULL and sufficiently sized
    if (NULL == key ||
        NULL == src || 32 != srcLen ||
        NULL == dst || dstLen < cryptoSignerSignLength (signer, key, src, srcLen)) {
        assert (0);
        return CRYPTO_FALSE;
    }

    BRCryptoBoolean result = CRYPTO_FALSE;

    switch (signer->type) {
        case CRYPTO_SIGNER_BASIC_DER: {
            result = AS_CRYPTO_BOOLEAN (BRKeySign (cryptoKeyGetCore (key), dst, dstLen, UInt256Get (src)));
            break;
        }
        case CRYPTO_SIGNER_BASIC_JOSE: {
            result = AS_CRYPTO_BOOLEAN (BRKeySignJOSE (cryptoKeyGetCore (key), dst, dstLen, UInt256Get (src)));
            break;
        }
        case CRYPTO_SIGNER_COMPACT: {
            result = AS_CRYPTO_BOOLEAN (BRKeyCompactSign (cryptoKeyGetCore (key), dst, dstLen, UInt256Get (src)));
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

extern BRCryptoKey
cryptoSignerRecover (BRCryptoSigner signer,
                     const uint8_t *digest,
                     size_t digestLen,
                     const uint8_t *signature,
                     size_t signatureLen) {
    // - digest CANNOT be NULL and must be 32 bytes long (i.e. a UINT256)
    // - signature CANNOT be NULL (size checked independently in recovery methods)
    if (NULL == digest || 32 != digestLen || NULL == signature) {
        assert (0);
        return NULL;
    }

    BRCryptoKey key = NULL;

    switch (signer->type) {
        case CRYPTO_SIGNER_BASIC_DER:
        case CRYPTO_SIGNER_BASIC_JOSE: {
            // not supported, but not necessarily worth an assert
            break;
        }
        case CRYPTO_SIGNER_COMPACT: {
            BRKey k;
            if (1 == BRKeyRecoverPubKey (&k, UInt256Get (digest), signature, signatureLen) ) {
                key = cryptoKeyCreateFromKey (&k);
            }
            BRKeyClean (&k);
            break;
        }
        default: {
            // for an unsupported algorithm, assert
            assert (0);
            break;
        }
    }

    return key;
}
