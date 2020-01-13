//
//  BRCryptoUnit.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRCryptoUnit.h"
#include "support/BRArray.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

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
                          const char *code,
                          const char *name,
                          const char *symbol) {
    BRCryptoUnit unit = malloc (sizeof (struct BRCryptoUnitRecord));

    unit->currency = cryptoCurrencyTake (currency);
    unit->name   = strdup (name);
    unit->symbol = strdup (symbol);
    unit->uids   = malloc (strlen (cryptoCurrencyGetUids(currency)) + 1 + strlen(code) + 1);
    sprintf (unit->uids, "%s:%s", cryptoCurrencyGetUids(currency), code);

    unit->ref = CRYPTO_REF_ASSIGN (cryptoUnitRelease);
    return unit;
}

extern BRCryptoUnit
cryptoUnitCreateAsBase (BRCryptoCurrency currency,
                        const char *code,
                        const char *name,
                        const char *symbol) {
    BRCryptoUnit unit = cryptoUnitCreateInternal (currency, code, name, symbol);

    unit->base = NULL;
    unit->decimals = 0;

    return unit;
}

extern BRCryptoUnit
cryptoUnitCreate (BRCryptoCurrency currency,
                  const char *code,
                  const char *name,
                  const char *symbol,
                  BRCryptoUnit baseUnit,
                  uint8_t powerOffset) {
    assert (NULL != baseUnit);
    BRCryptoUnit unit = cryptoUnitCreateInternal (currency, code, name, symbol);

    unit->base = cryptoUnitTake (baseUnit);
    unit->decimals = powerOffset;

    return unit;
}

static void
cryptoUnitRelease (BRCryptoUnit unit) {
    if (NULL != unit->base) cryptoUnitGive (unit->base);
    cryptoCurrencyGive (unit->currency);
    free (unit->uids);
    free (unit->name);
    free (unit->symbol);

    memset (unit, 0, sizeof(*unit));
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
