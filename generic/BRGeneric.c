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

#include "BRGeneric.h"
#include "BRGenericHandlers.h"

// This is, admittedly, a little odd.

static BRGenericAccount
gwmAccountCreateInternal (BRGenericHandlers handlers, void *base) {

    BRGenericAccountWithType account = calloc (1, sizeof (struct BRGenericAccountWithTypeRecord));

    account->handlers = handlers;
    account->base = base;

    return account;
}

extern BRGenericAccount
gwmAccountCreate (const char *type,
                  UInt512 seed) {
    BRGenericHandlers handlers = genericHandlerLookup(type);
    assert (NULL != handlers);

    return gwmAccountCreateInternal (handlers, handlers->account.create (type, seed));
}

extern BRGenericAccount
gwmAccountCreateWithPublicKey (const char *type,
                               BRKey publicKey) {
    BRGenericHandlers handlers = genericHandlerLookup(type);
    assert (NULL != handlers);

    return gwmAccountCreateInternal (handlers, handlers->account.createWithPublicKey (type, publicKey));
}

extern BRGenericAccount
gwmAccountCreateWithSerialization(const char *type,
                                  uint8_t *bytes,
                                  size_t   bytesCount) {
    BRGenericHandlers handlers = genericHandlerLookup(type);
    assert (NULL != handlers);

    return gwmAccountCreateInternal (handlers, handlers->account.createWithSerialization (type, bytes, bytesCount));
}

extern void
gwmAccountRelease (BRGenericAccount account) {
    BRGenericAccountWithType accountWithType = account;
    accountWithType->handlers->account.free (accountWithType->base);
    free (accountWithType);
}

extern const char *
gwmAccountGetType (BRGenericAccount account) {
    BRGenericAccountWithType accountWithType = account;
    return accountWithType->handlers->type;
}

extern int
gwmAccountHasType (BRGenericAccount account,
                   const char *type) {
    BRGenericAccountWithType accountWithType = account;
    return 0 == strcmp (type, accountWithType->handlers->type);
}

extern BRGenericAddress
gwmAccountGetAddress (BRGenericAccount account) {
    BRGenericAccountWithType accountWithType = account;
    return accountWithType->handlers->account.getAddress (accountWithType->base);
}

extern uint8_t *
gwmAccountGetSerialization (BRGenericAccount account, size_t *bytesCount) {
    assert (NULL != bytesCount);

    BRGenericAccountWithType accountWithType = account;
    return accountWithType->handlers->account.getSerialization(accountWithType->base, bytesCount);
}

// MARK: - Address

extern char *
gwmAddressAsString (BRGenericWalletManager gwm,
                    BRGenericAddress aid) {
    return gwmGetHandlers(gwm)->address.asString (aid);
}

extern int
gwmAddressEqual (BRGenericWalletManager gwm,
                 BRGenericAddress aid1,
                 BRGenericAddress aid2) {
    return gwmGetHandlers(gwm)->address.equal (aid1, aid2);
}

// MARK: - Transfer

extern BRGenericAddress
gwmTransferGetSourceAddress (BRGenericWalletManager gwm,
                             BRGenericTransfer tid) {
    return gwmGetHandlers(gwm)->transfer.sourceAddress (tid);
}

extern BRGenericAddress
gwmTransferGetTargetAddress (BRGenericWalletManager gwm,
                             BRGenericTransfer tid) {
    return gwmGetHandlers(gwm)->transfer.targetAddress (tid);
}

extern UInt256
gwmTransferGetAmount (BRGenericWalletManager gwm,
                      BRGenericTransfer tid) {
    return gwmGetHandlers(gwm)->transfer.amount (tid);
}

// TODO: Direction is needed?

extern UInt256
gwmTransferGetFee (BRGenericWalletManager gwm,
                   BRGenericTransfer tid) {
    return gwmGetHandlers(gwm)->transfer.fee (tid);
}

extern BRGenericFeeBasis
gwmTransferGetFeeBasis (BRGenericWalletManager gwm,
                        BRGenericTransfer tid) {
    return gwmGetHandlers(gwm)->transfer.feeBasis (tid);
}

extern BRGenericHash
gwmTransferGetHash (BRGenericWalletManager gwm,
                    BRGenericTransfer tid) {
    return gwmGetHandlers(gwm)->transfer.hash (tid);
}

// MARK: - Wallet

extern BRGenericWallet
gwmWalletCreate (BRGenericWalletManager gwm) {
    return gwmGetHandlers(gwm)->wallet.create (gwmGetAccount(gwm));
}

extern UInt256
gwmWalletGetBalance (BRGenericWalletManager gwm,
                     BRGenericWallet wallet) {
    return gwmGetHandlers(gwm)->wallet.balance (wallet);
}

// TODO: Set Balance?  Add transfer/directed-amount?

extern BRGenericAddress
gwmWalletGetAddress (BRGenericWalletManager gwm,
                     BRGenericWallet wid) {
    return NULL;
}

extern BRGenericFeeBasis
gwmWalletGetDefaultFeeBasis (BRGenericWalletManager gwm,
                             BRGenericWallet wid) {
    return NULL;
}

extern void
gwmWalletSetDefaultFeeBasis (BRGenericWalletManager gwm,
                             BRGenericWallet wid,
                             BRGenericFeeBasis bid) {
    return;
}

extern BRGenericTransfer
gwmWalletCreateTransfer (BRGenericWalletManager gwm,
                         BRGenericWallet wid,
                         BRGenericAddress target, // TODO: BRGenericAddress - ownership given
                         UInt256 amount) {
    return NULL;
}

extern UInt256
gwmWalletEstimateTransferFee (BRGenericWalletManager gwm,
                              BRGenericWallet wid,
                              UInt256 amount,
                              BRGenericFeeBasis feeBasis,
                              int *overflow) {
    return UINT256_ZERO;
}

extern void
gwmWalletSubmitTransfer (BRGenericWalletManager gwm,
                         BRGenericWallet wid,
                         BRGenericTransfer tid,
                         UInt512 seed) {
    return;
}

