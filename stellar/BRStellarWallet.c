//
//  BRStellarWallet.c
//  Core
//
//  Created by Carl Cherry on 6/12/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <stdlib.h>
#include "BRStellarWallet.h"

//
// Wallet
//
struct BRStellarWalletRecord
{
    // Stellar account
    BRStellarAccount account;
    BRStellarAmount  balance;
};

extern BRStellarWallet
stellarWalletCreate (BRStellarAccount account)
{
    BRStellarWallet wallet = calloc(1, sizeof(struct BRStellarWalletRecord));
    wallet->account = account;
    return wallet;
}

extern void
stellarWalletFree (BRStellarWallet wallet)
{
    if (wallet) {
        free(wallet);
    }
}

extern BRStellarAddress
stellarWalletGetSourceAddress (BRStellarWallet wallet)
{
    assert(wallet);
    return stellarAccountGetPrimaryAddress(wallet->account);
}

extern BRStellarAddress
stellarWalletGetTargetAddress (BRStellarWallet wallet)
{
    assert(wallet);
    return stellarAccountGetPrimaryAddress(wallet->account);
}

extern BRStellarAmount
stellarWalletGetBalance (BRStellarWallet wallet)
{
    assert(wallet);
    return wallet->balance;
}

extern void
stellarWalletSetBalance (BRStellarWallet wallet, BRStellarAmount balance)
{
    assert(wallet);
    wallet->balance = balance;
}

