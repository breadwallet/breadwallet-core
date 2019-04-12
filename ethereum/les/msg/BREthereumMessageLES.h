//
//  BREthereumMessageLES.h
//  BRCore
//
//  Created by Ed Gamble on 9/1/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Message_LES_H
#define BR_Ethereum_Message_LES_H

#include "ethereum/mpt/BREthereumMPT.h"
#include "ethereum/blockchain/BREthereumBlock.h"
#include "ethereum/blockchain/BREthereumTransactionReceipt.h"
#include "../BREthereumLESBase.h"
#include "BREthereumMessageP2P.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * LES (v2 GETH) defines these message types
 */
typedef enum {
    LES_MESSAGE_STATUS             = 0x00,
    LES_MESSAGE_ANNOUNCE           = 0x01,
    LES_MESSAGE_GET_BLOCK_HEADERS  = 0x02,
    LES_MESSAGE_BLOCK_HEADERS      = 0x03,
    LES_MESSAGE_GET_BLOCK_BODIES   = 0x04,
    LES_MESSAGE_BLOCK_BODIES       = 0x05,
    LES_MESSAGE_GET_RECEIPTS       = 0x06,
    LES_MESSAGE_RECEIPTS           = 0x07,
    LES_MESSAGE_GET_PROOFS         = 0x08,
    LES_MESSAGE_PROOFS             = 0x09,
    LES_MESSAGE_GET_CONTRACT_CODES = 0x0a,
    LES_MESSAGE_CONTRACT_CODES     = 0x0b,
    LES_MESSAGE_SEND_TX            = 0x0c,
    LES_MESSAGE_GET_HEADER_PROOFS  = 0x0d,
    LES_MESSAGE_HEADER_PROOFS      = 0x0e,
    LES_MESSAGE_GET_PROOFS_V2      = 0x0f,
    LES_MESSAGE_PROOFS_V2          = 0x10,
    LES_MESSAGE_GET_HELPER_TRIE_PROOFS = 0x11,
    LES_MESSAGE_HELPER_TRIE_PROOFS     = 0x12,
    LES_MESSAGE_SEND_TX2           = 0x13,
    LES_MESSAGE_GET_TX_STATUS      = 0x14,
    LES_MESSAGE_TX_STATUS          = 0x15,
} BREthereumLESMessageIdentifier;

#define NUMBER_OF_LES_MESSAGE_IDENTIFIERS    (LES_MESSAGE_TX_STATUS + 1)

extern const char *
messageLESGetIdentifierName (BREthereumLESMessageIdentifier id);

/// MARK: LES Status

/** */
typedef struct {
    uint64_t msgCode;
    uint64_t baseCost;
    uint64_t reqCost;
} BREthereumLESMessageStatusMRC;

/**
 * A LES Status message ...
 */
typedef struct {
    BREthereumP2PMessageStatus p2p;
    BREthereumLESMessageStatusMRC costs[NUMBER_OF_LES_MESSAGE_IDENTIFIERS];
} BREthereumLESMessageStatus;

extern BREthereumLESMessageStatus
messageLESStatusCreate (uint64_t protocolVersion,
                        uint64_t chainId,
                        uint64_t headNum,
                        BREthereumHash headHash,
                        UInt256 headTd,
                        BREthereumHash genesisHash,
                        uint64_t announceType);

extern BRRlpItem
messageLESStatusEncode (BREthereumLESMessageStatus *status,
                        BREthereumMessageCoder coder);

extern BREthereumLESMessageStatus
messageLESStatusDecode (BRRlpItem item,
                        BREthereumMessageCoder coder);

extern void
messageLESStatusShow(BREthereumLESMessageStatus *status);

/// MARK: LES Announce

/**
 * A LES Announce Message ...
 */
typedef struct {
    // [+0x01, headHash: B_32, headNumber: P, headTd: P, reorgDepth: P, [key_0, value_0], [key_1, value_1], ...]
    BREthereumHash headHash;
    uint64_t headNumber;
    UInt256 headTotalDifficulty;
    uint64_t reorgDepth;
    BRArrayOf(BREthereumP2PMessageStatusKeyValuePair) pairs;
} BREthereumLESMessageAnnounce;

extern void
messageLESAnnounceConsume (BREthereumLESMessageAnnounce *message,
                           BRArrayOf(BREthereumP2PMessageStatusKeyValuePair) *pairs);
//    {
//        if (NULL == pairs) array_free (message->pairs);
//            else *pairs = message->pairs;
//    }

/// MARK: LES Get Block Headers

/**
 * A LES Get Block Headers Message ...
 *     [+0x02, reqID: P, [block: { P , B_32 }, maxHeaders: P, skip: P, reverse: P in { 0 , 1 } ]]
 */
typedef struct {
    uint64_t reqId;
    int useBlockNumber;
    union {
        uint64_t number;
        BREthereumHash hash;
    } block;
    uint32_t maxHeaders;
    uint64_t skip;
    uint8_t reverse;
} BREthereumLESMessageGetBlockHeaders;

extern BREthereumLESMessageGetBlockHeaders
messageLESGetBlockHeadersCreate (uint64_t reqId,
                                 uint64_t number,
                                 uint32_t maxHeaders,
                                 uint64_t skip,
                                 uint8_t  reverse);

// TODO: Include `reqId` or not?  Include `msgId` (w/ offset) or not?  Depends on encryption...
extern BRRlpItem
messageLESGetBlockHeadersEncode (BREthereumLESMessageGetBlockHeaders message,
                                 BREthereumMessageCoder coder);

/// MARK: LES Block Headers

/**
 * A LES Block Header Message ...
 *     [+0x03, reqID: P, BV: P, [blockHeader_0, blockHeader_1, ...]]
 */
typedef struct {
    uint64_t reqId;
    uint64_t bv;
    BRArrayOf(BREthereumBlockHeader) headers;
} BREthereumLESMessageBlockHeaders;

extern void
messageLESBlockHeadersConsume (BREthereumLESMessageBlockHeaders *message,
                               BRArrayOf(BREthereumBlockHeader) *headers);

// TODO: Include `reqId` or not?  Include `msgId` (w/ offset) or not?  Depends on encryption...
extern BREthereumLESMessageBlockHeaders
messageLESBlockHeadersDecode (BRRlpItem item,
                              BREthereumMessageCoder coder);

/// MARK: LES GetBlockBodies

/**
 * A LES Get Block Bodies Message ...
 *     [+0x04, reqID: P, [hash_0: B_32, hash_1: B_32, ...]]
 */
typedef struct {
    uint64_t reqId;
    BRArrayOf (BREthereumHash) hashes;
} BREthereumLESMessageGetBlockBodies;

/// MARK: LES BlockBodies

/**
 * A LES Block Bodies Message ...
 *     [+0x05, reqID: P, BV: P, [ [transactions_0, uncles_0] , ...]]
 */
typedef struct {
    uint64_t reqId;
    uint64_t bv;
    BRArrayOf(BREthereumBlockBodyPair) pairs;
} BREthereumLESMessageBlockBodies;

extern void
messageLESBlockBodiesConsume (BREthereumLESMessageBlockBodies *message,
                              BRArrayOf(BREthereumBlockBodyPair) *pairs);

/// MARK: LES GetReceipts

/**
 * A LES Get Receipts Message ...
 *     [+0x06, reqID: P, [hash_0: B_32, hash_1: B_32, ...]]
 */
typedef struct {
    uint64_t reqId;
    BRArrayOf(BREthereumHash) hashes;
} BREthereumLESMessageGetReceipts;

/// MARK: LES Receipts

typedef struct {
    BRArrayOf(BREthereumTransactionReceipt) receipts;
} BREthereumLESMessageReceiptsArray;

/**
 * A LES Receipts Message ...
 *     [+0x07, reqID: P, BV: P, [ [receipt_0, receipt_1, ...], ...]]
 */
typedef struct {
    uint64_t reqId;
    uint64_t bv;
    BRArrayOf (BREthereumLESMessageReceiptsArray) arrays;
} BREthereumLESMessageReceipts;

extern void
messageLESReceiptsConsume (BREthereumLESMessageReceipts *receipts,
                            BRArrayOf (BREthereumLESMessageReceiptsArray) *arrays);

/// MARK: LES GetProofs

typedef struct {
    BREthereumHash blockHash;
    BREthereumAddress address;
    uint64_t fromLevel;
} BREthereumLESMessageGetProofsSpec;

/**
 * A LES Get Proofs Message ...
 *     [+0x08, reqID: P, [ [blockhash: B_32, key: B_32, key2: B_32, fromLevel: P], ...]]
 */
typedef struct {
    uint64_t reqId;
    BRArrayOf(BREthereumLESMessageGetProofsSpec) specs;
} BREthereumLESMessageGetProofs;

/// MARK: LES Proofs

/**
 * A LES Proofs message
 *     [+0x09, reqID: P, BV: P, [ [node_1, node_2, ...], ...]]
 */
typedef struct {
    uint64_t reqId;
    uint64_t bv;
    BRArrayOf(BREthereumMPTNodePath) paths;
} BREthereumLESMessageProofs;

extern void
messageLESProofsConsume (BREthereumLESMessageProofs *message,
                         BRArrayOf(BREthereumMPTNodePath) *paths);

/// MARK: LES GetContractCodes

typedef struct {
    uint64_t reqId;
} BREthereumLESMessageGetContractCodes;

/// MARK: LES ContractCodes

typedef struct {
    uint64_t reqId;
    uint64_t bv;
} BREthereumLESMessageContractCodes;

/// MARK: LES SendTx

/**
 *
 *     [+0x0c, txdata_1, txdata_2, ...]
 */
typedef struct {
    uint64_t reqId;
    BRArrayOf(BREthereumTransaction) transactions;
} BREthereumLESMessageSendTx;

/// MARK: LES GetHeaderProofs

    /**
     * GetHeaderProofs [+0x0d, reqID: P, [ [chtNumber: P, blockNumber: P, fromLevel: P], ...]]
     */
typedef struct {
    uint64_t reqId;
    BRArrayOf(uint64_t) chtNumbers;
    BRArrayOf(uint64_t) blkNumbers;;
} BREthereumLESMessageGetHeaderProofs;

/// MARK: LES HeaderProofs

    /**
     * HeaderProofs [+0x0e, reqID: P, BV: P, [ [blockHeader, [node_1, node_2...]], ...]]
     */
typedef struct {
    uint64_t reqId;
    uint64_t bv;
    BRArrayOf(BREthereumBlockHeader) headers;
    BRArrayOf(BREthereumMPTNodePath) paths;
} BREthereumLESMessageHeaderProofs;

extern void
messageLESHeaderProofsConsume (BREthereumLESMessageHeaderProofs *message,
                               BRArrayOf(BREthereumBlockHeader) *headers,
                               BRArrayOf(BREthereumMPTNodePath) *paths);

/// MARK: LES GetProofsV2

typedef struct {
    uint64_t reqId;
    BRArrayOf(BREthereumLESMessageGetProofsSpec) specs;
} BREthereumLESMessageGetProofsV2;

/// MARK: LES ProofsV2

typedef struct {
    uint64_t reqId;
    uint64_t bv;
    BREthereumMPTNodePath path;
} BREthereumLESMessageProofsV2;


/// MARK: LES GetHelperTrieProofs

typedef struct {
    uint64_t reqId;
} BREthereumLESMessageGetHelperTrieProofs;

/// MARK: LES HelperTrieProofs
    
typedef struct {
    uint64_t reqId;
    uint64_t bv;
} BREthereumLESMessageHelperTrieProofs;

/// MARK: LES SendTx2

/**
 *
 *     [+0x13, reqID: P, [txdata_1, txdata_2, ...]]
 */
typedef struct {
    uint64_t reqId;
    BRArrayOf(BREthereumTransaction) transactions;
} BREthereumLESMessageSendTx2;

/// MARK: LES GetTxStatus

/**
 *
 *     [+0x14, reqID: P, [txHash_1, txHash_2, ...]]
 */
typedef struct {
    uint64_t reqId;
    BRArrayOf(BREthereumHash) hashes;
} BREthereumLESMessageGetTxStatus;

/// MARK: LES TxStatus

/**
 *
 *
 */
typedef struct {
    uint64_t reqId;
    uint64_t bv;
    BRArrayOf(BREthereumTransactionStatus) stati;
} BREthereumLESMessageTxStatus;

extern void
messageLESTxStatusConsume (BREthereumLESMessageTxStatus *message,
                           BRArrayOf(BREthereumTransactionStatus) *stati);

//
// ...
//

/// MARK: LES Message

typedef enum {
    LES_MESSAGE_USE_STATUS,
    LES_MESSAGE_USE_REQUEST,
    LES_MESSAGE_USE_RESPONSE
} BREthereumLESMessageUse;


typedef struct {
    /** Name (Displayable) */
    const char *name;
    
    /** Use */
    BREthereumLESMessageUse use;
    
    /** Maximum number of messages that can be sent/requested */
    size_t limit;
    
    /** Cost for 0 messages */
    uint64_t baseCost;
    
    /** Cost of each message */
    uint64_t reqCost;
} BREthereumLESMessageSpec;

extern BREthereumLESMessageSpec
messageLESSpecs [NUMBER_OF_LES_MESSAGE_IDENTIFIERS];

/**
 * A LES Message is a union of the above LES messages
 */
typedef struct {
    BREthereumLESMessageIdentifier identifier;
    union {
        BREthereumLESMessageStatus status;
        BREthereumLESMessageAnnounce announce;
        BREthereumLESMessageGetBlockHeaders getBlockHeaders;
        BREthereumLESMessageBlockHeaders blockHeaders;
        BREthereumLESMessageGetBlockBodies getBlockBodies;
        BREthereumLESMessageBlockBodies blockBodies;
        BREthereumLESMessageGetReceipts getReceipts;
        BREthereumLESMessageReceipts receipts;
        BREthereumLESMessageGetProofs getProofs;
        BREthereumLESMessageProofs proofs;
        BREthereumLESMessageGetContractCodes getContractCodes;
        BREthereumLESMessageContractCodes contractCodes;
        BREthereumLESMessageSendTx sendTx;
        BREthereumLESMessageGetHeaderProofs getHeaderProofs;
        BREthereumLESMessageHeaderProofs headerProofs;
        BREthereumLESMessageGetProofsV2 getProofsV2;
        BREthereumLESMessageProofsV2 proofsV2;
        BREthereumLESMessageGetHelperTrieProofs getHelperTrieProofs;
        BREthereumLESMessageHelperTrieProofs helperTrieProofs;
        BREthereumLESMessageSendTx2 sendTx2;
        BREthereumLESMessageGetTxStatus getTxStatus;
        BREthereumLESMessageTxStatus txStatus;
    } u;
} BREthereumLESMessage;

/**
 *  Decode a LES message.
 *
 * @param item
 * @param coder
 * @param identifier
 * @return The decoded LES Message
 */
extern BREthereumLESMessage
messageLESDecode (BRRlpItem item,
                  BREthereumMessageCoder coder,
                  BREthereumLESMessageIdentifier identifier);


/**
 * Encode a LES message
 *
 * @param message
 * @param coder
 * @return The encoded message as an `RLP Item`
 */
extern BRRlpItem
messageLESEncode (BREthereumLESMessage message,
                  BREthereumMessageCoder coder);

extern void
messageLESRelease (BREthereumLESMessage *message);

extern int
messageLESHasUse (const BREthereumLESMessage *message,
                  BREthereumLESMessageUse use);
// 0 if not response
extern uint64_t
messageLESGetCredits (const BREthereumLESMessage *message);

extern uint64_t
messageLESGetCreditsCount (const BREthereumLESMessage *message);

#define LES_MESSAGE_NO_REQUEST_ID    (-1)
extern size_t
messageLESGetRequestId (const BREthereumLESMessage *message);

extern uint64_t
messageLESGetChtNumber (uint64_t blkNumber);

extern const char *lesTransactionErrorPreface[];
    
//    extern int
//    messageLESIsChtBlockNumber (uint64_t blkNumber);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Message_LES_H */
