//
//  BREthereumLESNode.h
//  Core
//
//  Created by Ed Gamble on 8/13/18.
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

#ifndef BR_Ethereum_LES_Node_H
#define BR_Ethereum_LES_Node_H

#include "BREthereumLESMessage.h"
#include "BREthereumLESNodeEndpoint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREthereumLESNodeRecord *BREthereumLESNode;

typedef enum {
    NODE_ERROR,
    NODE_SUCCESS,
} BREthereumLESNodeStatus;


typedef void *BREthereumLESNodeContext;

typedef void
(*BREthereumLESNodeCallbackMessage) (BREthereumLESNodeContext context,
                                     BREthereumLESNode node,
                                     BREthereumLESMessage message);

typedef void
(*BREthereumLESNodeCallbackConnect) (BREthereumLESNodeContext context,
                                     BREthereumLESNode node,
                                     BREthereumLESNodeStatus status);

// connect
// disconnect
// network reachable

extern BREthereumLESNode // add 'message id offset'?
nodeCreate (BREthereumNetwork network,
            BREthereumLESNodeEndpoint remote,  // remote, local ??
            BREthereumLESNodeEndpoint local,
            BREthereumLESNodeContext context,
            BREthereumLESNodeCallbackMessage callbackMessage,
            BREthereumLESNodeCallbackConnect callbackConnect);

extern void
nodeRelease (BREthereumLESNode node);

extern BREthereumLESNodeEndpoint *
nodeGetRemoteEndpoint (BREthereumLESNode node);

extern BREthereumLESNodeEndpoint *
nodeGetLocalEndpoint (BREthereumLESNode node);

extern void
nodeConnect (BREthereumLESNode node,
             BREthereumLESNodeEndpointRoute route);

extern void
nodeDisconnect (BREthereumLESNode node,
                BREthereumLESNodeEndpointRoute route);

extern int
nodeIsConnected (BREthereumLESNode node,
                 BREthereumLESNodeEndpointRoute route);

extern int
nodeIsConnecting (BREthereumLESNode node,
                  BREthereumLESNodeEndpointRoute route);

extern int
nodeUpdateDescriptors (BREthereumLESNode node,
                       fd_set *read,
                       fd_set *write);

extern int
nodeCanProcess (BREthereumLESNode node,
                BREthereumLESNodeEndpointRoute route,
                fd_set *descriptors);

extern void
nodeSend (BREthereumLESNode node,
          BREthereumLESNodeEndpointRoute route,
          BREthereumMessage message);   // BRRlpData/BRRlpItem *optionalMessageData/Item

extern BREthereumMessage
nodeRecv (BREthereumLESNode node,
          BREthereumLESNodeEndpointRoute route);

extern uint64_t
nodeEstimateCredits (BREthereumLESNode node,
                     BREthereumMessage message);

extern uint64_t
nodeGetCredits (BREthereumLESNode node);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_Node_H */
