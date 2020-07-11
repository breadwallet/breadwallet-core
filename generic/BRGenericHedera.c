//
//  BRGenericHedera.c
//  Core
//
//  Created by Ed Gamble on 6/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRGenericHedera.h"
#include "hedera/BRHederaAccount.h"
#include "hedera/BRHederaWallet.h"
#include "hedera/BRHederaTransaction.h"
//#include "hedera/BRHederaFeeBasis.h"
#include "support/BRSet.h"
#include "ethereum/util/BRUtilHex.h"

// MARK: - Generic Network

// MARK: - Generic Account

static BRGenericAccountRef
genericHederaAccountCreate (const char *type, UInt512 seed) {
    return (BRGenericAccountRef) hederaAccountCreateWithSeed (seed);
}

static BRGenericAccountRef
genericHederaAccountCreateWithPublicKey (const char *type, BRKey key) {
    //return (BRGenericAccountRef) hederaAccountCreateWithKey (key);
    // TODO - this function will most likey be removed
    return NULL;
}

static BRGenericAccountRef
genericHederaAccountCreateWithSerialization (const char *type, uint8_t *bytes, size_t bytesCount) {
    return (BRGenericAccountRef) hederaAccountCreateWithSerialization (bytes, bytesCount);
}

static void
genericHederaAccountFree (BRGenericAccountRef account) {
    hederaAccountFree ((BRHederaAccount) account);
}

static BRGenericAddressRef
genericHederaAccountGetAddress (BRGenericAccountRef account) {
    return (BRGenericAddressRef) hederaAccountGetAddress((BRHederaAccount) account);
}

static uint8_t *
genericHederaAccountGetSerialization (BRGenericAccountRef account,
                                      size_t *bytesCount) {
    return hederaAccountGetSerialization ((BRHederaAccount) account, bytesCount);
}

static void
genericHederaAccountSignTransferWithSeed (BRGenericAccountRef account,
                                          BRGenericTransferRef transfer,
                                          UInt512 seed)
{
    BRKey publicKey = hederaAccountGetPublicKey((BRHederaAccount)account);
    hederaTransactionSignTransaction ((BRHederaTransaction)transfer, publicKey, seed);
}

static void
genericHederaAccountSignTransferWithKey (BRGenericAccountRef account,
                                         BRGenericTransferRef transfer,
                                         BRKey *key)
{
    // TODO - depracated???
}

// MARK: - Generic Address

static BRGenericAddressRef
genericHederaAddressCreate (const char *string) {
    return (BRGenericAddressRef) hederaAddressCreateFromString (string);
}

static char *
genericHederaAddressAsString (BRGenericAddressRef address) {
    return hederaAddressAsString ((BRHederaAddress) address);
}

static int
genericHederaAddressEqual (BRGenericAddressRef address1,
                           BRGenericAddressRef address2) {
    return hederaAddressEqual ((BRHederaAddress) address1,
                               (BRHederaAddress) address2);
}

static void
genericHederaAddressFree (BRGenericAddressRef address) {
    hederaAddressFree ((BRHederaAddress) address);
}

// MARK: - Generic Transfer

static BRGenericTransferRef
genericHederaTransferCreate (BRGenericAddressRef source,
                             BRGenericAddressRef target,
                             UInt256 amount)
{
    // TODO - I think this is depracated - we only create transfers via the wallet.
    // BRHederaUnitTinyBar thbar = UInt64GetLE(amount.u8);
    // return (BRGenericTransferRef) hederaTransactionCreateNew ((BRHederaAddress) source,
    //                                                       (BRHederaAddress) target,
    //                                                       amountDrops);
    return NULL;
}

static BRGenericTransferRef
genericHederaTransferCopy (BRGenericTransferRef transfer) {
    return (BRGenericTransferRef) hederaTransactionClone ((BRHederaTransaction) transfer);
}

static void
genericHederaTransferFree (BRGenericTransferRef transfer) {
    hederaTransactionFree ((BRHederaTransaction) transfer);
}

static BRGenericAddressRef
genericHederaTransferGetSourceAddress (BRGenericTransferRef transfer) {
    return (BRGenericAddressRef) hederaTransactionGetSource ((BRHederaTransaction) transfer);
}

static BRGenericAddressRef
genericHederaTransferGetTargetAddress (BRGenericTransferRef transfer) {
    return (BRGenericAddressRef) hederaTransactionGetTarget ((BRHederaTransaction) transfer);
}

static UInt256
genericHederaTransferGetAmount (BRGenericTransferRef transfer) {
    BRHederaUnitTinyBar thbar = hederaTransactionGetAmount ((BRHederaTransaction) transfer);
    return uint256Create(thbar);
}

static BRGenericFeeBasis
genericHederaTransferGetFeeBasis (BRGenericTransferRef transfer) {
    // TODO -
    BRHederaUnitTinyBar hederaFee = hederaTransactionGetFee ((BRHederaTransaction) transfer);
    return (BRGenericFeeBasis) {
        uint256Create (hederaFee),
        1
    };
}

static BRGenericHash
genericHederaTransferGetHash (BRGenericTransferRef transfer) {
    BRHederaTransactionHash hash = hederaTransactionGetHash ((BRHederaTransaction) transfer);
    UInt256 value;
    memcpy (value.u8, hash.bytes, 32);
    return (BRGenericHash) { value };
}

static uint8_t *
genericHederaTransferGetSerialization (BRGenericTransferRef transfer, size_t *bytesCount)
{
    *bytesCount = 0;
    return hederaTransactionSerialize((BRHederaTransaction) transfer, bytesCount);
}

// MARK: Generic Wallet

static BRGenericWalletRef
genericHederaWalletCreate (BRGenericAccountRef account) {
    return (BRGenericWalletRef) hederaWalletCreate ((BRHederaAccount) account);
}

static void
genericHederaWalletFree (BRGenericWalletRef wallet) {
    hederaWalletFree ((BRHederaWallet) wallet);
}

static UInt256
genericHederaWalletGetBalance (BRGenericWalletRef wallet) {
    return uint256Create (hederaWalletGetBalance ((BRHederaWallet) wallet));
}

static UInt256
genericHederaWalletGetBalanceLimit (BRGenericWalletRef wallet,
                                    int asMaximum,
                                    int *hasLimit) {
    return uint256Create(0);
}

static BRGenericAddressRef
genericHederaWalletGetAddress (BRGenericWalletRef wallet, int asSource) {
    return (BRGenericAddressRef) (asSource
                                  ? hederaWalletGetSourceAddress ((BRHederaWallet) wallet)
                                  : hederaWalletGetTargetAddress ((BRHederaWallet) wallet));
}

static int
genericHederaWalletHasAddress (BRGenericWalletRef wallet,
                               BRGenericAddressRef address) {
    return hederaWalletHasAddress ((BRHederaWallet) wallet,
                                   (BRHederaAddress) address);
}

static int
genericHederaWalletHasTransfer (BRGenericWalletRef wallet,
                                BRGenericTransferRef transfer) {
    return hederaWalletHasTransfer ((BRHederaWallet) wallet, (BRHederaTransaction) transfer);
}

static void
genericHederaWalletAddTransfer (BRGenericWalletRef wallet,
                                BRGenericTransferRef transfer) {
    // TODO - I don't think it is used
    hederaWalletAddTransfer ((BRHederaWallet) wallet, (BRHederaTransaction) transfer);
}

static void
genericHederaWalletRemTransfer (BRGenericWalletRef wallet,
                                OwnershipKept BRGenericTransferRef transfer) {
    hederaWalletRemTransfer ((BRHederaWallet) wallet, (BRHederaTransaction) transfer);
}

static BRGenericTransferRef
genericHederaWalletCreateTransfer (BRGenericWalletRef wallet,
                                   BRGenericAddressRef target,
                                   UInt256 amount,
                                   BRGenericFeeBasis estimatedFeeBasis,
                                   size_t attributesCount,
                                   BRGenericTransferAttribute *attributes) {
    BRHederaAddress source = hederaWalletGetSourceAddress ((BRHederaWallet) wallet);
    BRHederaUnitTinyBar thbar  = amount.u64[0];
    BRHederaAddress nodeAddress = hederaWalletGetNodeAddress((BRHederaWallet) wallet);
    BRHederaFeeBasis feeBasis;
    feeBasis.costFactor = estimatedFeeBasis.costFactor;
    int overflow = 0;
    feeBasis.pricePerCostFactor = uint64Coerce(estimatedFeeBasis.pricePerCostFactor, &overflow);
    assert(overflow == 0);
    return (BRGenericTransferRef) hederaTransactionCreateNew (source, (BRHederaAddress) target,
                                                           thbar, feeBasis, nodeAddress, NULL);
}

static BRGenericFeeBasis
genericHederaWalletEstimateFeeBasis (BRGenericWalletRef wallet,
                                     BRGenericAddressRef address,
                                     UInt256 amount,
                                     UInt256 pricePerCostFactor) {
    return (BRGenericFeeBasis) {
        pricePerCostFactor,
        1
    };
}

// MARK: - Generic Manager

static BRGenericTransferRef
genericHederaWalletManagerRecoverTransfer (const char *hash,
                                           const char *from,
                                           const char *to,
                                           const char *amount,
                                           const char *currency,
                                           const char *fee,
                                           uint64_t timestamp,
                                           uint64_t blockHeight,
                                           int error) {
    BRHederaUnitTinyBar amountHbar, feeHbar = 0;
    sscanf(amount, "%llu", &amountHbar);
    if (NULL != fee) sscanf(fee,    "%llu", &feeHbar);
    BRHederaAddress toAddress   = hederaAddressCreateFromString(to);
    BRHederaAddress fromAddress = hederaAddressCreateFromString(from);
    // Convert the hash string to bytes
    BRHederaTransactionHash txId;
    hexDecode(txId.bytes, sizeof(txId.bytes), hash, strlen(hash));

    BRHederaTransactionHash txHash;
    memset(txHash.bytes, 0x00, sizeof(txHash.bytes));
    if (hash != NULL) {
        assert(96 == strlen(hash));
        hexDecode(txHash.bytes, sizeof(txHash.bytes), hash, strlen(hash));
    }

    BRHederaTransaction transfer = hederaTransactionCreate(fromAddress, toAddress, amountHbar,
                                                           feeHbar, NULL, txHash, timestamp, blockHeight,
                                                           error);

    hederaAddressFree (toAddress);
    hederaAddressFree (fromAddress);

    return (BRGenericTransferRef) transfer;
}

static BRArrayOf(BRGenericTransferRef)
genericHederaWalletManagerRecoverTransfersFromRawTransaction (uint8_t *bytes,
                                                            size_t   bytesCount) {
    return NULL;
}

static BRGenericAPISyncType
genericHederaWalletManagerGetAPISyncType (void) {
    return GENERIC_SYNC_TYPE_TRANSFER;
}

// MARK: - Generic Handlers

struct BRGenericHandersRecord genericHederaHandlersRecord = {
    "hbar",
    { // Network
    },

    {    // Account
        genericHederaAccountCreate,
        genericHederaAccountCreateWithPublicKey,
        genericHederaAccountCreateWithSerialization,
        genericHederaAccountFree,
        genericHederaAccountGetAddress,
        genericHederaAccountGetSerialization,
        genericHederaAccountSignTransferWithSeed,
        genericHederaAccountSignTransferWithKey,
    },

    {    // Address
        genericHederaAddressCreate,
        genericHederaAddressAsString,
        genericHederaAddressEqual,
        genericHederaAddressFree
    },

    {    // Transfer
        genericHederaTransferCreate,
        genericHederaTransferCopy,
        genericHederaTransferFree,
        genericHederaTransferGetSourceAddress,
        genericHederaTransferGetTargetAddress,
        genericHederaTransferGetAmount,
        genericHederaTransferGetFeeBasis,
        genericHederaTransferGetHash,
        genericHederaTransferGetSerialization,
    },

    {   // Wallet
        genericHederaWalletCreate,
        genericHederaWalletFree,
        genericHederaWalletGetBalance,
        genericHederaWalletGetBalanceLimit,
        genericHederaWalletGetAddress,
        genericHederaWalletHasAddress,
        genericHederaWalletHasTransfer,
        genericHederaWalletAddTransfer,
        genericHederaWalletRemTransfer,
        genericHederaWalletCreateTransfer,
        genericHederaWalletEstimateFeeBasis
    },

    { // Wallet Manager
        genericHederaWalletManagerRecoverTransfer,
        genericHederaWalletManagerRecoverTransfersFromRawTransaction,
        genericHederaWalletManagerGetAPISyncType,
    },
};

const BRGenericHandlers genericHederaHandlers = &genericHederaHandlersRecord;
