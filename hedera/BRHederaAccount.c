/**
*/

#include "BRHederaAccount.h"
#include "BRHederaCrypto.h"

#include <stdlib.h>
#include <assert.h>

struct BRHederaAccountRecord {
    BRHederaAccountID accountID;
    BRKey publicKey;
};

extern BRHederaAccount hederaAccountCreateWithSeed (UInt512 seed)
{
    BRHederaAccount account = calloc(1, sizeof(struct BRHederaAccountRecord));

    // Generate the secret from the seed
    BRKey privateKey = hederaKeyCreate(seed);

    // From the secret get the public key
    account->publicKey = hederaKeyGetPublicKey(privateKey);

    return account;
}

extern void hederaAccountFree (BRHederaAccount account)
{
    assert(account);
    free(account);
}

extern void hederaAccountSetAccountID (BRHederaAccount account, BRHederaAccountID accountID)
{
    assert(account);
    account->accountID = accountID;
}

extern BRKey hederaAccountGetPublicKey (BRHederaAccount account)
{
    assert(account);
    return account->publicKey;
}

extern BRHederaAccountID hederaAccountGetAccountID (BRHederaAccount account)
{
    assert(account);
    return account->accountID;
}
