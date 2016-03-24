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
#include <stdio.h>
#include <pthread.h>

#define BITCOIN_PRIVKEY      128
#define BITCOIN_PRIVKEY_TEST 239

#define DETERMINISTIC    1
#define USE_BASIC_CONFIG 1

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "secp256k1/src/basic-config.h"
#include "secp256k1/src/secp256k1.c"
#include "secp256k1/src/modules/recovery/main_impl.h"
#pragma clang diagnostic pop
#pragma GCC diagnostic pop

static secp256k1_context *_ctx = NULL;
static pthread_once_t _ctx_once = PTHREAD_ONCE_INIT;

static void _ctx_init()
{
    _ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
}

// add 256bit big endian ints (mod secp256k1 order)
UInt256 BRSecp256k1ModAdd(UInt256 a, UInt256 b)
{
    secp256k1_scalar as, bs, rs;
    UInt256 r;
    
    secp256k1_scalar_set_b32(&as, a.u8, NULL);
    secp256k1_scalar_set_b32(&bs, b.u8, NULL);
    secp256k1_scalar_add(&rs, &as, &bs);
    secp256k1_scalar_clear(&bs);
    secp256k1_scalar_clear(&as);
    secp256k1_scalar_get_b32(r.u8, &rs);
    secp256k1_scalar_clear(&rs);
    return r;
}

// multiply 256bit big endian ints (mod secp256k1 order)
UInt256 BRSecp256k1ModMul(UInt256 a, UInt256 b)
{
    secp256k1_scalar as, bs, rs;
    UInt256 r;
    
    secp256k1_scalar_set_b32(&as, a.u8, NULL);
    secp256k1_scalar_set_b32(&bs, b.u8, NULL);
    secp256k1_scalar_mul(&rs, &as, &bs);
    secp256k1_scalar_clear(&bs);
    secp256k1_scalar_clear(&as);
    secp256k1_scalar_get_b32(r.u8, &rs);
    secp256k1_scalar_clear(&rs);
    return r;
}

// add secp256k1 ec-points
size_t BRSecp256k1PointAdd(void *r, const void *a, const void *b, int compressed)
{
    secp256k1_ge ap, bp, rp;
    secp256k1_gej aj, rj;
    size_t size = 0;
    
    if (secp256k1_eckey_pubkey_parse(&ap, a, 33) && secp256k1_eckey_pubkey_parse(&bp, b, 33)) {
        secp256k1_gej_set_ge(&aj, &ap);
        secp256k1_ge_clear(&ap);
        secp256k1_gej_add_ge(&rj, &aj, &bp);
        secp256k1_gej_clear(&aj);
        secp256k1_ge_clear(&bp);
        secp256k1_ge_set_gej(&rp, &rj);
        secp256k1_gej_clear(&rj);
        secp256k1_eckey_pubkey_serialize(&rp, r, &size, compressed);
        secp256k1_ge_clear(&rp);
    }
    
    return size;
}

// multiply ec-point by 256bit BE int
size_t BRSecp256k1PointMul(void *r, const void *p, UInt256 i, int compressed)
{
    secp256k1_scalar is;
    secp256k1_gej rj = SECP256K1_GEJ_CONST_INFINITY;
    secp256k1_ge rp, pp;
    size_t size = 0;
    
    secp256k1_scalar_set_b32(&is, i.u8, NULL);
    
    if (! p) {
        pthread_once(&_ctx_once, _ctx_init);
        secp256k1_ecmult_gen(&_ctx->ecmult_gen_ctx, &rj, &is);
    }
    else if (secp256k1_eckey_pubkey_parse(&pp, p, 33)) {
        secp256k1_ecmult_const(&rj, &pp, &is);
        secp256k1_ge_clear(&pp);
    }

    secp256k1_scalar_clear(&is);
    secp256k1_ge_set_gej(&rp, &rj);
    secp256k1_gej_clear(&rj);
    secp256k1_eckey_pubkey_serialize(&rp, r, &size, compressed);
    secp256k1_ge_clear(&rp);

    return size;
}

// returns true of privKey is a valid private key
int BRPrivKeyIsValid(const char *privKey)
{
    uint8_t data[34];
    size_t dataLen = BRBase58CheckDecode(data, sizeof(data), privKey);
    size_t strLen = strlen(privKey);
    int r = 0;
    
    if (dataLen == 33 || dataLen == 34) { // wallet import format: https://en.bitcoin.it/wiki/Wallet_import_format
#if BITCOIN_TESTNET
        r = (data[0] == BITCOIN_PRIVKEY_TEST);
#else
        r = (data[0] == BITCOIN_PRIVKEY);
#endif
    }
    else if ((strLen == 30 || strLen == 22) && privKey[0] == 'S') { // mini private key format
        UInt256 hash = UINT256_ZERO;
        char s[strLen + 2];
        
        strncpy(s, privKey, sizeof(s));
        s[sizeof(s) - 2] = '?';
        BRSHA256(&hash, s, sizeof(s) - 1);
        memset(s, 0, sizeof(s));
        r = (hash.u8[0] == 0);
    }
    else r = (strspn(privKey, "0123456789ABCDEFabcdef") == 64); // hex encoded key
    
    memset(data, 0, sizeof(data));
    return r;
}

// assigns secret to key and returns true on success
int BRKeySetSecret(BRKey *key, const UInt256 *secret, int compressed)
{
    pthread_once(&_ctx_once, _ctx_init);
    BRKeyClean(key);
    key->secret = *secret;
    key->compressed = compressed;
    return secp256k1_ec_seckey_verify(_ctx, key->secret.u8);
}

// assigns privKey to key and returns true on success
int BRKeySetPrivKey(BRKey *key, const char *privKey)
{
    size_t len = strlen(privKey);
    uint8_t data[34], version = BITCOIN_PRIVKEY;
    int r = 0;
    
#if BITCOIN_TESTNET
    version = BITCOIN_PRIVKEY_TEST;
#endif
    
    // mini private key format
    if ((len == 30 || len == 22) && privKey[0] == 'S') {
        if (! BRPrivKeyIsValid(privKey)) return 0;
        BRSHA256(data, privKey, strlen(privKey));
        r = BRKeySetSecret(key, (UInt256 *)data, 0);
    }
    else {
        len = BRBase58CheckDecode(data, sizeof(data), privKey);
        if (len == 0 || len == 28) len = BRBase58Decode(data, sizeof(data), privKey);

        if (len < sizeof(UInt256) || len > sizeof(UInt256) + 2) { // treat as hex string
            for (len = 0; privKey[len*2] && privKey[len*2 + 1] && len < sizeof(data); len++) {
                if (sscanf(&privKey[len*2], "%2hhx", &data[len]) != 1) break;
            }
        }

        if ((len == sizeof(UInt256) + 1 || len == sizeof(UInt256) + 2) && data[0] == version) {
            r = BRKeySetSecret(key, (UInt256 *)&data[1], (len == sizeof(UInt256) + 2));
        }
        else if (len == sizeof(UInt256)) {
            r = BRKeySetSecret(key, (UInt256 *)data, 0);
        }
    }

    memset(data, 0, sizeof(data));
    return r;
}

// assigns pubKey to key and returns true on success
int BRKeySetPubKey(BRKey *key, const uint8_t *pubKey, size_t len)
{
    secp256k1_pubkey pk;
    
    pthread_once(&_ctx_once, _ctx_init);
    BRKeyClean(key);
    memcpy(key->pubKey, pubKey, len);
    key->compressed = (len <= 33);
    return secp256k1_ec_pubkey_parse(_ctx, &pk, key->pubKey, len);
}

// writes the private key to privKey and returns the number of bytes writen, or len needed if privKey is NULL
size_t BRKeyPrivKey(BRKey *key, char *privKey, size_t len)
{
    uint8_t data[34];

    if (secp256k1_ec_seckey_verify(_ctx, key->secret.u8)) {
        data[0] = BITCOIN_PRIVKEY;
#if BITCOIN_TESTNET
        data[0] = BITCOIN_PRIVKEY_TEST;
#endif
        
        *(UInt256 *)&data[1] = key->secret;
        if (key->compressed) data[33] = 0x01;
        len = BRBase58CheckEncode(privKey, len, data, (key->compressed) ? 34 : 33);
        memset(data, 0, sizeof(data));
    }
    else len = 0;
    
    return len;
}

// writes the public key to pubKey and returns the number of bytes written, or len needed if pubKey is NULL
size_t BRKeyPubKey(BRKey *key, void *pubKey, size_t len)
{
    static uint8_t empty[65]; // static vars initialize to zero
    size_t size = (key->compressed) ? 33 : 65;
    secp256k1_pubkey pk;

    if (memcmp(key->pubKey, empty, size) == 0 && secp256k1_ec_pubkey_create(_ctx, &pk, key->secret.u8)) {
        secp256k1_ec_pubkey_serialize(_ctx, key->pubKey, &size, &pk,
                                      (key->compressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED));
    }

    if (pubKey && size <= len) memcpy(pubKey, key->pubKey, size);
    return (! pubKey || size <= len) ? size : 0;
}

// returns the ripemd160 hash of the sha256 hash of the public key
UInt160 BRKeyHash160(BRKey *key)
{
    UInt160 hash = UINT160_ZERO;
    size_t len = BRKeyPubKey(key, NULL, 0);

    if (len > 0) BRHash160(&hash, key->pubKey, len);
    return hash;
}

// writes the bitcoin address for key to addr and returns the number of bytes written, or len needed if addr is NULL
size_t BRKeyAddress(BRKey *key, char *addr, size_t len)
{
    uint8_t data[21];

    data[0] = BITCOIN_PUBKEY_ADDRESS;
#if BITCOIN_TESTNET
    data[0] = BITCOIN_PUBKEY_ADDRESS_TEST;
#endif
    *(UInt160 *)&data[1] = BRKeyHash160(key);

    if (! UInt160IsZero(*(UInt160 *)&data[1])) {
        len = BRBase58CheckEncode(addr, len, data, sizeof(data));
    }
    else len = 0;
    
    return len;
}

// signs md with key and writes signature to sig, returns the number of bytes written, or len needed if sig is NULL
size_t BRKeySign(BRKey *key, void *sig, size_t len, UInt256 md)
{
    secp256k1_ecdsa_signature s;
    
    if (secp256k1_ecdsa_sign(_ctx, &s, md.u8, key->secret.u8, secp256k1_nonce_function_rfc6979, NULL)) {
        if (! secp256k1_ecdsa_signature_serialize_der(_ctx, sig, &len, &s)) len = 0;
    }
    else len = 0;
    
    return len;
}

// returns true if the signature for md is verified to have been made by key
int BRKeyVerify(BRKey *key, UInt256 md, const void *sig, size_t sigLen)
{
    size_t len = BRKeyPubKey(key, NULL, 0);
    secp256k1_pubkey pk;
    secp256k1_ecdsa_signature s;
    int r = 0;
    
    if (len > 0 && secp256k1_ec_pubkey_parse(_ctx, &pk, key->pubKey, len) &&
        secp256k1_ecdsa_signature_parse_der(_ctx, &s, sig, sigLen)) {
        if (secp256k1_ecdsa_verify(_ctx, &s, md.u8, &pk) == 1) r = 1; // success is 1, all other values are fail
    }
    
    return r;
}

// wipes key material from key
void BRKeyClean(BRKey *key)
{
    memset(key, 0, sizeof(*key));
}

// Pieter Wuille's compact signature encoding used for bitcoin message signing
// to verify a compact signature, recover a public key from the signature and verify that it matches the signer's pubkey
size_t BRKeyCompactSign(BRKey *key, void *compactSig, size_t sigLen, UInt256 md)
{
    size_t r = 0;
    int recid = 0;
    secp256k1_ecdsa_recoverable_signature s;

    if (! UInt256IsZero(key->secret)) { // can't sign with a public key
        if (compactSig && sigLen >= 65 &&
            secp256k1_ecdsa_sign_recoverable(_ctx, &s, md.u8, key->secret.u8, secp256k1_nonce_function_rfc6979, NULL) &&
            secp256k1_ecdsa_recoverable_signature_serialize_compact(_ctx, (uint8_t *)compactSig + 1, &recid, &s)) {
            ((uint8_t *)compactSig)[0] = 27 + recid + (key->compressed ? 4 : 0);
            r = 65;
        }
        else if (! compactSig) r = 65;
    }
    
    return r;
}

// writes the public key recovered from compactSig to pubKey and returns number of bytes written, or pkLen needed if
// pubKey is NULL
size_t BRPubKeyRecover(uint8_t *pubKey, size_t pkLen, const void *compactSig, size_t sigLen, UInt256 md)
{
    size_t len = pkLen, r = 0;
    int compressed = 0, recid = 0;
    secp256k1_ecdsa_recoverable_signature s;
    secp256k1_pubkey pk;
    
    if (sigLen == 65) {
        if (((uint8_t *)compactSig)[0] - 27 >= 4) compressed = 1;
        recid = (((uint8_t *)compactSig)[0] - 27) % 4;
        
        if (pubKey && len >= (compressed ? 33 : 65) &&
            secp256k1_ecdsa_recoverable_signature_parse_compact(_ctx, &s, (const uint8_t *)compactSig + 1, recid) &&
            secp256k1_ecdsa_recover(_ctx, &pk, &s, md.u8) &&
            secp256k1_ec_pubkey_serialize(_ctx, pubKey, &len, &pk,
                                          (compressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED))) {
            r = len;
        }
        else if (! pubKey) r = (compressed) ? 33 : 65;
    }

    return r;
}
