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

#include <stdlib.h>
#include <string.h>

static void
cryptoCurrencyRelease (BRCryptoCurrency currency);

struct BRCryptoCurrencyRecord {
    char *name;
    char *code;
    char *type;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoCurrency, cryptoCurrency)

/* private */ extern BRCryptoCurrency
cryptoCurrencyCreate (const char *name,
                      const char *code,
                      const char *type) {
    BRCryptoCurrency currency = malloc (sizeof (struct BRCryptoCurrencyRecord));

    currency->name = strdup (name);
    currency->code = strdup (code);
    currency->type = strdup (type);
    currency->ref  = CRYPTO_REF_ASSIGN (cryptoCurrencyRelease);

    return currency;
}

static void
cryptoCurrencyRelease (BRCryptoCurrency currency) {
    printf ("Currency: Release\n");
    free (currency->type);
    free (currency->code);
    free (currency->name);
    free (currency);
}

extern const char *
cryptoCurrencyGetName (BRCryptoCurrency currency) {
    return currency->name;
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

#if 0
extern BRCryptoCurrency
cryptoCurrencyTake (BRCryptoCurrency currency) {
    currency->refs++;
    return currency;
}

extern void
cryptoCurrencyGive (BRCryptoCurrency currency) {
    currency->refs--;
    if (0 == currency->refs)
        cryptoCurrencyRelease (currency);
}
#endif
