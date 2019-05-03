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
#include "BRRippleAccount.h"

//
// Wallet
//
typedef struct BRRippleWalletRecord *BRRippleWallet;

/**
 * Create a ripple wallet object
 *
 * @param account   the ripple account that is associated with the wallet
 *
 * @return wallet   pointer to the wallet
 */
extern BRRippleWallet
rippleWalletCreate (BRRippleAccount account);

/**
 * Release memory for the wallet
 *
 * @param wallet   the ripple wallet to release
 */
extern void
rippleWalletRelease (BRRippleWallet wallet);

/**
 * Return an address suitable for sending Ripple.  Depending on the nature of Ripple this
 * might just be the account's primary address or this might be different every time a
 * source address is requested.
 *
 * @param wallet the walelt for source address
 *
 * @return address  ripple address associated with this account
 */
extern BRRippleAddress
rippleWalletGetSourceAddress (BRRippleWallet wallet);


/**
 * Return an address suitable for receiving Ripple  Depending on the nature of Ripple this
 * might just be the accounts' primary address or this might be different every time a
 * target address is requested.
 *
 * @param wallet the wallet for target address
 *
 * @return address  ripple address associated with this account
 */
extern BRRippleAddress
rippleWalletGetTargetAddress (BRRippleWallet wallet);

/**
 * Return the ripple balance for this wallet
 *
 * @param wallet the specified ripple wallet
 *
 * @return balance  ripple balance in drops
 */
extern uint64_t
rippleWalletGetBalance (BRRippleWallet wallet);

/**
 * Set the ripple balance in this wallet
 *
 * @param balance   the ripple balance in drops
 *
 * @return void
 */
extern void
rippleWalletSetBalance (BRRippleWallet wallet, uint64_t balance);

#endif
