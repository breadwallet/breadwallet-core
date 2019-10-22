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

IMPLEMENT_GENERIC_TYPE(Network,network)

extern BRGenericNetwork
gwmNetworkCreate (const char *type) {
    // There is no 'gen handler' for network create
    return genNetworkAllocAndInit (type, NULL);
}

extern void
gwmNetworkRelease (BRGenericNetwork network) {
    free (network);
}

extern const char *
gwmNetworkGetType (BRGenericNetwork network) {
    return network->type;
}

// MARK: - Account

IMPLEMENT_GENERIC_TYPE(Account, account)

extern BRGenericAccount
gwmAccountCreate (const char *type,
                  UInt512 seed) {
    return genAccountAllocAndInit (type, genHandlerLookup(type)->account.create (type, seed));
}

extern BRGenericAccount
gwmAccountCreateWithPublicKey (const char *type,
                               BRKey publicKey) {
    return genAccountAllocAndInit (type, genHandlerLookup(type)->account.createWithPublicKey (type, publicKey));
}

extern BRGenericAccount
gwmAccountCreateWithSerialization(const char *type,
                                  uint8_t *bytes,
                                  size_t   bytesCount) {
    return genAccountAllocAndInit (type, genHandlerLookup(type)->account.createWithSerialization (type, bytes, bytesCount));
}

extern void
gwmAccountRelease (BRGenericAccount account) {
    account->handlers.free (account->ref);
    free (account);
}

extern const char *
gwmAccountGetType (BRGenericAccount account) {
    return account->type;
}

extern int
gwmAccountHasType (BRGenericAccount account,
                   const char *type) {
    return type == account->type || 0 == strcmp  (type, account->type);
}

extern BRGenericAddress
gwmAccountGetAddress (BRGenericAccount account) {
    return genAddressAllocAndInit (account->type, account->handlers.getAddress (account->ref));
}

extern uint8_t *
gwmAccountGetSerialization (BRGenericAccount account, size_t *bytesCount) {
    return account->handlers.getSerialization (account->ref, bytesCount);
}

extern void
gwmAccountSignTransferWithSeed (BRGenericAccount account,
                                BRGenericTransfer transfer,
                                UInt512 seed) {
    account->handlers.signTransferWithSeed (account->ref, transfer->ref, seed);
}

extern void
gwmAccountSignTransferWithKey (BRGenericAccount account,
                                BRGenericTransfer transfer,
                               BRKey *key) {
    account->handlers.signTransferWithKey (account->ref, transfer->ref, key);
}

// MARK: - Address

IMPLEMENT_GENERIC_TYPE(Address, address)

extern BRGenericAddress
gwmAddressCreate (const char *type,
                  const char *string) {
    BRGenericAddressRef ref = genHandlerLookup(type)->address.create (string);
    return (NULL != ref
            ? genAddressAllocAndInit (type, ref)
            : NULL);
}

extern char *
gwmAddressAsString (BRGenericAddress account) {
    return account->handlers.asString (account->ref);
}

extern int
gwmAddressEqual (BRGenericAddress aid1,
                 BRGenericAddress aid2) {
    assert (0 == strcmp (aid1->type, aid2->type));
    return aid1->handlers.equal (aid1->ref, aid2->ref);
}

extern void
gwmAddressRelease (BRGenericAddress address) {
    address->handlers.free (address->ref);
    free (address);
}

// MARK: - Transfer

IMPLEMENT_GENERIC_TYPE(Transfer, transfer)

extern void
gwmTransferRelease (BRGenericTransfer transfer) {
    transfer->handlers.free (transfer->ref);
    free (transfer);
}

extern BRGenericAddress
gwmTransferGetSourceAddress (BRGenericTransfer transfer) {
    return genAddressAllocAndInit (transfer->type,
                                   transfer->handlers.sourceAddress (transfer->ref));
}

extern BRGenericAddress
gwmTransferGetTargetAddress (BRGenericTransfer transfer) {
    return genAddressAllocAndInit (transfer->type,
                                   transfer->handlers.targetAddress (transfer->ref));
}

extern UInt256
gwmTransferGetAmount (BRGenericTransfer transfer) {
    return transfer->handlers.amount (transfer->ref);
}

// TODO: Direction is needed?

extern UInt256
gwmTransferGetFee (BRGenericTransfer transfer) {
    return transfer->handlers.fee (transfer->ref);
}

extern BRGenericFeeBasis
gwmTransferGetFeeBasis (BRGenericTransfer transfer) {
    return transfer->handlers.feeBasis (transfer->ref);
}

extern BRGenericTransferDirection
gwmTransferGetDirection (BRGenericTransfer transfer) {
    return transfer->handlers.direction (transfer->ref);
}

extern BRGenericHash
gwmTransferGetHash (BRGenericTransfer transfer) {
    return transfer->handlers.hash (transfer->ref);
}

extern uint8_t *
gwmTransferSerialize (BRGenericTransfer transfer, size_t *bytesCount) {
    return transfer->handlers.getSerialization (transfer->ref, bytesCount);
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
gwmWalletCreate (BRGenericAccount account) {
    return genWalletAllocAndInit (account->type,
                                  genHandlerLookup(account->type)->wallet.create (account->ref));
}

extern void
gwmWalletRelease (BRGenericWallet wallet) {
    wallet->handlers.free (wallet->ref);
    free (wallet);
}

extern UInt256
gwmWalletGetBalance (BRGenericWallet wallet) {
    return wallet->handlers.balance (wallet->ref);
}

// TODO: Set Balance?  Add transfer/directed-amount?

extern BRGenericAddress
gwmWalletGetAddress (BRGenericWallet wid) {
    return NULL;
}

extern BRGenericFeeBasis
gwmWalletGetDefaultFeeBasis (BRGenericWallet wid) {
    return wid->defaultFeeBasis;
}

extern void
gwmWalletSetDefaultFeeBasis (BRGenericWallet wid,
                             BRGenericFeeBasis bid) {
    wid->defaultFeeBasis = bid;
}

extern BRGenericTransfer
gwmWalletCreateTransfer (BRGenericWallet wallet,
                         BRGenericAddress target, // TODO: BRGenericAddress - ownership given
                         UInt256 amount,
                         BRGenericFeeBasis estimatedFeeBasis) {
    return genTransferAllocAndInit (wallet->type,
                                    wallet->handlers.createTransfer (wallet->ref,
                                                                     target->ref,
                                                                     amount,
                                                                     estimatedFeeBasis));
}

extern UInt256
gwmWalletEstimateTransferFee (BRGenericWallet wallet,
                              BRGenericAddress target,
                              UInt256 amount,
                              UInt256 pricePerCostFactor,
                              int *overflow) {
    BRGenericFeeBasis feeBasis = wallet->handlers.estimateFeeBasis (wallet->ref,
                                                                    target->ref,
                                                                    amount,
                                                                    pricePerCostFactor);

    return genFeeBasisGetFee (&feeBasis, overflow);
}
