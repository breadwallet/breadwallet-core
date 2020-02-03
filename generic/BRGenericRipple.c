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
#include "ripple/BRRippleAccount.h"
#include "ripple/BRRippleWallet.h"
#include "ripple/BRRippleTransaction.h"
#include "ripple/BRRippleFeeBasis.h"
#include "support/BRSet.h"
#include "ethereum/util/BRUtilHex.h"

// MARK: - Generic Network

// MARK: - Generic Account

static BRGenericAccountRef
genericRippleAccountCreate (const char *type, UInt512 seed) {
    return (BRGenericAccountRef) rippleAccountCreateWithSeed (seed);
}

static BRGenericAccountRef
genericRippleAccountCreateWithPublicKey (const char *type, BRKey key) {
    return (BRGenericAccountRef) rippleAccountCreateWithKey (key);
}

static BRGenericAccountRef
genericRippleAccountCreateWithSerialization (const char *type, uint8_t *bytes, size_t bytesCount) {
    return (BRGenericAccountRef) rippleAccountCreateWithSerialization (bytes, bytesCount);
}

static void
genericRippleAccountFree (BRGenericAccountRef account) {
    rippleAccountFree ((BRRippleAccount) account);
}

static BRGenericAddressRef
genericRippleAccountGetAddress (BRGenericAccountRef account) {
    return (BRGenericAddressRef) rippleAccountGetAddress((BRRippleAccount) account);
}

static uint8_t *
genericRippleAccountGetSerialization (BRGenericAccountRef account,
                                      size_t *bytesCount) {
    return rippleAccountGetSerialization ((BRRippleAccount) account, bytesCount);
}

static void
genericRippleAccountSignTransferWithSeed (BRGenericAccountRef account,
                                          BRGenericTransferRef transfer,
                                          UInt512 seed)
{
    // Get the transaction pointer from this transfer
    BRRippleTransaction transaction = rippleTransferGetTransaction((BRRippleTransfer) transfer);
    if (transaction) {
        rippleAccountSignTransaction ((BRRippleAccount) account, transaction, seed);
    }
}

static void
genericRippleAccountSignTransferWithKey (BRGenericAccountRef account,
                                         BRGenericTransferRef transfer,
                                         BRKey *key)
{
    // Get the transaction pointer from this transfer
    BRRippleTransaction transaction = rippleTransferGetTransaction ((BRRippleTransfer) transfer);
    if (transaction) {
//        rippleAccountSignTransaction(account, transaction, seed);
        assert (0);
    }
}

// MARK: - Generic Address

static BRGenericAddressRef
genericRippleAddressCreate (const char *string) {
    return (BRGenericAddressRef) rippleAddressCreateFromString (string);
}

static char *
genericRippleAddressAsString (BRGenericAddressRef address) {
    return rippleAddressAsString ((BRRippleAddress) address);
}

static int
genericRippleAddressEqual (BRGenericAddressRef address1,
                           BRGenericAddressRef address2) {
    return rippleAddressEqual ((BRRippleAddress) address1,
                               (BRRippleAddress) address2);
}

static void
genericRippleAddressFree (BRGenericAddressRef address) {
    rippleAddressFree ((BRRippleAddress) address);
}

// MARK: - Generic Transfer

static BRGenericTransferRef
genericRippleTransferCreate (BRGenericAddressRef source,
                             BRGenericAddressRef target,
                             UInt256 amount)
{
    BRRippleUnitDrops amountDrops = UInt64GetLE(amount.u8);

    return (BRGenericTransferRef) rippleTransferCreateNew ((BRRippleAddress) source,
                                                           (BRRippleAddress) target,
                                                           amountDrops);
}

static BRGenericTransferRef
genericRippleTransferCopy (BRGenericTransferRef transfer) {
    return (BRGenericTransferRef) rippleTransferClone ((BRRippleTransfer) transfer);
}

static void
genericRippleTransferFree (BRGenericTransferRef transfer) {
    rippleTransferFree ((BRRippleTransfer) transfer);
}

static BRGenericAddressRef
genericRippleTransferGetSourceAddress (BRGenericTransferRef transfer) {
    return (BRGenericAddressRef) rippleTransferGetSource ((BRRippleTransfer) transfer);
}

static BRGenericAddressRef
genericRippleTransferGetTargetAddress (BRGenericTransferRef transfer) {
    return (BRGenericAddressRef) rippleTransferGetTarget ((BRRippleTransfer) transfer);
}

static UInt256
genericRippleTransferGetAmount (BRGenericTransferRef transfer) {
    BRRippleUnitDrops drops = rippleTransferGetAmount ((BRRippleTransfer) transfer);
    return createUInt256(drops);
}

static BRGenericFeeBasis
genericRippleTransferGetFeeBasis (BRGenericTransferRef transfer) {
    BRRippleUnitDrops rippleFee = rippleTransferGetFee ((BRRippleTransfer) transfer);
    return (BRGenericFeeBasis) {
        createUInt256 (rippleFee),
        1
    };
}

static BRGenericHash
genericRippleTransferGetHash (BRGenericTransferRef transfer) {
    BRRippleTransactionHash hash = rippleTransferGetTransactionId ((BRRippleTransfer) transfer);
    UInt256 value;
    memcpy (value.u8, hash.bytes, 32);
    return (BRGenericHash) { value };
}

static uint8_t *
genericRippleTransferGetSerialization (BRGenericTransferRef transfer, size_t *bytesCount)
{
    uint8_t * result = NULL;
    *bytesCount = 0;
    BRRippleTransaction transaction = rippleTransferGetTransaction ((BRRippleTransfer) transfer);
    if (transaction) {
        result = rippleTransactionSerialize(transaction, bytesCount);
    }
    return result;
}

// MARK: Generic Wallet

static BRGenericWalletRef
genericRippleWalletCreate (BRGenericAccountRef account) {
    return (BRGenericWalletRef) rippleWalletCreate ((BRRippleAccount) account);
}

static void
genericRippleWalletFree (BRGenericWalletRef wallet) {
    rippleWalletFree ((BRRippleWallet) wallet);
}

static UInt256
genericRippleWalletGetBalance (BRGenericWalletRef wallet) {
    return createUInt256 (rippleWalletGetBalance ((BRRippleWallet) wallet));
}

static UInt256
genericRippleWalletGetBalanceLimit (BRGenericWalletRef wallet,
                                    int asMaximum,
                                    int *hasLimit) {
    return createUInt256 (rippleWalletGetBalanceLimit ((BRRippleWallet) wallet, asMaximum, hasLimit));
}

static BRGenericAddressRef
genericRippleGetAddress (BRGenericWalletRef wallet, int asSource) {
    return (BRGenericAddressRef) (asSource
                                  ? rippleWalletGetSourceAddress ((BRRippleWallet) wallet)
                                  : rippleWalletGetTargetAddress ((BRRippleWallet) wallet));
}

static int
genericRippleWalletHasAddress (BRGenericWalletRef wallet,
                               BRGenericAddressRef address) {
    return rippleWalletHasAddress ((BRRippleWallet) wallet,
                                   (BRRippleAddress) address);
}

static int
genericRippleWalletHasTransfer (BRGenericWalletRef wallet,
                                BRGenericTransferRef transfer) {
    return rippleWalletHasTransfer ((BRRippleWallet) wallet, (BRRippleTransfer) transfer);
}

static void
genericRippleWalletAddTransfer (BRGenericWalletRef wallet,
                                OwnershipKept BRGenericTransferRef transfer) {
    rippleWalletAddTransfer ((BRRippleWallet) wallet, (BRRippleTransfer) transfer);
}

static void
genericRippleWalletRemTransfer (BRGenericWalletRef wallet,
                                OwnershipKept BRGenericTransferRef transfer) {
    rippleWalletRemTransfer ((BRRippleWallet) wallet, (BRRippleTransfer) transfer);
}

#define FIELD_OPTION_DESTINATION_TAG        "DestinationTag"
#define FIELD_OPTION_INVOICE_ID             "InvoiceId"

static int // 1 if equal, 0 if not.
genericRippleCompareFieldOption (const char *t1, const char *t2) {
    return 0 == strcasecmp (t1, t2);
}

static BRGenericTransferRef
genericRippleWalletCreateTransfer (BRGenericWalletRef wallet,
                                   BRGenericAddressRef target,
                                   UInt256 amount,
                                   BRGenericFeeBasis estimatedFeeBasis,
                                   size_t attributeCount,
                                   BRGenericTransferAttribute *attributes) {
    BRRippleAddress source  = rippleWalletGetSourceAddress ((BRRippleWallet) wallet);
    BRRippleUnitDrops drops = amount.u64[0];

    BRRippleTransfer transfer = rippleTransferCreateNew (source,
                                                         (BRRippleAddress) target,
                                                         drops);

    BRRippleTransaction transaction = rippleTransferGetTransaction(transfer);

    for (size_t index = 0; index < attributeCount; index++) {
        BRGenericTransferAttribute attribute = attributes[index];
        if (NULL != genTransferAttributeGetVal(attribute)) {
            if (genericRippleCompareFieldOption (genTransferAttributeGetKey(attribute), FIELD_OPTION_DESTINATION_TAG)) {
                BRCoreParseStatus tag;
                sscanf (genTransferAttributeGetVal(attribute), "%u", &tag);
                rippleTransactionSetDestinationTag (transaction, tag);
            }
            else if (genericRippleCompareFieldOption (genTransferAttributeGetKey(attribute), FIELD_OPTION_INVOICE_ID)) {
                // TODO:
            }
            else {
                // TODO: Impossible if validated?
            }
        }
    }

    rippleAddressFree(source);

    return (BRGenericTransferRef) transfer;
}

static BRGenericFeeBasis
genericRippleWalletEstimateFeeBasis (BRGenericWalletRef wallet,
                                     BRGenericAddressRef address,
                                     UInt256 amount,
                                     UInt256 pricePerCostFactor) {
    return (BRGenericFeeBasis) {
        pricePerCostFactor,
        1
    };
}

static const char *knownDestinationTagRequiringAddresses[] = {
    "rLNaPoKeeBjZe2qs6x52yVPZpZ8td4dc6w",           // Coinbase(1)
    "rw2ciyaNshpHe7bCHo4bRWq6pqqynnWKQg",           // Coinbase(2)
    "rEb8TK3gBgk5auZkwc6sHnwrGVJH8DuaLh",           // Binance(1)
    "rJb5KsHsDHF1YS5B5DU6QCkH5NsPaKQTcy",           // Binance(2)
    "rEy8TFcrAPvhpKrwyrscNYyqBGUkE9hKaJ",           // Binance(3)
    "rXieaAC3nevTKgVu2SYoShjTCS2Tfczqx",            // Wirex(1)
    "r9HwsqBnAUN4nF6nDqxd4sgP8DrDnDcZP3",           // BitBay
    "rLbKbPyuvs4wc1h13BEPHgbFGsRXMeFGL6",           // BitBank(1)
    "rw7m3CtVHwGSdhFjV4MyJozmZJv3DYQnsA",           // BitBank(2)
    NULL
};

static int
genericRippleRequiresDestinationTag (BRRippleAddress address) {
    if (NULL == address) return 0;

    char *addressAsString = rippleAddressAsString(address);
    int isRequired = 0;

    for (size_t index = 0; NULL != knownDestinationTagRequiringAddresses[index]; index++)
        if (0 == strcasecmp (addressAsString, knownDestinationTagRequiringAddresses[index])) {
            isRequired = 1;
            break;
        }

    free (addressAsString);
    return isRequired;
}

static const char **
genericRippleWalletGetTransactionAttributeKeys (BRGenericWalletRef wallet,
                                                BRGenericAddressRef address,
                                                int asRequired,
                                                size_t *count) {

    if (genericRippleRequiresDestinationTag ((BRRippleAddress) address)) {
        static size_t requiredCount = 1;
        static const char *requiredNames[] = {
            FIELD_OPTION_DESTINATION_TAG,
        };

        static size_t optionalCount = 1;
        static const char *optionalNames[] = {
            FIELD_OPTION_INVOICE_ID
        };

        if (asRequired) { *count = requiredCount; return requiredNames; }
        else {            *count = optionalCount; return optionalNames; }
    }

    else {
        static size_t requiredCount = 0;
        static const char **requiredNames = NULL;

        static size_t optionalCount = 2;
        static const char *optionalNames[] = {
            FIELD_OPTION_DESTINATION_TAG,
            FIELD_OPTION_INVOICE_ID
        };

        if (asRequired) { *count = requiredCount; return requiredNames; }
        else {            *count = optionalCount; return optionalNames; }
    }
}

static int
genericRippleWalletValidateTransactionAttribute (BRGenericWalletRef wallet,
                                                 BRGenericTransferAttribute attribute) {
    const char *key = genTransferAttributeGetKey (attribute);
    const char *val = genTransferAttributeGetVal (attribute);

    // If attribute.value is NULL, we validate unless the attribute.value is required.
    if (NULL == val) return !genTransferAttributeIsRequired(attribute);

    if (genericRippleCompareFieldOption (key, FIELD_OPTION_DESTINATION_TAG)) {
        uint32_t tag;
        return 1 == sscanf(val, "%u", &tag);
    }
    else if (genericRippleCompareFieldOption (key, FIELD_OPTION_INVOICE_ID)) {
        BRCoreParseStatus status;
        createUInt256Parse(val, 10, &status);
        return CORE_PARSE_OK == status;
    }
    else return 0;
}

static int
genericRippleWalletValidateTransactionAttributes (BRGenericWalletRef wallet,
                                                  size_t attributesCount,
                                                  BRGenericTransferAttribute *attributes) {
    // Validate one-by-one
    for (size_t index = 0; index < attributesCount; index++)
        if (0 == genericRippleWalletValidateTransactionAttribute (wallet, attributes[index]))
            return 0;
    return 1;
}

// MARK: - Generic Manager

static BRGenericTransferRef
genericRippleWalletManagerRecoverTransfer (const char *hash,
                                           const char *from,
                                           const char *to,
                                           const char *amount,
                                           const char *currency,
                                           const char *fee,
                                           uint64_t timestamp,
                                           uint64_t blockHeight,
                                           int error) {
    BRRippleUnitDrops amountDrops, feeDrops = 0;
    sscanf(amount, "%llu", &amountDrops);
    if (NULL != fee) sscanf(fee,    "%llu", &feeDrops);
    BRRippleAddress toAddress   = rippleAddressCreateFromString(to);
    BRRippleAddress fromAddress = rippleAddressCreateFromString(from);
    // Convert the hash string to bytes
    BRRippleTransactionHash txId;
    decodeHex(txId.bytes, sizeof(txId.bytes), hash, strlen(hash));

    BRRippleTransfer transfer = rippleTransferCreate(fromAddress, toAddress, amountDrops, feeDrops, txId, timestamp, blockHeight, error);

    rippleAddressFree (toAddress);
    rippleAddressFree (fromAddress);

    return (BRGenericTransferRef) transfer;
}

static BRArrayOf(BRGenericTransferRef)
genericRippleWalletManagerRecoverTransfersFromRawTransaction (uint8_t *bytes,
                                                            size_t   bytesCount) {
    return NULL;
}

static BRGenericAPISyncType
genericRippleWalletManagerGetAPISyncType (void) {
    return GENERIC_SYNC_TYPE_TRANSFER;
}

// MARK: - Generic Handlers

struct BRGenericHandersRecord genericRippleHandlersRecord = {
    GEN_NETWORK_TYPE_XRP,
    { // Network
    },

    {    // Account
        genericRippleAccountCreate,
        genericRippleAccountCreateWithPublicKey,
        genericRippleAccountCreateWithSerialization,
        genericRippleAccountFree,
        genericRippleAccountGetAddress,
        genericRippleAccountGetSerialization,
        genericRippleAccountSignTransferWithSeed,
        genericRippleAccountSignTransferWithKey,
    },

    {    // Address
        genericRippleAddressCreate,
        genericRippleAddressAsString,
        genericRippleAddressEqual,
        genericRippleAddressFree
    },

    {    // Transfer
        genericRippleTransferCreate,
        genericRippleTransferCopy,
        genericRippleTransferFree,
        genericRippleTransferGetSourceAddress,
        genericRippleTransferGetTargetAddress,
        genericRippleTransferGetAmount,
        genericRippleTransferGetFeeBasis,
        genericRippleTransferGetHash,
        genericRippleTransferGetSerialization,
    },

    {   // Wallet
        genericRippleWalletCreate,
        genericRippleWalletFree,
        genericRippleWalletGetBalance,
        /* set balance */
        genericRippleWalletGetBalanceLimit,
        genericRippleGetAddress,
        genericRippleWalletHasAddress,
        genericRippleWalletHasTransfer,
        genericRippleWalletAddTransfer,
        genericRippleWalletRemTransfer,
        genericRippleWalletCreateTransfer,
        genericRippleWalletEstimateFeeBasis,

        genericRippleWalletGetTransactionAttributeKeys,
        genericRippleWalletValidateTransactionAttribute,
        genericRippleWalletValidateTransactionAttributes
    },

    { // Wallet Manager
        genericRippleWalletManagerRecoverTransfer,
        genericRippleWalletManagerRecoverTransfersFromRawTransaction,
        genericRippleWalletManagerGetAPISyncType,
    },
};

const BRGenericHandlers genericRippleHandlers = &genericRippleHandlersRecord;
