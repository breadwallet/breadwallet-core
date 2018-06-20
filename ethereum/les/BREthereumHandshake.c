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
#include "../util/BRUtil.h"
#include "BRInt.h"
#include "../base/BREthereumBase.h"
#include "BRKey.h"
#include "BRCrypto.h"
#include "BREthereumHandshake.h"
#include "BREthereumNode.h"
#include "BREthereumLESBase.h"
#include "../rlp/BRRlpCoder.h"
#include "BRArray.h"
#include "BRKeyECIES.h"
#include "BREthereumP2PCoder.h"

#define SIG_SIZE_BYTES      65
#define PUBLIC_SIZE_BYTES   64
#define HEPUBLIC_BYTES      32
#define NONCE_BYTES         32
#define HANDSHAKE_LOG_TOPIC "BREthereumHandsake"

static const ssize_t authBufLen = SIG_SIZE_BYTES + HEPUBLIC_BYTES + PUBLIC_SIZE_BYTES + NONCE_BYTES + 1;
static const ssize_t authCipherBufLen =  authBufLen + 65 + 16 + 32;

static const ssize_t ackBufLen = PUBLIC_SIZE_BYTES + NONCE_BYTES + 1;
static const ssize_t ackCipherBufLen =  ackBufLen + 65 + 16 + 32;


struct BREthereumHandshakeContext {

    //A weak reference to the ethereum node
    BREthereumNode node;

    //The next state of the handshake
    BREthereumHandshakeStatus nextState;

    //The plain auth buffer
    uint8_t authBuf[authBufLen];
    
    //The cipher auth buffer
    uint8_t authBufCipher[authCipherBufLen];
    
    //The plain ack buffer
    uint8_t ackBuf[ackBufLen];
    
    //The cipher ack buffer
    uint8_t ackBufCipher[ackCipherBufLen];
        
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
void _sendAuthInitiator(BREthereumHandshake ctx){

    BREthereumNode node = ctx->node;
    
    eth_log(HANDSHAKE_LOG_TOPIC, "%s", "generating auth initiator");
    
    // authInitiator -> E(remote-pubk, S(ephemeral-privk, static-shared-secret ^ nonce) || H(ephemeral-pubk) || pubk || nonce || 0x0)
    uint8_t * authBuf = ctx->authBuf;
    uint8_t * authBufCipher = ctx->authBufCipher;

    uint8_t* signature = &authBuf[0];
    uint8_t* hPubKey = &authBuf[SIG_SIZE_BYTES];
    uint8_t* pubKey = &authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES];
    uint8_t* nonce =  &authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES + PUBLIC_SIZE_BYTES];
    BRKey* nodeKey = ethereumNodeGetKey(node);
    BRKey* remoteKey = ethereumNodeGetPeerKey(node);
    
    //static-shared-secret = ecdh.agree(privkey, remote-pubk)
    UInt256 staticSharedSecret;
    _BRECDH(staticSharedSecret.u8, nodeKey, remoteKey);
    
    //static-shared-secret ^ nonce
    UInt256 xorStaticNonce;
    UInt256* localNonce = ethereumNodeGetNonce(node);
    BRKey* localEphemeral = ethereumNodeGetEphemeral(node);
    memset(xorStaticNonce.u8, 0, 32); 
    ethereumXORBytes(staticSharedSecret.u8, localNonce->u8, xorStaticNonce.u8, sizeof(localNonce->u8));
    
    
    // S(ephemeral-privk, static-shared-secret ^ nonce)
    
    // Determine the signature length
    size_t signatureLen = 65; BRKeyCompactSignEthereum(localEphemeral,
                                                       NULL, 0,
                                                       xorStaticNonce);

    // Fill the signature
    signatureLen = BRKeyCompactSignEthereum(localEphemeral,
                                    signature, signatureLen,
                                    xorStaticNonce);
    
    // || H(ephemeral-pubk)||
    memset(&hPubKey[32], 0, 32);
    uint8_t ephPublicKey[65];
    size_t ephPubKeyLength = BRKeyPubKey(localEphemeral, ephPublicKey, 65);
    BRKeccak256(hPubKey, &ephPublicKey[1], PUBLIC_SIZE_BYTES);
    // || pubK ||
    uint8_t nodePublicKey[65] = {0}; 
    BRKeyPubKey(nodeKey, nodePublicKey, 65);
    memcpy(pubKey, &nodePublicKey[1], PUBLIC_SIZE_BYTES);
    // || nonce ||
    memcpy(nonce, localNonce->u8, sizeof(localNonce->u8));
    // || 0x0   ||
    authBuf[authBufLen - 1] = 0x0;

    // E(remote-pubk, S(ephemeral-privk, static-shared-secret ^ nonce) || H(ephemeral-pubk) || pubk || nonce || 0x0)
    BRKeyECIESAES128SHA256Encrypt(remoteKey, authBufCipher, authCipherBufLen, localEphemeral, authBuf, authBufLen);
}
void _readAuthFromInitiator(BREthereumHandshake ctx) {
    
    BREthereumNode node = ctx->node;
    BRKey* nodeKey = ethereumNodeGetKey(node);
    eth_log(HANDSHAKE_LOG_TOPIC, "%s", "received auth from initiator");
    
    size_t len = BRKeyECIESAES128SHA256Decrypt(nodeKey, ctx->authBuf, authBufLen, ctx->authBufCipher, authCipherBufLen);

    if (len != authBufLen){
        //TODO: call _readAuthFromInitiatorEIP8...
    }else {
         //copy remote nonce
        UInt256* remoteNonce = ethereumNodeGetPeerNonce(node);
        memcpy(remoteNonce->u8, &ctx->authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES + PUBLIC_SIZE_BYTES], sizeof(remoteNonce->u8));
        
        //copy remote public key
        uint8_t remotePubKey[65];
        remotePubKey[0] = 0x04;
        BRKey* remoteKey = ethereumNodeGetPeerKey(node);
        remoteKey->compressed = 0; 
        memcpy(&remotePubKey[1], &ctx->authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES], PUBLIC_SIZE_BYTES);
        BRKeySetPubKey(remoteKey, remotePubKey, 65);
        
        UInt256 sharedSecret;
        _BRECDH(sharedSecret.u8, nodeKey, remoteKey);

        UInt256 xOrSharedSecret;
        ethereumXORBytes(sharedSecret.u8, remoteNonce->u8, xOrSharedSecret.u8, sizeof(xOrSharedSecret.u8));
        
        // The ephemeral public key of the remote peer
        BRKey* remoteEphemeral = ethereumNodeGetPeerEphemeral(node);
        BRKeyRecoverPubKeyEthereum(remoteEphemeral, xOrSharedSecret, ctx->authBuf, SIG_SIZE_BYTES);
    }
}
void _sendAuthAckToInitiator(BREthereumHandshake ctx) {

    BREthereumNode node = ctx->node;
    
    eth_log(HANDSHAKE_LOG_TOPIC, "%s", "generating auth ack for initiator");

    // authRecipient -> E(remote-pubk, epubK|| nonce || 0x0)
    uint8_t* ackBuf = ctx->ackBuf;
    uint8_t* ackBufCipher = ctx->ackBufCipher;
    BRKey* remoteKey = ethereumNodeGetPeerKey(node);
    
    uint8_t* pubKey = &ackBuf[0];
    uint8_t* nonce =  &ackBuf[PUBLIC_SIZE_BYTES];
    
    // || epubK ||
    uint8_t localEphPublicKey[65];
    BRKey* localEphemeral = ethereumNodeGetEphemeral(node);
    size_t ephPubKeyLength = BRKeyPubKey(localEphemeral, localEphPublicKey, 65);
    assert(ephPubKeyLength == 65);
    memcpy(pubKey, &localEphPublicKey[1], 64);

    // || nonce ||
    UInt256* localNonce = ethereumNodeGetNonce(node);
    memcpy(nonce, localNonce->u8, sizeof(localNonce->u8));
    // || 0x0   ||
    ackBuf[ackBufLen- 1] = 0x0;

    //E(remote-pubk, epubK || nonce || 0x0)
    BRKeyECIESAES128SHA256Encrypt(remoteKey, ackBufCipher, ackCipherBufLen, localEphemeral, ackBuf, ackBufLen);
    
}
void _readAuthAckFromRecipient(BREthereumHandshake ctx) {

    BREthereumNode node = ctx->node;
    BRKey* nodeKey = ethereumNodeGetKey(node);
    
    eth_log(HANDSHAKE_LOG_TOPIC,"%s", "received auth ack from recipient");
    
    size_t len = BRKeyECIESAES128SHA256Decrypt(nodeKey, ctx->ackBuf, ackBufLen, ctx->ackBufCipher, ackCipherBufLen);

    if (len != ackBufLen){
        //TODO: call _readAckAuthFromRecipientEIP8...
        eth_log(HANDSHAKE_LOG_TOPIC,"%s", "Something went wrong with AUK");
        assert(1); 
    }
    else
    {
        //copy remote nonce key
        UInt256* nonce = ethereumNodeGetPeerNonce(node);
        memcpy(nonce->u8, &ctx->ackBuf[PUBLIC_SIZE_BYTES], sizeof(nonce->u8));
        
        //copy ephemeral public key of the remote peer
        uint8_t remoteEPubKey[65];
        remoteEPubKey[0] = 0x04;
        BRKey* remoteEphemeral = ethereumNodeGetPeerEphemeral(node);
        memcpy(&remoteEPubKey[1], ctx->ackBuf, PUBLIC_SIZE_BYTES);
        BRKeySetPubKey(remoteEphemeral, remoteEPubKey, 65);
    }
}
void _sendHelloMessage(BREthereumHandshake ctx, uint8_t** encryptedHello, size_t* encryptedHelloSize){
    
    BREthereumNode node = ctx->node;
    
    BRRlpData data = ethereumNodeRLPP2PHello(node);
    
    BREthereumFrameCoder coder = ethereumNodeGetFrameCoder(node);
    
    ethereumFrameCoderEncrypt(coder, data.bytes, data.bytesCount, encryptedHello, encryptedHelloSize);

    rlpDataRelease(data);
    
}
int _decryptMessageHelloHeader(BREthereumHandshake ctx, uint8_t* header, size_t headerSize, size_t* payloadSize) {

    // authenticate and decrypt header
    BREthereumFrameCoder coder = ethereumNodeGetFrameCoder(ctx->node);

    if(ETHEREUM_BOOLEAN_IS_FALSE(ethereumFrameCoderDecryptHeader(coder, header, headerSize)))
    {
         eth_log(HANDSHAKE_LOG_TOPIC, "%s", "Error: Decryption of hello header from peer failed.");
        return BRE_HANDSHAKE_ERROR;
    }
    
    eth_log(HANDSHAKE_LOG_TOPIC, "%s", "descrypted hello message");

    //Get frame size
    uint32_t framePayloadSize = (uint32_t)(header[2]) | ((uint32_t)(header[1]))<<8 | ((uint32_t)(header[0]))<<16;
    
    eth_log(HANDSHAKE_LOG_TOPIC, "frame size:%d", framePayloadSize);
    
    if(framePayloadSize > 1024){
        eth_log(HANDSHAKE_LOG_TOPIC, "%s", "Error: hello message is too large");
        return BRE_HANDSHAKE_ERROR;
    }
    
     *payloadSize = framePayloadSize;

    return 0;
}
int _decryptMessageHelloFrame(BREthereumHandshake ctx, uint8_t* frame, size_t frameSize, size_t payloadSize, BREthereumP2PHello* peerHello) {

    // authenticate and decrypt frame
    BREthereumFrameCoder coder = ethereumNodeGetFrameCoder(ctx->node);
    
    if(ETHEREUM_BOOLEAN_IS_FALSE(ethereumFrameCoderDecryptFrame(coder, frame, frameSize)))
    {
        eth_log(HANDSHAKE_LOG_TOPIC, "%s","Error: failed to decrypt hello frame from remote peer");
        return BRE_HANDSHAKE_ERROR;
    }
    
    BRRlpCoder rlpCoder = rlpCoderCreate();
    BRRlpData framePacketTypeData = {1, &frame[0]};
    BRRlpItem item = rlpGetItem (rlpCoder, framePacketTypeData);
    
    uint64_t packetTypeMsg = rlpDecodeItemUInt64(rlpCoder, item, 0);

    if(packetTypeMsg != 0x00){
        if(packetTypeMsg == 0x01) {
            //The remote node is choosing to disconnect from us and not complete the handshake 
            BRRlpData frameData = {payloadSize - 1, &frame[1]};
            rlpShow(frameData, "HANDSHAKE"); 
            BREthereumDisconnect reason = ethereumP2PDisconnectDecode(rlpCoder, frameData);
            eth_log(HANDSHAKE_LOG_TOPIC, "Remote Peer requested to disconnect:%s", ethereumP2PDisconnectToString(reason));
        }
        eth_log(HANDSHAKE_LOG_TOPIC, "invalid packet type. Expected: Hello Message 0x00, got:%" PRIu64, packetTypeMsg);
        rlpCoderRelease(rlpCoder);
        return BRE_HANDSHAKE_ERROR;
    }
    rlpCoderRelease(rlpCoder);
    
    rlpCoder  = rlpCoderCreate();
    BRRlpData frameData = {payloadSize - 1, &frame[1]};
    
    *peerHello = ethereumP2PHelloDecode(rlpCoder, frameData);

    rlpCoderRelease(rlpCoder);
    
    return 0;
}
BREthereumBoolean _initFrameCoder(BREthereumHandshake ctx) {

    BREthereumNode node = ctx->node;
    
    //Now we need to initilize the frameCoder with the information from the auth
    BREthereumFrameCoder coder = ethereumNodeGetFrameCoder(node);
    BRKey* remoteEphemeral = ethereumNodeGetPeerEphemeral(node);
    UInt256* remoteNonce = ethereumNodeGetPeerNonce(node);
    BRKey* localEpheremal = ethereumNodeGetEphemeral(node);
    UInt256* localNonce = ethereumNodeGetNonce(node);
    BREthereumBoolean didOriginate = ethereumNodeDidOriginate(node);
    return ethereumFrameCoderInit(coder, remoteEphemeral, remoteNonce, localEpheremal, localNonce, ctx->ackBufCipher, ackCipherBufLen, ctx->authBufCipher, authCipherBufLen, didOriginate);
}

//
// Public functions
//
BREthereumHandshake ethereumHandshakeCreate(BREthereumNode node) {

    BREthereumHandshake ctx =  (BREthereumHandshake) calloc (1, sizeof(struct BREthereumHandshakeContext));
    ctx->node = node; 
    ctx->nextState = BRE_HANDSHAKE_NEW;
    return ctx;
}
void ethereumHandshakeRelease(BREthereumHandshake handshake) {
    free(handshake);
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
            ethereumNodeWriteToPeer(node, handshake->authBufCipher, authCipherBufLen, "auth initiator");
        }
        else
        {
            int ec = ethereumNodeReadFromPeer(node, handshake->authBufCipher, authCipherBufLen, "auth from receiver");
            if (ec) {
                return BRE_HANDSHAKE_ERROR;
            }
            _readAuthFromInitiator(handshake);
        }
    }
    else if (handshake->nextState == BRE_HANDSHAKE_ACKAUTH)
    {
        handshake->nextState = BRE_HANDSHAKE_WRITEHELLO;
        if (ETHEREUM_BOOLEAN_IS_TRUE(originated))
        {
            int ec = ethereumNodeReadFromPeer(node, handshake->ackBufCipher, ackCipherBufLen, "auth ack from receivier");
            if (ec) {
                return BRE_HANDSHAKE_ERROR;
            }
            _readAuthAckFromRecipient(handshake);
        }
        else
        {
            _sendAuthAckToInitiator(handshake);
            ethereumNodeWriteToPeer(node, handshake->ackBufCipher, ackCipherBufLen, "auth ack to initiator");
        }
        //Now we need to initilize the frameCoder with the information from the auth
        _initFrameCoder(handshake);
    }
    else if (handshake->nextState == BRE_HANDSHAKE_WRITEHELLO)
    {
        handshake->nextState = BRE_HANDSHAKE_READHELLO;
        uint8_t* encryptedHello = NULL;
        size_t encryptedHelloSize = 0;
        
        _sendHelloMessage(handshake, &encryptedHello, &encryptedHelloSize);
        int ec = ethereumNodeWriteToPeer(node, encryptedHello, encryptedHelloSize, "hello message to remote peer");
        if(ec) {
            if(encryptedHello != NULL){
                free(encryptedHello);
            }
            return BRE_HANDSHAKE_ERROR;
        }
        free(encryptedHello);
    }
    else if (handshake->nextState == BRE_HANDSHAKE_READHELLO)
    {
        // Authenticate and decrypt initial hello frame with initial RLPXFrameCoder
        uint8_t header[32];
        int ec = ethereumNodeReadFromPeer(node, header, 32, "hello message header from remote peer ");
    
        if(ec){
            eth_log(HANDSHAKE_LOG_TOPIC, "%s","Error: reading in hello message from remote peer");
            return BRE_HANDSHAKE_ERROR;
        }
        size_t payloadSize = 0;
        
        ec = _decryptMessageHelloHeader(handshake, header, 32, &payloadSize);
        
        if(ec){
            return BRE_HANDSHAKE_ERROR;
        }
        
        size_t frameSize = payloadSize + ((16 - (payloadSize % 16)) % 16) + 16;
        
        uint8_t* frame;
        array_new(frame, frameSize);
        
        ec = ethereumNodeReadFromPeer(node, frame, frameSize, "reading hello message body from remote peer ");
    
        if(ec){
            eth_log(HANDSHAKE_LOG_TOPIC, "%s","Error: reading in hello message from remote peer");
            array_free(frame);
            return BRE_HANDSHAKE_ERROR;
        }
        
        BREthereumP2PHello remoteHello;
        
        ec = _decryptMessageHelloFrame(handshake, frame, frameSize,payloadSize,  &remoteHello);
        
        if(ec) {
            return BRE_HANDSHAKE_ERROR;
        }
        handshake->nextState = BRE_HANDSHAKE_FINISHED;
    }
    return handshake->nextState;
}
//
// Private Test functions
//
//This data comes from https://gist.github.com/fjl/3a78780d17c755d22df2
#define INITIATOR_PRIVATE_KEY "5e173f6ac3c669587538e7727cf19b782a4f2fda07c1eaa662c593e5e85e3051"
#define RECEIVER_PRIVATE_KEY  "c45f950382d542169ea207959ee0220ec1491755abe405cd7498d6b16adb6df8"
#define INITIATOR_EPHEMERAL_PRIVATE_KEY "19c2185f4f40634926ebed3af09070ca9e029f2edd5fae6253074896205f5f6c"
#define RECEIVER_EPHEMERAL_PRIVATE_KEY "d25688cf0ab10afa1a0e2dba7853ed5f1e5bf1c631757ed4e103b593ff3f5620"
#define AUTH_PLAINTEXT "884c36f7ae6b406637c1f61b2f57e1d2cab813d24c6559aaf843c3f48962f32f46662c066d39669b7b2e3ba14781477417600e7728399278b1b5d801a519aa570034fdb5419558137e0d44cd13d319afe5629eeccb47fd9dfe55cc6089426e46cc762dd8a0636e07a54b31169eba0c7a20a1ac1ef68596f1f283b5c676bae4064abfcce24799d09f67e392632d3ffdc12e3d6430dcb0ea19c318343ffa7aae74d4cd26fecb93657d1cd9e9eaf4f8be720b56dd1d39f190c4e1c6b7ec66f077bb1100"
#define AUTHRESP_PLAINTEST "802b052f8b066640bba94a4fc39d63815c377fced6fcb84d27f791c9921ddf3e9bf0108e298f490812847109cbd778fae393e80323fd643209841a3b7f110397f37ec61d84cea03dcc5e8385db93248584e8af4b4d1c832d8c7453c0089687a700"
#define AUTH_CIPHERTEXT "04a0274c5951e32132e7f088c9bdfdc76c9d91f0dc6078e848f8e3361193dbdc43b94351ea3d89e4ff33ddcefbc80070498824857f499656c4f79bbd97b6c51a514251d69fd1785ef8764bd1d262a883f780964cce6a14ff206daf1206aa073a2d35ce2697ebf3514225bef186631b2fd2316a4b7bcdefec8d75a1025ba2c5404a34e7795e1dd4bc01c6113ece07b0df13b69d3ba654a36e35e69ff9d482d88d2f0228e7d96fe11dccbb465a1831c7d4ad3a026924b182fc2bdfe016a6944312021da5cc459713b13b86a686cf34d6fe6615020e4acf26bf0d5b7579ba813e7723eb95b3cef9942f01a58bd61baee7c9bdd438956b426a4ffe238e61746a8c93d5e10680617c82e48d706ac4953f5e1c4c4f7d013c87d34a06626f498f34576dc017fdd3d581e83cfd26cf125b6d2bda1f1d56"
#define AUTHRESP_CIPHERTEXT "049934a7b2d7f9af8fd9db941d9da281ac9381b5740e1f64f7092f3588d4f87f5ce55191a6653e5e80c1c5dd538169aa123e70dc6ffc5af1827e546c0e958e42dad355bcc1fcb9cdf2cf47ff524d2ad98cbf275e661bf4cf00960e74b5956b799771334f426df007350b46049adb21a6e78ab1408d5e6ccde6fb5e69f0f4c92bb9c725c02f99fa72b9cdc8dd53cff089e0e73317f61cc5abf6152513cb7d833f09d2851603919bf0fbe44d79a09245c6e8338eb502083dc84b846f2fee1cc310d2cc8b1b9334728f97220bb799376233e113"

#define ECDHE_SHARED_SECRET "e3f407f83fc012470c26a93fdff534100f2c6f736439ce0ca90e9914f7d1c381"
#define INITIATOR_NONCE "cd26fecb93657d1cd9e9eaf4f8be720b56dd1d39f190c4e1c6b7ec66f077bb11"
#define RECEIVER_NONCE "f37ec61d84cea03dcc5e8385db93248584e8af4b4d1c832d8c7453c0089687a7"
#define AES_SECRET "c0458fa97a5230830e05f4f20b7c755c1d4e54b1ce5cf43260bb191eef4e418d"
#define MAC_SECRET  "48c938884d5067a1598272fcddaa4b833cd5e7d92e8228c0ecdfabbe68aef7f1"
#define TOKEN "3f9ec2592d1554852b1f54d228f042ed0a9310ea86d038dc2b401ba8cd7fdac4"
#define INITIAL_EGRESS_MAC "09771e93b1a6109e97074cbe2d2b0cf3d3878efafe68f53c41bb60c0ec49097e"
#define INITIAL_INGRESS_MAC "75823d96e23136c89666ee025fb21a432be906512b3dd4a3049e898adb433847"
#define INITIATOR_HELLO_PACKET  "6ef23fcf1cec7312df623f9ae701e63b550cdb8517fefd8dd398fc2acd1d935e6e0434a2b96769078477637347b7b01924fff9ff1c06df2f804df3b0402bbb9f87365b3c6856b45e1e2b6470986813c3816a71bff9d69dd297a5dbd935ab578f6e5d7e93e4506a44f307c332d95e8a4b102585fd8ef9fc9e3e055537a5cec2e9"
#define RECEIVER_HELLO_PACKET   "6ef23fcf1cec7312df623f9ae701e63be36a1cdd1b19179146019984f3625d4a6e0434a2b96769050577657247b7b02bc6c314470eca7e3ef650b98c83e9d7dd4830b3f718ff562349aead2530a8d28a8484604f92e5fced2c6183f304344ab0e7c301a0c05559f4c25db65e36820b4b909a226171a60ac6cb7beea09376d6d8"

extern int testInitatorHandshake(BREthereumHandshake ctx, BRKey* remoteEph) {

    _sendAuthInitiator(ctx);
    
    //Expected AuthPlain Text
    size_t authPlainTextHexSize = strlen(AUTH_PLAINTEXT);
    size_t authPlainTextSize = authPlainTextHexSize/2;
    
    uint8_t authPlainTextExpected[authPlainTextSize];
    decodeHex(authPlainTextExpected, authPlainTextSize, AUTH_PLAINTEXT, authPlainTextHexSize);
    
    //Expected AuthCiphderText
    size_t authCipherTextHexSize = strlen(AUTH_CIPHERTEXT);
    size_t authCipherSizeExepected = authCipherTextHexSize/2;
    
    uint8_t authCipherText[authCipherSizeExepected];
    decodeHex(authCipherText, authCipherSizeExepected, AUTH_CIPHERTEXT, authCipherTextHexSize);
    
    // Check that the auth message generated by the initiator is what we expect. Notice that we
    // can't use the auth_init generated here because the non-deterministic prefix would cause the
    // derived secrets to not match the expected values.
    assert(memcmp(&authPlainTextExpected[65], &ctx->authBuf[65], authBufLen - 65) == 0);
    assert(authPlainTextSize == authBufLen);
    
    //Read auth ack from the responder.
    size_t ackCipherTextHexSize = strlen(AUTHRESP_CIPHERTEXT);
    size_t ackCipherSizeExepected = ackCipherTextHexSize/2;
    
    //Verify buffer sizes are correct.
    assert(ackCipherSizeExepected == ackCipherBufLen);

    uint8_t ackCipherExpected[ackCipherSizeExepected];
    decodeHex(ackCipherExpected, ackCipherSizeExepected, AUTHRESP_CIPHERTEXT, ackCipherTextHexSize);
    memcpy(ctx->ackBufCipher, ackCipherExpected, ackCipherBufLen);
    memset(ctx->ackBuf, 0, ackBufLen);
    
    _readAuthAckFromRecipient(ctx);
    
    //Check to make sure the decrypted cipher matches with the expected plain text of the ack
    size_t ackPlainTextHexSize = strlen(AUTHRESP_PLAINTEST);
    size_t ackPlainSizeExepected = ackPlainTextHexSize/2;
    uint8_t ackPlainExpected[ackPlainSizeExepected];
    decodeHex(ackPlainExpected, ackPlainSizeExepected, AUTHRESP_PLAINTEST, ackPlainTextHexSize);
    assert(ackPlainSizeExepected == ackBufLen);
    assert(memcmp(ackPlainExpected, ctx->ackBuf, ackBufLen) == 0);

    //Check to make sure the nonce is correct
    UInt256* nonce = ethereumNodeGetPeerNonce(ctx->node);
    UInt256 recvNonce;
    decodeHex(recvNonce.u8, 32, RECEIVER_NONCE, 64);
    assert(memcmp(nonce->u8, recvNonce.u8, 32) == 0);

    //Check to make sure the remote emperal is correct
    uint8_t remotePubEmpRawExpected[65];
    uint8_t remotePubEmpRawGot[65];
    assert(BRKeyPubKey(remoteEph,  remotePubEmpRawExpected, 65) == 65);
    BRKey* remoteEmpGot = ethereumNodeGetPeerEphemeral(ctx->node);
    assert(BRKeyPubKey(remoteEmpGot,  remotePubEmpRawGot, 65) == 65);
    assert(memcmp(remotePubEmpRawGot, remotePubEmpRawExpected, 65) == 0);

    //Check to make sure the derive secrets are correct for inititaor
    //We need to first copy over the authCipherText provided since ours (ctx->authCipher) is different due to the signature. 
    memcpy(ctx->authBufCipher, authCipherText, authCipherBufLen);
    _initFrameCoder(ctx);
    BREthereumFrameCoder coder = ethereumNodeGetFrameCoder(ctx->node);
    testFrameCoderInitiator(coder);
    
    //Decode the Hello Packet
    size_t initHelloPacketHexSize = strlen(RECEIVER_HELLO_PACKET);
    size_t initHelloPacketSize = initHelloPacketHexSize/2;
    
    uint8_t initHelloPacket[initHelloPacketSize];
    decodeHex(initHelloPacket, initHelloPacketSize, RECEIVER_HELLO_PACKET, initHelloPacketHexSize);
    
    uint8_t header[32];
    size_t payloadSize;
    memcpy(header, initHelloPacket, 32);
    assert(ETHEREUM_BOOLEAN_IS_TRUE(_decryptMessageHelloHeader(ctx, header, 32, &payloadSize)));
    
    size_t  frameSize = payloadSize + ((16 - (payloadSize % 16)) % 16) + 16;
  
    uint8_t* frame;
    array_new(frame, frameSize);

    memcpy(frame, &initHelloPacket[32], frameSize);

    BREthereumP2PHello remoteP2P;
    
    assert(ETHEREUM_BOOLEAN_IS_TRUE(_decryptMessageHelloFrame(ctx, frame, frameSize, payloadSize, &remoteP2P)));

    eth_log(HANDSHAKE_LOG_TOPIC,"p2pVersion = %llu\n", remoteP2P.version);
    eth_log(HANDSHAKE_LOG_TOPIC,"clientID=%s\n", remoteP2P.clientId);
    eth_log(HANDSHAKE_LOG_TOPIC,"listenPort =%llu\n", remoteP2P.listenPort);
    for(int i = 0; i < array_count(remoteP2P.caps); ++i){
        eth_log(HANDSHAKE_LOG_TOPIC,"\tcap=%s\n", remoteP2P.caps[i].cap);
        eth_log(HANDSHAKE_LOG_TOPIC,"\tcap version=%llu\n", remoteP2P.caps[i].capVersion);
    }
    size_t targetLen = 64 * 2 + 1;
    char target[targetLen];
    encodeHex(target, targetLen, remoteP2P.nodeId.u8, 64);
    eth_log(HANDSHAKE_LOG_TOPIC,"Node id=%s", target);
 
    return 0;
}
extern int testReceiverHandshake(BREthereumHandshake ctx, BRKey* remoteKeyExpected, BRKey*remoteEph) {

    // Check that the responder correctly decodes the auth msg.
    size_t authCipherTextHexSize = strlen(AUTH_CIPHERTEXT);
    size_t authCipherSizeExepected = authCipherTextHexSize/2;
    
    uint8_t authCipherText[authCipherSizeExepected];
    decodeHex(authCipherText, authCipherSizeExepected, AUTH_CIPHERTEXT, authCipherTextHexSize);
    
    assert(authCipherSizeExepected == authCipherBufLen);
    memcpy(ctx->authBufCipher, authCipherText, authCipherBufLen);
    _readAuthFromInitiator(ctx);
    
    //Check Nonce
    UInt256* nonce = ethereumNodeGetPeerNonce(ctx->node);
    UInt256 recvNonce;
    decodeHex(recvNonce.u8, 32, INITIATOR_NONCE, 64);
    assert(memcmp(nonce->u8, recvNonce.u8, 32) == 0);
    
    //Check remote pub key is correct
    uint8_t remotePubRawExpected[65];
    uint8_t remotePubRawGot[65];
    BRKey* remoteKeyGot = ethereumNodeGetPeerKey(ctx-> node);
    assert(BRKeyPubKey(remoteKeyGot, remotePubRawGot, 65) == 65);
    assert(BRKeyPubKey(remoteKeyExpected,  remotePubRawExpected, 65) == 65);
    assert(memcmp(remotePubRawExpected, remotePubRawGot, 65) == 0);

    //Check ephemeral key
    uint8_t remotePubEmpRawExpected[65];
    uint8_t remotePubEmpRawGot[65];
    assert(BRKeyPubKey(remoteEph,  remotePubEmpRawExpected, 65) == 65);
    BRKey* remoteEmpGot = ethereumNodeGetPeerEphemeral(ctx->node);
    assert(BRKeyPubKey(remoteEmpGot,  remotePubEmpRawGot, 65) == 65);
    assert(memcmp(remotePubEmpRawGot, remotePubEmpRawExpected, 65) == 0);
 
    // Check that the auth_ack msg generated by the responder is what we expect.
    _sendAuthAckToInitiator(ctx);
    
    size_t ackRespPlainTextHexSize = strlen(AUTHRESP_PLAINTEST);
    size_t ackRespPlainSizeExepected = ackRespPlainTextHexSize/2;
    
    //Verify buffer sizes are correct.
    assert(ackRespPlainSizeExepected == ackBufLen);

    uint8_t ackRespPlainExpected[ackRespPlainSizeExepected];
    decodeHex(ackRespPlainExpected, ackRespPlainSizeExepected, AUTHRESP_PLAINTEST, ackRespPlainTextHexSize);
    
    assert(memcmp(ctx->ackBuf, ackRespPlainExpected, ackBufLen) == 0);

    //Check to make sure the derive secrets are correct for receiver
    //We need to first copy over the auckCipherText provided since ours (ctx->authCipher) is different due to the random encryption.
    size_t ackCipherTextHexSize = strlen(AUTHRESP_CIPHERTEXT);
    size_t ackCipherSizeExepected = ackCipherTextHexSize/2;
    uint8_t ackCipherExpected[ackCipherSizeExepected];
    decodeHex(ackCipherExpected, ackCipherSizeExepected, AUTHRESP_CIPHERTEXT, ackCipherTextHexSize);
    memcpy(ctx->ackBufCipher, ackCipherExpected, ackCipherBufLen);
    memcpy(remoteEmpGot, remoteEph, sizeof(BRKey));
    _initFrameCoder(ctx);
    BREthereumFrameCoder coder = ethereumNodeGetFrameCoder(ctx->node);
    testFrameCoderReceiver(coder);
    
    
    size_t initHelloPacketHexSize = strlen(INITIATOR_HELLO_PACKET);
    size_t initHelloPacketSize = initHelloPacketHexSize/2;
    
    uint8_t initHelloPacket[initHelloPacketSize];
    decodeHex(initHelloPacket, initHelloPacketSize, INITIATOR_HELLO_PACKET, initHelloPacketHexSize);
    
    uint8_t header[32];
    size_t payloadSize;
    memcpy(header, initHelloPacket, 32);
    assert(ETHEREUM_BOOLEAN_IS_TRUE(_decryptMessageHelloHeader(ctx, header, 32, &payloadSize)));
    
    size_t  frameSize = payloadSize + ((16 - (payloadSize % 16)) % 16) + 16;
    
    uint8_t* frame;
    array_new(frame, frameSize);

    memcpy(frame, &initHelloPacket[32], frameSize);

    BREthereumP2PHello remoteP2P;

    assert(ETHEREUM_BOOLEAN_IS_TRUE(_decryptMessageHelloFrame(ctx, frame, frameSize, payloadSize, &remoteP2P)));

    return 0;
}
