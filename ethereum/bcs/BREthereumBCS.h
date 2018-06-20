//
//  BREthereumBCS.h
//  Core
//
//  Created by Ed Gamble on 5/24/18.
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

#ifndef BR_Ethereum_BCS_h
#define BR_Ethereum_BCS_h

#include "../base/BREthereumBase.h"
#include "../les/BREthereumLES.h"
#include "../BREthereumAccount.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREthereumBCSStruct *BREthereumBCS;

//
// BCS Listener
//
typedef void* BREthereumBCSListenerContext;

typedef void
(*BREthereumBCSListenerNonceCallback) (BREthereumBCSListenerContext context,
                                       uint64_t nonce);

typedef void
(*BREthereumBCSListenerBalanceCallback) (BREthereumBCSListenerContext context,
                                         BREthereumAmount balance);

    // Handle Orphan-ing
typedef void
(*BREthereumBCSListenerTransactionCallback) (BREthereumBCSListenerContext context,
                                             BREthereumTransaction transaction);

typedef void
(*BREthereumBCSListenerBlockchainCallback) (BREthereumBCSListenerContext context,
                                            BREthereumHash headBlockHash,
                                            uint64_t headBlockNumber,
                                            uint64_t headBlockTimestamp);

typedef struct {
    BREthereumBCSListenerContext context;
    BREthereumBCSListenerNonceCallback nonceCallback;
    BREthereumBCSListenerBalanceCallback balanceCallback;
    BREthereumBCSListenerTransactionCallback transactionCallback;
    BREthereumBCSListenerBlockchainCallback blockChainCallback;
    // ...
} BREthereumBCSListener;


/**
 * Create BCS (a 'BlockChain Slice`) providing a view of the Ethereum blockchain for `network`
 * focused on the `account` primary address.  Initialize the synchronization with the previously
 * saved `headers`.  Provide `listener` to anounce BCS 'events'.
 *
 * @parameters
 * @parameter headers - is this a BRArray; assume so for now.
 */
extern BREthereumBCS
bcsCreate (BREthereumNetwork network,
           BREthereumAccount account,
           BREthereumBlockHeader *headers,
           BREthereumBCSListener listener);

extern void
bcsStart (BREthereumBCS bcs);

extern void
bcsStop (BREthereumBCS bcs);

extern BREthereumBoolean
bcsIsStarted (BREthereumBCS bcs);
    
extern void
bcsDestroy (BREthereumBCS bcs);

// Should be unneeded
extern BREthereumLES
bcsGetLES (BREthereumBCS bcs);

extern void
bcsSync (BREthereumBCS bcs,
         uint64_t blockNumber);

extern void
bcsSendTransaction (BREthereumBCS bcs,
                    BREthereumTransaction transaction);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_BCS_h */
