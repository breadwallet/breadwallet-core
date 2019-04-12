//
//  BREthereumMessage.h
//  Core
//
//  Created by Ed Gamble on 8/13/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Message_H
#define BR_Ethereum_Message_H

#include "BREthereumLESBase.h"
#include "msg/BREthereumMessageP2P.h"
#include "msg/BREthereumMessageDIS.h"
#include "msg/BREthereumMessageETH.h"
#include "msg/BREthereumMessageLES.h"
#include "msg/BREthereumMessagePIP.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// BREthereumMessageIdentifier - The Ethereum Wire Protocol (WIP) defines four fundamental
// message types.  We'll explicitly handle each one.
//
typedef enum {
    MESSAGE_P2P   = 0x00,
    MESSAGE_DIS   = 0x01,
    MESSAGE_ETH   = 0x02,
    MESSAGE_LES   = 0x03,
    MESSAGE_PIP   = 0x04,
} BREthereumMessageIdentifier;


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
        BREthereumDISMessage dis;
        BREthereumETHMessage eth;
        BREthereumLESMessage les;
        BREthereumPIPMessage pip;
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

extern void
messageRelease (BREthereumMessage *message);

extern void
messagesRelease (BRArrayOf(BREthereumMessage) messages);

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

#endif /* BR_Ethereum_Message_H */
