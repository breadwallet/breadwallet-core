//
//  BRHederaAccount.h
//  Core
//
//  Created by Carl Cherry on Oct. 16, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRHederaAccount_h
#define BRHederaAccount_h


#include "support/BRKey.h"
#include "support/BRInt.h"
#include "BRHederaBase.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct BRHederaAccountRecord *BRHederaAccount;

/**
 * Create a Hedera account from a seed
 *
 * @param seed - UInt512 seed value
 *
 * @return account
 */
extern BRHederaAccount  /* caller must free - using "free" function */
hederaAccountCreateWithSeed (UInt512 seed);

/**
 * Free all memory associated with this account
 *
 * @param account
 *
 * @return void
 */
extern void
hederaAccountFree (BRHederaAccount account);

/**
 * Set the account ID for an account
 *
 * The Hedera account cannot be created offline - the process is for us to create
 * a public key, then some service with currency has to create an account by sending
 * it some HBAR.  That service will then return the accountID.
 *
 * @param account
 * @param accountID - account id created from the public key
 *
 * @return account
 */
extern void hederaAccountSetAccountID (BRHederaAccount account, BRHederaAccountID accountID);

/**
 * Get the public key for this Hedera account
 *
 * @param account
 *
 * @return public key
 */
extern BRKey hederaAccountGetPublicKey (BRHederaAccount account);

/**
 * Get the Hedera AccountID from the specified account.
 *
 * @param account
 *
 * @return accountID
 */
extern BRHederaAccountID hederaAccountGetAccountID (BRHederaAccount account);

#ifdef __cplusplus
}
#endif

#endif // BRHederaAccount_h
