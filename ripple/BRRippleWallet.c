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
typedef struct _BRRippleWalletRecord
{
    
} BRRippleWalletRecord;

extern BRRippleWallet
rippleWalletCreate (BRRippleAccount account)
{
    BRRippleWallet wallet = calloc(1, sizeof(BRRippleWalletRecord));
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

