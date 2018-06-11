//
//  BREthereumTransactionStatus.c
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

#include <stdlib.h>
#include <assert.h>
#include "BREthereumTransactionStatus.h"

extern BREthereumTransactionStatus
transactionStatusCreate (BREthereumTransactionStatusType type) {
    assert (TRANSACTION_STATUS_INCLUDED != type && TRANSACTION_STATUS_ERRORED != type);
    BREthereumTransactionStatus status;
    status.type = type;
    return status;
}

extern BREthereumTransactionStatus
transactionStatusCreateIncluded (BREthereumGas gasUsed,
                                 BREthereumHash blockHash,
                                 uint64_t blockNumber,
                                 uint64_t blockTransactionIndex) {
    BREthereumTransactionStatus status;
    status.type = TRANSACTION_STATUS_INCLUDED;
    status.u.included.gasUsed = gasUsed;
    status.u.included.blockHash = blockHash;
    status.u.included.blockNumber = blockNumber;
    status.u.included.transactionIndex = blockTransactionIndex;
    return status;
}

extern BREthereumTransactionStatus
transactionStatusCreateErrored (const char *reason) {
    BREthereumTransactionStatus status;
    status.type = TRANSACTION_STATUS_ERRORED;
    strlcpy (status.u.errored.reason, reason, TRANSACTION_STATUS_REASON_BYTES);
    return status;
}


extern BREthereumTransactionStatus
transactionStatusRLPDecodeItem (BRRlpItem item,
                                BRRlpCoder coder) {
    BREthereumTransactionStatusType type;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert (3 == itemsCount); // [type, [blockHash blockNumber, txIndex], error]

    type = (BREthereumTransactionStatusType) rlpDecodeItemUInt64(coder, items[0], 0);
    switch (type) {
        case TRANSACTION_STATUS_UNKNOWN:
        case TRANSACTION_STATUS_QUEUED:
        case TRANSACTION_STATUS_PENDING:
            // assert: [] == item[1], "" == item[2]
            return transactionStatusCreate(type);

        case TRANSACTION_STATUS_INCLUDED: {
            size_t othersCount;
            const BRRlpItem *others = rlpDecodeList(coder, items[1], &othersCount);
            assert (3 == othersCount);

            return transactionStatusCreateIncluded(gasCreate(0),
                                                   hashRlpDecode(others[0], coder),
                                                   rlpDecodeItemUInt64(coder, others[1], 0),
                                                   rlpDecodeItemUInt64(coder, others[2], 0));
        }
        
        case TRANSACTION_STATUS_ERRORED: {
            char *reason = rlpDecodeItemString(coder, items[2]);
            BREthereumTransactionStatus status = transactionStatusCreateErrored(reason);
            free (reason);
            return status;
        }

        case TRANSACTION_STATUS_CREATED:
        case TRANSACTION_STATUS_SIGNED:
        case TRANSACTION_STATUS_SUBMITTED:
            // Never here - these three are additions, not part of LES txStatus
            return transactionStatusCreate(type);
    }
}

/* GETH TxStatus
 ETH: TxtStatus: L  3: [
 ETH: TxtStatus:   I  0: 0x
 ETH: TxtStatus:   I  4: 0x11e19aa2
 ETH: TxtStatus:   L  1: [
 ETH: TxtStatus:     L  3: [
 ETH: TxtStatus:       I  0: 0x         # status: unknown (0)
 ETH: TxtStatus:       L  0: []         # [blockHash, blockNumber, transactionIndex]: []
 ETH: TxtStatus:       I  0: 0x         # error: ""
 ETH: TxtStatus:     ]
 ETH: TxtStatus:   ]
 ETH: TxtStatus: ]
 */
