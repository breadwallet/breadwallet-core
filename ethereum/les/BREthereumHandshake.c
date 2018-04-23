//
//  BREthereumNode.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 4/18/18.
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

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "secp256k1.h"
#include "secp256k1_ecdh.h"
#include "BRInt.h"
#include "BREthereumBase.h"
#include "BRKey.h"
#include "BRCrypto.h"
#include "BREthereumHandshake.h"
#include "BREthereumNode.h"

#ifndef MSG_NOSIGNAL   // linux based systems have a MSG_NOSIGNAL send flag, useful for supressing SIGPIPE signals
#define MSG_NOSIGNAL 0 // set to 0 if undefined (BSD has the SO_NOSIGPIPE sockopt, and windows has no signals at all)
#endif

#define SIG_SIZE_BYTES      65
#define PUBLIC_SIZE_BYTES   64
#define HEPUBLIC_BYTES      64
#define NONCE_BYTES         64

static const ssize_t authBufLen = SIG_SIZE_BYTES + HEPUBLIC_BYTES + PUBLIC_SIZE_BYTES + NONCE_BYTES + 1;
static const ssize_t ackBufLen = PUBLIC_SIZE_BYTES + NONCE_BYTES + 1;

typedef struct {

    //A weak reference to the remote peer information
    BREthereumPeer* peer;
    
    //The next state of the handshake
    BREthereumHandshakeStatus nextState;
    
    //A weak reference to the BREthereumNode's keypair
    BRKey* key;
    
    //A nonce for the handshake
    UInt256 nonce;
    
   // Local Ephemeral ECDH key pair
    BRKey ecdhe;
    
    //The plain auth buffer
    uint8_t authBuf[authBufLen];
    
    //The cipher auth buffer
    uint8_t authBufCipher[authBufLen];
    
    //The plain ack buffer
    uint8_t ackBuf[ackBufLen];
    
    //The cipher ack buffer
    uint8_t ackBufCipher[ackBufLen];
    
    //Represents whether remote peer or node initiated the handshake
    BREthereumBoolean didOriginate;
    
}BREthereumHandshakeContext;


/*** Private functions **/
BREthereumBoolean _ecdhAgree(BRKey* key, UInt512* pubKey, UInt256* outSecret)
{
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    secp256k1_pubkey rawPubKey;
    unsigned char compressedPoint[33];
    unsigned char serialPubKey[65];
    serialPubKey[0] = 0x04;
    memcpy(&serialPubKey[1], pubKey->u8, sizeof(pubKey->u8));
    if (!secp256k1_ec_pubkey_parse(ctx, &rawPubKey, serialPubKey, sizeof(serialPubKey)))
        return ETHEREUM_BOOLEAN_FALSE;  // Invalid public key.

    if (!secp256k1_ecdh(ctx, compressedPoint, &rawPubKey, (unsigned char*)key->secret.u8))
        return ETHEREUM_BOOLEAN_FALSE;  // Invalid secret key.
    
    memcpy(outSecret->u8, &compressedPoint[1], sizeof(outSecret->u8));
    secp256k1_context_destroy(ctx);
    return ETHEREUM_BOOLEAN_TRUE;
}
void _encryptECIES(UInt512* pubKey, uint8_t * plain, uint8_t * cipher, ssize_t len) {

    //TODO: Implement encrypt ECIES
    memcpy(cipher, plain, len);
}
BREthereumBoolean _decryptECIES(UInt256* priKey, uint8_t * plain, uint8_t * cipher)
{
    //TODO: Implement decrypt ECIES
    return ETHEREUM_BOOLEAN_FALSE;
}
BREthereumBoolean decryptECIES() {
    return ETHEREUM_BOOLEAN_TRUE;
}
void _xorBRInt256(UInt256 * opr1, UInt256 * opr2, UInt256 * result)
{
    for (unsigned int i = 0; i < sizeof(opr1->u8); ++i) {
        result->u8[i] = opr1->u8[i] ^ opr2->u8[i];
    }
}

int _readBuffer(BREthereumHandshakeContext* handshakeCtx, uint8_t * buf, size_t bufSize, const char * type){

    BREthereumPeer * peerCtx = handshakeCtx->peer;
    ssize_t n = 0, len = 0;
    int socket, error = 0;

    bre_peer_log(peerCtx, "handshake reading: %s", type);

    socket = peerCtx->socket;

    if (socket < 0) error = ENOTCONN;

    while (socket >= 0 && ! error && len < bufSize) {
        n = read(socket, &buf[len], bufSize - len);
        if (n > 0) len += n;
        if (n == 0) error = ECONNRESET;
        if (n < 0 && errno != EWOULDBLOCK) error = errno;
        
        socket = peerCtx->socket;
    }
    
    if (error) {
        bre_peer_log(peerCtx, "%s", strerror(error));
    }
    return error;
}
int _sendBuffer(BREthereumHandshakeContext* handshakeCtx, uint8_t * buf, size_t bufSize, char* type){

    BREthereumPeer * peerCtx = handshakeCtx->peer;
    ssize_t n = 0;
    int socket, error = 0;

    bre_peer_log(peerCtx, "handshake sending: %s", type);

    size_t offset = 0;
    socket = peerCtx->socket;

    if (socket < 0) error = ENOTCONN;

    while (socket >= 0 && !error && offset <  bufSize) {
        n = send(socket, &buf[offset], bufSize - offset, MSG_NOSIGNAL);
        if (n >= 0) offset += n;
        if (n < 0 && errno != EWOULDBLOCK) error = errno;
        socket = peerCtx->socket;
    }

    if (error) {
        bre_peer_log(peerCtx, "%s", strerror(error));
    }
    return error;
}
int _writeAuth(BREthereumHandshakeContext * ctx){

    BREthereumPeer* peer = ctx->peer;
    
    bre_peer_log(peer, "sending auth");

    // authInitiator -> E(remote-pubk, S(ecdhe-random, ecdh-shared-secret^nonce) || H(ecdhe-random-pubk) || pubk || nonce || 0x0)
    uint8_t * authBuf = ctx->authBuf;
    uint8_t * authBufCipher = ctx->authBufCipher;

    uint8_t* signature = &authBuf[0];
    uint8_t* hPubKey = &authBuf[SIG_SIZE_BYTES];
    uint8_t* pubKey = &authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES];
    uint8_t* nonce =  &authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES + NONCE_BYTES];
    
    //ephemeral-shared-secret = ecdh.agree(ephemeral-privkey, remote-ephemeral-pubk)
    UInt256 ephemeralSharedSecret;
    _ecdhAgree(ctx->key, &ctx->peer->remoteId, &ephemeralSharedSecret);
    
    //ecdh-shared-secret^nonce
    UInt256 xorStaticNonce;
    _xorBRInt256(&ephemeralSharedSecret, &ctx->nonce, &xorStaticNonce);
    
    // S(ecdhe-random, ecdh-shared-secret^nonce)
    BRKeySign(&ctx->ecdhe, signature, SIG_SIZE_BYTES, xorStaticNonce);
    // || H(ecdhe-random-pubk) ||
    BRKeccak256(hPubKey, ctx->ecdhe.pubKey, 32);
    memset(&hPubKey[32], 0, 32);
    // || pubK ||
    memcpy(pubKey, ctx->key->pubKey, sizeof(ctx->key->pubKey));
    // || nonce ||
    memcpy(nonce, ctx->nonce.u8, sizeof(ctx->nonce.u8));
    // || 0x0   ||
    authBuf[authBufLen - 1] = 0x0;

    //E(remote-pubk, S(ecdhe-random, ecdh-shared-secret^nonce) || H(ecdhe-random-pubk) || pubk || nonce || 0x0)
    _encryptECIES(&ctx->peer->remoteId, authBuf, authBufCipher, authBufLen);
    
    return _sendBuffer(ctx, authBufCipher, authBufLen, "writeAuth");

}
void _writeAck(BREthereumHandshakeContext * ctx) {

    BREthereumPeer* peer = ctx->peer;
    
    bre_peer_log(peer, "sending ack");

    // ack -> E( epubk || nonce || 0x0)
    uint8_t* ackBuf = ctx->ackBuf;
    uint8_t* ackBufCipher = ctx->ackBufCipher;

    uint8_t* pubKey = &ackBuf[0];
    uint8_t* nonce =  &ackBuf[PUBLIC_SIZE_BYTES];
    
    // || epubK ||
    memcpy(pubKey, ctx->ecdhe.pubKey, sizeof(ctx->ecdhe.pubKey));
    // || nonce ||
    memcpy(nonce, ctx->nonce.u8, sizeof(ctx->nonce.u8));
    // || 0x0   ||
    ackBuf[ackBufLen- 1] = 0x0;

    //E( epubk || nonce || 0x0)
    _encryptECIES(&ctx->peer->remoteId, ackBuf, ackBufCipher, ackBufLen);
    
    _sendBuffer(ctx, ackBufCipher, ackBufLen, "writeAuck");
}
int _readAuth(BREthereumHandshakeContext * ctx) {

    BREthereumPeer* peer = ctx->peer;
    
    bre_peer_log(peer, "receiving auth");
    
    int ec = _readBuffer(ctx, ctx->authBufCipher, authBufLen, "auth");
    
    if (ec) {
        return ec;
    }
    else if (_decryptECIES(&ctx->key->secret, ctx->ackBufCipher, ctx->authBuf))
    {
        // TODO: Implement the reading of the Auth. For now we assume we connect to our own Ethereum node first
    }
    return ec;
}
int _readAck(BREthereumHandshakeContext * ctx) {

    BREthereumPeer* peer = ctx->peer;
    
    bre_peer_log(peer, "receiving ack");
    
    int ec = _readBuffer(ctx, ctx->ackBufCipher, ackBufLen, "ack");
    
    if (ec) {
        return ec;
    }
    else if (_decryptECIES(&ctx->key->secret, ctx->ackBufCipher, ctx->authBuf))
    {
        // TODO: Implement the reading of the Ack. For now we assume we connect to our own Ethereum node first
    }
    return ec;
}
int _writeLESStatus(BREthereumHandshakeContext * ctx){
    
    BREthereumPeer* peer = ctx->peer;
    
    bre_peer_log(peer, "sending status message with capabilities handshake");
    
    return 0;
}
int _readLESStatus(BREthereumHandshakeContext * ctx) {

    BREthereumPeer* peer = ctx->peer;

    bre_peer_log(peer, "sending status message with capabilities handshake");
    
    //TODO: Make sure to authenticate the header with original status message

    return 0;
}
/*** Public functions ***/
BREthereumHandShake ethereumHandshakeCreate(BREthereumPeer * peer, BRKey* nodeKey, BREthereumBoolean didOriginate) {

    BREthereumHandshakeContext * ctx =  ( BREthereumHandshakeContext *) calloc (1, sizeof(*ctx));
    ctx->peer = peer;
    ctx->nextState = BRE_HANDSHAKE_NEW;
    ctx->key = nodeKey;
    ctx->didOriginate = didOriginate;
    //ctx->nonce = ((uint64_t)BRRand(0) << 32) | (uint64_t)BRRand(0); // random nonce
    return (BREthereumHandShake)ctx;
}
BREthereumHandshakeStatus ethereumHandshakeTransition(BREthereumHandShake handshake){

    BREthereumHandshakeContext* ctx = (BREthereumHandshakeContext *)handshake;
    
    if (ctx->nextState == BRE_HANDSHAKE_NEW)
    {
        ctx->nextState = BRE_HANDSHAKE_ACKAUTH;
        if (ETHEREUM_BOOLEAN_IS_TRUE(ctx->didOriginate))
        {
            _writeAuth(ctx);
        }
        else
        {
            _readAuth(ctx);
        }
    }
    else if (ctx->nextState == BRE_HANDSHAKE_ACKAUTH)
    {
        ctx->nextState = BRE_HANDSHAKE_WRITESTATUS;
        if (ETHEREUM_BOOLEAN_IS_TRUE(ctx->didOriginate))
        {
            _readAck(ctx);
        }
        else
        {
            _writeAck(ctx);
        }
    }
    else if (ctx->nextState == BRE_HANDSHAKE_WRITESTATUS)
    {
        ctx->nextState = BRE_HANDSHAKE_READSTATUS;
       _writeLESStatus(ctx);
       
    }
    else if (ctx->nextState == BRE_HANDSHAKE_READSTATUS)
    {
        // Authenticate and decrypt initial hello frame with initial RLPXFrameCoder
        // and request m_host to start session.
        ctx->nextState = BRE_HANDSHAKE_FINISHED;
        _readLESStatus (ctx);
    }
    return ctx->nextState;
}
void ethereumHandshakeFree(BREthereumHandShake handshake) {

    BREthereumHandshakeContext * cxt =  (BREthereumHandshakeContext *) handshake;
    free(cxt);
}
