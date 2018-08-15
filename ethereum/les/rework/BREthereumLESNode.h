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

typedef struct BREthereumLESNodeXRecord *BREthereumLESNodeX;

typedef enum {
    NODE_ERROR,
    NODE_SUCCESS,
} BREthereumLESNodeStatus;


typedef void *BREthereumLESNodeContext;

typedef void
(*BREthereumLESNodeCallbackMessage) (BREthereumLESNodeContext context,
                                     BREthereumLESNodeX node,
                                     BREthereumLESMessage message);

// connect
// disconnect
// network reachable

extern BREthereumLESNodeX // add 'message id offset'?
nodeXCreate (BREthereumLESNodeEndpoint endpoint,  // remote, local ??
             BREthereumLESNodeEndpoint local,
             BREthereumLESNodeContext context,
             BREthereumLESNodeCallbackMessage callbackMessage
// ...
);

extern void
nodeXStart (BREthereumLESNodeX node);

extern void
nodeXStop (BREthereumLESNodeX node);

extern void
nodeXRelease (BREthereumLESNodeX node);

extern void
nodeXSend (BREthereumLESNodeX node,
           BREthereumMessage message);   // BRRlpData/BRRlpItem *optionalMessageData/Item

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_Node_H */
