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
    BRRippleUnitDrops balance; // XRP balance
    BRRippleUnitDrops feeBasis; // Base fee for transactions

    // Ripple account
    BRRippleAccount account;
};

extern BRRippleWallet
rippleWalletCreate (BRRippleAccount account)
{
    BRRippleWallet wallet = calloc(1, sizeof(struct BRRippleWalletRecord));
    wallet->account = account;
    return wallet;
}

extern void
rippleWalletFree (BRRippleWallet wallet)
{
    if (wallet) {
        free(wallet);
    }
}

extern BRRippleAddress
rippleWalletGetSourceAddress (BRRippleWallet wallet)
{
    assert(wallet);
    return rippleAccountGetPrimaryAddress(wallet->account);
}

extern BRRippleAddress
rippleWalletGetTargetAddress (BRRippleWallet wallet)
{
    assert(wallet);
    return rippleAccountGetPrimaryAddress(wallet->account);
}

extern BRRippleUnitDrops
rippleWalletGetBalance (BRRippleWallet wallet)
{
    assert(wallet);
    return wallet->balance;
}

extern void
rippleWalletSetBalance (BRRippleWallet wallet, BRRippleUnitDrops balance)
{
    assert(wallet);
    wallet->balance = balance;
}

extern void rippleWalletSetDefaultFeeBasis (BRRippleWallet wallet, BRRippleUnitDrops feeBasis)
{
    assert(wallet);
    wallet->feeBasis = feeBasis;
}

extern BRRippleUnitDrops rippleWalletGetDefaultFeeBasis (BRRippleWallet wallet)
{
    assert(wallet);
    return wallet->feeBasis;
}

