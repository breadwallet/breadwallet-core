/**
*/

#include "BRHederaWallet.h"

#include <stdlib.h>
#include <assert.h>

struct BRHederaWalletRecord {
    BRHederaAccount account;
    BRHederaUnitTinyBar balance;
};

extern BRHederaWallet
hederaWalletCreate (BRHederaAccount account)
{
    assert(account);
    BRHederaWallet wallet = calloc(1, sizeof(struct BRHederaWalletRecord));
    wallet->account = account;
    return wallet;
}

extern void
hederaWalletFree (BRHederaWallet wallet)
{
    assert(wallet);
    free(wallet);
}

extern BRHederaAddress
hederaWalletGetSourceAddress (BRHederaWallet wallet)
{
    assert(wallet);
    assert(wallet->account);
    // In the Hedera system there is only a single address per account
    // No need to clone the address here since the account code will do that
    return hederaAccountGetPrimaryAddress(wallet->account);
}

extern BRHederaAddress
hederaWalletGetTargetAddress (BRHederaWallet wallet)
{
    assert(wallet);
    assert(wallet->account);
    // In the Hedera system there is only a single address per account
    // No need to clone the address here since the account code will do that
    return hederaAccountGetPrimaryAddress(wallet->account);
}

extern void
hederaWalletSetBalance (BRHederaWallet wallet, BRHederaUnitTinyBar balance)
{
    assert(wallet);
    wallet->balance = balance;
}

extern BRHederaUnitTinyBar
hederaWalletGetBalance (BRHederaWallet wallet)
{
    assert(wallet);
    return (wallet->balance);
}


