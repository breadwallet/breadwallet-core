//
//  BRRippleUtils.c
//  Core
//
//  Created by Carl Cherry on 9/06/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include <stdint.h>
#include "BRRippleUtils.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


static uint8_t char2int(char input)
{
    if(input >= '0' && input <= '9')
        return input - '0';
    if(input >= 'A' && input <= 'F')
        return input - 'A' + 10;
    if(input >= 'a' && input <= 'f')
        return input - 'a' + 10;
    return 0;
}
    
static void hex2bin(const char* src, uint8_t * target)
{
    while(*src && src[1])
    {
        *(target++) = (char2int(src[0]) << 4) | (char2int(src[1]) & 0xff);
        src += 2;
    }
}

extern BRRippleTransactionHash rippleCreateTransactionHashFromString(const char* value)
{
    BRRippleTransactionHash hash;
    hex2bin(value, hash.bytes);
    return hash;
}

extern BRRippleUnitDrops rippleCreateDropsFromString(const char* value)
{
    BRRippleUnitDrops drops;
    sscanf(value, "%llu", &drops);
    return drops;
}

#ifdef __cplusplus
}
#endif

