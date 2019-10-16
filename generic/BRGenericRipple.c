//
//  BRGenericRipple.c
//  Core
//
//  Created by Ed Gamble on 6/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRGenericRipple.h"

#include "ripple/BRRippleWallet.h"
#include "ripple/BRRippleTransaction.h"

#include "support/BRSet.h"

// Account
static void *
genericRippleAccountCreate (const char *type, UInt512 seed) {
    return rippleAccountCreateWithSeed (seed);
}

static void *
genericRippleAccountCreateWithPublicKey (const char *type, BRKey key) {
    return rippleAccountCreateWithKey (key);
}

static void *
genericRippleAccountCreateWithSerialization (const char *type, uint8_t *bytes, size_t   bytesCount) {
    return rippleAccountCreateWithSerialization (bytes, bytesCount);
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

static uint8_t *
genericRippleAccountGetSerialization (void *account, size_t *bytesCount) {
    BRRippleAccount ripple = account;
    return rippleAccountGetSerialization (ripple, bytesCount);
}

// Address
static char *
genericRippleAddressAsString (BRGenericAddress address) {
    BRRippleAddress *ripple = address;
    return rippleAddressAsString(*ripple);
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

static BRGenericTransfer
genericRippleWalletManagerRecoverTransfer (uint8_t *bytes,
                                           size_t   bytesCount) {
    return rippleTransactionCreateFromBytes (bytes, (int) bytesCount);
}


/// MARK: - File Service

static const char *fileServiceTypeTransactions = "transactions";

enum {
    RIPPLE_TRANSACTION_VERSION_1
};

static UInt256
fileServiceTypeTransactionV1Identifier (BRFileServiceContext context,
                                        BRFileService fs,
                                        const void *entity) {
    BRRippleTransaction transaction = (BRRippleTransaction) entity;
    BRRippleTransactionHash transactionHash = rippleTransactionGetHash(transaction);

    UInt256 hash;
    memcpy (hash.u32, transactionHash.bytes, 32);

    return hash;
}

static uint8_t *
fileServiceTypeTransactionV1Writer (BRFileServiceContext context,
                                    BRFileService fs,
                                    const void* entity,
                                    uint32_t *bytesCount) {
    BRRippleTransaction transaction = (BRRippleTransaction) entity;

    size_t bufferCount;
    uint8_t *buffer = rippleTransactionSerialize (transaction, &bufferCount);

    // Require (for now) a valid serialization
    assert (NULL != buffer && 0 != bufferCount);

    if (NULL != bytesCount) *bytesCount = (uint32_t) bufferCount;
    return buffer;
}

static void *
fileServiceTypeTransactionV1Reader (BRFileServiceContext context,
                                    BRFileService fs,
                                    uint8_t *bytes,
                                    uint32_t bytesCount) {
    return rippleTransactionCreateFromBytes (bytes, bytesCount);
}

static void
genericRippleWalletManagerInitializeFileService (BRFileServiceContext context,
                                                 BRFileService fileService) {
    if (1 != fileServiceDefineType (fileService, fileServiceTypeTransactions, RIPPLE_TRANSACTION_VERSION_1,
                                    context,
                                    fileServiceTypeTransactionV1Identifier,
                                    fileServiceTypeTransactionV1Reader,
                                    fileServiceTypeTransactionV1Writer) ||

        1 != fileServiceDefineCurrentVersion (fileService, fileServiceTypeTransactions,
                                              RIPPLE_TRANSACTION_VERSION_1))

        return; //  bwmCreateErrorHandler (bwm, 1, fileServiceTypeTransactions);
}

static BRArrayOf(BRGenericTransfer)
genericRippleWalletManagerLoadTransfers (BRFileServiceContext context,
                                         BRFileService fileService) {
    BRArrayOf(BRGenericTransfer) result;
    BRSetOf(BRRippleTransaction) transactionsSet = rippleTransactionSetCreate (5);

    // Load all transactions while upgrading.
    fileServiceLoad (fileService, transactionsSet, fileServiceTypeTransactions, 1);

    array_new (result, BRSetCount(transactionsSet));
    FOR_SET (BRRippleTransaction, transaction, transactionsSet) {
        array_add (result, (BRGenericTransfer) transaction);
    }

    return result;
}

struct BRGenericHandersRecord genericRippleHandlersRecord = {
    "xrp",
    {    // Account
        genericRippleAccountCreate,
        genericRippleAccountCreateWithPublicKey,
        genericRippleAccountCreateWithSerialization,
        genericRippleAccountFree,
        genericRippleAccountGetAddress,
        genericRippleAccountGetSerialization
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
        genericRippleWalletGetBalance,
        // ...
    },

    { // Wallet Manager
        genericRippleWalletManagerRecoverTransfer,
        genericRippleWalletManagerInitializeFileService,
        genericRippleWalletManagerLoadTransfers
    }
};

const BRGenericHandlers genericRippleHandlers = &genericRippleHandlersRecord;
