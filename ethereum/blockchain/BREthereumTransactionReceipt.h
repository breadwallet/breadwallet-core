//
//  BREthereumTransactionReceipt.h
//  BRCore
//
//  Created by Ed Gamble on 5/10/18.
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

#ifndef BR_Ethereum_Transaction_Receipt_h
#define BR_Ethereum_Transaction_Receipt_h

#include "ethereum/base/BREthereumBase.h"
#include "BREthereumBloomFilter.h"
#include "BREthereumLog.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Etheruem Transaction Receipt contains data pertinent to the execution of a transaction.
 *
 * As per the Ethereum specification: The transaction receipt, R, is a tuple of four items
 * comprising: {gasUsed, logs, bloomfilter, statusCode}
 *
 * [Note: there appears to be a change in interpretation for 'status code'; it is shown here
 * as stateRoot
 */
typedef struct BREthereumTransactionReceiptRecord *BREthereumTransactionReceipt;

extern uint64_t
transactionReceiptGetGasUsed (BREthereumTransactionReceipt receipt);

extern size_t
transactionReceiptGetLogsCount (BREthereumTransactionReceipt receipt);

extern BREthereumLog
transactionReceiptGetLog (BREthereumTransactionReceipt receipt, size_t index);

extern BREthereumBloomFilter
transactionReceiptGetBloomFilter (BREthereumTransactionReceipt receipt);

extern BREthereumBoolean
transactionReceiptMatch (BREthereumTransactionReceipt receipt,
                         BREthereumBloomFilter filter);

extern BREthereumBoolean
transactionReceiptMatchAddress (BREthereumTransactionReceipt receipt,
                                BREthereumAddress address);

extern BREthereumTransactionReceipt
transactionReceiptRlpDecode (BRRlpItem item,
                             BRRlpCoder coder);
    
extern BRRlpItem
transactionReceiptRlpEncode(BREthereumTransactionReceipt receipt,
                            BRRlpCoder coder);

extern BRArrayOf (BREthereumTransactionReceipt)
transactionReceiptDecodeList (BRRlpItem item,
                              BRRlpCoder coder);

extern void
transactionReceiptRelease (BREthereumTransactionReceipt receipt);

extern void
transactionReceiptsRelease (BRArrayOf(BREthereumTransactionReceipt) receipts);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Transaction_Receipt_h */
