//
//  BRCryptoStatusP.h
//  BRCore
//
//  Created by Ed Gamble on 12/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoStatusP_h
#define BRCryptoStatusP_h

#include "BRCryptoStatus.h"
#include "ethereum/BREthereum.h"

#ifdef __cplusplus
extern "C" {
#endif

private_extern BRCryptoStatus
cryptoStatusFromETH (BREthereumStatus status);

private_extern BREthereumStatus
cryptoStatusAsETH (BRCryptoStatus status);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoStatusP_h */
