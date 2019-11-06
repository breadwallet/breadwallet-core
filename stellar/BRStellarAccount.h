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

#ifdef __cplusplus
extern "C" {
#endif

#include "support/BRKey.h"
#include "BRStellarBase.h"
#include "BRStellarTransaction.h"

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

/**
 * Get the stellar address for this account
 *
 * @param account   BRStellarAccount
 * @return address  the stellar address for this account
 */
extern BRStellarAddress
stellarAccountGetAddress(BRStellarAccount account);

/**
 * Get the stellar address for this account
 *
 * @param account     BRStellarAccount
 * @return accountID  raw account ID bytes
 */
extern BRStellarAccountID
stellarAccountGetAccountID(BRStellarAccount account);
    
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
 * @param  account  the account
 * @return address  the primary stellar address for this account
 */
extern BRStellarAddress stellarAccountGetPrimaryAddress (BRStellarAccount account);

/**
 * Get the account's public key (for the primary address)
 *
 * @param  account     the account
 * @return publicKey   A BRKey object holding the public key
 */
extern BRKey stellarAccountGetPublicKey(BRStellarAccount account);

/**
 * Set the sequence number for this account
 *
 * The sequence number is used when sending transcations
 *
 * NOTE: The current transaction sequence number of the account. This number starts
 * equal to the ledger number at which the account was created, not at 0.
 *
 * @param account the account
 * @param sequence  a 64-bit unsigned number. It should be retrieved from network
 *                  or should be set to 1 more that the value in the lastest transaction
 */
extern void stellarAccountSetSequence(BRStellarAccount account, uint64_t sequence);

/**
 * Set the network type being used for this account
 *
 * The account will default to the production network if this fuction is not called
 *
 * @param account the account
 * @param networkType      STELLAR_NETWORK_PUBLIC or STELLAR_NETWORK_TESTNET
 */
extern void stellarAccountSetNetworkType(BRStellarAccount account, BRStellarNetworkType networkType);

/*
 * Get stellar account ID from string
 *
 * @param address     a string containing a valid Stellar account in base64 form. e.g.
 *                    GBKWF42EWZDRISFXW3V6WW5OTQOOZSJQ54UINC7CXN4.......
 * @return accountID  a BRStellarAccountID object
 */
extern BRStellarAccountID stellerAccountCreateStellarAccountID(const char * stellarAddress);

#ifdef __cplusplus
}
#endif

#endif // BRStellar_account_h
