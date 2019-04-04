//
//  BRCryptoAccount.h
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
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

#ifndef BRCryptoAccount_h
#define BRCryptoAccount_h

#include <inttypes.h>
#include "BRCryptoBase.h"
#include "../ethereum/ewm/BREthereumAccount.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoAccountRecord *BRCryptoAccount;

    extern UInt512
    cryptoAccountDeriveSeed (const char *phrase);

    extern BRCryptoAccount
    cryptoAccountCreate (const char *paperKey);

    extern BRCryptoAccount
    cryptoAccountCreateFromSeed (UInt512 seed);

    extern BRCryptoAccount
    cryptoAccountCreateFromSeedBytes (uint8_t *bytes);

    extern uint64_t
    cryptoAccountGetTimestamp (BRCryptoAccount account);

    extern void
    cryptoAccountSetTimestamp (BRCryptoAccount account,
                               uint64_t timestamp);
    
    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoAccount, cryptoAccount);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoAccount_h */
