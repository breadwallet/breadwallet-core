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
#include "BRCryptoNetwork.h"

#ifdef __cplusplus
extern "C" {
#endif

    /// MARK: - Address

    typedef struct BRCryptoAddressRecord *BRCryptoAddress;

    ///
    /// Create an address for `string` on `network`.  The string representation of addresses differ
    /// depending on the network.  If `string` is not valid for `network`, then `NULL` is returned.
    ///
    /// @param network
    /// @param string
    ///
    /// @return An Address or NULL
    ///
    extern BRCryptoAddress
    cryptoAddressCreateFromString (BRCryptoNetwork network,
                                   const char *string);

    ///
    /// Returns the address' string representation which is suitable for display.  Note that an
    /// address representing BCH will have a prefix included, typically one of 'bitcoincash' or
    ///'bchtest'.  And, there is not the reverse function of `cryptoAddressCreateFromString()`
    /// whereby the type (BTC, BCH, ETH, ...) is derived from the string - one must know
    /// beforehand in order to process the string.
    ///
    /// @param address the address
    ///
    /// @return A string representation which is newly allocated and must be freed.
    ///
    extern char *
    cryptoAddressAsString (BRCryptoAddress address);

    ///
    /// Compare two address for identity.
    ///
    /// @param a1
    /// @param a2
    ///
    /// @return CRYPTO_TRUE if identical, CRYPTO_FALSE otherwise
    ///
    extern BRCryptoBoolean
    cryptoAddressIsIdentical (BRCryptoAddress a1,
                              BRCryptoAddress a2);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoAddress, cryptoAddress);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoAddress_h */
