//
//  BRCryptoCipher.c
//  BRCore
//
//  Created by Michael Carrara on 9/23/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <assert.h>
#include <stdlib.h>

#include "BRCryptoCipher.h"
#include "BRCryptoKeyP.h"

#include "support/BRBase.h"
#include "support/BRCrypto.h"
#include "support/BRKeyECIES.h"

struct BRCryptoCipherRecord {
    BRCryptoCipherType type;

    union {
        struct {
            uint8_t key[32];
            size_t keyLen;
        } aesecb;

        struct {
            BRCryptoKey key;
            uint8_t nonce[12];
            uint8_t *ad;
            size_t adLen;
        } chacha20;

        struct {
            BRCryptoKey privKey;
            BRCryptoKey pubKey;
            uint8_t nonce[12];
        } pigeon;
    } u;

    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoCipher, cryptoCipher);

static BRCryptoCipher
cryptoCipherCreateInternal(BRCryptoCipherType type) {
    BRCryptoCipher cipher = calloc (1, sizeof(struct BRCryptoCipherRecord));
    cipher->type = type;
    cipher->ref = CRYPTO_REF_ASSIGN(cryptoCipherRelease);
    return cipher;
}

extern BRCryptoCipher
cryptoCipherCreateForAESECB(const uint8_t *key,
                            size_t keyLen) {
    // argument check; early exit
    if (NULL == key || (keyLen != 16 && keyLen != 24 && keyLen != 32)) {
        assert (0);
        return NULL;
    }

    BRCryptoCipher cipher  = cryptoCipherCreateInternal (CRYPTO_CIPHER_AESECB);
    // aesecb.key initialized to zero via calloc
    memcpy (cipher->u.aesecb.key, key, keyLen);
    cipher->u.aesecb.keyLen = keyLen;

    return cipher;
}

extern BRCryptoCipher
cryptoCipherCreateForChacha20Poly1305(BRCryptoKey key,
                                      const uint8_t *nonce, size_t nonceLen,
                                      const uint8_t *ad, size_t adLen) {
    // argument check; early exit
    if (NULL == key || NULL == nonce || nonceLen != 12 ||
        (NULL == ad && 0 != adLen)) {
        assert (0);
        return NULL;
    }

    BRCryptoCipher cipher  = cryptoCipherCreateInternal (CRYPTO_CIPHER_CHACHA20_POLY1305);
    cipher->u.chacha20.key = cryptoKeyTake (key);
    memcpy (cipher->u.chacha20.nonce, nonce, nonceLen);
    if (NULL != ad && 0 != adLen) {
        cipher->u.chacha20.ad = malloc (adLen);
        cipher->u.chacha20.adLen = adLen;
        memcpy (cipher->u.chacha20.ad, ad, adLen);
    }

    return cipher;
}

extern BRCryptoCipher
cryptoCipherCreateForPigeon(BRCryptoKey privKey,
                            BRCryptoKey pubKey,
                            const uint8_t *nonce, size_t nonceLen) {
    // argument check; early exit
    if (NULL == pubKey || NULL == privKey || NULL == nonce || nonceLen != 12) {
        assert (0);
        return NULL;
    }

    BRCryptoCipher cipher  = cryptoCipherCreateInternal (CRYPTO_CIPHER_PIGEON);
    cipher->u.pigeon.privKey = cryptoKeyTake (privKey);
    cipher->u.pigeon.pubKey = cryptoKeyTake (pubKey);
    memcpy (cipher->u.pigeon.nonce, nonce, nonceLen);

    return cipher;
}

static void
cryptoCipherRelease (BRCryptoCipher cipher) {
    switch (cipher->type) {
        case CRYPTO_CIPHER_AESECB: {
            break;
        }
        case CRYPTO_CIPHER_CHACHA20_POLY1305: {
            cryptoKeyGive (cipher->u.chacha20.key);
            if (NULL != cipher->u.chacha20.ad) {
                free (cipher->u.chacha20.ad);
            }
            break;
        }
        case CRYPTO_CIPHER_PIGEON: {
            cryptoKeyGive (cipher->u.pigeon.privKey);
            cryptoKeyGive (cipher->u.pigeon.pubKey);
            break;
        }
        default: {
            break;
        }
    }

    memset (cipher, 0, sizeof(*cipher));
    free (cipher);
}

extern size_t
cryptoCipherEncryptLength (BRCryptoCipher cipher,
                           const uint8_t *src,
                           size_t srcLen) {
    // - src CAN be NULL, if srcLen is 0
    if (NULL == src && 0 != srcLen) {
        assert (0);
        return 0;
    }

    size_t length = 0;
    switch (cipher->type) {
        case CRYPTO_CIPHER_AESECB: {
            length = (0 == srcLen % 16) ? srcLen : 0;
            break;
        }
        case CRYPTO_CIPHER_CHACHA20_POLY1305: {
            BRCryptoSecret secret = cryptoKeyGetSecret (cipher->u.chacha20.key);
            length = BRChacha20Poly1305AEADEncrypt (NULL,
                                                    0,
                                                    secret.data,
                                                    cipher->u.chacha20.nonce,
                                                    src,
                                                    srcLen,
                                                    cipher->u.chacha20.ad,
                                                    cipher->u.chacha20.adLen);
            cryptoSecretClear(&secret);
            break;
        }
        case CRYPTO_CIPHER_PIGEON: {
            length = BRKeyPigeonEncrypt (NULL,
                                         NULL,
                                         0,
                                         NULL,
                                         cipher->u.pigeon.nonce,
                                         src,
                                         srcLen);
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
cryptoCipherEncrypt (BRCryptoCipher cipher,
                     uint8_t *dst,
                     size_t dstLen,
                     const uint8_t *src,
                     size_t srcLen) {
    // - src CAN be NULL, if srcLen is 0
    // - dst MUST be non-NULL and sufficiently sized
    if ((NULL == src && 0 != srcLen) ||
        NULL == dst || dstLen < cryptoCipherEncryptLength (cipher, src, srcLen)) {
        assert (0);
        return CRYPTO_FALSE;
    }

    BRCryptoBoolean result = CRYPTO_FALSE;

    switch (cipher->type) {
        case CRYPTO_CIPHER_AESECB: {
            if (srcLen == dstLen && (0 == srcLen % 16)) {
                memcpy (dst, src, dstLen);
                for (size_t index = 0; index < dstLen; index += 16) {
                    BRAESECBEncrypt (&dst[index], cipher->u.aesecb.key, cipher->u.aesecb.keyLen);
                }
                result = CRYPTO_TRUE;
            }
            break;
        }
        case CRYPTO_CIPHER_CHACHA20_POLY1305: {
            BRCryptoSecret secret = cryptoKeyGetSecret (cipher->u.chacha20.key);
            result = AS_CRYPTO_BOOLEAN (BRChacha20Poly1305AEADEncrypt (dst,
                                                                       dstLen,
                                                                       secret.data,
                                                                       cipher->u.chacha20.nonce,
                                                                       src,
                                                                       srcLen,
                                                                       cipher->u.chacha20.ad,
                                                                       cipher->u.chacha20.adLen));
            cryptoSecretClear(&secret);
            break;
        }
        case CRYPTO_CIPHER_PIGEON: {
            result = AS_CRYPTO_BOOLEAN (BRKeyPigeonEncrypt (cryptoKeyGetCore (cipher->u.pigeon.privKey),
                                                            dst,
                                                            dstLen,
                                                            cryptoKeyGetCore (cipher->u.pigeon.pubKey),
                                                            cipher->u.pigeon.nonce,
                                                            src,
                                                            srcLen));
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

extern size_t
cryptoCipherDecryptLength (BRCryptoCipher cipher,
                           const uint8_t *src,
                           size_t srcLen) {
    // - src CAN be NULL, if srcLen is 0
    if (NULL == src && 0 != srcLen) {
        assert (0);
        return 0;
    }

    size_t length = 0;

    switch (cipher->type) {
        case CRYPTO_CIPHER_AESECB: {
            length = (0 == srcLen % 16) ? srcLen : 0;
            break;
        }
        case CRYPTO_CIPHER_CHACHA20_POLY1305: {
            BRCryptoSecret secret = cryptoKeyGetSecret (cipher->u.chacha20.key);
            length = BRChacha20Poly1305AEADDecrypt (NULL,
                                                    0,
                                                    secret.data,
                                                    cipher->u.chacha20.nonce,
                                                    src,
                                                    srcLen,
                                                    cipher->u.chacha20.ad,
                                                    cipher->u.chacha20.adLen);
            cryptoSecretClear(&secret);
            break;
        }
        case CRYPTO_CIPHER_PIGEON: {
            length = BRKeyPigeonDecrypt (NULL,
                                         NULL,
                                         0,
                                         NULL,
                                         cipher->u.pigeon.nonce,
                                         src,
                                         srcLen);
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
cryptoCipherDecrypt (BRCryptoCipher cipher,
                     uint8_t *dst,
                     size_t dstLen,
                     const uint8_t *src,
                     size_t srcLen) {
    // - src CAN be NULL, if srcLen is 0
    // - dst MUST be non-NULL and sufficiently sized
    if ((NULL == src && 0 != srcLen) ||
        NULL == dst || dstLen < cryptoCipherDecryptLength (cipher, src, srcLen)) {
        assert (0);
        return CRYPTO_FALSE;
    }

    BRCryptoBoolean result = CRYPTO_FALSE;

    switch (cipher->type) {
        case CRYPTO_CIPHER_AESECB: {
            if (srcLen == dstLen && (0 == srcLen % 16)) {
                memcpy (dst, src, dstLen);
                for (size_t index = 0; index < dstLen; index += 16) {
                    BRAESECBDecrypt (&dst[index], cipher->u.aesecb.key, cipher->u.aesecb.keyLen);
                }
                result = CRYPTO_TRUE;
            }
            break;
        }
        case CRYPTO_CIPHER_CHACHA20_POLY1305: {
            BRCryptoSecret secret = cryptoKeyGetSecret (cipher->u.chacha20.key);
            result = AS_CRYPTO_BOOLEAN (BRChacha20Poly1305AEADDecrypt (dst,
                                                                       dstLen,
                                                                       secret.data,
                                                                       cipher->u.chacha20.nonce,
                                                                       src,
                                                                       srcLen,
                                                                       cipher->u.chacha20.ad,
                                                                       cipher->u.chacha20.adLen));
            cryptoSecretClear(&secret);
            break;
        }
        case CRYPTO_CIPHER_PIGEON: {
            result = AS_CRYPTO_BOOLEAN (BRKeyPigeonDecrypt (cryptoKeyGetCore (cipher->u.pigeon.privKey),
                                                            dst,
                                                            dstLen,
                                                            cryptoKeyGetCore (cipher->u.pigeon.pubKey),
                                                            cipher->u.pigeon.nonce,
                                                            src,
                                                            srcLen));
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

static size_t
cryptoCipherDecryptForMigrateLength (BRCryptoCipher cipher,
                                     const uint8_t *src,
                                     size_t srcLen) {
    // - src CAN be NULL, if srcLen is 0
    if (NULL == src && 0 != srcLen) {
        assert (0);
        return 0;
    }

    size_t length = 0;

    switch (cipher->type) {
        case CRYPTO_CIPHER_CHACHA20_POLY1305: {
            BRKey *coreKey  = cryptoKeyGetCore (cipher->u.chacha20.key);

            uint8_t pubKeyBytes[65];
            size_t pubKeyLen = BRKeyPubKey (coreKey, pubKeyBytes, sizeof(pubKeyBytes));
            if (0 == pubKeyLen || pubKeyLen > sizeof(pubKeyBytes)) break;

            UInt256 secret;
            BRSHA256 (&secret, &pubKeyBytes[1], pubKeyLen - 1);
            length = BRChacha20Poly1305AEADDecrypt (NULL,
                                                    0,
                                                    &secret,
                                                    cipher->u.chacha20.nonce,
                                                    src,
                                                    srcLen,
                                                    cipher->u.chacha20.ad,
                                                    cipher->u.chacha20.adLen);
            secret = UINT256_ZERO; (void) &secret;
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

static BRCryptoBoolean
cryptoCipherDecryptForMigrate (BRCryptoCipher cipher,
                               uint8_t *dst,
                               size_t dstLen,
                               const uint8_t *src,
                               size_t srcLen) {
    // - src CAN be NULL, if srcLen is 0
    // - dst MUST be non-NULL and sufficiently sized
    if ((NULL == src && 0 != srcLen) ||
        NULL == dst || dstLen < cryptoCipherDecryptLength (cipher, src, srcLen)) {
        assert (0);
        return CRYPTO_FALSE;
    }

    BRCryptoBoolean result = CRYPTO_FALSE;

    switch (cipher->type) {
        case CRYPTO_CIPHER_CHACHA20_POLY1305: {
            BRKey *coreKey  = cryptoKeyGetCore (cipher->u.chacha20.key);

            uint8_t pubKeyBytes[65];
            size_t pubKeyLen = BRKeyPubKey (coreKey, pubKeyBytes, sizeof(pubKeyBytes));
            if (0 == pubKeyLen || pubKeyLen > sizeof(pubKeyBytes)) break;

            UInt256 secret;
            BRSHA256 (&secret, &pubKeyBytes[1], pubKeyLen - 1);
            result = AS_CRYPTO_BOOLEAN (BRChacha20Poly1305AEADDecrypt (dst,
                                                                       dstLen,
                                                                       &secret,
                                                                       cipher->u.chacha20.nonce,
                                                                       src,
                                                                       srcLen,
                                                                       cipher->u.chacha20.ad,
                                                                       cipher->u.chacha20.adLen));
            secret = UINT256_ZERO; (void) &secret;
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

extern BRCryptoBoolean
cryptoCipherMigrateBRCoreKeyCiphertext (BRCryptoCipher cipher,
                                        uint8_t *migratedCiphertext,
                                        size_t migratedCiphertextLen,
                                        const uint8_t *originalCiphertext,
                                        size_t originalCiphertextLen) {
    // calculate the length of the plaintext using the modified decryption routine
    size_t plaintextLen = cryptoCipherDecryptForMigrateLength (cipher,
                                                               originalCiphertext,
                                                               originalCiphertextLen);
    if (0 == plaintextLen) {
        return CRYPTO_FALSE;
    }

    // allocate the plaintext buffer
    uint8_t *plaintext = (uint8_t *) malloc (plaintextLen);
    if (NULL == plaintext) {
        return CRYPTO_FALSE;
    }

    // decrypt the original ciphertext using the modified decryption routine
    BRCryptoBoolean decryptResult = cryptoCipherDecryptForMigrate (cipher,
                                                                   plaintext,
                                                                   plaintextLen,
                                                                   originalCiphertext,
                                                                   originalCiphertextLen);
    if (CRYPTO_TRUE != decryptResult) {
        memset (plaintext, 0, plaintextLen);
        free (plaintext);
        return CRYPTO_FALSE;
    }

    // calculate the length of the migrated ciphertext using the current encryption routine
    if (migratedCiphertextLen < cryptoCipherEncryptLength (cipher,
                                                           plaintext,
                                                           plaintextLen)) {
        memset (plaintext, 0, plaintextLen);
        free (plaintext);
        return CRYPTO_FALSE;
    }

    // encrypt the plaintext using the current encryption routine
    BRCryptoBoolean encryptResult = cryptoCipherEncrypt (cipher,
                                                         migratedCiphertext,
                                                         migratedCiphertextLen,
                                                         plaintext,
                                                         plaintextLen);

    // release the cipher and plaintext memory
    memset (plaintext, 0, plaintextLen);
    free (plaintext);

    return encryptResult;
}
