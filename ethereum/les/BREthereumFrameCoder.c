//
//  BREthereumFrameCoder.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 4/26/18.
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

#include "aes.h"
#include "sha3.h"
#include "BRCrypto.h"
#include "BRKey.h"
#include "BREthereumFrameCoder.h"
#include "BREthereumLESBase.h"
#include "BRRlpCoder.h"

/**
 *
 * The context for a frame coder
 */
typedef struct {

    //Encryption for frame
    struct aes256_ctx frameEncrypt;
    
    //Encryption for Mac
    struct aes256_ctx macEncrypt;
    
    // Ingress ciphertext
    struct sha3_256_ctx ingressMac;
    
    // Egress ciphertext
    struct sha3_256_ctx egressMac;
    
}BREthereumFrameCoderContext;

//
// Private Functions
//
static BREthereumBoolean _initFrameCoder(BREthereumFrameCoderContext* ctx,
                     UInt512* remoteEphemeral,
                     UInt256* remoteNonce,
                     BRKey* ecdheLocal,
                     UInt256* localNonce,
                     uint8_t* aukCipher,
                     size_t aukCipherLen,
                     uint8_t* authCiper,
                     size_t authCipherLen,
                     BREthereumBoolean didOriginate)

{
    uint8_t keyMaterial[64];
    uint8_t keyMaterialBuff[64];
    size_t uint256_size = 32;
    
    // shared-secret = sha3(ecdhe-shared-secret || sha3(nonce || initiator-nonce))
    UInt256 ephemeralShared;
    
    if(etheruemECDHAgree(ecdheLocal, remoteEphemeral, &ephemeralShared)){
        return ETHEREUM_BOOLEAN_FALSE;
    }
    
    memcpy(keyMaterial, &ephemeralShared, uint256_size);

    UInt512 nonceMaterial;
    
    UInt256* lNonce = ETHEREUM_BOOLEAN_IS_TRUE(didOriginate) ? remoteNonce : localNonce;
    memcpy(nonceMaterial.u8, lNonce->u8, uint256_size);


    Uint256* rNonce = ETHEREUM_BOOLEAN_IS_TRUE(didOriginate)  ? localNonce : remoteNonce;
    memcpy(&nonceMaterial.u8[sizeof(lNonce->u8)], rNonce->u8, uint256_size);

    
    // sha3(nonce || initiator-nonce)
    BRKeccak256(&keyMaterial[sizeof(lNonce->u8)], &nonceMaterial.u8,uint256_size);
    
    
    // shared-secret = sha3(ecdhe-shared-secret || sha3(nonce || initiator-nonce))
    BRKeccak256(&keyMaterial[uint256_size], keyMaterial, uint256_size);


    // aes-secret = sha3(ecdhe-shared-secret || shared-secret)
    BRKeccak256(&keyMaterial[uint256_size], &keyMaterial[uint256_size], uint256_size);
    
    aes256_set_encrypt_key(&ctx->frameEncrypt, &keyMaterial[uint256_size]);
    aes256_set_decrypt_key(&ctx->frameEncrypt, &keyMaterial[uint256_size]);
    

    // mac-secret = sha3(ecdhe-shared-secret || aes-secret)
    BRKeccak256(&keyMaterial[uint256_size], &keyMaterial[uint256_size], uint256_size);
    aes256_set_encrypt_key(&ctx->macEncrypt, &keyMaterial[uint256_size]);


    // Initiator egress-mac: sha3(mac-secret^recipient-nonce || auth-sent-init)
    //           ingress-mac: sha3(mac-secret^initiator-nonce || auth-recvd-ack)
    // Recipient egress-mac: sha3(mac-secret^initiator-nonce || auth-sent-ack)
    //           ingress-mac: sha3(mac-secret^recipient-nonce || auth-recvd-init)
    UInt256 xORMacRemoteNonce;
    ethereumXORBytes(&keyMaterial[uint256_size], remoteNonce, xORMacRemoteNonce,uint256_size);
    memcpy(keyMaterial, xORMacRemoteNonce, uint256_size);
    uint8_t* egressCipher, ingressCipher;
    size_t egreesCipherLen, ingressCipherLen;
    
    if(didOriginate)
    {
        egressCipher = aukCipher;
        egreesCipherLen = aukCipherLen;
        ingressCipher = aukCipher;
        ingressCipherLen = aukCipherLen;
    }
    else
    {
        egressCipher = authCiper;
        egreesCipherLen = authCipherLen;
        ingressCipher = authCiper;
        ingressCipherLen = authCipherLen;
    }
    
    char* gressBytes;
    size_t egressBytesLen = uint256_size + egressCipherLen;
    
    array_new(gressBytes, egressBytesLen);
    array_add_array(gressBytes, keyMaterial, uint256_size);
    array_insert_array(gressBytes, uint256_size, egressCipher, egreesCipherLen);
    sha3_256_init(&ctx->egressMac);
    sha3_256_update(&ctx->egressMac, egressBytesLen, gressBytes);
    
    // recover mac-secret by re-xoring remoteNonce
    UInt256 xOrMacSecret;
    ethereumXORBytes(xORMacRemoteNonce.u8, localNonce->u8, xOrMacSecret,uint256_size);
    size_t ingressBytesLen = uint256_size + egressCipherLen;

    array_set_capacity(gressBytes, ingressBytesLen);
    array_insert_array(gressBytes, 0, xOrMacSecret.u8, uint256_size);
    array_insert_array(gressBytes, uint256_size, ingressCipher, ingressBytesLen);
    sha3_256_init(&ctx->ingressMac);
    sha3_256_update(&ctx->ingressMac, ingressBytesLen, gressBytes);

    array_free(gressBytes);
}
void _egressDigest(BREthereumFrameCoder fCoder, UInt128 * digest)
{

    struct sha3_256_ctx curEgressMacH;
    memcpy(&curEgressMacH, fCoder->egressMac, sizeof(struct sha3_256_ctx));
    sha3_256_digest(&curEgressMacH, sizeof(digest.u8), digest.u8);
}
void _ingressDigest(BREthereumFrameCoder fCoder, UInt128 * digest)
{

    struct sha3_256_ctx curIngressMacH;
    memcpy(&curIngressMacH, fCoder->ingressMac, sizeof(struct sha3_256_ctx));
    sha3_256_digest(&curIngressMacH, sizeof(digest.u8), digest.u8);
}
void _updateMac(BREthereumFrameCoder fCoder, struct sha3_256_ctx* mac, uint8_t* sData, size_t sDataSize) {

    //Peform check for sData size is h1238 _seed.size() && _seed.size() != h128::size)
    struct sha3_256_ctx prevDigest;
    memcpy(&prevDigest, mac, sizeof(struct sha3_256_ctx));
    UInt128 encDigest;
    
    sha3_256_digest(&prevDigest, encDigest.u8,16);
    
    UInt128 pDigest;
    
    memcpy(&pDigest.u8, &encDigest.u8, 16);
    
    aes256_encrypt(fCoder->macEncrypt, 16, encDigest.u8, encDigest.u8);

    UInt128 xOrDigest;
    
    if (sDataSize){
        ethereumXORBytes(encDigest.u8, sData, xOrDigest.u8, 128);
    }
    else{
        ethereumXORBytes(encDigest.u8, pDigest.u8, xOrDigest.u8, 128);
    }

    sha3_256_update(mac, encDigest.u8, sizeof(encDigest.u8));
    
}
void _writeFrame(BREthereumFrameCoder fCoder, BRRlpData * headerData, uint8_t* payload, size_t payloadSize, uint8_t** oBytes, size_t * oBytesSize)
{
    // TODO: SECURITY check header values && header <= 16 bytes
    size_t uint256_size = 32;
    uint8_t headerMac[uint256_size];
    memcpy(headerMac, headerData->bytes, headerData->bytesCount);
    
    aes256_encrypt(fCoder->frameEncrypt, 16, headerMac, headerMac);
    
    _updateMac(fCoder, &fCoder->egressMac, headerMac, 16);
    UInt128 egressDigest;
    
    _egressDigest(fCoder, &egressDigest);
    
    
    memcpy(&headerMac[16], egressDigest.u8, sizeof(egressDigest.u8));
    
    uint32_t padding = (16 - (payloadSize % 16)) % 16;
    

    oBytesSize = 32 + payloadSize + padding + 16;
    oBytes = (uint8_t *)calloc(oBytesSize, sizeof(uint8_t));
    
    memcpy(oBytes, headerMac, sizeof(headerMac));
    
    aes256_encrypt(fCoder->frameEncrypt, payloadSize, &oBytes[32], payload);

    bytesRef paddingRef(o_bytes.data() + 32 + _payload.size(), padding);
    
    
    if (padding) {
        aes256_encrypt(fCoder->frameEncrypt, padding, &oBytes[32 + _payload.size()], &oBytes[32 + _payload.size()]);
    }
    
    
    sha3_256_update(&ctx->egressMac, &[oBytes + 32], payloadSize + padding);
    _updateMac(fCoder, &fCoder->egressMac, NULL, 0);
    m_impl->egressMac.Update(_cipher.data(), _cipher.size());
    
    
    UInt128 egressDigestFrame;
    _egressDigest(fCoder, &egressDigestFrame);
    memcpy(&oBytes[32 + payloadSize + padding], egressDigestFrame.u8, sizeof(egressDigestFrame.u8));
}
//
// Public Functions
//
BREthereumFrameCoder ethereumFrameCoderCreate(UInt512* remoteEphemeral,
                                              UInt256* remoteNonce,
                                              BRKey* ecdheLocal,
                                              UInt256* localNonce,
                                              uint8_t* aukCipher,
                                              size_t aukCipherLen,
                                              uint8_t* authCiper,
                                              size_t authCipherLen,
                                              BREthereumBoolean didOriginate) {
    
    BREthereumPacketFramerContext * ctx = (BREthereumFrameCoderContext) calloc (1, sizeof(*ctx));
    _initFrameCoder(ctx,remoteEphemeral,remoteNonce,ecdheLocal,localNonce,aukCipher,aukCipherLen,authCipherLen,authCipherLen,didOriginate);
    return ctx;
}
void ethereumFrameCoderWrite(BREthereumFrameCoder fCoder, uint8_t msgId,  uint8_t* payload, size_t payloadSize, uint8_t** oBytes, size_t * oBytesSize) {

    BRRlpCoder coder = rlpCoderCreate();

    BRRlpItem headerItem;
    
    uint32_t frameSize =  sizeof(msgId) + payloadSize;
    
    uint8_t header[4] = {(uint8_t)((frameSize >> 16) & 0xff), ((uint8_t))((frameSize >> 8) & 0xff), (uint8_t)(frameSize & 0xff), msgId};
    
    headerItem = rlpEncodeItemBytes(coder, header, sizeof(header));
    
    BRRlpData headerData;

    rlpDataExtract(coder, headerItem, &headerData);

    _writeFrame(fCoder, &headerData, payload, payloadSize, oBytes, oBytesSize);
}



