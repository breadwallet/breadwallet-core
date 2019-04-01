//
//  BREthereumMessageETH.h
//  BRCore
//
//  Created by Ed Gamble on 9/1/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Message_ETH_H
#define BR_Ethereum_Message_ETH_H

#include "../BREthereumLESBase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The 'Ethereum Subprotocol' define N messages - we are 'Light Client' ...
 * ... and thus we don't need these.  Included for completeness and wholely undefined.
 */
typedef enum {
    ETH_MESSAGE_FOO = 0x01,
    ETH_MESSAGE_BAR = 0x02
} BREthereumETHMessageIdentifier;

typedef struct {} BREthereumETHMessageFoo;
typedef struct {} BREthereumETHMessageBar;

/**
 * An ETH Message is one of the above ETH message types.
 */
typedef struct {
    BREthereumETHMessageIdentifier identifier;
    union {
        BREthereumETHMessageFoo foo;
        BREthereumETHMessageBar bar;
    } u;
} BREthereumETHMessage;

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Message_ETH_H */
