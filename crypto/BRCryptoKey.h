//
//  BRCryptoKey.h
//  BRCore
//
//  Created by Ed Gamble on 7/30/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BRCryptoKey_h
#define BRCryptoKey_h

#include "BRCryptoBase.h"
#include "BRKey.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoKeyRecord *BRCryptoKey;

    private_extern BRCryptoKey
    cryptoKeyCreateFromKey (BRKey *key);

    extern BRCryptoKey
    cryptoKeyCreateFromPhraseWithWords (const char *phrase, const char *words[]);

    extern BRCryptoKey
    cryptoKeyCreateFromSerializationPublic (uint8_t *data, size_t dataCount);

    extern BRCryptoKey
    cryptoKeyCreateFromSerializationPrivate (uint8_t *data, size_t dataCount);

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

    extern int
    cryptoKeySecretMatch (BRCryptoKey key1, BRCryptoKey key2);

    extern int
    cryptoKeyPublicMatch (BRCryptoKey key1, BRCryptoKey key2);

//    extern size_t
//    cryptoKeySign (BRCryptoKey key, void *sig, size_t sigLen, UInt256 md);

    extern BRKey *
    cryptoKeyGetCore (BRCryptoKey key);

    extern void
    cryptoKeyProvidePublicKey (BRCryptoKey key, int compressed);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoKey, cryptoKey);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoKey_h */
