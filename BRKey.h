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

UInt256 BRSecp256k1ModAdd(UInt256 a, UInt256 b); // add 256bit big endian ints (mod secp256k1 order)
UInt256 BRSecp256k1ModMul(UInt256 a, UInt256 b); // multiply 256bit big endian ints (mod secp256k1 order)
size_t BRSecp256k1PointAdd(void *r, const void *a, const void *b, int compressed); // add secp256k1 ec-points
size_t BRSecp256k1PointMul(void *r, const void *p, UInt256 i, int compressed); // multiply ec-point by 256bit BE int

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
int BRKeySetPubKey(BRKey *key, const uint8_t *pubKey, size_t len);

// writes the private key to privKey and returns the number of bytes writen, or len needed if privKey is NULL
size_t BRKeyPrivKey(BRKey *key, char *privKey, size_t len);

// writes the public key to pubKey and returns the number of bytes written, or len needed if pubKey is NULL
size_t BRKeyPubKey(BRKey *key, void *pubKey, size_t len);

// returns the ripemd160 hash of the sha256 hash of the public key
UInt160 BRKeyHash160(BRKey *key);

// writes the bitcoin address for key to addr and returns the number of bytes written, or len needed if addr is NULL
size_t BRKeyAddress(BRKey *key, char *addr, size_t len);

// signs md with key and writes signature to sig and returns the number of bytes written, or len needed if sig is NULL
size_t BRKeySign(BRKey *key, void *sig, size_t len, UInt256 md);

// returns true if the signature for md is verified to have been made by key
int BRKeyVerify(BRKey *key, UInt256 md, const void *sig, size_t len);

// wipes key material from key
void BRKeyClean(BRKey *key);

// Pieter Wuille's compact signature encoding used for bitcoin message signing
// to verify a compact signature, recover a public key from the signature and verify that it matches the signer's pubkey
size_t BRKeyCompactSign(BRKey *key, void *compactSig, size_t sigLen, UInt256 md);

// writes the public key recovered from compactSig to pubKey
// returns number of bytes written, or pkLen needed if pubKey is NULL
size_t BRPubKeyRecover(uint8_t *pubKey, size_t pkLen, const void *compactSig, size_t sigLen, UInt256 md);

#ifdef __cplusplus
}
#endif

#endif // BRKey_h
