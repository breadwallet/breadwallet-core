//
//  BREthereumBase.h
//  BRCore
//
//  Created by Ed Gamble on 11/19/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

#ifndef BREthereumBase_h
#define BREthereumBase_h

#include "ethereum/base/BREthereumBase.h"

typedef struct BREthereumAccountRecord *BREthereumAccount;
typedef struct BREthereumTransferRecord *BREthereumTransfer;
typedef struct BREthereumWalletRecord *BREthereumWallet;
typedef struct BREthereumEWMRecord *BREthereumEWM;

typedef enum {
    FEE_BASIS_NONE,
    FEE_BASIS_GAS
} BREthereumFeeBasisType;

typedef struct {
    BREthereumFeeBasisType type;
    union {
        struct {
            BREthereumGas limit;
            BREthereumGasPrice price;
        } gas;
    } u;
} BREthereumFeeBasis;

extern BREthereumFeeBasis
feeBasisCreate (BREthereumGas limit,
                BREthereumGasPrice price);

//
// Errors - Right Up Front - 'The Emperor Has No Clothes' ??
//
typedef enum {
    SUCCESS,

    // Reference access
    ERROR_UNKNOWN_NODE,
    ERROR_UNKNOWN_TRANSACTION,
    ERROR_UNKNOWN_ACCOUNT,
    ERROR_UNKNOWN_WALLET,
    ERROR_UNKNOWN_BLOCK,
    ERROR_UNKNOWN_LISTENER,

    // Node
    ERROR_NODE_NOT_CONNECTED,

    // Transfer
    ERROR_TRANSACTION_HASH_MISMATCH,
    ERROR_TRANSACTION_SUBMISSION,

    // Acount
    // Wallet
    // Block
    // Listener

    // Numeric
    ERROR_NUMERIC_PARSE,

} BREthereumStatus;


#endif /* BREthereumBase_h */
