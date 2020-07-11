//
//  BRCryptoCurrency.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRCryptoCurrency.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct BRCryptoCurrencyRecord {
    char *uids;
    char *name;
    char *code;
    char *type;
    char *issuer;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoCurrency, cryptoCurrency)

extern BRCryptoCurrency
cryptoCurrencyCreate (const char *uids,
                      const char *name,
                      const char *code,
                      const char *type,
                      const char *issuer) {
    BRCryptoCurrency currency = malloc (sizeof (struct BRCryptoCurrencyRecord));

    currency->uids = strdup (uids);
    currency->name = strdup (name);
    currency->code = strdup (code);
    currency->type = strdup (type);
    currency->issuer = (NULL == issuer ? NULL : strdup (issuer));
    currency->ref  = CRYPTO_REF_ASSIGN (cryptoCurrencyRelease);

    return currency;
}

static void
cryptoCurrencyRelease (BRCryptoCurrency currency) {
    free (currency->type);
    free (currency->code);
    free (currency->name);
    free (currency->uids);
    if (NULL != currency->issuer) free (currency->issuer);

    memset (currency, 0, sizeof(*currency));
    free (currency);
}

extern const char *
cryptoCurrencyGetUids (BRCryptoCurrency currency) {
    return currency->uids;
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

extern const char *
cryptoCurrencyGetIssuer (BRCryptoCurrency currency) {
    return currency->issuer;
}

// total supply
// initial supply

// units (aka demoninations)

extern BRCryptoBoolean
cryptoCurrencyIsIdentical (BRCryptoCurrency c1,
                           BRCryptoCurrency c2) {
    return AS_CRYPTO_BOOLEAN (c1 == c2
                              || c1->uids == c2->uids
                              || 0 == strcmp (c1->uids, c2->uids));
}
