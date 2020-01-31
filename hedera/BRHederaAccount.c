/**
*/

#include "BRHederaAccount.h"
#include "BRHederaCrypto.h"

#include <stdlib.h>
#include <assert.h>

// Our ed25519 implementation is simple and only support the raw
// 64-byte private key and 32-byte public key. We have no need to
// convert them to any other format internally.
#define HEDERA_PRIVATE_KEY_SIZE 64
#define HEDERA_PUBLIC_KEY_SIZE 32

struct BRHederaAccountRecord {
    BRHederaAddress address;
    uint8_t publicKey[HEDERA_PUBLIC_KEY_SIZE];
};

extern BRHederaAccount hederaAccountCreateWithSeed (UInt512 seed)
{
    BRHederaAccount account = calloc(1, sizeof(struct BRHederaAccountRecord));

    // Generate the secret from the seed
    BRKey privateKey = hederaKeyCreate(seed);

    // From the secret get the public key
    hederaKeyGetPublicKey(privateKey, account->publicKey);

    // We cannot generate an address from the public key - but
    // when running the demo app we need to put in some address
    // account->address = hederaAddressCreateFromString("0.0.3229");

    return account;
}

extern BRHederaAccount
hederaAccountCreateWithSerialization (uint8_t *bytes, size_t bytesCount)
{
    assert(bytesCount == HEDERA_PUBLIC_KEY_SIZE + HEDERA_ADDRESS_SERIALIZED_SIZE);
    BRHederaAccount account = calloc(1, sizeof(struct BRHederaAccountRecord));

    // The first 24 bytes will be the account address
    uint8_t addressBytes[HEDERA_ADDRESS_SERIALIZED_SIZE];
    memcpy(addressBytes, bytes, HEDERA_ADDRESS_SERIALIZED_SIZE);
    int64_t shard, realm, accountNum;
    memcpy(&shard, addressBytes, 8);
    memcpy(&realm, addressBytes + 8, 8);
    memcpy(&accountNum, addressBytes + 16, 8);
    account->address = hederaAddressCreate(ntohll(shard), ntohll(realm), ntohll(accountNum));

    // Now copy the public key
    memcpy(account->publicKey, bytes + HEDERA_ADDRESS_SERIALIZED_SIZE, HEDERA_PUBLIC_KEY_SIZE);

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
    // TODO - we need to extend the BRKey to support other key types or ???
    BRKey key;
    memset(&key, 0x00, sizeof(BRKey));
    memcpy(key.pubKey, account->publicKey, HEDERA_PUBLIC_KEY_SIZE);
    return key;
}

extern BRHederaAddress hederaAccountGetAddress (BRHederaAccount account)
{
    assert(account);
    if (account->address) {
        return hederaAddressClone (account->address);
    } else {
        return NULL;
    }
}

extern BRHederaAddress hederaAccountGetPrimaryAddress (BRHederaAccount account)
{
    return hederaAccountGetAddress(account);
}

extern uint8_t *hederaAccountGetSerialization (BRHederaAccount account, size_t *bytesCount) {
    assert (NULL != bytesCount);
    assert (NULL != account);

    // Get the sizes of what we are storing and allocate storage
    uint8_t addressBuffer[HEDERA_ADDRESS_SERIALIZED_SIZE];
    hederaAddressSerialize (account->address, addressBuffer, HEDERA_ADDRESS_SERIALIZED_SIZE);
    *bytesCount = HEDERA_ADDRESS_SERIALIZED_SIZE + HEDERA_PUBLIC_KEY_SIZE;
    uint8_t *bytes = calloc (1, *bytesCount);

    // Copy the serialized address
    memcpy(bytes, addressBuffer, HEDERA_ADDRESS_SERIALIZED_SIZE);

    // Copy the public key
    memcpy(bytes + HEDERA_ADDRESS_SERIALIZED_SIZE, account->publicKey, HEDERA_PUBLIC_KEY_SIZE);

    return bytes;
}

extern int
hederaAccountHasAddress (BRHederaAccount account,
                         BRHederaAddress address) {
    assert(account);
    assert(address);
    assert(account->address);
    return hederaAddressEqual (account->address, address);
}
