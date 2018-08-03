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
#include "BREthereumEndpoint.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/****
TODO: Implement Node Discovery

The following was a early/first attempt at implementing node discovery. Many of the below functions can be changed to interact
better with the current state of LES module. The following can be used as references for implementing node discovery:

1. PyEthTutorial provides a python implementation of Ping/Pong that I started implemented below. https://ocalog.com/post/10/
2. You may want to also check cpp-ethereum implementaiton of Node discovery.

*****/


/**
 * Declaration for a BREthereumPingNode
 */
typedef struct BREthereumPingNodeContext* BREthereumPingNode;


/**
 * Declaration for a BREthereumPongNode
 */
typedef struct BREthereumPongNodeContext* BREthereumPongNode;


/**
 * Creates a Ping Node
 */
extern BREthereumPingNode ethereumNodeDiscoveryCreatePing(BREthereumEndpoint to, BREthereumEndpoint from);

/**
 * Create a Endpoint Node
 */
extern BREthereumEndpoint ethereumNodeDiscoveryCreateEndpoint(int addr_family, char*address, uint16_t udpPort, uint16_t tcpPort);

/**
 * Send a Ping Packet
 */
extern int ethereumNodeDiscoveryPing(BRKey* nodeKey, BREthereumPingNode message, BREthereumPongNode node, BRKey* remotePubKey);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Node_Discovery_h */
