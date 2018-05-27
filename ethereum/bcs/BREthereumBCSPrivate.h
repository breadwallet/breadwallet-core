//
//  BREthereumBCSPrivate.h
//  BRCore
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

#ifndef BR_Ethereum_BCS_Private_h
#define BR_Ethereum_BCS_Private_h

#include "BREthereumBCS.h"
#include "../blockchain/BREthereumBlockChain.h"
#include "../event/BREvent.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BREthereumBCSStruct {
    BREthereumAccount account;
    BREthereumAddress address;
    BREthereumBloomFilter filter;

    BREthereumBCSListener listener;
    
    BREthereumLES les;
    BREventHandler handler;

    // chain (block header)
    // orphans 
};

extern const BREventType *bcsEventTypes[];
extern const unsigned int bcsEventTypesCount;

//
// Announce
//
extern void
bcsSignalAnnounce (BREthereumBCS bcs,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   uint64_t headTotalDifficulty);

extern void
bcsHandleAnnounce (BREthereumBCS bcs,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   uint64_t headTotalDifficulty);

//
// Block Headers
//
extern void
bcsHandleBlockHeader (BREthereumBCS bcs,
                      BREthereumBlockHeader header);

extern void
bcsSignalBlockHeader (BREthereumBCS bcs,
                      BREthereumBlockHeader header);

//
// Block Bodies
//
extern void
bcsHandleBlockBodies (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transactions[],
                      BREthereumHash ommers[]);

extern void
bcsSignalBlockBodies (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transactions[],
                      BREthereumHash ommers[]);

//
// Transactions
//
extern void
bcsHandleTransaction (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transaction);

extern void
bcsSignalTransaction (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transaction);

//
// Transaction Status
//
extern void
bcsHandleTransactionStatus (BREthereumBCS bcs,
                            BREthereumHash transactionHash,
                            BREthereumTransactionStatus status);

extern void
bcsSignalTransactionStatus (BREthereumBCS bcs,
                            BREthereumHash transactionHash,
                            BREthereumTransactionStatus status);

//
// Transaction Receipt
//
extern void
bcsHandleTransactionReceipt (BREthereumBCS bcs,
                             BREthereumHash blockHash,
                             BREthereumTransactionReceipt receipt,
                             unsigned int receiptIndex);

extern void
bcsSignalTransactionReceipt (BREthereumBCS bcs,
                             BREthereumHash blockHash,
                             BREthereumTransactionReceipt receipt,
                             unsigned int receiptIndex);

//
// Logs
//
extern void
bcsHandleLog (BREthereumBCS bcs,
              BREthereumHash blockHash,
              BREthereumHash transactionHash,
              BREthereumLog log);

extern void
bcsSignalLog (BREthereumBCS bcs,
              BREthereumHash blockHash,
              BREthereumHash transactionHash,
              BREthereumLog log);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_BCS_Private_h */
