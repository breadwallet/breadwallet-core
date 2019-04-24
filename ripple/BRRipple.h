//
//  BRRipple.h
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRipple_h
#define BRRipple_h

#include <stdint.h>
#include "BRRippleBase.h"
#include "BRKey.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct BRRippleTransactionRecord *BRRippleTransaction;
typedef struct BRRippleSerializedTransactionRecord *BRRippleSerializedTransaction;
typedef struct BRRippleAccountRecord *BRRippleAccount;

/**
 * Create a Ripple transaction
 *
 * NOTE: for this first iteration the only transaction type supported
 * is a Payment, in XRP only. The payment only supports the required fields.
 *
 * @param  sourceAddress  Ripple address of owner account
 * @param  targetAddress  Ripple address of recieving account
 * @param  txType         Ripple transaction type (only supporing PAYMENT for now)
 * @param  amount         XRP drop amount to be sent
 * @param  sequence       Next valid sequence number for the owner account
 * @param  fee            XRP fee in drops
 *
 * @return transaction    a ripple transaction
 */
extern BRRippleTransaction
rippleTransactionCreate(BRRippleAddress sourceAddress,
                        BRRippleAddress targetAddress,
                        BRRippleTransactionType txType,
                        uint64_t amount, // For now assume XRP drops.
                        uint32_t sequence,
                        uint64_t fee,
                        BRKey publicKey);

/**
 * Delete a Ripple transaction
 *
 * @param transaction  BRRippleTransaction
 */
extern void deleteRippleTransaction(BRRippleTransaction transaction);

/**
 * Serialize a Ripple transaction (in a form suitable signing)
 *
 * @param transaction the transaction to serialize
 */
extern BRRippleSerializedTransaction
rippleTransactionSerialize /* ForSigning */ (BRRippleTransaction transaction);

/**
 * Get the size of a serialized transaction
 *
 * @param  s     serialized transaction
 * @return size
 */
extern uint32_t getSerializedSize(BRRippleSerializedTransaction s);
/**
 * Get the raw bytes of a serialized transaction
 *
 * @param  s     serialized transaction
 * @return bytes uint8_t
 */
extern uint8_t* getSerializedBytes(BRRippleSerializedTransaction s);

// Some other function to 'attach a signature' and then serialize the transaction +
// signature for submission?
//
// Account
//

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
extern void /* ?? const char * ?? */
rippleAccountGetPrimaryAddress (BRRippleAccount account);

extern BRKey rippleAccountGetPublicKey(BRRippleAccount account);

/**
 * Sign `bytes` for `account` with `paperKey`
 *
 * @param account the account to sign with
 * @param bytes bytes to sign
 * @param bytesCount the count of bytes to sign
 * @param paperKey the paperKey for signing
 */
extern void /* ?? signature ?? */
rippleAccountSignBytes (BRRippleAccount account,
                        /* address */
                        /* signature type */
                        uint8_t *bytes,
                        size_t bytesCount,
                        const char *paperKey);

//
// Wallet
//
typedef struct BRRippleWalletRecord *BRRippleWallet;

extern BRRippleWallet
rippleWalletCreate (BRRippleAccount account);


/**
 * Return an address suitable for sending Ripple.  Depending on the nature of Ripple this
 * might just be the account's primary address or this might be different every time a
 * source address is requested.
 *
 * @param wallet the walelt for source address
 */
extern void /* ?? const char * ?? */
rippleWalletGetSourceAddress (BRRippleWallet wallet);


/**
 * Return an address suitable for receiving Ripple  Depending on the nature of Ripple this
 * might just be the accounts' primary address or this might be different every time a
 * target address is requested.
 *
 * @param wallet the wallet for target address
 */
extern void /* ?? const char * ?? */
rippleWalletGetTargetAddress (BRRippleWallet wallet);

#ifdef __cplusplus
}
#endif

#endif /* BRRipple_h */
