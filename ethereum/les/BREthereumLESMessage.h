//
//  BREthereumLESMessage.h
//  Core
//
//  Created by Ed Gamble on 8/13/18.
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

#ifndef BR_Ethereum_LES_Message_H
#define BR_Ethereum_LES_Message_H

#include <stdlib.h>
#include <memory.h>
#include <limits.h>
#include "BRArray.h"
#include "BRSet.h"
#include "../base/BREthereumBase.h"
#include "../blockchain/BREthereumBlockChain.h"
#include "../mpt/BREthereumMPT.h"

#define BRArrayOf(type)    type*
#define BRSetOf(type)      BRSet*

#define LES_LOG_TOPIC "LES"

#ifdef __cplusplus
extern "C" {
#endif

//
// BREthereumMessageCoder - when RLP encoding and decoding messages, we need both the
// RLP Coder, the Network and a LES message offset.  The network is required for decoding
// transactions - where the signature encodes the network's chain id.  The LES message offset
// is required to offset a LES message id (away from the P2P id space).
//
// We could have considered modifying BRRlpCoder to include the network - however, the RLP
// module *absolutely does not* depend on anything...  So we'll use this 'MessageCoder' abstraction
// to bundle all the LES specific needs.
//
typedef struct {
    BRRlpCoder rlp;
    BREthereumNetwork network;

    // The offset for LES messages.  This is determined by 'negotiating' subprotocols
    // https://github.com/ethereum/wiki/wiki/ÐΞVp2p-Wire-Protocol
    //
    // "ÐΞVp2p is designed to support arbitrary sub-protocols (aka capabilities) over the basic
    // wire protocol. Each sub-protocol is given as much of the message-ID space as it needs (all
    // such protocols must statically specify how many message IDs they require). On connection and
    // reception of the Hello message, both peers have equivalent information about what
    // subprotocols they share (including versions) and are able to form consensus over the
    // composition of message ID space.
    //
    // "Message IDs are assumed to be compact from ID 0x10 onwards (0x00-0x10 is reserved for
    // ÐΞVp2p messages) and given to each shared (equal-version, equal name) sub-protocol in
    // alphabetic order. Sub-protocols that are not shared are ignored. If multiple versions are
    // shared of the same (equal name) sub-protocol, the numerically highest wins, others are
    // ignored."
    //
    // Generally, we have one protocol specified
    uint64_t lesMessageIdOffset;
} BREthereumMessageCoder;

//
// BREthereumMessageIdentifier - The Ethereum Wire Protocol (WIP) defines four fundamental
// message types.  We'll explicitly handle each one.
//
typedef enum {
    MESSAGE_P2P   = 0x00,
    MESSAGE_ETH   = 0x01,
    MESSAGE_LES   = 0x02,
    MESSAGE_DIS   = 0x03
} BREthereumMessageIdentifier;

/// MARK: - P2P (Peer-to-Peer) Messages

//
// BREthereumP2PMessageIdentifier - The Ethereum P2P protocol defines four messages.
//
typedef enum {
    P2P_MESSAGE_HELLO      = 0x00,
    P2P_MESSAGE_DISCONNECT = 0x01,
    P2P_MESSAGE_PING       = 0x02,
    P2P_MESSAGE_PONG       = 0x03
} BREthereumP2PMessageIdentifier;

extern const char *
messageP2PGetIdentifierName (BREthereumP2PMessageIdentifier identifier);

//
// P2P Hello
//
typedef struct {
    char name[4];
    uint32_t version;
} BREthereumP2PCapability;

typedef struct {
    // 0x00 [p2pVersion: P, clientId: B, [[cap1: B_3, capVersion1: P], [cap2: B_3, capVersion2: P], ...], listenPort: P, nodeId: B_64]
    uint64_t version;
    char    *clientId;
    BRArrayOf (BREthereumP2PCapability) capabilities;
    uint64_t port;
    UInt512  nodeId;
} BREthereumP2PMessageHello;

extern BRRlpItem
messageP2PHelloEncode (BREthereumP2PMessageHello message, BREthereumMessageCoder coder);

extern BREthereumP2PMessageHello
messageP2PHelloDecode (BRRlpItem item, BREthereumMessageCoder coder);

extern void
messageP2PHelloShow (BREthereumP2PMessageHello hello);

extern BREthereumBoolean
messageP2PCababilityEqual (const BREthereumP2PCapability *cap1,
                           const BREthereumP2PCapability *cap2);

extern BREthereumBoolean
messageP2PHelloHasCapability (const BREthereumP2PMessageHello *hello,
                              const BREthereumP2PCapability *capability);
//
// P2P Disconnect
//
typedef enum {
    P2P_MESSAGE_DISCONNECT_REQUESTED         = 0x00,
    P2P_MESSAGE_DISCONNECT_TCP_ERROR         = 0x01,
    P2P_MESSAGE_DISCONNECT_BREACH_PROTO      = 0x02,
    P2P_MESSAGE_DISCONNECT_USELESS_PEER      = 0x03,
    P2P_MESSAGE_DISCONNECT_TOO_MANY_PEERS    = 0x04,
    P2P_MESSAGE_DISCONNECT_ALREADY_CONNECTED = 0x05,
    P2P_MESSAGE_DISCONNECT_INCOMPATIBLE_P2P  = 0x06,
    P2P_MESSAGE_DISCONNECT_NULL_NODE         = 0x07,
    P2P_MESSAGE_DISCONNECT_CLIENT_QUIT       = 0x08,
    P2P_MESSAGE_DISCONNECT_UNEXPECTED_ID     = 0x09,
    P2P_MESSAGE_DISCONNECT_ID_SAME           = 0x0a,
    P2P_MESSAGE_DISCONNECT_TIMEOUT           = 0x0b,
    P2P_MESSAGE_DISCONNECT_UNKNOWN           = 0x10
} BREthereumP2PDisconnectReason;

extern const char *
messageP2PDisconnectDescription (BREthereumP2PDisconnectReason identifier);

typedef struct {
    // 0x01 [reason: P]
    BREthereumP2PDisconnectReason reason;
} BREthereumP2PMessageDisconnect;

//
// P2P Ping
//
typedef struct {} BREthereumP2PMessagePing;

//
// P2P Pong
//
typedef struct {} BREthererumP2PMessagePong;

//
// P2P Message
//
typedef struct {
    BREthereumP2PMessageIdentifier identifier;
    union {
        BREthereumP2PMessageHello hello;
        BREthereumP2PMessageDisconnect disconnect;
        BREthereumP2PMessagePing ping;
        BREthereumP2PMessagePing pong;
    } u;
} BREthereumP2PMessage;

extern BRRlpItem
messageP2PEncode (BREthereumP2PMessage message,
                  BREthereumMessageCoder coder);

/// MARK: - ETH (Ethereum) Messages

/**
 * The 'Ethereum Subprotocol' define N messages - we are 'Light Client' ...
 * ... and thus we don't need these.  Included for completeness and wholely undefined.
 */
typedef enum {
    ETH_MESSAGE_FOO = 0x01,
    ETH_MESSAGE_BAR = 0x02
} BREthereumETHMessageIdentifier;

typedef struct {} BREthereumETHMessageFoo;
typedef struct {} BREthereumETHMessageBar;

/**
 * An ETH Message is one of the above ETH message types.
 */
typedef struct {
    BREthereumETHMessageIdentifier identifier;
    union {
        BREthereumETHMessageFoo foo;
        BREthereumETHMessageBar bar;
    } u;
} BREthereumETHMessage;


/// MARK: - DIS (Node Discovery) Messages

/**
 * Node DIScovery (apparently V4) defines four message types: PING, PONG, FIND_NEIGHBORS and
 * NEIGHBORS.  Note that the message identifier of '0x00' is reserved.  This DIS identifier of
 * 0x00, ..., 0x04 overlap with the P2P identifier; however, the DIS identifier are used on the
 * UDP route (and PIP identifiers are use don the TCP route).
 */
typedef enum {
    DIS_MESSAGE_PING           = 0x01,
    DIS_MESSAGE_PONG           = 0x02,
    DIS_MESSAGE_FIND_NEIGHBORS = 0x03,
    DIS_MESSAGE_NEIGHBORS      = 0x04
} BREthereumDISMessageIdentifier;

#define MESSAGE_DIS_IDENTIFIER_ANY   ((BREthereumDISMessageIdentifier) 0x00)

extern const char *
messageDISGetIdentifierName (BREthereumDISMessageIdentifier identifier);

/**
 * A DIS Endpoint is commonly used to identify an INET address.  Sometimes a DIS Endpoint is
 * provided to identify from/to (like in the Ping/Pong mesages) and, most importantly, to identify
 * peers (like in the Neighbor message).
 */
typedef struct {
    /** The AF (Address Family) domain - one of AF_INET or AF_INET6 */
    int domain;

    /** The IP address - 4 bytes for ivp4; 16 bytes for ivp6 */
    union {
        uint8_t ipv4[4];   // struct in_addr
        uint8_t ipv6[16];
    } addr;

    /** The UDP port */
    uint16_t portUDP;

    /** The TCP port */
    uint16_t portTCP;
} BREthereumDISEndpoint;

extern BRRlpItem
endpointDISEncode (const BREthereumDISEndpoint *endpoint, BRRlpCoder coder);

extern BREthereumDISEndpoint
endpointDISDecode (BRRlpItem item, BRRlpCoder coder);

/**
 * The DIS Ping Message.  The first four fields are RLP encoded.  We
 */
typedef struct {
    int version; // = 4
    BREthereumDISEndpoint from;
    BREthereumDISEndpoint to;
    uint64_t expiration;

    /** The Ping hash (from the DIS header) is used in the Pong reply. We record is here */
    BREthereumHash hash;
} BREthereumDISMessagePing;

extern BREthereumDISMessagePing
messageDISPingCreate (BREthereumDISEndpoint to,
                      BREthereumDISEndpoint from,
                      uint64_t expiration);

/**
 * The DIS Pong Message
 */
typedef struct {
    BREthereumDISEndpoint to;
    BREthereumHash pingHash;
    uint64_t expiration;
} BREthereumDISMessagePong;

extern BREthereumDISMessagePong
messageDISPongCreate (BREthereumDISEndpoint to,
                      BREthereumHash pingHash,
                      uint64_t expiration);

/**
 * The DIS Find Neighbors Message
 */
typedef struct {
    BRKey target; // 65-byte secp256k1 public key
    uint64_t expiration;
} BREthereumDISMessageFindNeighbors;

extern BREthereumDISMessageFindNeighbors
messageDISFindNeighborsCreate (BRKey target,
                               uint64_t expiration);

/**
 * A DIS Neighbor.  The result of Find Neighbors will be an array of these
 */
typedef struct {
    BREthereumDISEndpoint node;
    UInt512 nodeID;     // a 64 byte public key
} BREthereumDISNeighbor;

/**
 * The DIS Neighbors Message.
 */
typedef struct {
    BRArrayOf(BREthereumDISNeighbor) neighbors;
    uint64_t expiration;
} BREthereumDISMessageNeighbors;

/**
 * A DIS Message is one of four messages.  We include the `privateKeyForSigning` - although
 * it might be better to provide the key at the point when the message is serialized so that
 * the key doesn't sit here-and-there in memory
 */
typedef struct {
    BREthereumDISMessageIdentifier identifier;
    union {
        BREthereumDISMessagePing ping;
        BREthereumDISMessagePong pong;
        BREthereumDISMessageFindNeighbors findNeighbors;
        BREthereumDISMessageNeighbors neighbors;
    } u;
    BRKey privateKeyForSigning;
} BREthereumDISMessage;

/// MARK: - LES (Light Ethereum Subprotocol) Message

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
typedef const char *BREthereumLESMessageStatusKey;

typedef union {
    uint32_t number;
    int boolean;
    UInt256 bignum;
    BREthereumHash hash;
    // flow control
} BREthereumLESMessageStatusValue;

typedef struct {
    // int provided;
    BREthereumLESMessageStatusKey key;
    BREthereumLESMessageStatusValue value;
} BREthereumLESMessageStatusKeyValuePair;

typedef struct {
    uint64_t msgCode;
    uint64_t baseCost;
    uint64_t reqCost;
} BREthereumLESMessageStatusMRC;

/**
 * A LES Status message ...
 */
typedef struct {
    uint64_t protocolVersion;
    uint64_t chainId;

    uint64_t headNum;
    BREthereumHash headHash;
    UInt256 headTd;
    BREthereumHash genesisHash;

    // Note: The below fields are optional LPV1
    BREthereumBoolean serveHeaders;
    uint64_t *serveChainSince;
    uint64_t *serveStateSince;
    BREthereumBoolean txRelay;

    uint64_t *flowControlBL;
    BREthereumLESMessageStatusMRC *flowControlMRC;
    size_t   *flowControlMRCCount;
    uint64_t *flowControlMRR;
    uint64_t  announceType;
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
messageLESStatusEncode (BREthereumLESMessageStatus *status, BREthereumMessageCoder coder);

extern BREthereumLESMessageStatus
messageLESStatusDecode (BRRlpItem item, BREthereumMessageCoder coder);

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
    BRArrayOf(BREthereumLESMessageStatusKeyValuePair) pairs;
} BREthereumLESMessageAnnounce;

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

/// MARK: LES GetProofs

typedef struct {
    BREthereumHash blockHash;
    BRRlpData key1;
    BRRlpData key2;
    uint64_t fromLevel;
    // Not RLP encoded
    uint64_t blockNumber;
    BREthereumAddress address;
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
typedef struct {
    uint64_t reqId;
} BREthereumLESMessageGetHeaderProofs;

/// MARK: LES HeaderProofs
typedef struct {
    uint64_t reqId;
    uint64_t bv;
} BREthereumLESMessageHeaderProofs;

/// MARK: LES GetProofsV2
typedef struct {
    uint64_t reqId;
    BRArrayOf(BREthereumLESMessageGetProofsSpec) specs;
} BREthereumLESMessageGetProofsV2;

/// MARK: LES ProofsV2
typedef struct {
    uint64_t reqId;
    uint64_t bv;
    BRArrayOf(BREthereumMPTNodePath) paths;
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
    uint64_t limit;

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

extern int
messageLESHasUse (const BREthereumLESMessage *message,
                  BREthereumLESMessageUse use);

// 0 if not response
extern uint64_t
messageLESGetCredits (const BREthereumLESMessage *message);

#define LES_MESSAGE_NO_REQUEST_ID    (-1)
extern uint64_t
messageLESGetRequestId (const BREthereumLESMessage *message);

/// MARK: - Wire Protocol Messages

/**
 * An ANYMessageIdentifier can be any of P2P, DIS, LES or ETH message identifiers
 */
typedef int16_t BREthereumANYMessageIdentifier;

/**
 * An Ethereum Message is one of the P2P, DIS, LES or ETH messages
 */
typedef struct {
    BREthereumMessageIdentifier identifier;
    union {
        BREthereumP2PMessage p2p;
        BREthereumETHMessage eth;
        BREthereumDISMessage dis;
        BREthereumLESMessage les;
    } u;
} BREthereumMessage;

extern BRRlpItem
messageEncode (BREthereumMessage message,
               BREthereumMessageCoder coder);

extern BREthereumMessage
messageDecode (BRRlpItem item,
               BREthereumMessageCoder coder,
               BREthereumMessageIdentifier type,
               BREthereumANYMessageIdentifier subtype);

/**
 * Check if `message` has the provided `identifier`
 *
 * @param message
 * @param identifer
 *
 * @return TRUE (1) if message has identifier, FALSE (0) otherwise.
 */
extern int
messageHasIdentifier (BREthereumMessage *message,
                      BREthereumMessageIdentifier identifer);

/**
 * Check if `message` has the provided `identifier` and 'sub identifier`
 *
 * @param message
 * @param identifer
 * @param anyIdentifer
 *
 * @return TRUE (1) if message has both identifiers, FALSE (0) otherwise.
 */
extern int
messageHasIdentifiers (BREthereumMessage *message,
                       BREthereumMessageIdentifier identifer,
                       BREthereumANYMessageIdentifier anyIdentifier);

const char *
messageGetIdentifierName (BREthereumMessage *message);

const char *
messageGetAnyIdentifierName (BREthereumMessage *message);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_Message_H */
