//
//  BRCryptoKeyP.h
//  BRCore
//
//  Created by Ed Gamble on 12/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoKeyP_h
#define BRCryptoKeyP_h

#include "BRCryptoKey.h"
#include "support/BRKey.h"

#ifdef __cplusplus
extern "C" {
#endif

private_extern BRCryptoKey
cryptoKeyCreateFromKey (BRKey *key);

private_extern BRKey *
cryptoKeyGetCore (BRCryptoKey key);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoKeyP_h */
