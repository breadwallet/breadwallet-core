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


/// MARK: - Wire Protocol Messagees

extern BRRlpItem
messageEncode (BREthereumMessage message,
               BREthereumMessageCoder coder) {
    switch (message.identifier) {
        case MESSAGE_P2P: return messageP2PEncode (message.u.p2p, coder);
        case MESSAGE_DIS: return messageDISEncode (message.u.dis, coder);
        case MESSAGE_ETH: assert (0);
        case MESSAGE_LES: return messageLESEncode (message.u.les, coder);
        case MESSAGE_PIP: return messagePIPEncode(message.u.pip, coder);
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

        case MESSAGE_DIS:
            return (BREthereumMessage) {
                MESSAGE_DIS,
                { .dis = messageDISDecode (item, coder) }
            };

        case MESSAGE_ETH:
            assert (0);

        case MESSAGE_LES:
            return (BREthereumMessage) {
                MESSAGE_LES,
                { .les = messageLESDecode (item, coder, (BREthereumLESMessageIdentifier) subtype) }
            };

        case MESSAGE_PIP:
            return (BREthereumMessage) {
                MESSAGE_PIP,
                { .pip = messagePIPDecode (item, coder, (BREthereumPIPMessageType) subtype) }
            };
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
        case MESSAGE_PIP: return anyIdentifier == message->u.pip.type;
    }
}

static const char *messageNames[] = { "P2P", "DIS", "ETH", "LES", "PIP" };

const char *
messageGetIdentifierName (BREthereumMessage *message) {
    return messageNames [message->identifier];
}

const char *
messageGetAnyIdentifierName (BREthereumMessage *message) {
    switch (message->identifier) {
        case MESSAGE_P2P: return messageP2PGetIdentifierName (message->u.p2p.identifier);
        case MESSAGE_DIS: return messageDISGetIdentifierName (message->u.dis.identifier);
        case MESSAGE_ETH: return "";
        case MESSAGE_LES: return messageLESGetIdentifierName (message->u.les.identifier);
        case MESSAGE_PIP: return messagePIPGetIdentifierName (message->u.pip.type);
    }
}

