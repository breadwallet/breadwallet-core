//
//  BREthereumFrameCoder.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 4/26/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include "support/BRArray.h"
#include "support/BRCrypto.h"
#include "support/BRKey.h"
#include "bitcoin/BRBIP38Key.h"
#include "ethereum/rlp/BRRlpCoder.h"
#include "ethereum/util/BRKeccak.h"
#include "BREthereumLESFrameCoder.h"

#define UINT256_SIZE 32
#define HEADER_LEN 16
#define MAC_LEN 16

static void
bytesXOR(uint8_t * op1, uint8_t* op2, uint8_t* result, size_t len) {
    for (unsigned int i = 0; i < len;  ++i) {
        result[i] = op1[i] ^ op2[i];
    }
}

/**
 *
 * The context for a frame coder
 * More details about the fields in the frame coder context are described here:
    https://github.com/ethereum/devp2p/blob/master/rlpx.md#encrypted-handshake
 */
struct BREthereumLESFrameCoderContext {

    //Encryption for Mac
    UInt256 macSecretKey;
    
    // Ingress ciphertext
    BRKeccak ingressMac;
    
    // Egress ciphertext
    BRKeccak egressMac;
    
    //IV for the AES-CTR
    UInt128 ivEnc, ivDec;
    
    //Encrpty Key for AES-CTR frame
    uint8_t* aesEncryptKey;
    
    //AES Cipher Total Amount
    size_t aesEncryptCipherLen;

    //Decrypty Key for AES-CTR frame
    uint8_t* aesDecryptKey;
    
    //Decrypt Cipher Total Amount
    size_t aesDecrptyCipherLen;
    
};

//
// Private Functions
//

// ecies-aes128-sha256 as specified in SEC 1, 5.1: http://www.secg.org/SEC1-Ver-1.0.pdf
// NOTE: these are not implemented using constant time algorithms
static void _BRECDH(void *out32, const BRKey *privKey, BRKey *pubKey)
{
    uint8_t p[65];
    size_t pLen = BRKeyPubKey(pubKey, p, sizeof(p));
    
    if (pLen == 65) p[0] = (p[64] % 2) ? 0x03 : 0x02; // convert to compressed pubkey format
    BRSecp256k1PointMul((BRECPoint *)p, &privKey->secret); // calculate shared secret ec-point
    memcpy(out32, &p[1], 32); // unpack the x coordinate
    mem_clean(p, sizeof(p));
}


// BIP38 is a method for encrypting private keys with a passphrase
// https://github.com/bitcoin/bips/blob/master/bip-0038.mediawiki
static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const uint8_t sboxi[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

#define xt(x) (((x) << 1) ^ ((((x) >> 7) & 1)*0x1b))

static void _BRAES256ECBEncrypt(const void *key32, void *buf16)
{
    size_t i, j;
    uint32_t key[32/4], buf[16/4];
    uint8_t *x = (uint8_t *)buf, *k = (uint8_t *)key, r = 1, a, b, c, d, e;
    
    memcpy(key, key32, sizeof(key));
    memcpy(buf, buf16, sizeof(buf));
    
    for (i = 0; i < 14; i++) {
        for (j = 0; j < 4; j++) buf[j] ^= key[j + (i & 1)*4]; // add round key
        
        for (j = 0; j < 16; j++) x[j] = sbox[x[j]]; // sub bytes
        
        // shift rows
        a = x[1], x[1] = x[5], x[5] = x[9], x[9] = x[13], x[13] = a, a = x[10], x[10] = x[2], x[2] = a;
        a = x[3], x[3] = x[15], x[15] = x[11], x[11] = x[7], x[7] = a, a = x[14], x[14] = x[6], x[6] = a;
        
        for (j = 0; i < 13 && j < 16; j += 4) { // mix columns
            a = x[j], b = x[j + 1], c = x[j + 2], d = x[j + 3], e = a ^ b ^ c ^ d;
            x[j] ^= e ^ xt(a ^ b), x[j + 1] ^= e ^ xt(b ^ c), x[j + 2] ^= e ^ xt(c ^ d), x[j + 3] ^= e ^ xt(d ^ a);
        }
        
        if ((i % 2) != 0) { // expand key
            k[0] ^= sbox[k[29]] ^ r, k[1] ^= sbox[k[30]], k[2] ^= sbox[k[31]], k[3] ^= sbox[k[28]], r = xt(r);
            for (j = 4; j < 16; j++) k[j] ^= k[j - 4];
            k[16] ^= sbox[k[12]], k[17] ^= sbox[k[13]], k[18] ^= sbox[k[14]], k[19] ^= sbox[k[15]];
            for (j = 20; j < 32; j++) k[j] ^= k[j - 4];
        }
    }
    
    var_clean(&r, &a, &b, &c, &d, &e);
    for (i = 0; i < 4; i++) buf[i] ^= key[i]; // final add round key
    mem_clean(key, sizeof(key));
    memcpy(buf16, buf, sizeof(buf));
    mem_clean(buf, sizeof(buf));
}

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-function"
static void _BRAES256ECBDecrypt(const void *key32, void *buf16)
{
    size_t i, j;
    uint32_t key[32/4], buf[16/4];
    uint8_t *x = (uint8_t *)buf, *k = (uint8_t *)key, r = 1, a, b, c, d, e, f, g, h;
    
    memcpy(key, key32, sizeof(key));
    memcpy(buf, buf16, sizeof(buf));
    
    for (i = 0; i < 7; i++) { // expand key
        k[0] ^= sbox[k[29]] ^ r, k[1] ^= sbox[k[30]], k[2] ^= sbox[k[31]], k[3] ^= sbox[k[28]], r = xt(r);
        for (j = 4; j < 16; j++) k[j] ^= k[j - 4];
        k[16] ^= sbox[k[12]], k[17] ^= sbox[k[13]], k[18] ^= sbox[k[14]], k[19] ^= sbox[k[15]];
        for (j = 20; j < 32; j++) k[j] ^= k[j - 4];
    }
    
    for (i = 0; i < 14; i++) {
        for (j = 0; j < 4; j++) buf[j] ^= key[j + (i & 1)*4]; // add round key
        
        for (j = 0; i > 0 && j < 16; j += 4) { // unmix columns
            a = x[j], b = x[j + 1], c = x[j + 2], d = x[j + 3], e = a ^ b ^ c ^ d;
            h = xt(e), f = e ^ xt(xt(h ^ a ^ c)), g = e ^ xt(xt(h ^ b ^ d));
            x[j] ^= f ^ xt(a ^ b), x[j + 1] ^= g ^ xt(b ^ c), x[j + 2] ^= f ^ xt(c ^ d), x[j + 3] ^= g ^ xt(d ^ a);
        }
        
        // unshift rows
        a = x[1], x[1] = x[13], x[13] = x[9], x[9] = x[5], x[5] = a, a = x[2], x[2] = x[10], x[10] = a;
        a = x[3], x[3] = x[7], x[7] = x[11], x[11] = x[15], x[15] = a, a = x[6], x[6] = x[14], x[14] = a;
        
        for (j = 0; j < 16; j++) x[j] = sboxi[x[j]]; // unsub bytes
        
        if ((i % 2) == 0) { // unexpand key
            for (j = 28; j > 16; j--) k[j + 3] ^= k[j - 1];
            k[16] ^= sbox[k[12]], k[17] ^= sbox[k[13]], k[18] ^= sbox[k[14]], k[19] ^= sbox[k[15]];
            for (j = 12; j > 0; j--) k[j + 3] ^= k[j - 1];
            r = (r >> 1) ^ ((r & 1)*0x8d);
            k[0] ^= sbox[k[29]] ^ r, k[1] ^= sbox[k[30]], k[2] ^= sbox[k[31]], k[3] ^= sbox[k[28]];
        }
    }
    
    var_clean(&r, &a, &b, &c, &d, &e, &f, &g, &h);
    for (i = 0; i < 4; i++) buf[i] ^= key[i]; // final add round key
    mem_clean(key, sizeof(key));
    memcpy(buf16, buf, sizeof(buf));
    mem_clean(buf, sizeof(buf));
}
#pragma clang diagnostic pop
#pragma GCC diagnostic pop
//
// Public Functions
//
BREthereumLESFrameCoder frameCoderCreate(void) {
    BREthereumLESFrameCoder coder = (BREthereumLESFrameCoder) calloc (1, sizeof(struct BREthereumLESFrameCoderContext));
    coder->aesDecryptKey = NULL;
    coder->aesEncryptKey = NULL;
    coder->egressMac = NULL;
    coder->ingressMac = NULL;
    return coder;
}

BREthereumBoolean frameCoderInit(BREthereumLESFrameCoder fcoder,
                                         BRKey* remoteEphemeral,
                                         UInt256* remoteNonce,
                                         BRKey* localEphemeral,
                                         UInt256* localNonce,
                                         uint8_t* ackCipher,
                                         size_t ackCipherLen,
                                         uint8_t* authCiper,
                                         size_t authCipherLen,
                                         BREthereumBoolean didOriginate) {
    uint8_t keyMaterial[64];
   
    /********
    * The initialzation of the coder is all described here:
    * 1. https://github.com/ethereum/devp2p/blob/master/rlpx.md#encrypted-handshake
    * 2. https://github.com/ethereum/devp2p/blob/master/rlpx.md#framing
    ******/
   
    // ephemeral-shared-secret = ecdh.agree(ephemeral-privkey, remote-ephemeral-pubk)
    UInt256 ephemeralShared;
    _BRECDH(ephemeralShared.u8, localEphemeral, remoteEphemeral);
    
    memcpy(keyMaterial, ephemeralShared.u8, 32);

    UInt512 nonceMaterial;
    
    UInt256* lNonce = ETHEREUM_BOOLEAN_IS_TRUE(didOriginate) ? remoteNonce : localNonce;
    memcpy(nonceMaterial.u8, lNonce->u8, 32);
    
    UInt256* rNonce = ETHEREUM_BOOLEAN_IS_TRUE(didOriginate)  ? localNonce : remoteNonce;
    memcpy(&nonceMaterial.u8[32], rNonce->u8, 32);

    
    // sha3(nonce || initiator-nonce)
    BRKeccak256(&keyMaterial[32], nonceMaterial.u8, 64);
    
    
    // shared-secret = sha3(ecdhe-shared-secret || sha3(nonce || initiator-nonce))
    BRKeccak256(&keyMaterial[32], keyMaterial, 64);


    // aes-secret = sha3(ecdhe-shared-secret || shared-secret)
    BRKeccak256(&keyMaterial[32], keyMaterial, 64);

    // ase-crt iv: 1
    memset(fcoder->ivEnc.u8, 0, 16);
    memset(fcoder->ivDec.u8, 0, 16);
    array_new(fcoder->aesDecryptKey, 32);
    array_new(fcoder->aesEncryptKey, 32);
    array_add_array(fcoder->aesDecryptKey, &keyMaterial[32], 32);
    array_add_array(fcoder->aesEncryptKey, &keyMaterial[32], 32);
    fcoder->aesDecrptyCipherLen = 0;
    fcoder->aesEncryptCipherLen = 0;

    // mac-secret = sha3(ecdhe-shared-secret || aes-secret)
    BRKeccak256(&keyMaterial[32], keyMaterial, 64);
    memcpy(fcoder->macSecretKey.u8,&keyMaterial[32], 32);
    
    // Initiator:
    // egress-mac = sha3.update(mac-secret ^ recipient-nonce || auth-sent-init)
    // # destroy nonce
    // ingress-mac = sha3.update(mac-secret ^ initiator-nonce || auth-recvd-ack)
    // # destroy remote-nonce

    // Recipient:
    // egress-mac = sha3.update(mac-secret ^ initiator-nonce || auth-sent-ack)
    // # destroy nonce
    // ingress-mac = sha3.update(mac-secret ^ recipient-nonce || auth-recvd-init)
    // # destroy remote-nonce
    
    UInt256 xORMacNonceEgress;
    bytesXOR(fcoder->macSecretKey.u8, remoteNonce->u8, xORMacNonceEgress.u8, 32);
    
    UInt256 xORMacNonceIngress;
    bytesXOR(fcoder->macSecretKey.u8, localNonce->u8, xORMacNonceIngress.u8, 32);

    
    uint8_t* egressCipher, *ingressCipher;
    size_t egressCipherLen, ingressCipherLen;
    uint8_t* gressBytes;
    
    
    if(ETHEREUM_BOOLEAN_IS_TRUE(didOriginate))
    {
        egressCipher = authCiper;
        egressCipherLen = authCipherLen;
        ingressCipher = ackCipher;
        ingressCipherLen = ackCipherLen;
    }
    else
    {
        egressCipher = ackCipher;
        egressCipherLen = ackCipherLen;
        ingressCipher = authCiper;
        ingressCipherLen = authCipherLen;
    }
    
    size_t egressBytesLen = 32 + egressCipherLen;
    array_new(gressBytes, egressBytesLen);
    array_add_array(gressBytes, xORMacNonceEgress.u8, 32);
    array_add_array(gressBytes, egressCipher, egressCipherLen);
    
    // egress-mac = sha3.update(mac-secret ^ initiator-nonce || auth-sent-ack)
    fcoder->egressMac = keccak_create256();
    keccak_update(fcoder->egressMac, gressBytes, egressBytesLen);
    
    size_t ingressBytesLen = 32 + ingressCipherLen;
    array_insert_array(gressBytes, 0, xORMacNonceIngress.u8, 32);
    array_insert_array(gressBytes, 32, ingressCipher, ingressCipherLen);
    
    // ingress-mac = sha3.update(mac-secret ^ recipient-nonce || auth-recvd-init)
    fcoder->ingressMac = keccak_create256();
    keccak_update(fcoder->ingressMac, gressBytes, ingressBytesLen);
    
    //Destroy secrets & nonces.
    var_clean(localNonce->u8,remoteNonce->u8, keyMaterial);
    array_free(gressBytes);
    
    return ETHEREUM_BOOLEAN_TRUE; 
}

void frameCoderRelease(BREthereumLESFrameCoder fcoder) {

    if(fcoder->aesDecryptKey != NULL){
        array_free(fcoder->aesDecryptKey);
    }
    if(fcoder->aesEncryptKey != NULL){
        array_free(fcoder->aesEncryptKey);
    }
    if(fcoder->egressMac != NULL){
        keccak_release(fcoder->egressMac);
    }
    if(fcoder->ingressMac != NULL){
        keccak_release(fcoder->ingressMac);
    }
    free(fcoder);
}

void frameCoderEncrypt(BREthereumLESFrameCoder fCoder, uint8_t* payload, size_t payloadSize, uint8_t** rlpBytes, size_t * rlpBytesSize) {

    uint8_t headerPlain[HEADER_LEN] = {(uint8_t)((payloadSize >> 16) & 0xff), (uint8_t)((payloadSize >> 8) & 0xff), (uint8_t)(payloadSize & 0xff), 0xc2, 0x80, 0x80, 0};
    
    uint8_t headerCipher[HEADER_LEN];
    fCoder->aesEncryptCipherLen += HEADER_LEN;
    BRAESCTR_OFFSET(headerCipher, HEADER_LEN, fCoder->aesEncryptKey, 32, fCoder->ivEnc.u8, headerPlain, fCoder->aesEncryptCipherLen);
    
    // Encrypt HEADER-MAC
    uint8_t egressDigest[32];
    keccak_digest(fCoder->egressMac, egressDigest);

    uint8_t macSecret[HEADER_LEN];
    memcpy(macSecret, egressDigest, HEADER_LEN);
   _BRAES256ECBEncrypt(fCoder->macSecretKey.u8, macSecret);
   
    uint8_t xORMacCipher[16];
    bytesXOR(macSecret, headerCipher, xORMacCipher, 16);

    keccak_update(fCoder->egressMac, xORMacCipher, 16);
    
    keccak_digest(fCoder->egressMac, egressDigest);

    uint8_t headerMac[16];
    memcpy(headerMac, egressDigest, 16);

    //Allocate the oBytes and oBytesSize
    size_t payloadPadding = (16 - (payloadSize % 16)) % 16;
    size_t oBytesSize = 32 + payloadSize + payloadPadding + 16; // header_cipher + headerMac + payload + padding + frameMac
    
    uint8_t * oBytes = (uint8_t*)malloc(oBytesSize);

    memcpy(oBytes, headerCipher, HEADER_LEN);
    memcpy(&oBytes[HEADER_LEN], headerMac, HEADER_LEN);
    
    uint8_t * frameCipher = &oBytes[32];
    
    // TODO: Need AES-CTRL encryption to frameCipher
    uint8_t frameData[payloadPadding + payloadSize];
    size_t frameDataSize = payloadPadding + payloadSize;
    memcpy(frameData, payload, payloadSize);
    
    if(payloadPadding){
        memset(&frameData[payloadSize], 0, payloadPadding);
    }
    
    fCoder->aesEncryptCipherLen += frameDataSize;
    BRAESCTR_OFFSET(frameCipher, frameDataSize, fCoder->aesEncryptKey, 32, fCoder->ivEnc.u8, frameData, fCoder->aesEncryptCipherLen);
    
    keccak_update(fCoder->egressMac, frameCipher, payloadSize + payloadPadding);
    
    keccak_digest(fCoder->egressMac, egressDigest);
    
    uint8_t fmac_seed[16];
    memcpy(fmac_seed, egressDigest, 16);
    memcpy(macSecret, egressDigest, 16);
    
    _BRAES256ECBEncrypt(fCoder->macSecretKey.u8, macSecret);
    bytesXOR(macSecret, fmac_seed, xORMacCipher, 16);

    keccak_update(fCoder->egressMac, xORMacCipher, 16);
    
    keccak_digest(fCoder->egressMac, egressDigest);

    memcpy(&oBytes[32 + payloadSize + payloadPadding],egressDigest, 16);
    
    *rlpBytes = oBytes;
    *rlpBytesSize = oBytesSize;
    
}

BREthereumBoolean frameCoderDecryptHeader(BREthereumLESFrameCoder fCoder, uint8_t * oBytes, size_t outSize) {

    if(outSize != HEADER_LEN + MAC_LEN) {
        return ETHEREUM_BOOLEAN_FALSE;
    }
    uint8_t* headerCipher = oBytes;
    uint8_t* headerMac = &oBytes[HEADER_LEN];
    
    uint8_t mac_secret[HEADER_LEN];
    uint8_t ingressDigest[32];
    
    //Digest the ingressMac and store it in mac_secret
    keccak_digest(fCoder->ingressMac, ingressDigest);
    memcpy(mac_secret, ingressDigest, HEADER_LEN);
    
    _BRAES256ECBEncrypt(fCoder->macSecretKey.u8, mac_secret);

    uint8_t xORMacCipher[HEADER_LEN];
    bytesXOR(mac_secret, headerCipher, xORMacCipher, HEADER_LEN);
    
    keccak_update(fCoder->ingressMac, xORMacCipher, HEADER_LEN);
    
    uint8_t headerMacExpected[HEADER_LEN];
    
    keccak_digest(fCoder->ingressMac, ingressDigest);
    memcpy(headerMacExpected, ingressDigest, HEADER_LEN);

    if(memcmp(headerMacExpected, headerMac, HEADER_LEN) != 0) {
        return ETHEREUM_BOOLEAN_FALSE;
    }
    
    fCoder->aesDecrptyCipherLen += HEADER_LEN;
    BRAESCTR_OFFSET(oBytes, HEADER_LEN, fCoder->aesEncryptKey, 32, fCoder->ivDec.u8, headerCipher, fCoder->aesDecrptyCipherLen);
    
    return ETHEREUM_BOOLEAN_TRUE;
    
}

BREthereumBoolean frameCoderDecryptFrame(BREthereumLESFrameCoder fCoder, uint8_t * oBytes, size_t outSize) {

    uint8_t* frameCipherText = oBytes;
    uint8_t* frameMac = &oBytes[outSize - MAC_LEN];

    keccak_update(fCoder->ingressMac, frameCipherText, outSize - MAC_LEN);
    
    uint8_t fmacSeed[16];
    uint8_t fmacSeedEncrypt[16];

    uint8_t ingressDigest[32];
   
    keccak_digest(fCoder->ingressMac, ingressDigest);
    memcpy(fmacSeed, ingressDigest, 16);
    memcpy(fmacSeedEncrypt, ingressDigest, 16);
   
    uint8_t xORMacCipher[16];
    _BRAES256ECBEncrypt(fCoder->macSecretKey.u8, fmacSeedEncrypt);
    bytesXOR(fmacSeedEncrypt,fmacSeed, xORMacCipher, 16);
    
    keccak_update(fCoder->ingressMac, xORMacCipher, 16);
    
    uint8_t framMacExpected[16];
    keccak_digest(fCoder->ingressMac, ingressDigest);
    memcpy(framMacExpected, ingressDigest, 16);
    
    if(memcmp(framMacExpected, frameMac, 16) != 0) {
        return ETHEREUM_BOOLEAN_FALSE;
    }

    fCoder->aesDecrptyCipherLen += (outSize - MAC_LEN);
    BRAESCTR_OFFSET(oBytes, outSize - MAC_LEN, fCoder->aesEncryptKey, 32, fCoder->ivDec.u8, frameCipherText, fCoder->aesDecrptyCipherLen);
    
    return ETHEREUM_BOOLEAN_TRUE;
}

//
// Private Test function
//
//This data comes from https://gist.github.com/fjl/3a78780d17c755d22df2
#define ECDHE_SHARED_SECRET "e3f407f83fc012470c26a93fdff534100f2c6f736439ce0ca90e9914f7d1c381"
#define AES_SECRET "c0458fa97a5230830e05f4f20b7c755c1d4e54b1ce5cf43260bb191eef4e418d"
#define MAC_SECRET  "48c938884d5067a1598272fcddaa4b833cd5e7d92e8228c0ecdfabbe68aef7f1"
#define TOKEN "3f9ec2592d1554852b1f54d228f042ed0a9310ea86d038dc2b401ba8cd7fdac4"
#define INITIAL_EGRESS_MAC  "09771e93b1a6109e97074cbe2d2b0cf3d3878efafe68f53c41bb60c0ec49097e"
#define INITIAL_INGRESS_MAC "75823d96e23136c89666ee025fb21a432be906512b3dd4a3049e898adb433847"

extern int testFrameCoderInitiator(BREthereumLESFrameCoder fCoder) {

    //TODO: THIS TEST IS BROKEN NEEDS TO BE FIXED

    //Check to ensure AES_SECRET is valid
    uint8_t aesSecret[32];
    decodeHex(aesSecret, 32, AES_SECRET, 64);
    assert(memcmp(aesSecret, fCoder->aesDecryptKey, 32) == 0);

    
    //MAC_SECRET
    uint8_t macSecret[32];
    decodeHex(macSecret, 32, MAC_SECRET, 64);
    assert(memcmp(macSecret, fCoder->macSecretKey.u8, 32) == 0);

    //TOKEN
    //uint8_t token[32];
    //decodeHex(token, 32, TOKEN, 64);
    
    //INITIAL_EGRESS_MAC
    uint8_t initialEgressMac[32];
    decodeHex(initialEgressMac, 32, INITIAL_EGRESS_MAC, 64);
    uint8_t egressDigest[32];
    
    keccak_digest(fCoder->egressMac, egressDigest);
    assert(memcmp(initialEgressMac,egressDigest, 32) == 0);
    

    //INITIAL_INGRESS_MAC
    uint8_t initialIngressMac[32];
    decodeHex(initialIngressMac, 32, INITIAL_INGRESS_MAC, 64);
    uint8_t ingressDigest[32];
    keccak_digest(fCoder->ingressMac, ingressDigest);
    
    assert(memcmp(initialIngressMac,ingressDigest, 32) == 0);

    return 0;
}
extern int testFrameCoderReceiver(BREthereumLESFrameCoder fCoder) {

    //Check to ensure AES_SECRET is valid
    uint8_t aesSecret[32];
    decodeHex(aesSecret, 32, AES_SECRET, 64);
    assert(memcmp(aesSecret, fCoder->aesDecryptKey, 32) == 0);

    
    //MAC_SECRET
    uint8_t macSecret[32];
    decodeHex(macSecret, 32, MAC_SECRET, 64);
    assert(memcmp(macSecret, fCoder->macSecretKey.u8, 32) == 0);

    //TOKEN
    //uint8_t token[32];
    //decodeHex(token, 32, TOKEN, 64);
    
    //INITIAL_EGRESS_MAC
    uint8_t initialEgressMac[32];
    decodeHex(initialEgressMac, 32, INITIAL_INGRESS_MAC, 64);
    uint8_t egressDigest[32];
    keccak_digest(fCoder->egressMac, egressDigest);
    assert(memcmp(initialEgressMac,egressDigest, 32) == 0);
    

    //INITIAL_INGRESS_MAC
    uint8_t initialIngressMac[32];
    decodeHex(initialIngressMac, 32, INITIAL_EGRESS_MAC, 64);
    uint8_t ingressDigest[32];
    keccak_digest(fCoder->ingressMac , ingressDigest);
    assert(memcmp(initialIngressMac,ingressDigest, 32) == 0);

    return 0;
}
