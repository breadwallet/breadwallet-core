//
//  BRCryptoBaseP.h
//  BRCore
//
//  Created by Ed Gamble on 12/10/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoBaseP_h
#define BRCryptoBaseP_h

#include "BRCryptoBase.h"

/// Private-ish
///
/// This is an implementation detail
///
typedef enum {
    BLOCK_CHAIN_TYPE_BTC,
    BLOCK_CHAIN_TYPE_ETH,
    BLOCK_CHAIN_TYPE_GEN
} BRCryptoBlockChainType;


#endif /* BRCryptoBaseP_h */
