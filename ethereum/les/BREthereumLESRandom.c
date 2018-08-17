//
//  BREthereumRandom.c
//  breadwallet
//
//  Created by Lamont Samuels on 4/24/18.
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

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include "BRCrypto.h"
#include "BREthereumLESRandom.h"

#define KECCAK_HASH_SIZE 32
#define NONCE_SIZE 8

/*
 *
 * BREthereumRandomRecord contains fields needed by hmac-drbg
 */
struct BREthereumLESRandomRecord {
    uint8_t k[KECCAK_HASH_SIZE];
    uint8_t v[KECCAK_HASH_SIZE];
};

//
// Public functions
//
extern BREthereumLESRandomContext randomCreate(const void *seed, size_t seedLen) {

    BREthereumLESRandomContext ctx = (BREthereumLESRandomContext) calloc(1, sizeof(struct BREthereumLESRandomRecord));
    
    uint32_t currTime = ((uint32_t)time(NULL));
    uint32_t curPid   = (uint32_t)getpid();
    uint8_t nonce[NONCE_SIZE];
    
    nonce[0] = (currTime >> 24) & 0xFF;
    nonce[1] = (currTime >> 16) & 0xFF;
    nonce[2] = (currTime >> 8) & 0xFF;
    nonce[3] =  currTime & 0xFF;
    nonce[4] = (curPid >> 24) & 0xFF;
    nonce[5] = (curPid >> 16) & 0xFF;
    nonce[6] = (curPid >> 8) & 0xFF;
    nonce[7] =  curPid & 0xFF;
    
    // hmac-drbg with no prediction resistance or additional input
    // K and V must point to buffers of size hashLen, and ps (personalization string) may be NULL
    // to generate additional drbg output, use K and V from the previous call, and set seed, nonce and ps to NULL
    uint8_t dummy;
    //Run the DRB intially to set the K and V for subsequent calls to random context
    BRHMACDRBG(&dummy, 0, ctx->k, ctx->v, BRKeccak256, KECCAK_HASH_SIZE, seed, seedLen, nonce, NONCE_SIZE, NULL, 0);
    
    return ctx;
}
extern void randomRelease(BREthereumLESRandomContext ctx) {
    free(ctx);
}
extern void randomGenData(BREthereumLESRandomContext ctx, uint8_t* data, size_t dataSize) {
    BRHMACDRBG(data, dataSize, ctx->k, ctx->v, BRKeccak256, KECCAK_HASH_SIZE, NULL, 0, NULL, 0, NULL, 0);
}
extern void randomGenPriKey(BREthereumLESRandomContext ctx, BRKey* key) {

    assert(key != NULL);
    UInt256 secret;
    BRHMACDRBG(secret.u8, 32, ctx->k, ctx->v, BRKeccak256, KECCAK_HASH_SIZE, NULL, 0, NULL, 0, NULL, 0);
    BRKeySetSecret(key, &secret, 0);
    var_clean(&secret);
}
extern void randomGenUInt256(BREthereumLESRandomContext ctx, UInt256* out) {
    BRHMACDRBG(out->u8, 32, ctx->k, ctx->v, BRKeccak256, KECCAK_HASH_SIZE, NULL, 0, NULL, 0, NULL, 0);
}
