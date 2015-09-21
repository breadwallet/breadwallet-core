//
//  BRKey.c
//
//  Created by Aaron Voisine on 8/19/15.
//  Copyright (c) 2015 breadwallet LLC
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

#include "BRKey.h"

//#define HAVE_CONFIG_H 1
//#define DETERMINISTIC 1
//
//#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wconversion"
//#pragma clang diagnostic ignored "-Wunused-function"
//#import "secp256k1/src/secp256k1.c"
//#pragma clang diagnostic pop
//
//static secp256k1_context_t *_ctx = NULL;

// add 256bit big endian ints (mod secp256k1 order)
UInt256 secp256k1_mod_add(UInt256 a, UInt256 b)
{
//    secp256k1_scalar_t as, bs, rs;
    UInt256 r;
//    
//    secp256k1_scalar_set_b32(&as, (const unsigned char *)&a, NULL);
//    secp256k1_scalar_set_b32(&bs, (const unsigned char *)&b, NULL);
//    secp256k1_scalar_add(&rs, &as, &bs);
//    secp256k1_scalar_clear(&bs);
//    secp256k1_scalar_clear(&as);
//    secp256k1_scalar_get_b32((unsigned char *)&r, &rs);
//    secp256k1_scalar_clear(&rs);
    return r;
}

// multiply 256bit big endian ints (mod secp256k1 order)
UInt256 secp256k1_mod_mul(UInt256 a, UInt256 b)
{
//    secp256k1_scalar_t as, bs, rs;
    UInt256 r;
//    
//    secp256k1_scalar_set_b32(&as, (const unsigned char *)&a, NULL);
//    secp256k1_scalar_set_b32(&bs, (const unsigned char *)&b, NULL);
//    secp256k1_scalar_mul(&rs, &as, &bs);
//    secp256k1_scalar_clear(&bs);
//    secp256k1_scalar_clear(&as);
//    secp256k1_scalar_get_b32((unsigned char *)&r, &rs);
//    secp256k1_scalar_clear(&rs);
    return r;
}

// add secp256k1 ec-points
int secp256k1_point_add(void *r, const void *a, const void *b, int compressed)
{
//    secp256k1_ge_t ap, bp, rp;
//    secp256k1_gej_t aj, rj;
    int size = 0;
//    
//    if (! secp256k1_eckey_pubkey_parse(&ap, a, 33)) return 0;
//    if (! secp256k1_eckey_pubkey_parse(&bp, b, 33)) return 0;
//    secp256k1_gej_set_ge(&aj, &ap);
//    secp256k1_ge_clear(&ap);
//    secp256k1_gej_add_ge(&rj, &aj, &bp);
//    secp256k1_gej_clear(&aj);
//    secp256k1_ge_clear(&bp);
//    secp256k1_ge_set_gej(&rp, &rj);
//    secp256k1_gej_clear(&rj);
//    secp256k1_eckey_pubkey_serialize(&rp, r, &size, compressed);
//    secp256k1_ge_clear(&rp);
    return size;
}

// multiply ec-point by 256bit big endian int
int secp256k1_point_mul(void *r, const void *p, UInt256 i, int compressed)
{
//    static dispatch_once_t onceToken = 0;
//    
//    dispatch_once(&onceToken, ^{
//        if (! _ctx) _ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
//    });
//    
//    secp256k1_scalar_t is, zs;
//    secp256k1_gej_t rj, pj;
//    secp256k1_ge_t rp, pp;
    int size = 0;
//    
//    secp256k1_scalar_set_b32(&is, (const unsigned char *)&i, NULL);
//    
//    if (p) {
//        if (! secp256k1_eckey_pubkey_parse(&pp, p, 33)) return 0;
//        secp256k1_gej_set_ge(&pj, &pp);
//        secp256k1_ge_clear(&pp);
//        secp256k1_scalar_clear(&zs);
//        secp256k1_ecmult(&_ctx->ecmult_ctx, &rj, &pj, &is, &zs);
//        secp256k1_gej_clear(&pj);
//    }
//    else secp256k1_ecmult_gen(&_ctx->ecmult_gen_ctx, &rj, &is);
//    
//    secp256k1_scalar_clear(&is);
//    secp256k1_ge_set_gej(&rp, &rj);
//    secp256k1_gej_clear(&rj);
//    secp256k1_eckey_pubkey_serialize(&rp, r, &size, compressed);
//    secp256k1_ge_clear(&rp);
    return size;
}

void BRKeySetSecret(BRKey *key, UInt256 secret, int compressed)
{
}

void BRKeySetPrivKey(BRKey *key, const char *privKey)
{
}

void BRKeySetPubKey(BRKey *key, BRPubKey pubKey)
{
}

size_t BRKeyPrivKey(BRKey *key, char *privKey, size_t len)
{
    return 0;
}

size_t BRKeyPubKey(BRKey *key, void *pubKey, size_t len)
{
    return 0;
}

UInt160 BRKeyHash160(BRKey *key)
{
    UInt160 hash;
    
    return hash;
}

size_t BRKeyAddress(BRKey *key, char *addr, size_t addrLen)
{
    return 0;
}

size_t BRKeySign(BRKey *key, void *sig, size_t len, UInt256 md)
{
    return 0;
}

int BRKeyVerify(BRKey *key, UInt256 md, const void *sig, size_t sigLen)
{
    return 0;
}
