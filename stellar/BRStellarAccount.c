//
//  BRStellarAccount.c
//  Core
//
//  Created by Carl Cherry on 5/21/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "support/BRCrypto.h"
#include "support/BRKey.h"
#include "support/BRBIP32Sequence.h"
#include "support/BRBIP39WordsEn.h"
#include "BRStellar.h"
#include "BRStellarBase.h"
#include "BRStellarAccount.h"
#include "BRStellarSignature.h"
#include "utils/base32.h"
#include "ed25519/ed25519.h"
#include "utils/crc16.h"
#include "BRStellarAccountUtils.h"

struct BRStellarAccountRecord {
    BRStellarAddress address;
    // The public key - needed when sending 
    BRKey publicKey;
};

extern char * createStellarAddressString (BRStellarAddress address, int useChecksum)
{    return NULL;
}

static BRStellarAccount createAccountObject(BRKey * key)
{
    BRStellarAccount account = (BRStellarAccount) calloc (1, sizeof (struct BRStellarAccountRecord));

    unsigned char privateKey[64] = {0};
    unsigned char publicKey[32] = {0};
    ed25519_create_keypair(publicKey, privateKey, key->secret.u8);
    memcpy(&account->publicKey.pubKey[0], &publicKey[0], 32);
    return account;
}

// Create an account from the paper key
extern BRStellarAccount stellarAccountCreate (const char *paperKey)
{
    BRKey key = createStellarKeyFromPaperKey(paperKey);
    return createAccountObject(&key);
}

// Create an account object with the seed
extern BRStellarAccount stellarAccountCreateWithSeed(UInt512 seed)
{
    BRKey key = createStellarKeyFromSeed(seed);
    return createAccountObject(&key);
}

// Create an account object using the key
extern BRStellarAccount stellarAccountCreateWithKey(BRKey key)
{
    return createAccountObject(&key);
}

extern BRStellarAddress stellarAccountGetAddress(BRStellarAccount account)
{
    assert(account);
    return createStellarAddressFromPublicKey(&account->publicKey);
}

extern int stellarAccountGetAddressString(BRStellarAccount account, char * stellarAddress, int length)
{
    BRStellarAddress address = stellarAccountGetAddress(account);
    // The stellar address is alread a string
    if (length >= sizeof(address.bytes)) {
        memcpy(stellarAddress, address.bytes, sizeof(address.bytes));
    }
    return sizeof(address.bytes);
}

extern BRKey stellarAccountGetPublicKey(BRStellarAccount account)
{
    // Before returning this - make sure there is no private key.
    // It should NOT be there but better to make sure.
    account->publicKey.secret = UINT256_ZERO;
    return account->publicKey;
}

extern void stellarAccountFree(BRStellarAccount account)
{
    // Currently there is not any allocated memory inside the account
    // so just delete the account itself
    free(account);
}

extern BRStellarAddress stellarAccountGetPrimaryAddress (BRStellarAccount account)
{
    // Currently we only have the primary address - so just return it
    return stellarAccountGetAddress(account);
}

extern BRStellarSerializedTransaction
stellarTransactionSerializeAndSign(BRStellarTransaction transaction, BRKey *privateKey,
                                  BRKey *publicKey, uint32_t sequence, uint32_t lastLedgerSequence);

extern const BRStellarSerializedTransaction
stellarAccountSignTransaction(BRStellarAccount account, BRStellarTransaction transaction, const char *paperKey)
{
    assert(account);
    assert(transaction);
    assert(paperKey);

    return NULL;
}

extern int stellarAddressStringToAddress(const char* input, BRStellarAddress *address)
{
    return 0;
}

extern BRStellarAddress
stellarAddressCreate(const char * stellarAddressString)
{
    BRStellarAddress address;
    memset(address.bytes, 0x00, sizeof(address.bytes));
    // Work backwards from this stellar address (string) to what is
    // known as the acount ID (20 bytes)
    stellarAddressStringToAddress(stellarAddressString, &address);
    return address;
}

extern int // 1 if equal
stellarAddressEqual (BRStellarAddress a1, BRStellarAddress a2) {
    return 0 == memcmp (a1.bytes, a2.bytes, 20);
}
