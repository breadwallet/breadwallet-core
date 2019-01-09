//
//  BREthereumNodeEndpoint.h
//  Core
//
//  Created by Ed Gamble on 8/14/18.
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

#ifndef BR_Ethereum_Node_Endpoint_H
#define BR_Ethereum_Node_Endpoint_H

#include <limits.h>
#include "BRKey.h"
#include "BRInt.h"
#include "BREthereumMessage.h"
#include "BREthereumLESRandom.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NODE_ROUTE_UDP,
    NODE_ROUTE_TCP
} BREthereumNodeEndpointRoute;

#define NUMBER_OF_NODE_ROUTES  (1 + NODE_ROUTE_TCP)

#define FOR_EACH_ROUTE( route ) \
    for (BREthereumNodeEndpointRoute route = NODE_ROUTE_UDP; route <= NODE_ROUTE_TCP; route++)

static inline const char *
nodeEndpointRouteGetName (BREthereumNodeEndpointRoute route) {
    return (NODE_ROUTE_TCP == route ? "TCP" : "UDP");
}

/**
 * A Node Endpoint is an IP:PORT pair over which UDP and TCP data transfer can occur.  A Node
 * Endpoint has a socket for each route (UDP or TCP) and one can send or recv data along those
 * routes.
 *
 * A NodeEndpoint has 'DIS Neighbor' which represents the Ethereum DIScovery protocol data
 * associated with this endpoint.  That data includes the 64 byte public key which represents
 * the NodeId
 *
 * A Node Endpoint includes the 'hello' and 'status' P2P messages that are received from this
 * node during the communication handshake.  These messages include information about that node
 * that is used to assess the viability of the node for use as a provier of Light Cliet data.
 * Notably, the 'hello' message includes the protocols support by the endpoint; we require some
 * version of LES (for Geth) or PIP (for Parity).  Additionally, the 'status' message includes
 * `networkId`,  `headNum`, `serveStateSince` and others the need to be assessd.
 *
 * Node Endpoint defines one half of a communction pair; a node includes a 'local' and a 'remote'
 * endpoint.
 */
 typedef struct BREthereumNodeEndpointRecord  *BREthereumNodeEndpoint;

extern BREthereumNodeEndpoint
nodeEndpointCreate (BREthereumDISNeighbor dis);

extern BREthereumNodeEndpoint
nodeEndpointCreateLocal (BREthereumLESRandomContext randomContext);

extern BREthereumNodeEndpoint
nodeEndpointCreateEnode (const char *enode);

extern void
nodeEndpointRelease (OwnershipGiven BREthereumNodeEndpoint endpoint);

extern BREthereumHash
nodeEndpointGetHash (BREthereumNodeEndpoint endpoint);

extern const char *
nodeEndpointGetHostname (BREthereumNodeEndpoint endpoint);

extern int // remote.dis.node.portTCP)
nodeEndpointGetPort (BREthereumNodeEndpoint endpoint,
                     BREthereumNodeEndpointRoute route);

extern int
nodeEndpointGetSocket (BREthereumNodeEndpoint endpoint,
                       BREthereumNodeEndpointRoute route);

extern BREthereumDISNeighbor
nodeEndpointGetDISNeighbor (BREthereumNodeEndpoint endpoint);

extern BRKey *
nodeEndpointGetKey (BREthereumNodeEndpoint endpoint);

extern BRKey *
nodeEndpointGetEphemeralKey (BREthereumNodeEndpoint endpoint);

extern UInt256 *
nodeEndpointGetNonce (BREthereumNodeEndpoint endpoint);

extern BREthereumP2PMessageHello
nodeEndpointGetHello (const BREthereumNodeEndpoint endpoint);

extern void
nodeEndpointSetHello (BREthereumNodeEndpoint endpoint,
                      OwnershipGiven BREthereumP2PMessageHello hello);

extern void
nodeEndpointDefineHello (BREthereumNodeEndpoint endpoint,
                         const char *name,
                         OwnershipGiven BRArrayOf(BREthereumP2PCapability) capabilities);

extern void
nodeEndpointShowHello (BREthereumNodeEndpoint endpoint);

/**
 * Return TRUE if `endpoint` has a capability with `name`
 */
extern BREthereumBoolean
nodeEndpointHasHelloCapability (BREthereumNodeEndpoint endpoint,
                                const char *name,
                                uint32_t version);

/**
 * Return the first `source` capability matched by `target`
 */
extern const BREthereumP2PCapability *
nodeEndpointHasHelloMatchingCapability (BREthereumNodeEndpoint source,
                                        BREthereumNodeEndpoint target);

extern BREthereumP2PMessageStatus
nodeEndpointGetStatus (const BREthereumNodeEndpoint endpoint);

extern void
nodeEndpointSetStatus (BREthereumNodeEndpoint endpoint,
                       OwnershipGiven BREthereumP2PMessageStatus status);

extern void
nodeEndpointShowStatus (BREthereumNodeEndpoint endpoint);

///
/// MARK: Open/Close, Send/Recv
///

extern int
nodeEndpointOpen (BREthereumNodeEndpoint endpoint,
                  BREthereumNodeEndpointRoute route);

extern int
nodeEndpointClose (BREthereumNodeEndpoint endpoint,
                   BREthereumNodeEndpointRoute route,
                   int needShutdown);

extern int
nodeEndpointIsOpen (const BREthereumNodeEndpoint endpoint,
                    BREthereumNodeEndpointRoute route);

extern int
nodeEndpointRecvData (BREthereumNodeEndpoint endpoint,
                      BREthereumNodeEndpointRoute route,
                      uint8_t *bytes,
                      size_t *bytesCount,
                      int needBytesCount);

extern int
nodeEndpointSendData (BREthereumNodeEndpoint endpoint,
                      BREthereumNodeEndpointRoute route,
                      uint8_t *bytes,
                      size_t bytesCount);

// Support BRSet
extern size_t
nodeEndpointHashValue (const void *h);

// Support BRSet
extern int
nodeEndpointHashEqual (const void *h1, const void *h2);

//
// Enodes
//

// Mainnet
extern const char *bootstrapLCLEnodes[];
extern const char *bootstrapBRDEnodes[];
extern const char *bootstrapLESEnodes[];
extern const char *bootstrapParityEnodes[];
extern const char *bootstrapGethEnodes[];

extern const char **bootstrapMainnetEnodeSets[];
extern size_t NUMBER_OF_MAINNET_ENDPOINT_SETS;

// Ropsten
extern const char *bootstrapBRDEnodesTestnet[];

extern const char **bootstrapTestnetEnodeSets[];
extern size_t NUMBER_OF_TESTNET_ENDPOINT_SETS;

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Node_Endpoint_H */
