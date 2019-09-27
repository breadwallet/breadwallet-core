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
#include "ripple/BRRippleFeeBasis.h"
#include "support/BRSet.h"
#include "ethereum/util/BRUtilHex.h"

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

    *address = rippleTransferGetTarget(transfer);
    return address;
}

static UInt256
genericRippleTransferGetAmount (BRGenericTransfer transfer) {
    BRRippleUnitDrops drops = rippleTransferGetAmount (transfer);
    return createUInt256(drops);
}

static UInt256
genericRippleTransferGetFee (BRGenericTransfer transfer) {
    BRRippleTransfer ripple = transfer;
    BRRippleUnitDrops drops = rippleTransferGetFee (ripple);
    return createUInt256(drops);
}

static BRGenericFeeBasis
genericRippleTransferGetFeeBasis (BRGenericTransfer transfer) {
    BRRippleUnitDrops rippleFee = rippleTransferGetFee (transfer);
    BRRippleFeeBasis *feeBasis = (BRRippleFeeBasis*) calloc(1, sizeof(BRRippleFeeBasis));
    feeBasis->pricePerCostFactor = rippleFee;
    feeBasis->costFactor = 1;
    return feeBasis;
}

static BRGenericHash genericRippleTransferGetHash (BRGenericTransfer transfer) {
    BRRippleTransfer ripple = transfer;
    BRRippleTransactionHash hash = rippleTransferGetTransactionId(ripple);
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

static BRArrayOf(BRGenericTransfer)
genericRippleWalletManagerRecoverTransfersFromRawTransaction (uint8_t *bytes,
                                                            size_t   bytesCount) {
    return NULL;
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

static BRGenericTransfer
genericRippleWalletManagerRecoverTransfer (const char *hash,
                                           const char *from,
                                           const char *to,
                                           const char *amount,
                                           const char *currency,
                                           uint64_t timestamp,
                                           uint64_t blockHeight) {
    BRRippleUnitDrops amountDrops;
    sscanf(amount, "%llu", &amountDrops);
    BRRippleAddress toAddress = rippleAddressCreate(to);
    BRRippleAddress fromAddress = rippleAddressCreate(from);
    // Convert the hash string to bytes
    BRRippleTransactionHash txId;
    decodeHex(txId.bytes, sizeof(txId.bytes), hash, strlen(hash));
    return rippleTransferCreate(fromAddress, toAddress, amountDrops, txId, timestamp, blockHeight);
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

static BRGenericAPISyncType
genericRippleWalletManagerGetAPISyncType (void) {
    return GENERIC_SYNC_TYPE_TRANSFER;
}

static UInt256
genericRippleFeeBasisGetPricePerCostFactor (BRGenericFeeBasis feeBasis)
{
    BRRippleUnitDrops pricePerCostFactor = rippleFeeBasisGetPricePerCostFactor(feeBasis);
    printf("pricePerCostFactor: %llu\n", pricePerCostFactor);

    return createUInt256(pricePerCostFactor);
}

static double
genericRippleFeeBasisGetCostFactor (BRGenericFeeBasis feeBasis)
{
    return rippleFeeBasisGetCostFactor(feeBasis);
}

static uint32_t
genericRippleFeeBasisIsEqual (BRGenericFeeBasis fb1, BRGenericFeeBasis fb2)
{
    return rippleFeeBasisIsEqual(fb1, fb2);
}

static void
genericRippleFeeBasisFree (BRGenericFeeBasis feeBasis) {
    BRRippleFeeBasis *rippleFeeBasis = feeBasis;
    free (rippleFeeBasis);
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
        genericRippleWalletManagerRecoverTransfersFromRawTransaction,
        genericRippleWalletManagerInitializeFileService,
        genericRippleWalletManagerLoadTransfers,
        genericRippleWalletManagerGetAPISyncType,
    },

    { // fee basis
        genericRippleFeeBasisGetPricePerCostFactor,
        genericRippleFeeBasisGetCostFactor,
        genericRippleFeeBasisIsEqual,
        genericRippleFeeBasisFree
    }
};

const BRGenericHandlers genericRippleHandlers = &genericRippleHandlersRecord;
