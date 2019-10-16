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

extern BREthereumGas
feeBasisGetGasLimit (BREthereumFeeBasis basis) {
    return (FEE_BASIS_GAS == basis.type ? basis.u.gas.limit : gasCreate(0));
}

extern BREthereumGasPrice
feeBasisGetGasPrice (BREthereumFeeBasis basis) {
    return (FEE_BASIS_GAS == basis.type ? basis.u.gas.price : gasPriceCreate(etherCreateZero()));
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

extern BREthereumBoolean
feeBasisEqual (const BREthereumFeeBasis *feeBasis1,
               const BREthereumFeeBasis *feeBasis2) {
    if (feeBasis1 == feeBasis2) return ETHEREUM_BOOLEAN_TRUE;
    if (feeBasis1->type != feeBasis2->type) return ETHEREUM_BOOLEAN_FALSE;
    switch (feeBasis1->type) {
        case FEE_BASIS_NONE:
            return ETHEREUM_BOOLEAN_TRUE;

        case FEE_BASIS_GAS:
            return AS_ETHEREUM_BOOLEAN (ETHEREUM_COMPARISON_EQ == gasCompare (feeBasis1->u.gas.limit, feeBasis2->u.gas.limit) &&
                                        ETHEREUM_COMPARISON_EQ == gasPriceCompare(feeBasis1->u.gas.price, feeBasis2->u.gas.price));
    }
}
