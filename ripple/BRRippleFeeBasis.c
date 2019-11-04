//
//  BRRippleFeeBasis.c
//  Core
//
//  Created by Carl Cherry on 9/27/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include <stdint.h>
#include "BRRippleFeeBasis.h"
#include <stdio.h>

extern BRRippleUnitDrops rippleFeeBasisGetPricePerCostFactor(BRRippleFeeBasis *feeBasis)
{
    return feeBasis ? feeBasis->pricePerCostFactor : 10;
}

extern uint32_t rippleFeeBasisGetCostFactor(BRRippleFeeBasis *feeBasis)
{
    return feeBasis ? feeBasis->costFactor : 1;
}

extern uint32_t rippleFeeBasisIsEqual(BRRippleFeeBasis *fb1, BRRippleFeeBasis *fb2)
{
    assert(fb1);
    assert(fb2);
    if (fb1->pricePerCostFactor == fb2->pricePerCostFactor &&
        fb1->costFactor == fb2->costFactor) {
        return 1;
    }
    return 0;
}

