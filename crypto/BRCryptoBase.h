//
//  BRCryptoBase.h
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

#ifndef BRCryptoBase_h
#define BRCryptoBase_h

#include <inttypes.h>

typedef enum {
    CRYPTO_FALSE = 0,
    CRYPTO_TRUE  = 1
} BRCryptoBoolean;

#define AS_CRYPTO_BOOLEAN(zeroIfFalse)   ((zeroIfFalse) ? CRYPTO_TRUE : CRYPTO_FALSE)

typedef uint64_t BRCryptoBlockChainHeight;

// Private-ish
typedef enum {
    BLOCK_CHAIN_TYPE_BTC,
    BLOCK_CHAIN_TYPE_ETH,
    BLOCK_CHAIN_TYPE_GEN
} BRCryptoBlockChainType;

typedef enum {
    SYNC_MODE_BRD_ONLY,
    SYNC_MODE_BRD_WITH_P2P_SEND,
    SYNC_MODE_P2P_WITH_BRD_SYNC,
    SYNC_MODE_P2P_ONLY
} BRCryptoSyncMode;

#endif /* BRCryptoBase_h */
