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
#include "BREthereumTransactionStatus.h"

#if ! defined (BRArrayOf)
#define BRArrayOf(type)     type*
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// MARK: - Log Topic
//
#define LOG_TOPIC_BYTES_COUNT   32

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

//
// MARK: - Log
//
typedef struct BREthereumLogRecord *BREthereumLog;

extern BREthereumLog
logCreate (BREthereumAddress address,
           unsigned int topicsCount,
           BREthereumLogTopic *topics);

extern void
logInitializeIdentifier (BREthereumLog log,
                         BREthereumHash transactionHash,
                         size_t transactionReceiptIndex);

extern void
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

extern BREthereumBoolean
logMatchesAddress (BREthereumLog log,
                   BREthereumAddress address,
                   BREthereumBoolean topicsOnly);

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
logReleaseForSet (void *ignore, void *item);
    
extern BREthereumLog
logCopy (BREthereumLog log);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Log_h */
