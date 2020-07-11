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
#include "BRRippleTransfer.h"

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
extern BRRippleWallet /* caller must free - rippleWalletFree */
rippleWalletCreate (BRRippleAccount account);

/**
 * Free memory for the specified wallet object
 *
 * @param wallet   the ripple wallet to release
 */
extern void
rippleWalletFree (BRRippleWallet wallet);

/**
 * Return an address suitable for sending Ripple.  Depending on the nature of Ripple this
 * might just be the account's primary address or this might be different every time a
 * source address is requested.
 *
 * @param wallet the walelt for source address
 *
 * @return address  ripple address associated with this account
 */
extern BRRippleAddress // caller owns object, must free with rippleAddressFree
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
extern BRRippleAddress // caller owns object, must free with rippleAddressFree
rippleWalletGetTargetAddress (BRRippleWallet wallet);

extern int
rippleWalletHasAddress (BRRippleWallet wallet,
                        BRRippleAddress address);

/**
 * Return the ripple balance for this wallet
 *
 * @param wallet the specified ripple wallet
 *
 * @return balance  ripple balance in drops
 */
extern BRRippleUnitDrops
rippleWalletGetBalance (BRRippleWallet wallet);

/**
 * Set the ripple balance in this wallet
 *
 * @param wallet     the specified wallet
 * @param balance   the ripple balance in drops
 *
 * @return void
 */
extern void
rippleWalletSetBalance (BRRippleWallet wallet, BRRippleUnitDrops balance);

/**
 * Return the ripple balance limit for this wallet
 *
 * @param wallet the specified ripple wallet
 * @param asMaximum if true, return the maximum limit; otherwise the minimum limit
 * @param hasLimit set to true (1) if the wallet has a limit
 *
 * @return the limit
 */
extern BRRippleUnitDrops
rippleWalletGetBalanceLimit (BRRippleWallet wallet,
                             int asMaximum,
                             int *hasLimit);

/**
 * Set the ripple fee basis default amount
 *
 * @param wallet     the specified wallet
 * @param feeBasis   the default fee basis to be used for all transactions
 *
 * @return void
 */
extern void
rippleWalletSetDefaultFeeBasis (BRRippleWallet wallet, BRRippleFeeBasis feeBasis);

/**
 * Get the ripple default fee basis that is stored with this wallet
 *
 * @param wallet the specified ripple wallet
 *
 * @return feeBasis  the default base fee that has been set for this wallet
 */
extern BRRippleFeeBasis
rippleWalletGetDefaultFeeBasis (BRRippleWallet wallet);

extern int rippleWalletHasTransfer (BRRippleWallet wallet, BRRippleTransfer transfer);

extern void rippleWalletAddTransfer (BRRippleWallet wallet,
                                     OwnershipKept BRRippleTransfer transfer);

extern void rippleWalletRemTransfer (BRRippleWallet wallet,
                                     OwnershipKept BRRippleTransfer transfer);

#endif
