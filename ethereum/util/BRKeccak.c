//
//  BRKeccak.c
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "BRKeccak.h"

typedef enum  {
    
    KECCAK_256 = 256,
    KECCAK_384 = 384,
    KECCAK_512 = 512

}BRKeccakType;

/* 'Words' here refers to uint64_t */
#define SHA3_KECCAK_SPONGE_WORDS \
    (((1600)/8/*bits to byte*/)/sizeof(uint64_t))
struct BRKeccakContext {
    uint64_t saved;             /* the portion of the input message that we
                                 * didn't consume yet */
    union {                     /* Keccak's state */
        uint64_t s[SHA3_KECCAK_SPONGE_WORDS];
        uint8_t sb[SHA3_KECCAK_SPONGE_WORDS * 8];
    };
    unsigned byteIndex;         /* 0..7--the next byte after the set one
                                 * (starts from 0; 0--none are buffered) */
    unsigned wordIndex;         /* 0..24--the next word to integrate input
                                 * (starts from 0) */
    unsigned capacityWords;     /* the double size of the hash output in
                                 * words (e.g. 16 for Keccak 512) */
    BRKeccakType type;
};

#if defined(_MSC_VER)
#define SHA3_CONST(x) x
#else
#define SHA3_CONST(x) x##L
#endif

#ifndef SHA3_ROTL64
#define SHA3_ROTL64(x, y) \
    (((x) << (y)) | ((x) >> ((sizeof(uint64_t)*8) - (y))))
#endif

static const uint64_t keccakf_rndc[24] = {
    SHA3_CONST(0x0000000000000001UL), SHA3_CONST(0x0000000000008082UL),
    SHA3_CONST(0x800000000000808aUL), SHA3_CONST(0x8000000080008000UL),
    SHA3_CONST(0x000000000000808bUL), SHA3_CONST(0x0000000080000001UL),
    SHA3_CONST(0x8000000080008081UL), SHA3_CONST(0x8000000000008009UL),
    SHA3_CONST(0x000000000000008aUL), SHA3_CONST(0x0000000000000088UL),
    SHA3_CONST(0x0000000080008009UL), SHA3_CONST(0x000000008000000aUL),
    SHA3_CONST(0x000000008000808bUL), SHA3_CONST(0x800000000000008bUL),
    SHA3_CONST(0x8000000000008089UL), SHA3_CONST(0x8000000000008003UL),
    SHA3_CONST(0x8000000000008002UL), SHA3_CONST(0x8000000000000080UL),
    SHA3_CONST(0x000000000000800aUL), SHA3_CONST(0x800000008000000aUL),
    SHA3_CONST(0x8000000080008081UL), SHA3_CONST(0x8000000000008080UL),
    SHA3_CONST(0x0000000080000001UL), SHA3_CONST(0x8000000080008008UL)
};

static const unsigned keccakf_rotc[24] = {
    1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14, 27, 41, 56, 8, 25, 43, 62,
    18, 39, 61, 20, 44
};

static const unsigned keccakf_piln[24] = {
    10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4, 15, 23, 19, 13, 12, 2, 20,
    14, 22, 9, 6, 1
};

/* generally called after SHA3_KECCAK_SPONGE_WORDS-ctx->capacityWords words
 * are XORed into the state s
 */
static void
keccakf(uint64_t s[25])
{
    int i, j, round;
    uint64_t t, bc[5];
#define KECCAK_ROUNDS 24

    for(round = 0; round < KECCAK_ROUNDS; round++) {

        /* Theta */
        for(i = 0; i < 5; i++)
            bc[i] = s[i] ^ s[i + 5] ^ s[i + 10] ^ s[i + 15] ^ s[i + 20];

        for(i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ SHA3_ROTL64(bc[(i + 1) % 5], 1);
            for(j = 0; j < 25; j += 5)
                s[j + i] ^= t;
        }

        /* Rho Pi */
        t = s[1];
        for(i = 0; i < 24; i++) {
            j = keccakf_piln[i];
            bc[0] = s[j];
            s[j] = SHA3_ROTL64(t, keccakf_rotc[i]);
            t = bc[0];
        }

        /* Chi */
        for(j = 0; j < 25; j += 5) {
            for(i = 0; i < 5; i++)
                bc[i] = s[j + i];
            for(i = 0; i < 5; i++)
                s[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        /* Iota */
        s[0] ^= keccakf_rndc[round];
    }
}

//
// Public functions
//
extern BRKeccak
    keccak_create256(void) {
    
    BRKeccak hashCtx = (BRKeccak)malloc(sizeof(struct BRKeccakContext));
    memset(hashCtx, 0, sizeof(*hashCtx));
    hashCtx->capacityWords = 2 * 256 / (8 * sizeof(uint64_t));
    hashCtx->type = KECCAK_256;
    
    return hashCtx;
}

extern BRKeccak
  keccak_create384(void) {
  
    BRKeccak hashCtx = (BRKeccak)malloc(sizeof(struct BRKeccakContext));
    memset(hashCtx, 0, sizeof(*hashCtx));
    hashCtx->capacityWords = 2 * 384 / (8 * sizeof(uint64_t));
    hashCtx->type = KECCAK_384;
    
    return hashCtx;
}

extern BRKeccak
    keccak_create512(void) {
    
    BRKeccak hashCtx = (BRKeccak)malloc(sizeof(struct BRKeccakContext));
    memset(hashCtx, 0, sizeof(*hashCtx));
    hashCtx->capacityWords = 2 * 512 / (8 * sizeof(uint64_t));
    hashCtx->type = KECCAK_512;

    return hashCtx;
}
extern void
    keccak_release(BRKeccak hashCtx) {
    
    free(hashCtx);
}

extern void
    keccak_update(BRKeccak hashCtx, void const *input, size_t len) {
    
    /* 0...7 -- how much is needed to have a word */
    unsigned old_tail = (8 - hashCtx->byteIndex) & 7;

    size_t words;
    unsigned long tail;
    size_t i;

    const uint8_t *buf = input;

    assert(hashCtx->byteIndex < 8);
    assert(hashCtx->wordIndex < sizeof(hashCtx->s) / sizeof(hashCtx->s[0]));

    if(len < old_tail) {        /* have no complete word or haven't started
                                 * the word yet */
      
        /* endian-independent code follows: */
        while (len--)
            hashCtx->saved |= (uint64_t) (*(buf++)) << ((hashCtx->byteIndex++) * 8);
        assert(hashCtx->byteIndex < 8);
        return;
    }

    if(old_tail) {              /* will have one word to process */
        /* endian-independent code follows: */
        len -= old_tail;
        while (old_tail--)
            hashCtx->saved |= (uint64_t) (*(buf++)) << ((hashCtx->byteIndex++) * 8);

        /* now ready to add saved to the sponge */
        hashCtx->s[hashCtx->wordIndex] ^= hashCtx->saved;
        assert(hashCtx->byteIndex == 8);
        hashCtx->byteIndex = 0;
        hashCtx->saved = 0;
        if(++hashCtx->wordIndex ==
                (SHA3_KECCAK_SPONGE_WORDS - hashCtx->capacityWords)) {
            keccakf(hashCtx->s);
            hashCtx->wordIndex = 0;
        }
    }

    /* now work in full words directly from input */

    assert(hashCtx->byteIndex == 0);

    words = len / sizeof(uint64_t);
    tail = len - words * sizeof(uint64_t);

    for(i = 0; i < words; i++, buf += sizeof(uint64_t)) {
        const uint64_t t = (uint64_t) (buf[0]) |
                ((uint64_t) (buf[1]) << 8 * 1) |
                ((uint64_t) (buf[2]) << 8 * 2) |
                ((uint64_t) (buf[3]) << 8 * 3) |
                ((uint64_t) (buf[4]) << 8 * 4) |
                ((uint64_t) (buf[5]) << 8 * 5) |
                ((uint64_t) (buf[6]) << 8 * 6) |
                ((uint64_t) (buf[7]) << 8 * 7);
#if defined(__x86_64__ ) || defined(__i386__)
        assert(memcmp(&t, buf, 8) == 0);
#endif
        hashCtx->s[hashCtx->wordIndex] ^= t;
        if(++hashCtx->wordIndex ==
                (SHA3_KECCAK_SPONGE_WORDS - hashCtx->capacityWords)) {
            keccakf(hashCtx->s);
            hashCtx->wordIndex = 0;
        }
    }

    /* finally, save the partial word */
    assert(hashCtx->byteIndex == 0 && tail < 8);
    while (tail--) {
        hashCtx->saved |= (uint64_t) (*(buf++)) << ((hashCtx->byteIndex++) * 8);
    }
    assert(hashCtx->byteIndex < 8);
}

extern void
    keccak_digest(BRKeccak hashCtx, void* output) {
    
    BRKeccak hashCtxCpy = keccak_create256();
    memcpy(hashCtxCpy, hashCtx, sizeof(struct BRKeccakContext));
    keccak_final(hashCtxCpy, output);
    keccak_release(hashCtxCpy);
}

extern void
    keccak_final(BRKeccak hashCtx, void* output) {
    
    /* Append 2-bit suffix 01, per SHA-3 spec. Instead of 1 for padding we
     * use 1<<2 below. The 0x02 below corresponds to the suffix 01.
     * Overall, we feed 0, then 1, and finally 1 to start padding. Without
     * M || 01, we would simply use 1 to start padding. */

    hashCtx->s[hashCtx->wordIndex] ^=
            (hashCtx->saved ^ ((uint64_t) ((uint64_t) 1 << (hashCtx->byteIndex *
                                    8))));
 
    hashCtx->s[SHA3_KECCAK_SPONGE_WORDS - hashCtx->capacityWords - 1] ^=
            SHA3_CONST(0x8000000000000000UL);
    keccakf(hashCtx->s);

    /* Return first bytes of the ctx->s. This conversion is not needed for
     * little-endian platforms e.g. wrap with #if !defined(__BYTE_ORDER__)
     * || !defined(__ORDER_LITTLE_ENDIAN__) || __BYTE_ORDER__!=__ORDER_LITTLE_ENDIAN__
     *    ... the conversion below ...
     * #endif */
    {
        unsigned i;
        for(i = 0; i < SHA3_KECCAK_SPONGE_WORDS; i++) {
            const unsigned t1 = (uint32_t) hashCtx->s[i];
            const unsigned t2 = (uint32_t) ((hashCtx->s[i] >> 16) >> 16);
            hashCtx->sb[i * 8 + 0] = (uint8_t) (t1);
            hashCtx->sb[i * 8 + 1] = (uint8_t) (t1 >> 8);
            hashCtx->sb[i * 8 + 2] = (uint8_t) (t1 >> 16);
            hashCtx->sb[i * 8 + 3] = (uint8_t) (t1 >> 24);
            hashCtx->sb[i * 8 + 4] = (uint8_t) (t2);
            hashCtx->sb[i * 8 + 5] = (uint8_t) (t2 >> 8);
            hashCtx->sb[i * 8 + 6] = (uint8_t) (t2 >> 16);
            hashCtx->sb[i * 8 + 7] = (uint8_t) (t2 >> 24);
        }
    }
    memcpy(output, hashCtx->sb, hashCtx->type/8);
}

