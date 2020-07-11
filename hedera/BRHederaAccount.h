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
#include "BRHederaAddress.h"

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
 * Create a Hedera account from a byte array (previously serialized
 *
 * @param bytes              array of bytes
 * @param bytesCount     size of byte array
 *
 * @return account
*/
extern BRHederaAccount  /* caller must free - using "free" function */
hederaAccountCreateWithSerialization (uint8_t *bytes, size_t bytesCount);

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
 * Set the hedera address for an account
 *
 * The Hedera address cannot be created offline - the process is for us to create
 * a public key, then some service with currency has to create an account by sending
 * it some HBAR.  That service will then return the account address.
 *
 * @param account
 * @param accountID - account id created from the public key
 *
 * @return account
 */
extern void hederaAccountSetAddress (BRHederaAccount account, BRHederaAddress accountID);

/**
 * Get the public key for this Hedera account
 *
 * @param account
 *
 * @return public key
 */
extern BRKey hederaAccountGetPublicKey (BRHederaAccount account);

/**
 * Get the Hedera Address from the specified account.
 *
 * @param account
 *
 * @return address
 */
extern BRHederaAddress hederaAccountGetAddress (BRHederaAccount account);

/**
 * Get the primary Hedera Address from the specified account.
 *
 * @param account
 *
 * @return address
 */
extern BRHederaAddress hederaAccountGetPrimaryAddress (BRHederaAccount account);

extern uint8_t * // Caller owns memory and must delete calling "free"
hederaAccountGetSerialization (BRHederaAccount account, size_t *bytesCount);

/**
 * Check if this account has the specified address
 *
 * @param account   hedera account
 * @param address   hedera address to check
 *
 * @return 1 if true, 0 if false
*/
extern int hederaAccountHasAddress (BRHederaAccount account, BRHederaAddress address);

#ifdef __cplusplus
}
#endif

#endif // BRHederaAccount_h
