//
//  BRRippleBase58.h
//  Core
//
//  Created by Carl Cherry on May 3, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRipple_base58_h
#define BRRipple_base58_h

#include <stdlib.h>
#include "BRRippleBase.h"

size_t rippleEncodeBase58(char *str, size_t strLen, const uint8_t *data, size_t dataLen);
int rippleDecodeBase58(const char* input, uint8_t *output);

#endif
