//
//  BRCryptoAmountP.h
//  BRCore
//
//  Created by Ed Gamble on 12/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoAmountP_h
#define BRCryptoAmountP_h

#include "BRCryptoAmount.h"

#ifdef __cplusplus
extern "C" {
#endif

private_extern BRCryptoAmount
cryptoAmountCreate (BRCryptoUnit unit,
                    BRCryptoBoolean isNegative,
                    UInt256 value);

private_extern BRCryptoAmount
cryptoAmountCreateInternal (BRCryptoUnit unit,
                            BRCryptoBoolean isNegative,
                            UInt256 value,
                            int takeCurrency);

private_extern UInt256
cryptoAmountGetValue (BRCryptoAmount amount);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoAmountP_h */
