//
//  BREthereumLESMessage.c
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

#include <assert.h>
#include <sys/socket.h>
#include "BREthereumLESMessage.h"

// GETH Limits
// MaxHeaderFetch           = 192 // Amount of block headers to be fetched per retrieval request
// MaxBodyFetch             = 32  // Amount of block bodies to be fetched per retrieval request
// MaxReceiptFetch          = 128 // Amount of transaction receipts to allow fetching per request
// MaxCodeFetch             = 64  // Amount of contract codes to allow fetching per request
// MaxProofsFetch           = 64  // Amount of merkle proofs to be fetched per retrieval request
// MaxHelperTrieProofsFetch = 64  // Amount of merkle proofs to be fetched per retrieval request
// MaxTxSend                = 64  // Amount of transactions to be send per request
// MaxTxStatus              = 256 // Amount of transactions to queried per request

// #define NEED_TO_PRINT_DIS_NEIGHBOR_DETAILS

static BREthereumLESMessageStatusKey lesMessageStatusKeys[] = {
    "protocolVersion",
    "networkId",
    //...
    "flowControl/MRR"
};

/// MARK: - P2P (Peer-to-Peer) Messages

static const char *messageP2PNames[] = { "Hello", "Disconnect", "Ping", "Pong" };

extern const char *
messageP2PGetIdentifierName (BREthereumP2PMessageIdentifier identifier) {
    return messageP2PNames [identifier];
}

//
// P2P Hello
//
extern BRRlpItem
messageP2PHelloEncode (BREthereumP2PMessageHello message, BREthereumMessageCoder coder) {
    size_t capsCount = array_count(message.capabilities);
    BRRlpItem capItems[capsCount];

    for(int i = 0; i < capsCount; ++i)
        capItems[i] = rlpEncodeList (coder.rlp, 2,
                                     rlpEncodeString (coder.rlp, message.capabilities[i].name),
                                     rlpEncodeUInt64 (coder.rlp, message.capabilities[i].version, 1));

    BRRlpItem capsItem = rlpEncodeListItems(coder.rlp, capItems, capsCount);

    return rlpEncodeList (coder.rlp, 5,
                          rlpEncodeUInt64 (coder.rlp, message.version, 1),
                          rlpEncodeString (coder.rlp, message.clientId),
                          capsItem,
                          rlpEncodeUInt64 (coder.rlp, 0x00, 1),
                          rlpEncodeBytes (coder.rlp, message.nodeId.u8, sizeof (message.nodeId.u8)));
}

extern BREthereumP2PMessageHello
messageP2PHelloDecode (BRRlpItem item, BREthereumMessageCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    assert (5 == itemsCount);

    BREthereumP2PMessageHello message = {
        rlpDecodeUInt64 (coder.rlp, items[0], 1),
        rlpDecodeString (coder.rlp, items[1]),
        NULL,
        rlpDecodeUInt64 (coder.rlp, items[3], 1),
    };

    BRRlpData nodeData = rlpDecodeBytesSharedDontRelease (coder.rlp, items[4]);
    assert (sizeof (message.nodeId.u8) == nodeData.bytesCount);
    memcpy (message.nodeId.u8, nodeData.bytes, nodeData.bytesCount);

    size_t capsCount = 0;
    const BRRlpItem *capItems = rlpDecodeList (coder.rlp, items[2], &capsCount);
    array_new (message.capabilities, capsCount);
    
    for (size_t index = 0; index < capsCount; index++) {
        size_t capCount;
        const BRRlpItem *caps = rlpDecodeList (coder.rlp, capItems[index], &capCount);
        assert (2 == capCount);

        BREthereumP2PCapability cap;

        char *name = rlpDecodeString (coder.rlp, caps[0]);
        assert (strlen (name) == 3);
        strncpy (cap.name, name, 3);
        cap.name[3] = '\0';

        cap.version = (uint32_t) rlpDecodeUInt64 (coder.rlp, caps[1], 1);

        array_add (message.capabilities, cap);
    }

    return message;
}

extern void
messageP2PHelloShow (BREthereumP2PMessageHello hello) {
    size_t nodeIdLen = 2 * sizeof (hello.nodeId.u8) + 1;
    char nodeId[nodeIdLen];
    encodeHex(nodeId, nodeIdLen, hello.nodeId.u8, sizeof (hello.nodeId.u8));

    eth_log (LES_LOG_TOPIC, "Hello%s", "");
    eth_log (LES_LOG_TOPIC, "    Version     : %llu", hello.version);
    eth_log (LES_LOG_TOPIC, "    ClientId    : %s",   hello.clientId);
    eth_log (LES_LOG_TOPIC, "    ListenPort  : %llu", hello.port);
    eth_log (LES_LOG_TOPIC, "    NodeId      : 0x%s", nodeId);
    eth_log (LES_LOG_TOPIC, "    Capabilities:%s", "");
    for (size_t index = 0; index < array_count(hello.capabilities); index++)
        eth_log (LES_LOG_TOPIC, "        %s = %u",
                 hello.capabilities[index].name,
                 hello.capabilities[index].version);
}

extern BREthereumBoolean
messageP2PCababilityEqual (const BREthereumP2PCapability *cap1,
                           const BREthereumP2PCapability *cap2) {
    return AS_ETHEREUM_BOOLEAN (0 == strcmp (cap1->name, cap2->name) &&
                                cap1->version == cap2->version);
}

extern BREthereumBoolean
messageP2PHelloHasCapability (const BREthereumP2PMessageHello *hello,
                              const BREthereumP2PCapability *capability) {
    for (size_t index = 0; index < array_count (hello->capabilities); index++)
        if (ETHEREUM_BOOLEAN_IS_TRUE(messageP2PCababilityEqual(capability, &hello->capabilities[index])))
            return ETHEREUM_BOOLEAN_TRUE;
    return ETHEREUM_BOOLEAN_FALSE;
}

extern const char *
messageP2PDisconnectDescription (BREthereumP2PDisconnectReason identifier);

//
static BREthereumP2PMessageDisconnect
messageP2PDisconnectDecode (BRRlpItem item, BREthereumMessageCoder coder) {
    BREthereumP2PMessageDisconnect disconnect;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    assert (1 == itemsCount);

    disconnect.reason = (BREthereumP2PDisconnectReason) rlpDecodeUInt64 (coder.rlp, items[0], 1);

    return disconnect;
}

//
// P2P
//
extern BRRlpItem
messageP2PEncode (BREthereumP2PMessage message, BREthereumMessageCoder coder) {
    BRRlpItem messageBody = NULL;
    switch (message.identifier) {
        case P2P_MESSAGE_HELLO:
            messageBody = messageP2PHelloEncode(message.u.hello, coder);
            break;
        case P2P_MESSAGE_PING:
        case P2P_MESSAGE_PONG:
            messageBody = rlpEncodeList (coder.rlp, 0);
            break;
        case P2P_MESSAGE_DISCONNECT:
            messageBody = rlpEncodeUInt64 (coder.rlp, message.u.disconnect.reason, 1);
            break;
    }
    BRRlpItem identifierItem = rlpEncodeUInt64 (coder.rlp, message.identifier, 1);

    return rlpEncodeList2 (coder.rlp, identifierItem, messageBody);
}

static BREthereumP2PMessage
messageP2PDecode (BRRlpItem item, BREthereumMessageCoder coder, BREthereumP2PMessageIdentifier identifer) {
    switch (identifer) {
        case P2P_MESSAGE_HELLO:
            return (BREthereumP2PMessage) {
                P2P_MESSAGE_HELLO,
                { .hello = messageP2PHelloDecode (item, coder) }
            };

        case P2P_MESSAGE_PING:
            return (BREthereumP2PMessage) {
                P2P_MESSAGE_PING,
                { .ping = {}}
            };

        case P2P_MESSAGE_PONG:
            return (BREthereumP2PMessage) {
                P2P_MESSAGE_PONG,
                { .pong = {} }
            };

        case P2P_MESSAGE_DISCONNECT:
            return (BREthereumP2PMessage) {
                P2P_MESSAGE_DISCONNECT,
                { .disconnect = messageP2PDisconnectDecode (item, coder) }
            };
    }
}

/// MARK: - DIS (Node Discovery) Messages

static const char *messageDISNames[] = { NULL, "Ping", "Pong", "FindNeighbors", "Neighbors" };

extern const char *
messageDISGetIdentifierName (BREthereumDISMessageIdentifier identifier) {
    return messageDISNames [identifier];
}

//
// DIS Endpoint Encode/Decode
//
static BRRlpItem
endpointDISEncode (const BREthereumDISEndpoint *endpoint, BREthereumMessageCoder coder) {
    return rlpEncodeList (coder.rlp, 3,
                          (endpoint->domain == AF_INET
                           ? rlpEncodeBytes (coder.rlp, (uint8_t*) endpoint->addr.ipv4, 4)
                           : rlpEncodeBytes (coder.rlp, (uint8_t*) endpoint->addr.ipv6, 16)),
                          rlpEncodeUInt64 (coder.rlp, endpoint->portUDP, 1),
                          rlpEncodeUInt64 (coder.rlp, endpoint->portTCP, 1));
}

static BREthereumDISEndpoint
endpointDISDecode (BRRlpItem item, BREthereumMessageCoder coder) {
    BREthereumDISEndpoint endpoint;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    BRRlpData addrData = rlpDecodeBytesSharedDontRelease (coder.rlp, items[0]);
    assert (4 == addrData.bytesCount || 16 == addrData.bytesCount);
    endpoint.domain = (4 == addrData.bytesCount ? AF_INET : AF_INET6);
    memcpy ((endpoint.domain == AF_INET ? endpoint.addr.ipv4 : endpoint.addr.ipv6),
            addrData.bytes,
            addrData.bytesCount);

    endpoint.portUDP = (uint16_t) rlpDecodeUInt64 (coder.rlp, items[1], 1);
    endpoint.portTCP = (uint16_t) rlpDecodeUInt64 (coder.rlp, items[2], 1);

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
                          endpointDISEncode (&message.from, coder),
                          endpointDISEncode (&message.to, coder),
                          rlpEncodeUInt64 (coder.rlp, message.expiration, 1));
}

static BREthereumDISMessagePing
messageDISPingDecode (BRRlpItem item, BREthereumMessageCoder coder, BREthereumHash hash, int releaseItem) {
    BREthereumDISMessagePing ping;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    assert (4 == itemsCount);

    ping.version = (int) rlpDecodeUInt64 (coder.rlp, items[0], 1);
    ping.from = endpointDISDecode (items[1], coder);
    ping.to   = endpointDISDecode (items[2], coder);
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
                          endpointDISEncode (&message.to, coder),
                          hashRlpEncode (message.pingHash, coder.rlp),
                          rlpEncodeUInt64 (coder.rlp, message.expiration, 1));
}

static BREthereumDISMessagePong
messageDISPongDecode (BRRlpItem item, BREthereumMessageCoder coder, int releaseItem) {
    BREthereumDISMessagePong pong;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    pong.to = endpointDISDecode (items[0], coder);
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

static BRRlpItem
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

/// MARK: - LES (Light Ethereum Subprotocol) Messages

// Static
BREthereumLESMessageSpec messageLESSpecs [NUMBER_OF_LES_MESSAGE_IDENTIFIERS] = {
    { "Status",           LES_MESSAGE_USE_STATUS           },
    { "Announce",         LES_MESSAGE_USE_STATUS           },
    { "GetBlockHeaders",  LES_MESSAGE_USE_REQUEST,    192  },
    { "BlockHeaders",     LES_MESSAGE_USE_RESPONSE         },
    { "GetBlockBodies",   LES_MESSAGE_USE_REQUEST,     32  },
    { "BlockBodies",      LES_MESSAGE_USE_RESPONSE         },
    { "GetReceipts",      LES_MESSAGE_USE_REQUEST,    128  },
    { "Receipts",         LES_MESSAGE_USE_RESPONSE         },
    { "GetProofs",        LES_MESSAGE_USE_REQUEST,     64  },
    { "Proofs",           LES_MESSAGE_USE_RESPONSE         },
    { "GetContractCodes", LES_MESSAGE_USE_REQUEST,     64  },
    { "ContractCodes",    LES_MESSAGE_USE_RESPONSE         },
    { "SendTx",           LES_MESSAGE_USE_STATUS,      64  }, // has cost, no response
    { "GetHeaderProofs",  LES_MESSAGE_USE_REQUEST,     64  },
    { "HeaderProofs",     LES_MESSAGE_USE_RESPONSE         },
    { "GetProofsV2",      LES_MESSAGE_USE_REQUEST,     64  },
    { "ProofsV2",         LES_MESSAGE_USE_RESPONSE         },
    { "GetHelperTrieProofs", LES_MESSAGE_USE_REQUEST,  64  },
    { "HelperTrieProofs",    LES_MESSAGE_USE_RESPONSE      },
    { "SendTx2",          LES_MESSAGE_USE_REQUEST,     64  },
    { "GetTxStatus",      LES_MESSAGE_USE_REQUEST,    256  },
    { "TxStatus",         LES_MESSAGE_USE_RESPONSE         }
};
extern const char *
messageLESGetIdentifierName (BREthereumLESMessageIdentifier identifer) {
    return messageLESSpecs[identifer].name;
}

#if 0
typedef BRRlpItem (*BREthereumLESMessageStatusValueRLPEncoder) (BREthereumLESMessageStatusValue value, BREthereumMessageCoder coder);
typedef BREthereumLESMessageStatusValue (*BREthereumLESMessageStatusValueRLPDecoder) (BRRlpItem item, BREthereumMessageCoder coder);

static BRRlpItem
messageStatusValueRlpEncodeNumber (BREthereumLESMessageStatusValue value, BREthereumMessageCoder coder) {
    return rlpEncodeUInt64(coder.rlp, value.number, 1);
}

static BREthereumLESMessageStatusValue
messageStatusValueRlpDecodeNumber (BRRlpItem item, BREthereumMessageCoder coder) {
    return (BREthereumLESMessageStatusValue) { .number = (uint32_t) rlpDecodeUInt64 (coder.rlp, item, 1) };
}

static BRRlpItem
messageStatusValueRlpEncodeBoolean (BREthereumLESMessageStatusValue value, BREthereumMessageCoder coder) {
    return rlpEncodeUInt64(coder.rlp, value.boolean, 1);
}

static BREthereumLESMessageStatusValue
messageStatusValueRlpDecodeBoolean(BRRlpItem item, BREthereumMessageCoder coder) {
    return (BREthereumLESMessageStatusValue) { .boolean = (int) rlpDecodeUInt64 (coder.rlp, item, 1) };
}

static BRRlpItem
messageStatusValueRlpEncodeBignum (BREthereumLESMessageStatusValue value, BREthereumMessageCoder coder) {
    return rlpEncodeUInt256(coder.rlp, value.bignum, 1);
}

static BREthereumLESMessageStatusValue
messageStatusValueRlpDecodeBignum(BRRlpItem item, BREthereumMessageCoder coder) {
    return (BREthereumLESMessageStatusValue) { .bignum = rlpDecodeUInt256 (coder.rlp, item, 1) };
}


static BRRlpItem
messageStatusValueRlpEncodeHash (BREthereumLESMessageStatusValue value, BREthereumMessageCoder coder) {
    return hashRlpEncode(value.hash, coder);
}

static BREthereumLESMessageStatusValue
messageStatusValueRlpDecodeHash(BRRlpItem item, BREthereumMessageCoder coder) {
    return (BREthereumLESMessageStatusValue) { .hash = hashRlpDecode (item, coder) };
}

struct {
    char *key;
    BREthereumLESMessageStatusValueRLPEncoder encoder;
    BREthereumLESMessageStatusValueRLPDecoder decoder;
} messageStatusKeyValueHandler[] = {
    { "protocolVersion", messageStatusValueRlpEncodeNumber,  messageStatusValueRlpDecodeNumber  },
    { "networkId",       messageStatusValueRlpEncodeNumber,  messageStatusValueRlpDecodeNumber  },
    { "headTd",          messageStatusValueRlpEncodeBignum,  messageStatusValueRlpDecodeBignum  },
    { "headHash",        messageStatusValueRlpEncodeHash,    messageStatusValueRlpDecodeHash    },
    { "headNum" },
    { "genesisHash", },
    { "announceType", },
};
extern BRRlpItem
messageLESStatusRLPEncode (BREthereumLESMessageStatus status, BREthereumMessageCoder coder) {
    size_t itemsCount = array_count(status.pairs);
    BRRlpItem items [itemsCount];

    for (size_t index = 0; index < itemsCount; index++) {
        BREthereumLESMessageStatusKeyValuePair pair = status.pairs[index];
        items[index] = rlpEncodeList2 (coder.rlp,
                                       rlpEncodeString (coder.rlp, (char *) pair.key),
                                       foo);
    }
}
#endif

//
// MARK: LES Status
//

extern BREthereumLESMessageStatus
messageLESStatusCreate (uint64_t protocolVersion,
                        uint64_t chainId,
                        uint64_t headNum,
                        BREthereumHash headHash,
                        UInt256 headTd,
                        BREthereumHash genesisHash,
                        uint64_t announceType) {
    return (BREthereumLESMessageStatus) {
        protocolVersion,            // 2
        chainId,
        headNum,
        headHash,
        headTd,
        genesisHash,

        ETHEREUM_BOOLEAN_FALSE,
        NULL,
        NULL,

        ETHEREUM_BOOLEAN_FALSE,

        NULL,
        NULL,
        NULL,
        NULL,

        announceType                // 1
    };
}

extern BRRlpItem
messageLESStatusEncode (BREthereumLESMessageStatus *status, BREthereumMessageCoder coder) {

    size_t index = 0;
    BRRlpItem items[15];

    items[index++] = rlpEncodeList2 (coder.rlp,
                                      rlpEncodeString(coder.rlp, "protocolVersion"),
                                      rlpEncodeUInt64(coder.rlp, status->protocolVersion,1));
    
    items[index++] = rlpEncodeList2 (coder.rlp,
                                      rlpEncodeString(coder.rlp, "networkId"),
                                      rlpEncodeUInt64(coder.rlp, status->chainId,1));

    items[index++] = rlpEncodeList2 (coder.rlp,
                                      rlpEncodeString(coder.rlp, "headTd"),
                                      rlpEncodeUInt256(coder.rlp, status->headTd,1));
    
    items[index++] = rlpEncodeList2(coder.rlp,
                                     rlpEncodeString(coder.rlp, "headHash"),
                                     hashRlpEncode(status->headHash, coder.rlp));
    
    items[index++] = rlpEncodeList2(coder.rlp,
                                     rlpEncodeString(coder.rlp, "headNum"),
                                     rlpEncodeUInt64(coder.rlp, status->headNum,1));
    
    items[index++] = rlpEncodeList2 (coder.rlp,
                                      rlpEncodeString(coder.rlp, "genesisHash"),
                                      hashRlpEncode(status->genesisHash, coder.rlp));
    
    if(ETHEREUM_BOOLEAN_IS_TRUE(status->serveHeaders))
        items[index++] = rlpEncodeList1 (coder.rlp,
                                          rlpEncodeString(coder.rlp, "serveHeaders"));

    if (status->serveChainSince != NULL)
        items[index++] = rlpEncodeList2 (coder.rlp,
                                          rlpEncodeString(coder.rlp, "serveChainSince"),
                                          rlpEncodeUInt64(coder.rlp, *(status->serveChainSince),1));

    if (status->serveStateSince != NULL)
        items[index++] = rlpEncodeList2 (coder.rlp,
                                          rlpEncodeString(coder.rlp, "serveStateSince"),
                                          rlpEncodeUInt64(coder.rlp, *(status->serveStateSince),1));

    if(ETHEREUM_BOOLEAN_IS_TRUE(status->txRelay))
        items[index++] = rlpEncodeList1 (coder.rlp, rlpEncodeString(coder.rlp, "txRelay"));

    //flowControl/BL
    if (status->flowControlBL != NULL)
        items[index++] = rlpEncodeList2 (coder.rlp,
                                          rlpEncodeString(coder.rlp, "flowControl/BL"),
                                          rlpEncodeUInt64(coder.rlp, *(status->flowControlBL),1));

    //flowControl/MRC
    if(status->flowControlBL != NULL) {
        size_t count = *(status->flowControlMRCCount);
        BRRlpItem mrcItems[count];

        for(int idx = 0; idx < count; ++idx){
            BRRlpItem mrcElements [3];
            mrcElements[0] = rlpEncodeUInt64(coder.rlp,status->flowControlMRC[idx].msgCode,1);
            mrcElements[1] = rlpEncodeUInt64(coder.rlp,status->flowControlMRC[idx].baseCost,1);
            mrcElements[2] = rlpEncodeUInt64(coder.rlp,status->flowControlMRC[idx].reqCost,1);
            mrcItems[idx] = rlpEncodeListItems(coder.rlp, mrcElements, 3);
        }

        items[index++] = rlpEncodeList2 (coder.rlp,
                                          rlpEncodeString(coder.rlp, "flowControl/MRC"),
                                          rlpEncodeListItems(coder.rlp, mrcItems, count));
    }
    //flowControl/MRR
    if (status->flowControlMRR != NULL)
        items[index++] = rlpEncodeList2 (coder.rlp,
                                          rlpEncodeString(coder.rlp, "flowControl/MRR"),
                                          rlpEncodeUInt64(coder.rlp, *(status->flowControlMRR),1));

    if (status->protocolVersion == 0x02)
        items[index++] = rlpEncodeList2 (coder.rlp,
                                          rlpEncodeString(coder.rlp, "announceType"),
                                          rlpEncodeUInt64(coder.rlp, status->announceType,1));

    return rlpEncodeListItems (coder.rlp, items, index);
}

extern BREthereumLESMessageStatus
messageLESStatusDecode (BRRlpItem item, BREthereumMessageCoder coder) {
    BREthereumLESMessageStatus status = {};

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);

    for(int i= 0; i < itemsCount; ++i) {
        size_t keyPairCount;
        const BRRlpItem *keyPairs = rlpDecodeList (coder.rlp, items[i], &keyPairCount);
        if (keyPairCount > 0) {
            char *key = rlpDecodeString(coder.rlp, keyPairs[0]);

            if (strcmp(key, "protocolVersion") == 0) {
                status.protocolVersion = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "networkId") == 0) {
                status.chainId = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "headTd") == 0) {
                status.headTd = rlpDecodeUInt256(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "headHash") == 0) {
                status.headHash = hashRlpDecode(keyPairs[1], coder.rlp);
            } else if (strcmp(key, "announceType") == 0) {
                status.announceType = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "headNum") == 0) {
                status.headNum = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "genesisHash") == 0) {
                status.genesisHash = hashRlpDecode(keyPairs[1], coder.rlp);
            } else if (strcmp(key, "serveHeaders") == 0) {
                status.serveHeaders = ETHEREUM_BOOLEAN_TRUE;
            } else if (strcmp(key, "serveChainSince") == 0) {
                status.serveChainSince = malloc(sizeof(uint64_t));
                *(status.serveChainSince) = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "serveStateSince") == 0) {
                status.serveStateSince = malloc(sizeof(uint64_t));
                *(status.serveStateSince) = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "txRelay") == 0) {
                status.txRelay = ETHEREUM_BOOLEAN_TRUE;
            } else if (strcmp(key, "flowControl/BL") == 0) {
                status.flowControlBL = malloc(sizeof(uint64_t));
                *(status.flowControlBL) = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "flowControl/MRC") == 0) {
                //status.flowControlMRR = malloc(sizeof(uint64_t));
                size_t mrrItemsCount  = 0;
                const BRRlpItem* mrrItems = rlpDecodeList(coder.rlp, keyPairs[1], &mrrItemsCount);
                BREthereumLESMessageStatusMRC *mrcs = NULL;
                if(mrrItemsCount > 0){
                    mrcs = (BREthereumLESMessageStatusMRC*) calloc (mrrItemsCount, sizeof(BREthereumLESMessageStatusMRC));
                    for(int mrrIdx = 0; mrrIdx < mrrItemsCount; ++mrrIdx){
                        size_t mrrElementsCount  = 0;
                        const BRRlpItem* mrrElements = rlpDecodeList(coder.rlp, mrrItems[mrrIdx], &mrrElementsCount);
                        mrcs[mrrIdx].msgCode  = rlpDecodeUInt64(coder.rlp, mrrElements[0], 1);
                        mrcs[mrrIdx].baseCost = rlpDecodeUInt64(coder.rlp, mrrElements[1], 1);
                        mrcs[mrrIdx].reqCost  = rlpDecodeUInt64(coder.rlp, mrrElements[2], 1);
                    }
                }
                status.flowControlMRCCount = malloc (sizeof (size_t));
                *status.flowControlMRCCount = mrrItemsCount;
                status.flowControlMRC = mrcs;
            } else if (strcmp(key, "flowControl/MRR") == 0) {
                status.flowControlMRR = malloc(sizeof(uint64_t));
                *(status.flowControlMRR) = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            }
            free (key);
        }
    }
    return status;
}

extern void
messageLESStatusShow(BREthereumLESMessageStatus *message) {
    BREthereumHashString headHashString, genesisHashString;
    hashFillString (message->headHash, headHashString);
    hashFillString (message->genesisHash, genesisHashString);

    char *headTotalDifficulty = coerceString (message->headTd, 10);

    eth_log (LES_LOG_TOPIC, "StatusMessage:%s", "");
    eth_log (LES_LOG_TOPIC, "    ProtocolVersion: %llu", message->protocolVersion);
    eth_log (LES_LOG_TOPIC, "    AnnounceType   : %llu", message->announceType);
    eth_log (LES_LOG_TOPIC, "    NetworkId      : %llu", message->chainId);
    eth_log (LES_LOG_TOPIC, "    HeadNum        : %llu", message->headNum);
    eth_log (LES_LOG_TOPIC, "    HeadHash       : %s",   headHashString);
    eth_log (LES_LOG_TOPIC, "    HeadTD         : %s",   headTotalDifficulty);
    eth_log (LES_LOG_TOPIC, "    GenesisHash    : %s",   genesisHashString);
    eth_log (LES_LOG_TOPIC, "    ServeHeaders   : %s", ETHEREUM_BOOLEAN_IS_TRUE(message->serveHeaders) ? "Yes" : "No");
    eth_log (LES_LOG_TOPIC, "    ServeChainSince: %llu", (NULL != message->serveChainSince ? *message->serveChainSince : -1)) ;
    eth_log (LES_LOG_TOPIC, "    ServeStateSince: %llu", (NULL != message->serveStateSince ? *message->serveStateSince : -1)) ;
    eth_log (LES_LOG_TOPIC, "    TxRelay        : %s", ETHEREUM_BOOLEAN_IS_TRUE(message->txRelay) ? "Yes" : "No");
    eth_log (LES_LOG_TOPIC, "    FlowControl/BL : %llu", (NULL != message->flowControlBL  ? *message->flowControlBL  : -1));
    eth_log (LES_LOG_TOPIC, "    FlowControl/MRR: %llu", (NULL != message->flowControlMRR ? *message->flowControlMRR : -1));

    size_t count = *(message->flowControlMRCCount);
    eth_log (LES_LOG_TOPIC, "    FlowControl/MRC:%s", "");
    for (size_t index = 0; index < count; index++) {
        const char *label = messageLESGetIdentifierName ((BREthereumLESMessageIdentifier) message->flowControlMRC[index].msgCode);
        if (NULL != label) {
            eth_log (LES_LOG_TOPIC, "      %2d", (BREthereumLESMessageIdentifier) message->flowControlMRC[index].msgCode);
            eth_log (LES_LOG_TOPIC, "        Request : %s", label);
            eth_log (LES_LOG_TOPIC, "        BaseCost: %llu", message->flowControlMRC[index].baseCost);
            eth_log (LES_LOG_TOPIC, "        ReqCost : %llu", message->flowControlMRC[index].reqCost);
        }
    }

    free (headTotalDifficulty);
}

/// Mark: LES Announce

//
// Announce
//
extern BREthereumLESMessageAnnounce
messageLESAnnounceDecode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    BREthereumLESMessageAnnounce message;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (5 == itemsCount);

    message.headHash = hashRlpDecode (items[0], coder.rlp);
    message.headNumber = rlpDecodeUInt64 (coder.rlp, items[1], 1);
    message.headTotalDifficulty = rlpDecodeUInt256 (coder.rlp, items[2], 1);
    message.reorgDepth = rlpDecodeUInt64 (coder.rlp, items[3], 1);

    // TODO: Decode Keys
    size_t pairCount = 0;
    const BRRlpItem *pairItems = rlpDecodeList (coder.rlp, items[4], &pairCount);
    array_new(message.pairs, pairCount);
    for (size_t index = 0; index < pairCount; index++)
        ;

    return message;
}

/// MARK: LES Get Block Headers

//
// Get Block Headers
//
extern BREthereumLESMessageGetBlockHeaders
messageLESGetBlockHeadersCreate (uint64_t reqId,
                                 uint64_t number,
                                 uint32_t maxHeaders,
                                 uint64_t skip,
                                 uint8_t  reverse) {
    return (BREthereumLESMessageGetBlockHeaders) {
        reqId,
        1,
        { .number = number },
        maxHeaders,
        skip,
        reverse};
}

extern BRRlpItem
messageLESGetBlockHeadersEncode (BREthereumLESMessageGetBlockHeaders message,
                                 BREthereumMessageCoder coder) {
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           rlpEncodeList (coder.rlp, 4,
                                          (message.useBlockNumber
                                           ? rlpEncodeUInt64 (coder.rlp, message.block.number, 1)
                                           : hashRlpEncode (message.block.hash, coder.rlp)),
                                          rlpEncodeUInt64 (coder.rlp, message.maxHeaders, 1),
                                          rlpEncodeUInt64 (coder.rlp, message.skip, 1),
                                          rlpEncodeUInt64 (coder.rlp, message.reverse, 1)));
}

extern BREthereumLESMessageGetBlockHeaders
messageLESGetBlockHeadersDecode (BRRlpItem item,
                                 BREthereumMessageCoder coder) {
    assert (0);
}

/// MARK: BlockHeaders

extern BREthereumLESMessageBlockHeaders
messageLESBlockHeadersDecode (BRRlpItem item,
                                 BREthereumMessageCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

    size_t headerItemsCount = 0;
    const BRRlpItem *headerItems = rlpDecodeList (coder.rlp, items[2], &headerItemsCount);

    BRArrayOf(BREthereumBlockHeader) headers;
    array_new (headers, headerItemsCount);
    for (size_t index = 0; index < headerItemsCount; index++)
        array_add (headers, blockHeaderRlpDecode (headerItems[index], RLP_TYPE_NETWORK, coder.rlp));

    return (BREthereumLESMessageBlockHeaders) {
        reqId,
        bv,
        headers
    };
}

/// MARK: LES GetBlockBodies

static BRRlpItem
messageLESGetBlockBodiesEncode (BREthereumLESMessageGetBlockBodies message, BREthereumMessageCoder coder) {
    return rlpEncodeList2 (coder.rlp,
                          rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                          hashEncodeList (message.hashes, coder.rlp));
}

/// MARK: LES BlockBodies

static BREthereumLESMessageBlockBodies
messageLESBlockBodiesDecode (BRRlpItem item,
                             BREthereumMessageCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

    size_t pairItemsCount = 0;
    const BRRlpItem *pairItems = rlpDecodeList (coder.rlp, items[2], &pairItemsCount);

    BRArrayOf(BREthereumBlockBodyPair) pairs;
    array_new(pairs, pairItemsCount);
    for (size_t index = 0; index < pairItemsCount; index++) {
        size_t bodyItemsCount;
        const BRRlpItem *bodyItems = rlpDecodeList (coder.rlp, pairItems[index], &bodyItemsCount);
        assert (2 == bodyItemsCount);

        BREthereumBlockBodyPair pair = {
            blockTransactionsRlpDecode (bodyItems[0], coder.network, RLP_TYPE_NETWORK, coder.rlp),
            blockOmmersRlpDecode (bodyItems[1], coder.network, RLP_TYPE_NETWORK, coder.rlp)
        };
        array_add(pairs, pair);
    }
    return (BREthereumLESMessageBlockBodies) {
        reqId,
        bv,
        pairs
    };
}

/// MARK: LES GetReceipts

static BRRlpItem
messageLESGetReceiptsEncode (BREthereumLESMessageGetReceipts message, BREthereumMessageCoder coder) {
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           hashEncodeList (message.hashes, coder.rlp));
}

/// MARK: LES Receipts

static BREthereumLESMessageReceipts
messageLESReceiptsDecode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

    size_t arrayItemsCount = 0;
    const BRRlpItem *arrayItems = rlpDecodeList (coder.rlp, items[2], &arrayItemsCount);

    BRArrayOf(BREthereumLESMessageReceiptsArray) arrays;
    array_new(arrays, arrayItemsCount);
    for (size_t index = 0; index < arrayItemsCount; index++) {
        BREthereumLESMessageReceiptsArray array = {
            transactionReceiptDecodeList (arrayItems[index], coder.rlp)
        };
        array_add (arrays, array);
    }
    return (BREthereumLESMessageReceipts) {
        reqId,
        bv,
        arrays
    };
}

/// MARK: LES GetProofs

static BRRlpItem
proofsSpecEncode (BREthereumLESMessageGetProofsSpec spec,
                  BREthereumMessageCoder coder) {
    return rlpEncodeList (coder.rlp, 4,
                          hashRlpEncode (spec.blockHash, coder.rlp),
                          rlpEncodeBytes (coder.rlp, spec.key1.bytes, spec.key1.bytesCount),
                          rlpEncodeBytes (coder.rlp, spec.key2.bytes, spec.key2.bytesCount),
                          rlpEncodeUInt64 (coder.rlp, spec.fromLevel, 1));
}

static BRRlpItem
proofsSpecEncodeList (BRArrayOf(BREthereumLESMessageGetProofsSpec) specs,
                      BREthereumMessageCoder coder) {
    size_t itemsCount = array_count(specs);
    BRRlpItem items[itemsCount];

    for (size_t index = 0; index < itemsCount; index++)
        items[index] = proofsSpecEncode (specs[index], coder);

    return rlpEncodeListItems (coder.rlp, items, itemsCount);
}

static BRRlpItem
messageLESGetProofsEncode (BREthereumLESMessageGetProofs message, BREthereumMessageCoder coder) {
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           proofsSpecEncodeList(message.specs, coder));
}

/// MARK: LES Proofs

static BREthereumLESMessageProofs
messageLESProofsDecode (BRRlpItem item,
                        BREthereumMessageCoder coder) {
    // rlpShowItem (coder.rlp, item, "LES Proofs");
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

    return (BREthereumLESMessageProofs) {
        reqId,
        bv,
        mptProofDecodeList (items[2], coder.rlp)
    };
}

/// MARK: LES GetContractCodes

/// MARK: LES ContractCodes

/// MARK: LES SendTx

/// MARK: LES GetHeaderProofs

/// MARK: LES HeaderProofs

/// MARK: LES GetProofsV2

static BRRlpItem
messageLESGetProofsV2Encode (BREthereumLESMessageGetProofsV2 message, BREthereumMessageCoder coder) {
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           proofsSpecEncodeList(message.specs, coder));
}

/// MARK: LES ProofsV2

static BREthereumLESMessageProofsV2
messageLESProofsV2Decode (BRRlpItem item,
                     BREthereumMessageCoder coder) {
    // rlpShowItem (coder.rlp, item, "LES ProofsV2");
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

    return (BREthereumLESMessageProofsV2) {
        reqId,
        bv,
        mptProofDecodeList (items[2], coder.rlp)
    };
}

/// MARK: LES GetHelperTrieProofs

/// MARK: LES HelperTrieProofs

/// MARK: LES SendTx2

static BRRlpItem
messageLESSendTx2Encode (BREthereumLESMessageSendTx2 message, BREthereumMessageCoder coder) {
    size_t itemsCount = array_count(message.transactions);
    BRRlpItem items[itemsCount];

    for (size_t index = 0; index < itemsCount; index++)
        items[index] = transactionRlpEncode(message.transactions[index],
                                            coder.network,
                                            RLP_TYPE_TRANSACTION_SIGNED,
                                            coder.rlp);

    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           rlpEncodeListItems (coder.rlp, items, itemsCount));
}

/// MARK: LES GetTxStatus

static BRRlpItem
messageLESGetTxStatusEncode (BREthereumLESMessageGetTxStatus message, BREthereumMessageCoder coder) {
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           hashEncodeList (message.hashes, coder.rlp));
}

/// MARK: LES TxStatus

static BREthereumLESMessageTxStatus
messageLESTxStatusDecode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

    return (BREthereumLESMessageTxStatus) {
        reqId,
        bv,
        transactionStatusDecodeList (items[2], coder.rlp)
    };
}

/// MARK: LES Messages

extern BREthereumLESMessage
messageLESDecode (BRRlpItem item,
                  BREthereumMessageCoder coder,
                  BREthereumLESMessageIdentifier identifier) {
    switch (identifier) {
        case LES_MESSAGE_STATUS:
            return (BREthereumLESMessage) {
                LES_MESSAGE_STATUS,
                { .status = messageLESStatusDecode (item, coder) }
            };

        case LES_MESSAGE_ANNOUNCE:
            return (BREthereumLESMessage) {
                LES_MESSAGE_ANNOUNCE,
                { .announce = messageLESAnnounceDecode (item, coder) }
            };

        case LES_MESSAGE_BLOCK_HEADERS:
            return (BREthereumLESMessage) {
                LES_MESSAGE_BLOCK_HEADERS,
                { .blockHeaders = messageLESBlockHeadersDecode (item, coder)} };

        case LES_MESSAGE_BLOCK_BODIES:
            return (BREthereumLESMessage) {
                LES_MESSAGE_BLOCK_BODIES,
                { .blockBodies = messageLESBlockBodiesDecode (item, coder)} };

        case LES_MESSAGE_RECEIPTS:
            return (BREthereumLESMessage) {
                LES_MESSAGE_RECEIPTS,
                { .receipts = messageLESReceiptsDecode (item, coder)} };

        case LES_MESSAGE_PROOFS:
            return (BREthereumLESMessage) {
                LES_MESSAGE_PROOFS,
                { .proofs = messageLESProofsDecode (item, coder)} };

        case LES_MESSAGE_PROOFS_V2:
            return (BREthereumLESMessage) {
                LES_MESSAGE_PROOFS_V2,
                { .proofsV2 = messageLESProofsV2Decode (item, coder)} };

        case LES_MESSAGE_TX_STATUS:
            return (BREthereumLESMessage) {
                LES_MESSAGE_TX_STATUS,
                { .txStatus = messageLESTxStatusDecode (item, coder)} };

        default:
            assert (0);
    }
}

extern BRRlpItem
messageLESEncode (BREthereumLESMessage message,
                  BREthereumMessageCoder coder) {
    BRRlpItem body = NULL;
    switch (message.identifier) {
        case LES_MESSAGE_STATUS:
            body = messageLESStatusEncode(&message.u.status, coder);
            break;

        case LES_MESSAGE_GET_BLOCK_HEADERS:
            body = messageLESGetBlockHeadersEncode (message.u.getBlockHeaders, coder);
            break;

        case LES_MESSAGE_GET_BLOCK_BODIES:
            body = messageLESGetBlockBodiesEncode (message.u.getBlockBodies, coder);
            break;

        case LES_MESSAGE_GET_RECEIPTS:
            body = messageLESGetReceiptsEncode (message.u.getReceipts, coder);
            break;

        case LES_MESSAGE_GET_PROOFS:
            body = messageLESGetProofsEncode (message.u.getProofs, coder);
            // rlpShowItem (coder.rlp, body, "LES GetProofs");
            break;

        case LES_MESSAGE_GET_PROOFS_V2:
            body = messageLESGetProofsV2Encode (message.u.getProofsV2, coder);
            // rlpShowItem (coder.rlp, body, "LES GetProofsV2");
            break;

        case LES_MESSAGE_GET_TX_STATUS:
            body = messageLESGetTxStatusEncode (message.u.getTxStatus, coder);
            break;

        case LES_MESSAGE_SEND_TX2:
            body = messageLESSendTx2Encode (message.u.sendTx2, coder);
            break;

        default:
            assert (0);
    }

    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.identifier + coder.lesMessageIdOffset, 1),
                           body);
}

extern int
messageLESHasUse (const BREthereumLESMessage *message,
                  BREthereumLESMessageUse use) {
    return use == messageLESSpecs[message->identifier].use;
}

// 0 if not response
extern uint64_t
messageLESGetCredits (const BREthereumLESMessage *message) {
    switch (message->identifier) {
        case LES_MESSAGE_BLOCK_HEADERS:  return message->u.blockHeaders.bv;
        case LES_MESSAGE_BLOCK_BODIES:   return message->u.blockBodies.bv;
        case LES_MESSAGE_RECEIPTS:       return message->u.receipts.bv;
        case LES_MESSAGE_PROOFS:         return message->u.proofs.bv;
        case LES_MESSAGE_CONTRACT_CODES: return 0;
        case LES_MESSAGE_HEADER_PROOFS:  return 0;
        case LES_MESSAGE_PROOFS_V2:      return message->u.proofsV2.bv;
        case LES_MESSAGE_HELPER_TRIE_PROOFS: return 0;
        case LES_MESSAGE_TX_STATUS:      return message->u.txStatus.bv;
        default: return 0;
    }
}

/// MARK: - Wire Protocol Messagees

extern BRRlpItem
messageEncode (BREthereumMessage message,
               BREthereumMessageCoder coder) {
    switch (message.identifier) {
        case MESSAGE_P2P:
            return messageP2PEncode (message.u.p2p, coder);
        case MESSAGE_LES:
            return messageLESEncode (message.u.les, coder);
        case MESSAGE_DIS:
            return messageDISEncode (message.u.dis, coder);
        case MESSAGE_ETH:
            assert (0);
    }
}

extern BREthereumMessage
messageDecode (BRRlpItem item,
               BREthereumMessageCoder coder,
               BREthereumMessageIdentifier type,
               BREthereumANYMessageIdentifier subtype) {
    switch (type) {
        case MESSAGE_P2P:
            return (BREthereumMessage) {
                MESSAGE_P2P,
                { .p2p = messageP2PDecode (item, coder, (BREthereumP2PMessageIdentifier) subtype) }
            };

        case MESSAGE_LES:
            return (BREthereumMessage) {
                MESSAGE_LES,
                { .les = messageLESDecode (item, coder, (BREthereumLESMessageIdentifier) subtype) }
            };

        case MESSAGE_DIS:
            return (BREthereumMessage) {
                MESSAGE_DIS,
                { .dis = messageDISDecode (item, coder) }
            };

        case MESSAGE_ETH:
            assert (0);
    }
}

extern int
messageHasIdentifier (BREthereumMessage *message,
                      BREthereumMessageIdentifier identifer) {
    return identifer == message->identifier;
}

extern int
messageHasIdentifiers (BREthereumMessage *message,
                       BREthereumMessageIdentifier identifer,
                       BREthereumANYMessageIdentifier anyIdentifier) {
    if (identifer != message->identifier) return 0;

    switch (message->identifier) {
        case MESSAGE_P2P: return anyIdentifier == message->u.p2p.identifier;
        case MESSAGE_DIS: return anyIdentifier == message->u.dis.identifier;
        case MESSAGE_ETH: return anyIdentifier == message->u.eth.identifier;
        case MESSAGE_LES: return anyIdentifier == message->u.les.identifier;
    }
}

static const char *messageNames[] = { "P2P", "ETH", "LES", "DIS" };

const char *
messageGetIdentifierName (BREthereumMessage *message) {
    return messageNames [message->identifier];
}

const char *
messageGetAnyIdentifierName (BREthereumMessage *message) {
    switch (message->identifier) {
        case MESSAGE_P2P: return messageP2PGetIdentifierName (message->u.p2p.identifier);
        case MESSAGE_ETH: return "";
        case MESSAGE_LES: return messageLESGetIdentifierName (message->u.les.identifier);
        case MESSAGE_DIS: return messageDISGetIdentifierName (message->u.dis.identifier);
    }
}

