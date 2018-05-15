//
//  BREthereumNode.c
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
#include "BREthereumNode.h"
#include "BREthereumHandshake.h"
#include "BREthereumBase.h"
#include "BRKey.h"
#include "BREthereumNodeEventHandler.h"
#include "BREthereumFrameCoder.h"

#ifndef MSG_NOSIGNAL   // linux based systems have a MSG_NOSIGNAL send flag, useful for supressing SIGPIPE signals
#define MSG_NOSIGNAL 0 // set to 0 if undefined (BSD has the SO_NOSIGPIPE sockopt, and windows has no signals at all)
#endif

#define PTHREAD_STACK_SIZE  (512 * 1024)
#define CONNECTION_TIME 3.0
#define DEFAULT_LES_PORT 30303


/**
 * BREthereumPeerContext - holds information about the remote peer
 */
typedef struct {

    //ipv6 address of the peer
    UInt128 address;
    
    //port number of peer connection
    uint16_t port;
    
    //The name for a host
    char host[INET6_ADDRSTRLEN];
    
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
struct BREthereumNodeContext {
    
    //The peer information for this node
    BREthereumPeer peer;
    
    //The Key for for a node
    BRKey key; 
    
    //The current connection status of a node
    BREthereumNodeStatus status;
    
    //Information about the protocool this node supports
    BREthereumLESStatus lesStatusInfo;
    
    //Framecoder for this node context
    BREthereumFrameCoder ioCoder;
    
    //The handshake context
    BREthereumHandshake handshake;
    
    //Represents whether this ndoe should start the handshake or wait for auth
    BREthereumBoolean shouldOriginate;
    
    //Represents the event handler for the node
    BREthereumNodeEventHandler eventHandler;
    
    //Flag to notify the processing thread to free the context
    BREthereumBoolean shouldFree;
    
    //Flag to notify the processing thread to disconnect from remote peer.
    BREthereumBoolean shouldDisconnect;
    
    //The status information for the remote peer
    BREthereumLESStatus remoteStatus;
    
    //A local nonce for the handshake
    UInt256 nonce;
    
    // Local Ephemeral ECDH key
    BRKey ephemeral;
    
    //The thread representing this node
    pthread_t thread;
    
    //Lock to handle shared resources in the node
    pthread_mutex_t lock;
};




/**** Private functions ****/
static BREthereumBoolean _isAddressIPv4(UInt128 address);
static int _openEtheruemPeerSocket(BREthereumNode node, int domain, double timeout, int *error);
//static void _updateStatus(BREthereumNodeContext * node, BREthereumNodeStatus status);
static void *_nodeThreadRunFunc(void *arg);


/**
 * Note: This function is a direct copy of Aaron's _BRPeerOpenSocket function with a few modifications to
 * work for the Ethereum Core.
 * TODO: May want to make this more modular to work for both etheruem and bitcoin
 */
static int _openEtheruemPeerSocket(BREthereumNode node, int domain, double timeout, int *error)
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
        
        if (domain == PF_INET6) {
            ((struct sockaddr_in6 *)&addr)->sin6_family = AF_INET6;
            ((struct sockaddr_in6 *)&addr)->sin6_addr = *(struct in6_addr *)&peer->address;
            ((struct sockaddr_in6 *)&addr)->sin6_port = htons(peer->port);
            addrLen = sizeof(struct sockaddr_in6);
        }
        else {
            ((struct sockaddr_in *)&addr)->sin_family = AF_INET;
            ((struct sockaddr_in *)&addr)->sin_addr = *(struct in_addr *)&peer->address.u32[3];
            ((struct sockaddr_in *)&addr)->sin_port = htons(peer->port);
            addrLen = sizeof(struct sockaddr_in);
        }
        
        if (connect(peer->socket, (struct sockaddr *)&addr, addrLen) < 0) err = errno;
        
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
        else if (err && domain == PF_INET6 && ETHEREUM_BOOLEAN_IS_TRUE(_isAddressIPv4(peer->address))) {
            return _openEtheruemPeerSocket(node, PF_INET, timeout, error); // fallback to IPv4
        }
        else if (err) r = 0;

        if (r) {
            bre_node_log(node, "ethereum socket connected");
        }
        fcntl(node->peer.socket, F_SETFL, arg); // restore socket non-blocking status
    }

    if (! r && err) {
         bre_node_log(node, "ethereum connect error: %s", strerror(err));
    }
    if (error && err) *error = err;
    return r;
}
static BREthereumNodeStatus _readStatus(BREthereumNode node){

    BREthereumNodeStatus ret;
    pthread_mutex_lock(&node->lock);
    ret = node->status;
    pthread_mutex_unlock(&node->lock);
    return ret;
}
static void _updateStatus(BREthereumNode node, BREthereumNodeStatus status){

    pthread_mutex_lock(&node->lock);
    node->status = status;
    pthread_mutex_unlock(&node->lock);
    
}
static void _processEvent(BREthereumNode node){

    
}


/**
 * This is the theard run functions for an ethereum function. This function is called
 * when a node needs to begin connecting to a remote peer and start sending messages to the
 * remote node.
 */
static void *_nodeThreadRunFunc(void *arg) {

    BREthereumNode node = (BREthereumNode)arg;
    BREthereumNodeStatus status;

    while((status = _readStatus(node)) != BRE_NODE_DISCONNECTED)
    {
    
        if(node->shouldFree && status != BRE_NODE_DISCONNECTED){
            //TODO: Implement
                // 1. Disconnect from the remote-peer
                // 2. Free resources  i.e. context

        }
        else if (node->shouldDisconnect && status != BRE_NODE_DISCONNECTED){
            //TODO: Implement
                // 1. Disconnect from the remote-peer
        }
        else
        {
        
            switch (status) {
                case BRE_NODE_DISCONNECTED:
                {
                    continue;
                }
                break;
                case BRE_NODE_CONNECTING:
                {
                 //   ctx->handshake = ethereumHandshakeCreate(&ctx->peer, &ctx->key, ctx->shouldOriginate);
                    _updateStatus(node, BRE_NODE_PERFORMING_HANDSHAKE);
                }
                break;
                case BRE_NODE_PERFORMING_HANDSHAKE:
                {
                    if(ethereumHandshakeTransition(node->handshake) == BRE_HANDSHAKE_FINISHED) {
                        _updateStatus(node,BRE_NODE_CONNECTED);
                        // TODO: Get information about the status of the remote node before
                        //ethereumHandshakeFree(node->handshake);
                    }
                }
                break;
                case BRE_NODE_CONNECTED:
                {
                    /** Steps to take once connected to a remote node
                     * 1.) Read from the event queue
                     *     1.a) Send request to peer if needed
                     * 3.) Read response from the peer, or timeout if its taking too long
                     * 4.) If received response then notify the NodeManger and pass the receipt to manager
                     * 5.) Jump back to step 1.
                    */
                    //_proccessEvent(ctx);
                    
                    
                    
                }
                break;
                default:
                break;
            }
        }
    }
    return NULL;
}
static BREthereumBoolean _isAddressIPv4(UInt128 address)
{
    return (address.u64[0] == 0 && address.u16[4] == 0 && address.u16[5] == 0xffff) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}


//
// Public functions
//
BREthereumNode ethereumNodeCreate (BREthereumPeerConfig config,
                                   BREthereumBoolean originate) {

    BREthereumNode node = (BREthereumNode) calloc (1, sizeof(struct BREthereumNodeContext));
    node->status = BRE_NODE_DISCONNECTED;
    node->handshake = NULL;
    node->shouldOriginate = originate;
        //ctx->nonce = UINT256_ZERO; //TODO: Get Random Nonce;
    /// TODO GET RANDOM KEY ctx->localEphemeral = BRKEY
    node->eventHandler = ethereumNodeEventHandlerCreate();
    memcpy(node->peer.address.u8, config.address.u8, sizeof(config.address.u8));
    node->peer.port = config.port;
    node->peer.timestamp = config.timestamp;
    
    //Initiliaze thread information
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&node->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    
    return (BREthereumNode)node;

}
void ethereumNodeConnect(BREthereumNode node) {

    int error = 0;
    pthread_attr_t attr;
    
    node->shouldDisconnect = ETHEREUM_BOOLEAN_FALSE;
    node->shouldFree = ETHEREUM_BOOLEAN_FALSE;
    
    if(_openEtheruemPeerSocket(node, PF_INET6, CONNECTION_TIME, &error)) {

        if (node->status == BRE_NODE_DISCONNECTED) {
            node->status = BRE_NODE_CONNECTING;
            
            if (pthread_attr_init(&attr) != 0) {
                error = ENOMEM;
                bre_node_log(node, "error creating thread");
                node->status = BRE_NODE_DISCONNECTED;
            }
            else if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 ||
                     pthread_attr_setstacksize(&attr, PTHREAD_STACK_SIZE) != 0 ||
                     pthread_create(&node->thread, &attr, _nodeThreadRunFunc, node) != 0) {
                error = EAGAIN;
                bre_node_log(node, "error creating thread");
                pthread_attr_destroy(&attr);
                node->status = BRE_NODE_DISCONNECTED;
            }
        }
    }
}
BREthereumNodeStatus ethereumNodeStatus(BREthereumNode node){

    return _readStatus(node);
}
void ethereumNodeDisconnect(BREthereumNode node) {
    
    node->status = BRE_NODE_DISCONNECTED;
    int socket = node->peer.socket;
    node->shouldDisconnect = ETHEREUM_BOOLEAN_TRUE;
    if (socket >= 0) {
        node->peer.socket = -1;
        if (shutdown(socket, SHUT_RDWR) < 0){
            bre_node_log(node, "%s", strerror(errno));
        }
        close(socket);
    }
}
BREthereumFrameCoder ethereumNodeGetFrameCoder(BREthereumNode node) {
    return node->ioCoder;
}
BRKey* ethereumNodeGetKey(BREthereumNode node){
    return &node->key;
 }

BRKey* ethereumNodeGetRemoteKey(BREthereumNode node) {

    return &node->peer.remoteKey;
    
 }
BRRlpData ethereumNodeGetStatusData(BREthereumNode node) {

    BRRlpData data;
    ethereumLESEncodeStatus(&node->lesStatusInfo, &data.bytes, &data.bytesCount);
    return data;
}
BREthereumBoolean ethereumNodeDidOriginate(BREthereumNode node){
    return node->shouldOriginate;
}
BREthereumLESStatus* BREthereumNodeGetPeerStatus(BREthereumNode node) {
    return &node->remoteStatus;
}
BREthereumPeer ethereumNodeGetPeer(BREthereumNode node) {
    return node->peer;
}
BRKey* ethereumNodeGetEphemeral(BREthereumNode node) {
    return &node->ephemeral;
}
BRKey* ethereumNodeGetPeerEphemeral(BREthereumNode node) {
    return &node->peer.ephemeral;
}
UInt256* ethereumNodeGetNonce(BREthereumNode node) {
    return &node->nonce;
}
UInt256* ethereumNodeGetPeerNonce(BREthereumNode node) {
    return &node->peer.nonce;
}
void ethereumNodeFree(BREthereumNode node) {

    node->shouldFree = ETHEREUM_BOOLEAN_TRUE;
    free(node);
}
BRRlpData ethereumNodeGetEncodedHelloData(BREthereumNode node) {

    bre_node_log(node, "encoding hello message");
    
    BRRlpData data;
    
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem helloItems[2];
    BRRlpItem helloDataItems[5];
    
    /**
            Hello 0x00 [p2pVersion: P, clientId: B, [[cap1: B_3, capVersion1: P], [cap2: B_3, capVersion2: P], ...], listenPort: P, nodeId: B_64] First packet sent over the connection, and sent once by both sides. No other messages may be sent until a Hello is received.

            p2pVersion Specifies the implemented version of the P2P protocol. Now must be 1.
            clientId Specifies the client software identity, as a human-readable string (e.g. "Ethereum(++)/1.0.0").
            cap Specifies a peer capability name as a length-3 ASCII string. Current supported capabilities are eth, shh.
            capVersion Specifies a peer capability version as a positive integer. Current supported versions are 34 for eth, and 1 for shh.
            listenPort specifies the port that the client is listening on (on the interface that the present connection traverses). If 0 it indicates the client is not listening.
            nodeId is the Unique Identity of the node and specifies a 512-bit hash that identifies this node.
    **/
    /** Encode the following : [[cap1: B_3, capVersion1: P], [cap2: B_3, capVersion2: P], ...], */
    BRRlpItem ethCapItems[2];
    ethCapItems[0] = rlpEncodeItemString(coder, "eth");
    ethCapItems[1] = rlpEncodeItemUInt64(coder, 34, 0);
    BRRlpItem etheCapItemsEncoding = rlpEncodeListItems(coder, ethCapItems, 2);
    BRRlpItem capItems[1];
    capItems[0] = etheCapItemsEncoding;
    BRRlpItem capsItem = rlpEncodeListItems(coder, capItems, 1);


    /** Encode the following : [p2pVersion: P, clientId: B, [[cap1: B_3, capVersion1: P], [cap2: B_3, capVersion2: P], ...], listenPort: P, nodeId: B_64] */
    helloDataItems[0] = rlpEncodeItemUInt64(coder, 0x00,0);
    helloDataItems[1] = rlpEncodeItemString(coder, "Ethereum(++)/1.0.0");
    helloDataItems[2] = capsItem;
    helloDataItems[3] = rlpEncodeItemUInt64(coder, 0,0);
    uint8_t pubKey[65];
    size_t pubKeyLength = BRKeyPubKey(&node->key, pubKey, 0);
    assert(pubKeyLength == 65);
    helloDataItems[4] = rlpEncodeItemBytes(coder, &pubKey[1], 64);

    /** Encode the following :  Hello 0x00 [p2pVersion: P, clientId: B, [[cap1: B_3, capVersion1: P], [cap2: B_3, capVersion2: P], ...], listenPort: P, nodeId: B_64] */
    BRRlpData listData, idData;
    rlpDataExtract(coder, rlpEncodeItemUInt64(coder, 0x00,0),&idData.bytes, &idData.bytesCount);
    rlpDataExtract(coder, rlpEncodeListItems(coder, helloDataItems, 5), &listData.bytes, &listData.bytesCount);
    
    uint8_t * rlpData = malloc(idData.bytesCount + listData.bytesCount);
    memcpy(rlpData, idData.bytes, idData.bytesCount);
    memcpy(&rlpData[idData.bytesCount], listData.bytes, listData.bytesCount);

    data.bytes = rlpData;
    data.bytesCount = idData.bytesCount + listData.bytesCount;
    
    rlpDataRelease(listData);
    rlpDataRelease(idData);
    rlpCoderRelease(coder);
    
    
    return data;
}
uint16_t ethereumNodeGetPeerPort(BREthereumNode node) {
    return node->peer.port;
}
const char * ethereumNodeGetPeerHost(BREthereumNode node){
    
    BREthereumPeer * peerCtx =  &node->peer;
    
    if( peerCtx->host[0] == '\0') {
        if (ETHEREUM_BOOLEAN_IS_TRUE(_isAddressIPv4(peerCtx->address))) {
            inet_ntop(AF_INET, &peerCtx->address.u32[3], peerCtx->host, sizeof(peerCtx->host));
        }else {
            inet_ntop(AF_INET6, &peerCtx->address, peerCtx->host, sizeof(peerCtx->host));
        }
    }
    return peerCtx->host;
}
int ethereumNodeReadFromPeer(BREthereumNode node, uint8_t * buf, size_t bufSize, const char * type){

    BREthereumPeer* peerCtx = &node->peer;
    ssize_t n = 0, len = 0;
    int socket, error = 0;

    bre_node_log(node, "now reading from peer: %s", type);

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
        bre_node_log(node, "%s", strerror(error));
    }
    return error;
}
int ethereumNodeWriteToPeer(BREthereumNode node, uint8_t * buf, size_t bufSize, char* type){

    BREthereumPeer* peerCtx = &node->peer;
    ssize_t n = 0;
    int socket, error = 0;

    bre_node_log(node, "now sending to peer: %s", type);

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
        bre_node_log(node, "%s", strerror(error));
    }
    return error;
}
void ethereumNodeAnnounceEvent(BREthereumNode node, BREthereumNodeEvent evt){
    ethereumNodeEventHandlerEnqueue(node->eventHandler, evt);
}
