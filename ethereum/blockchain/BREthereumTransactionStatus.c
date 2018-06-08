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

extern void
transactionStatusRelease (BREthereumTransactionStatus status) {
    switch (status.type) {
        case TRANSACTION_STATUS_ERROR:
            if (NULL != status.u.error.message)
                free (status.u.error.message);
            break;
        default:
            break;
    }
}

extern BREthereumTransactionStatus
transactionStatusRLPDecodeItem (BRRlpItem item,
                                BRRlpCoder coder) {
    BREthereumTransactionStatus status;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert (3 == itemsCount); // [type, [blockHash blockNumber, txIndex], error]

    status.type = (BREthereumTransactionStatusType) rlpDecodeItemUInt64(coder, items[0], 0);
    switch (status.type) {
        case TRANSACTION_STATUS_UNKNOWN:
        case TRANSACTION_STATUS_QUEUED:
        case TRANSACTION_STATUS_PENDING:
            // assert: [] == item[1], "" == item[2]
            break;

        case TRANSACTION_STATUS_INCLUDED: {
            size_t othersCount;
            const BRRlpItem *others = rlpDecodeList(coder, items[1], &othersCount);
            assert (3 == othersCount);

            status.u.included.blockHash = hashRlpDecode(others[0], coder);
            status.u.included.blockNumber = rlpDecodeItemUInt64(coder, others[1], 0);
            status.u.included.transactionIndex = rlpDecodeItemUInt64(coder, others[2], 0);
            break;
        }
        
        case TRANSACTION_STATUS_ERROR:
            status.u.error.message = rlpDecodeItemString(coder, items[2]);
            break;

        case TRANSACTION_STATUS_CREATED:
        case TRANSACTION_STATUS_SIGNED:
        case TRANSACTION_STATUS_SUBMITTED:
            // Never here - these three are additions, not part of LES txStatus
            break;
    }

    return status;
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
