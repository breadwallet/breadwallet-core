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

#include <pthread.h>

#include "BRCryptoAccount.h"
#include "BRCryptoPrivate.h"

#include "support/BRBIP32Sequence.h"
#include "support/BRBIP39Mnemonic.h"
#include "support/BRKey.h"
#include "ethereum/BREthereum.h"
#include "generic/BRGenericRipple.h"

static pthread_once_t  _accounts_once = PTHREAD_ONCE_INIT;

#include "generic/BRGenericHandlers.h"

static void _accounts_init (void) {
    genericHandlersInstall (genericRippleHandlers);
    // ...
}

static void
cryptoAccountRelease (BRCryptoAccount account);

struct BRCryptoAccountRecord {
    BRMasterPubKey btc;
    BREthereumAccount eth;
    BRGenericAccount xrp;
    // ...

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

static BRCryptoAccount
cryptoAccountCreateFromSeedInternal (UInt512 seed,
                                     uint64_t timestamp) {
    pthread_once (&_accounts_once, _accounts_init);

    BRCryptoAccount account = malloc (sizeof (struct BRCryptoAccountRecord));

    account->btc = BRBIP32MasterPubKey (seed.u8, sizeof (seed.u8));
    account->eth = createAccountWithBIP32Seed(seed);
    account->xrp = gwmAccountCreate (genericRippleHandlers->type, seed);
    // ...

    account->timestamp = timestamp;
    account->ref = CRYPTO_REF_ASSIGN(cryptoAccountRelease);

    return account;
}

extern UInt512
cryptoAccountDeriveSeed (const char *phrase) {
    return cryptoAccountDeriveSeedInternal(phrase);
}

extern BRCryptoAccount
cryptoAccountCreate (const char *phrase, uint64_t timestamp) {
    return cryptoAccountCreateFromSeedInternal (cryptoAccountDeriveSeedInternal(phrase), timestamp);
}

extern BRCryptoAccount
cryptoAccountCreateFromSeed (UInt512 seed, uint64_t timestamp) {
    return cryptoAccountCreateFromSeedInternal (seed, timestamp);
}

extern BRCryptoAccount
cryptoAccountCreateFromSeedBytes (const uint8_t *bytes, uint64_t timestamp) {
    UInt512 seed;
    memcpy (seed.u8, bytes, sizeof (seed.u8));
    return cryptoAccountCreateFromSeedInternal (seed, timestamp);
}

static void
cryptoAccountRelease (BRCryptoAccount account) {
//    printf ("Account: Release\n");
    accountFree(account->eth);
    gwmAccountRelease(account->xrp);

    free (account);
}

extern uint64_t
cryptoAccountGetTimestamp (BRCryptoAccount account) {
    return account->timestamp;
}

extern void
cryptoAccountSetTimestamp (BRCryptoAccount account,
                           uint64_t timestamp) {
    account->timestamp = timestamp;
}

private_extern BREthereumAccount
cryptoAccountAsETH (BRCryptoAccount account) {
    return account->eth;
}

private_extern BRGenericAccount
cryptoAccountAsGEN (BRCryptoAccount account,
                    const char *type) {
    if (gwmAccountHasType (account->xrp, type)) return account->xrp;

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
