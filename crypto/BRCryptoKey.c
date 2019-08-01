//
//  BRCryptoKey.c
//  BRCore
//
//  Created by Ed Gamble on 7/30/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
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

#include "BRCryptoKey.h"
#include "BRKey.h"
#include "BRBIP39Mnemonic.h"
#include "BRBIP32Sequence.h"
#include "BRKeyECIES.h"

// We create an arbitary BRAddressParams - disconnected from any know BITCOIN, etc address params.
// We do this until we know how the Key is used.  Right now, the only accessors are to get the
// serialized form of the Key.  Is somebody expecting a particular encoding?  Is somebody expectring
// to decode the serialization and then pass it to some other 'string-y' interface?  Until we know
// what those other interfaces require, we don't know appropriate address parameters.
//
// Our serialization/deserialization works because we use the same address params.  If the above
// questions are answered, the various cryptoKeyCreate*() functions will need to specify their
// address params AND (AND AND) the serialization will need to WRITE OUT the specific params.
//
#define CRYPTO_ADDRESS_PARAMS  ((BRAddressParams) { \
    3 + BITCOIN_PUBKEY_PREFIX,  \
    3 + BITCOIN_SCRIPT_PREFIX,  \
    3 + BITCOIN_PRIVKEY_PREFIX, \
    "cry" \
})

static void
cryptoKeyRelease (BRCryptoKey key);

struct BRCryptoKeyRecord {
    BRKey core;
    BRAddressParams coreAddressParams;
    BRCryptoRef ref;
};

static BRCryptoKey
cryptoKeyCreateInternal (BRKey core, BRAddressParams params) {
    BRCryptoKey key = calloc (1, sizeof (struct BRCryptoKeyRecord));

    key->core = core;
    key->coreAddressParams = params;
    key->ref = CRYPTO_REF_ASSIGN(cryptoKeyRelease);

    return key;
}

static void
cryptoKeyRelease (BRCryptoKey key) {
    //    printf ("Key: Release\n");

    BRKeyClean (&key->core);
    free (key);
}

private_extern BRCryptoKey
cryptoKeyCreateFromKey (BRKey *key) {
    return cryptoKeyCreateInternal (*key, CRYPTO_ADDRESS_PARAMS);
}

extern BRCryptoKey
cryptoKeyCreateFromPhraseWithWords (const char *phrase, const char *words[]) {
    if (!BRBIP39PhraseIsValid (words, phrase)) return NULL;

    UInt512 seed;
    BRBIP39DeriveKey (seed.u8, phrase, NULL);

    UInt256 *secret = (UInt256*) &seed;
    BRKey core;
    BRKeySetSecret(&core, secret, 1);

    return cryptoKeyCreateInternal(core, CRYPTO_ADDRESS_PARAMS);
}

static BRAddressParams
cryptoKeyFindAddressParams (const char *string) {
    if (BRPrivKeyIsValid (BITCOIN_ADDRESS_PARAMS, string)) return BITCOIN_ADDRESS_PARAMS;
    if (BRPrivKeyIsValid (BITCOIN_TEST_ADDRESS_PARAMS, string)) return BITCOIN_TEST_ADDRESS_PARAMS;
    return EMPTY_ADDRESS_PARAMS;
}

extern BRCryptoKey
cryptoKeyCreateFromStringPrivate (const char *string) {
    BRAddressParams params = cryptoKeyFindAddressParams(string);

    BRKey core;

    return (1 == BRKeySetPrivKey (&core, params, string)
            ? cryptoKeyCreateInternal (core, params)
            : NULL);
}

extern BRCryptoKey
cryptoKeyCreateForPigeon (BRCryptoKey privateKey, uint8_t *nonce, size_t nonceCount) {
    BRKey pairingKey;

    BRKeyPigeonPairingKey (&privateKey->core, &pairingKey, nonce, nonceCount);

    return cryptoKeyCreateInternal (pairingKey, CRYPTO_ADDRESS_PARAMS);
}

extern BRCryptoKey
cryptoKeyCreateForBIP32ApiAuth (const char *phrase, const char *words[]) {
    if (!BRBIP39PhraseIsValid (words, phrase)) return NULL;


    UInt512 seed;
    BRBIP39DeriveKey (seed.u8, phrase, NULL);

    BRKey core;
    BRBIP32APIAuthKey (&core, &seed, sizeof (UInt512));


    //        BRBIP39DeriveKey(&seed, phrase, nil)
    //        BRBIP32APIAuthKey(&key, &seed, MemoryLayout<UInt512>.size)
    //        seed = UInt512() // clear seed
    //        let pkLen = BRKeyPrivKey(&key, nil, 0)
    //        var pkData = CFDataCreateMutable(secureAllocator, pkLen) as Data
    //        pkData.count = pkLen
    //        guard pkData.withUnsafeMutableBytes({ BRKeyPrivKey(&key, $0.baseAddress?.assumingMemoryBound(to: Int8.self), pkLen) }) == pkLen else { return nil }
    //        key.clean()

    return cryptoKeyCreateInternal(core, CRYPTO_ADDRESS_PARAMS);
}

extern BRCryptoKey
cryptoKeyCreateForBIP32BitID (const char *phrase, int index, const char *uri,  const char *words[]) {
    if (!BRBIP39PhraseIsValid (words, phrase)) return NULL;


    UInt512 seed;
    BRBIP39DeriveKey (seed.u8, phrase, NULL);

    BRKey core;
    BRBIP32BitIDKey (&core, &seed, sizeof(UInt512), index, uri);

    return cryptoKeyCreateInternal(core, CRYPTO_ADDRESS_PARAMS);
}

typedef enum {
    SERIALIZE_PUBLIC_IDENTIFIER = 1,
    SERIALIZE_PRIVATE_IDENTIFIER = 2
} BRCryptoKeySerializeIdentifier;


extern BRCryptoKey
cryptoKeyCreateFromSerializationPublic (uint8_t *data, size_t dataCount) {
    if (dataCount < 1) return NULL;
    if (SERIALIZE_PUBLIC_IDENTIFIER != (BRCryptoKeySerializeIdentifier) data[0]) return NULL;

    BRKey core;

    return (1 == BRKeySetPubKey (&core, &data[1], dataCount - 1)
            ? cryptoKeyCreateInternal (core, CRYPTO_ADDRESS_PARAMS)
            : NULL);
}

extern size_t
cryptoKeySerializePublic (BRCryptoKey key, /* ... */ uint8_t *data, size_t dataCount) {
    size_t publicKeySize = BRKeyPubKey (&key->core, NULL, 0);
    size_t serializeSize = 1 + publicKeySize;

    if (NULL == data) return serializeSize;
    if (dataCount < serializeSize) return 0;

    data[0] = SERIALIZE_PUBLIC_IDENTIFIER;
    BRKeyPubKey (&key->core, &data[1], dataCount);
    return dataCount;
}

extern BRCryptoKey
cryptoKeyCreateFromSerializationPrivate (uint8_t *data, size_t dataCount) {
    if (dataCount < 1) return NULL;
    if (SERIALIZE_PRIVATE_IDENTIFIER != (BRCryptoKeySerializeIdentifier) data[0]) return NULL;

    // Get the AddressParms
    BRAddressParams addressParams = CRYPTO_ADDRESS_PARAMS;
    BRKey core;

    // Make a string
    char privKey[dataCount];
    memcpy (privKey, &data[1], dataCount - 1);
    privKey[dataCount - 1] = 0;

    if (!BRPrivKeyIsValid (addressParams, privKey)) return NULL;

    return (1 == BRKeySetPrivKey (&core, addressParams, privKey)
            ? cryptoKeyCreateInternal (core, addressParams)
            : NULL);
}

extern size_t
cryptoKeySerializePrivate (BRCryptoKey key, /* ... */ uint8_t *data, size_t dataCount) {
    // If we encode the BRAddressParams, we'll need to increate the required dataCount
    size_t privateKeySize = BRKeyPrivKey (&key->core, NULL, 0, key->coreAddressParams);
    size_t serializeSize  = 1 + privateKeySize;

    if (NULL == data) return serializeSize;
    if (dataCount < serializeSize) return 0;

    data[0] = SERIALIZE_PRIVATE_IDENTIFIER;

    // Make a string
    char privKey[dataCount]; // includes 1 for '\0'
    memcpy (privKey, &data[1], dataCount - 1);
    privKey[dataCount - 1] = 0;

    return (0 != BRKeyPrivKey (&key->core, privKey, dataCount - 1, key->coreAddressParams)
            ? dataCount
            : 0);
}



extern int
cryptoKeyHasSecret (BRCryptoKey key) {
    UInt256 zero = UINT256_ZERO;
    return 0 != memcmp (key->core.secret.u8, zero.u8, sizeof (zero.u8));
}

extern char *
cryptoKeyEncodePrivate (BRCryptoKey key) {
    size_t encodedLength = BRKeyPrivKey (&key->core, NULL, 0, key->coreAddressParams);
    char  *encoded = malloc (encodedLength + 1);
    BRKeyPrivKey (&key->core, encoded, encodedLength, key->coreAddressParams);
    encoded[encodedLength] = '\0';
    return encoded;
}


extern int
cryptoKeySecretMatch (BRCryptoKey key1, BRCryptoKey key2) {
    return 0 == memcmp (key1->core.secret.u8, key2->core.secret.u8, sizeof (key1->core.secret));
}

extern int
cryptoKeyPublicMatch (BRCryptoKey key1, BRCryptoKey key2) {
    return 1 == BRKeyPubKeyMatch (&key1->core, &key2->core);
}

//extern size_t
//cryptoKeySign (BRCryptoKey key, void *sig, size_t sigLen, UInt256 md) {
//    return BRKeySign (&key->core, sig, sigLen, md);
//}

extern BRKey *
cryptoKeyGetCore (BRCryptoKey key) {
    return &key->core;
}

extern void
cryptoKeyProvidePublicKey (BRCryptoKey key, int useCompressed, int compressed) {
    if (useCompressed) BRKeySetCompressed (&key->core, compressed);
    BRKeyPubKey (&key->core, NULL, 0);
}

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoKey, cryptoKey);
