//
//  BREthereumLog.h
//  BRCore
//
//  Created by Ed Gamble on 5/10/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Log_h
#define BR_Ethereum_Log_h

#include "ethereum/base/BREthereumBase.h"
#include "BREthereumBloomFilter.h"
#include "BREthereumTransactionStatus.h"

#ifdef __cplusplus
extern "C" {
#endif

/// MARK: - Log Topic

#define LOG_TOPIC_BYTES_COUNT   32

/**
 * An Ethereum Log Topic is 32 bytes of arbitary data.
 */
typedef struct {
    uint8_t bytes[LOG_TOPIC_BYTES_COUNT];
} BREthereumLogTopic;

/**
 * Create a LogTopic from a 0x-prefaces, 67 (1 + 2 + 64) character string; otherwise fatal.
 *
 * @param string
 * @return
 */
extern BREthereumLogTopic
logTopicCreateFromString (const char *string);

extern BREthereumBloomFilter
logTopicGetBloomFilter (BREthereumLogTopic topic);

extern BREthereumBloomFilter
logTopicGetBloomFilterAddress (BREthereumAddress address);

extern BREthereumBoolean
logTopicMatchesAddress (BREthereumLogTopic topic,
                        BREthereumAddress address);

typedef struct {
    char chars[2 /* 0x */ + 2 * LOG_TOPIC_BYTES_COUNT + 1];
} BREthereumLogTopicString;

extern BREthereumLogTopicString
logTopicAsString (BREthereumLogTopic topic);

extern BREthereumAddress
logTopicAsAddress (BREthereumLogTopic topic);


/// MARK: - Log

/**
 * An Ethereum Log is the output of Smart Contract execution.
 *
 * From the Ethereum specificaion:  A log entry, O, is: {address, types, data}.  To that we add
 * a status, an identifier pair as { transactionHash, transactionReceiptIndex} and a hash (of the
 * identifier).
 */
typedef struct BREthereumLogRecord *BREthereumLog;

extern BREthereumLog
logCreate (BREthereumAddress address,
           unsigned int topicsCount,
           BREthereumLogTopic *topics,
           BRRlpData data);


extern void
logInitializeIdentifier (BREthereumLog log,
                         BREthereumHash transactionHash,
                         size_t transactionReceiptIndex);

/**
 * An identifier for an unknown receipt index.
 */
#define LOG_TRANSACTION_RECEIPT_INDEX_UNKNOWN       (SIZE_MAX)

/**
 * Extract the log's identifier components.  A Log is identified by the transaction hash that
 * originated the log and the index of the log in the block's transaction receipts array.
 *
 * If the log has not been recorded in a block, then FALSE is returned.  Otherwise TRUE is returned
 * and `transactionHash` and `transacetionReceiptIndex` will be filled.
 *
 * @param log the log
 * @param transactionHash a hash pointer; if non-NULL will be filled with the transaction's hash
 * @param transactionReceiptIndex an index pointer, if non-NULL will be filled with the logs
 *    index in the block's transaction receipts array.
 *
 * @return TRUE if recorded in a block, FALSE otherwise.
 */
extern BREthereumBoolean
logExtractIdentifier (BREthereumLog log,
                      BREthereumHash *transactionHash,
                      size_t *transactionReceiptIndex);

extern BREthereumHash
logGetHash (BREthereumLog log);

extern BREthereumAddress
logGetAddress (BREthereumLog log);

extern BREthereumBoolean
logHasAddress (BREthereumLog log,
               BREthereumAddress address);

extern size_t
logGetTopicsCount (BREthereumLog log);

extern  BREthereumLogTopic
logGetTopic (BREthereumLog log, size_t index);

extern BRRlpData
logGetData (BREthereumLog log);

extern BRRlpData
logGetDataShared (BREthereumLog log);
    
extern BREthereumBoolean
logMatchesAddress (BREthereumLog log,
                   BREthereumAddress address,
                   BREthereumBoolean topicsOnly);

extern BREthereumComparison
logCompare (BREthereumLog l1,
            BREthereumLog l2);

extern BREthereumTransactionStatus
logGetStatus (BREthereumLog log);

extern void
logSetStatus (BREthereumLog log,
              BREthereumTransactionStatus status);

extern BREthereumBoolean
logIsConfirmed (BREthereumLog log);

extern BREthereumBoolean
logIsErrored (BREthereumLog log);

// Support BRSet
extern size_t
logHashValue (const void *h);

// Support BRSet
extern int
logHashEqual (const void *h1, const void *h2);

extern BREthereumLog
logRlpDecode (BRRlpItem item,
              BREthereumRlpType type,
              BRRlpCoder coder);
/**
 * [QUASI-INTERNAL - used by BREthereumBlock]
 */
extern BRRlpItem
logRlpEncode(BREthereumLog log,
             BREthereumRlpType type,
             BRRlpCoder coder);

extern void
logRelease (BREthereumLog log);

extern void
logsRelease (BRArrayOf(BREthereumLog) logs);

extern void
logReleaseForSet (void *ignore, void *item);
    
extern BREthereumLog
logCopy (BREthereumLog log);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Log_h */
