//
//  BREthereumLog.c
//  BRCore
//
//  Created by Ed Gamble on 5/10/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "support/BRArray.h"
#include "BREthereumLog.h"

/**
 * A Log *cannot* be identified by its associated transaction (hash) - because one transaction
 * can result in multiple Logs (even identical Logs: address, topics, etc).
 *
 * Imagine a Block that includes a Log of interest is announced and chained.  The Log is 'included'.
 * Later another Block arrives with the same Log - the original Log is now 'pending' and the new
 * Log is 'included'.   How do we know that the two Logs are identical?  If we can't tell, then
 * two will be reported to the User - one as included, one as pending - when instead the pending
 * one, being identical, should just be reported as included.
 *
 * We have the same issue with transactions.  When a transaction is pending and a new block is
 * announced we search the pending transactions for a matching hash - if found we update the
 * transation to included.
 *
 * Referring to the Ethereum Yellow Paper, it appears that the only way to disambiguate Logs is
 * using the pair {Transaction-Hash, Receipt-Index}.  [One asumption here is that a given
 * transaction's contract execution must produced Logs is an deterministic order.]
 *
 * General Note: We only see Logs when they are included in a Block.  For every Log we thus know:
 * Block (hash, number, ...), TransactionHash, TransactionIndex, ReceiptIndex.  The 'same' Log
 * my have a different Block and TransactionIndex.
 */
static BREthereumLogTopic empty;

//
// Log Topic
//
extern BREthereumLogTopic
logTopicCreateFromString (const char *string) {
    if (NULL == string) return empty;

    // Ensure
    size_t stringLen = strlen(string);
    assert (0 == strncmp (string, "0x", 2) && (2 + 2 * LOG_TOPIC_BYTES_COUNT == stringLen));

    BREthereumLogTopic topic;
    decodeHex(topic.bytes, sizeof(BREthereumLogTopic), &string[2], stringLen - 2);
    return topic;
}

static BREthereumLogTopic
logTopicCreateAddress (BREthereumAddress raw) {
    BREthereumLogTopic topic = empty;
    unsigned int addressBytes = sizeof (raw.bytes);
    unsigned int topicBytes = sizeof (topic.bytes);
    assert (topicBytes >= addressBytes);

    memcpy (&topic.bytes[topicBytes - addressBytes], raw.bytes, addressBytes);
    return topic;
}

extern BREthereumBloomFilter
logTopicGetBloomFilter (BREthereumLogTopic topic) {
    BRRlpData data;
    data.bytes = topic.bytes;
    data.bytesCount = sizeof (topic.bytes);
    return bloomFilterCreateData(data);
}

extern BREthereumBloomFilter
logTopicGetBloomFilterAddress (BREthereumAddress address) {
    return logTopicGetBloomFilter (logTopicCreateAddress(address));
}

static int
logTopicMatchesAddressBool (BREthereumLogTopic topic,
                        BREthereumAddress address) {
    return (0 == memcmp (&topic.bytes[0], &empty.bytes[0], 12) &&
            0 == memcmp (&topic.bytes[12], &address.bytes[0], 20));
}

extern BREthereumBoolean
logTopicMatchesAddress (BREthereumLogTopic topic,
                        BREthereumAddress address) {
    return AS_ETHEREUM_BOOLEAN(logTopicMatchesAddressBool(topic, address));
}

extern BREthereumLogTopicString
logTopicAsString (BREthereumLogTopic topic) {
    BREthereumLogTopicString string;
    string.chars[0] = '0';
    string.chars[1] = 'x';
    encodeHex(&string.chars[2], 65, topic.bytes, 32);
    return string;
}

extern BREthereumAddress
logTopicAsAddress (BREthereumLogTopic topic) {
    BREthereumAddress address;
    memcpy (address.bytes, &topic.bytes[12], 20);
    return address;
}

//
// Support
//
static BREthereumLogTopic
logTopicRlpDecode (BRRlpItem item,
                       BRRlpCoder coder) {
    BREthereumLogTopic topic;

    BRRlpData data = rlpDecodeBytes(coder, item);
    assert (32 == data.bytesCount);

    memcpy (topic.bytes, data.bytes, 32);
    rlpDataRelease(data);

    return topic;
}

static BRRlpItem
logTopicRlpEncode(BREthereumLogTopic topic,
                      BRRlpCoder coder) {
    return rlpEncodeBytes(coder, topic.bytes, 32);
}

static BREthereumLogTopic emptyTopic;

static BRArrayOf(BREthereumLogTopic)
logTopicsCopy (BRArrayOf(BREthereumLogTopic) topics) {
    BRArrayOf(BREthereumLogTopic) copy;
    array_new(copy, array_count(topics));
    array_add_array(copy, topics, array_count(topics));
    return copy;
}

//
// Log
//
struct BREthereumLogRecord {
    // THIS MUST BE FIRST to support BRSet operations.

    /**
     * The hash - computed from the identifier pair {Transaction-Hash, Receipt-Index} using
     * BREthereumLogStatus
     */
    BREthereumHash hash;

    /**
     * a tuple of the logger’s address, Oa;
     */
    BREthereumAddress address;

    /**
     * a series of 32-byte log topics, Ot;
     */
    BRArrayOf(BREthereumLogTopic) topics;

    /**
     * and some number of bytes of data, Od
     */
    BRRlpData data;

    /**
     * A unique identifer - derived from the transactionHash and the transactionReceiptIndex
     */
    struct {
        /**
         * The hash of the transaction producing this log.  This value *does not* depend on
         * which block records the Log.
         */
        BREthereumHash transactionHash;

        /**
         * The receipt index from the transaction's contract execution for this log.  It can't
         * possibly be the case that this number varies, can it - contract execution, regarding
         * event generating, must be deterministic?
         */
        size_t transactionReceiptIndex;
    } identifier;

    /**
     * status
     */
    BREthereumTransactionStatus status;
};

extern BREthereumLog
logCreate (BREthereumAddress address,
           unsigned int topicsCount,
           BREthereumLogTopic *topics,
           BRRlpData data) {
    BREthereumLog log = calloc (1, sizeof(struct BREthereumLogRecord));

    log->hash = hashCreateEmpty();
    log->address = address;

    array_new(log->topics, topicsCount);
    for (size_t index = 0; index < topicsCount; index++)
        array_add (log->topics, topics[index]);

    // RLP Decoded (see below) performs: log->data = rlpDecodeBytes(coder, items[2]);
    // We'll assume `data` has the proper form....

    log->data = rlpDataCopy(data);

    // Mark the `identifier` as unknown.
    log->identifier.transactionReceiptIndex = LOG_TRANSACTION_RECEIPT_INDEX_UNKNOWN;

    return log;
}

extern void
logInitializeIdentifier (BREthereumLog log,
                     BREthereumHash transactionHash,
                     size_t transactionReceiptIndex) {
    log->identifier.transactionHash = transactionHash;
    log->identifier.transactionReceiptIndex = transactionReceiptIndex;

    BRRlpData data = { sizeof (log->identifier), (uint8_t*) &log->identifier };
    log->hash = hashCreateFromData(data);
}

extern BREthereumBoolean
logExtractIdentifier (BREthereumLog log,
                      BREthereumHash *transactionHash,
                      size_t *transactionReceiptIndex) {
    if (LOG_TRANSACTION_RECEIPT_INDEX_UNKNOWN == log->identifier.transactionReceiptIndex)
        return ETHEREUM_BOOLEAN_FALSE;

    if (NULL != transactionHash) *transactionHash = log->identifier.transactionHash;
    if (NULL != transactionReceiptIndex) *transactionReceiptIndex = log->identifier.transactionReceiptIndex;

    return ETHEREUM_BOOLEAN_TRUE;
}

static inline int
logHasStatus (BREthereumLog log,
              BREthereumTransactionStatusType type) {
    return type == log->status.type;
}

extern BREthereumComparison
logCompare (BREthereumLog l1,
            BREthereumLog l2) {

    if (  l1 == l2) return ETHEREUM_COMPARISON_EQ;
    if (NULL == l2) return ETHEREUM_COMPARISON_LT;
    if (NULL == l1) return ETHEREUM_COMPARISON_GT;

    int t1Blocked = logHasStatus(l1, TRANSACTION_STATUS_INCLUDED);
    int t2Blocked = logHasStatus(l2, TRANSACTION_STATUS_INCLUDED);

    if (t1Blocked && t2Blocked)
        return (l1->status.u.included.blockNumber < l2->status.u.included.blockNumber
                ? ETHEREUM_COMPARISON_LT
                : (l1->status.u.included.blockNumber > l2->status.u.included.blockNumber
                   ? ETHEREUM_COMPARISON_GT
                   : (l1->status.u.included.transactionIndex < l2->status.u.included.transactionIndex
                      ? ETHEREUM_COMPARISON_LT
                      : (l1->status.u.included.transactionIndex > l2->status.u.included.transactionIndex
                         ? ETHEREUM_COMPARISON_GT
                         : (l1->identifier.transactionReceiptIndex < l2->identifier.transactionReceiptIndex
                            ? ETHEREUM_COMPARISON_LT
                            : (l1->identifier.transactionReceiptIndex > l2->identifier.transactionReceiptIndex
                               ? ETHEREUM_COMPARISON_GT
                               : ETHEREUM_COMPARISON_EQ))))));

    else if (!t1Blocked && t2Blocked)
        return ETHEREUM_COMPARISON_GT;

    else if (t1Blocked && !t2Blocked)
        return ETHEREUM_COMPARISON_LT;

    else
        return ETHEREUM_COMPARISON_EQ;

}
extern BREthereumTransactionStatus
logGetStatus (BREthereumLog log) {
    return log->status;
}

extern void
logSetStatus (BREthereumLog log,
              BREthereumTransactionStatus status) {
    log->status = status;
}

extern BREthereumHash
logGetHash (BREthereumLog log) {
    // The hash only exists when we've got an identifier; must not be referenced otherwise.
    assert (LOG_TRANSACTION_RECEIPT_INDEX_UNKNOWN != log->identifier.transactionReceiptIndex);
    return log->hash;
}

extern BREthereumAddress
logGetAddress (BREthereumLog log) {
    return log->address;
}

extern BREthereumBoolean
logHasAddress (BREthereumLog log,
               BREthereumAddress address) {
    return addressEqual(log->address, address);
}

extern size_t
logGetTopicsCount (BREthereumLog log) {
    return array_count(log->topics);
}

extern  BREthereumLogTopic
logGetTopic (BREthereumLog log, size_t index) {
    return (index < array_count(log->topics)
            ? log->topics[index]
            : emptyTopic);
}

extern BRRlpData
logGetData (BREthereumLog log) {
    return rlpDataCopy(log->data);
}

extern BRRlpData
logGetDataShared (BREthereumLog log) {
    return log->data;
}

extern BREthereumBoolean
logMatchesAddress (BREthereumLog log,
                   BREthereumAddress address,
                   BREthereumBoolean topicsOnly) {
    int match = 0;
    size_t count = logGetTopicsCount(log);
    for (int i = 0; i < count; i++)
        match |= logTopicMatchesAddressBool(log->topics[i], address);

    return (ETHEREUM_BOOLEAN_IS_TRUE(topicsOnly)
            ? AS_ETHEREUM_BOOLEAN(match)
            : AS_ETHEREUM_BOOLEAN(match | ETHEREUM_BOOLEAN_IS_TRUE(logHasAddress(log, address))));

}

extern BREthereumBoolean
logIsConfirmed (BREthereumLog log) {
    return AS_ETHEREUM_BOOLEAN(TRANSACTION_STATUS_INCLUDED == log->status.type);
}

extern BREthereumBoolean
logIsErrored (BREthereumLog log) {
    return AS_ETHEREUM_BOOLEAN(TRANSACTION_STATUS_ERRORED == log->status.type);
}

// Support BRSet
extern size_t
logHashValue (const void *l) {
    assert (LOG_TRANSACTION_RECEIPT_INDEX_UNKNOWN != ((BREthereumLog) l)->identifier.transactionReceiptIndex);
    return hashSetValue(&((BREthereumLog) l)->hash);
}

// Support BRSet
extern int
logHashEqual (const void *l1, const void *l2) {
    if (l1 == l2) return 1;

    assert (LOG_TRANSACTION_RECEIPT_INDEX_UNKNOWN != ((BREthereumLog) l1)->identifier.transactionReceiptIndex);
    assert (LOG_TRANSACTION_RECEIPT_INDEX_UNKNOWN != ((BREthereumLog) l2)->identifier.transactionReceiptIndex);
    return hashSetEqual (&((BREthereumLog) l1)->hash,
                         &((BREthereumLog) l2)->hash);
}

/// MARK: - Release // Copy

extern void
logRelease (BREthereumLog log) {
    if (NULL != log) {
        array_free(log->topics);
        rlpDataRelease(log->data);
        free (log);
    }
}

extern void
logsRelease (BRArrayOf(BREthereumLog) logs) {
    if (NULL != logs) {
        size_t count = array_count(logs);
        for (size_t index = 0; index < count; index++)
            logRelease (logs[index]);
        array_free(logs);
    }
}

extern void
logReleaseForSet (void *ignore, void *item) {
    logRelease((BREthereumLog) item);
}

extern BREthereumLog
logCopy (BREthereumLog log) {
    BREthereumLog copy = calloc (1, sizeof(struct BREthereumLogRecord));
    memcpy (copy, log, sizeof(struct BREthereumLogRecord));

    // Copy the topics
    copy->topics = logTopicsCopy(log->topics);

    // Copy the data
    copy->data = rlpDataCopy(log->data);

    return copy;
}

/// MARK: - RLP Encode/Decode

static BRRlpItem
logTopicsRlpEncode (BREthereumLog log,
                    BRRlpCoder coder) {
    size_t itemsCount = array_count(log->topics);
    BRRlpItem items[itemsCount];

    for (int i = 0; i < itemsCount; i++)
        items[i] = logTopicRlpEncode(log->topics[i], coder);

    return rlpEncodeListItems(coder, items, itemsCount);
}

static BREthereumLogTopic *
logTopicsRlpDecode (BRRlpItem item,
                    BRRlpCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    BREthereumLogTopic *topics;
    array_new(topics, itemsCount);

    for (int i = 0; i < itemsCount; i++) {
        BREthereumLogTopic topic = logTopicRlpDecode(items[i], coder);
        array_add(topics, topic);
    }

    return topics;
}

extern BREthereumLog
logRlpDecode (BRRlpItem item,
              BREthereumRlpType type,
              BRRlpCoder coder) {
    BREthereumLog log = (BREthereumLog) calloc (1, sizeof (struct BREthereumLogRecord));

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert ((3 == itemsCount && RLP_TYPE_NETWORK == type) ||
            (6 == itemsCount && RLP_TYPE_ARCHIVE == type));

    log->address = addressRlpDecode(items[0], coder);
    log->topics = logTopicsRlpDecode (items[1], coder);

    log->data = rlpGetData (coder, items[2]); //  rlpDecodeBytes(coder, items[2]);

    // 
    log->identifier.transactionReceiptIndex = LOG_TRANSACTION_RECEIPT_INDEX_UNKNOWN;

    if (RLP_TYPE_ARCHIVE == type) {
        BREthereumHash hash = hashRlpDecode(items[3], coder);

        uint64_t transactionReceiptIndex = rlpDecodeUInt64(coder, items[4], 0);
        assert (transactionReceiptIndex <= (uint64_t) SIZE_MAX);

        logInitializeIdentifier (log, hash, (size_t) transactionReceiptIndex);
        log->status = transactionStatusRLPDecode(items[5], NULL, coder);
    }
    return log;
}

extern BRRlpItem
logRlpEncode(BREthereumLog log,
             BREthereumRlpType type,
             BRRlpCoder coder) {
    
    BRRlpItem items[6]; // more than enough

    items[0] = addressRlpEncode(log->address, coder);
    items[1] = logTopicsRlpEncode(log, coder);
    items[2] = rlpGetItem(coder, log->data); //  rlpEncodeBytes(coder, log->data.bytes, log->data.bytesCount);

    if (RLP_TYPE_ARCHIVE == type) {
        items[3] = hashRlpEncode(log->identifier.transactionHash, coder);
        items[4] = rlpEncodeUInt64(coder, log->identifier.transactionReceiptIndex, 0);
        items[5] = transactionStatusRLPEncode(log->status, coder);
    }

    return rlpEncodeListItems(coder, items, (RLP_TYPE_ARCHIVE == type ? 6 : 3));
}

/* Log (2) w/ LogTopic (3)
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
*/

