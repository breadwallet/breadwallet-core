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

    // TOTO - Hard code the address for now - until someone calls
    // the hederaAccountSetAddress function
    account->address = hederaAddressCreateFromString("0.0.16395");

    return account;
}

extern BRHederaAccount
hederaAccountCreateWithSerialization (uint8_t *bytes, size_t bytesCount)
{
    // For Hedera this should be 24 bytes for the address and 32 for the public key
    assert(bytes);
    assert(bytesCount == 56);
    BRHederaAccount account = calloc(1, sizeof(struct BRHederaAccountRecord));
    account->address = hederaAddressCreateFromBytes(bytes, 24);
    memcpy(account->publicKey.pubKey, bytes + 24, 32);
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

    size_t pubKeySize = 32; // ed25519 public key

    // Serialize the account address and find out the size
    size_t sizeOfAddress = 0;
    uint8_t * address = hederaAddressSerialize(account->address, &sizeOfAddress);

    // Create the buffer to hold the full account serialized bytes
    *bytesCount = pubKeySize + sizeOfAddress;
    uint8_t * bytes = calloc(1, *bytesCount);

    // Copy the address
    memcpy(bytes, address, sizeOfAddress);
    free(address);

    // Hedera public key are ed25519 - always 32 bytes
    memcpy(bytes + sizeOfAddress, account->publicKey.pubKey, 32);
    return bytes;
}

extern int
hederaAccountHasAddress (BRHederaAccount account,
                         BRHederaAddress address) {
    return hederaAddressEqual (account->address, address);
}
