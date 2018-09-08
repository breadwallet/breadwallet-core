//
//  BREthereumMessageDIS.c
//  Core
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
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <sys/socket.h>
#include "BRKey.h"
#include "../../base/BREthereumSignature.h"
#include "BREthereumMessageDIS.h"

// #define NEED_TO_PRINT_DIS_NEIGHBOR_DETAILS

static const char *messageDISNames[] = { NULL, "Ping", "Pong", "FindNeighbors", "Neighbors" };

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
    BREthereumDISEndpoint endpoint;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);
    assert (3 == itemsCount);

    BRRlpData addrData = rlpDecodeBytesSharedDontRelease (coder, items[0]);
    assert (4 == addrData.bytesCount || 16 == addrData.bytesCount);
    endpoint.domain = (4 == addrData.bytesCount ? AF_INET : AF_INET6);
    memcpy ((endpoint.domain == AF_INET ? endpoint.addr.ipv4 : endpoint.addr.ipv6),
            addrData.bytes,
            addrData.bytesCount);

    endpoint.portUDP = (uint16_t) rlpDecodeUInt64 (coder, items[1], 1);
    endpoint.portTCP = (uint16_t) rlpDecodeUInt64 (coder, items[2], 1);

    return endpoint;
}

//
// DIS Neighbor Encode/Decode
//
static BREthereumDISNeighbor
neighborDISDecode (BRRlpItem item, BREthereumMessageCoder coder) {
    BREthereumDISNeighbor neighbor;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    assert (4 == itemsCount);

    // Node ID
    // Endpoint - Somehow GETH explodes it.
    BRRlpData addrData = rlpDecodeBytesSharedDontRelease (coder.rlp, items[0]);
    assert (4 == addrData.bytesCount || 16 == addrData.bytesCount);
    neighbor.node.domain = (4 == addrData.bytesCount ? AF_INET : AF_INET6);
    memcpy ((neighbor.node.domain == AF_INET ? neighbor.node.addr.ipv4 : neighbor.node.addr.ipv6),
            addrData.bytes,
            addrData.bytesCount);

    BRRlpData nodeIDData = rlpDecodeBytesSharedDontRelease (coder.rlp, items[3]);
    assert (64 == nodeIDData.bytesCount);
    memcpy (neighbor.nodeID.u8, nodeIDData.bytes, nodeIDData.bytesCount);

    neighbor.node.portUDP = (uint16_t) rlpDecodeUInt64 (coder.rlp, items[1], 1);
    neighbor.node.portTCP = (uint16_t) rlpDecodeUInt64 (coder.rlp, items[2], 1);

    return neighbor;
}

extern BREthereumDISMessagePing
messageDISPingCreate (BREthereumDISEndpoint to,
                      BREthereumDISEndpoint from,
                      uint64_t expiration) {
    return (BREthereumDISMessagePing) { 4, to, from, expiration };
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
    BREthereumDISMessagePing ping;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    assert (4 == itemsCount);

    ping.version = (int) rlpDecodeUInt64 (coder.rlp, items[0], 1);
    ping.from = endpointDISDecode (items[1], coder.rlp);
    ping.to   = endpointDISDecode (items[2], coder.rlp);
    ping.expiration = rlpDecodeUInt64 (coder.rlp, items[3], 1);

    ping.hash = hash;

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
    BREthereumDISMessagePong pong;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    pong.to = endpointDISDecode (items[0], coder.rlp);
    pong.pingHash = hashRlpDecode (items[1], coder.rlp);
    pong.expiration = rlpDecodeUInt64 (coder.rlp, items[2], 1);

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
    BREthereumDISMessageNeighbors message;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    assert (2 == itemsCount);

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

static_assert (98 == sizeof (BREthereumDISMessagePacket), "BREthereumDISMessagePacket must be 98 bytes");

extern BREthereumDISMessage
messageDISDecode (BRRlpItem item,
                  BREthereumMessageCoder coder) {
    // ITEM is encoded bytes as (hash || signature || identifier || data)
    BRRlpData packetData = rlpDecodeBytesSharedDontRelease (coder.rlp, item);

    // Overaly packet on the packetData.  We've constructed BREthereumDISMessagePacket so as
    // to make this viable.
    BREthereumDISMessagePacket *packet = (BREthereumDISMessagePacket*) packetData.bytes;
    size_t packetSize = sizeof (BREthereumDISMessagePacket);

    // TODO: Use packet->hash + packet->signature to validate the packet.

    // Get the identifier and then decode the message contents
    BREthereumDISMessageIdentifier identifier = packet->identifier;

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
            assert (0);
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
            assert (0);
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
    // Somewhere in Ethereum documentation, is states that DIS messages (using UDP) are limited to
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

