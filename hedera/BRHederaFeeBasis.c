//
//  BRHederaFeeBasis.c
//  Core
//
//  Created by Carl Cherry on 9/27/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include <stdint.h>
#include "BRHederaFeeBasis.h"
#include "ethereum/util/BRUtilMath.h"
#include <stdio.h>
#include <assert.h>

extern BRHederaUnitTinyBar hederaFeeBasisGetPricePerCostFactor(BRHederaFeeBasis *feeBasis)
{
    return feeBasis ? feeBasis->pricePerCostFactor : 500000;
}

extern uint32_t hederaFeeBasisGetCostFactor(BRHederaFeeBasis *feeBasis)
{
    return feeBasis ? feeBasis->costFactor : 1;
}

extern BRHederaUnitTinyBar hederaFeeBasisGetFee(BRHederaFeeBasis *feeBasis)
{
    return feeBasis ? feeBasis->costFactor * feeBasis->pricePerCostFactor : 500000;
}


extern uint32_t hederaFeeBasisIsEqual(BRHederaFeeBasis *fb1, BRHederaFeeBasis *fb2)
{
    assert(fb1);
    assert(fb2);
    if (fb1->pricePerCostFactor == fb2->pricePerCostFactor &&
        fb1->costFactor == fb2->costFactor) {
        return 1;
    }
    return 0;
}

extern void hederaFeeBasisSet (uint32_t costFactor, BRHederaUnitTinyBar pricePerCostFactor, BRHederaFeeBasis *feeBasis)
{
    assert (feeBasis);
    feeBasis->costFactor = costFactor;
    feeBasis->pricePerCostFactor = pricePerCostFactor;
}

