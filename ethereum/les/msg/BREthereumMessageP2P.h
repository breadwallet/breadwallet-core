//
//  BREthereumMessageP2P.h
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

#ifndef BR_Ethereum_Message_P2P_H
#define BR_Ethereum_Message_P2P_H

#include "BREthereumLESBase.h"

#ifdef __cplusplus
extern "C" {
#endif

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

extern BREthereumP2PMessage
messageP2PDecode (BRRlpItem item,
                  BREthereumMessageCoder coder,
                  BREthereumP2PMessageIdentifier identifer);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Message_P2P_H */
