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
#include "utils/base32.h"
#include "vendor/ed25519/ed25519.h"
#include "utils/crc16.h"
#include "BRStellarAccountUtils.h"
#include "BRStellarSerialize.h"

struct BRStellarAccountRecord {
    BRStellarAddress address;
    BRStellarAccountID accountID;

    // The public key - needed when sending 
    BRKey publicKey;

    uint64_t sequence;
    BRStellarNetworkType networkType;
};

extern void stellarAccountSetNetworkType(BRStellarAccount account, BRStellarNetworkType networkType)
{
    // The network type is required when we sign the transaction. The string included in the
    // data to hash must match the network we connect to
    account->networkType = networkType;
}

static BRStellarAccount createAccountObject(BRKey * key)
{
    // Create an initialize a BRStellarAccountRecord object
    BRStellarAccount account = (BRStellarAccount) calloc (1, sizeof (struct BRStellarAccountRecord));

    // Generate the public key from the secret
    unsigned char privateKey[64] = {0};
    unsigned char publicKey[32] = {0};
    ed25519_create_keypair(publicKey, privateKey, key->secret.u8);
    var_clean(&privateKey); // never leave the private key in memory
    var_clean(&key);
    memcpy(&account->publicKey.pubKey[0], &publicKey[0], 32);
    account->networkType = STELLAR_NETWORK_PUBLIC;
    account->accountID.accountType = PUBLIC_KEY_TYPE_ED25519;
    memcpy(account->accountID.accountID, publicKey, 32);
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
    // NOTE: since this is a public function that passes in a copy
    // of the key/secret it is up to the caller to wipe the secret from memory
    return createAccountObject(&key);
}

extern BRStellarAddress stellarAccountGetAddress(BRStellarAccount account)
{
    assert(account);
    // The account object should already have a public key - so generate the
    // stellar address from the public key.
    return createStellarAddressFromPublicKey(&account->publicKey);
}

extern BRStellarAccountID stellarAccountGetAccountID(BRStellarAccount account)
{
    assert(account);
    return account->accountID;
}

extern BRKey stellarAccountGetPublicKey(BRStellarAccount account)
{
    // The accounts BRKey object should NEVER have the secret but zero it out just in case
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

// Private function implemented in BRStellarTransaction.c
extern BRStellarSerializedTransaction
stellarTransactionSerializeAndSign(BRStellarTransaction transaction, uint8_t *privateKey,
                                   uint8_t *publicKey, uint64_t sequence, BRStellarNetworkType networkType);

extern const BRStellarSerializedTransaction
stellarAccountSignTransaction(BRStellarAccount account, BRStellarTransaction transaction, const char *paperKey)
{
    assert(account);
    assert(transaction);
    assert(paperKey);

    BRKey key = createStellarKeyFromPaperKey(paperKey);
    unsigned char privateKey[64] = {0};
    unsigned char publicKey[32] = {0};
    ed25519_create_keypair(publicKey, privateKey, key.secret.u8);

    // Send it off to the transaction code to serialize and sign since we don't know
    // the internal details of a transaction
    BRStellarSerializedTransaction s =  stellarTransactionSerializeAndSign(transaction, privateKey, publicKey, account->sequence, account->networkType);

    return s;
}

extern int // 1 if equal
stellarAddressEqual (BRStellarAddress a1, BRStellarAddress a2) {
    return 0 == memcmp (a1.bytes, a2.bytes, 20);
}

extern void stellarAccountSetSequence(BRStellarAccount account, uint64_t sequence)
{
    assert(account);
    // The sequence is very important as it must be 1 greater than the previous
    // transaction sequence.
    account->sequence = sequence;
}

extern BRStellarAccountID stellerAccountCreateStellarAccountID(const char * stellarAddress)
{
    BRStellarAccountID accountID = createStellarAccountIDFromStellarAddress(stellarAddress);
    return accountID;
}
