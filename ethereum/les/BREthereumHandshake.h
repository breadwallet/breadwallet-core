//
//  BREthereumNode.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 4/18/18.
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


#ifndef BR_Ethereum_Handshake_h
#define BR_Ethereum_Handshake_h

#include <inttypes.h>
#include "BRInt.h"
#include "BREthereumNode.h"
#include "BRKey.h"
#include "BREthereumBase.h"
#include "BREthereumLES.h"
#include "BREthereumFrameCoder.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The current state of the handshake
 */
typedef enum {
    BRE_HANDSHAKE_ERROR = -1,
    BRE_HANDSHAKE_NEW,
    BRE_HANDSHAKE_ACKAUTH,
    BRE_HANDSHAKE_WRITESTATUS,
    BRE_HANDSHAKE_READSTATUS,
    BRE_HANDSHAKE_FINISHED
}BREthereumHandshakeStatus;

/**
 * The context for the ethereum handhsake
 *
 */
typedef struct BREthereumHandshakeContext* BREthereumHandshake;

/**
 * Creates an etheruem handshake context
 *
 * @param node - weak reference to the node performing the handshake
 */
extern BREthereumHandshake ethereumHandshakeCreate(BREthereumNode node) ;

/**
 * Checks whether the state of the handhsake needs to be updated based on recieving/sending messages
 * to/from the remote peer
 *
 * @returns - the current state of the handshake after performing an update
 */
extern BREthereumHandshakeStatus ethereumHandshakeTransition(BREthereumHandshake handshake);

/**
 * Deletes the memory of a handshake context
 *
 * @param handshakeCxt - the hande shake context information to delete
 */
extern void ethereumHandshakeRelease(BREthereumHandshake handshake);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Handshake_h */

