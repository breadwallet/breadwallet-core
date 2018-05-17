//
//  BBREthereumAddress.c
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/21/2018.
//  Copyright (c) 2018 breadwallet LLC
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <BRKey.h>
#include "BRBIP32Sequence.h"
#include "BRBIP39Mnemonic.h"
#include "BRCrypto.h"
#include "BRBase58.h"
#include "BRBIP39WordsEn.h"

#include "base/BREthereumBase.h"
#include "BREthereumAccount.h"

// BIP39 test vectors
// https://github.com/trezor/python-mnemonic/blob/master/vectors.json

// Ethereum
// https://mytokenwallet.com/bip39.html

#define PRIMARY_ADDRESS_BIP44_INDEX 0

/* Forward Declarations */
static BREthereumEncodedAddress
accountCreateAddress (BREthereumAccount account, UInt512 seed, uint32_t index);

//
// Locale-Based BIP-39 Word List
//
static const char **sharedWordList;

#define WORD_LIST_LENGTH 2048

extern int
installSharedWordList (const char *wordList[], int wordListLength) {
    if (BIP39_WORDLIST_COUNT != wordListLength)
        return 0;
    
    sharedWordList = wordList;
    
    return 1;
}

//
// Account
//
struct BREthereumAccountRecord {
    
    BRMasterPubKey masterPubKey;
    
    /**
     * The primary address for this account - aka address[0].
     */
    BREthereumEncodedAddress primaryAddress;
};

static BREthereumAccount
createAccountWithBIP32Seed (UInt512 seed) {
    BREthereumAccount account = (BREthereumAccount) calloc (1, sizeof (struct BREthereumAccountRecord));

    // Assign the key; create the primary address.
    account->masterPubKey = BRBIP32MasterPubKey(&seed, sizeof(seed));
    account->primaryAddress = accountCreateAddress(account, seed, PRIMARY_ADDRESS_BIP44_INDEX);

    return account;

}

extern BREthereumAccount
createAccountWithPublicKey (const BRKey key) { // 65 bytes, 0x04-prefixed, uncompressed public key
    BREthereumAccount account = (BREthereumAccount) calloc (1, sizeof (struct BREthereumAccountRecord));

    // Assign the key; create the primary address.
    account->masterPubKey = BR_MASTER_PUBKEY_NONE;
    account->primaryAddress = createAddressDerived (&key, PRIMARY_ADDRESS_BIP44_INDEX);

    return account;
}

extern BREthereumAccount
createAccountDetailed(const char *paperKey, const char *wordList[], const int wordListLength) {

    // Validate arguments
    if (NULL == paperKey || NULL == wordList || BIP39_WORDLIST_COUNT != wordListLength)
        return NULL;

    // Validate paperKey
    if (0 == BRBIP39Decode(NULL, 0, wordList, paperKey))
        return NULL;

    // Generate the 512bit private key using a BIP39 paperKey
    return createAccountWithBIP32Seed(deriveSeedFromPaperKey(paperKey));
}

extern BREthereumAccount
createAccount(const char *paperKey) {
    if (NULL == sharedWordList)
        installSharedWordList(BRBIP39WordsEn, BIP39_WORDLIST_COUNT);
    
    return createAccountDetailed(paperKey, sharedWordList, BIP39_WORDLIST_COUNT);
}

extern void
accountFree (BREthereumAccount account) {
    addressFree(account->primaryAddress);
    free (account);
}

extern BREthereumEncodedAddress
accountGetPrimaryAddress (BREthereumAccount account) {
    return account->primaryAddress;
}

extern BRKey
accountGetPrimaryAddressPublicKey (BREthereumAccount account) {
    return addressGetPublicKey(account->primaryAddress);
}

extern BRKey
accountGetPrimaryAddressPrivateKey (BREthereumAccount account,
                                    const char *paperKey) {
 return derivePrivateKeyFromSeed(deriveSeedFromPaperKey(paperKey),
                                 addressGetIndex (account->primaryAddress));
}

extern BREthereumBoolean
accountHasAddress (BREthereumAccount account,
                   BREthereumEncodedAddress address) {
    return addressEqual(account->primaryAddress, address);
}

static BREthereumEncodedAddress
accountCreateAddress (BREthereumAccount account, UInt512 seed, uint32_t index) {
    BRKey key = derivePrivateKeyFromSeed (seed, index);
    
    // Seriously???
    //
    // https://kobl.one/blog/create-full-ethereum-keypair-and-address/#derive-the-ethereum-address-from-the-public-key
    //
    // "The private key must be 32 bytes and not begin with 0x00 and the public one must be
    // uncompressed and 64 bytes long or 65 with the constant 0x04 prefix. More on that in the
    // next section. ...
    
    assert (65 == BRKeyPubKey(&key, NULL, 0));
    
    // "The public key is what we need in order to derive its Ethereum address. Every EC public key
    // begins with the 0x04 prefix before giving the location of the two point on the curve. You
    // should remove this leading 0x04 byte in order to hash it correctly. ...
    
    return createAddressDerived(&key, index);
}


extern BREthereumSignature
accountSignBytesWithPrivateKey(BREthereumAccount account,
                               BREthereumEncodedAddress address,
                               BREthereumSignatureType type,
                               uint8_t *bytes,
                               size_t bytesCount,
                               BRKey privateKey) {
    return signatureCreate(type, bytes, bytesCount, privateKey);
}

extern BREthereumSignature
accountSignBytes(BREthereumAccount account,
                 BREthereumEncodedAddress address,
                 BREthereumSignatureType type,
                 uint8_t *bytes,
                 size_t bytesCount,
                 const char *paperKey) {
    UInt512 seed = deriveSeedFromPaperKey(paperKey);
    return accountSignBytesWithPrivateKey(account,
                             address,
                             type,
                             bytes,
                             bytesCount,
                             derivePrivateKeyFromSeed(seed, addressGetIndex(address)));
}

//
// Support
//

extern UInt512
deriveSeedFromPaperKey (const char *paperKey) {
    // Generate the 512bit private key using a BIP39 paperKey
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, paperKey, NULL); // no passphrase
    return seed;
}

extern BRKey
derivePrivateKeyFromSeed (UInt512 seed, uint32_t index) {
    BRKey privateKey;
    
    // The BIP32 privateKey for m/44'/60'/0'/0/index
    BRBIP32PrivKeyPath(&privateKey, &seed, sizeof(UInt512), 5,
                       44 | BIP32_HARD,          // purpose  : BIP-44
                       60 | BIP32_HARD,          // coin_type: Ethereum
                       0 | BIP32_HARD,          // account  : <n/a>
                       0,                        // change   : not change
                       index);                   // index    :
    
    privateKey.compressed = 0;
    
    return privateKey;
}
