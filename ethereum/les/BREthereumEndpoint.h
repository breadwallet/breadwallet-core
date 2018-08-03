//
//  BREthereumEndpoint.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 5/23/18.
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

#ifndef BR_Ethereum_Endpoint_h
#define BR_Ethereum_Endpoint_h

#include "BRKey.h"
#include "BRInt.h"
#include "BREthereumLESBase.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declaration for a BREthereumEndpoint
 */
typedef struct BREthereumEndpointContext* BREthereumEndpoint;


/**
 * Create a BREthereumEndpoint
 * @param isIPV4Address - determines whether the endpoint address is IPV4 (ETHEREUM_BOOLEAN_TRUE), otherwise its IPV6 (ETHEREUM_BOOLEAN_FALSE)
 * @param address - a string representing the "." address for the endpoint
 * @param udpPort - the udp port for the endpoint
 * @param tcpPort - the tcp port for the endpoint
 * @return BREthereumEndpoint
 */
extern BREthereumEndpoint ethereumEndpointCreate(BREthereumBoolean isIPV4Address, char*address, uint16_t udpPort, uint16_t tcpPort);

/**
 * Releases the BREthereumEndpoint context
 * @param endponint - the endpoint context
 */
extern void ethereumEndpointRelease(BREthereumEndpoint endpoint);

/**
 * Retrieve the tcpPort of an endpoint
 * @param endponint - the endpoint context
 */
extern uint16_t ethereumEndpointGetTCP(BREthereumEndpoint endpoint);

/**
 * Retrieve the udport of an endpoint
 * @param endponint - the endpoint context
 */
extern uint16_t ethereumEndpointGetUDP(BREthereumEndpoint endpoint);

/**
 * Retrieve the host address of an endpoint
 * @param endponint - the endpoint context
 */
extern const char* ethereumEndpointGetHost(BREthereumEndpoint endpoint);

/**
 * Determines whether the endpoint contains a IPV4 address or not
 * @param endponint - the endpoint context
 */
extern BREthereumBoolean ethereumEndpointIsIPV4(BREthereumEndpoint endpoint);



#ifdef __cplusplus
}
#endif

#endif /* BREthereumEndpoint_h */
