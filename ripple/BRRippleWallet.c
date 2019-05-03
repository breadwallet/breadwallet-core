//
//  BRRippleWallet.c
//  Core
//
//  Created by Carl Cherry on 5/3/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <stdlib.h>
#include "BRRippleWallet.h"

//
// Wallet
//
struct BRRippleWalletRecord
{
    uint64_t balance;
};

extern BRRippleWallet
rippleWalletCreate (BRRippleAccount account)
{
    BRRippleWallet wallet = calloc(1, sizeof(struct BRRippleWalletRecord));
    wallet->balance = 0;
    return wallet;
}

extern void
rippleWalletRelease (BRRippleWallet wallet)
{
    if (wallet) {
        free(wallet);
    }
}

extern BRRippleAddress
rippleWalletGetSourceAddress (BRRippleWallet wallet)
{
    BRRippleAddress address;
    return address;
}

extern BRRippleAddress
rippleWalletGetTargetAddress (BRRippleWallet wallet)
{
    BRRippleAddress address;
    return address;
}

extern uint64_t
rippleWalletGetBalance (BRRippleWallet wallet)
{
    return wallet->balance;
}

extern void
rippleWalletSetBalance (BRRippleWallet wallet, uint64_t balance)
{
    wallet->balance = balance;
}

