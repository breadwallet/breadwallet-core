//
//  BRCryptoKey.h
//  BRCore
//
//  Created by Ed Gamble on 7/30/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoKey_h
#define BRCryptoKey_h

#include "BRCryptoBase.h"

#ifdef __cplusplus
extern "C" {
#endif

    /// MARK: - Crypto Secret

    typedef struct {
        uint8_t data[256/8];
    } BRCryptoSecret;

    static inline void cryptoSecretClear (BRCryptoSecret *secret) {
        memset (secret->data, 0, sizeof (secret->data));
    }

    /// MARK: Crypto Key

    typedef struct BRCryptoKeyRecord *BRCryptoKey;

    extern BRCryptoBoolean
    cryptoKeyIsProtectedPrivate (const char *privateKey);

    extern BRCryptoKey
    cryptoKeyCreateFromSecret (BRCryptoSecret secret);

    extern BRCryptoKey
    cryptoKeyCreateFromPhraseWithWords (const char *phrase, const char *words[]);

    extern BRCryptoKey
    cryptoKeyCreateFromStringProtectedPrivate (const char *privateKey, const char * passphrase);

    extern BRCryptoKey
    cryptoKeyCreateFromStringPrivate (const char *string);

    extern BRCryptoKey
    cryptoKeyCreateFromStringPublic (const char *string);

//    extern BRCryptoKey
//    cryptoKeyCreateFromSerializationPublic (uint8_t *data, size_t dataCount);
//
//    extern BRCryptoKey
//    cryptoKeyCreateFromSerializationPrivate (uint8_t *data, size_t dataCount);
//
    extern BRCryptoKey
    cryptoKeyCreateForPigeon (BRCryptoKey key, uint8_t *nonce, size_t nonceCount);

    extern BRCryptoKey
    cryptoKeyCreateForBIP32ApiAuth (const char *phrase, const char *words[]);

    extern BRCryptoKey
    cryptoKeyCreateForBIP32BitID (const char *phrase, int index, const char *uri,  const char *words[]);

    extern size_t
    cryptoKeySerializePublic (BRCryptoKey key, /* ... */ uint8_t *data, size_t dataCount);

    extern size_t
    cryptoKeySerializePrivate(BRCryptoKey key, /* ... */ uint8_t *data, size_t dataCount);

    extern int
    cryptoKeyHasSecret (BRCryptoKey key);

    extern char *
    cryptoKeyEncodePrivate (BRCryptoKey key);

    extern char *
    cryptoKeyEncodePublic (BRCryptoKey key);

    extern BRCryptoSecret
    cryptoKeyGetSecret (BRCryptoKey key);

    extern int
    cryptoKeySecretMatch (BRCryptoKey key1, BRCryptoKey key2);

    extern int
    cryptoKeyPublicMatch (BRCryptoKey key1, BRCryptoKey key2);

//    extern size_t
//    cryptoKeySign (BRCryptoKey key, void *sig, size_t sigLen, UInt256 md);

    extern void
    cryptoKeyProvidePublicKey (BRCryptoKey key, int useCompressed, int compressed);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoKey, cryptoKey);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoKey_h */
