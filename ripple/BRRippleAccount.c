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
//#include "BRBase58.h"

#define RIPPLE_ADDRESS_PARAMS  ((BRAddressParams) { BITCOIN_PUBKEY_PREFIX, BITCOIN_SCRIPT_PREFIX, BITCOIN_PRIVKEY_PREFIX, BITCOIN_BECH32_PREFIX })

#define PRIMARY_ADDRESS_BIP44_INDEX 0

#define WORD_LIST_LENGTH 2048

struct BRRippleAccountRecord {
    BRRippleAddress raw; // The 20 byte account id

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

extern char * createRippleAddressString (BRRippleAddress address, int useChecksum)
{
    char *string = calloc (1, 36);

    // The process is this:
    // 1. Prepend the Ripple address indicator (0) to the 20 bytes
    // 2. Do a douple sha265 hash on the bytes
    // 3. Use the first 4 bytes of the hash as checksum and append to the bytes
    uint8_t input[25];
    input[0] = 0; // Ripple address type
    memcpy(&input[1], address.bytes, 20);
    uint8_t hash[32];
    BRSHA256_2(hash, input, 21);
    memcpy(&input[21], hash, 4);

    // Now base58 encode the result
    rippleEncodeBase58(string, 35, input, 25);
    // NOTE: once the following function is "approved" we can switch to using it
    // and remove the base58 function above
    // static const char rippleAlphabet[] = "rpshnaf39wBUDNEGHJKLM4PQRST7VWXYZ2bcdeCg65jkm8oFqi1tuvAxyz";
    // BRBase58EncodeEx(string, 36, input, 25, rippleAlphabet);
    return string;
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
    UInt160 hash = BRKeyHash160(&account->publicKey);
    memcpy(account->raw.bytes, hash.u8, 20);

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
    BRRippleAddress address;
    memcpy(address.bytes, account->raw.bytes, sizeof(address.bytes));
    return(address);
}

extern uint8_t *rippleAccountGetSerialization (BRRippleAccount account, size_t *bytesCount) {
    assert (NULL != bytesCount);
    assert (NULL != account);

    *bytesCount = BRKeyPubKey (&account->publicKey, NULL, 0);
    uint8_t *bytes = calloc (1, *bytesCount);
    BRKeyPubKey(&account->publicKey, bytes, *bytesCount);
    return bytes;
}

extern int rippleAccountGetAddressString(BRRippleAccount account, char * rippleAddress, int length)
{
    // Create the ripple address string
    char *address = createRippleAddressString(account->raw, 1);
    if (address) {
        int addressLength = (int)strlen(address);
        // Copy the address if we have enough room in the buffer
        if (length > addressLength) {
            strcpy(rippleAddress, address);
        }
        free(address);
        return(addressLength + 1); // string length plus terminating byte
    } else {
        return 0;
    }
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
    // Currently there is not any allocated memory inside the account
    // so just delete the account itself
    free(account);
}

extern BRRippleAddress rippleAccountGetPrimaryAddress (BRRippleAccount account)
{
    // Currently we only have the primary address - so just return it
    return account->raw;
}

extern size_t
rippleTransactionSerializeAndSign(BRRippleTransaction transaction, BRKey *privateKey,
                                  BRKey *publicKey, uint32_t sequence, uint32_t lastLedgerSequence);

extern size_t
rippleAccountSignTransaction(BRRippleAccount account, BRRippleTransaction transaction, const char *paperKey)
{
    assert(account);
    assert(transaction);
    assert(paperKey);

    // Create the private key from the paperKey
    UInt512 seed = getSeed(paperKey);
    BRKey key = deriveRippleKeyFromSeed (seed, 0, false);

    size_t tx_size =
        rippleTransactionSerializeAndSign(transaction, &key, &account->publicKey,
                                          account->sequence, account->lastLedgerSequence);

    // Increment the sequence number if we were able to sign the bytes
    if (tx_size > 0) {
        account->sequence++;
    }

    return tx_size;
}

extern int rippleAddressStringToAddress(const char* input, BRRippleAddress *address)
{
    // The ripple address "string" is a base58 encoded string
    // using the ripple base58 alphabet.  After decoding the address
    // the output lookes like this
    // [ 0, 20-bytes, 4-byte checksum ] for a total of 25 bytes
    // In the ripple alphabet "r" maps to 0 which is why the first byte is 0
    assert(address);

    // Decode the
    uint8_t bytes[25];
    //int length = BRBase58DecodeEx(NULL, 0, input, rippleAlphabet);
    int length = rippleDecodeBase58(input, NULL);
    if (length != 25) {
        // Since ripple addresses are created from 20 byte account IDs the
        // needed space to covert it back has to be 25 bytes.
        // LOG message?
        return 0;
    }

    // We got the correct length so go ahead and decode.
    rippleDecodeBase58(input, bytes);
    //BRBase58DecodeEx(bytes, 25, input, rippleAlphabet);

    // We need to do a checksum on all but the last 4 bytes.
    // From trial and error is appears that the checksum is just
    // sha256(sha256(first21bytes))
    uint8_t md32[32];
    BRSHA256_2(md32, &bytes[0], 21);
    // Compare the first 4 bytes of the sha hash to the last 4 bytes
    // of the decoded bytes (i.e. starting a byte 21)
    if (0 == memcmp(md32, &bytes[21], 4)) {
        // The checksum has passed so copy the 20 bytes that form the account id
        // to our ripple address structure.
        // i.e. strip off the 1 bytes token and the 4 byte checksum
        memcpy(address->bytes, &bytes[1], 20);
        return 20;
    } else {
        // Checksum does not match - do we log this somewhere
    }
    return 0;
}

extern BRRippleAddress
rippleAddressCreate(const char * rippleAddressString)
{
    BRRippleAddress address;
    memset(address.bytes, 0x00, sizeof(address.bytes));
    // Work backwards from this ripple address (string) to what is
    // known as the acount ID (20 bytes)
    rippleAddressStringToAddress(rippleAddressString, &address);
    return address;
}

extern int // 1 if equal
rippleAddressEqual (BRRippleAddress a1, BRRippleAddress a2) {
    return 0 == memcmp (a1.bytes, a2.bytes, 20);
}

extern char *
rippleAddressAsString (BRRippleAddress address) {
    return createRippleAddressString(address, 1);
}

