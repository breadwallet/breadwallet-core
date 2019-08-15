//
//  BREthereumFeeBasis.c
//  Core
//
//  Created by Ed Gamble on 8/13/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BREthereumFeeBasis.h"

extern BREthereumFeeBasis
feeBasisCreate (BREthereumGas limit,
                BREthereumGasPrice price) {
    return (BREthereumFeeBasis) {
        FEE_BASIS_GAS,
        { .gas = { limit, price }}
    };
}

extern BREthereumEther
feeBasisGetFee (BREthereumFeeBasis feeBasis, int *overflow) {  // BREthereumBoolean
    *overflow = 0;

    switch (feeBasis.type) {
        case FEE_BASIS_NONE:
            return etherCreateZero();

        case FEE_BASIS_GAS:
            return etherCreate (mulUInt256_Overflow (feeBasis.u.gas.price.etherPerGas.valueInWEI,
                                                     createUInt256 (feeBasis.u.gas.limit.amountOfGas),
                                                     overflow));
    }
}

