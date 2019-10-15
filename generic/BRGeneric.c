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
    return genNetworkAllocAndInit (type, sizeof (struct BRGenericNetworkRecord));
}

extern void
gwmNetworkRelease (BRGenericNetwork network) {
    free (network);
}

extern BRGenericAddress
gwmNetworkAddressCreate (BRGenericNetwork network, const char * address) {
    return network->handlers.networkAddressCreate(address);
}

extern void
gwmNetworkAddressRelease (BRGenericNetwork network, BRGenericAddress address) {
    return network->handlers.networkAddressFree(address);
}

// This is, admittedly, a little odd.

// MARK: - Account

IMPLEMENT_GENERIC_TYPE(Account, account)

extern BRGenericAccount
gwmAccountCreate (const char *type,
                  UInt512 seed) {
    return genHandlerLookup(type)->account.create (type, seed);
}

extern BRGenericAccount
gwmAccountCreateWithPublicKey (const char *type,
                               BRKey publicKey) {
    return genHandlerLookup(type)->account.createWithPublicKey (type, publicKey);
}

extern BRGenericAccount
gwmAccountCreateWithSerialization(const char *type,
                                  uint8_t *bytes,
                                  size_t   bytesCount) {
    return genHandlerLookup(type)->account.createWithSerialization (type, bytes, bytesCount);
}

extern void
gwmAccountRelease (BRGenericAccount account) {
    account->handlers.free (account);
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
    return account->handlers.getAddress (account);
}

extern uint8_t *
gwmAccountGetSerialization (BRGenericAccount account, size_t *bytesCount) {
    return account->handlers.getSerialization (account, bytesCount);
}

// MARK: - Address

IMPLEMENT_GENERIC_TYPE(Address, address)

extern char *
gwmAddressAsString (BRGenericNetwork nid,
                    BRGenericAddress aid) {
    return aid->handlers.asString (aid);
}

extern int
gwmAddressEqual (BRGenericNetwork nid,
                 BRGenericAddress aid1,
                 BRGenericAddress aid2) {
    assert (0 == strcmp (aid1->type, aid2->type));
    return aid1->handlers.equal (aid1, aid2);
}

// MARK: - Transfer

IMPLEMENT_GENERIC_TYPE(Transfer, transfer)

extern void
gwmTransferRelease (BRGenericTransfer transfer) {
    transfer->handlers.free (transfer);
}

extern BRGenericAddress
gwmTransferGetSourceAddress (BRGenericTransfer transfer) {
    return transfer->handlers.sourceAddress (transfer);
}

extern BRGenericAddress
gwmTransferGetTargetAddress (BRGenericTransfer transfer) {
    return transfer->handlers.targetAddress (transfer);
}

extern UInt256
gwmTransferGetAmount (BRGenericTransfer transfer) {
    return transfer->handlers.amount (transfer);
}

// TODO: Direction is needed?

extern UInt256
gwmTransferGetFee (BRGenericTransfer transfer) {
    return transfer->handlers.fee (transfer);
}

extern BRGenericFeeBasis
gwmTransferGetFeeBasis (BRGenericTransfer transfer) {
    return transfer->handlers.feeBasis (transfer);
}

extern BRGenericHash
gwmTransferGetHash (BRGenericTransfer transfer) {
    return transfer->handlers.hash (transfer);
}

// MARK: - Wallet

IMPLEMENT_GENERIC_TYPE(Wallet, wallet)

extern BRGenericWallet
gwmWalletCreate (BRGenericAccount account) {
    return genHandlerLookup(account->type)->wallet.create (account);
}

extern void
gwmWalletRelease (BRGenericWallet wallet) {
    wallet->handlers.free (wallet);
}

extern UInt256
gwmWalletGetBalance (BRGenericWallet wallet) {
    return wallet->handlers.balance (wallet);
}

// TODO: Set Balance?  Add transfer/directed-amount?

extern BRGenericAddress
gwmWalletGetAddress (BRGenericWallet wid) {
    return NULL;
}

extern BRGenericFeeBasis
gwmWalletGetDefaultFeeBasis (BRGenericWallet wid) {
    return NULL;
}

extern void
gwmWalletSetDefaultFeeBasis (BRGenericWallet wid,
                             BRGenericFeeBasis bid) {
    return;
}

extern BRGenericTransfer
gwmWalletCreateTransfer (BRGenericWallet wallet,
                         BRGenericAddress target, // TODO: BRGenericAddress - ownership given
                         UInt256 amount) {
    return wallet->handlers.createTransfer (wallet, target, amount);
}

extern UInt256
gwmWalletEstimateTransferFee (BRGenericWallet wid,
                              UInt256 amount,
                              BRGenericFeeBasis feeBasis,
                              int *overflow) {
    return UINT256_ZERO;
}

extern void
gwmWalletSubmitTransfer (BRGenericWallet wid,
                         BRGenericTransfer transfer,
                         UInt512 seed) {
    // Sign and serialize
    BRGenericAccount account = gwmGetAccount(gwm);
    account->handlers.serializeTransfer (account, transfer, seed);

    // Get the raw bytes
    size_t txSize = 0;
    uint8_t * tx = transfer->handlers.getSerialization(transfer, &txSize);
    // Get the hash
    BRGenericHash hash = transfer->handlers.hash(transfer);
    BRGenericClient client = gwmGetClient(gwm);
    client.submitTransaction(client.context, gwm, wid, transfer, tx, txSize, hash, 0);
}

