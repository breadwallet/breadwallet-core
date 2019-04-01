//
//  BREthereumNodeEndpoint.h
//  Core
//
//  Created by Ed Gamble on 8/14/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Node_Endpoint_H
#define BR_Ethereum_Node_Endpoint_H

#include <limits.h>
#include "support/BRInt.h"
#include "support/BRKey.h"
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

/// MARK: - Open/Close, Send/Recv

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

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Node_Endpoint_H */
