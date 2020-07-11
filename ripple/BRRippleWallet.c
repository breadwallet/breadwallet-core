//
//  BRRippleWallet.c
//  Core
//
//  Created by Carl Cherry on 5/3/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <stdlib.h>
#include <pthread.h>
#include "BRRippleWallet.h"
#include "support/BRArray.h"
#include "BRRipplePrivateStructs.h"
#include "BRRippleFeeBasis.h"
#include "BRRippleAddress.h"
#include <stdio.h>

#define RIPPLE_WALLET_MINIMUM_IN_XRP            (20)

//
// Wallet
//
struct BRRippleWalletRecord
{
    BRRippleUnitDrops balance; // XRP balance
    BRRippleFeeBasis feeBasis; // Base fee for transactions

    // Ripple account
    BRRippleAccount account;

    BRArrayOf(BRRippleTransfer) transfers;

    pthread_mutex_t lock;
};

extern BRRippleWallet
rippleWalletCreate (BRRippleAccount account)
{
    BRRippleWallet wallet = (BRRippleWallet) calloc (1, sizeof(struct BRRippleWalletRecord));
    array_new(wallet->transfers, 0);
    wallet->account = account;
    wallet->balance = 0;
    wallet->feeBasis = (BRRippleFeeBasis) {
        10, 1
    };

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

        pthread_mutex_init(&wallet->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    return wallet;
}

extern void
rippleWalletFree (BRRippleWallet wallet)
{
    if (wallet) {
        pthread_mutex_lock (&wallet->lock);
        for (size_t index = 0; index < array_count(wallet->transfers); index++)
            rippleTransferFree (wallet->transfers[index]);
        array_free(wallet->transfers);
        pthread_mutex_unlock (&wallet->lock);

        pthread_mutex_destroy (&wallet->lock);
        free(wallet);
    }
}

extern BRRippleAddress
rippleWalletGetSourceAddress (BRRippleWallet wallet)
{
    assert(wallet);
    assert(wallet->account);
    // NOTE - the following call will create a copy of the address
    // so we don't need to here as well
    return rippleAccountGetPrimaryAddress(wallet->account);
}

extern BRRippleAddress
rippleWalletGetTargetAddress (BRRippleWallet wallet)
{
    assert(wallet);
    assert(wallet->account);
    // NOTE - the following call will create a copy of the address
    // so we don't need to here as well
    return rippleAccountGetPrimaryAddress(wallet->account);
}

extern int
rippleWalletHasAddress (BRRippleWallet wallet,
                        BRRippleAddress address) {
    return rippleAccountHasAddress (wallet->account, address);
}

extern BRRippleUnitDrops
rippleWalletGetBalance (BRRippleWallet wallet)
{
    assert(wallet);
    return wallet->balance;
}

extern void
rippleWalletSetBalance (BRRippleWallet wallet, BRRippleUnitDrops balance)
{
    assert(wallet);
    wallet->balance = balance;
}

extern BRRippleUnitDrops
rippleWalletGetBalanceLimit (BRRippleWallet wallet,
                             int asMaximum,
                             int *hasLimit) {
    assert (NULL != hasLimit);
    
    *hasLimit = !asMaximum;
    return (asMaximum ? 0 : RIPPLE_XRP_TO_DROPS (RIPPLE_WALLET_MINIMUM_IN_XRP));
}

extern void rippleWalletSetDefaultFeeBasis (BRRippleWallet wallet, BRRippleFeeBasis feeBasis)
{
    assert(wallet);
    wallet->feeBasis = feeBasis;
}

extern BRRippleFeeBasis rippleWalletGetDefaultFeeBasis (BRRippleWallet wallet)
{
    assert(wallet);
    return wallet->feeBasis;
}

static bool rippleTransferEqual(BRRippleTransfer t1, BRRippleTransfer t2) {
    // Equal means the same transaction id, source, target
    bool result = false;
    BRRippleTransactionHash hash1 = rippleTransferGetTransactionId(t1);
    BRRippleTransactionHash hash2 = rippleTransferGetTransactionId(t2);
    if (memcmp(hash1.bytes, hash2.bytes, sizeof(hash1.bytes)) == 0) {
        // Hash is the same - compare the source
        BRRippleAddress source1 = rippleTransferGetSource(t1);
        BRRippleAddress source2 = rippleTransferGetSource(t2);
        if (1 == rippleAddressEqual(source1, source2)) {
            // OK - compare the target
            BRRippleAddress target1 = rippleTransferGetTarget(t1);
            BRRippleAddress target2 = rippleTransferGetTarget(t2);
            if (1 == rippleAddressEqual(target1, target2)) {
                result = true;
            }
            rippleAddressFree(target1);
            rippleAddressFree(target2);
        }
        rippleAddressFree (source1);
        rippleAddressFree (source2);
    }
    return result;
}

static bool
walletHasTransfer (BRRippleWallet wallet, BRRippleTransfer transfer) {
    bool r = false;
    for (size_t index = 0; index < array_count(wallet->transfers) && false == r; index++) {
        r = rippleTransferEqual (transfer, wallet->transfers[index]);
    }
    return r;
}

extern int rippleWalletHasTransfer (BRRippleWallet wallet, BRRippleTransfer transfer) {
    pthread_mutex_lock (&wallet->lock);
    int result = walletHasTransfer (wallet, transfer);
    pthread_mutex_unlock (&wallet->lock);
    return result;
}

static void rippleWalletUpdateSequence (BRRippleWallet wallet,
                                        OwnershipKept BRRippleAddress accountAddress) {
    // Now update the account's sequence id
    BRRippleSequence sequence = 0;
    for (size_t index = 0; index < array_count(wallet->transfers); index++)
        if (rippleTransferHasSource (wallet->transfers[index], accountAddress))
            sequence += 1;

    rippleAccountSetSequence (wallet->account, sequence);
}

extern void rippleWalletAddTransfer (BRRippleWallet wallet,
                                     OwnershipKept BRRippleTransfer transfer)
{
    assert(wallet);
    assert(transfer);
    pthread_mutex_lock (&wallet->lock);
    if (!walletHasTransfer(wallet, transfer)) {
        // We'll add `transfer` to `wallet->transfers`; since we don't own `transfer` we must copy.
        transfer = rippleTransferClone(transfer);
        array_add(wallet->transfers, transfer);

        // Update the balance
        BRRippleUnitDrops amount = (rippleTransferHasError(transfer)
                                    ? 0
                                    : rippleTransferGetAmount(transfer));
        BRRippleUnitDrops fee    = rippleTransferGetFee(transfer);

        BRRippleAddress accountAddress = rippleAccountGetAddress(wallet->account);
        BRRippleAddress source = rippleTransferGetSource(transfer);
        BRRippleAddress target = rippleTransferGetTarget(transfer);

        int isSource = rippleAccountHasAddress (wallet->account, source);
        int isTarget = rippleAccountHasAddress (wallet->account, target);

        if (isSource && isTarget)
            wallet->balance -= fee;
        else if (isSource)
            wallet->balance -= (amount + fee);
        else if (isTarget)
            wallet->balance += amount;
        else {
            // something is seriously wrong
        }
        rippleAddressFree (source);
        rippleAddressFree (target);

        rippleWalletUpdateSequence(wallet, accountAddress);
        rippleAddressFree (accountAddress);
    }
    pthread_mutex_unlock (&wallet->lock);
    // Now update the balance
}

extern void rippleWalletRemTransfer (BRRippleWallet wallet,
                                     OwnershipKept BRRippleTransfer transfer)
{
    assert(wallet);
    assert(transfer);
    pthread_mutex_lock (&wallet->lock);
    if (walletHasTransfer(wallet, transfer)) {
        for (size_t index = 0; index < array_count(wallet->transfers); index++)
            if (rippleTransferEqual (transfer, wallet->transfers[index])) {
                rippleTransferFree(wallet->transfers[index]);
                array_rm (wallet->transfers, index);
                break;
            }

        // Update the balance
        BRRippleUnitDrops amount = (rippleTransferHasError(transfer)
                                    ? 0
                                    : rippleTransferGetAmount(transfer));

        BRRippleUnitDrops fee    = rippleTransferGetFee(transfer);

        BRRippleAddress accountAddress = rippleAccountGetAddress(wallet->account);
        BRRippleAddress source = rippleTransferGetSource(transfer);
        BRRippleAddress target = rippleTransferGetTarget(transfer);

        int isSource = rippleAccountHasAddress (wallet->account, source);
        int isTarget = rippleAccountHasAddress (wallet->account, target);

        if (isSource && isTarget)
            wallet->balance += fee;
        else if (isSource)
            wallet->balance += (amount + fee);
        else if (isTarget)
            wallet->balance -= amount;
        else {
            // something is seriously wrong
        }
        rippleAddressFree (source);
        rippleAddressFree (target);

        rippleWalletUpdateSequence(wallet, accountAddress);
        rippleAddressFree (accountAddress);
    }
    pthread_mutex_unlock (&wallet->lock);
    // Now update the balance
}
