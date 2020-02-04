//
//  BRGeneric.c
//  Core
//
//  Created by Ed Gamble on 6/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "BRGenericPrivate.h"

// MARK: - Network

// IMPLEMENT_GENERIC_TYPE(Network,network)

private_extern BRGenericNetwork
genNetworkAllocAndInit (const char *type,
                        BRGenericNetworkRef ref,
                        int isMainnet) {
    BRGenericNetwork network = calloc (1, sizeof (struct BRGenericNetworkRecord));
    network->type = type;
    network->handlers = genHandlerLookup(type)->network;
    network->ref = ref;
    network->isMainnet = isMainnet;
    return network;
}

extern BRGenericNetwork
genNetworkCreate (const char *type, int isMainnet) {
    // There is no 'gen handler' for network create
    return genNetworkAllocAndInit (type, NULL, isMainnet);
}

extern void
genNetworkRelease (BRGenericNetwork network) {
    free (network);
}

extern const char *
genNetworkGetType (BRGenericNetwork network) {
    return network->type;
}

extern int
genNetworkIsMainnet (BRGenericNetwork network) {
    return network->isMainnet;
}

// MARK: - Account

IMPLEMENT_GENERIC_TYPE(Account, account)

extern BRGenericAccount
genAccountCreate (const char *type,
                  UInt512 seed) {
    return genAccountAllocAndInit (type, genHandlerLookup(type)->account.create (type, seed));
}

extern BRGenericAccount
genAccountCreateWithPublicKey (const char *type,
                               BRKey publicKey) {
    return genAccountAllocAndInit (type, genHandlerLookup(type)->account.createWithPublicKey (type, publicKey));
}

extern BRGenericAccount
genAccountCreateWithSerialization(const char *type,
                                  uint8_t *bytes,
                                  size_t   bytesCount) {
    return genAccountAllocAndInit (type, genHandlerLookup(type)->account.createWithSerialization (type, bytes, bytesCount));
}

extern void
genAccountRelease (BRGenericAccount account) {
    account->handlers.free (account->ref);
    free (account);
}

extern const char *
genAccountGetType (BRGenericAccount account) {
    return account->type;
}

extern int
genAccountHasType (BRGenericAccount account,
                   const char *type) {
    return type == account->type || 0 == strcmp  (type, account->type);
}

extern BRGenericAddress
genAccountGetAddress (BRGenericAccount account) {
    return genAddressAllocAndInit (account->type, account->handlers.getAddress (account->ref));
}

extern uint8_t *
genAccountGetSerialization (BRGenericAccount account, size_t *bytesCount) {
    return account->handlers.getSerialization (account->ref, bytesCount);
}

extern void
genAccountSignTransferWithSeed (BRGenericAccount account,
                                BRGenericTransfer transfer,
                                UInt512 seed) {
    account->handlers.signTransferWithSeed (account->ref, transfer->ref, seed);
}

extern void
genAccountSignTransferWithKey (BRGenericAccount account,
                               BRGenericTransfer transfer,
                               BRKey *key) {
    account->handlers.signTransferWithKey (account->ref, transfer->ref, key);
}

// MARK: - Address

IMPLEMENT_GENERIC_TYPE(Address, address)

extern BRGenericAddress
genAddressCreate (const char *type,
                  const char *string) {
    BRGenericAddressRef ref = genHandlerLookup(type)->address.create (string);
    return (NULL != ref
            ? genAddressAllocAndInit (type, ref)
            : NULL);
}

extern char *
genAddressAsString (BRGenericAddress account) {
    return account->handlers.asString (account->ref);
}

extern int
genAddressEqual (BRGenericAddress aid1,
                 BRGenericAddress aid2) {
    assert (0 == strcmp (aid1->type, aid2->type));
    return aid1->handlers.equal (aid1->ref, aid2->ref);
}

extern void
genAddressRelease (BRGenericAddress address) {
    address->handlers.free (address->ref);
    free (address);
}

// MARK: - Transfer Attribute

struct BRGenericTransferAttributeRecord {
        char *key;
        char *val;
        int isRequired;
};

extern BRGenericTransferAttribute
genTransferAttributeCreate (const char *key, const char *val, int isRequired) {
    assert (NULL != key);
    BRGenericTransferAttribute attribute = calloc (1, sizeof (struct BRGenericTransferAttributeRecord));

    attribute->key = strdup (key);
    attribute->val = (NULL == val ? NULL : strdup(val));
    attribute->isRequired = isRequired;

    return attribute;
}

extern void
genTransferAttributeRelease (BRGenericTransferAttribute attribute) {
    free (attribute->key);
    if (NULL != attribute->val) free (attribute->val);
    memset (attribute, 0, sizeof (struct BRGenericTransferAttributeRecord));
    free (attribute);
}


extern const char *
genTransferAttributeGetKey (BRGenericTransferAttribute attribute) {
    return attribute->key;
}

extern const char *
genTransferAttributeGetVal (BRGenericTransferAttribute attribute) {
    return attribute->val;
}

extern int
genTransferAttributeIsRequired (BRGenericTransferAttribute attribute) {
    return attribute->isRequired;
}

extern void
genTransferAttributeReleaseAll (OwnershipGiven BRArrayOf(BRGenericTransferAttribute) attributes) {
    if (NULL == attributes) return;
    array_free_all (attributes, genTransferAttributeRelease);
}

// MARK: - Transfer

private_extern BRGenericTransfer
genTransferAllocAndInit (const char *type,
                         BRGenericTransferRef ref) {
    BRGenericTransfer transfer = calloc (1, sizeof (struct BRGenericTransferRecord));
    transfer->type = type;
    transfer->uids = NULL;
    transfer->handlers = genHandlerLookup (type)->transfer;
    transfer->ref = ref;
    transfer->state = genTransferStateCreateOther (GENERIC_TRANSFER_STATE_CREATED);
    transfer->direction = GENERIC_TRANSFER_RECOVERED;
    array_new(transfer->attributes, 0);
    return transfer;
}

extern BRGenericTransfer
genTransferCopy (const BRGenericTransfer transfer) {
    BRGenericTransfer copy = genTransferAllocAndInit (transfer->type,
                                                      transfer->handlers.copy (transfer->ref));
    copy->uids = (NULL == transfer->uids ? NULL : strdup (transfer->uids));
    copy->state = transfer->state;
    copy->direction = transfer->direction;
    genTransferSetAttributes (copy, transfer->attributes);

    return copy;
}

extern void
genTransferRelease (BRGenericTransfer transfer) {
    transfer->handlers.free (transfer->ref);
    if (NULL != transfer->uids) free (transfer->uids);
    genTransferAttributeReleaseAll (transfer->attributes);

    memset (transfer, 0, sizeof (struct BRGenericTransferRecord));
    free (transfer);
}

extern BRGenericAddress
genTransferGetSourceAddress (BRGenericTransfer transfer) {
    return genAddressAllocAndInit (transfer->type,
                                   transfer->handlers.sourceAddress (transfer->ref));
}

extern BRGenericAddress
genTransferGetTargetAddress (BRGenericTransfer transfer) {
    return genAddressAllocAndInit (transfer->type,
                                   transfer->handlers.targetAddress (transfer->ref));
}

extern UInt256
genTransferGetAmount (BRGenericTransfer transfer) {
    return transfer->handlers.amount (transfer->ref);
}

// TODO: Direction is needed?

extern BRGenericFeeBasis
genTransferGetFeeBasis (BRGenericTransfer transfer) {
    return transfer->handlers.feeBasis (transfer->ref);
}

extern BRGenericTransferDirection
genTransferGetDirection (BRGenericTransfer transfer) {
    return transfer->direction;
}

extern BRGenericHash
genTransferGetHash (BRGenericTransfer transfer) {
    return transfer->handlers.hash (transfer->ref);
}


extern const char *
genTransferGetUIDS (BRGenericTransfer transfer) {
    return transfer->uids;
}

extern void
genTransferSetUIDS (BRGenericTransfer transfer,
                    const char *uids) {
    if (NULL != transfer->uids) free (transfer->uids);
    transfer->uids = (NULL == uids ? NULL : strdup (uids));
}

extern BRGenericTransferState
genTransferGetState (BRGenericTransfer transfer) {
    return transfer->state;
}

extern void
genTransferSetState (BRGenericTransfer transfer,
                     BRGenericTransferState state) {
    transfer->state = state;
}

extern OwnershipKept BRArrayOf(BRGenericTransferAttribute)
genTransferGetAttributes (BRGenericTransfer transfer) {
    return transfer->attributes;
}

extern void
genTransferSetAttributes (BRGenericTransfer transfer,
                          OwnershipKept BRArrayOf(BRGenericTransferAttribute) attributes) {
    genTransferAttributeReleaseAll (transfer->attributes);

    size_t attributesCount = (NULL == attributes ? 0 : array_count(attributes));

    array_new (transfer->attributes, (attributesCount > 0 ? attributesCount : 1));
    for (size_t index = 0; index < attributesCount; index++) {
        BRGenericTransferAttribute attribute = attributes[index];
        array_add (transfer->attributes,
                   genTransferAttributeCreate (genTransferAttributeGetKey(attribute),
                                               genTransferAttributeGetVal(attribute),
                                               genTransferAttributeIsRequired(attribute)));
    }
}

extern int
genTransferEqual (BRGenericTransfer t1,
                  BRGenericTransfer t2) {
    return (t1 == t2 ||
            (NULL != t1->uids && NULL != t2->uids
             ? 0 == strcmp (t1->uids, t2->uids)
             : genericHashEqual (genTransferGetHash (t1),
                                 genTransferGetHash (t2))));
}

extern uint8_t *
genTransferSerialize (BRGenericTransfer transfer, size_t *bytesCount) {
    return transfer->handlers.getSerialization (transfer->ref, bytesCount);
}

static size_t
genTransferGetHashForSet (const void *transferPtr) {
    BRGenericTransfer transfer = (BRGenericTransfer) transferPtr;
    return genTransferGetHash (transfer).value.u32[0];
}

static int
genTransferIsEqualForSet (const void *transferPtr1, const void *transferPtr2) {
    return eqUInt256 (genTransferGetHash((BRGenericTransfer) transferPtr1).value,
                      genTransferGetHash((BRGenericTransfer) transferPtr2).value);
}

extern BRSetOf (BRGenericTransfer)
genTransferSetCreate (size_t capacity) {
    return BRSetNew (genTransferGetHashForSet, genTransferIsEqualForSet, capacity);
}

// MARK: - Wallet

private_extern BRGenericWallet
genWalletAllocAndInit (const char *type,
                       BRGenericWalletRef ref) {
    BRGenericWallet wallet = calloc (1, sizeof (struct BRGenericWalletRecord));
    wallet->type = type;
    wallet->handlers = genHandlerLookup(type)->wallet;
    wallet->ref = ref;
    wallet->defaultFeeBasis = genFeeBasisCreate (UINT256_ZERO, 0);
    return wallet;
}

extern BRGenericWallet
genWalletCreate (BRGenericAccount account) {
    return genWalletAllocAndInit (account->type,
                                  genHandlerLookup(account->type)->wallet.create (account->ref));
}

extern void
genWalletRelease (BRGenericWallet wallet) {
    wallet->handlers.free (wallet->ref);
    free (wallet);
}

extern UInt256
genWalletGetBalance (BRGenericWallet wallet) {
    return wallet->handlers.balance (wallet->ref);
}

extern UInt256
genWalletGetBalanceLimit (BRGenericWallet wallet,
                          BRCryptoBoolean asMaximum,
                          BRCryptoBoolean *hasLimit) {
    int genHasLimit = 0;
    UInt256 limit = wallet->handlers.balanceLimit (wallet->ref, CRYPTO_TRUE == asMaximum, &genHasLimit);
    *hasLimit = AS_CRYPTO_BOOLEAN(genHasLimit);
    return limit;
}

// TODO: Set Balance?  Add transfer/directed-amount?

extern BRGenericAddress
genWalletGetAddress (BRGenericWallet wid) {
    return genAddressAllocAndInit (wid->type,
                                   wid->handlers.getAddress (wid->ref, 1));
}

extern int
genWalletHasAddress (BRGenericWallet wallet,
                     BRGenericAddress address) {
    return wallet->handlers.hasAddress (wallet->ref, address->ref);
}

extern BRGenericFeeBasis
genWalletGetDefaultFeeBasis (BRGenericWallet wid) {
    return wid->defaultFeeBasis;
}

extern void
genWalletSetDefaultFeeBasis (BRGenericWallet wid,
                             BRGenericFeeBasis bid) {
    wid->defaultFeeBasis = bid;
}

extern int
genWalletHasTransfer (BRGenericWallet wallet,
                      BRGenericTransfer transfer) {
    return wallet->handlers.hasTransfer (wallet->ref, transfer->ref);
}

extern void
genWalletAddTransfer (BRGenericWallet wallet,
                      OwnershipKept BRGenericTransfer transfer) {
    wallet->handlers.addTransfer (wallet->ref, transfer->ref);
}

extern void
genWalletRemTransfer (BRGenericWallet wallet,
                      OwnershipKept BRGenericTransfer transfer) {
    wallet->handlers.remTransfer (wallet->ref, transfer->ref);
}

extern BRGenericTransfer
genWalletCreateTransfer (BRGenericWallet wallet,
                         BRGenericAddress target, // TODO: BRGenericAddress - ownership given
                         UInt256 amount,
                         BRGenericFeeBasis estimatedFeeBasis) {
    return genWalletCreateTransferWithAttributes (wallet, target, amount, estimatedFeeBasis, NULL);
}

extern BRGenericTransfer
genWalletCreateTransferWithAttributes (BRGenericWallet wallet,
                                       BRGenericAddress target,
                                       UInt256 amount,
                                       BRGenericFeeBasis estimatedFeeBasis,
                                       OwnershipKept BRArrayOf(BRGenericTransferAttribute) attributes) {

    size_t attributesCount = (NULL == attributes ? 0 : array_count(attributes));

    // Create the transfer (XRP, etc) with the provided attributes.
    BRGenericTransfer transfer = genTransferAllocAndInit (wallet->type,
                                                          wallet->handlers.createTransfer (wallet->ref,
                                                                                           target->ref,
                                                                                           amount,
                                                                                           estimatedFeeBasis,
                                                                                           attributesCount,
                                                                                           attributes));

    // Save the attributes so that they can be persistently stored.
    genTransferSetAttributes (transfer, attributes);

    int isSource = 1;
    int isTarget = genWalletHasAddress (wallet, target);

    transfer->direction = (isSource && isTarget
                           ? GENERIC_TRANSFER_RECOVERED
                           : (isSource
                              ? GENERIC_TRANSFER_SENT
                              : GENERIC_TRANSFER_RECEIVED));

    // We don't add `transfer` to `wallet`.  Let an explicit `genWalletAddTransfer` do that.

    return transfer;
}

extern BRGenericFeeBasis
genWalletEstimateTransferFee (BRGenericWallet wallet,
                              BRGenericAddress target,
                              UInt256 amount,
                              UInt256 pricePerCostFactor) {
    return wallet->handlers.estimateFeeBasis (wallet->ref,
                                              target->ref,
                                              amount,
                                              pricePerCostFactor);
}


extern size_t
genWalletGetTransferAttributeCount (BRGenericWallet wallet,
                                    BRGenericAddress target) {
    size_t countRequired, countOptional;
    BRGenericAddressRef targetRef = (NULL == target ? NULL : target->ref);
    wallet->handlers.getTransactionAttributeKeys (wallet->ref, targetRef, 1, &countRequired);
    wallet->handlers.getTransactionAttributeKeys (wallet->ref, targetRef, 0, &countOptional);
    return countRequired + countOptional;
}

extern OwnershipGiven BRGenericTransferAttribute
genWalletGetTransferAttributeAt (BRGenericWallet wallet,
                                 BRGenericAddress target,
                                 size_t index) {
    size_t countRequired, countOptional;
    BRGenericAddressRef targetRef = (NULL == target ? NULL : target->ref);
    const char **keysRequired = wallet->handlers.getTransactionAttributeKeys (wallet->ref, targetRef, 1, &countRequired);
    const char **keysOptional = wallet->handlers.getTransactionAttributeKeys (wallet->ref, targetRef, 0, &countOptional);

    assert (index < (countRequired + countOptional));

    int isRequired = (index < countRequired);
    const char **keys      = (isRequired ? keysRequired : keysOptional);
    size_t       keysIndex = (isRequired ? index : (index - countRequired));

    return genTransferAttributeCreate (keys[keysIndex], NULL, isRequired);
}

static int // 1 if equal, 0 if not.
genWalletCompareFieldOption (const char *t1, const char *t2) {
    return 0 == strcasecmp (t1, t2);
}

extern BRCryptoBoolean
genWalletHasTransferAttributeForKey (BRGenericWallet wallet,
                                     BRGenericAddress target,
                                     const char *key,
                                     const char **keyFound,
                                     BRCryptoBoolean *isRequired) {
    assert (NULL != keyFound);

    size_t countRequired, countOptional;
    BRGenericAddressRef targetRef = (NULL == target ? NULL : target->ref);
    const char **keysRequired = wallet->handlers.getTransactionAttributeKeys (wallet->ref, targetRef, 1, &countRequired);
    const char **keysOptional = wallet->handlers.getTransactionAttributeKeys (wallet->ref, targetRef, 0, &countOptional);

    if (NULL != keysRequired)
        for (size_t index = 0; index < countRequired; index++)
            if (genWalletCompareFieldOption (key, keysRequired[index])) {
                *keyFound = keysRequired[index];
                *isRequired = CRYPTO_TRUE;
                return CRYPTO_TRUE;
            }

    if (NULL != keysOptional)
        for (size_t index = 0; index < countOptional; index++)
            if (genWalletCompareFieldOption (key, keysOptional[index])) {
                *keyFound = keysOptional[index];
                *isRequired = CRYPTO_FALSE;
                return CRYPTO_TRUE;
            }

    *keyFound = NULL;
    *isRequired = CRYPTO_FALSE;
    return CRYPTO_FALSE;
}


extern BRCryptoBoolean
genWalletRequiresTransferAttributeForKey (BRGenericWallet wallet,
                                          BRGenericAddress target,
                                          const char *key) {
    size_t countRequired;
    BRGenericAddressRef targetRef = (NULL == target ? NULL : target->ref);
    const char **keysRequired = wallet->handlers.getTransactionAttributeKeys (wallet->ref, targetRef, 1, &countRequired);

    if (NULL != keysRequired)
        for (size_t index = 0; NULL != keysRequired[index]; index++)
            if (genWalletCompareFieldOption (key, keysRequired[index]))
                return CRYPTO_TRUE;
    return CRYPTO_FALSE;
}

extern BRCryptoBoolean
genWalletValidateTransferAttribute (BRGenericWallet wallet,
                                    BRGenericTransferAttribute attribute) {
    return AS_CRYPTO_BOOLEAN (wallet->handlers.validateTransactionAttribute (wallet->ref, attribute));
}

extern BRCryptoBoolean
genWalletValidateTransferAttributes (BRGenericWallet wallet,
                                     OwnershipKept BRArrayOf(BRGenericTransferAttribute) attributes) {
    size_t attributesCount = (NULL == attributes ? 0 : array_count(attributes));

    return AS_CRYPTO_BOOLEAN (0 == attributesCount
                              || wallet->handlers.validateTransactionAttributes (wallet->ref, attributesCount, attributes));
}
