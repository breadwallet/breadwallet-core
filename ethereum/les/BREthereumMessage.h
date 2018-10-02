//
//  BREthereumMessage.h
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
