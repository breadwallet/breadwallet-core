//
//  BREthereumLESMessagePIP.h
//  BRCore
//
//  Created by Ed Gamble on 9/1/18.
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
//  LIABILITY, WHPIPER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BR_Ethereum_LES_Message_PIP_H
#define BR_Ethereum_LES_Message_PIP_H

#include "BREthereumLESBase.h"
#include "../blockchain/BREthereumBlockChain.h"

#ifdef __cplusplus
extern "C" {
#endif

/// MARK: PIP Requests

typedef uint64_t BREthereumPIPRequestIdentifier;
typedef uint64_t BREthereumPIPRequestCredits;

typedef enum {
    PIP_REQUEST_HEADERS           = 0x00,
    PIP_REQUEST_HEADER_PROOF      = 0x01,
    PIP_REQUEST_TRANSACTION_INDEX = 0x02,
    PIP_REQUEST_BLOCK_RECEIPTS    = 0x03,
    PIP_REQUEST_BLOCK_BODY        = 0x04,
    PIP_REQUEST_ACCOUNT           = 0x05,
    PIP_REQUEST_STORAGE           = 0x06,
    PIP_REQUEST_CODE              = 0x07,
    PIP_REQUEST_EXECUTION         = 0x08
} BREthereumPIPRequestType;

extern const char *
messagePIPGetRequestName (BREthereumPIPRequestType type);

/// Headers

typedef struct {
    int useBlockNumber;
    union {
        uint64_t number;
        BREthereumHash hash;
    } block;
    uint64_t skip;
    uint64_t max;
    BREthereumBoolean reverse;
} BREthereumPIPRequestHeadersInput;

typedef struct {
    BRArrayOf(BREthereumBlockHeader) headers;
} BREthereumPIPRequestHeadersOutput;

/// Header Proof

typedef struct {
    uint64_t blockNumber;
} BREthereumPIPRequestHeaderProofInput;

typedef struct {
    // [U8](U8) // merkle inclusion proof from CHT
    BREthereumHash blockHash;
    UInt256 blockTotalDifficulty;
} BREthereumPIPRequestHeaderProofOutput;

/// Transaction Index

typedef struct {
    BREthereumHash transactionHash;
} BREthereumPIPRequestTransactionIndexInput;

typedef struct {
    uint64_t blockNumber;
    BREthereumHash blockHash;
    uint64_t transactionIndex;
} BREthereumPIPRequestTransactionIndexOutput;

/// Block Receipts

typedef struct {
    BREthereumHash blockHash;
} BREthereumPIPRequestBlockReceiptInput;

typedef struct {
    BRArrayOf(BREthereumTransactionReceipt) receipts;
} BREthereumPIPRequestBlockReceiptOutput;

/// Block Body

typedef struct {
    BREthereumHash blockHash;
} BREthereumPIPRequestBlockBodyInput;

typedef struct {
    BRArrayOf(BREthereumBlockHeader) headers;
    BRArrayOf(BREthereumTransaction) transactions;
} BREthereumPIPRequestBlockBodyOutput;

/// Account

typedef struct {
    BREthereumHash blockHash;
    BREthereumHash addressHash;
} BREthereumPIPRequestAccountInput;

typedef struct {
    // [U8](U8) // merkle inclusion proof from state trie
    uint64_t nonce;
    UInt256 balance;
    BREthereumHash codeHash;
    BREthereumHash storageRootHash;
} BREthereumPIPRequestAccountOutput;

/// Storage

typedef struct {
    BREthereumHash blockHash;
    BREthereumHash addressHash;
    BREthereumHash storageRootHash; // storageKey?
} BREthereumPIPRequestStorageInput;

typedef struct {
    // [U8](U8) // merkle inclusion proof from storage trie
    BREthereumHash storageValueHash;
} BREthereumPIPRequestStorageOutput;

/// Code

typedef struct {
    BREthereumHash blockHash;
    BREthereumHash codeHash;
} BREthereumPIPRequestCodeInput;

typedef struct {
    BRArrayOf(uint8_t) bytecode;
} BREthereumPIPRequestCodeOutput;

/// Execution

typedef struct {
    BREthereumHash blockHash;
    BREthereumAddress address;
    int isCall;
    union {
        BREthereumAddress address;
        // empty
    } call;
    uint64_t gasToProve;
    uint64_t gasPrice;
    uint64_t valueToTransfer;
    BRArrayOf(uint8_t) callData;
} BREthereumPIPRequestExecutionInput;

typedef struct {
    // [U8](U8) // state items necessary to prove execution
} BREthereumPIPRequestExecutionOutput;

/// MARK: Request Input / Output

typedef struct {
    BREthereumPIPRequestType identifier;
    union {
        BREthereumPIPRequestHeadersInput headers;
        BREthereumPIPRequestHeaderProofInput headerProof;
        BREthereumPIPRequestTransactionIndexInput transactionIndex;
        BREthereumPIPRequestBlockReceiptInput blockReceipt;
        BREthereumPIPRequestBlockBodyInput blockBody;
        BREthereumPIPRequestAccountInput account;
        BREthereumPIPRequestStorageInput storage;
        BREthereumPIPRequestCodeInput code;
        BREthereumPIPRequestExecutionInput execution;
    } u;
} BREthereumPIPRequestInput;

typedef struct {
    BREthereumPIPRequestType identifier;
    union {
        BREthereumPIPRequestHeadersOutput headers;
        BREthereumPIPRequestHeaderProofOutput headerProof;
        BREthereumPIPRequestTransactionIndexOutput transactionIndex;
        BREthereumPIPRequestBlockReceiptOutput blockReceipt;
        BREthereumPIPRequestBlockBodyOutput blockBody;
        BREthereumPIPRequestAccountOutput account;
        BREthereumPIPRequestStorageOutput storage;
        BREthereumPIPRequestCodeOutput code;
        BREthereumPIPRequestExecutionOutput execution;
    } u;
} BREthereumPIPRequestOutput;

/// MARK: PIP Messages

typedef enum {
    PIP_MESSAGE_STATUS                   = 0x00,
    PIP_MESSAGE_ANNOUNCE                 = 0x01,
    PIP_MESSAGE_REQUEST                  = 0x02,
    PIP_MESSAGE_RESPONSE                 = 0x03,
    PIP_MESSAGE_UPDATE_CREDIT_PARAMETERS = 0x04,
    PIP_MESSAGE_ACKNOWLEDGE_UPDATE       = 0x05,
    PIP_MESSAGE_RELAY_TRANSACTIONS       = 0x06,
} BREthereumPIPMessageType;

extern const char *
messagePIPGetIdentifierName (BREthereumPIPMessageType identifer);

typedef enum {
    PIP_MESSAGE_STATUS_VALUE_INTEGER,
    PIP_MESSAGE_STATUS_VALUE_HASH,
    PIP_MESSAGE_STATUS_VALUE_EMPTY,
    PIP_MESSAGE_STATUS_VALUE_COST_TABLE,
    PIP_MESSAGE_STATUS_VALUE_RECHARGE_RATE,
} BREthereumPIPStatusValueType;

typedef struct {
    BREthereumPIPStatusValueType type;
    union {
        uint64_t integer;
        BREthereumHash hash;
        // ...
    } u;
} BREthereumPIPStatusValue;

typedef struct {
    const char *key;
    BREthereumPIPStatusValue value;
} BREthereumPIPStatusKeyValuePair;

typedef struct {
    // Extracted from `pairs`
    uint64_t protocolVersion;
    uint64_t chainId;

    uint64_t headNum;
    BREthereumHash headHash;
    UInt256 headTd;
    BREthereumHash genesisHash;

    // Other pairs
    BRArrayOf(BREthereumPIPStatusKeyValuePair) pairs;
} BREthereumPIPMessageStatus;

typedef struct {
    BREthereumHash headHash;
    uint64_t headNum;
    UInt256 headTotalDifficulty;
    uint64_t reorgDepth;
    BRArrayOf(BREthereumPIPStatusKeyValuePair) pairs;
} BREthereumPIPMessageAnnounce;

typedef struct {
    BREthereumPIPRequestIdentifier reqId;
    BRArrayOf(BREthereumPIPRequestInput) inputs;
} BREthereumPIPMessageRequest;

typedef struct {
    BREthereumPIPRequestIdentifier reqId;
    BREthereumPIPRequestCredits credits;
    BRArrayOf(BREthereumPIPRequestOutput) outputs;
} BREthereumPIPMessageResponse;

typedef struct {
    UInt256 max;
    UInt256 recharge;
    // cost table
} BREthereumPIPMessageUpdateCreditParameters;
typedef struct {} BREthereumPIPMessageAcknowledgeUpdate;

typedef struct {
    BRArrayOf(BREthereumTransaction) transactions;
} BREthereumPIPMessageRelayTransactions;

/**
 * An PIP Message is one of the above PIP message types.
 */
typedef struct {
    BREthereumPIPMessageType type;
    union {
        BREthereumPIPMessageStatus status;
        BREthereumPIPMessageAnnounce announce;
        BREthereumPIPMessageRequest request;
        BREthereumPIPMessageResponse response;
        BREthereumPIPMessageUpdateCreditParameters updateCreditParameters;
        BREthereumPIPMessageAcknowledgeUpdate acknowledgeUpdate;
        BREthereumPIPMessageRelayTransactions relayTransactions;
    } u;
} BREthereumPIPMessage;

extern BRRlpItem
messagePIPEncode (BREthereumPIPMessage message,
                  BREthereumMessageCoder coder);

extern BREthereumPIPMessage
messagePIPDecode (BRRlpItem item,
                  BREthereumMessageCoder coder,
                  BREthereumPIPMessageType identifier);

#define PIP_MESSAGE_NO_REQUEST_ID    (-1)
extern uint64_t
messagePIPGetRequestId (const BREthereumPIPMessage *message);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_Message_PIP_H */
