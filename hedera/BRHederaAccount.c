/**
*/

#include "BRHederaAccount.h"
#include "BRHederaCrypto.h"

#include <stdlib.h>
#include <assert.h>

struct BRHederaAccountRecord {
    BRHederaAddress address;
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
    if (account->address) hederaAddressFree (account->address);
    free(account);
}

extern void hederaAccountSetAddress (BRHederaAccount account, BRHederaAddress address)
{
    assert(account);
    account->address = hederaAddressClone (address);
}

extern BRKey hederaAccountGetPublicKey (BRHederaAccount account)
{
    assert(account);
    return account->publicKey;
}

extern BRHederaAddress hederaAccountGetAddress (BRHederaAccount account)
{
    assert(account);
    return hederaAddressClone (account->address);
}

extern BRHederaAddress hederaAccountGetPrimaryAddress (BRHederaAccount account)
{
    assert(account);
    return hederaAddressClone (account->address);
}

extern uint8_t *hederaAccountGetSerialization (BRHederaAccount account, size_t *bytesCount) {
    assert (NULL != bytesCount);
    assert (NULL != account);

    *bytesCount = BRKeyPubKey (&account->publicKey, NULL, 0);
    uint8_t *bytes = calloc (1, *bytesCount);
    // TODO - since we cannot generate the account string from the public key
    // who stores it?
    BRKeyPubKey(&account->publicKey, bytes, *bytesCount);
    return bytes;
}
