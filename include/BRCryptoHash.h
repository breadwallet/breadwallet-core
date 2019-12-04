//
//  BRCryptoHash.h
//  Core
//
//  Created by Ed Gamble on 5/15/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoHash_h
#define BRCryptoHash_h

#include "BRCryptoBase.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoHashRecord *BRCryptoHash;

    extern BRCryptoBoolean
    cryptoHashEqual (BRCryptoHash h1, BRCryptoHash h2);

    extern char *
    cryptoHashString (BRCryptoHash hash);

    extern int
    cryptoHashGetHashValue (BRCryptoHash hash);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoHash, cryptoHash);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoHash_h */
