//
//  BREthereumLog.h
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

#ifndef BR_Ethereum_Log_h
#define BR_Ethereum_Log_h

#include "../base/BREthereumBase.h"
#include "BREthereumBloomFilter.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Log Status
//
typedef enum {
    LOG_STATUS_UNKNOWN = 0,
    LOG_STATUS_PENDING = 1,
    LOG_STATUS_INCLUDED = 2,
    LOG_STATUS_ERRORED = 3
} BREthereumLogStatusType;

#define LOG_STATUS_REASON_BYTES   \
(sizeof (BREthereumHash) + sizeof(uint64_t))

typedef struct {
    BREthereumLogStatusType type;
    
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
    
    union {
        struct {
        } pending;
        
        struct {
            BREthereumHash blockHash;
            uint64_t blockNumber;
        } included;
        
        struct {
            char reason [LOG_STATUS_REASON_BYTES];
        } errored;
    } u;
} BREthereumLogStatus;

static inline BREthereumLogStatus
logStatusCreate (BREthereumLogStatusType type,
                  BREthereumHash transactionHash,
                  size_t transactionReceiptIndex) {
    BREthereumLogStatus status;
    memset (&status, 0, sizeof (status));
    status.type = type;
    status.identifier.transactionHash = transactionHash;
    status.identifier.transactionReceiptIndex = transactionReceiptIndex;
    return status;
}

static inline void
logStatusUpdateIncluded (BREthereumLogStatus *status,
                         BREthereumHash blockHash,
                         uint64_t blockNumber) {
    status->type = LOG_STATUS_INCLUDED;
    status->u.included.blockHash = blockHash;
    status->u.included.blockNumber = blockNumber;
}

static inline void
logStatusUpdateErrored (BREthereumLogStatus *status,
                        const char *reason) {
    status->type = LOG_STATUS_ERRORED;
    strlcpy(status->u.errored.reason, reason, LOG_STATUS_REASON_BYTES);
}

static inline void
logStatusUpdatePending (BREthereumLogStatus *status) {
    status->type = LOG_STATUS_PENDING;
}

static inline BREthereumHash
logStatusCreateHash (BREthereumLogStatus *status) {
    BRRlpData data = { sizeof (status->identifier), (uint8_t*) &status->identifier };
    return hashCreateFromData(data);
}

//
// Log Topic
//
typedef struct {
    uint8_t bytes[32];
} BREthereumLogTopic;

extern BREthereumBloomFilter
logTopicGetBloomFilter (BREthereumLogTopic topic);

extern BREthereumBloomFilter
logTopicGetBloomFilterAddress (BREthereumAddress address);

extern BREthereumBoolean
logTopicMatchesAddress (BREthereumLogTopic topic,
                        BREthereumAddress address);

typedef struct {
    char chars[67];
} BREthereumLogTopicString;

extern BREthereumLogTopicString
logTopicAsString (BREthereumLogTopic topic);

extern BREthereumAddress
logTopicAsAddress (BREthereumLogTopic topic);

//
// Log
//
typedef struct BREthereumLogRecord *BREthereumLog;

extern void
logInitializeStatus (BREthereumLog log,
                     BREthereumHash transactionHash,
                     size_t transactionReceiptIndex);

extern BREthereumLogStatus
logGetStatus (BREthereumLog log);

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

extern BREthereumBoolean
logMatchesAddress (BREthereumLog log,
                   BREthereumAddress address,
                   BREthereumBoolean topicsOnly);

extern int
logExtractIncluded(BREthereumLog log,
                   BREthereumHash *blockHash,
                   uint64_t *blockNumber);

// Support BRSet
extern size_t
logHashValue (const void *h);

// Support BRSet
extern int
logHashEqual (const void *h1, const void *h2);

extern BREthereumLog
logRlpDecodeItem (BRRlpItem item,
                  BRRlpCoder coder);
/**
 * [QUASI-INTERNAL - used by BREthereumBlock]
 */
extern BRRlpItem
logRlpEncodeItem(BREthereumLog log,
                 BRRlpCoder coder);

extern BRRlpData
logEncodeRLP (BREthereumLog log);

extern BREthereumLog
logDecodeRLP (BRRlpData data);

extern void
logRelease (BREthereumLog log);

extern void
logReleaseForSet (void *ignore, void *item);
    
extern BREthereumLog
logCopy (BREthereumLog log);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Log_h */
