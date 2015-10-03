//
//  BRBIP32Sequence.c
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

#include "BRBIP32Sequence.h"
#include "BRHash.h"
#include "BRBase58.h"
#include <strings.h>

#define BIP32_HARD     0x80000000u
#define BIP32_SEED_KEY "Bitcoin seed"
#define BIP32_XPRV     "\x04\x88\xAD\xE4"
#define BIP32_XPUB     "\x04\x88\xB2\x1E"

typedef struct {
    uint8_t u8[33];
} BRPubKey;

#define PUBKEY_NONE ((BRPubKey)\
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 })

// BIP32 is a scheme for deriving chains of addresses from a seed value
// https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki

// Private parent key -> private child key
//
// CKDpriv((kpar, cpar), i) -> (ki, ci) computes a child extended private key from the parent extended private key:
//
// - Check whether i >= 2^31 (whether the child is a hardened key).
//     - If so (hardened child): let I = HMAC-SHA512(Key = cpar, Data = 0x00 || ser256(kpar) || ser32(i)).
//       (Note: The 0x00 pads the private key to make it 33 bytes long.)
//     - If not (normal child): let I = HMAC-SHA512(Key = cpar, Data = serP(point(kpar)) || ser32(i)).
// - Split I into two 32-byte sequences, IL and IR.
// - The returned child key ki is parse256(IL) + kpar (mod n).
// - The returned chain code ci is IR.
// - In case parse256(IL) >= n or ki = 0, the resulting key is invalid, and one should proceed with the next value for i
//   (Note: this has probability lower than 1 in 2^127.)
//
static void CKDpriv(UInt256 *k, UInt256 *c, uint32_t i)
{
    uint8_t buf[sizeof(BRPubKey) + sizeof(i)];
    UInt512 I;
    
    if (i & BIP32_HARD) {
        buf[0] = 0;
        *(UInt256 *)&buf[1] = *k;
    }
    else BRSecp256k1PointMul(buf, NULL, *k, 1);
    
    *(uint32_t *)&buf[sizeof(BRPubKey)] = be32(i);
    
    BRHMAC(&I, BRSHA512, sizeof(UInt512), c, sizeof(*c), buf, sizeof(buf)); // I = HMAC-SHA512(c, k|P(k) || i)
    
    *k = BRSecp256k1ModAdd(*(UInt256 *)&I, *k); // k = IL + k (mod n)
    *c = *(UInt256 *)&I.u8[sizeof(UInt256)]; // c = IR
    
    I = UINT512_ZERO;
    memset(buf, 0, sizeof(buf));
}

// Public parent key -> public child key
//
// CKDpub((Kpar, cpar), i) -> (Ki, ci) computes a child extended public key from the parent extended public key.
// It is only defined for non-hardened child keys.
//
// - Check whether i >= 2^31 (whether the child is a hardened key).
//     - If so (hardened child): return failure
//     - If not (normal child): let I = HMAC-SHA512(Key = cpar, Data = serP(Kpar) || ser32(i)).
// - Split I into two 32-byte sequences, IL and IR.
// - The returned child key Ki is point(parse256(IL)) + Kpar.
// - The returned chain code ci is IR.
// - In case parse256(IL) >= n or Ki is the point at infinity, the resulting key is invalid, and one should proceed with
//   the next value for i.
//
static void CKDpub(BRPubKey *K, UInt256 *c, uint32_t i)
{
    if (i & BIP32_HARD) return; // can't derive private child key from public parent key
    
    uint8_t buf[sizeof(*K) + sizeof(i)];
    UInt512 I;
    BRPubKey pIL;
    
    *(BRPubKey *)buf = *K;
    *(uint32_t *)&buf[sizeof(*K)] = be32(i);
    
    BRHMAC(&I, BRSHA512, sizeof(UInt512), c, sizeof(*c), buf, sizeof(buf)); // I = HMAC-SHA512(c, P(K) || i)
    
    *c = *(UInt256 *)&I.u8[sizeof(UInt256)]; // c = IR
    
    BRSecp256k1PointMul(&pIL, NULL, *(UInt256 *)&I, 1);
    BRSecp256k1PointAdd(K, &pIL, K, 1); // K = P(IL) + K
    
    pIL = PUBKEY_NONE;
    I = UINT512_ZERO;
    memset(buf, 0, sizeof(buf));
}

BRMasterPubKey BRBIP32MasterPubKey(const void *seed, size_t seedLen)
{
    BRMasterPubKey mpk;
    UInt512 I;
    UInt256 secret, chain;
    BRKey key;

    if (! seed) return MASTER_PUBKEY_NONE;
    
    BRHMAC(&I, BRSHA512, 64, BIP32_SEED_KEY, strlen(BIP32_SEED_KEY), seed, seedLen);
    secret = *(UInt256 *)&I;
    chain = *(UInt256 *)&I.u8[sizeof(UInt256)];
    I = UINT512_ZERO;
    
    BRKeySetSecret(&key, &secret, 1);
    mpk.fingerPrint = BRKeyHash160(&key).u32[0];
    
    CKDpriv(&secret, &chain, 0 | BIP32_HARD); // account 0H
    
    mpk.chainCode = chain;
    BRKeySetSecret(&key, &secret, 1);
    secret = chain = UINT256_ZERO;
    if (! BRKeyPubKey(&key, &mpk.pubKey, sizeof(mpk.pubKey))) mpk = MASTER_PUBKEY_NONE;
    BRKeyClean(&key);
    return mpk;
}

size_t BRBIP32PubKey(uint8_t *pubKey, size_t pubKeyLen, BRMasterPubKey mpk, int internal, uint32_t index)
{
    UInt256 chain = mpk.chainCode;

    if (! pubKey) return sizeof(BRPubKey);
    if (pubKeyLen < sizeof(BRPubKey)) return 0;

    *(BRPubKey *)pubKey = *(BRPubKey *)mpk.pubKey;

    CKDpub((BRPubKey *)pubKey, &chain, internal ? 1 : 0); // internal or external chain
    CKDpub((BRPubKey *)pubKey, &chain, index); // index'th key in chain
    chain = UINT256_ZERO;
    return sizeof(BRPubKey);
}

void BRBIP32PrivKey(BRKey *key, const void *seed, size_t seedlen, int internal, uint32_t index)
{
    return BRBIP32PrivKeyList(key, 1, seed, seedlen, internal, &index);
}

void BRBIP32PrivKeyList(BRKey keys[], size_t count, const void *seed, size_t seedLen, int internal,
                        const unsigned *indexes)
{
    UInt512 I;
    UInt256 secret, chain, s, c;
    
    if (! keys || count == 0 || ! seed || ! indexes) return;
    
    BRHMAC(&I, BRSHA512, 64, BIP32_SEED_KEY, strlen(BIP32_SEED_KEY), seed, seedLen);
    secret = *(UInt256 *)&I;
    chain = *(UInt256 *)&I.u8[sizeof(UInt256)];
    I = UINT512_ZERO;

    CKDpriv(&secret, &chain, 0 | BIP32_HARD); // account 0H
    CKDpriv(&secret, &chain, internal ? 1 : 0); // internal or external chain
    
    for (size_t i = 0; i < count; i++) {
        s = secret;
        c = chain;
        CKDpriv(&s, &c, indexes[i]); // index'th key in chain
        BRKeySetSecret(&keys[i], &s, 1);
    }
    
    secret = chain = c = s = UINT256_ZERO;
}

size_t BRBIP32SerializeMasterPrivKey(char *s, size_t sLen, const void *seed, size_t slen)
{
    return 0;
}

size_t BRBIP32DeserializeMasterPrivKey(void *seed, size_t seedLen, const char *s)
{
    return 0;
}

size_t BRBIP32SerializeMasterPubKey(char *s, size_t sLen, BRMasterPubKey mpk)
{
    return 0;
}

BRMasterPubKey BRBIP32DeserializeMasterPubKey(const char *s)
{
    return MASTER_PUBKEY_NONE;
}