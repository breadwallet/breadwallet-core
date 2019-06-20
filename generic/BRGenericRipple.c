//
//  BRGenericRipple.c
//  Core
//
//  Created by Ed Gamble on 6/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

#include "BRGenericRipple.h"

#include "ripple/BRRippleWallet.h"
#include "ripple/BRRippleTransaction.h"

// Account
static void *
genericRippleAccountCreate (const char *type, UInt512 seed) {
    return rippleAccountCreateWithSeed(seed);
}

static void
genericRippleAccountFree (void *account) {
    BRRippleAccount ripple = account;
    rippleAccountFree (ripple);
}

static BRGenericAddress
genericRippleAccountGetAddress (void *account) {
    BRRippleAccount ripple = account;
    BRRippleAddress address = rippleAccountGetAddress(ripple);

    BRRippleAddress *result = malloc (sizeof (BRRippleAddress));
    memcpy (result, address.bytes, sizeof (BRRippleAddress));
    return result;
}

// Address
static char *
genericRippleAddressAsString (BRGenericAddress address) {
    // BRRippleAddress *ripple = address;
    return "rippleAddress";
}

static int
genericRippleAddressEqual (BRGenericAddress address1,
                           BRGenericAddress address2) {
    BRRippleAddress *ripple1 = address1;
    BRRippleAddress *ripple2 = address2;
    return rippleAddressEqual (*ripple1, *ripple2);
}
// Transfer

static BRGenericAddress
genericRippleTransferGetSourceAddress (BRGenericTransfer transfer) {
    BRRippleAddress *address = malloc (sizeof (BRRippleAddress));

    *address = rippleTransactionGetSource(transfer);
    return address;
}

static BRGenericAddress
genericRippleTransferGetTargetAddress (BRGenericTransfer transfer) {
    BRRippleAddress *address = malloc (sizeof (BRRippleAddress));

    *address = rippleTransactionGetTarget(transfer);
    return address;
}

static UInt256
genericRippleTransferGetAmount (BRGenericTransfer transfer) {
    BRRippleUnitDrops drops = rippleTransactionGetAmount (transfer);
    return createUInt256(drops);
}

static UInt256
genericRippleTransferGetFee (BRGenericTransfer transfer) {
    BRRippleTransaction ripple = transfer;
    BRRippleUnitDrops drops = rippleTransactionGetFee (ripple);
    return createUInt256(drops);
}

static BRGenericFeeBasis
genericRippleTransferGetFeeBasis (BRGenericTransfer transfer) {
    BRRippleTransaction ripple = transfer;
    BRRippleFeeBasis rippleFeeBasis = rippleTransactionGetFeeBasis (ripple);

    BRGenericFeeBasis feeBasis = malloc (sizeof (BRRippleFeeBasis));
    memcpy (feeBasis, &rippleFeeBasis, sizeof (BRRippleFeeBasis));
    return feeBasis;
}

static BRGenericHash genericRippleTransferGetHash (BRGenericTransfer transfer) {
    BRRippleTransaction ripple = transfer;
    BRRippleTransactionHash hash = rippleTransactionGetHash(ripple);
    UInt256 value;
    memcpy (value.u8, hash.bytes, 32);
    return (BRGenericHash) { value };
}

// Wallet

static BRGenericWallet
genericRippleWalletCreate (BRGenericAccount account) {
    BRGenericAccountWithType accountWithType = account;
    return rippleWalletCreate(accountWithType->base);
}

static void
genericRippleWalletFree (BRGenericWallet wallet) {
    BRRippleWallet ripple = wallet;
    rippleWalletFree (ripple);
}

static UInt256
genericRippleWalletGetBalance (BRGenericWallet wallet) {
    BRGenericWallet ripple = wallet;
    return createUInt256(rippleWalletGetBalance(ripple));
}

struct BRGenericHandersRecord genericRippleHandlersRecord = {
    "xrp",
    {    // Account
        genericRippleAccountCreate,
        genericRippleAccountFree,
        genericRippleAccountGetAddress
    },

    {    // Address
        genericRippleAddressAsString,
        genericRippleAddressEqual
    },

    {    // Transfer
        genericRippleTransferGetSourceAddress,
        genericRippleTransferGetTargetAddress,
        genericRippleTransferGetAmount,
        genericRippleTransferGetFee,
        genericRippleTransferGetFeeBasis,
        genericRippleTransferGetHash
    },

    {   // Wallet
        genericRippleWalletCreate,
        genericRippleWalletFree,
        genericRippleWalletGetBalance
    }
};

const BRGenericHandlers genericRippleHandlers = &genericRippleHandlersRecord;
