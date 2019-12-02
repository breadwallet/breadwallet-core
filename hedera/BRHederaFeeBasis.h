//
//  BRHederaFeeBasis.h
//  Core
//
//  Created by Carl Cherry on 25/11/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRHederaFeeBasis_h
#define BRHederaFeeBasis_h

#include <stdint.h>
#include "BRHederaBase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    BRHederaUnitTinyBar pricePerCostFactor;
    uint32_t            costFactor;
} BRHederaFeeBasis;

extern BRHederaUnitTinyBar hederaFeeBasisGetPricePerCostFactor(BRHederaFeeBasis *feeBasis);
extern uint32_t hederaFeeBasisGetCostFactor(BRHederaFeeBasis *feeBasis);
extern uint32_t hederaFeeBasisIsEqual(BRHederaFeeBasis *fb1, BRHederaFeeBasis *fb2);

extern BRHederaUnitTinyBar hederaFeeBasisGetFee(BRHederaFeeBasis *feeBasis);

extern void hederaFeeBasisSet (uint32_t costFactor, BRHederaUnitTinyBar pricePerCostFactor, BRHederaFeeBasis *feeBasis);

#ifdef __cplusplus
}
#endif

#endif /* BRHederaFeeBasis_h */
