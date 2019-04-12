//
//  BREthereumRandom.h
//  breadwallet-core Ethereum

//  Created by Lamont Samuels on 5/23/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

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
