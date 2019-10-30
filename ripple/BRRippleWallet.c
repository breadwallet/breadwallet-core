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
    return wallet;
}

extern void
rippleWalletFree (BRRippleWallet wallet)
{
    if (wallet) {
        array_free(wallet->transfers);
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
    pthread_mutex_lock (&wallet->lock);
    for (size_t index = 0; index < array_count(wallet->transfers) && false == r; index++) {
        r = rippleTransferEqual (transfer, wallet->transfers[index]);
    }
    pthread_mutex_unlock (&wallet->lock);
    return r;
}

extern int rippleWalletHasTransfer (BRRippleWallet wallet, BRRippleTransfer transfer) {
    return walletHasTransfer (wallet, transfer);
}

extern void rippleWalletAddTransfer(BRRippleWallet wallet, BRRippleTransfer transfer)
{
    assert(wallet);
    assert(transfer);
    pthread_mutex_lock (&wallet->lock);
    if (!walletHasTransfer(wallet, transfer)) {
        array_add(wallet->transfers, transfer);
        // Update the balance
        BRRippleUnitDrops amount = rippleTransferGetAmount(transfer);
        BRRippleAddress accountAddress = rippleAccountGetAddress(wallet->account);
        BRRippleAddress source = rippleTransferGetSource(transfer);
        if (1 == rippleAddressEqual(accountAddress, source)) {
            wallet->balance -= (amount + rippleTransferGetFee (transfer));
        } else {
            wallet->balance += amount;
        }
        rippleAddressFree (source);

        // Now update the account's sequence id
        BRRippleSequence sequence = 0;
        for (size_t index = 0; index < array_count(wallet->transfers); index++)
            if (rippleTransferHasSource (wallet->transfers[index], accountAddress))
                sequence += 1;

        rippleAccountSetSequence (wallet->account, sequence);
        rippleAddressFree (accountAddress);

    }
    pthread_mutex_unlock (&wallet->lock);
    // Now update the balance
}

