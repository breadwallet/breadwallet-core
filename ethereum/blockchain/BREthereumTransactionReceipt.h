//
//  BREthereumTransactionReceipt.h
//  BRCore
//
//  Created by Ed Gamble on 5/10/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

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
