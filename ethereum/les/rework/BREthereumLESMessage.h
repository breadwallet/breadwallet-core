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
#include "BRArray.h"
#include "BRSet.h"
#include "../../base/BREthereumBase.h"
#include "../../blockchain/BREthereumBlockChain.h"

#define BRArrayOf(type)    type*
#define BRSetOf(type)      BRSet*

#define LES_IDENTIFIER_OFFSET_DEAL_WITH_IT   (0x10)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MESSAGE_P2P   = 0x00,
    MESSAGE_ETH   = 0x01,
    MESSAGE_LES   = 0x02,
    MESSAGE_DIS   = 0x03
} BREthereumMessageIdentifier;

/// MARK: - P2P (Peer-to-Peer) Messages

/**
 *
 */
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
messageP2PHelloEncode (BREthereumP2PMessageHello message, BRRlpCoder coder);

extern BREthereumP2PMessageHello
messageP2PHelloDecode (BRRlpItem item, BRRlpCoder coder);

extern void
messageP2PHelloShow (BREthereumP2PMessageHello hello);

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
                  BRRlpCoder coder);

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
 * NEIGHBORS.
 */
typedef enum {
    DIS_MESSAGE_PING           = 0x01,
    DIS_MESSAGE_PONG           = 0x02,
    DIS_MESSAGE_FIND_NEIGHBORS = 0x03,
    DIS_MESSAGE_NEIGHBORS      = 0x04
} BREthereumDISMessageIdentifier;

extern const char *
messageDISGetIdentifierName (BREthereumDISMessageIdentifier identifier);

/**
 * A DIS Endpoint is commonly used to identify an INET address.  Sometimes this is provided
 * to identify the sender and, most importantly, to identify peers.
 */
typedef struct {
    /** isIPV4 identies the `addr` type and is ascertained from the RLP encoding as 4 or 16 */
    int isIPV4;

    /** The IPV address - 4 bytes for ivp4; 16 bytes for ivp6 */
    union {
        uint8_t ipv4[4];
        uint8_t ipv6[16];
    } addr;

    /** The UDP port */
    uint16_t portUDP;

    /** The TCP port */
    uint16_t portTCP;
} BREthereumDISEndpoint;

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
    LES_MESSAGE_CONTRACT_CODE      = 0x0b,
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
messageLESStatusEncode (BREthereumLESMessageStatus *status, BRRlpCoder coder);

extern BREthereumLESMessageStatus
messageLESStatusDecode (BRRlpItem item, BRRlpCoder coder);

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
 */
typedef struct {
    // [+0x02, reqID: P, [block: { P , B_32 }, maxHeaders: P, skip: P, reverse: P in { 0 , 1 } ]]
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
                              BREthereumHash hash,
                              uint32_t maxHeaders,
                              uint64_t skip,
                              uint8_t  reverse);

// TODO: Include `reqId` or not?  Include `msgId` (w/ offset) or not?  Depends on encryption...
extern BRRlpItem
messageLESGetBlockHeadersEncode (BREthereumLESMessageGetBlockHeaders message,
                                 BRRlpCoder coder);

/// MARK: LES Block Headers

/**
 * A LES Block Header Message ...
 */
typedef struct {
    // [+0x03, reqID: P, BV: P, [blockHeader_0, blockHeader_1, ...]]
    uint64_t reqId;
    uint64_t bv;
    BRArrayOf(BREthereumBlockHeader) headers;
} BREthereumLESMessageBlockHeaders;

// TODO: Include `reqId` or not?  Include `msgId` (w/ offset) or not?  Depends on encryption...
extern BREthereumLESMessageBlockHeaders
messageLESBlockHeadersDecode (BRRlpItem item,
                              BRRlpCoder coder);

//
// ...
//

/// MARK: LES Message

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
        // ...
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
                  BRRlpCoder coder,
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
                  BRRlpCoder coder);

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
               BRRlpCoder coder);

extern BREthereumMessage
messageDecode (BRRlpItem item,
               BRRlpCoder coder,
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
