//
//  BREthereumLESMessage.c
//  Core
//
//  Created by Ed Gamble on 8/13/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <assert.h>
#include <sys/socket.h>
#include "BREthereumMessage.h"

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
        case MESSAGE_PIP: return messagePIPGetIdentifierName (message->u.pip);
    }
}

extern void
messageRelease (BREthereumMessage *message) {
    switch (message->identifier) {
        case MESSAGE_P2P:
            messageP2PRelease (&message->u.p2p);
            break;

        case MESSAGE_DIS:
            messageDISRelease (&message->u.dis);
            break;

        case MESSAGE_ETH:
            break;

        case MESSAGE_LES:
            messageLESRelease (&message->u.les);
            break;

        case MESSAGE_PIP:
            messagePIPRelease (&message->u.pip);
            break;
    }
}

extern void
messagesRelease (BRArrayOf(BREthereumMessage) messages) {
    if (NULL != messages) {
        size_t count = array_count(messages);
        for (size_t index = 0; index < count; index++)
            messageRelease (&messages[index]);
        array_free (messages);
    }
}

