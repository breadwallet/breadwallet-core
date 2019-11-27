//
//  BRCryptoCurrency.h
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoCurrency_h
#define BRCryptoCurrency_h

#include "BRCryptoBase.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoCurrencyRecord *BRCryptoCurrency;

    /**
     * Create a currency
     *
     * @param uids the 'unique identifier string'.  This will be globally unique
     * @param name the name, such as "The Breadwallet Token"
     * @param code the code, such as "BRD"
     * @param type the type, such a 'erc20'
     * @param issuer the issuer or NULL.  For currency derived from an ERC20 token, the issue must
     *    be a 'hex string' (starts with '0x') representing the Smart Contract Address.
     *
     * @return a currency
     */
    extern BRCryptoCurrency
    cryptoCurrencyCreate (const char *uids,
                          const char *name,
                          const char *code,
                          const char *type,
                          const char *issuer);



    extern const char *
    cryptoCurrencyGetUids (BRCryptoCurrency currency);

    extern const char *
    cryptoCurrencyGetName (BRCryptoCurrency currency);

    extern const char *
    cryptoCurrencyGetCode (BRCryptoCurrency currency);

    extern const char *
    cryptoCurrencyGetType (BRCryptoCurrency currency);

    /**
     * Return the currency issuer or NULL if there is none.  For an ERC20-based currency, the
     * issuer will be the Smart Contract Address.
     *
     * @param currency the currency
     *
     *@return the issuer as a string or NULL
     */
    extern const char *
    cryptoCurrencyGetIssuer (BRCryptoCurrency currency);

    extern BRCryptoBoolean
    cryptoCurrencyIsIdentical (BRCryptoCurrency c1,
                               BRCryptoCurrency c2);

    // initial supply
    // total supply

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoCurrency, cryptoCurrency);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoCurrency_h */
