//
//  BRKey.h
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

#ifndef BRKey_h
#define BRKey_h

#include "BRInt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t p[33];
} BRECPoint;
    
#define BR_ECPOINT_ZERO \
    ((BRECPoint) { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 })
    
static inline int BRECPointIsZero(BRECPoint p)
{
    return ((p.p[0] | p.p[1] | p.p[2] | p.p[3] | p.p[4] | p.p[5] | p.p[6] | p.p[7] | p.p[8] | p.p[9] | p.p[10] |
             p.p[11] | p.p[12] | p.p[13] | p.p[14] | p.p[15] | p.p[16] | p.p[17] | p.p[18] | p.p[19] | p.p[20] |
             p.p[21] | p.p[22] | p.p[23] | p.p[24] | p.p[25] | p.p[26] | p.p[27] | p.p[28] | p.p[29] | p.p[30] |
             p.p[31] | p.p[32]) == 0);
}

// adds 256bit big endian ints (mod secp256k1 order)
UInt256 BRSecp256k1ModAdd(UInt256 a, UInt256 b);

// multiplies 256bit big endian ints (mod secp256k1 order)
UInt256 BRSecp256k1ModMul(UInt256 a, UInt256 b);

// adds secp256k1 ec-points and writes the result to r
// returns number of bytes written or total rLen needed if r is NULL
size_t BRSecp256k1PointAdd(void *r, size_t rLen, BRECPoint a, BRECPoint b, int compressed);

// multiplies secp256k1 ec-point by 256bit big endian int and writes result to r
// returns number of bytes written or total rLen needed if r is NULL
size_t BRSecp256k1PointMul(void *r, size_t rLen, BRECPoint p, UInt256 i, int compressed);

// returns true if privKey is a valid private key
int BRPrivKeyIsValid(const char *privKey);

typedef struct {
    UInt256 secret;
    uint8_t pubKey[65];
    int compressed;
} BRKey;

// assigns secret to key and returns true on success
int BRKeySetSecret(BRKey *key, const UInt256 *secret, int compressed);

// assigns privKey to key and returns true on success
int BRKeySetPrivKey(BRKey *key, const char *privKey);

// assigns pubKey to key and returns true on success
int BRKeySetPubKey(BRKey *key, const uint8_t *pubKey, size_t pklen);

// writes the private key to privKey and returns the number of bytes writen, or pkLen needed if privKey is NULL
size_t BRKeyPrivKey(BRKey *key, char *privKey, size_t pklen);

// writes the public key to pubKey and returns the number of bytes written, or pkLen needed if pubKey is NULL
size_t BRKeyPubKey(BRKey *key, void *pubKey, size_t pklen);

// returns the ripemd160 hash of the sha256 hash of the public key, or UINT160_ZERO on error
UInt160 BRKeyHash160(BRKey *key);

// writes the pay-to-pubkey-hash bitcoin address for key to addr
// returns the number of bytes written, or addrLen needed if addr is NULL
size_t BRKeyAddress(BRKey *key, char *addr, size_t addrLen);

// signs md with key and writes signature to sig and returns the number of bytes written or sigLen needed if sig is NULL
size_t BRKeySign(BRKey *key, void *sig, size_t sigLen, UInt256 md);

// returns true if the signature for md is verified to have been made by key
int BRKeyVerify(BRKey *key, UInt256 md, const void *sig, size_t sigLen);

// wipes key material from key
void BRKeyClean(BRKey *key);

// Pieter Wuille's compact signature encoding used for bitcoin message signing
// to verify a compact signature, recover a public key from the signature and verify that it matches the signer's pubkey
size_t BRKeyCompactSign(BRKey *key, void *compactSig, size_t sigLen, UInt256 md);

// assigns pubKey recovered from compactSig to key and returns true on success
int BRKeyRecoverPubKey(BRKey *key, const void *compactSig, size_t sigLen, UInt256 md);

#ifdef __cplusplus
}
#endif

#endif // BRKey_h
