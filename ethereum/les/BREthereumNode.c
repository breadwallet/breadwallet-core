//
//  BREthereumLESNode.c
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 3/10/2018.
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

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <assert.h>

#include <string.h>
#include "BRInt.h"
#include "BRCrypto.h"
#include "BRKey.h"

#include "../base/BREthereumBase.h"

#include "BREthereumLESBase.h"
#include "BREthereumNode.h"
#include "BREthereumHandshake.h"
#include "BREthereumNodeDiscovery.h"
#include "BREthereumCoder.h"
#include "BREthereumEndpoint.h"
#include "BREthereumFrameCoder.h"
#include "BREthereumNodeDiscovery.h"
#include "BREthereumP2PCoder.h"

#ifndef MSG_NOSIGNAL   // linux based systems have a MSG_NOSIGNAL send flag, useful for supressing SIGPIPE signals
#define MSG_NOSIGNAL 0 // set to 0 if undefined (BSD has the SO_NOSIGPIPE sockopt, and windows has no signals at all)
#endif

#define HEADER_LEN 32
#define PTHREAD_STACK_SIZE  (16 * 1024 * 1024)
#define CONNECTION_TIME 3.0
#define DEFAULT_UDP_PORT 30303
#define DEFAULT_TCP_PORT 30303

#undef LES_LOG_SEND
#undef LES_LOG_RECV


/**
 * BREthereumPeerContext - holds information about the remote peer
 */
typedef struct {

    //the endpoint for the node
    BREthereumLESEndpoint endpoint;
    
    //socket used for communicating with a peer
    volatile int socket;
    
    //The KeyPair for the remote node
    BRKey remoteKey;
    
    //Timestamp reported by the peer
    uint64_t timestamp;
    
    //The nonce for the remote peer
    UInt256 nonce;
    
    //The ephemeral public key of the remote peer
    BRKey ephemeral;

}BREthereumPeer;

/**
 * BREthereumNodeContext - holds information about the client les node
 */
struct BREthereumLESNodeContext {
    
    //The peer information for this node
    BREthereumPeer peer;
    
    //The Key for for a node
    BRKey* key;
    
    //The current connection status of a node
    BREthereumNodeStatus status;
    
    //Framecoder for this node context
    BREthereumLESFrameCoder ioCoder;
    
    //The handshake context
    BREthereumLESHandshake handshake;
    
    //Represents whether this ndoe should start the handshake or wait for auth
    BREthereumBoolean shouldOriginate;
    
    //Flag to notify the processing thread to free the context
    BREthereumBoolean shouldFree;
    
    //Flag to notify the processing thread to disconnect from remote peer.
    BREthereumBoolean shouldDisconnect;
    
    //A local nonce for the handshake
    UInt256* nonce;
    
    // Local Ephemeral ECDH key
    BRKey* ephemeral;
    
    //The thread representing this node
    pthread_t thread;
    
    //Lock to handle shared resources in the node
    pthread_mutex_t lock;
    
    //The header buffer for this node
    uint8_t header[HEADER_LEN];
    
    //The message body for a receiving message coming peer
    uint8_t* body;
    
    //The size of the body for a receiving message coming from peer
    size_t bodySize;
    
    //The capacity for the body;
    size_t bodyCompacity;
    
    //Represents the callback functions for this node.
    BREthereumManagerCallback callbacks;
    
    //The reason why the node needs to disconnect from the remote peer
    BREthereumLESDisconnect disconnectReason;
    
    //The information about the P2P context for this node;
    BREthereumLESP2PHello helloData;
};


//
// Private Functions
//
static int _openEtheruemPeerSocket(BREthereumLESNode node, int domain, double timeout, int *error);
static void *_nodeThreadRunFunc(void *arg);

/**
 * Note: This function is a direct copy of Aaron's _BRPeerOpenSocket function with a few modifications to
 * work for the Ethereum Core.
 * TODO: May want to make this more modular to work for both etheruem and bitcoin
 */
static int _openEtheruemPeerSocket(BREthereumLESNode node, int domain, double timeout, int *error)
{
    BREthereumPeer* peer = &node->peer;
    
    struct sockaddr_storage addr;
    struct timeval tv;
    fd_set fds;
    socklen_t addrLen, optLen;
    int count, arg = 0, err = 0, on = 1, r = 1;

    peer->socket = socket(domain, SOCK_STREAM, 0);
    
    if (peer->socket < 0) {
        err = errno;
        r = 0;
    }
    else {
        tv.tv_sec = 1; // one second timeout for send/receive, so thread doesn't block for too long
        tv.tv_usec = 0;
        setsockopt(peer->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(peer->socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        setsockopt(peer->socket, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
#ifdef SO_NOSIGPIPE // BSD based systems have a SO_NOSIGPIPE socket option to supress SIGPIPE signals
        setsockopt(peer->socket, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
        arg = fcntl(peer->socket, F_GETFL, NULL);
        if (arg < 0 || fcntl(peer->socket, F_SETFL, arg | O_NONBLOCK) < 0) r = 0; // temporarily set socket non-blocking
        if (! r) err = errno;
    }

    if (r) {
        memset(&addr, 0, sizeof(addr));
        uint16_t port = endpointGetTCP(node->peer.endpoint);
        const char* address = endpointGetHost(node->peer.endpoint);
        
        if (domain == PF_INET6) {
            struct sockaddr_in6 * addr_in6 = ((struct sockaddr_in6 *)&addr);
            addr_in6->sin6_family = AF_INET6;
            inet_pton(AF_INET6, address, &(addr_in6->sin6_addr));
            addr_in6->sin6_port = htons(port);
            addrLen = sizeof(struct sockaddr_in6);
        }
        else {
            struct sockaddr_in* addr_in4 = ((struct sockaddr_in *)&addr);
            addr_in4->sin_family = AF_INET;
            inet_pton(AF_INET, address, &(addr_in4->sin_addr));
            addr_in4->sin_port = htons(port);
            addrLen = sizeof(struct sockaddr_in);
        }
        if (connect(peer->socket, (struct sockaddr *)&addr, addrLen) < 0) {
            err = errno;
        }
        
        if (err == EINPROGRESS) {
            err = 0;
            optLen = sizeof(err);
            tv.tv_sec = timeout;
            tv.tv_usec = (long)(timeout*1000000) % 1000000;
            FD_ZERO(&fds);
            FD_SET(peer->socket, &fds);
            count = select(peer->socket + 1, NULL, &fds, NULL, &tv);

            if (count <= 0 || getsockopt(peer->socket, SOL_SOCKET, SO_ERROR, &err, &optLen) < 0 || err) {
                if (count == 0) err = ETIMEDOUT;
                if (count < 0 || ! err) err = errno;
                r = 0;
            }
        }
        else if (err && domain == PF_INET && ETHEREUM_BOOLEAN_IS_TRUE(endpointIsIPV4(node->peer.endpoint))) {
            return _openEtheruemPeerSocket(node, PF_INET6, timeout, error); // fallback to IPv4
        }
        else if (err) r = 0;

        if (r) {
            eth_log(ETH_LOG_TOPIC,"%s","ethereum socket connected");
        }
        fcntl(node->peer.socket, F_SETFL, arg); // restore socket non-blocking status
    }

    if (! r && err) {
         eth_log(ETH_LOG_TOPIC, "ethereum connect error: %s", strerror(err));
    }
    if (error && err) *error = err;
    return r;
}
static BREthereumNodeStatus _readStatus(BREthereumLESNode node){

    BREthereumNodeStatus ret;
    pthread_mutex_lock(&node->lock);
    if(ETHEREUM_BOOLEAN_IS_TRUE( node->shouldFree) || (ETHEREUM_BOOLEAN_IS_TRUE(node->shouldDisconnect) &&  node->status != BRE_NODE_DISCONNECTED)){
        ret = BRE_NODE_DICONNECTING;
    }else {
        ret = node->status;
    }
    pthread_mutex_unlock(&node->lock);
    return ret;
}
static void _updateStatus(BREthereumLESNode node, BREthereumNodeStatus status){

    pthread_mutex_lock(&node->lock);
    node->status = status;
    pthread_mutex_unlock(&node->lock);
    
}
static void _disconnect(BREthereumLESNode node) {
   
    //TODO: Check if you're connected.
    int socket = node->peer.socket;
    if (socket >= 0) {
        node->peer.socket = -1;
        if (shutdown(socket, SHUT_RDWR) < 0){
            eth_log(ETH_LOG_TOPIC, "%s", strerror(errno));
        }
        close(socket);
    }
}
static BREthereumBoolean _isP2PMessage(BREthereumLESNode node, BRRlpCoder rlpCoder, uint64_t packetType, BRRlpData messageBody){

    BREthereumBoolean retStatus = ETHEREUM_BOOLEAN_FALSE;
    
    switch (packetType) {
        case BRE_P2P_HELLO:
        {
            // Unused (but leaking memory):  BREthereumP2PHello remoteHello = ethereumP2PHelloDecode(rlpCoder, messageBody);
            //TODO: Check over capabalities of the message
            retStatus = ETHEREUM_BOOLEAN_TRUE;
        }
        break;
        case  BRE_P2P_DISCONNECT:
        {
            BREthereumLESDisconnect reason = p2pDisconnectDecode(rlpCoder, messageBody);
            eth_log(ETH_LOG_TOPIC, "Remote Peer requested to disconnect:%s", p2pDisconnectToString(reason));
            node->disconnectReason = reason;
            node->shouldDisconnect = ETHEREUM_BOOLEAN_TRUE;
            retStatus = ETHEREUM_BOOLEAN_TRUE;
        }
        break;
        case  BRE_P2P_PING:
        {
            BRRlpData data = p2pPongEncode();
            uint8_t* frame;
            size_t frameSize;
            frameCoderEncrypt(node->ioCoder, data.bytes, data.bytesCount, &frame, &frameSize);
            nodeWriteToPeer(node, frame, frameSize, "P2P Pong");
            rlpDataRelease(data);
            free(frame);
            retStatus = ETHEREUM_BOOLEAN_TRUE;
        }
        break;
        default:
        break;
    }
    return retStatus;
}
static int _readMessage(BREthereumLESNode node) {

    // eth_log(ETH_LOG_TOPIC, "%s", "reading message from peer");
    
    //1st. Read in the header from the remote peer' packet
    int ec = nodeReadFromPeer(node, node->header, 32, "");
    
    if(ec){
        eth_log(ETH_LOG_TOPIC, "%s","Error: reading in message from remote peer");
        return 1;
    }
    
    // authenticate and decrypt header
    if(ETHEREUM_BOOLEAN_IS_FALSE(frameCoderDecryptHeader(node->ioCoder, node->header, 32)))
    {
        eth_log(ETH_LOG_TOPIC, "%s", "Error: Decryption of header from peer failed.");
        return 1;
    }

    //Get frame size
    uint32_t frameSize = (uint32_t)(node->header[2]) | (uint32_t)(node->header[1])<<8 | (uint32_t)(node->header[0])<<16;
    
   /*
   //TODO: We should check to make sure the framze size is less then 3-bytes. Geth doesn't check for that currently.
   if(frameSize > 1024){
        eth_log(ETH_LOG_TOPIC, "%s", "Error: message frame size is too large");
        return 1;
    }*/ 

    uint32_t fullFrameSize = frameSize + ((16 - (frameSize % 16)) % 16) + 16;
    

    if(node->body == NULL){
      node->body = malloc(fullFrameSize);
      node->bodyCompacity = fullFrameSize;
    }else if (node->bodyCompacity <= fullFrameSize) {
      node->body = realloc(node->body, fullFrameSize);
      node->bodyCompacity = fullFrameSize;
    }
    
    ec = nodeReadFromPeer(node, node->body, fullFrameSize, "");
    
    if(ec) {
        eth_log(ETH_LOG_TOPIC, "%s", "Error: Reading in full body message from remote peer");
        return 1;
    }
    
    // authenticate and decrypt frame
    if(ETHEREUM_BOOLEAN_IS_FALSE(frameCoderDecryptFrame(node->ioCoder, node->body, fullFrameSize)))
    {
        eth_log(ETH_LOG_TOPIC, "%s","Error: failed to decrypt frame from remote peer");
        return 1;
    }
    
    node->bodySize = frameSize;
    
    return 0;
}
/**
 * This is the theard run functions for an ethereum function. This function is called
 * when a node needs to begin connecting to a remote peer and start sending messages to the
 * remote node.
 */
static void *_nodeThreadRunFunc(void *arg) {

    BREthereumLESNode node = (BREthereumLESNode)arg;
    BREthereumNodeStatus status;
    
#if defined (__ANDROID__)
    pthread_setname_np(clock->thread, "Core Ethereum LES Node");
#else
    pthread_setname_np("Core Ethereum LES Node");
#endif

    while(node != NULL && (status = _readStatus(node)) != BRE_NODE_DISCONNECTED)
    {

        if (ETHEREUM_BOOLEAN_IS_TRUE(node->shouldDisconnect)){
            _disconnect(node);
            _updateStatus(node, BRE_NODE_DISCONNECTED);
            break;
        }
        else
        {
            switch (status) {
                case BRE_NODE_CONNECTING:
                {
                    node->handshake = handshakeCreate(node);
                    _updateStatus(node, BRE_NODE_PERFORMING_HANDSHAKE);
                }
                break;
                case BRE_NODE_PERFORMING_HANDSHAKE:
                {
                    BREthereumLESHandshakeStatus handshakeStatus = handshakeTransition(node->handshake);
                    
                    if(handshakeStatus == BRE_HANDSHAKE_FINISHED) {
                        eth_log(ETH_LOG_TOPIC, "%s", "P2P Handshake completed");
                        uint8_t* status;
                        size_t statusSize;
                        //Notify the node manager that node finished its handshake connection and is ready to send
                        //sub protocool status message;
                        node->callbacks.connectedFuc(node->callbacks.info, node, &status, &statusSize);
                        //Check to see if we need to send a status message
                        if(status != NULL && statusSize > 0){
                            eth_log(ETH_LOG_TOPIC, "%s", "Generated sub protocool status message to be sent");
                            uint8_t* statusPayload;
                            size_t statusPayloadSize;
                            frameCoderEncrypt(node->ioCoder, status, statusSize, &statusPayload, &statusPayloadSize);
                            nodeWriteToPeer(node, statusPayload, statusPayloadSize, "status message of BRD node");
                            free(statusPayload);
                            free(status);
                        }
                        handshakeRelease(node->handshake);
                        _updateStatus(node,BRE_NODE_CONNECTED);
                    }
                    else if (handshakeStatus ==  BRE_HANDSHAKE_ERROR) {
                        node->shouldDisconnect = ETHEREUM_BOOLEAN_TRUE;
                    }
                }
                break;
                case BRE_NODE_CONNECTED:
                {
                    //Read message from peer
                    if(!_readMessage(node))
                    {
                        //Decode the Packet Type
                        BRRlpCoder rlpCoder = rlpCoderCreate();
                        BRRlpData framePacketTypeData = {1, node->body};
                        BRRlpItem item = rlpGetItem (rlpCoder, framePacketTypeData);
    
                        uint64_t packetType = rlpDecodeItemUInt64(rlpCoder, item, 1);
                        BRRlpData mesageBody = {node->bodySize - 1, &node->body[1]};
                        
                        //Check if the message is a P2P message before broadcasting the message to the manager
                        if(ETHEREUM_BOOLEAN_IS_FALSE(_isP2PMessage(node,rlpCoder,packetType, mesageBody)))
                        {
                            node->callbacks.receivedMsgFunc(node->callbacks.info, node, packetType, mesageBody);
                        }
                        rlpCoderRelease(rlpCoder);
                    }
                }
                break;
                default:
                break;
            }
        }
    }
    return NULL;
}
//
// Public functions
//
BREthereumLESNode nodeCreate(BREthereumLESPeerConfig config,
                                  BRKey* key,
                                  UInt256* nonce,
                                  BRKey* ephemeral,
                                  BREthereumManagerCallback callbacks,
                                  BREthereumBoolean originate) {

    BREthereumLESNode node = (BREthereumLESNode) calloc (1, sizeof(struct BREthereumLESNodeContext));
    node->status = BRE_NODE_DISCONNECTED;
    node->handshake = NULL;
    node->key = key;
    node->nonce = nonce;
    node->ephemeral = ephemeral;
    node->ioCoder = frameCoderCreate();
    node->callbacks = callbacks;
    node->body = NULL;
    node->bodySize = 0;
    node->peer.endpoint = config.endpoint;
    node->peer.timestamp = config.timestamp;
    node->peer.remoteKey =  *(config.remoteKey);
    //Initialize p2p data
    node->helloData.version = 0x03;
    char clientId[] = "BRD Light Client";
    node->helloData.clientId = malloc(strlen(clientId) + 1);
    strcpy(node->helloData.clientId, clientId);
    node->helloData.listenPort = 0;
    array_new(node->helloData.caps, 1);
    BREthereumLESCapabilities cap;
    char capStr[] = "les";
    cap.cap = malloc(strlen(capStr) + 1);
    strcpy(cap.cap, capStr);
    cap.capVersion = 2;
    array_add(node->helloData.caps, cap);
    uint8_t pubRawKey[65];
    size_t pLen = BRKeyPubKey(key, pubRawKey, sizeof(pubRawKey));
    memcpy(node->helloData.nodeId.u8, &pubRawKey[1], pLen - 1);
    
    node->shouldOriginate = originate;
    //Initiliaze thread information
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&node->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    return node;
}
int nodeConnect(BREthereumLESNode node) {

    int error = 0;
    pthread_attr_t attr;
    
    node->shouldDisconnect = ETHEREUM_BOOLEAN_FALSE;
    node->shouldFree = ETHEREUM_BOOLEAN_FALSE;
    
    if(!error && _openEtheruemPeerSocket(node, PF_INET, CONNECTION_TIME, &error)) {
        
        if (node->status == BRE_NODE_DISCONNECTED) {
            node->status = BRE_NODE_CONNECTING;
            
            if (pthread_attr_init(&attr) != 0) {
                error = ENOMEM;
                eth_log(ETH_LOG_TOPIC, "%s", "error creating thread");
                node->status = BRE_NODE_DISCONNECTED;
            }
            else if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 ||
                     pthread_attr_setstacksize(&attr, PTHREAD_STACK_SIZE) != 0 ||
                     pthread_create(&node->thread, &attr, _nodeThreadRunFunc, node) != 0) {
                error = EAGAIN;
                eth_log(ETH_LOG_TOPIC, "%s", "error creating thread");
                pthread_attr_destroy(&attr);
                node->status = BRE_NODE_DISCONNECTED;
            }
        }
    }
    return error;
}
BREthereumNodeStatus nodeStatus(BREthereumLESNode node){

    return _readStatus(node);
}
void nodeDisconnect(BREthereumLESNode node, BREthereumLESDisconnect reason) {

    eth_log(ETH_LOG_TOPIC, "Local node disconnecting from remote peer:%s", p2pDisconnectToString(reason));
    BRRlpData data = p2pDisconnectEncode(reason);
    uint8_t* frame;
    size_t frameSize;
    frameCoderEncrypt(node->ioCoder, data.bytes, data.bytesCount, &frame, &frameSize);
    nodeWriteToPeer(node, frame, frameSize, "P2P Local Disconnect");
    node->shouldDisconnect = ETHEREUM_BOOLEAN_TRUE;
    node->disconnectReason = reason;
    rlpDataRelease(data);
    free(frame);
}
void nodeRelease(BREthereumLESNode node){
   endpointRelease(node->peer.endpoint);
   if(node->body != NULL){
     free(node->body);
   }
   frameCoderRelease(node->ioCoder);
   free(node->key);
   free(node->ephemeral);
   free(node);
}
BREthereumBoolean nodeEQ(BREthereumLESNode node1, BREthereumLESNode node2) {

    if(memcmp(node1->key->secret.u8, node2->key->secret.u8, 32) == 0) {
        return ETHEREUM_BOOLEAN_TRUE;
    }
    
    return ETHEREUM_BOOLEAN_FALSE;
}
BREthereumBoolean nodeSendMessage(BREthereumLESNode node, uint64_t packetType, uint8_t* payload, size_t payloadSize) {
 
    assert(node != NULL);
    
    BREthereumNodeStatus status =  _readStatus(node);
    BREthereumBoolean retStatus = ETHEREUM_BOOLEAN_FALSE;
    
    if(status == BRE_NODE_CONNECTED){
        uint8_t* bytes;
        size_t bytesCount;
        frameCoderEncrypt(node->ioCoder, payload, payloadSize, &bytes, &bytesCount);
        if(!nodeWriteToPeer(node, bytes, bytesCount, "sending message")){
            retStatus = ETHEREUM_BOOLEAN_TRUE;
        }
        free(bytes);
    }
    return retStatus;
}

BREthereumLESFrameCoder nodeGetFrameCoder(BREthereumLESNode node) {
    return node->ioCoder;
}
BRKey* nodeGetKey(BREthereumLESNode node){
    return node->key;
 }

BRKey* nodeGetPeerKey(BREthereumLESNode node) {
    return &node->peer.remoteKey;
}
BREthereumBoolean nodeDidOriginate(BREthereumLESNode node){
    return node->shouldOriginate;
}
BREthereumPeer nodeGetPeer(BREthereumLESNode node) {
    return node->peer;
}
BRKey* nodeGetEphemeral(BREthereumLESNode node) {
    return node->ephemeral;
}
BRKey* nodeGetPeerEphemeral(BREthereumLESNode node) {

    return &node->peer.ephemeral;
}
UInt256* nodeGetNonce(BREthereumLESNode node) {
    return node->nonce;
}
UInt256* nodeGetPeerNonce(BREthereumLESNode node) {
    return &node->peer.nonce;
}
BRRlpData nodeRLPP2PHello(BREthereumLESNode node) {

    return p2pHelloEncode(&node->helloData);
}
int nodeReadFromPeer(BREthereumLESNode node, uint8_t * buf, size_t bufSize, const char * type){

    BREthereumPeer* peerCtx = &node->peer;
    ssize_t n = 0, len = 0;
    int socket, error = 0;

    const char* hostAddress = endpointGetHost(peerCtx->endpoint);

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
        if (node->status != BRE_NODE_ERROR)
            eth_log(ETH_LOG_TOPIC, "[READ FROM PEER ERROR]:%s", strerror(error));
        node->status = BRE_NODE_ERROR;
        return error;
    }

#if defined (LES_LOG_RECV)
        eth_log(ETH_LOG_TOPIC, "read (%zu bytes) from peer [%s], contents: %s", len, hostAddress, type);
#endif
    return error;
}
int nodeWriteToPeer(BREthereumLESNode node, uint8_t * buf, size_t bufSize, char* type){

    BREthereumPeer* peerCtx = &node->peer;
    ssize_t n = 0;
    int socket, error = 0;

    const char* hostAddress = endpointGetHost(peerCtx->endpoint);
    
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
        if (BRE_NODE_ERROR != node->status)
            eth_log(ETH_LOG_TOPIC, "[WRITE TO PEER ERROR]:%s", strerror(error));
        node->status = BRE_NODE_ERROR;
        return error;
    }

#if defined (LES_LOG_SEND)
        eth_log(ETH_LOG_TOPIC, "sent (%zu bytes) to peer[%s], contents: %s", offset, hostAddress, type);
#endif

    return error;
}
