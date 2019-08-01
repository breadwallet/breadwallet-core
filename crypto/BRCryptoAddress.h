//
//  BRCryptoAddress.h
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

#ifndef BRCryptoAddress_h
#define BRCryptoAddress_h

#include "BRCryptoBase.h"
#include "support/BRAddress.h"

#ifdef __cplusplus
extern "C" {
#endif

    /// MARK: - Address

    typedef struct BRCryptoAddressRecord *BRCryptoAddress;

    /**
     * Create an address from a NULL terminated BTC address string.
     *
     * The expected format differs depending on if the address is for mainnet vs
     * testnet, as well as legacy vs segwit. For example, a testnet segwit
     * address string will be of the form "tb1...".
     *
     * @return An address or NULL if the address string is invalid.
     */
    extern BRCryptoAddress
    cryptoAddressCreateFromStringAsBTC (BRAddressParams params, const char *address);

    /**
     * Create an address from a NULL terminated ETH address string.
     *
     * An addresss string will be of the form "0x8fB4..." (with prefix)
     * or "8fB4..." (without prefix).
     *
     * @return An address or NULL if the address string is invalid.
     */
    extern BRCryptoAddress
    cryptoAddressCreateFromStringAsETH (const char *address);

    /**
     * ASSERT currently
     *
     * @return An address or NULL if the address string is invalid.
     */
    extern BRCryptoAddress
    cryptoAddressCreateFromStringAsGEN (const char *ethAddress);

    /**
     * Returns the addresses' string representation which is suitable for display.
     *
     * @param address the addres
     *
     *@return A string representation which is newly allocated and must be freed.
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
