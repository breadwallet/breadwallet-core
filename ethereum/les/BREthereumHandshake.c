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
#include "BRInt.h"
#include "BREthereumBase.h"
#include "BRKey.h"
#include "BRCrypto.h"
#include "BREthereumHandshake.h"
#include "BREthereumNode.h"
#include "BREthereumLESBase.h"
#include "BRRlpCoder.h"
#include "BRArray.h"
#include "BRKeyECIES.h"

#define SIG_SIZE_BYTES      65
#define PUBLIC_SIZE_BYTES   64
#define HEPUBLIC_BYTES      32
#define NONCE_BYTES         32

static const ssize_t authBufLen = SIG_SIZE_BYTES + HEPUBLIC_BYTES + PUBLIC_SIZE_BYTES + NONCE_BYTES + 1;
static const ssize_t ackBufLen = PUBLIC_SIZE_BYTES + NONCE_BYTES + 1;

struct BREthereumHandshakeContext {

    //A weak reference to the ethereum node
    BREthereumNode node;

    //The next state of the handshake
    BREthereumHandshakeStatus nextState;

    //The plain auth buffer
    uint8_t authBuf[authBufLen];
    
    //The cipher auth buffer
    uint8_t authBufCipher[authBufLen];
    
    //The plain ack buffer
    uint8_t ackBuf[ackBufLen];
    
    //The cipher ack buffer
    uint8_t ackBufCipher[ackBufLen];
        
};

//
// Private functions
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
int _sendAuthInitiator(BREthereumHandshake ctx){

    BREthereumNode node = ctx->node;
    
    bre_node_log(node, "gneerating auth initiator");
    
    // authInitiator -> E(remote-pubk, S(ephemeral-privk, static-shared-secret ^ nonce) || H(ephemeral-pubk) || pubk || nonce || 0x0)
    uint8_t * authBuf = ctx->authBuf;
    uint8_t * authBufCipher = ctx->authBufCipher;

    uint8_t* signature = authBuf;
    uint8_t* hPubKey = &authBuf[SIG_SIZE_BYTES];
    uint8_t* pubKey = &authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES];
    uint8_t* nonce =  &authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES + PUBLIC_SIZE_BYTES];
    BRKey* nodeKey = ethereumNodeGetKey(node);
    BRKey* remoteKey = ethereumNodeGetRemoteKey(node);
    

    //static-shared-secret = ecdh.agree(privkey, remote-pubk)
    UInt256 staticSharedSecret;
    _BRECDH(staticSharedSecret.u8, nodeKey, remoteKey);
    
    //static-shared-secret ^ nonce
    UInt256 xorStaticNonce;
    UInt256* localNonce = ethereumNodeGetNonce(node);
    BRKey* localEphemeral = ethereumNodeGetEphemeral(node);
    ethereumXORBytes(staticSharedSecret.u8, localNonce->u8, xorStaticNonce.u8, sizeof(localNonce->u8));
    
    // S(ephemeral-privk, static-shared-secret ^ nonce)
    BRKeySign(localEphemeral, signature, SIG_SIZE_BYTES, xorStaticNonce);
    
    // || H(ephemeral-pubk)||
    memset(&hPubKey[32], 0, 32);
    uint8_t ephPublicKey[65];
    size_t ephPubKeyLength = BRKeyPubKey(localEphemeral, ephPublicKey, 0);
    BRKeccak256(hPubKey, &ephPublicKey[1], ephPubKeyLength);
    // || pubK ||
    uint8_t nodePublicKey[65];
    BRKeyPubKey(nodeKey, nodePublicKey, 0);
    memcpy(pubKey, &nodePublicKey[1], PUBLIC_SIZE_BYTES);
    // || nonce ||
    memcpy(nonce, localNonce->u8, sizeof(localNonce->u8));
    // || 0x0   ||
    authBuf[authBufLen - 1] = 0x0;

    // E(remote-pubk, S(ephemeral-privk, static-shared-secret ^ nonce) || H(ephemeral-pubk) || pubk || nonce || 0x0)
    BRKeyECIESAES128SHA256Encrypt(remoteKey, authBufCipher, authBufLen, localEphemeral, authBuf, sizeof(authBuf));
    
    return ethereumNodeWriteToPeer(node, authBufCipher, authBufLen, "sending auth initiator");
}
int _readAuthFromInitiator(BREthereumHandshake ctx) {
    
    BREthereumNode node = ctx->node;
    BRKey* nodeKey = ethereumNodeGetKey(node);
    bre_node_log(node, "receiving auth from initiator");
    
    int ec = ethereumNodeReadFromPeer(node, ctx->authBufCipher, authBufLen, "auth");
    
    if (ec) {
        return ec;
    }
    
    size_t len = BRKeyECIESAES128SHA256Decrypt(nodeKey, ctx->authBuf, authBufLen, ctx->authBufCipher, authBufLen);

    if (len != authBufLen - 1){
        //TODO: call _readAuthFromInitiatorEIP8...
    }else {
         //copy remote nonce
        UInt256* remoteNonce = ethereumNodeGetPeerNonce(node);
        memcpy(remoteNonce->u8, &ctx->authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES + PUBLIC_SIZE_BYTES], sizeof(remoteNonce->u8));
        
        //copy remote public key
        uint8_t remotePubKey[65];
        remotePubKey[0] = 0x04;
        BRKey* remoteKey = ethereumNodeGetRemoteKey(node);
        memcpy(remotePubKey, &ctx->authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES], PUBLIC_SIZE_BYTES);
        BRKeySetPubKey(remoteKey, remotePubKey, 65);
        

        UInt256 sharedSecret;
        _BRECDH(sharedSecret.u8, nodeKey, remoteKey);

        UInt256 xOrSharedSecret;
        ethereumXORBytes(sharedSecret.u8, remoteNonce->u8, xOrSharedSecret.u8, sizeof(xOrSharedSecret.u8));
        
        // The ephemeral public key of the remote peer
        BRKey* remoteEphemeral = ethereumNodeGetPeerEphemeral(node);
        BRKeyRecoverPubKey(remoteEphemeral, xOrSharedSecret, ctx->authBuf, SIG_SIZE_BYTES);
    }
    return ec;
}
void _sendAuthAckToInitiator(BREthereumHandshake ctx) {

    BREthereumNode node = ctx->node;
    
    bre_node_log(node, "generating auth ack for initiator");

    // authRecipient -> E(remote-pubk, epubK|| nonce || 0x0)
    uint8_t* ackBuf = ctx->ackBuf;
    uint8_t* ackBufCipher = ctx->ackBufCipher;
    BRKey* remoteKey = ethereumNodeGetRemoteKey(node);
    
    uint8_t* pubKey = &ackBuf[0];
    uint8_t* nonce =  &ackBuf[PUBLIC_SIZE_BYTES];
    
    // || epubK ||
    uint8_t localEphPublicKey[65];
    BRKey* localEphemeral = ethereumNodeGetEphemeral(node);
    size_t ephPubKeyLength = BRKeyPubKey(localEphemeral, localEphPublicKey, 0);
    assert(ephPubKeyLength == 65);
    memcpy(pubKey, &localEphPublicKey[1], 64);

    // || nonce ||
    UInt256* localNonce = ethereumNodeGetNonce(node);
    memcpy(nonce, localNonce->u8, sizeof(localNonce->u8));
    // || 0x0   ||
    ackBuf[ackBufLen- 1] = 0x0;

    //E(remote-pubk, remote-ephemeral-pubk || nonce || 0x0)
    BRKeyECIESAES128SHA256Encrypt(remoteKey, ackBufCipher, ackBufLen, localEphemeral, ackBuf, ackBufLen);

    ethereumNodeWriteToPeer(node, ackBufCipher, ackBufLen, "sending auth ack to initiator");
    
}
int _readAuthAckFromRecipient(BREthereumHandshake ctx) {

    BREthereumNode node = ctx->node;
    BRKey* nodeKey = ethereumNodeGetKey(node);
    
    bre_node_log(node, "receiving auth ack from recipient");
    
    int ec = ethereumNodeReadFromPeer(node, ctx->ackBufCipher, ackBufLen, "reading auth ack now");
    
    if (ec) {
        return ec;
    }
    
    size_t len = BRKeyECIESAES128SHA256Decrypt(nodeKey, ctx->ackBuf, ackBufLen, ctx->ackBufCipher, ackBufLen);

    if (len != authBufLen - 1){
        //TODO: call _readAckAuthFromRecipientEIP8...
    }
    else
    {
        //copy remote nonce key
        UInt256* nonce = ethereumNodeGetPeerNonce(node);
        memcpy(nonce->u8, &ctx->ackBuf[HEPUBLIC_BYTES], sizeof(nonce->u8));
        
        //copy ephemeral public key of the remote peer
        uint8_t remoteEPubKey[65];
        remoteEPubKey[0] = 0x04;
        BRKey* remoteEphemeral = ethereumNodeGetPeerEphemeral(node);
        memcpy(&remoteEPubKey[1], ctx->ackBuf, PUBLIC_SIZE_BYTES);
        BRKeySetPubKey(remoteEphemeral, remoteEPubKey, 65);
    }
    return ec;
}
int _sendHelloMessage(BREthereumHandshake ctx){
    
    BREthereumNode node = ctx->node;
    
    bre_node_log(node, "sending hello message with capabilities handshake");
   
    BRRlpData data = ethereumNodeGetEncodedHelloData(node);
    
    uint8_t* encryptedHello;
    size_t encryptedHelloSize;
    
    BREthereumFrameCoder coder = ethereumNodeGetFrameCoder(node);
    
    ethereumFrameCoderEncrypt(coder, data.bytes, data.bytesCount, &encryptedHello, &encryptedHelloSize);
    
    int ec = ethereumNodeWriteToPeer(node, encryptedHello, encryptedHelloSize, "sending hello message to remote peer");
    
    rlpDataRelease(data);
    free(encryptedHello);
    
    return ec;
}
int _readHelloMessage(BREthereumHandshake ctx) {

    BREthereumNode node = ctx->node;

    bre_node_log(node, "generating hello message with capabilities handshake from remote peer");

    uint8_t remoteStatus[32];
    
    int ec = ethereumNodeReadFromPeer(node, remoteStatus, 32, "reading hello message header from remote peer ");
    
    if(ec){
        bre_node_log(node, "Error: reading in hello message from remote peer");
        return BRE_HANDSHAKE_ERROR;
    }
    
    // authenticate and decrypt header
    BREthereumFrameCoder coder = ethereumNodeGetFrameCoder(node);
    
    if(ETHEREUM_BOOLEAN_IS_FALSE(ethereumFrameCoderDecryptHeader(coder, remoteStatus, 32)))
    {
        bre_node_log(node, "Error: Decryption of hello header from peer failed.");
        return BRE_HANDSHAKE_ERROR;
    }
    bre_node_log(node, "reaceived/descrypted hello message");

    //Get frame size
    uint32_t frameSize = (uint32_t)(remoteStatus[2]) | (uint32_t)(remoteStatus[1])<<8 | (uint32_t)(remoteStatus[0])<<16;
    
    if(frameSize > 1024){
        bre_node_log(node, "Error: hello message is too large");
        return BRE_HANDSHAKE_ERROR;
    }
    
    uint32_t fullFrameSize = frameSize + ((16 - (frameSize % 16)) % 16) + 16;
    uint8_t* body;
    
    array_new(body, fullFrameSize);
    
    ec = ethereumNodeReadFromPeer(node, body, fullFrameSize, "reading in hello message: frame body (packet type, packet-data)");
    
    if(ec) {
        bre_node_log(node, "Error: Reading in full body hello message from remote peer");
        return BRE_HANDSHAKE_ERROR;
    }
    
    // authenticate and decrypt frame
    if(ETHEREUM_BOOLEAN_IS_FALSE(ethereumFrameCoderDecryptFrame(coder, body, fullFrameSize)))
    {
        bre_node_log(node, "Error: failed to decrypt hello frame from remote peer");
        return BRE_HANDSHAKE_ERROR;
    }
    
    BRRlpCoder rlpCoder = rlpCoderCreate();
    BRRlpData framePacketTypeData = {1, body};
    BRRlpItem item = rlpGetItem (rlpCoder, framePacketTypeData);
    
    uint64_t packetTypeMsg = rlpDecodeItemUInt64(rlpCoder, item, 0);

    if(packetTypeMsg != 0x00){
        bre_node_log(node, "invalid packet type. Expected: Hello Message 0x80, got:%" PRIu64, packetTypeMsg);
        return BRE_HANDSHAKE_ERROR;
    }
    
    /*
    * TODO: Once Node discovery is implemented then we went to decode this this information back to th enode to add to the node table.
    * For now, we don't need to do this since we are connecting to known nodes.
    */
    
    array_free(body);
    rlpCoderRelease(rlpCoder);
    
    return 0;
}

//
// Public functions
//
BREthereumHandshake ethereumHandshakeCreate(BREthereumNode node) {

    BREthereumHandshake ctx =  (BREthereumHandshake) calloc (1, sizeof(struct BREthereumHandshakeContext));
    ctx->nextState = BRE_HANDSHAKE_NEW;
    return ctx;
}
BREthereumHandshakeStatus ethereumHandshakeTransition(BREthereumHandshake handshake){

    BREthereumNode node = handshake->node;
    BREthereumBoolean originated = ethereumNodeDidOriginate(handshake->node);

    if (handshake->nextState == BRE_HANDSHAKE_NEW)
    {
        handshake->nextState = BRE_HANDSHAKE_ACKAUTH;
        if (ETHEREUM_BOOLEAN_IS_TRUE(originated))
        {
            _sendAuthInitiator(handshake);
        }
        else
        {
            _readAuthFromInitiator(handshake);
        }
    }
    else if (handshake->nextState == BRE_HANDSHAKE_ACKAUTH)
    {
        handshake->nextState = BRE_HANDSHAKE_WRITESTATUS;
        if (ETHEREUM_BOOLEAN_IS_TRUE(originated))
        {
            _readAuthAckFromRecipient(handshake);
        }
        else
        {
            _sendAuthAckToInitiator(handshake);
        }
        //Now we need to initilize the frameCoder with the information from the auth
        BREthereumFrameCoder coder = ethereumNodeGetFrameCoder(node);
        BRKey* remoteEphemeral = ethereumNodeGetPeerEphemeral(node);
        UInt256* remoteNonce = ethereumNodeGetPeerNonce(node);
        BRKey* localEpheremal = ethereumNodeGetEphemeral(node);
        UInt256* localNonce = ethereumNodeGetNonce(node);
        BREthereumBoolean didOriginate = ethereumNodeDidOriginate(node);
        ethereumFrameCoderInit(coder, remoteEphemeral, remoteNonce, localEpheremal, localNonce, handshake->ackBufCipher, ackBufLen, handshake->ackBufCipher, authBufLen, didOriginate);
    }
    else if (handshake->nextState == BRE_HANDSHAKE_WRITESTATUS)
    {
        handshake->nextState = BRE_HANDSHAKE_READSTATUS;
        _sendHelloMessage(handshake);
    }
    else if (handshake->nextState == BRE_HANDSHAKE_READSTATUS)
    {
        // Authenticate and decrypt initial hello frame with initial RLPXFrameCoder
        _readHelloMessage (handshake);
        handshake->nextState = BRE_HANDSHAKE_FINISHED;
    }
    return handshake->nextState;
}
void ethereumHandshakeRelease(BREthereumHandshake handshake) {
    free(handshake);
}
