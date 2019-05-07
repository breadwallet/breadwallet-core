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

#include "BRRippleBase.h"
#include "BRRippleTransaction.h"
#include "BRKey.h"

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

/**
 * Serialize a Ripple transaction (in a form suitable signing)
 *
 * @param transaction         the transaction to serialize
 * @param paperKey            paper key of the sending account
 * @param sequence            the next valid sequence number for the account
 * @param lastLedgerSequence  0 - don't send this value
 *                            > 0 - last ledger sequence from the server
 */
extern BRRippleSerializedTransaction
rippleAccountSignTransaction(BRRippleTransaction transaction, const char *paperKey,
                             uint32_t sequence, uint32_t lastLedgerSequence);

// Accessor function for the account object
extern uint8_t * getRippleAccountBytes(BRRippleAccount account);
extern char * getRippleAddress(BRRippleAccount account);

/**
 * Create a BRRippleAddress from the ripple string
 *
 * @param rippleAddres  address in the form r41...
 *
 * @return address      BRRippleAddres
 */
extern BRRippleAddress
rippleAddressCreate(const char * rippleAddressString);

/**
 * Compare 2 ripple addresses
 *
 * @param a1  first address
 * @param a2  second address
 *
 * @return 1 - if addresses are equal
 *         0 - if not equal
 */
extern int
rippleAddressEqual (BRRippleAddress a1, BRRippleAddress a2);

/**
 * Get the account's primary address
 *
 * @param account the account
 */
extern BRRippleAddress rippleAccountGetPrimaryAddress (BRRippleAccount account);

extern BRKey rippleAccountGetPublicKey(BRRippleAccount account);

extern BRRippleAccount rippleAccountCreateWithSeed(UInt512 seed);
extern BRRippleAccount rippleAccountCreateWithKey(BRKey key);

#endif