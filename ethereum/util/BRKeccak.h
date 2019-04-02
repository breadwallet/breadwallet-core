//
//  BRKeccak.h
//  breadwallet-core Ethereum

//  Created by Lamont Samuels on 7/19/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  *****LICENSE DISCLAIMER*****
//  The original code was written by Andrey Jivsov
//  @cite :  Andrey Jivsov crypto@brainhub.org  Aug 2015
//  License : Released in the Public Domain
//  *********************
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Keccak_h
#define BR_Keccak_h

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Ethereum LES Node
 *
 */
typedef struct BRKeccakContext* BRKeccak;

/**
 * Initializes the keccak context to a 256 hash size
 *
 * @param hashCtx - the context that will be initialzed
 */
extern BRKeccak
    keccak_create256(void);

/**
 * Initializes the keccak context to a 384 hash size
 *
 * @param hashCtx - the context that will be initialzed
 */
extern BRKeccak
  keccak_create384(void);
  
/**
 * Initializes the keccak context to a 512 hash size
 *
 * @param hashCtx - the context that will be initialzed
 */
extern BRKeccak
    keccak_create512(void);
    
/**
 * Releases a keccak context
 *
 * @param hashCtx - the context to release
 */
extern void
    keccak_release(BRKeccak hashCtx);

/**
 * Updates the hash context with additional input
 *
 * @param hashCtx - the context that will be initialized
 * @param input - the input buffer to add to the context
 * @param len - the length of the input buffer
 */
extern void
    keccak_update(BRKeccak hashCtx, void const *input, size_t len);

/**
 * Produces a digest of the current data in the context
 * @param hashCtx - the hash context
 * @param output - the location to place the digest
 */
extern void
    keccak_digest(BRKeccak hashCtx, void* output);

/**
 * Finalizes the hash calculation from the context and places the result into output.
 * the amount written to output is determined based on how the context was initialized.
 * After you call final, the algorithm is reset to its initial state
 *
 * @param hashCtx - the hash context
 * @param output - the location to place the digest
 */
extern void
    keccak_final(BRKeccak hashCtx, void* output);


#ifdef __cplusplus
}
#endif

#endif/* BR_Keccak_h */
