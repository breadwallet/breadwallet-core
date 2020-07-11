//
//  BRCryptoStatus.c
//  BRCore
//
//  Created by Michael Carrara on 7/31/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRCryptoStatusP.h"
#include "ethereum/BREthereum.h"

extern BRCryptoStatus
    cryptoStatusFromETH (BREthereumStatus status) {
    switch (status) {
        case SUCCESS:                           return CRYPTO_SUCCESS;

        // Reference access
        case ERROR_UNKNOWN_NODE:                return CRYPTO_ERROR_UNKNOWN_NODE;
        case ERROR_UNKNOWN_TRANSACTION:         return CRYPTO_ERROR_UNKNOWN_TRANSFER;
        case ERROR_UNKNOWN_ACCOUNT:             return CRYPTO_ERROR_UNKNOWN_ACCOUNT;
        case ERROR_UNKNOWN_WALLET:              return CRYPTO_ERROR_UNKNOWN_WALLET;
        case ERROR_UNKNOWN_BLOCK:               return CRYPTO_ERROR_UNKNOWN_BLOCK;
        case ERROR_UNKNOWN_LISTENER:            return CRYPTO_ERROR_UNKNOWN_LISTENER;

        // Node
        case ERROR_NODE_NOT_CONNECTED:          return CRYPTO_ERROR_NODE_NOT_CONNECTED;

        // Transfer
        case ERROR_TRANSACTION_HASH_MISMATCH:   return CRYPTO_ERROR_TRANSFER_HASH_MISMATCH;
        case ERROR_TRANSACTION_SUBMISSION:      return CRYPTO_ERROR_TRANSFER_SUBMISSION;

        // Number
        case ERROR_NUMERIC_PARSE:               return CRYPTO_ERROR_NUMERIC_PARSE;

        default:                                return CRYPTO_ERROR_FAILED;
    }
}

extern BREthereumStatus
    cryptoStatusAsETH (BRCryptoStatus status) {
    switch (status) {
        case CRYPTO_SUCCESS:                        return SUCCESS;

        // Reference access
        case CRYPTO_ERROR_UNKNOWN_NODE:             return ERROR_UNKNOWN_NODE;
        case CRYPTO_ERROR_UNKNOWN_TRANSFER:         return ERROR_UNKNOWN_TRANSACTION;
        case CRYPTO_ERROR_UNKNOWN_ACCOUNT:          return ERROR_UNKNOWN_ACCOUNT;
        case CRYPTO_ERROR_UNKNOWN_WALLET:           return ERROR_UNKNOWN_WALLET;
        case CRYPTO_ERROR_UNKNOWN_BLOCK:            return ERROR_UNKNOWN_BLOCK;
        case CRYPTO_ERROR_UNKNOWN_LISTENER:         return ERROR_UNKNOWN_LISTENER;

        // Node
        case CRYPTO_ERROR_NODE_NOT_CONNECTED:       return ERROR_NODE_NOT_CONNECTED;

        // Transfer
        case CRYPTO_ERROR_TRANSFER_HASH_MISMATCH:   return ERROR_TRANSACTION_HASH_MISMATCH;
        case CRYPTO_ERROR_TRANSFER_SUBMISSION:      return ERROR_TRANSACTION_SUBMISSION;

        // Number
        case CRYPTO_ERROR_NUMERIC_PARSE:            return ERROR_NUMERIC_PARSE;

        default:                                    return ERROR_FAILED;
    }
}
