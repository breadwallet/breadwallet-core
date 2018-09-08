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

static inline const char *
nodeEndpointRouteGetName (BREthereumNodeEndpointRoute route) {
    return (NODE_ROUTE_TCP == route ? "TCP" : "UDP");
}

//
// Endpoint
//
typedef struct {
    /** An optional hostname - if not provided this will be a IP addr string */
    char hostname[_POSIX_HOST_NAME_MAX + 1];

    /** The 'Discovery Endpoint' */
     BREthereumDISEndpoint dis;

     /** The socket - will be -1 if not connected */
    int sockets[NUMBER_OF_NODE_ROUTES];

    /** */
    uint64_t timestamp;

    /** Public Key (for the remote peer ) */
    BRKey key;

    /** The ephemeral public key */
    BRKey ephemeralKey;

    /** The nonce */
    UInt256 nonce;

    /** The Hello PSP message */
    BREthereumP2PMessage hello;     // BREthereumP2PMessageHello

    /** The Status LES message */
    BREthereumLESMessage status;    // BREthereumLESMessageStatus

} BREthereumNodeEndpoint;

// Use for 'local
extern BREthereumNodeEndpoint
nodeEndpointCreateDetailed (BREthereumDISEndpoint dis,
                            BRKey key,
                            BRKey ephemeralKey,
                            UInt256 nonce);

extern BREthereumNodeEndpoint
nodeEndpointCreate (BREthereumDISEndpoint dis,
                    BRKey key);

extern BREthereumNodeEndpoint
nodeEndpointCreateLocal (BREthereumLESRandomContext randomContext);

extern BREthereumNodeEndpoint
nodeEndpointCreateEnode (const char *enode);

extern void
nodeEndpointSetHello (BREthereumNodeEndpoint *endpoint,
                      BREthereumP2PMessage hello);

extern void
nodeEndpointSetStatus (BREthereumNodeEndpoint *endpoint,
                       BREthereumLESMessage status);

extern int
nodeEndpointOpen (BREthereumNodeEndpoint *endpoint,
                  BREthereumNodeEndpointRoute route);

extern int
nodeEndpointClose (BREthereumNodeEndpoint *endpoint,
                   BREthereumNodeEndpointRoute route,
                   int needShutdown);

extern int
nodeEndpointIsOpen (BREthereumNodeEndpoint *endpoint,
                    BREthereumNodeEndpointRoute route);

//extern int
//nodeEndpointHasRecvDataAvailable (BREthereumNodeEndpoint *endpoint,
//                                  fd_set *readFds);
//
//extern void
//nodeEndpointSetRecvDataAvailableFDS (BREthereumNodeEndpoint *endpoint,
//                                     fd_set *readFds);
//
//extern void
//nodeEndpointClrRecvDataAvailableFDS (BREthereumNodeEndpoint *endpoint,
//                                     fd_set *readFds);
//
//extern int
//nodeEndpointGetRecvDataAvailableFDSNum (BREthereumNodeEndpoint *endpoint);

extern int
nodeEndpointRecvData (BREthereumNodeEndpoint *endpoint,
                      BREthereumNodeEndpointRoute route,
                      uint8_t *bytes,
                      size_t *bytesCount,
                      int needBytesCount);

extern int
nodeEndpointSendData (BREthereumNodeEndpoint *endpoint,
                      BREthereumNodeEndpointRoute route,
                      uint8_t *bytes,
                      size_t bytesCount);

    extern const char *localLESEnode;
    extern const char *bootstrapLESEnodes[];
    extern size_t NUMBER_OF_NODE_ENDPOINT_SPECS;

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Node_Endpoint_H */
