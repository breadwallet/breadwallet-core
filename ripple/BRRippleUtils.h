//
//  BRRippleUtils.h
//  Core
//
//  Created by Carl Cherry on 9/06/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRippleUtils_h
#define BRRippleUtils_h

#include <stdint.h>
#include "BRRippleBase.h"

#ifdef __cplusplus
extern "C" {
#endif

extern BRRippleUnitDrops rippleCreateDropsFromString(const char* value);
extern BRRippleTransactionHash rippleCreateTransactionHashFromString(const char* value);

#ifdef __cplusplus
}
#endif

#endif /* BRRippleUtils_h */
