//
//  BRHederaBase.h
//  Core
//
//  Created by Carl Cherry on Oct. 15, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRHederaBase_h
#define BRHederaBase_h

#include <inttypes.h>
#include <stdbool.h>
#include "BRCryptoBase.h"

#ifdef __cplusplus
extern "C" {
#endif

// Declare public/shared items
typedef int64_t BRHederaUnitTinyBar;

typedef struct __hedera_timestamp {
    int64_t seconds; // Number of complete seconds since the start of the epoch
    int32_t nano; // Number of nanoseconds since the start of the last second
} BRHederaTimeStamp;

typedef struct {
    uint8_t bytes[48];
} BRHederaTransactionHash;

#ifdef __cplusplus
}
#endif

#endif /* BRHederaBase_h */
