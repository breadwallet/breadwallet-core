/**
*/

#include "BRHederaAccount.h"
#include "BRHederaCrypto.h"
#include "BRHederaUtils.h"

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
    free(account);
}

extern void hederaAccountSetAddress (BRHederaAccount account, BRHederaAddress address)
{
    assert(account);
    account->address = address;
}

extern BRKey hederaAccountGetPublicKey (BRHederaAccount account)
{
    assert(account);
    return account->publicKey;
}

extern BRHederaAddress hederaAccountGetAddress (BRHederaAccount account)
{
    assert(account);
    return account->address;
}

extern BRHederaAddress hederaAccountGetPrimaryAddress (BRHederaAccount account)
{
    assert(account);
    return account->address;
}

extern size_t hederaAccountGetAddressString(BRHederaAccount account, char * address, size_t addressLength)
{
    assert(account);

    // Check if the account ID has been set
    if (account->address.account > 0) {
        char accountIDString[128];
        hederaAddressGetString(account->address, accountIDString, sizeof(accountIDString));
        // Copy the address if we have enough room in the buffer
        size_t accountStringLength = strlen(accountIDString);
        // Copy the address if we have a buffer and it is big enough
        // to hold the string and the null terminator
        if (address && addressLength > accountStringLength) {
            strcpy(address, accountIDString);
        }
        return (accountStringLength + 1); // string length plus terminating byte
    } else {
        return 0;
    }
}

extern int // 1 if equal
hederaAddressEqual (BRHederaAddress a1, BRHederaAddress a2) {
    return (a1.shard == a2.shard &&
            a1.realm == a2.shard &&
            a1.account == a2.account) ? 1 : 0;
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
