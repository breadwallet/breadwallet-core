//
//  BREthereumRandom.h
//  breadwallet-core Ethereum

//  Created by Lamont Samuels on 5/23/18.
//  Copyright (c) 2018 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BR_Ethereum_Random_h
#define BR_Ethereum_Random_h

#include "BRKey.h"
#include "BRInt.h"
#include "../base/BREthereumBase.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREthereumRandomRecord* BREthereumRandomContext;

extern BREthereumRandomContext ethereumRandomCreate(const void *seed, size_t seedLen);

extern void ethereumRandomRelease(BREthereumRandomContext ctx);

extern void ethereumRandomGenData(BREthereumRandomContext ctx, uint8_t* data, size_t dataSize);

extern void ethereumRandomGenPriKey(BREthereumRandomContext ctx, BRKey* key);

extern void ethereumRandomGenUInt256(BREthereumRandomContext ctx, UInt256* out);


#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Random_h */
