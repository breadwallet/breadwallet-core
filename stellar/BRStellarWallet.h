//
//  BRStellarWallet.h
//  Core
//
//  Created by Carl Cherry on 6/12/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRStellar_wallet_h
#define BRStellar_wallet_h

#include "BRStellarBase.h"
#include "BRStellarAccount.h"

#ifdef __cplusplus
extern "C" {
#endif


//
// Wallet
//
typedef struct BRStellarWalletRecord *BRStellarWallet;

/**
 * Create a stellar wallet object
 *
 * @param account   the stellar account that is associated with the wallet
 *
 * @return wallet   pointer to the wallet
 */
extern BRStellarWallet /* caller must free - stellarWalletFree */
stellarWalletCreate (BRStellarAccount account);

/**
 * Free memory for the specified wallet object
 *
 * @param wallet   the stellar wallet to release
 */
extern void
stellarWalletFree (BRStellarWallet wallet);

/**
 * Return an address suitable for sending Stellar.  Depending on the nature of Stellar this
 * might just be the account's primary address or this might be different every time a
 * source address is requested.
 *
 * @param wallet    the wallet for source address
 *
 * @return address  stellar address associated with this account
 */
extern BRStellarAddress
stellarWalletGetSourceAddress (BRStellarWallet wallet);

/**
 * Return an address suitable for receiving Stellar  Depending on the nature of Stellar this
 * might just be the accounts' primary address or this might be different every time a
 * target address is requested.
 *
 * @param wallet the wallet for target address
 *
 * @return address  stellar address associated with this account
 */
extern BRStellarAddress
stellarWalletGetTargetAddress (BRStellarWallet wallet);

/**
 * Return the XML balance for this wallet
 *
 * @param wallet the specified stellar wallet
 *
 * @return balance  XML balance
 */
extern BRStellarAmount
stellarWalletGetBalance (BRStellarWallet wallet);

/**
 * Set the stellar balance in this wallet
 *
 * @param balance   the stellar XML balance
 *
 * @return void
 */
extern void
stellarWalletSetBalance (BRStellarWallet wallet, BRStellarAmount balance);

#ifdef __cplusplus
}
#endif

#endif // BRStellar_wallet_h
