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
    return transfer;
}

extern void
genTransferRelease (BRGenericTransfer transfer) {
    transfer->handlers.free (transfer->ref);
    if (NULL != transfer->uids) free (transfer->uids);
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

// TODO: Set Balance?  Add transfer/directed-amount?

extern BRGenericAddress
genWalletGetAddress (BRGenericWallet wid) {
    return NULL;
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
                      BRGenericTransfer transfer) {
    wallet->handlers.addTransfer (wallet->ref, transfer->ref);
}

extern BRGenericTransfer
genWalletCreateTransfer (BRGenericWallet wallet,
                         BRGenericAddress target, // TODO: BRGenericAddress - ownership given
                         UInt256 amount,
                         BRGenericFeeBasis estimatedFeeBasis) {
    BRGenericTransfer transfer = genTransferAllocAndInit (wallet->type,
                                                          wallet->handlers.createTransfer (wallet->ref,
                                                                                           target->ref,
                                                                                           amount,
                                                                                           estimatedFeeBasis));
    int isSource = 1;
    int isTarget = genWalletHasAddress (wallet, target);

    transfer->direction = (isSource && isTarget
                           ? GENERIC_TRANSFER_RECOVERED
                           : (isSource
                              ? GENERIC_TRANSFER_SENT
                              : GENERIC_TRANSFER_RECEIVED));

    genWalletAddTransfer (wallet, transfer);

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
