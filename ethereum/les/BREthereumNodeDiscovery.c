//
//  BREthereumNodeDiscovery.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 5/15/18.
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
#include <arpa/inet.h>
#include <stdio.h>
#include "BRKey.h"
#include "BRInt.h"
#include "../rlp/BRRlp.h"
#include "BRArray.h"
#include "BRCrypto.h"
#include "../util/BRUtil.h"
#include "BREthereumNodeDiscovery.h"

#ifndef MSG_NOSIGNAL   // linux based systems have a MSG_NOSIGNAL send flag, useful for supressing SIGPIPE signals
#define MSG_NOSIGNAL 0 // set to 0 if undefined (BSD has the SO_NOSIGPIPE sockopt, and windows has no signals at all)
#endif


#define HASH_BYTES_SIZE 32
#define SIGNATURE_BYTES_SIZE 65
#define PACKET_TYPE_BYTES_SIZE 1

#define LOG_TOPIC "Node Discovery"

#define MAX_PAYLOAD_SIZE 1280
#define CONNECTION_TIME 3  // receiver only accept packets created within the last 3 seconds (RLP Node Discovery Spec)

typedef struct {
    //port number of peer connection
    uint16_t port;
    
    //socket used for communicating with a peer
    volatile int socket;
}BREthereumNodeDiscoveryContext;


#define MAX_HOST_NAME 1024

typedef enum {
    BRE_PACKET_TYPE_PING = 0x01,
    BRE_PACKET_TYPE_PONG = 0x02,
    BRE_PACKET_TYPE_FIND_NEIGHBORS = 0x03,
    BRE_PACKET_TYPE_NEIGHBORS = 0x04
}BREthereumDiscoveryPacketType;

struct BREthereumLESEndpointContext {
    // BE encoded 4-byte or 16-byte address (size determines ipv4 vs ipv6)
    int addr_family;
    char hostname[MAX_HOST_NAME];
    union{
        struct {
            uint8_t address_bytes[4];
            struct sockaddr_in addr;
        }ipv4;
        struct {
            uint8_t address_bytes[16];
            struct sockaddr_in6 addr;
        }ipv6;
    }u;
    socklen_t addrSize;
    uint16_t udpPort; // BE encoded 16-bit unsigned
    uint16_t tcpPort; // BE encoded 16-bit unsigned
};

struct BREthereumLESPingNodeContext
{
    BREthereumLESEndpoint from;
    BREthereumLESEndpoint to;
};

struct BREthereumLESPongNodeContext
{
    BREthereumLESEndpoint to;
    UInt256 echo;
    uint32_t timestamp;
};
/*
typedef struct
{
    UInt512 target; // Id of a node. The responding node will send back nodes closest to the target.
    uint32_t timestamp;
}BREthereumFindNeighbours;


typedef struct
{
    BREthereumEndpoint endpoint;
    BREthereumNodeId node;
}BREthereumNeighborRequest;

typedef struct
{
    BREthereumNeighborRequest* requests;
    size_t requestCount;
    uint32_t timestamp;
}BREthereumNeighbours;
*/


//
// Private functions
//
/*static BREthereumBoolean _isAddressIPv4(UInt128 address)
{
    return (address.u64[0] == 0 && address.u16[4] == 0 && address.u16[5] == 0xffff) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}*/

static int _openUDPSocket(BREthereumLESEndpoint endpoint, int domain,  double timeout, int* retSocket, int *error){

    struct sockaddr_in addr;
    struct timeval tv;
    int arg = 0, err = 0, on = 1, r = 1, sock;
    
    //Create socket
    if ((sock = socket(domain, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        eth_log(LOG_TOPIC, "Failed to create UDP socket:%d", sock);
        err = errno;
        r = 0;
    }else {
        tv.tv_sec = 3; // three second timeout for send/receive, so thread doesn't block for too long
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
#ifdef SO_NOSIGPIPE // BSD based systems have a SO_NOSIGPIPE socket option to supress SIGPIPE signals
        setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
        arg = fcntl(sock, F_GETFL, NULL);
        if (arg < 0 || fcntl(sock, F_SETFL, arg | O_NONBLOCK) < 0) r = 0; // temporarily set socket non-blocking
        if (! r) err = errno;
    }
    
    if (r) {
        memset(&addr, 0, sizeof(addr));
        
        //TODO: Make sure to cover for IPV6
        addr.sin_family = endpoint->addr_family;
        addr.sin_port  = htons(endpoint->udpPort);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        
        if (bind(sock, (struct sockaddr *) &endpoint->u.ipv4.addr, endpoint->addrSize) < 0) {
            err = errno;
            eth_log(LOG_TOPIC, "bind error:%s", strerror(err));
        }else {
            fcntl(sock, F_SETFL, arg); // restore socket non-blocking status
            eth_log(LOG_TOPIC, "successfully opened socket:%d", endpoint->udpPort);
            *retSocket = sock;
        }
    }
    if (error && err ){
        *error = err;
    }
    return r;
}


/**
 * Note: This function is a direct copy of Aaron's _BRPeerOpenSocket function with a few modifications to
 * work for the Ethereum Core.
 * TODO: May want to make this more modular to work for both etheruem and bitcoin
 */
 /*
static int _openSocket(BREthereumNodeDiscoveryContext* ctx, int domain, double timeout, int *error)
{
    struct sockaddr_storage addr;
    struct timeval tv;
    fd_set fds;
    socklen_t addrLen, optLen;
    int count, arg = 0, err = 0, on = 1, r = 1;

    ctx->socket = socket(domain, SOCK_DGRAM, 0);
    
    if (ctx->socket < 0) {
        err = errno;
        r = 0;
    }
    else {
        tv.tv_sec = timeout; // three second timeout for send/receive, so thread doesn't block for too long
        tv.tv_usec = 0;
        setsockopt(ctx->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(ctx->socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        setsockopt(ctx->socket, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
#ifdef SO_NOSIGPIPE // BSD based systems have a SO_NOSIGPIPE socket option to supress SIGPIPE signals
        setsockopt(ctx->socket, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
        arg = fcntl(ctx->socket, F_GETFL, NULL);
        if (arg < 0 || fcntl(ctx->socket, F_SETFL, arg | O_NONBLOCK) < 0) r = 0; // temporarily set socket non-blocking
        if (! r) err = errno;
    }

    if (r) {
        memset(&addr, 0, sizeof(addr));
        
        if (domain == PF_INET6) {
            ((struct sockaddr_in6 *)&addr)->sin6_family = AF_INET6;
            ((struct sockaddr_in6 *)&addr)->sin6_addr = *(struct in6_addr *)&ctx->address;
            ((struct sockaddr_in6 *)&addr)->sin6_port = htons(ctx->port);
            addrLen = sizeof(struct sockaddr_in6);
        }
        else {
            ((struct sockaddr_in *)&addr)->sin_family = AF_INET;
            ((struct sockaddr_in *)&addr)->sin_addr = *(struct in_addr *)&ctx->address.u32[3];
            ((struct sockaddr_in *)&addr)->sin_port = htons(ctx->port);
            addrLen = sizeof(struct sockaddr_in);
        }
        
        struct sockaddr_storage addr;

        if (connect(ctx->socket, (struct sockaddr *)&addr, addrLen) < 0) err = errno;
        
        if (err == EINPROGRESS) {
            err = 0;
            optLen = sizeof(err);
            tv.tv_sec = timeout;
            tv.tv_usec = (long)(timeout*1000000) % 1000000;
            FD_ZERO(&fds);
            FD_SET(ctx->socket, &fds);
            count = select(ctx->socket + 1, NULL, &fds, NULL, &tv);

            if (count <= 0 || getsockopt(ctx->socket, SOL_SOCKET, SO_ERROR, &err, &optLen) < 0 || err) {
                if (count == 0) err = ETIMEDOUT;
                if (count < 0 || ! err) err = errno;
                r = 0;
            }
        }
        else if (err && domain == PF_INET6 && ETHEREUM_BOOLEAN_IS_TRUE(_isAddressIPv4(ctx->address))) {
            return _openSocket(ctx, PF_INET, timeout, error); // fallback to IPv4
        }
        else if (err) {
            r = 0;
         }

        if (r) {
            printf("Node discovery socket connected\n");
        }
        fcntl(ctx->socket, F_SETFL, arg); // restore socket non-blocking status
    }

    if (! r && err) {
         printf("ethereum connect error: %s", strerror(err));
    }
    if (error && err ){
        *error = err;
    }
    return r;
    return 0;
}
*/
static BRRlpItem _encodeEndpoint(BRRlpCoder coder, BREthereumLESEndpoint endpoint) {

    BRRlpItem items[3];
    if(endpoint->addr_family == AF_INET) {
        items[0] = rlpEncodeItemBytes(coder, endpoint->u.ipv4.address_bytes, 4);
    }else {
        items[0] = rlpEncodeItemBytes(coder, endpoint->u.ipv6.address_bytes, 16);
    }
    items[1] = rlpEncodeItemUInt64(coder, endpoint->udpPort, 0);
    items[2] = rlpEncodeItemUInt64(coder, endpoint->tcpPort, 0);
    
    return rlpEncodeListItems(coder, items, 3);
}

static int _readPacket(int sock, BREthereumLESEndpoint endpoint, uint8_t* bytes, size_t* bytesCount) {

    ssize_t n = 0;
    int error = 0;
    //struct sockaddr_storage destination;
    socklen_t destLen;
    *bytesCount = 0;
    
    if (sock < 0) error = ENOTCONN;

    n = recvfrom(sock, bytes, 1024, 0, NULL, &destLen);
    
    if(n < 0){
       error = errno;
       eth_log(LOG_TOPIC, "Error reading in packet data %s", strerror(error));
    }
    if (n > 0) {
        eth_log(LOG_TOPIC, "received %zd bytes from [%s]", n, endpoint->hostname);
       *bytesCount = n;
    }
    
    return error;

}
static int _sendPacket(int socket, BREthereumLESEndpoint endpoint, uint8_t packetType, uint8_t* packetData, size_t packetDataSize) {

    ssize_t n = 0;
    struct sockaddr* destination;
    socklen_t destLen = endpoint->addrSize;
    int error = 0;
    
    if(endpoint->addr_family == AF_INET){
        destination =(struct sockaddr *)&(endpoint->u.ipv4.addr);
    }else {
        destination =(struct sockaddr *)&(endpoint->u.ipv6.addr);
    }
    
    n = sendto(socket, packetData, packetDataSize, 0,destination, destLen);
    
    if (socket < 0) {
        error = ENOTCONN;
    }
    
    if (n < 0 && errno != EWOULDBLOCK){
        error = errno;
        eth_log(LOG_TOPIC,"error sending packet(%d):%s", packetType, strerror(error));
    } else {
        eth_log(LOG_TOPIC,"sent packet (%d) to:[%s]", packetType, endpoint->hostname);
    }
    
    return error;
}

static int _decodePong(uint8_t*packet, size_t packetSize, BREthereumLESPongNode pongNode, BRKey* remoteKey) {

    uint8_t* hashPtr       = packet;
    uint8_t* signaturePtr  = &packet[HASH_BYTES_SIZE];
    uint8_t* packetTypePtr = &packet[HASH_BYTES_SIZE + SIGNATURE_BYTES_SIZE];
    //uint8_t* packetDataPtr = &packet[HASH_BYTES_SIZE + SIGNATURE_BYTES_SIZE + PACKET_TYPE_BYTES_SIZE];

    //verify the hash
    UInt256 digest;
    BRKeccak256(digest.u8, hashPtr, HASH_BYTES_SIZE);
    if(memcmp(digest.u8, hashPtr, HASH_BYTES_SIZE)) {
        printf("Node Discovery error (_decodePong): First 32 bytes are not Keccak256");
        return -1;
    }
    
    //Recover Publickey from the signature
    BRKeccak256(digest.u8, signaturePtr, SIGNATURE_BYTES_SIZE);
    if(!BRKeyRecoverPubKey(remoteKey, digest, signaturePtr, SIGNATURE_BYTES_SIZE)) {
        printf("Node Discovery error (_decodePont): Could not recover public key from signature");
        return -1;
    }
    
    //Check to make sure we got a pong packet
    if(*packetTypePtr != 0x02) {
        printf("Node Discovery error (_decodePong): Received the wrong packet type, got: %d", *packetTypePtr);
        return -1;
    }
    
    //Decode the pong message
   
    
    printf("Successfuklly decoded the Pong Message");
    
    return 0; 
}
static int _generateAndSendPacket(int sock, BREthereumLESEndpoint to, BRKey* key, uint8_t packetType, uint8_t* packetData, size_t packetDataSize){

    int ec;
    uint8_t* packet;
    size_t packetSize = HASH_BYTES_SIZE + SIGNATURE_BYTES_SIZE + PACKET_TYPE_BYTES_SIZE + packetDataSize; //sizeof(hash) + sizeof(signature) + sizeof(packet-type) + sizeof(packet-data)
    
    //signature: sign(privkey, sha3(packet-type || packet-data))
    size_t compactSignSize = PACKET_TYPE_BYTES_SIZE + packetDataSize; // packet_type_size(1 byte) + packet_data_size;
    uint8_t* compactSign;
    array_new(compactSign, compactSignSize);
    array_add(compactSign, 0x01);
    array_add_array(compactSign, packetData, packetDataSize);
    UInt256 compactSignDigest;
    BRKeccak256(compactSignDigest.u8, compactSign, compactSignSize);


   // uint8_t* data = calloc(32, sizeof(uint8_t));
    
  //  BRKeccak256(data, compactSign, compactSignSize);

    printf("\nCompact(%d)*********\n", 32);
    for(int i = 0; i < 32; ++i){
        printf("%02x ", compactSignDigest.u8[i]);
    }
    printf("\n");

    // Determine the signature length
    size_t signatureLen = BRKeyCompactSignEthereum(key,
                                           NULL, 0,
                                           compactSignDigest);

    // Fill the signature
    uint8_t signature[signatureLen];
    signatureLen = BRKeyCompactSignEthereum(key,
                                    signature, signatureLen,
                                    compactSignDigest);
    
    printf("\nSign(%d)*********\n", 65);
    for(int i = 0; i < 65; ++i){
        printf("%02x ", signature[i]);
    }
    printf("\n");
    /*
    char hex[131];
    encodeHex (hex, 131, signature, 65);
    printf("Hex = %s", hex);
    */

    if(signatureLen != 65) {
        eth_log(LOG_TOPIC, "Error: could not sign siganture for %s", "ping");
        ec = 1;
    }
    else
    {
        // hash: sha3(signature || packet-type || packet-data)
        UInt256 hashDigest;
        uint8_t* hash;
        size_t hashSize = SIGNATURE_BYTES_SIZE + PACKET_TYPE_BYTES_SIZE + packetDataSize; // sizeof(signature) + sizeof(packet_type) + sizeof(packetData)
        array_new(hash, hashSize);
        array_add_array(hash, signature, SIGNATURE_BYTES_SIZE);
        array_add(hash, packetType);
        array_add_array(hash, packetData, packetDataSize);
        BRKeccak256(hashDigest.u8, hash, hashSize);
    
        array_new(packet, packetSize);
        array_add_array(packet, hashDigest.u8, HASH_BYTES_SIZE);
        array_add_array(packet, signature, SIGNATURE_BYTES_SIZE);
        array_add(packet, packetType);
        array_add_array(packet, packetData, packetDataSize);
        ec = _sendPacket(sock, to, packetType, packet, packetSize);
        
        array_free(packet);
        array_free(hash);
    }
    array_free(compactSign);
    return ec;
}

//
// Public Functions
//
BREthereumLESEndpoint nodeDiscoveryCreateEndpoint(int addr_family, char*address, uint16_t udpPort, uint16_t tcpPort){
    
    BREthereumLESEndpoint endpoint = (BREthereumLESEndpoint)calloc(1,sizeof(struct BREthereumLESEndpointContext));
    assert(endpoint != NULL);
    assert(address != NULL);
    endpoint->addr_family = addr_family;
    endpoint->tcpPort = tcpPort;
    endpoint->udpPort = udpPort;
    strcpy(endpoint->hostname, address);
    if(addr_family == AF_INET){
        assert(inet_pton(AF_INET, address, endpoint->u.ipv4.address_bytes) == 1);
        endpoint->u.ipv4.addr.sin_family = AF_INET;
        assert(inet_pton(AF_INET, address, &(endpoint->u.ipv4.addr.sin_addr)) > 0);    
        endpoint->u.ipv4.addr.sin_port = htons(udpPort);
        endpoint->addrSize = sizeof(struct sockaddr_in);
    }
    else {
        assert(inet_pton(AF_INET6, address, endpoint->u.ipv6.address_bytes) == 1);
        endpoint->u.ipv6.addr.sin6_family = AF_INET6;
        inet_pton(AF_INET6, address, &(endpoint->u.ipv6.addr.sin6_addr));
        endpoint->u.ipv6.addr.sin6_port =  htons(udpPort);
        endpoint->addrSize = sizeof(struct sockaddr_in6);
    }
    return endpoint;
}
BREthereumLESPingNode nodeDiscoveryCreatePing(BREthereumLESEndpoint to, BREthereumLESEndpoint from){

    BREthereumLESPingNode node = (BREthereumLESPingNode)calloc(1,sizeof(struct BREthereumLESPingNodeContext));
    node->from = from;
    node->to = to;
    return node;
}

int nodeDiscoveryPing(BRKey* nodeKey, BREthereumLESPingNode message, BREthereumLESPongNode reply, BRKey* remotePubKey) {

    int error = 0, sock;

    int ec = _openUDPSocket(message->from, message->from->addr_family, CONNECTION_TIME, &sock, &error);
    
    if(ec){
        //Encode packet data (BREthereumPingNode)
        BRRlpData data;
        BRRlpCoder coder = rlpCoderCreate();
        BRRlpItem items[4];
        int idx = 0;
        items[idx++] = rlpEncodeItemUInt64(coder, 0x03, 0);//rlpEncodeItemUInt256(coder, version,0);
        items[idx++] = _encodeEndpoint(coder,message->from);
        items[idx++] = _encodeEndpoint(coder,message->to);
        struct timeval tv;
        gettimeofday(&tv, NULL); 
        items[idx++] = rlpEncodeItemUInt64(coder,  (uint64_t)(tv.tv_sec + (double)tv.tv_usec/1000000 + 60), 0);
        //items[idx++] = rlpEncodeItemUInt64(coder, 1526904881, 0);
        BRRlpItem encoding = rlpEncodeListItems(coder, items, idx);
        rlpDataExtract(coder, encoding, &data.bytes, &data.bytesCount);
        
        ec = _generateAndSendPacket(sock, message->to, nodeKey, 0x01, data.bytes, data.bytesCount);
        if(!ec){
            size_t bufferSize = 0;
            uint8_t buffer[MAX_PAYLOAD_SIZE];
            ec = _readPacket(sock, message->to, buffer, &bufferSize);
            if(!ec){
                ec = _decodePong(buffer, bufferSize, reply, remotePubKey);
            }
        }
        //free data
        rlpDataRelease(data);
        rlpCoderRelease(coder);
    }
    return ec;
}
