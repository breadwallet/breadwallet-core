//
//  BREthereumMessageDIS.h
//  BRCore
//
//  Created by Ed Gamble on 9/1/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Message_DIS_H
#define BR_Ethereum_Message_DIS_H

#include "support/BRKey.h"
#include "../BREthereumLESBase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Node DIScovery (apparently V4) defines four message types: PING, PONG, FIND_NEIGHBORS and
 * NEIGHBORS.  Note that the message identifier of '0x00' is reserved.  This DIS identifier of
 * 0x00, ..., 0x04 overlap with the P2P identifier; however, the DIS identifier are used on the
 * UDP route (and PIP identifiers are use don the TCP route).
 */
typedef enum {
    /* No 0x00 value; caution on indexing */
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
    
    /** The Ping hash (from the DIS header) is used in the Pong reply. We record it here */
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
    BRKey key; // a 64 byte public key
} BREthereumDISNeighbor;

extern BREthereumHash
neighborDISHash (BREthereumDISNeighbor neighbor);

/**
 * The DIS Neighbor Distance
 *
 * https://github.com/ethereum/devp2p/blob/master/discv4.md
 * distance(n₁, n₂) = keccak256(n₁) XOR keccak256(n₂)
 * "The 'distance' between two node IDs is the bitwise exclusive or on the hashes of the public
 * keys, taken as the number."
 *
 * @param n1 The DISNeighbor, n1, w/ nodeID
 * @param n2 The DISNeighbor, n2, w/ nodeID
 *
 * @return The distance as a UInt256
 */
extern UInt256
neighborDISDistance (BREthereumDISNeighbor n1,
                     BREthereumDISNeighbor n2);

// enode://<nodeid>@<ip>:<port>
#define NEIGHBOR_DIS_ENODE_STRLEN   (8 + 2 * 64 + 1 + 39 + 1 + 5 + 1)
typedef struct {
    char chars[NEIGHBOR_DIS_ENODE_STRLEN];
} BREthereumDISNeighborEnode;

extern BREthereumDISNeighborEnode
neighborDISAsEnode (BREthereumDISNeighbor neighbor,
                    int useTCP);


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
 * the key doesn't sit here-and-there in memory.
 *
 * Confirm but... the `privateKeyForSigning` has NOTHING to do with the User's paperKey.
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

extern BRRlpItem
messageDISEncode (BREthereumDISMessage message,
                  BREthereumMessageCoder coder);

extern BREthereumDISMessage
messageDISDecode (BRRlpItem item,
                  BREthereumMessageCoder coder);

extern void
messageDISRelease (BREthereumDISMessage *message);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Message_DIS_H */
