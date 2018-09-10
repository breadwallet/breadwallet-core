//
//  BREthereumMessageP2P.c
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

#include "BREthereumMessageP2P.h"

/// MARK: - P2P (Peer-to-Peer) Messages

static const char *messageP2PNames[] = {
    "Hello",
    "Disconnect",
    "Ping",
    "Pong" };

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
        // We've seen 'hive' here - restrict to 3
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

static const char *disconnectReasonNames[] = {
    "Requested",
    "TCP Error",
    "Breach Proto",
    "Useless Peer",
    "Too Many Peers",
    "Already Connected",
    "Incompatible P2P",
    "Null Node",
    "Client Quit",
    "Unexpected ID",
    "ID Same",
    "Timeout",
    "", // 0x0c
    "", // 0x0d
    "", // 0x0e
    "", // 0x0f
    "Unknown"
};
extern const char *
messageP2PDisconnectDescription (BREthereumP2PDisconnectReason identifier) {
    return disconnectReasonNames [identifier];
}

//
static BREthereumP2PMessageDisconnect
messageP2PDisconnectDecode (BRRlpItem item, BREthereumMessageCoder coder) {
    BREthereumP2PMessageDisconnect disconnect;
//#if P2P_MESSAGE_VERSION == 0x04
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);
    assert (1 == itemsCount);

    disconnect.reason = (BREthereumP2PDisconnectReason) rlpDecodeUInt64 (coder.rlp, items[0], 1);
//#elif P2P_MESSAGE_VERSION == 0x05
//    disconnect.reason = (BREthereumP2PDisconnectReason) rlpDecodeUInt64 (coder.rlp, item, 1);
//#endif
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

extern BREthereumP2PMessage
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


