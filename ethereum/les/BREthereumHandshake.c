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
    
    //The header information to exchange with the peer.
    BREthereumLESHeader header;
    
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
    
    etheruemECDHAgree(ctx->key, &ctx->peer->remoteId, &ephemeralSharedSecret);
    
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
    ethereumEncryptECIES(&ctx->peer->remoteId, authBuf, authBufCipher, authBufLen);
    
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
    ethereumEncryptECIES(&ctx->peer->remoteId, ackBuf, ackBufCipher, ackBufLen);
    
    _sendBuffer(ctx, ackBufCipher, ackBufLen, "writeAuck");
}
int _readAuth(BREthereumHandshakeContext * ctx) {

    BREthereumPeer* peer = ctx->peer;
    
    bre_peer_log(peer, "receiving auth");
    
    int ec = _readBuffer(ctx, ctx->authBufCipher, authBufLen, "auth");
    
    if (ec) {
        return ec;
    }
    else if (ethereumDecryptECIES(&ctx->key->secret, ctx->ackBufCipher, ctx->authBuf, authBufLen))
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
    else if (ethereumDecryptECIES(&ctx->key->secret, ctx->ackBufCipher, ctx->authBuf, authBufLen))
    {
        // TODO: Implement the reading of the Ack. For now we assume we connect to our own Ethereum node first
    }
    return ec;
}
int _writeLESStatus(BREthereumHandshakeContext * ctx){
    
    BREthereumPeer* peer = ctx->peer;
    
    bre_peer_log(peer, "sending status message with capabilities handshake");
    
    BRRlpCoder coder = rlpCoderCreate();

    BRRlpItem headerItems[10];
    size_t itemsCount = 0;

    /**
        [+0x00, [key_0, value_0], [key_1, value_1], …]
        “protocolVersion” P: is 1 for the LPV1 protocol version.
        “networkId” P: should be 0 for testnet, 1 for mainnet.
        “headTd” P: Total Difficulty of the best chain. Integer, as found in block header.
        “headHash” B_32: the hash of the best (i.e. highest TD) known block.
        “headNum” P: the number of the best (i.e. highest TD) known block.
        “genesisHash” B_32: the hash of the Genesis block.
        “serveHeaders” (no value): present if the peer can serve header chain downloads.
        “serveChainSince” P: present if the peer can serve Body/Receipts ODR requests starting from the given block number.
        “serveStateSince” P: present if the peer can serve Proof/Code ODR requests starting from the given block number.
        “txRelay” (no value): present if the peer can relay transactions to the ETH network.
        “flowControl/BL”, “flowControl/MRC”, “flowControl/MRR”: see Client Side Flow Control
    */
    
    headerItems[0] = rlpEncodeItemUInt64(coder, 0x00);
    
    //protocolVersion
    BRRlpItem keyPair [2];
    keyPair[0] = rlpEncodeItemString(coder, "protocolVersion");
    keyPair[1] = rlpEncodeItemUInt64(coder, 1);
    headerItems[1] = rlpEncodeListItems(coder, keyPair, 2);
    
    //networkId
    keyPair[0] = rlpEncodeItemString(coder, "networkId");
    keyPair[1] = rlpEncodeItemUInt64(coder, ctx->header.chainId);
    headerItems[2] = rlpEncodeListItems(coder, keyPair, 2);

    //headTd
    keyPair[0] = rlpEncodeItemString(coder, "headTd");
    keyPair[1] = rlpEncodeItemUInt64(coder, ctx->header.headerTd);
    headerItems[3] = rlpEncodeListItems(coder, keyPair, 2);
    
    //headHash
    keyPair[0] = rlpEncodeItemString(coder, "headHash");
    keyPair[1] = rlpEncodeItemBytes(coder, ctx->header.headHash, sizeof(ctx->header.headHash));
    headerItems[4] = rlpEncodeListItems(coder, keyPair, 2);
    
    //headNum
    keyPair[0] = rlpEncodeItemString(coder, "headNum");
    keyPair[1] = rlpEncodeItemUInt64(coder, ctx->header.headerTd);
    headerItems[5] = rlpEncodeListItems(coder, keyPair, 2);
    
    //genesisHash
    keyPair[0] = rlpEncodeItemString(coder, "genesisHash");
    keyPair[1] = rlpEncodeItemBytes(coder, ctx->header.genesisHash, sizeof(ctx->header.genesisHash));
    headerItems[6] = rlpEncodeListItems(coder, keyPair, 2);
    
    BRRlpItem encoding = rlpEncodeListItems(coder, headerItems, itemsCount);
    BRRlpData result;

    rlpDataExtract(coder, encoding, &result.bytes, &result.bytesCount);
    
    _sendBuffer(ctx, result.bytes, result.bytesCount, "write status");
    
    rlpCoderRelease(coder);

    return 0;
}
int _readLESStatus(BREthereumHandshakeContext * ctx) {

    BREthereumPeer* peer = ctx->peer;

    bre_peer_log(peer, "sending status message with capabilities handshake");
    
    //TODO: Make sure to authenticate the header with original status message
    
    /*
    _readBuffer(ctx, <#uint8_t *buf#>, <#size_t bufSize#>, <#const char *type#>); 
    
    
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = rlpGetItem (coder, data);

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert (9 == itemsCount);

    transaction->nonce = transactionDecodeNonce(items[0], coder);
    transaction->gasPrice = gasPriceRlpDecode(items[1], coder);
    transaction->gasLimit = gasRlpDecode(items[2], coder);

    char *strData = rlpDecodeItemHexString (coder, items[5], "0x");
    assert (NULL != strData);
    if ('\0' == strData[0] || 0 == strcmp (strData, "0x")) {
        // This is a ETHER transfer
        transaction->targetAddress = addressRlpDecode(items[3], coder);
        transaction->amount = amountRlpDecodeAsEther(items[4], coder);
        transaction->data = strData;
    }
    */


    return 0;
}
/*** Public functions ***/
BREthereumHandShake ethereumHandshakeCreate(BREthereumPeer * peer,
                                            BRKey* nodeKey,
                                            BREthereumBoolean didOriginate,
                                            BREthereumLESHeader* header) {

    BREthereumHandshakeContext * ctx =  ( BREthereumHandshakeContext *) calloc (1, sizeof(*ctx));
    ctx->peer = peer;
    ctx->nextState = BRE_HANDSHAKE_NEW;
    ctx->key = nodeKey;
    ctx->didOriginate = didOriginate;
    memcpy(&ctx->header, header, sizeof(ctx->header));
    //ctx->nonce = ((uint64_t)BRRand(0) << 32) | (uint64_t)BRRand(0); // random nonce
    return (BREthereumHandShake)ctx;
}
void ethereumHandshakePeerStatus(BREthereumHandShake handshake, BREthereumLESHeader* oHeader) {

    BREthereumHandshakeContext* ctx = (BREthereumHandshakeContext *)handshake;
    memcpy(oHeader, &ctx->header, sizeof(ctx->header));
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
