//
//  BREthereumMessagePIP.h
//  BRCore
//
//  Created by Ed Gamble on 9/1/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Message_PIP_H
#define BR_Ethereum_Message_PIP_H

#include "ethereum/blockchain/BREthereumBlockChain.h"
#include "ethereum/mpt/BREthereumMPT.h"
#include "../BREthereumLESBase.h"
#include "BREthereumMessageP2P.h"       // BREthereumP2PMessageStatus

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

extern void
messagePIPRequestHeadersOutputConsume (BREthereumPIPRequestHeadersOutput *output,
                                       BRArrayOf(BREthereumBlockHeader) *headers);

/// Header Proof

typedef struct {
    uint64_t blockNumber;
} BREthereumPIPRequestHeaderProofInput;

typedef struct {
    // [U8](U8) // merkle inclusion proof from CHT
    BREthereumMPTNodePath path;
    BREthereumHash blockHash;
    UInt256 blockTotalDifficulty;
} BREthereumPIPRequestHeaderProofOutput;

extern void
messagePIPRequestHeaderProofOutputConsume (BREthereumPIPRequestHeaderProofOutput *output,
                                           BREthereumMPTNodePath *path);

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

typedef struct {
    BREthereumP2PMessageStatus p2p;
} BREthereumPIPMessageStatus;

extern void
messagePIPStatusShow(BREthereumPIPMessageStatus *status);

typedef struct {
    BREthereumHash headHash;
    uint64_t headNumber;
    UInt256 headTotalDifficulty;
    uint64_t reorgDepth;
    BRArrayOf(BREthereumP2PMessageStatusKeyValuePair) pairs;
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

    /**
     * Consume the `message` `outputs`.  Modify message so that a subsequent `messagePIPRelease()`
     * recognizes the `outputs` are now owned elsewhere.
     */
extern void
messagePIPResponseConsume (BREthereumPIPMessageResponse *message,
                           BRArrayOf(BREthereumPIPRequestOutput) *outputs);

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

extern void
messagePIPRelease (BREthereumPIPMessage *message);

extern uint64_t
messagePIPGetCredits (const BREthereumPIPMessage *message);

extern uint64_t
messagePIPGetCreditsCount (const BREthereumPIPMessage *message);

#define PIP_MESSAGE_NO_REQUEST_ID    (-1)
extern size_t
messagePIPGetRequestId (const BREthereumPIPMessage *message);

extern const char *
messagePIPGetIdentifierName (BREthereumPIPMessage message);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Message_PIP_H */
