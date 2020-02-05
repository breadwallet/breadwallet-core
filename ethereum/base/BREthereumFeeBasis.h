//
//  BREthereumFeeBasis.h
//  Core
//
//  Created by Ed Gamble on 8/13/19.
//  Copyright © 2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Fee_Basis_h
#define BR_Ethereum_Fee_Basis_h

#include "BREthereumGas.h"

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
ethFeeBasisCreate (BREthereumGas limit,
                   BREthereumGasPrice price);

extern BREthereumGas
ethFeeBasisGetGasLimit (BREthereumFeeBasis basis);

extern BREthereumGasPrice
ethFeeBasisGetGasPrice (BREthereumFeeBasis basis);

extern BREthereumEther
ethFeeBasisGetFee (BREthereumFeeBasis feeBasis, int *overflow);

extern BREthereumBoolean
ethFeeBasisEqual (const BREthereumFeeBasis *feeBasis1,
                  const BREthereumFeeBasis *feeBasis2);

#endif /* BR_Ethereum_Fee_Basis_h */
