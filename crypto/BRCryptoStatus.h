//
//  BRCryptoStatus.c
//  BRCore
//
//  Created by Michael Carrara on 7/31/19.
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

#ifndef BRCryptoStatus_h
#define BRCryptoStatus_h

#include "BRCryptoBase.h"
#include "ethereum/ewm/BREthereumBase.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        CRYPTO_SUCCESS = 0,
        // Generic catch-all failure. This should only be used as if creating a
        // specific error code does not make sense (you really should create
        // a specifc error code...).
        CRYPTO_ERROR_FAILED,

        // Reference access
        CRYPTO_ERROR_UNKNOWN_NODE = 10000,
        CRYPTO_ERROR_UNKNOWN_TRANSFER,
        CRYPTO_ERROR_UNKNOWN_ACCOUNT,
        CRYPTO_ERROR_UNKNOWN_WALLET,
        CRYPTO_ERROR_UNKNOWN_BLOCK,
        CRYPTO_ERROR_UNKNOWN_LISTENER,

        // Node
        CRYPTO_ERROR_NODE_NOT_CONNECTED = 20000,

        // Transfer
        CRYPTO_ERROR_TRANSFER_HASH_MISMATCH = 30000,
        CRYPTO_ERROR_TRANSFER_SUBMISSION,

        // Numeric
        CRYPTO_ERROR_NUMERIC_PARSE = 40000,

        // Acount
        // Wallet
        // Block
        // Listener
    } BRCryptoStatus;

    extern BRCryptoStatus
        cryptoStatusFromETH (BREthereumStatus status);

    extern BREthereumStatus
        cryptoStatusAsETH (BRCryptoStatus status);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoStatus_h */
