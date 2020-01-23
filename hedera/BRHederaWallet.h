//
//  BRHederaWallet.h
//  Core
//
//  Created by Carl Cherry on Oct. 16, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRHederaWallet_h
#define BRHederaWallet_h


#include "BRHederaAccount.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct BRHederaWalletRecord *BRHederaWallet;

/**
 * Create a Hedera wallet
 *
 * @param account   - a BRHederaAccount
 *
 * @return wallet   - a valid BRHederaWallet object
 */
extern BRHederaWallet  /* caller must free - using hederaWalletFree */
hederaWalletCreate (BRHederaAccount account);

/**
 * Free all memory associated with this wallet
 *
 * @param wallet    - a valid BRHederaWallet
 *
 * @return void
 */
extern void
hederaWalletFree (BRHederaWallet wallet);

/**
 * Return an address suitable for sending HBAR. Depending on the nature of Hedera this
 * might just be the accounts' primary address or this might be different every time a
 * source address is requested.
 *
 * @param wallet the wallet for target address
 *
 * @return address  hedera sending address associated with this account
 */
extern BRHederaAddress
hederaWalletGetSourceAddress (BRHederaWallet wallet);

/**
 * Return an address suitable for receiving HBAR. Depending on the nature of Hedera this
 * might just be the accounts' primary address or this might be different every time a
 * target address is requested.
 *
 * @param wallet the wallet for target address
 *
 * @return address  hedera receiving address associated with this account
 */
extern BRHederaAddress
hederaWalletGetTargetAddress (BRHederaWallet wallet);

/**
 * Set the balance for this wallet - in HBAR TinyBar units
 *
 * @param wallet    - the wallet for target address
 * @param balance   - tiny bar amount for this account
 *
 * @return address  hedera receiving address associated with this account
 */
extern void
hederaWalletSetBalance (BRHederaWallet wallet, BRHederaUnitTinyBar balance);

/**
 * Return the balance for this wallet - in HBAR TinyBar units
 *
 * @param wallet the wallet for target address
 *
 * @return balance  - balance of the wallet's account in tiny bar units
 */
extern BRHederaUnitTinyBar
hederaWalletGetBalance (BRHederaWallet wallet);

#ifdef __cplusplus
}
#endif

#endif // BRHederaWallet_h
