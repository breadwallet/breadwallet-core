//
//  BREthereumMessageDIS.c
//  Core
//
//  Created by Ed Gamble on 9/1/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <sys/socket.h>
#include <assert.h>
#include "support/BRKey.h"
#include "support/BRInt.h"
#include "support/BRAssert.h"
#include "ethereum/base/BREthereumSignature.h"
#include "BREthereumMessageDIS.h"

// #define NEED_TO_PRINT_DIS_NEIGHBOR_DETAILS

/// MARK: UInt512

typedef enum {
    INT_BITWISE_AND,
    INT_BITWISE_OR,
    INT_BITWISE_XOR
} BREthereumIntBitwiseType;

static UInt256
uint256Bitwise (BREthereumIntBitwiseType type,
                UInt256 *x,
                UInt256 *y) {
    UInt256 z = UINT256_ZERO;
    for (size_t i = 0; i < sizeof(UInt256)/sizeof(uint64_t); i++)
        switch (type) {
            case INT_BITWISE_AND:
                z.u64[i] = x->u64[i] & y->u64[i];
                break;
            case INT_BITWISE_OR:
                z.u64[i] = x->u64[i] | y->u64[i];
                break;
            case INT_BITWISE_XOR:
                z.u64[i] = x->u64[i] ^ y->u64[i];
                break;
        }
    return z;
}

#if 0
static int
uint256BitOffset (UInt256 x) {
    for (unsigned int index = 0; index < 256; index++)
        if (0x01 & (x.u64[index/64] >> (index % 64)))
            return index;
    return -1;
}

static int
uint64tBitCount (uint64_t val) {
    int count = 0;
    for (int i = 0; i < 8 * sizeof (uint64_t); i++) {
        count += (val & 0x01);
        val >>= 1;
    }
    return count;
}
static int
uint512BitCount (UInt512 val) {
    int count = 0;
    for (size_t i = 0; i < sizeof(UInt512)/sizeof(uint64_t); i++)
        count += uint64tBitCount(val.u64[i]);
    return count;
}
#endif

static const char *messageDISNames[] = {
    NULL,
    "Ping",
    "Pong",
    "FindNeighbors",
    "Neighbors" };

extern const char *
messageDISGetIdentifierName (BREthereumDISMessageIdentifier identifier) {
    return messageDISNames [identifier];
}

//
// DIS Endpoint Encode/Decode
//
extern BRRlpItem
endpointDISEncode (const BREthereumDISEndpoint *endpoint, BRRlpCoder coder) {
    return rlpEncodeList (coder, 3,
                          (endpoint->domain == AF_INET
                           ? rlpEncodeBytes (coder, (uint8_t*) endpoint->addr.ipv4, 4)
                           : rlpEncodeBytes (coder, (uint8_t*) endpoint->addr.ipv6, 16)),
                          rlpEncodeUInt64 (coder, endpoint->portUDP, 1),
                          rlpEncodeUInt64 (coder, endpoint->portTCP, 1));
}

extern BREthereumDISEndpoint
endpointDISDecode (BRRlpItem item, BRRlpCoder coder) {
    BREthereumDISEndpoint endpoint = {};

    // Zero out - we'll be hashing this and don't need random, untouched memory.
    memset (&endpoint, 0, sizeof (BREthereumDISEndpoint));

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);
    if (3 != itemsCount) { rlpCoderSetFailed (coder); return endpoint; }

    BRRlpData addrData = rlpDecodeBytesSharedDontRelease (coder, items[0]);
    if (4 != addrData.bytesCount && 16 != addrData.bytesCount)  { rlpCoderSetFailed (coder); return endpoint; }

    endpoint.domain = (4 == addrData.bytesCount ? AF_INET : AF_INET6);
    memcpy ((endpoint.domain == AF_INET ? endpoint.addr.ipv4 : endpoint.addr.ipv6),
            addrData.bytes,
            addrData.bytesCount);

    endpoint.portUDP = (uint16_t) rlpDecodeUInt64 (coder, items[1], 1);
    endpoint.portTCP = (uint16_t) rlpDecodeUInt64 (coder, items[2], 1);

    return endpoint;
}

/// MARK: - DIS Neighbor

static BREthereumDISNeighbor
neighborDISDecode (BRRlpItem item, BREthereumMessageCoder coder) {
    BREthereumDISNeighbor neighbor = {};

    // Zero out - we'll be hashing this and don't need random, untouched memory.
    memset (&neighbor, 0, sizeof (BREthereumDISNeighbor));

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    if (4 != itemsCount) { rlpCoderSetFailed (coder.rlp); return neighbor; }

    // Node ID
    // Endpoint - Somehow GETH explodes it.
    BRRlpData addrData = rlpDecodeBytesSharedDontRelease (coder.rlp, items[0]);
    if (4 != addrData.bytesCount && 16 != addrData.bytesCount) { rlpCoderSetFailed (coder.rlp); return neighbor; }

    neighbor.node.domain = (4 == addrData.bytesCount ? AF_INET : AF_INET6);
    memcpy ((neighbor.node.domain == AF_INET ? neighbor.node.addr.ipv4 : neighbor.node.addr.ipv6),
            addrData.bytes,
            addrData.bytesCount);

    BRRlpData nodeIDData = rlpDecodeBytesSharedDontRelease (coder.rlp, items[3]);
    if (64 != nodeIDData.bytesCount) { rlpCoderSetFailed (coder.rlp); return neighbor; }

    // Get the 65-byte 0x04-prefaced public key.
    uint8_t key[65] = { 0x04 };
    memcpy (&key[1], nodeIDData.bytes, nodeIDData.bytesCount);

    memset (&neighbor.key, 0, sizeof (BRKey));
    BRKeySetPubKey(&neighbor.key, key, 65);

    neighbor.node.portUDP = (uint16_t) rlpDecodeUInt64 (coder.rlp, items[1], 1);
    neighbor.node.portTCP = (uint16_t) rlpDecodeUInt64 (coder.rlp, items[2], 1);

    return neighbor;
}

extern BREthereumHash
neighborDISHash (BREthereumDISNeighbor neighbor) {
    BRRlpData data = { sizeof (BREthereumDISNeighbor), (uint8_t *) &neighbor };
    return hashCreateFromData(data);
}

extern UInt256
neighborDISDistance (BREthereumDISNeighbor n1,
                     BREthereumDISNeighbor n2) {
    // For each `n` we could perform:
    //   UInt512 *val1 = (UInt512*) &n1.key.pubKey[1];
    // but that produces a 'misaligned address error.  So, instead, we'll do something that is
    // clear and correct but a little tiny bit inefficient.
    UInt512 val1, val2;

    assert (64 == sizeof(UInt512));
    memcpy (val1.u8, &n1.key.pubKey[1], 64);
    memcpy (val2.u8, &n2.key.pubKey[1], 64);

    // This hash is a 'Keccak256' hash.
    BREthereumHash hash1 = hashCreateFromData((BRRlpData) { 64, val1.u8});
    BREthereumHash hash2 = hashCreateFromData((BRRlpData) { 64, val2.u8});

    return uint256Bitwise (INT_BITWISE_XOR,
                           (UInt256*) hash1.bytes,
                           (UInt256*) hash2.bytes);
}

extern BREthereumDISNeighborEnode
neighborDISAsEnode (BREthereumDISNeighbor neighbor,
                    int useTCP) {
    BREthereumDISNeighborEnode enode = { '\0' };

    char nodeID [129];
    encodeHex(nodeID, 129, &neighbor.key.pubKey[1], 64);

    char IP [39];
    if (neighbor.node.domain == AF_INET)
        sprintf (IP, "%d.%d.%d.%d",
                 neighbor.node.addr.ipv4[0],
                 neighbor.node.addr.ipv4[1],
                 neighbor.node.addr.ipv4[2],
                 neighbor.node.addr.ipv4[3]);
    else
        sprintf (IP, "%02d%02d:%02d%02d:%02d%02d:%02d%02d:%02d%02d:%02d%02d:%02d%02d:%02d%02d",
                 neighbor.node.addr.ipv6[0],
                 neighbor.node.addr.ipv6[1],
                 neighbor.node.addr.ipv6[2],
                 neighbor.node.addr.ipv6[3],
                 neighbor.node.addr.ipv6[4],
                 neighbor.node.addr.ipv6[5],
                 neighbor.node.addr.ipv6[6],
                 neighbor.node.addr.ipv6[7],
                 neighbor.node.addr.ipv6[8],
                 neighbor.node.addr.ipv6[9],
                 neighbor.node.addr.ipv6[10],
                 neighbor.node.addr.ipv6[11],
                 neighbor.node.addr.ipv6[12],
                 neighbor.node.addr.ipv6[13],
                 neighbor.node.addr.ipv6[14],
                 neighbor.node.addr.ipv6[15]);

    sprintf (enode.chars, "enode://%s@%s:%d", nodeID, IP,
             (useTCP ? neighbor.node.portTCP : neighbor.node.portUDP));

    return enode;
}

/// MARK: DIS Ping

extern BREthereumDISMessagePing
messageDISPingCreate (BREthereumDISEndpoint to,
                      BREthereumDISEndpoint from,
                      uint64_t expiration) {
    return (BREthereumDISMessagePing) { P2P_MESSAGE_VERSION, to, from, expiration };
}

static BRRlpItem
messageDISPingEncode (BREthereumDISMessagePing message, BREthereumMessageCoder coder) {
    return rlpEncodeList (coder.rlp, 4,
                          rlpEncodeUInt64 (coder.rlp, message.version, 1),
                          endpointDISEncode (&message.from, coder.rlp),
                          endpointDISEncode (&message.to, coder.rlp),
                          rlpEncodeUInt64 (coder.rlp, message.expiration, 1));
}

static BREthereumDISMessagePing
messageDISPingDecode (BRRlpItem item, BREthereumMessageCoder coder, BREthereumHash hash, int releaseItem) {
    BREthereumDISMessagePing ping = {};

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    if (4 != itemsCount) rlpCoderSetFailed (coder.rlp);
    else {
        ping.version = (int) rlpDecodeUInt64 (coder.rlp, items[0], 1);
        ping.from = endpointDISDecode (items[1], coder.rlp);
        ping.to   = endpointDISDecode (items[2], coder.rlp);
        ping.expiration = rlpDecodeUInt64 (coder.rlp, items[3], 1);

        ping.hash = hash;
    }
    if (releaseItem) rlpReleaseItem (coder.rlp, item);
    return ping;
}

/// MARK: DIS Pong

extern BREthereumDISMessagePong
messageDISPongCreate (BREthereumDISEndpoint to,
                      BREthereumHash pingHash,
                      uint64_t expiration) {
    return (BREthereumDISMessagePong) { to, pingHash, expiration };
}

static BRRlpItem
messageDISPongEncode (BREthereumDISMessagePong message, BREthereumMessageCoder coder) {
    return rlpEncodeList (coder.rlp, 3,
                          endpointDISEncode (&message.to, coder.rlp),
                          hashRlpEncode (message.pingHash, coder.rlp),
                          rlpEncodeUInt64 (coder.rlp, message.expiration, 1));
}

static BREthereumDISMessagePong
messageDISPongDecode (BRRlpItem item, BREthereumMessageCoder coder, int releaseItem) {
    BREthereumDISMessagePong pong = {};

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    if (3 != itemsCount) rlpCoderSetFailed(coder.rlp);
    else {
        pong.to = endpointDISDecode (items[0], coder.rlp);
        pong.pingHash = hashRlpDecode (items[1], coder.rlp);
        pong.expiration = rlpDecodeUInt64 (coder.rlp, items[2], 1);
    }

    if (releaseItem) rlpReleaseItem (coder.rlp, item);
    return pong;
}

/// MARK: DIS Find Neighbors

extern BREthereumDISMessageFindNeighbors
messageDISFindNeighborsCreate (BRKey target,
                               uint64_t expiration) {
    return (BREthereumDISMessageFindNeighbors) { target, expiration };
}

static BRRlpItem
messageDISFindNeighborsEncode (BREthereumDISMessageFindNeighbors message, BREthereumMessageCoder coder) {
    assert (!message.target.compressed);

    uint8_t pubKey[65];
    BRKeyPubKey (&message.target, pubKey, 65);

    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeBytes (coder.rlp, &pubKey[1], 64),
                           rlpEncodeUInt64 (coder.rlp, message.expiration, 1));
}

static BREthereumDISMessageNeighbors
messageDISNeighborsDecode (BRRlpItem item, BREthereumMessageCoder coder, int releaseItem) {
    BREthereumDISMessageNeighbors message = {};

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    if (2 != itemsCount) rlpCoderSetFailed(coder.rlp);
    else {
        size_t neighborsCount = 0;
        const BRRlpItem *neighborItems = rlpDecodeList (coder.rlp, items[0], &neighborsCount);

        array_new(message.neighbors, neighborsCount);
        for (size_t index = 0; index < neighborsCount; index++)
            array_add (message.neighbors,
                       neighborDISDecode (neighborItems[index], coder));

#if defined (NEED_TO_PRINT_DIS_NEIGHBOR_DETAILS)
        eth_log (LES_LOG_TOPIC, "Neighbors: %s", "");
        for (size_t index = 0; index < neighborsCount; index++) {
            BREthereumDISNeighbor neighbor = message.neighbors[index];
            eth_log (LES_LOG_TOPIC, "    IP: %3d.%3d.%3d.%3d, UDP: %6d, TCP: %6d",
                     neighbor.node.addr.ipv4[0],
                     neighbor.node.addr.ipv4[1],
                     neighbor.node.addr.ipv4[2],
                     neighbor.node.addr.ipv4[3],
                     neighbor.node.portUDP,
                     neighbor.node.portTCP);
        }
#endif

        message.expiration = rlpDecodeUInt64 (coder.rlp, items[1], 1);
    }

    if (releaseItem) rlpReleaseItem (coder.rlp, item);

    return message;
}

// https://github.com/ethereum/devp2p/blob/master/discv4.md
// Node discovery messages are sent as UDP datagrams. The maximum size of any packet is 1280 bytes.
//    packet = packet-header || packet-data
// Every packet starts with a header:
//    packet-header = hash || signature || packet-type
//    hash = keccak256(signature || packet-type || packet-data)
//    signature = sign(packet-type || packet-data)

typedef struct {
    BREthereumHash hash;
    BREthereumSignatureRSV signature;
    uint8_t identifier;
    uint8_t data[0];  // 'Pro Skill'
} BREthereumDISMessagePacket;

#if defined (__ANDROID__)
#elif defined (static_assert)
static_assert (98 == sizeof (BREthereumDISMessagePacket), "BREthereumDISMessagePacket must be 98 bytes");
#else
#warning "Missed static_assert()"
#endif

extern BREthereumDISMessage
messageDISDecode (BRRlpItem item,
                  BREthereumMessageCoder coder) {
    // ITEM is encoded bytes as (hash || signature || identifier || data)
    BRRlpData packetData = rlpDecodeBytesSharedDontRelease (coder.rlp, item);

    // Overaly packet on the packetData.  We've constructed BREthereumDISMessagePacket so as
    // to make this viable.
    BREthereumDISMessagePacket *packet = (BREthereumDISMessagePacket*) packetData.bytes;
    size_t packetSize = sizeof (BREthereumDISMessagePacket);

    // Get the identifier and then decode the message contents
    BREthereumDISMessageIdentifier identifier = packet->identifier;

    // Perform the most basic validation - just of identifier
    if (DIS_MESSAGE_PING != identifier &&
        DIS_MESSAGE_PONG != identifier &&
        DIS_MESSAGE_FIND_NEIGHBORS != identifier &&
        DIS_MESSAGE_NEIGHBORS != identifier) {
        rlpCoderHasFailed (coder.rlp);
        return (BREthereumDISMessage) { (BREthereumDISMessageIdentifier) NULL };
    }

    // TODO: Use packet->hash + packet->signature to validate the packet.

    // Get the rlpItem from packet->data
    BRRlpData messageData = { packetData.bytesCount - packetSize, &packetData.bytes[packetSize] };
    BRRlpItem messageItem = rlpGetItem (coder.rlp, messageData);

    switch (identifier) {
        case DIS_MESSAGE_PONG:
            return (BREthereumDISMessage) {
                DIS_MESSAGE_PONG,
                { .pong = messageDISPongDecode (messageItem, coder, 1) }
            };

        case DIS_MESSAGE_NEIGHBORS:
            return (BREthereumDISMessage) {
                DIS_MESSAGE_NEIGHBORS,
                { .neighbors = messageDISNeighborsDecode (messageItem, coder, 1) }
            };

        case DIS_MESSAGE_PING:
            return (BREthereumDISMessage) {
                DIS_MESSAGE_PING,
                { .ping = messageDISPingDecode (messageItem, coder, packet->hash, 1) }
            };

        case DIS_MESSAGE_FIND_NEIGHBORS:
            // We never expect this message - node sending will get disconnected
            return (BREthereumDISMessage) {
                DIS_MESSAGE_FIND_NEIGHBORS,
                { .findNeighbors = {} }
            };
    }
}

extern BRRlpItem
messageDISEncode (BREthereumDISMessage message,
                  BREthereumMessageCoder coder) {
    BRRlpItem bodyItem;

    switch (message.identifier) {
        case DIS_MESSAGE_PING:
            bodyItem = messageDISPingEncode (message.u.ping, coder);
            break;

        case DIS_MESSAGE_FIND_NEIGHBORS:
            bodyItem = messageDISFindNeighborsEncode (message.u.findNeighbors, coder);
            break;

        case DIS_MESSAGE_PONG:
            bodyItem = messageDISPongEncode (message.u.pong, coder);
            break;

        case DIS_MESSAGE_NEIGHBORS:
            BRFail();
    }
    BRKey key = message.privateKeyForSigning;

    BRRlpData data = rlpGetDataSharedDontRelease (coder.rlp, bodyItem);

    // We are producing a 'packet' which as described above is a concatenation of a header and
    // data where the header IS NOT rlp encoded but the data IS rlp encoded.  We will take the
    // following approach: RLP encode the data (which is above as `bodyItem`), populate the header,
    // concatenate the head with the data (bytes), AND THEN, RLP encode the whole thing as bytes.

    // Then, like our other 'send' routines - we strip off the RLP length.  Why?  We'll all guess...
    // Because 'the Ethereum guys' want to save max 9 bytes (typically 2-4) over RLP consistency?

    // Given the data.bytesCount we can stack allocate all the bytes we'll need for the packet.
    // Somewhere in Ethereum documentation, it states that DIS messages (using UDP) are limited to
    // ~1500 bytes.
    assert (data.bytesCount < 8 * 1024);

    // Okay, the stack allocation.
    size_t packetSize = sizeof (BREthereumDISMessagePacket) + data.bytesCount;
    uint8_t packetMemory[packetSize];
    memset (packetMemory, 0x00, packetSize);

    // Overlay a packet on the packetMemory;
    BREthereumDISMessagePacket *packet = (BREthereumDISMessagePacket*) packetMemory;

    // Fill in the identifier and the data
    memcpy (packet->data, data.bytes, data.bytesCount);
    packet->identifier = message.identifier;

    // Compute the signature over (identifier || data)
    size_t signatureSize = 65;
    BRRlpData signatureData = { 1 + data.bytesCount, (uint8_t *) &packet->identifier };
    packet->signature = signatureCreate (SIGNATURE_TYPE_RECOVERABLE_RSV,
                                         signatureData.bytes, signatureData.bytesCount,
                                         key).sig.rsv;

    // Compute the hash over ( signature || identifier || data )
    BRRlpData hashData = { signatureSize + 1 + data.bytesCount, (uint8_t*) &packet->signature };
    packet->hash = hashCreateFromData (hashData);

#if defined (NEED_TO_PRINT_DIS_HEADER_DETAILS)
    {
        size_t ignore;
        printf ("DATA     : %s\n", encodeHexCreate(&ignore, data.bytes, data.bytesCount));
        printf ("SIG      : %s\n", encodeHexCreate(&ignore, (uint8_t*) &packet->signature, 65));

        printf ("HASH DATA: %s\n", encodeHexCreate(&ignore, hashData.bytes, hashData.bytesCount));
        printf ("HASH RSLT: %s\n", hashAsString(packet->hash));
        printf ("PACKET: %s\n", encodeHexCreate(&ignore, packetMemory, packetSize));
    }
#endif
    rlpReleaseItem(coder.rlp, bodyItem);

    // Now, finally, `packet` and `packetMemory` are complete.
    return rlpEncodeBytes (coder.rlp, packetMemory, packetSize);
}

extern void
messageDISRelease (BREthereumDISMessage *message) {
    switch (message->identifier) {
        case DIS_MESSAGE_PING:
        case DIS_MESSAGE_PONG:
        case DIS_MESSAGE_FIND_NEIGHBORS:
            break;
        case DIS_MESSAGE_NEIGHBORS:
            if (NULL != message->u.neighbors.neighbors) array_free (message->u.neighbors.neighbors);
            break;
    }
}
