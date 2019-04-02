//
//  BREthereum
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/24/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_H
#define BR_Ethereum_H

#include "base/BREthereumBase.h"
#include "ewm/BREthereumEWM.h"
#include "ewm/BREthereumAccount.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Install 'wordList' as the default BIP39 Word List.  THIS IS SHARED MEMORY; DO NOT FREE wordList.
 *
 * @param wordList
 * @param wordListLength
 * @return
 */
extern int
installSharedWordList (const char *wordList[], int wordListLength);

static inline UInt512
zeroUInt512 (void) {
    return UINT512_ZERO;
}

#ifdef __cplusplus
}
#endif

#endif // BR_Ethereum_H
