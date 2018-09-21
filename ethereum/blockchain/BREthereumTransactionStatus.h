//
//  BREthereumTransactionStatus.h
//  BRCore
//
//  Created by Ed Gamble on 5/15/18.
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

#ifndef BR_Ethereum_Transaction_Status_h
#define BR_Ethereum_Transaction_Status_h

#include "../base/BREthereumBase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    // Unknown (0): transaction is unknown
    TRANSACTION_STATUS_UNKNOWN = 0,

    //Queued (1): transaction is queued (not processable yet)
    TRANSACTION_STATUS_QUEUED = 1,

    // Pending (2): transaction is pending (processable)
    TRANSACTION_STATUS_PENDING = 2,

    // Included (3): transaction is already included in the canonical chain. data contains an
    // RLP-encoded [blockHash: B_32, blockNumber: P, txIndex: P] structure.
    TRANSACTION_STATUS_INCLUDED = 3,

    // Error (4): transaction sending failed. data contains a text error message
    TRANSACTION_STATUS_ERRORED = 4,
} BREthereumTransactionStatusType;

#define TRANSACTION_STATUS_REASON_BYTES   \
    (sizeof (BREthereumGas) + sizeof (BREthereumHash) + 2 * sizeof(uint64_t))

typedef struct BREthereumTransactionStatusLESRecord {
    BREthereumTransactionStatusType type;
    union {
        struct {
            BREthereumGas gasUsed;      // Internal
            BREthereumHash blockHash;
            uint64_t blockNumber;
            uint64_t transactionIndex;
        } included;

        struct {
            char reason[TRANSACTION_STATUS_REASON_BYTES + 1];
        } errored;
    } u;
} BREthereumTransactionStatus;

extern BREthereumTransactionStatus
transactionStatusCreate (BREthereumTransactionStatusType type);

extern BREthereumTransactionStatus
transactionStatusCreateIncluded (BREthereumGas gasUsed,
                                 BREthereumHash blockHash,
                                 uint64_t blockNumber,
                                 uint64_t transactionIndex);

extern BREthereumTransactionStatus
transactionStatusCreateErrored (const char *reason);

static inline BREthereumBoolean
transactionStatusHasType (const BREthereumTransactionStatus *status,
                          BREthereumTransactionStatusType type) {
    return AS_ETHEREUM_BOOLEAN(status->type == type);
}

extern int
transactionStatusExtractIncluded(const BREthereumTransactionStatus *status,
                                 BREthereumGas *gas,
                                 BREthereumHash *blockHash,
                                 uint64_t *blockNumber,
                                 uint64_t *blockTransactionIndex);

extern BREthereumBoolean
transactionStatusEqual (BREthereumTransactionStatus ts1,
                        BREthereumTransactionStatus ts2);

extern BREthereumTransactionStatus
transactionStatusRLPDecode (BRRlpItem item,
                            BRRlpCoder coder);

extern BRRlpItem
transactionStatusRLPEncode (BREthereumTransactionStatus status,
                            BRRlpCoder coder);

extern BRArrayOf (BREthereumTransactionStatus)
transactionStatusDecodeList (BRRlpItem item,
                             BRRlpCoder coder);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Transaction_Status_h */
