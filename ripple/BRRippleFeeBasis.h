//
//  BRRippleFeeBasis.h
//  Core
//
//  Created by Carl Cherry on 9/27/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRippleFeeBasis_h
#define BRRippleFeeBasis_h

#include <stdint.h>
#include "BRRippleBase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    BRRippleUnitDrops pricePerCostFactor;
    uint32_t          costFactor;
} BRRippleFeeBasis;

extern BRRippleUnitDrops rippleFeeBasisGetPricePerCostFactor(BRRippleFeeBasis *feeBasis);
extern uint32_t rippleFeeBasisGetCostFactor(BRRippleFeeBasis *feeBasis);
extern uint32_t rippleFeeBasisIsEqual(BRRippleFeeBasis *fb1, BRRippleFeeBasis *fb2);

#ifdef __cplusplus
}
#endif

#endif /* BRRippleUtils_h */
