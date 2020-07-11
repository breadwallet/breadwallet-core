//
//  BRCryptoUnit.h
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoUnit_h
#define BRCryptoUnit_h

#include "BRCryptoBase.h"
#include "BRCryptoCurrency.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoUnitRecord *BRCryptoUnit;

    private_extern BRCryptoUnit
    cryptoUnitCreateAsBase (BRCryptoCurrency currency,
                            const char *code,
                            const char *name,
                            const char *symbol);

    private_extern BRCryptoUnit
    cryptoUnitCreate (BRCryptoCurrency currency,
                      const char *code,
                      const char *name,
                      const char *symbol,
                      BRCryptoUnit baseUnit,
                      uint8_t powerOffset);


    extern const char *
    cryptoUnitGetUids(BRCryptoUnit unit);

    extern const char *
    cryptoUnitGetName (BRCryptoUnit unit);

    extern const char *
    cryptoUnitGetSymbol (BRCryptoUnit unit);

    /**
     * Returns the unit's currency
     *
     * @param unit the Unit
     *
     * @return The currency w/ an incremented reference count (aka 'taken')
     */
    extern BRCryptoCurrency
    cryptoUnitGetCurrency (BRCryptoUnit unit);

    extern BRCryptoBoolean
    cryptoUnitHasCurrency (BRCryptoUnit unit,
                           BRCryptoCurrency currency);

    /**
     * Returns the unit's base unit.  If unit is itself the base unit then unit is returned
     *
     * @param unit The unit
     *
     * @return the base unit w/ an incremented reference count (aka 'taken')
     */
    extern BRCryptoUnit
    cryptoUnitGetBaseUnit (BRCryptoUnit unit);

    extern uint8_t
    cryptoUnitGetBaseDecimalOffset (BRCryptoUnit unit);

    extern BRCryptoBoolean
    cryptoUnitIsCompatible (BRCryptoUnit u1,
                            BRCryptoUnit u2);

    extern BRCryptoBoolean
    cryptoUnitIsIdentical (BRCryptoUnit u1,
                           BRCryptoUnit u2);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoUnit, cryptoUnit);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoUnit_h */
