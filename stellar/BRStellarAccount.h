//
//  BRStellarAccount.h
//  Core
//
//  Created by Carl Cherry on 5/21/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRStellar_account_h
#define BRStellar_account_h

#include "BRStellarTransaction.h"
#include "BRKey.h"

typedef struct BRStellarAccountRecord *BRStellarAccount;

/**
 * Create a Stellar account object
 *
 * @param  paperKey  12 word mnemonic string
 * @return account
 */
extern BRStellarAccount /* caller must free - stellarAccountFree */
stellarAccountCreate (const char *paperKey);

/**
 * Create a Stellar account object
 *
 * @param  seed     512-bit secret
 * @return account
 */
extern BRStellarAccount /* caller must free - stellarAccountFree */
stellarAccountCreateWithSeed(UInt512 seed);

/**
 * Create a Stellar account object
 *
 * @param  key      BRKey with valid private key
 * @return account
 */
extern BRStellarAccount /* caller must free - stellarAccountFree */
stellarAccountCreateWithKey(BRKey key);

/**
 * Free the memory for a stellar account object
 *
 * @param account BRStellarAccount to free
 *
 * @return void
 */
extern void stellarAccountFree(BRStellarAccount account);

/**
 * Serialize (and sign) a Stellar Transaction.  Ready for submission to the ledger.
 *
 * @param account         the account sending the payment
 * @param transaction     the transaction to serialize
 * @param paperKey        paper key of the sending account
 *
 * @return handle         BRStellarSerializedTransaction handle, use this to get the size and bytes
 *                        NOTE: If successful then the sequence number in the account is incremented
 *                        NOTE: is the handle is NULL then the serialization failed AND the sequence
 *                              was not incremented
 */
extern
const BRStellarSerializedTransaction /* do NOT free, owned by transaction */
stellarAccountSignTransaction(BRStellarAccount account, BRStellarTransaction transaction, const char *paperKey);

// Accessor function for the account address (Stellar ID)
extern BRStellarAddress
stellarAccountGetAddress(BRStellarAccount account);

/*
 * Get the string version of the stellar address
 *
 * @param account        handle of a valid account object
 * @param stellarAddress  pointer to char buffer to hold the address
 * @param length         length of the stellarAddress buffer
 *
 * @return               number of bytes copied to stellarAddress + 1 (for terminating byte)
 *                       otherwise return the number of bytes needed to store the address
 *                       including the 0 termination byte
 */
extern int stellarAccountGetAddressString(BRStellarAccount account,
                                         char * stellarAddress, /* memory owned by caller */
                                         int length);

/**
 * Create a BRStellarAddress from the stellar string
 *
 * @param stellarAddres  address in the form r41...
 *
 * @return address      BRStellarAddres
 */
extern BRStellarAddress
stellarAddressCreate(const char * stellarAddressString);

/**
 * Compare 2 stellar addresses
 *
 * @param a1  first address
 * @param a2  second address
 *
 * @return 1 - if addresses are equal
 *         0 - if not equal
 */
extern int
stellarAddressEqual (BRStellarAddress a1, BRStellarAddress a2);

/**
 * Get the account's primary address
 *
 * @param account the account
 */
extern BRStellarAddress stellarAccountGetPrimaryAddress (BRStellarAccount account);

extern BRKey stellarAccountGetPublicKey(BRStellarAccount account);

extern void stellarAccountSetSequence(BRStellarAccount account, uint64_t sequence);

#endif
