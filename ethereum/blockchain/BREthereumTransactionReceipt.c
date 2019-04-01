//
//  BREthereumTransactionReceipt.c
//  BRCore
//
//  Created by Ed Gamble on 5/10/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <string.h>
#include <assert.h>
#include "support/BRArray.h"
#include "BREthereumLog.h"
#include "BREthereumTransactionReceipt.h"

//
// Transaction Receipt
//
struct BREthereumTransactionReceiptRecord {
    /**
     * the cumulative gas used in the block containing the transaction receipt as of
     * immediately after the transaction has happened, Ru,
     */
    uint64_t gasUsed;

    /**
     * the set of logs created through execution of the transaction, Rl
     */
    BREthereumLog *logs;

    /**
     * the Bloom filter composed from information in those logs, Rb
     */
    BREthereumBloomFilter bloomFilter;

    /**
     * and the status code of the transaction, Rz
     */
    BRRlpData stateRoot;
};

extern uint64_t
transactionReceiptGetGasUsed (BREthereumTransactionReceipt receipt) {
    return receipt->gasUsed;
}

extern size_t
transactionReceiptGetLogsCount (BREthereumTransactionReceipt receipt) {
    return array_count(receipt->logs);
}

extern BREthereumLog
transactionReceiptGetLog (BREthereumTransactionReceipt receipt, size_t index) {
    return (index < array_count(receipt->logs)
            ? receipt->logs[index]
            : NULL);
}

extern BREthereumBloomFilter
transactionReceiptGetBloomFilter (BREthereumTransactionReceipt receipt) {
    return receipt->bloomFilter;
}

//
// Bloom Filter Matches
//
extern BREthereumBoolean
transactionReceiptMatch (BREthereumTransactionReceipt receipt,
                         BREthereumBloomFilter filter) {
    return bloomFilterMatch(receipt->bloomFilter, filter);
}

extern BREthereumBoolean
transactionReceiptMatchAddress (BREthereumTransactionReceipt receipt,
                                BREthereumAddress address) {
    return transactionReceiptMatch(receipt, logTopicGetBloomFilterAddress(address));
}

extern void
transactionReceiptRelease (BREthereumTransactionReceipt receipt) {
    if (NULL != receipt) {
        for (size_t index = 0; index < array_count(receipt->logs); index++)
            logRelease(receipt->logs[index]);
        array_free(receipt->logs);
        rlpDataRelease(receipt->stateRoot);
        free (receipt);
    }
}

//
// Transaction Receipt Logs - RLP Encode/Decode
//
static BRRlpItem
transactionReceiptLogsRlpEncode (BREthereumTransactionReceipt log,
                                 BRRlpCoder coder) {
    size_t itemsCount = array_count(log->logs);
    BRRlpItem items[itemsCount];
    
    for (int i = 0; i < itemsCount; i++)
        items[i] = logRlpEncode(log->logs[i], RLP_TYPE_NETWORK, coder);
    
    return rlpEncodeListItems(coder, items, itemsCount);
}

static BREthereumLog *
transactionReceiptLogsRlpDecode (BRRlpItem item,
                                 BRRlpCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    BREthereumLog *logs;
    array_new(logs, itemsCount);

    for (int i = 0; i < itemsCount; i++) {
        BREthereumLog log = logRlpDecode(items[i], RLP_TYPE_NETWORK, coder);
        array_add(logs, log);
    }

    return logs;
}

//
// Transaction Receipt - RLP Decode
//
extern BREthereumTransactionReceipt
transactionReceiptRlpDecode (BRRlpItem item,
                             BRRlpCoder coder) {
    BREthereumTransactionReceipt receipt = calloc (1, sizeof(struct BREthereumTransactionReceiptRecord));
    memset (receipt, 0, sizeof(struct BREthereumTransactionReceiptRecord));
    
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert (4 == itemsCount);
    
    receipt->stateRoot = rlpDecodeBytes(coder, items[0]);
    receipt->gasUsed = rlpDecodeUInt64(coder, items[1], 0);
    receipt->bloomFilter = bloomFilterRlpDecode(items[2], coder);
    receipt->logs = transactionReceiptLogsRlpDecode(items[3], coder);
    
    return receipt;
}

//
// Transaction Receipt - RLP Encode
//
extern BRRlpItem
transactionReceiptRlpEncode(BREthereumTransactionReceipt receipt,
                            BRRlpCoder coder) {
    BRRlpItem items[4];
    
    items[0] = rlpEncodeBytes(coder, receipt->stateRoot.bytes, receipt->stateRoot.bytesCount);
    items[1] = rlpEncodeUInt64(coder, receipt->gasUsed, 0);
    items[2] = bloomFilterRlpEncode(receipt->bloomFilter, coder);
    items[3] = transactionReceiptLogsRlpEncode(receipt, coder);
    
    return rlpEncodeListItems(coder, items, 4);
}

extern BRArrayOf (BREthereumTransactionReceipt)
transactionReceiptDecodeList (BRRlpItem item,
                              BRRlpCoder coder) {
    size_t itemCount;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemCount);

    BRArrayOf (BREthereumTransactionReceipt) receipts;
    array_new (receipts, itemCount);
    for (size_t index = 0; index < itemCount; index++)
        array_add (receipts, transactionReceiptRlpDecode (items[index], coder));
    return receipts;
}

extern void
transactionReceiptsRelease (BRArrayOf(BREthereumTransactionReceipt) receipts) {
    if (NULL != receipts) {
        size_t count = array_count(receipts);
        for (size_t index = 0; index < count; index++)
            transactionReceiptRelease (receipts[index]);
        array_free (receipts);
    }
}

/*  Transaction Receipts (184)
 ETH: LES-RECEIPTS:     L184: [
 ETH: LES-RECEIPTS:       L  4: [
 ETH: LES-RECEIPTS:         I  1: 0x01
 ETH: LES-RECEIPTS:         I  2: 0x5208
 ETH: LES-RECEIPTS:         I256: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
 ETH: LES-RECEIPTS:         L  0: []
 ETH: LES-RECEIPTS:       ]
 ETH: LES-RECEIPTS:       L  4: [
 ETH: LES-RECEIPTS:         I  1: 0x01
 ETH: LES-RECEIPTS:         I  2: 0xa410
 ETH: LES-RECEIPTS:         I256: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
 ETH: LES-RECEIPTS:         L  0: []
 ETH: LES-RECEIPTS:       ]
 ETH: LES-RECEIPTS:       L  4: [
 ETH: LES-RECEIPTS:         I  1: 0x01
 ETH: LES-RECEIPTS:         I  3: 0x018cc3
 ETH: LES-RECEIPTS:         I256: 0x00000000000000000000000200000000000000000000000000000100000000000000000000000000000000080000000000000004000000000000000000200000000000000000000000000000000000000000000800000000000000000000000000000000000000000008000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000020004000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004000000200000000800000001010000000000000000000000000000000000000000000000000000000000000
 ETH: LES-RECEIPTS:         L  2: [
 ETH: LES-RECEIPTS:           L  3: [
 ETH: LES-RECEIPTS:             I 20: 0x96477a1c968a0e64e53b7ed01d0d6e4a311945c2
 ETH: LES-RECEIPTS:             L  3: [
 ETH: LES-RECEIPTS:               I 32: 0x8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925
 ETH: LES-RECEIPTS:               I 32: 0x0000000000000000000000005c0f318407f37029f2a2b6b29468b79fbd178f2a
 ETH: LES-RECEIPTS:               I 32: 0x000000000000000000000000642ae78fafbb8032da552d619ad43f1d81e4dd7c
 ETH: LES-RECEIPTS:             ]
 ETH: LES-RECEIPTS:             I 32: 0x00000000000000000000000000000000000000000000000006f05b59d3b20000
 ETH: LES-RECEIPTS:           ]
 ETH: LES-RECEIPTS:           L  3: [
 ETH: LES-RECEIPTS:             I 20: 0xc66ea802717bfb9833400264dd12c2bceaa34a6d
 ETH: LES-RECEIPTS:             L  3: [
 ETH: LES-RECEIPTS:               I 32: 0x8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925
 ETH: LES-RECEIPTS:               I 32: 0x0000000000000000000000005c0f318407f37029f2a2b6b29468b79fbd178f2a
 ETH: LES-RECEIPTS:               I 32: 0x000000000000000000000000642ae78fafbb8032da552d619ad43f1d81e4dd7c
 ETH: LES-RECEIPTS:             ]
 ETH: LES-RECEIPTS:             I 32: 0x00000000000000000000000000000000000000000000000006f05b59d3b20000
 ETH: LES-RECEIPTS:           ]
 ETH: LES-RECEIPTS:         ]
 ETH: LES-RECEIPTS:       ]
 */
