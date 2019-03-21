//
//  BRCryptoUnit.c
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

#include "BRCryptoUnit.h"

struct BRCryptoUnitRecord {
    BRCryptoCurrency currency;
    char *name;
    char *symbol;
    BRCryptoUnit baseUnit;
    uint8_t powerOffset;
};

extern const char *
cryptoUnitGetName (BRCryptoUnit unit) {
    return unit->name;
}

extern const char *
cryptoUnitGetSymbol (BRCryptoUnit unit) {
    return unit->symbol;
}

extern BRCryptoCurrency
cryptoUnitGetCurrency (BRCryptoUnit unit) {
    return unit->currency;
}

extern BRCryptoUnit
cryptoUnitGetBaseUnit (BRCryptoUnit unit) {
    return NULL == unit->baseUnit ? unit : unit->baseUnit;
}

extern uint8_t
cryptoUnitGetBaseDecimalOffset (BRCryptoUnit unit) {
    return NULL == unit->baseUnit ? 0 : unit->powerOffset;
}

extern BRCryptoBoolean
cryptoUnitIsCompatible (BRCryptoUnit u1,
                        BRCryptoUnit u2) {
    return cryptoCurrencyIsIdentical (u1->currency, u2->currency);
}

extern BRCryptoBoolean
cryptoUnitIsIdentical (BRCryptoUnit u1,
                       BRCryptoUnit u2) {
    return AS_CRYPTO_BOOLEAN (u1 == u2);
}
