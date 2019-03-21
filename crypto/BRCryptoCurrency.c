//
//  BRCryptoCurrency.c
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

#include "BRCryptoCurrency.h"

struct BRCryptoCurrencyRecord {
    char *name;
    char *code;
    char *type;
};

extern const char *
cryptoCurrencyGetName (BRCryptoCurrency currency) {
    return currency->code;
}

extern const char *
cryptoCurrencyGetCode (BRCryptoCurrency currency) {
    return currency->code;
}

extern const char *
cryptoCurrencyGetType (BRCryptoCurrency currency) {
    return currency->type;
}

// total supply
// initial supply

// units (aka demoninations)

extern BRCryptoBoolean
cryptoCurrencyIsIdentical (BRCryptoCurrency c1,
                         BRCryptoCurrency c2) {
    return AS_CRYPTO_BOOLEAN (c1 == c2);
}
