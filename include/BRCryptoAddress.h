//
//  BRCryptoAddress.h
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoAddress_h
#define BRCryptoAddress_h

#include "BRCryptoBase.h"

#ifdef __cplusplus
extern "C" {
#endif

    /// MARK: - Address

    typedef struct BRCryptoAddressRecord *BRCryptoAddress;

    /**
     * Returns the address' string representation which is suitable for display.  Note that an
     * address representing BCH will have a prefix included, typically one of 'bitcoincash' or
     * 'bchtest'.  And, there is not the reverse function of `cryptoAddressCreateFromString()`
     * whereby the type (BTC, BCH, ETH, ...) is derived from the string - one must know
     * beforehand in order to process the string.
     *
     * @param address the addres
     *
     * @return A string representation which is newly allocated and must be freed.
     */
    extern char *
    cryptoAddressAsString (BRCryptoAddress address);

    extern BRCryptoBoolean
    cryptoAddressIsIdentical (BRCryptoAddress a1,
                              BRCryptoAddress a2);
    
    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoAddress, cryptoAddress);

    /// MARK: - Address Scheme

    typedef enum {
        CRYPTO_ADDRESS_SCHEME_BTC_LEGACY,
        CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT,
        CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT,
        CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT
    } BRCryptoAddressScheme;

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoAddress_h */
