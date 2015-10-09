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
#include "BRAddress.h"
#include "BRBase58.h"
#include "BRTypes.h"
#include <stdio.h>

#define BITCOIN_PRIVKEY      128
#define BITCOIN_PRIVKEY_TEST 239

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
UInt256 BRSecp256k1ModAdd(UInt256 a, UInt256 b)
{
//    secp256k1_scalar_t as, bs, rs;
    UInt256 r;
//    
//    secp256k1_scalar_set_b32(&as, a.u8, NULL);
//    secp256k1_scalar_set_b32(&bs, b.u8, NULL);
//    secp256k1_scalar_add(&rs, &as, &bs);
//    secp256k1_scalar_clear(&bs);
//    secp256k1_scalar_clear(&as);
//    secp256k1_scalar_get_b32(r.u8, &rs);
//    secp256k1_scalar_clear(&rs);
    return r;
}

// multiply 256bit big endian ints (mod secp256k1 order)
UInt256 BRSecp256k1ModMul(UInt256 a, UInt256 b)
{
//    secp256k1_scalar_t as, bs, rs;
    UInt256 r;
//    
//    secp256k1_scalar_set_b32(&as, a.u8, NULL);
//    secp256k1_scalar_set_b32(&bs, b.u8, NULL);
//    secp256k1_scalar_mul(&rs, &as, &bs);
//    secp256k1_scalar_clear(&bs);
//    secp256k1_scalar_clear(&as);
//    secp256k1_scalar_get_b32(r.u8, &rs);
//    secp256k1_scalar_clear(&rs);
    return r;
}

// add secp256k1 ec-points
int BRSecp256k1PointAdd(void *r, const void *a, const void *b, int compressed)
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
int BRSecp256k1PointMul(void *r, const void *p, UInt256 i, int compressed)
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
//    secp256k1_scalar_set_b32(&is, i.u8, NULL);
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

int BRPrivKeyIsValid(const char *privKey)
{
    uint8_t data[34];
    size_t len = BRBase58CheckDecode(data, sizeof(data), privKey);
    uint8_t version = data[0];
    
    memset(data, 0, sizeof(data));
    
    if (len == 33 || len == 34) { // wallet import format: https://en.bitcoin.it/wiki/Wallet_import_format
#if BITCOIN_TESTNET
        return (version == BITCOIN_PRIVKEY_TEST);
#else
        return (version == BITCOIN_PRIVKEY);
#endif
    }
    else if ((len == 30 || len == 22) && privKey[0] == 'S') { // mini private key format
        UInt256 hash = UINT256_ZERO;
        char s[strlen(privKey) + 2];
        
        strncpy(s, privKey, sizeof(s));
        s[sizeof(s) - 2] = '?';
        BRSHA256(&hash, s, sizeof(s) - 1);
        memset(s, 0, sizeof(s));
        return (hash.u8[0] == 0);
    }
    else return (strspn(privKey, "0123456789ABCDEFabcdef")/2 == 32); // hex encoded key
}

int BRKeySetSecret(BRKey *key, const UInt256 *secret, int compressed)
{
    BRKeyClean(key);
    key->secret = *secret;
    key->compressed = compressed;
//    return secp256k1_ec_seckey_verify(_ctx, key->secret.u8);
    return 0;
}

int BRKeySetPrivKey(BRKey *key, const char *privKey)
{
    size_t len = strlen(privKey);
    
    // mini private key format
    if ((len == 30 || len == 22) && privKey[0] == 'S') {
        if (! BRPrivKeyIsValid(privKey)) return 0;
        BRSHA256(&key->secret, privKey, strlen(privKey));
        key->compressed = 0;
    }
    else {
        uint8_t data[34];
        size_t len = BRBase58CheckDecode(data, sizeof(data), privKey);
        uint8_t version = BITCOIN_PRIVKEY;

#if BITCOIN_TESTNET
        version = BITCOIN_PRIVKEY_TEST;
#endif

        if (len == 0 || len == 28) len = BRBase58Decode(data, sizeof(data), privKey);

        if (len < sizeof(UInt256) || len > sizeof(UInt256) + 2) { // treat as hex string
            for (len = 0; privKey[len*2] && privKey[len*2 + 1] && len < sizeof(data); len++) {
                if (sscanf(&privKey[len*2], "%2hhx", &data[len]) != 1) break;
            }
        }

        if ((len == sizeof(UInt256) + 1 || len == sizeof(UInt256) + 2) && data[0] == version) {
            key->secret = *(UInt256 *)&data[1];
            key->compressed = (len == sizeof(UInt256) + 2);
        }
        else if (len == sizeof(UInt256)) {
            key->secret = *(UInt256 *)data;
            key->compressed = 0;
        }

        memset(data, 0, sizeof(data));
    }

//    return secp256k1_ec_seckey_verify(_ctx, key->secret.u8);
    return 0;
}

int BRKeySetPubKey(BRKey *key, const uint8_t *pubKey, size_t len)
{
    BRKeyClean(key);
    memcpy(key->pubKey, pubKey, len);
    key->compressed = (len <= 33);
//   return secp256k1_ec_pubkey_verify(_ctx, key->pubKey, (int)sizeof(key->pubKey));
    return 0;
}

size_t BRKeyPrivKey(BRKey *key, char *privKey, size_t len)
{
    uint8_t data[34];

//    if (! secp256k1_ec_seckey_verify(_ctx, key->secret.u8)) return 0;
    data[0] = BITCOIN_PRIVKEY;
#if BITCOIN_TESTNET
    data[0] = BITCOIN_PRIVKEY_TEST;
#endif

    *(UInt256 *)&data[1] = key->secret;
    if (key->compressed) data[33] = 0x01;
    len = BRBase58CheckEncode(privKey, len, data, (key->compressed) ? 34 : 33);
    memset(data, 0, sizeof(data));
    return len;
}

size_t BRKeyPubKey(BRKey *key, void *pubKey, size_t len)
{
    static uint8_t empty[65]; // static vars initialize to zero
    size_t size = (key->compressed) ? 33 : 65;

    if (memcmp(key->pubKey, empty, size) == 0) {
//        secp256k1_ec_pubkey_create(_ctx, key->pubKey, &size, key->secret.u8, key->compressed);
    }

    if (pubKey && len >= size) memcpy(pubKey, key->pubKey, size);
    return (! pubKey || len >= size) ? size : 0;
}

UInt160 BRKeyHash160(BRKey *key)
{
    UInt160 hash = UINT160_ZERO;
    size_t len = BRKeyPubKey(key, NULL, 0);

    if (len > 0) BRHash160(&hash, key->pubKey, len);
    return hash;
}

size_t BRKeyAddress(BRKey *key, char *addr, size_t addrLen)
{
    uint8_t data[21];

    data[0] = BITCOIN_PUBKEY_ADDRESS;
#if BITCOIN_TESTNET
    data[0] = BITCOIN_PUBKEY_ADDRESS_TEST;
#endif
    *(UInt160 *)&data[1] = BRKeyHash160(key);
    if (UInt160IsZero(*(UInt160 *)&data[1])) return 0;
    return BRBase58CheckEncode(addr, addrLen, data, sizeof(data));
}

size_t BRKeySign(BRKey *key, void *sig, size_t len, UInt256 md)
{
//    secp256k1_ecdsa_sign(_ctx, md.u8, sig, &len, key->secret.u8, secp256k1_nonce_function_rfc6979, NULL);
    return len;
}

int BRKeyVerify(BRKey *key, UInt256 md, const void *sig, size_t sigLen)
{
    size_t len = BRKeyPubKey(key, NULL, 0);
    
    // success is 1, all other values are fail
//  if (len > 0) return (secp256k1_ecdsa_verify(_ctx, md.u8, sig, (int)sigLen, key->pubKey, (int)len) == 1) ? 1 : 0;
    return 0;
}

void BRKeyClean(BRKey *key)
{
    memset(key, 0, sizeof(*key));
}
