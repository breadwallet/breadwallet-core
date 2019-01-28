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

const char *
transactionGetErrorName (BREthereumTransactionErrorType type) {
    static const char *names[] = {
        "* invalid signature",
        "* nonce too low",
        "* balance too low",
        "* gas price too low",
        "* gas too low",
        "* replacement under-pricesd",
        "* dropped",
        "* unknown"
    };

    return names[type];
}

extern BREthereumTransactionStatus
transactionStatusCreate (BREthereumTransactionStatusType type) {
    assert (TRANSACTION_STATUS_INCLUDED != type && TRANSACTION_STATUS_ERRORED != type);
    BREthereumTransactionStatus status;
    status.type = type;
    return status;
}

extern BREthereumTransactionStatus
transactionStatusCreateIncluded (BREthereumHash blockHash,
                                 uint64_t blockNumber,
                                 uint64_t transactionIndex,
                                 uint64_t blockTimestamp,
                                 BREthereumGas gasUsed) {
    return (BREthereumTransactionStatus) {
        TRANSACTION_STATUS_INCLUDED,
        blockHash,
        blockNumber,
        transactionIndex,
        blockTimestamp,
        gasUsed
    };
}

extern BREthereumTransactionStatus
transactionStatusCreateErrored (BREthereumTransactionErrorType type,
                                const char *detail) {
    BREthereumTransactionStatus status;
    status.type = TRANSACTION_STATUS_ERRORED;
    status.u.errored.type = type;
    strlcpy (status.u.errored.detail, detail, TRANSACTION_STATUS_DETAIL_BYTES);
    return status;
}

extern int
transactionStatusExtractIncluded(const BREthereumTransactionStatus *status,
                                 BREthereumHash *blockHash,
                                 uint64_t *blockNumber,
                                 uint64_t *blockTransactionIndex,
                                 uint64_t *blockTimestamp,
                                 BREthereumGas *gas) {
    if (status->type != TRANSACTION_STATUS_INCLUDED)
        return 0;

    if (NULL != blockHash) *blockHash = status->u.included.blockHash;
    if (NULL != blockNumber) *blockNumber = status->u.included.blockNumber;
    if (NULL != blockTransactionIndex) *blockTransactionIndex = status->u.included.transactionIndex;
    if (NULL != blockTimestamp) *blockTimestamp = status->u.included.blockTimestamp;
    if (NULL != gas) *gas = status->u.included.gasUsed;


    return 1;
}

extern BREthereumBoolean
transactionStatusEqual (BREthereumTransactionStatus ts1,
                        BREthereumTransactionStatus ts2) {
    return AS_ETHEREUM_BOOLEAN(ts1.type == ts2.type &&
                               ((TRANSACTION_STATUS_INCLUDED != ts1.type && TRANSACTION_STATUS_ERRORED != ts1.type) ||
                                (TRANSACTION_STATUS_INCLUDED == ts1.type &&
                                 ETHEREUM_COMPARISON_EQ == gasCompare(ts1.u.included.gasUsed, ts2.u.included.gasUsed) &&
                                 ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(ts1.u.included.blockHash, ts2.u.included.blockHash)) &&
                                 ts1.u.included.blockNumber == ts2.u.included.blockNumber &&
                                 ts1.u.included.transactionIndex == ts2.u.included.transactionIndex) ||
                                (TRANSACTION_STATUS_ERRORED == ts1.type &&
                                 ts1.u.errored.type == ts2.u.errored.type &&
                                 0 == strcmp (ts1.u.errored.detail, ts2.u.errored.detail))));
}


extern BREthereumTransactionErrorType
lookupTransactionErrorType (const char *reasons[],
                            const char *reason) {
    for (BREthereumTransactionErrorType type = TRANSACTION_ERROR_INVALID_SIGNATURE;
         type <= TRANSACTION_ERROR_UNKNOWN;
         type++) {
        if (0 == strncmp (reason, reasons[type], strlen(reasons[type])))
            return type;
    }
    return TRANSACTION_ERROR_UNKNOWN;
}

//
// NOTE: THIS IS LES SPECIFIC
//
extern BREthereumTransactionStatus
transactionStatusRLPDecode (BRRlpItem item,
                            const char *reasons[],
                            BRRlpCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert (3 == itemsCount); // [type, [blockHash blockNumber, txIndex], error]

    // We have seen (many) cases where the `type` is `unknown` but there is an `error`.  That
    // appears to violate the LES specfication.  Anyways, if we see an `error` we'll force the
    // type to be TRANSACTION_STATUS_ERRORED.
    char *reason = rlpDecodeString(coder, items[2]);
    if (NULL != reason && 0 != strcmp (reason, "") && 0 != strcmp (reason, "0x")) {
        BREthereumTransactionErrorType type = lookupTransactionErrorType (reasons, reason);
        BREthereumTransactionStatus status = transactionStatusCreateErrored (type, reason);
        free (reason);
        return status;
    }
    if (NULL != reason) free (reason);

    BREthereumTransactionStatusType type = (BREthereumTransactionStatusType) rlpDecodeUInt64(coder, items[0], 0);
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

            return transactionStatusCreateIncluded (hashRlpDecode(others[0], coder),
                                                    rlpDecodeUInt64(coder, others[1], 0),
                                                    rlpDecodeUInt64(coder, others[2], 0),
                                                    TRANSACTION_STATUS_BLOCK_TIMESTAMP_UNKNOWN,
                                                    gasCreate(0));
        }
        
        case TRANSACTION_STATUS_ERRORED: {
            // We should not be here....
            char *reason = rlpDecodeString(coder, items[2]);
            BREthereumTransactionErrorType type = lookupTransactionErrorType (reasons, reason);
            BREthereumTransactionStatus status = transactionStatusCreateErrored (type, reason);
            free (reason);
            return status;
        }
    }
}

//
// NOTE: THIS IS LES SPECIFIC
//
extern BRRlpItem
transactionStatusRLPEncode (BREthereumTransactionStatus status,
                            BRRlpCoder coder) {
    BRRlpItem items[3];

    items[0] = rlpEncodeUInt64(coder, status.type, 0);

    switch (status.type) {
        case TRANSACTION_STATUS_UNKNOWN:
        case TRANSACTION_STATUS_QUEUED:
        case TRANSACTION_STATUS_PENDING:
            items[1] = rlpEncodeList(coder, 0);
            items[2] = rlpEncodeString(coder, "");
            break;

        case TRANSACTION_STATUS_INCLUDED:
            items[1] = rlpEncodeList(coder, 3,
                                     hashRlpEncode(status.u.included.blockHash, coder),
                                     rlpEncodeUInt64(coder, status.u.included.blockNumber, 0),
                                     rlpEncodeUInt64(coder, status.u.included.transactionIndex, 0));
            items[2] = rlpEncodeString(coder, "");

            break;

        case TRANSACTION_STATUS_ERRORED:
            items[1] = rlpEncodeList(coder, 0);
            items[2] = rlpEncodeString(coder, status.u.errored.detail);
            break;
    }

    return rlpEncodeListItems(coder, items, 3);
}

extern BRArrayOf (BREthereumTransactionStatus)
transactionStatusDecodeList (BRRlpItem item,
                             const char *reasons[],
                             BRRlpCoder coder) {
    size_t itemCount;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemCount);

    BRArrayOf (BREthereumTransactionStatus) stati;
    array_new (stati, itemCount);
    for (size_t index = 0; index < itemCount; index++)
        array_add (stati, transactionStatusRLPDecode (items[index], reasons, coder));

    return stati;
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
