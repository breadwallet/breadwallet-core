//
//  BRRippleAccount.h
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRipple_account_h
#define BRRipple_account_h

#include "BRKey.h"
#include "BRRippleBase.h"

typedef struct BRRippleAccountRecord *BRRippleAccount;

/**
 * Create a Ripple account for the `paperKey`.  The account *must never* hold the privateKey
 * derived from the paperKey; but likely must hold some publicKey.
 *
 * @param paperKey
 *
 * @return An account for `paperKey`
 */
extern BRRippleAccount
rippleAccountCreate (const char *paperKey);

/**
 * Delete a ripple account (clean up memory)
 *
 * @param account BRRippleAccount to delete
 *
 * @return void
 */
extern void rippleAccountDelete(BRRippleAccount account);

// Accessor function for the account object
extern uint8_t * getRippleAccountBytes(BRRippleAccount account);
extern char * getRippleAddress(BRRippleAccount account);

/**
 * Get the account's primary address
 *
 * @param account the account
 */
extern BRRippleAddress rippleAccountGetPrimaryAddress (BRRippleAccount account);

extern BRKey rippleAccountGetPublicKey(BRRippleAccount account);

extern BRRippleAccount rippleAccountCreateWithSeed(UInt256 seed);
extern BRRippleAccount rippleAccountCreateWithKey(BRKey key);

#endif
