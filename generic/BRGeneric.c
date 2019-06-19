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

extern BRGenericAccount
gwmAccountCreate (const char *type,
                  UInt512 seed) {
    BRGenericAccountWithType account = calloc (1, sizeof (struct BRGenericAccountWithTypeRecord));

    BRGenericHandlers handlers = genericHandlerLookup(type);
    assert (NULL != handlers);

    account->handlers = handlers;
    account->base = handlers->account.create (type, seed);

    return account;
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

// MARK: - Wallet Manager

struct BRGenericWalletManagerRecord {
    BRGenericHandlers handlers;
    BRGenericAccount account;
};

extern BRGenericWalletManager
gwmCreate (const char *type,
           BRGenericAccount account) {
    BRGenericWalletManager gwm = calloc (1, sizeof (struct BRGenericWalletManagerRecord));

    gwm->handlers = genericHandlerLookup (type);
    assert (NULL != gwm->handlers);

    gwm->account = account;
    return gwm;
}

extern void
gwmRelease (BRGenericWalletManager gwm) {
    free (gwm);
}

extern BRGenericAddress
gwmGetAccountAddress (BRGenericWalletManager gwm) {
    return gwmAccountGetAddress (gwm->account);
}

extern BRGenericWallet
gwmCreatePrimaryWallet (BRGenericWalletManager gwm) {
    return gwmWalletCreate(gwm);
}

// MARK: - Address

extern char *
gwmAddressAsString (BRGenericWalletManager gwm,
                    BRGenericAddress aid) {
    return gwm->handlers->address.asString (aid);
}

extern int
gwmAddressEqual (BRGenericWalletManager gwm,
                 BRGenericAddress aid1,
                 BRGenericAddress aid2) {
    return gwm->handlers->address.equal (aid1, aid2);
}

// MARK: - Transfer

extern BRGenericAddress
gwmTransferGetSourceAddress (BRGenericWalletManager gwm,
                             BRGenericTransfer tid) {
    return gwm->handlers->transfer.sourceAddress (tid);
}

extern BRGenericAddress
gwmTransferGetTargetAddress (BRGenericWalletManager gwm,
                             BRGenericTransfer tid) {
    return gwm->handlers->transfer.targetAddress (tid);
}

extern UInt256
gwmTransferGetAmount (BRGenericWalletManager gwm,
                      BRGenericTransfer tid) {
    return gwm->handlers->transfer.amount (tid);
}

extern UInt256
gwmTransferGetFee (BRGenericWalletManager gwm,
                   BRGenericTransfer tid) {
    return gwm->handlers->transfer.fee (tid);
}

extern BRGenericFeeBasis
gwmTransferGetFeeBasis (BRGenericWalletManager gwm,
                        BRGenericTransfer tid) {
    return gwm->handlers->transfer.feeBasis (tid);
}

extern BRGenericHash
gwmTransferGetHash (BRGenericWalletManager gwm,
                    BRGenericTransfer tid) {
    return gwm->handlers->transfer.hash (tid);
}

// MARK: - Wallet

extern BRGenericWallet
gwmWalletCreate (BRGenericWalletManager gwm) {
    return gwm->handlers->wallet.create (gwm->account);
}

extern UInt256
gwmWalletGetBalance (BRGenericWalletManager gwm,
                     BRGenericWallet wallet) {
    return gwm->handlers->wallet.balance (wallet);
}

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
                         BRGenericAddress target,
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
