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
#include "BRCryptoPrivate.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void
cryptoUnitRelease (BRCryptoUnit unit);

struct BRCryptoUnitRecord {
    BRCryptoCurrency currency;
    char *uids;
    char *name;
    char *symbol;
    BRCryptoUnit base;
    uint8_t decimals;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoUnit, cryptoUnit)

static BRCryptoUnit
cryptoUnitCreateInternal (BRCryptoCurrency currency,
                          const char *uids,
                         const char *name,
                          const char *symbol) {
    BRCryptoUnit unit = malloc (sizeof (struct BRCryptoUnitRecord));

    unit->currency = cryptoCurrencyTake (currency);
    unit->uids   = strdup (uids);
    unit->name   = strdup (name);
    unit->symbol = strdup (symbol);
    unit->ref = CRYPTO_REF_ASSIGN (cryptoUnitRelease);
    return unit;
}

private_extern BRCryptoUnit
cryptoUnitCreateAsBase (BRCryptoCurrency currency,
                        const char *uids,
                        const char *name,
                        const char *symbol) {
    BRCryptoUnit unit = cryptoUnitCreateInternal (currency, uids, name, symbol);

    unit->base = NULL;
    unit->decimals = 0;

    return unit;
}

private_extern BRCryptoUnit
cryptoUnitCreate (BRCryptoCurrency currency,
                  const char *uids,
                  const char *name,
                  const char *symbol,
                  BRCryptoUnit baseUnit,
                  uint8_t powerOffset) {
    assert (NULL != baseUnit);
    BRCryptoUnit unit = cryptoUnitCreateInternal (currency, uids, name, symbol);

    unit->base = cryptoUnitTake (baseUnit);
    unit->decimals = powerOffset;

    return unit;
}

static void
cryptoUnitRelease (BRCryptoUnit unit) {
    printf ("Unit: Release\n");
    if (NULL != unit->base) cryptoUnitGive (unit->base);
    cryptoCurrencyGive (unit->currency);
    free (unit->uids);
    free (unit->name);
    free (unit->symbol);
    free (unit);
}

private_extern BRArrayOf(BRCryptoUnit)
cryptoUnitTakeAll (BRArrayOf(BRCryptoUnit) units) {
    if (NULL != units)
        for (size_t index = 0; index < array_count (units); index++)
            cryptoUnitTake(units[index]);
    return units;
}

private_extern BRArrayOf(BRCryptoUnit)
cryptoUnitGiveAll (BRArrayOf(BRCryptoUnit) units) {
    for (size_t index = 0; index < array_count (units); index++)
        cryptoUnitGive(units[index]);
    return units;
}

extern const char *
cryptoUnitGetUids(BRCryptoUnit unit) {
    return unit->uids;
}

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
    return cryptoCurrencyTake (unit->currency);
}

extern BRCryptoBoolean
cryptoUnitHasCurrency (BRCryptoUnit unit,
                       BRCryptoCurrency currency) {
    return AS_CRYPTO_BOOLEAN (unit->currency == currency);
}
extern BRCryptoUnit
cryptoUnitGetBaseUnit (BRCryptoUnit unit) {
    return cryptoUnitTake (NULL == unit->base ? unit : unit->base);
}

extern uint8_t
cryptoUnitGetBaseDecimalOffset (BRCryptoUnit unit) {
    return NULL == unit->base ? 0 : unit->decimals;
}

extern BRCryptoBoolean
cryptoUnitIsCompatible (BRCryptoUnit u1,
                        BRCryptoUnit u2) {
    return cryptoCurrencyIsIdentical (u1->currency, u2->currency);
}

extern BRCryptoBoolean
cryptoUnitIsIdentical (BRCryptoUnit u1,
                       BRCryptoUnit u2) {
    return AS_CRYPTO_BOOLEAN (u1 == u2
                              || u1->uids == u2->uids
                              || 0 == strcmp (u1->uids, u2->uids));
}
