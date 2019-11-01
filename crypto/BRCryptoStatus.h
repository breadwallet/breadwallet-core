//
//  BRCryptoStatus.c
//  BRCore
//
//  Created by Michael Carrara on 7/31/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoStatus_h
#define BRCryptoStatus_h

#include "BRCryptoBase.h"

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

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoStatus_h */
