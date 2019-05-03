//
//  BRRippleWallet.h
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRipple_wallet_h
#define BRRipple_wallet_h

#include "BRRippleBase.h"

//
// Wallet
//
typedef struct BRRippleWalletRecord *BRRippleWallet;

extern BRRippleWallet
rippleWalletCreate (BRRippleAccount account);


/**
 * Return an address suitable for sending Ripple.  Depending on the nature of Ripple this
 * might just be the account's primary address or this might be different every time a
 * source address is requested.
 *
 * @param wallet the walelt for source address
 */
extern void /* ?? const char * ?? */
rippleWalletGetSourceAddress (BRRippleWallet wallet);


/**
 * Return an address suitable for receiving Ripple  Depending on the nature of Ripple this
 * might just be the accounts' primary address or this might be different every time a
 * target address is requested.
 *
 * @param wallet the wallet for target address
 */
extern void /* ?? const char * ?? */
rippleWalletGetTargetAddress (BRRippleWallet wallet);

#endif
