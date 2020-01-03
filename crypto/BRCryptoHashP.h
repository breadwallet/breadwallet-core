//
//  BRCryptoHashP.h
//  BRCore
//
//  Created by Ed Gamble on 12/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoHashP_h
#define BRCryptoHashP_h

#include "BRCryptoHash.h"

#include "support/BRInt.h"
#include "support/BRArray.h"
#include "ethereum/base/BREthereumHash.h"
#include "generic/BRGeneric.h"

#ifdef __cplusplus
extern "C" {
#endif

private_extern BRCryptoHash
cryptoHashCreateAsBTC (UInt256 btc);

private_extern BRCryptoHash
cryptoHashCreateAsETH (BREthereumHash eth);

private_extern BRCryptoHash
cryptoHashCreateAsGEN (BRGenericHash gen);

#ifdef __cplusplus
}
#endif


#endif /* BRCryptoHashP_h */
