//
//  BRCryptoPrivate.h
//  BRCore
//
//  Created by Ed Gamble on 3/20/19.
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

#ifndef BRCryptoPrivate_h
#define BRCryptoPrivate_h

/// A 'private header' - to define interfaces that are implementation dependent and that require
/// including implementation specific headers.

#include <inttypes.h>

#include "BRCryptoNetwork.h"
#include "BRCryptoAccount.h"

///
#include "../support/BRAddress.h"
#include "../support/BRBIP32Sequence.h"
#include "../bitcoin/BRChainParams.h"
#include "../ethereum/BREthereum.h"

#ifdef __cplusplus
extern "C" {
#endif

    /// MARK: - Currency

    private_extern BRCryptoCurrency
    cryptoCurrencyCreate (const char *name,
                          const char *code,
                          const char *type);


    /// MARK: - Unit

    private_extern BRCryptoUnit
    cryptoUnitCreateAsBase (BRCryptoCurrency currency,
                            const char *name,
                            const char *symbol);

    private_extern BRCryptoUnit
    cryptoUnitCreate (BRCryptoCurrency currency,
                      const char *name,
                      const char *symbol,
                      BRCryptoUnit baseUnit,
                      uint8_t powerOffset);

    /// MARK: - Amount

    private_extern BRCryptoAmount
    cryptoAmountCreate (BRCryptoCurrency currency,
                        BRCryptoBoolean isNegative,
                        UInt256 value);

    /// MARK: - Address

    private_extern BRCryptoAddress
    cryptoAddressCreateAsETH (BREthereumAddress eth);

    private_extern BRCryptoAddress
    cryptoAddressCreateAsBTC (BRAddress btc);

    private_extern BRCryptoAddress
    cryptoAddressCreate (const char *string);

    /// MARK: - Network

    private_extern BREthereumNetwork
    cryptoNetworkAsETH (BRCryptoNetwork network);

    private_extern BRChainParams *
    cryptoNetworkAsBTC (BRCryptoNetwork network);

    /// MARK: - Account

    private_extern BREthereumAccount
    cryptoAccountAsETH (BRCryptoAccount account);

    private_extern BRMasterPubKey
    cryptoAccountAsBTC (BRCryptoAccount account);

#ifdef __cplusplus
}
#endif


#endif /* BRCryptoPrivate_h */
