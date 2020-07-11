//
//  BRHederaTransaction.h
//  Core
//
//  Created by Carl Cherry on Oct. 16, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRHederaCrypto_h
#define BRHederaCrypto_h

#include "support/BRKey.h"
#include "support/BRInt.h"

#ifdef __cplusplus
extern "C" {
#endif

BRKey hederaKeyCreate(UInt512 seed);

void hederaKeyGetPublicKey (BRKey key, uint8_t * publicKey);

#ifdef __cplusplus
}
#endif

#endif // BRHederaCrypto_h
