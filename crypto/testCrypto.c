//
//  testCrypto.c
//  CoreTests
//
//  Created by Ed Gamble on 3/28/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
#include <assert.h>
#include <math.h>

#include "BRCryptoPrivate.h"
#include "BRCryptoAmount.h"

static void
runCryptoAmountTests (void) {
    BRCryptoBoolean overflow;

    BRCryptoCurrency currency =
        cryptoCurrencyCreate ("Cuids",
                              "Cname",
                              "Ccode",
                              "Ctype",
                              NULL);

    BRCryptoUnit unitBase =
        cryptoUnitCreateAsBase (currency,
                                "UuidsBase",
                                "UnameBase",
                                "UsymbBase");

    BRCryptoUnit unitDef =
        cryptoUnitCreate (currency,
                          "UuidsDef",
                          "UnameDef",
                          "UsymbDef",
                          unitBase,
                          18);

    double value = 25.25434525155732538797258871;

    BRCryptoAmount amountInBase = cryptoAmountCreateDouble (value, unitBase);
    assert (NULL != amountInBase);

    double valueFromBase = cryptoAmountGetDouble (amountInBase, unitBase, &overflow);
    assert (CRYPTO_FALSE == overflow);
    assert (valueFromBase == 25.0);  // In base truncated fraction
    cryptoAmountGive(amountInBase);

    BRCryptoAmount amountInDef  = cryptoAmountCreateDouble (value, unitDef);
    assert (NULL != amountInDef);

    double valueFromDef = cryptoAmountGetDouble (amountInDef, unitDef, &overflow);
    assert (CRYPTO_FALSE == overflow);
    assert (fabs (valueFromDef - value) / value < 1e-10);
    cryptoAmountGive(amountInDef);

    value = 1e50;
    amountInBase = cryptoAmountCreateDouble (value, unitBase);
    assert (NULL != amountInBase);
    valueFromBase = cryptoAmountGetDouble (amountInBase, unitBase, &overflow);
    assert (CRYPTO_FALSE == overflow);
    assert (fabs (valueFromBase - value) / value < 1e-10);
    cryptoAmountGive(amountInBase);

    value = 1e100;
    amountInBase = cryptoAmountCreateDouble (value, unitBase);
    assert (NULL == amountInBase);

    cryptoUnitGive(unitDef);
    cryptoUnitGive(unitBase);
    cryptoCurrencyGive(currency);
}

extern void
runCryptoTests (void) {
    runCryptoAmountTests ();
    return;
}
