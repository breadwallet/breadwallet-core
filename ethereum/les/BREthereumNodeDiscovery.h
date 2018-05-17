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

#ifndef BR_Ethereum_Node_Discovery_h
#define BR_Ethereum_Node_Discovery_h

#include "BRKey.h"
#include "BRInt.h"
#include "BREthereumLESBase.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BRE_PACKET_TYPE_PING = 0x01,
    BRE_PACKET_TYPE_PONG = 0x02,
    BRE_PACKET_TYPE_FIND_NEIGHBORS = 0x03,
    BRE_PACKET_TYPE_NEIGHBORS = 0x04
}BREthereumDiscoveryPacketType;

typedef struct {
    UInt128 address; // BE encoded 4-byte or 16-byte address (size determines ipv4 vs ipv6)
    uint16_t udpPort; // BE encoded 16-bit unsigned
    uint16_t tcpPort; // BE encoded 16-bit unsigned
}BREthereumEndpoint;

typedef struct
{
    UInt256 version; // This should always be 0x3
    BREthereumEndpoint from;
    BREthereumEndpoint to;
    uint32_t timestamp;
}BREthereumPingNode;

typedef struct
{
    BREthereumEndpoint to;
    UInt256 echo;
    uint32_t timestamp;
}BREthereumPongNode;

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


/**
 * Send a Ping Packet
 */
extern int ethereumNodeDiscoveryPing(BRKey* nodeKey, BREthereumPingNode message, BREthereumPongNode* reply, BRKey* remoteId);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Node_Discovery_h */
