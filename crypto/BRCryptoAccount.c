//
//  BRCryptoAccount.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <pthread.h>
#include "BRCryptoAccountP.h"
#include "generic/BRGenericHandlers.h"  // genericHandlersInstall
#include "generic/BRGenericRipple.h"    // genericRippleHandlers

static pthread_once_t  _accounts_once = PTHREAD_ONCE_INIT;

static void _accounts_init (void) {
    genHandlersInstall (genericRippleHandlers);
    // ...
}

private_extern void
cryptoAccountInstall (void) {
    pthread_once (&_accounts_once, _accounts_init);
}

static uint16_t
checksumFletcher16 (const uint8_t *data, size_t count);

static void
randomBytes (void *bytes, size_t bytesCount);

// Version 1: BTC (w/ BCH), ETH
// Version 2: BTC (w/ BCH), ETH, XRP
#define ACCOUNT_SERIALIZE_DEFAULT_VERSION  2

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
    randomBytes (entropy.u8, sizeof(entropy));

    size_t phraseLen = BRBIP39Encode (NULL, 0, words, entropy.u8, sizeof(entropy));
    char  *phrase    = calloc (phraseLen, 1);

    // xor to avoid needing an additional variable to perform assert
    phraseLen ^= BRBIP39Encode (phrase, phraseLen, words, entropy.u8, sizeof(entropy));
    assert (0 == phraseLen);

    return phrase;
}

extern BRCryptoBoolean
cryptoAccountValidatePaperKey (const char *phrase, const char *words[]) {
    return AS_CRYPTO_BOOLEAN (BRBIP39PhraseIsValid (words, phrase));
}

extern BRCryptoBoolean
cryptoAccountValidateWordsList (size_t wordsCount) {
    return AS_CRYPTO_BOOLEAN (wordsCount == BIP39_WORDLIST_COUNT);
}

static BRCryptoAccount
cryptoAccountCreateInternal (BRMasterPubKey btc,
                             BREthereumAccount eth,
                             BRGenericAccount xrp,
                             uint64_t timestamp,
                             const char * uids) {
    BRCryptoAccount account = malloc (sizeof (struct BRCryptoAccountRecord));

    account->btc = btc;
    account->eth = eth;
    account->xrp = xrp;
    account->uids = strdup (uids);
    account->timestamp = timestamp;
    account->ref = CRYPTO_REF_ASSIGN(cryptoAccountRelease);

    return account;

}
static BRCryptoAccount
cryptoAccountCreateFromSeedInternal (UInt512 seed,
                                     uint64_t timestamp,
                                     const char *uids) {
    cryptoAccountInstall();

    return cryptoAccountCreateInternal (BRBIP32MasterPubKey (seed.u8, sizeof (seed.u8)),
                                        createAccountWithBIP32Seed(seed),
                                        genAccountCreate (genericRippleHandlers->type, seed),
                                        timestamp,
                                        uids);
}

extern BRCryptoAccount
cryptoAccountCreate (const char *phrase, uint64_t timestamp, const char *uids) {
    return cryptoAccountCreateFromSeedInternal (cryptoAccountDeriveSeedInternal(phrase), timestamp, uids);
}


/**
 * Deserialize into an Account.  The serialization format is:
 *  <checksum16><size32><version>
 *      <BTC size><BTC master public key>
 *      <ETH size><ETH public key>
 *      <XRP size><XRP public key>
 *
 * @param bytes the serialized bytes
 * @param bytesCount the number of serialized bytes
 *
 * @return An Account, or NULL.
 */
extern BRCryptoAccount
cryptoAccountCreateFromSerialization (const uint8_t *bytes, size_t bytesCount, const char *uids) {
    cryptoAccountInstall();

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
    BYTES_PTR_INCR_AND_CHECK (mpkSize);

    // ETH
    size_t ethSize = UInt32GetBE (bytesPtr);
    BYTES_PTR_INCR_AND_CHECK (szSize);
    assert (65 == ethSize);

    BRKey ethPublicKey;
    BRKeySetPubKey(&ethPublicKey, bytesPtr, 65);
    BYTES_PTR_INCR_AND_CHECK (65);
    BREthereumAccount eth = createAccountWithPublicKey(ethPublicKey);

    // XRP
    size_t xrpSize = UInt32GetBE(bytesPtr);
    BYTES_PTR_INCR_AND_CHECK (szSize);

    BRGenericAccount xrp = genAccountCreateWithSerialization (genericRippleHandlers->type, bytesPtr, xrpSize);
    assert (NULL != xrp);

    return cryptoAccountCreateInternal (mpk, eth, xrp, timestamp, uids);
#undef BYTES_PTR_INCR_AND_CHECK
}

static void
cryptoAccountRelease (BRCryptoAccount account) {
    accountFree(account->eth);
    genAccountRelease(account->xrp);

    free (account->uids);
    memset (account, 0, sizeof(*account));
    free (account);
}


/**
 * Serialize the account as per ACCOUNT_SERIALIZE_DEFAULT_VERSION.  The serialization format is:
 *  <checksum16><size32><version>
 *      <BTC size><BTC master public key>
 *      <ETH size><ETH public key>
 *      <XRP size><XRP public key>
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

    // XRP
    size_t   xrpSize = 0;
    uint8_t *xrpBytes = genAccountGetSerialization (account->xrp, &xrpSize);

    // Overall size - summing all factors.
    *bytesCount = (chkSize + szSize + verSize + tsSize
                   + (szSize + mpkSize)
                   + (szSize + ethSize)
                   + (szSize + xrpSize));
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

    // XRP
    UInt32SetBE (bytesPtr, (uint32_t) xrpSize);
    bytesPtr += szSize;

    memcpy (bytesPtr, xrpBytes, xrpSize);
    bytesPtr += xrpSize;

    // Avoid static analysis warning
    (void) bytesPtr;

    // checksum
    uint16_t checksum = checksumFletcher16 (&bytes[chkSize], (*bytesCount - chkSize));
    UInt16SetBE (bytes, checksum);

    free (xrpBytes);

    return bytes;
}

extern BRCryptoBoolean
cryptoAccountValidateSerialization (BRCryptoAccount account,
                                    const uint8_t *bytes,
                                    size_t bytesCount) {

    uint8_t *bytesPtr = (uint8_t *) bytes;
    uint8_t *bytesEnd = bytesPtr + bytesCount;

    size_t chkSize = sizeof (uint16_t); // checksum
    size_t szSize  = sizeof (uint32_t); // size
    size_t verSize = sizeof (uint16_t); // version
    size_t tsSize  = sizeof (uint64_t); // timestamp

    // Skip directly to the BTC MPK
    bytesPtr += (chkSize + szSize + verSize + tsSize);
    if (bytesPtr + szSize > bytesEnd) return CRYPTO_FALSE;

    // BTC
    size_t mpkSize = UInt32GetBE(bytesPtr);
    bytesPtr += szSize;
    // Not enough bytes
    if (bytesPtr + mpkSize > bytesEnd) return CRYPTO_FALSE;

    // We'll check thsee bytes
    uint8_t *mpkBytesToCheck = bytesPtr;

    // Generate a serialization from account->btc
    size_t mpkBytesCount = BRBIP32SerializeMasterPubKey (NULL, 0, account->btc);
    uint8_t mpkBytes[mpkBytesCount];
    BRBIP32SerializeMasterPubKey ((char *) mpkBytes, mpkBytesCount, account->btc);

    if (mpkSize != mpkBytesCount) return CRYPTO_FALSE;

    return AS_CRYPTO_BOOLEAN (0 == memcmp (mpkBytesToCheck, mpkBytes, mpkBytesCount));
}

extern uint64_t
cryptoAccountGetTimestamp (BRCryptoAccount account) {
    return account->timestamp;
}

extern char *
cryptoAccountGetFileSystemIdentifier (BRCryptoAccount account) {
    // Seriailize the master public key
    size_t   mpkSize  = BRBIP32SerializeMasterPubKey (NULL, 0, account->btc);
    uint8_t *mpkBytes = malloc (mpkSize);
    BRBIP32SerializeMasterPubKey ((char*) mpkBytes, mpkSize, account->btc);

    // Double SHA the serialization
    UInt256 hash;
    BRSHA256_2(&hash, mpkBytes, mpkSize);
    free (mpkBytes);

    // Take the first 32 characters.
    return strndup(u256hex(hash), 32);
}

extern const char *
cryptoAccountGetUids (BRCryptoAccount account) {
    return account->uids;
}

private_extern BREthereumAccount
cryptoAccountAsETH (BRCryptoAccount account) {
    return account->eth;
}

private_extern BRGenericAccount
cryptoAccountAsGEN (BRCryptoAccount account,
                    const char *type) {
    if (genAccountHasType (account->xrp, type)) return account->xrp;

    return NULL;
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

#if defined (__ANDROID__)
static void
randomBytes (void *bytes, size_t bytesCount) {
    arc4random_buf (bytes, bytesCount);
}

#else // IOS, MacOS
#include <Security/Security.h>

static void
randomBytes (void *bytes, size_t bytesCount) {
    if (0 != SecRandomCopyBytes(kSecRandomDefault, bytesCount, bytes))
        arc4random_buf (bytes, bytesCount); // fallback
}
#endif
