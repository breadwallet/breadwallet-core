//
//  BRCryptoCipher.h
//  BRCore
//
//  Created by Michael Carrara on 9/23/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoCipher_h
#define BRCryptoCipher_h

#include "BRCryptoBase.h"
#include "BRCryptoKey.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        CRYPTO_CIPHER_AESECB,
        CRYPTO_CIPHER_CHACHA20_POLY1305,
        CRYPTO_CIPHER_PIGEON
    } BRCryptoCipherType;

    typedef struct BRCryptoCipherRecord *BRCryptoCipher;

    extern BRCryptoCipher
    cryptoCipherCreateForAESECB(const uint8_t *key,
                                size_t keyLen);

    extern BRCryptoCipher
    cryptoCipherCreateForChacha20Poly1305(BRCryptoKey key,
                                          const uint8_t *nonce, size_t nonceLen,
                                          const uint8_t *authenticatedData, size_t authenticatedDataLen);

    extern BRCryptoCipher
    cryptoCipherCreateForPigeon(BRCryptoKey privKey,
                                BRCryptoKey pubKey,
                                const uint8_t *nonce, size_t nonceLen);

    extern size_t
    cryptoCipherEncryptLength (BRCryptoCipher cipher,
                               const uint8_t *plaintext,
                               size_t plaintextLen);

    extern BRCryptoBoolean
    cryptoCipherEncrypt (BRCryptoCipher cipher,
                         uint8_t *ciphertext,
                         size_t ciphertextLen,
                         const uint8_t *plaintext,
                         size_t plaintextLen);

    extern size_t
    cryptoCipherDecryptLength (BRCryptoCipher cipher,
                               const uint8_t *ciphertext,
                               size_t ciphertextLen);

    extern BRCryptoBoolean
    cryptoCipherDecrypt (BRCryptoCipher cipher,
                         uint8_t *plaintext,
                         size_t plaintextLen,
                         const uint8_t *ciphertext,
                         size_t ciphertextLen);

    extern BRCryptoBoolean
    cryptoCipherMigrateBRCoreKeyCiphertext (BRCryptoCipher cipher,
                                            uint8_t *migratedCiphertext,
                                            size_t migratedCiphertextLen,
                                            const uint8_t *originalCiphertext,
                                            size_t originalCiphertextLen);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoCipher, cryptoCipher);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoCipher_h */
