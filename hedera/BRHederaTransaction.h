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
#ifndef BRHederaTransaction_h
#define BRHederaTransaction_h

#include "support/BRInt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BRHederaTransactionRecord *BRHederaTransaction;

extern BRHederaTransaction
hederaTransactionCreate(void);

extern size_t hederaTransactionSign (BRHederaTransaction transaction, UInt512 seed);

#ifdef __cplusplus
}
#endif

#endif // BRHederaTransaction_h
