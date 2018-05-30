//
//  BREthereumLES.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 5/01/18.
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
#include <string.h>
#include <stdlib.h>
#include "BRRlpCoder.h"
#include "BREthereumLES.h"
#include "BREthereumBase.h"
#include "BREthereumTransaction.h"
#include "BREthereumNetwork.h"
#include "BRArray.h"
#include "BREthereumNodeManager.h"

struct BREthereumLESContext {
    BREthereumNodeManager nodeManager;
    
    BREthereumSubProtoCallbacks callbacks;
    uint64_t requestIdCount;
};



static void _receivedMessageCallback(BREthereumSubProtoContext info, uint8_t* message, size_t messageSize) {






}
static void _connectedToNetworkCallback(BREthereumSubProtoContext info) {






}
static void _networkReachableCallback(BREthereumSubProtoContext info, BREthereumBoolean isReachable) {




}




//
// Public functions
//

extern BREthereumLES
lesCreate (BREthereumNetwork network,
           BREthereumLESAnnounceContext announceContext,
           BREthereumLESAnnounceCallback announceCallback,
           BREthereumHash headHash,
           uint64_t headNumber,
           uint64_t headTotalDifficulty,
           BREthereumHash genesisHash) {
    
    BREthereumLES les = (BREthereumLES) calloc(1,sizeof(struct BREthereumLESContext));
    
    if(les != NULL)
    {
        les->callbacks.info = les;
        les->callbacks.connectedFunc = _connectedToNetworkCallback;
        les->callbacks.messageRecFunc = _receivedMessageCallback;
        les->callbacks.networkReachFunc = __networkReachableCallback;
        les->nodeManager = ethereumNodeManagerCreate(network, <#BRKey *key#>, headHash, headNumber, headTotalDifficulty, genesisHash, callbacks);

    }
    return les;
}

extern void lesConnect(BREthereumLES les) {



}

extern void lesDisconnect(BREthereumLES les) {



}

extern void lesRelease(BREthereumLES les) {

    ethereumNodeManagerRelease(les->nodeManager);
    free(les);
}
*/ 
