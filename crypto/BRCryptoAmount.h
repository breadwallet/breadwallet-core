//
//  BRCryptoAmount.h
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

#ifndef BRCryptoAmount_h
#define BRCryptoAmount_h

#include "BRCryptoBase.h"
#include "BRCryptoCurrency.h"
#include "BRCryptoUnit.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        CRYPTO_COMPARE_LT,
        CRYPTO_COMPARE_EQ,
        CRYPTO_COMPARE_GT
    } BRCryptoComparison;

    typedef struct BRCryptoAmountRecord *BRCryptoAmount;

    extern BRCryptoAmount
    cryptoAmountCreateDouble (double value,
                              BRCryptoUnit unit);

    extern BRCryptoAmount
    cryptoAmountCreateInteger (int64_t value,
                               BRCryptoUnit unit);

    extern BRCryptoCurrency
    cryptoAmountGetCurrency (BRCryptoAmount amount);
    
    extern BRCryptoBoolean
    cryptoAmountIsNegative (BRCryptoAmount amount);

    extern BRCryptoBoolean
    cryptoAmountIsCompatible (BRCryptoAmount a1,
                              BRCryptoAmount a2);

    extern BRCryptoComparison
    cryptoAmountCompare (BRCryptoAmount a1,
                         BRCryptoAmount a2);

    extern BRCryptoAmount
    cryptoAmountAdd (BRCryptoAmount a1,
                     BRCryptoAmount a2);

    extern BRCryptoAmount
    cryptoAmountSub (BRCryptoAmount a1,
                     BRCryptoAmount a2);

    extern double
    cryptoAmountGetDouble (BRCryptoAmount amount,
                           BRCryptoUnit unit,
                           BRCryptoBoolean *overflow);

    extern uint64_t
    cryptoAmountGetIntegerRaw (BRCryptoAmount amount,
                               BRCryptoBoolean *overflow);
    
    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoAmount, cryptoAmount);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoAmount_h */
