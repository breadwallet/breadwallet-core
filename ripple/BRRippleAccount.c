//
//  BRRippleAccount.c
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "support/BRCrypto.h"
#include "support/BRKey.h"
#include "support/BRBIP32Sequence.h"
#include "support/BRBIP39WordsEn.h"
#include "ethereum/util/BRUtilHex.h"
#include "BRRipple.h"
#include "BRRippleBase.h"
#include "BRRippleAccount.h"
#include "BRRippleSignature.h"
#include "BRRippleBase58.h"
#include "BRRippleAddress.h"

//#include "BRBase58.h"

#define RIPPLE_ADDRESS_PARAMS  ((BRAddressParams) { BITCOIN_PUBKEY_PREFIX, BITCOIN_SCRIPT_PREFIX, BITCOIN_PRIVKEY_PREFIX, BITCOIN_BECH32_PREFIX })

#define PRIMARY_ADDRESS_BIP44_INDEX 0

#define WORD_LIST_LENGTH 2048

struct BRRippleAccountRecord {
    BRRippleAddress address; // The 20 byte account id

    // The public key - needed when sending 
    BRKey publicKey;  // BIP44: 'Master Public Key 'M' (264 bits) - 8

    uint32_t index;     // The BIP-44 Index used for this key.
    
    BRRippleSequence sequence;   // The NEXT valid sequence number, must be exactly 1 greater
                                 // than the last transaction sent

    BRRippleLastLedgerSequence lastLedgerSequence; // (Optional; strongly recommended) Highest ledger
                                 // index this transaction
                                 // can appear in. Specifying this field places a strict upper limit on
                                 // how long the transaction can wait to be validated or rejected.
                                 // See Reliable Transaction Submission for more details.
};

extern UInt512 getSeed(const char *paperKey)
{
    // Generate the 512bit private key using a BIP39 paperKey
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, paperKey, NULL); // no passphrase
    return seed;
}

static BRKey deriveRippleKeyFromSeed (UInt512 seed, uint32_t index, bool cleanPrivateKey)
{
    BRKey key;
    mem_clean(&key, sizeof(BRKey));
    
    // The BIP32 privateKey for m/44'/60'/0'/0/index
    BRBIP32PrivKeyPath(&key, &seed, sizeof(UInt512), 5,
                       44 | BIP32_HARD,          // purpose  : BIP-44
                       144 | BIP32_HARD,        // coin_type: Ripple
                       0 | BIP32_HARD,          // account  : <n/a>
                       0,                        // change   : not change
                       index);                   // index    :

    // Generate the compressed public key
    key.compressed = 1;
    BRKeyPubKey(&key, &key.pubKey, 33);
    if (cleanPrivateKey) {
        // In some cases we don't want to wipe the secret: i.e. during signing
        mem_clean(&key.secret, sizeof(key.secret));
    }

    return key;
}

static BRRippleAccount createAccountObject(BRKey * key)
{
    if (0 == key->compressed) {
        // The expectation is that this key contains the compressed public key
        return NULL;
    }

    BRRippleAccount account = (BRRippleAccount) calloc (1, sizeof (struct BRRippleAccountRecord));

    // Take a copy of the key since we are changing at least once property
    account->publicKey = *key;

    // Before returning the account - wipe out the secret just in case someone forgot
    mem_clean(&account->publicKey.secret, sizeof(account->publicKey.secret));

    // Create the raw bytes for the 20 byte account id
    account->address = rippleAddressCreateFromKey(&account->publicKey);

    return account;
}

// Create an account from the paper key
extern BRRippleAccount rippleAccountCreate (const char *paperKey)
{
    UInt512 seed = getSeed(paperKey);
    BRKey key = deriveRippleKeyFromSeed (seed, 0, true);
    return createAccountObject(&key);
}

// Create an account object with the seed
extern BRRippleAccount rippleAccountCreateWithSeed(UInt512 seed)
{
    BRKey key = deriveRippleKeyFromSeed (seed, 0, true);
    return createAccountObject(&key);
}

// Create an account object using the key
extern BRRippleAccount rippleAccountCreateWithKey(BRKey key)
{
    return createAccountObject(&key);
}

extern BRRippleAccount rippleAccountCreateWithSerialization (uint8_t *bytes, size_t bytesCount) {
    if (!bytes || bytesCount != 33) {
        // We only support a compressed public key
        return NULL;
    }
    BRKey key;
    BRKeySetPubKey(&key, bytes, bytesCount);
    if (0 == key.compressed) {
        return NULL;
    }
    return createAccountObject(&key);
}

extern void rippleAccountSetSequence(BRRippleAccount account, BRRippleSequence sequence)
{
    assert(account);
    account->sequence = sequence;
}

extern void rippleAccountSetLastLedgerSequence(BRRippleAccount account,
                                               BRRippleLastLedgerSequence lastLedgerSequence)
{
    assert(account);
    account->lastLedgerSequence = lastLedgerSequence;
}

extern BRRippleAddress rippleAccountGetAddress(BRRippleAccount account)
{
    assert(account);
    return rippleAddressClone (account->address);
}

extern int
rippleAccountHasAddress (BRRippleAccount account,
                         BRRippleAddress address) {
    return rippleAddressEqual (account->address, address);
}

extern uint8_t *rippleAccountGetSerialization (BRRippleAccount account, size_t *bytesCount) {
    assert (NULL != bytesCount);
    assert (NULL != account);

    *bytesCount = BRKeyPubKey (&account->publicKey, NULL, 0);
    uint8_t *bytes = calloc (1, *bytesCount);
    BRKeyPubKey(&account->publicKey, bytes, *bytesCount);
    return bytes;
}

extern BRKey rippleAccountGetPublicKey(BRRippleAccount account)
{
    // Before returning this - make sure there is no private key.
    // It should NOT be there but better to make sure.
    account->publicKey.secret = UINT256_ZERO;
    return account->publicKey;
}

extern void rippleAccountFree(BRRippleAccount account)
{
    assert(account);
    if (account->address) rippleAddressFree(account->address);
    free(account);
}

extern BRRippleAddress rippleAccountGetPrimaryAddress (BRRippleAccount account)
{
    // Currently we only have the primary address - so just return it
    assert(account);
    return rippleAddressClone (account->address);
}

extern size_t
rippleTransactionSerializeAndSign(BRRippleTransaction transaction, BRKey *privateKey,
                                  BRKey *publicKey, uint32_t sequence, uint32_t lastLedgerSequence);

extern size_t
rippleAccountSignTransaction(BRRippleAccount account, BRRippleTransaction transaction, UInt512 seed)
{
    assert(account);
    assert(transaction);

    // Create the private key from the paperKey
    BRKey key = deriveRippleKeyFromSeed (seed, 0, false);

    size_t tx_size =
        rippleTransactionSerializeAndSign(transaction, &key, &account->publicKey,
                                          account->sequence + 1,  // next sequence number
                                          account->lastLedgerSequence);

    // Increment the sequence number if we were able to sign the bytes
    if (tx_size > 0) {
        account->sequence++;
    }

    return tx_size;
}
