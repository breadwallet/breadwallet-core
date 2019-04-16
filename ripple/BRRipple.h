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

#ifdef __cplusplus
extern "C" {
#endif

    //
    // Transaction
    //
    typedef struct BRRippleTransactionRecord *BRRippleTransaction;

    extern BRRippleTransaction
    rippleTransactionCreate (void /* TBD */);

    /**
     * Serialize a Ripple transaction (in a form suitable signing)
     *
     * @param transaction the transaction to serialize
     */
    extern void /* ?? uint8_t * ?? */
    rippleTransactionSerialize /* ForSigning */ (BRRippleTransaction transaction);

    // Some other function to 'attach a signature' and then serialize the transaction +
    // signature for submission?
    
    //
    // Account
    //
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
     * Get the account's primary address
     *
     * @param account the account
     */
    extern void /* ?? const char * ?? */
    rippleAccountGetPrimaryAddress (BRRippleAccount account);

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
