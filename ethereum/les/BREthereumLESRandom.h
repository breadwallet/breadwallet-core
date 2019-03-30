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

#include <inttypes.h>
#include "support/BRKey.h"
#include "support/BRInt.h"
#include "ethereum/base/BREthereumBase.h"

#ifdef __cplusplus
extern "C" {
#endif



typedef struct BREthereumLESRandomRecord* BREthereumLESRandomContext;

/**
 * Creates a random generator context
 * @param seed - the seed to initialze the random generator
 * @param seedLen - the length of the seed parameter
 * @return BREthereumRandomContext,
 * @post - the returned BREthereumRandomContext needs to be released using ethereumRandomRelease
 */
extern BREthereumLESRandomContext randomCreate(const void *seed, size_t seedLen);

/**
 * Frees the memeory associated with a random context
 * @param ctx - the random generator context
 **/
extern void randomRelease(BREthereumLESRandomContext ctx);

/**
 * Generates random data based on the dataSize parameter
 * @param ctx - the random generator context
 * @param data - the destination location to place the random data
 * @param dataSize - determines the amount of random data to place in the data parameter
 **/
extern void randomGenData(BREthereumLESRandomContext ctx, uint8_t* data, size_t dataSize);

/**
 * Generates a random private key
 * @param ctx - the random generator context
 * @param key - a reference to key that will contain the random private data
 **/
extern void randomGenPriKey(BREthereumLESRandomContext ctx, BRKey* key);

/**
 * Generates a random Int2256
 * @param ctx - the random generator context
* @param out - a reference to UInt256 that will contain the random data 
 **/
extern void randomGenUInt256(BREthereumLESRandomContext ctx, UInt256* out);


#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Random_h */
