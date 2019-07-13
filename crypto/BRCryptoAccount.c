//
//  BRCryptoAccount.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include "BRCryptoAccount.h"
#include "BRCryptoPrivate.h"

#include "support/BRBIP32Sequence.h"
#include "support/BRBIP39Mnemonic.h"
#include "support/BRKey.h"
#include "ethereum/BREthereum.h"

static uint16_t
checksumFletcher16 (const uint8_t *data, size_t count);

#define ACCOUNT_SERIALIZE_DEFAULT_VERSION  1

static void
cryptoAccountRelease (BRCryptoAccount account);

struct BRCryptoAccountRecord {
    BRMasterPubKey btc;
    BREthereumAccount eth;

    uint64_t timestamp;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoAccount, cryptoAccount);

static UInt512
cryptoAccountDeriveSeedInternal (const char *phrase) {
    UInt512 seed;
    BRBIP39DeriveKey (seed.u8, phrase, NULL);
    return seed;
}

extern UInt512
cryptoAccountDeriveSeed (const char *phrase) {
    return cryptoAccountDeriveSeedInternal(phrase);
}

extern char *
cryptoAccountGeneratePaperKey (const char *words[]) {
    UInt128 entropy;
    arc4random_buf (entropy.u8, sizeof (entropy));

    size_t phraseLen = BRBIP39Encode (NULL, 0, words, entropy.u8, sizeof(entropy));
    char  *phrase    = calloc (phraseLen, 1);

    assert (phraseLen == BRBIP39Encode (phrase, phraseLen, words, entropy.u8, sizeof(entropy)));

    return phrase;
}

extern BRCryptoBoolean
cryptoAccountValidateWordsList (int wordsCount) {
    return AS_CRYPTO_BOOLEAN (wordsCount == BIP39_WORDLIST_COUNT);
}

static BRCryptoAccount
cryptoAccountCreateInternal (BRMasterPubKey btc,
                             BRKey eth,
                             uint64_t timestamp) {
    BRCryptoAccount account = malloc (sizeof (struct BRCryptoAccountRecord));

    account->btc = btc;
    account->eth = createAccountWithPublicKey(eth);
    account->timestamp = timestamp;
    account->ref = CRYPTO_REF_ASSIGN(cryptoAccountRelease);

    return account;

}
static BRCryptoAccount
cryptoAccountCreateFromSeedInternal (UInt512 seed,
                                     uint64_t timestamp) {
    BRCryptoAccount account = malloc (sizeof (struct BRCryptoAccountRecord));

    account->btc = BRBIP32MasterPubKey (seed.u8, sizeof (seed.u8));
    account->eth = createAccountWithBIP32Seed(seed);
    account->timestamp = timestamp;
    account->ref = CRYPTO_REF_ASSIGN(cryptoAccountRelease);

    return account;
}

extern BRCryptoAccount
cryptoAccountCreate (const char *phrase, uint64_t timestamp) {
    return cryptoAccountCreateFromSeedInternal (cryptoAccountDeriveSeedInternal(phrase), timestamp);
}


/**
 * Deserialize into an Account.  The serialization format is:
 *  <checksum16><size32><version><BTC size><BTC master public key><ETH size><ETH public key>
 *
 *
 * @param bytes the serialized bytes
 * @param bytesCount the number of serialized bytes
 *
 * @return An Account, or NULL.
 */
extern BRCryptoAccount
cryptoAccountCreateFromSerialization (const uint8_t *bytes, size_t bytesCount) {
    uint8_t *bytesPtr = (uint8_t *) bytes;
    uint8_t *bytesEnd = bytesPtr + bytesCount;

#define BYTES_PTR_INCR_AND_CHECK(size) do {\
bytesPtr += (size);\
if (bytesPtr > bytesEnd) return NULL; /* overkill */ \
} while (0)

    size_t chkSize = sizeof (uint16_t); // checksum
    size_t szSize  = sizeof (uint32_t); // size
    size_t verSize = sizeof (uint16_t); // version
    size_t tsSize  = sizeof (uint64_t); // timestamp

    // Demand at least <checksum16><size32> in `bytes`
    if (bytesCount < (chkSize + szSize)) return NULL;

    // Checksum
    uint16_t checksum = UInt16GetBE(bytesPtr);
    bytesPtr += chkSize;

    // Confirm checksum, otherwise done
    if (checksum != checksumFletcher16 (&bytes[chkSize], (bytesCount - chkSize))) return NULL;

    // Size
    uint32_t size = UInt32GetBE(bytesPtr);
    bytesPtr += szSize;

    if (size != bytesCount) return NULL;

    // Version
    uint16_t version = UInt16GetBE (bytesPtr);
    BYTES_PTR_INCR_AND_CHECK(verSize);

    // Require the current verion, otherwise done.  Will force account create using
    // `cryptoAccountCreate()` and a re-serialization
    if (ACCOUNT_SERIALIZE_DEFAULT_VERSION != version) return NULL;

    // Timestamp
    uint64_t timestamp = UInt64GetBE (bytesPtr);
    BYTES_PTR_INCR_AND_CHECK (tsSize);

    // BTC
    size_t mpkSize = UInt32GetBE(bytesPtr);
    BYTES_PTR_INCR_AND_CHECK (szSize);

    // There is a slight chance that this fails IF AND ONLY IF the serialized format
    // of a MasterPublicKey either changes or is key dependent.  That is, we parse the MPK
    // from `bytes` but if THIS PARSE needs more than the original parse we might run
    // off the end of the provided `bytes`.  Must be REALLY UNLIKELY.
    //
    // TODO: Add `bytesCount` to BRBIP32ParseMasterPubKey()
    BRMasterPubKey mpk = BRBIP32ParseMasterPubKey ((const char *) bytesPtr);
    if (mpkSize != BRBIP32SerializeMasterPubKey (NULL, 0, mpk)) return NULL;

    // ETH
    size_t ethSize = UInt32GetBE (bytesPtr);
    BYTES_PTR_INCR_AND_CHECK (szSize);
    assert (65 == ethSize);

    BRKey ethPublicKey;
    BRKeySetPubKey(&ethPublicKey, bytesPtr, 65);
    BYTES_PTR_INCR_AND_CHECK (65);

    return cryptoAccountCreateInternal (mpk, ethPublicKey, timestamp);
#undef BYTES_PTR_INCR_AND_CHECK
}

static void
cryptoAccountRelease (BRCryptoAccount account) {
    accountFree(account->eth);  // Core holds???
//    printf ("Account: Release\n");
    free (account);
}


/**
 * Serialize the account as per ACCOUNT_SERIALIZE_DEFAULT_VERSION.  The serialization format is:
 *  <checksum16><size32><version><BTC size><BTC master public key><ETH size><ETH public key>
 *
 *
 * @param account The account
 * @param bytesCount A non-NULL size_t pointer filled with the bytes count
 *
 * @return The serialization as uint8_t*
 */
extern uint8_t *
cryptoAccountSerialize (BRCryptoAccount account, size_t *bytesCount) {
    assert (NULL != bytesCount);

    size_t chkSize = sizeof (uint16_t); // checksum
    size_t szSize  = sizeof (uint32_t); // size
    size_t verSize = sizeof (uint16_t); // version
    size_t tsSize  = sizeof (uint64_t); // timestamp

    // Version
    uint16_t version = ACCOUNT_SERIALIZE_DEFAULT_VERSION;

    // BTC/BCH
    size_t mpkSize = BRBIP32SerializeMasterPubKey (NULL, 0, account->btc);

    // ETH
    BRKey ethPublicKey = accountGetPrimaryAddressPublicKey (account->eth);
    ethPublicKey.compressed = 0;
    size_t ethSize = BRKeyPubKey (&ethPublicKey, NULL, 0);

    // Overall size - summing all factors.
    *bytesCount = chkSize + szSize + verSize + tsSize + (szSize + mpkSize) + (szSize + ethSize);
    uint8_t *bytes = calloc (1, *bytesCount);
    uint8_t *bytesPtr = bytes;

    // Encode

    // Skip the checksum; will comeback to it
    bytesPtr += chkSize;

    // Size
    UInt32SetBE (bytesPtr, (uint32_t) *bytesCount);
    bytesPtr += szSize;

    // Version
    UInt16SetBE (bytesPtr, version);
    bytesPtr += verSize;

    // timestamp
    UInt64SetBE (bytesPtr, account->timestamp);
    bytesPtr += tsSize;

    // BTC
    UInt32SetBE (bytesPtr, (uint32_t) mpkSize);
    bytesPtr += szSize;

    BRBIP32SerializeMasterPubKey ((char *) bytesPtr, mpkSize, account->btc);
    bytesPtr += mpkSize;

    // ETH
    UInt32SetBE (bytesPtr, (uint32_t) ethSize);
    bytesPtr += szSize;

    BRKeyPubKey (&ethPublicKey, bytesPtr, ethSize);
    bytesPtr += ethSize;

    (void) bytesPtr;  // Avoid static analysis warning

    // checksum
    uint16_t checksum = checksumFletcher16 (&bytes[chkSize], (*bytesCount - chkSize));
    UInt16SetBE (bytes, checksum);

    return bytes;
}

extern uint64_t
cryptoAccountGetTimestamp (BRCryptoAccount account) {
    return account->timestamp;
}

private_extern BREthereumAccount
cryptoAccountAsETH (BRCryptoAccount account) {
    return account->eth;
}

private_extern const char *
cryptoAccountAddressAsETH (BRCryptoAccount account) {
    return accountGetPrimaryAddressString (account->eth);
}

private_extern BRMasterPubKey
cryptoAccountAsBTC (BRCryptoAccount account) {
    return account->btc;
}

// https://en.wikipedia.org/wiki/Fletcher%27s_checksum
static uint16_t
checksumFletcher16(const uint8_t *data, size_t count )
{
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;
    int index;

    for( index = 0; index < count; ++index )
    {
        sum1 = (sum1 + data[index]) % 255;
        sum2 = (sum2 + sum1) % 255;
    }
    return (sum2 << 8) | sum1;
}
