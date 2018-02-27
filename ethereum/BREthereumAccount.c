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

#include <malloc.h>
#include <BRBIP39Mnemonic.h>
#include <string.h>
#include <BRKey.h>
#include <BRBIP32Sequence.h>
#include <BRCrypto.h>
#include "BREthereumEther.h"
#include "BREthereumAccount.h"

// Swift

// Wallet Manager
//     init(masterPubKey: BRMasterPubKey, earliestKeyTime: TimeInterval, dbPath: String? = nil, store: Store) throws {
//
// public key for etherium wallet
// var ethPubKey: [UInt8]? {
//    var key = BRKey(privKey: ethPrivKey!)
//    defer { key?.clean() }
//    key?.compressed = 0
//    guard let pubKey = key?.pubKey(), pubKey.count == 65 else { return nil }
//    return [UInt8](pubKey[1...])
//  }
//
//
//     var ethPrivKey: String? {
//        return autoreleasepool {
//            do {
//                if let ethKey: String? = try? keychainItem(key: KeychainKey.ethPrivKey) {
//                    if ethKey != nil { return ethKey }
//                }
//                var key = BRKey()
//                var seed = UInt512()
//                guard let phrase: String = try keychainItem(key: KeychainKey.mnemonic) else { return nil }
//                BRBIP39DeriveKey(&seed, phrase, nil)
//                // BIP44 etherium path m/44H/60H/0H/0/0: https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
//                BRBIP32vPrivKeyPath(&key, &seed, MemoryLayout<UInt512>.size, 5,
//                                    getVaList([44 | BIP32_HARD, 60 | BIP32_HARD, 0 | BIP32_HARD, 0, 0]))    // ... change(=0), index(=0)
//                seed = UInt512() // clear seed
//                let pkLen = BRKeyPrivKey(&key, nil, 0)
//                var pkData = CFDataCreateMutable(secureAllocator, pkLen) as Data
//                pkData.count = pkLen
//                guard pkData.withUnsafeMutableBytes({ BRKeyPrivKey(&key, $0, pkLen) }) == pkLen else { return nil }
//                let privKey = CFStringCreateFromExternalRepresentation(secureAllocator, pkData as CFData,
//                                                                       CFStringBuiltInEncodings.UTF8.rawValue) as String
//                try setKeychainItem(key: KeychainKey.ethPrivKey, item: privKey)
//                return privKey
//            }
//            catch let error {
//                print("apiAuthKey error: \(error)")
//                return nil
//            }
//        }
//    }

// BIP39 test vectors
// https://github.com/trezor/python-mnemonic/blob/master/vectors.json

// Ethereum
// https://mytokenwallet.com/bip39.html

/* Forward Declarations */
static BREthereumAddress
accountCreatePrimaryAddress (BREthereumAccount account);

//
// Locale-Based BIP-39 Word List
static char **sharedWordList = NULL;

#define WORD_LIST_LENGTH 2048

extern int
installSharedWordList (char *wordList[], int wordListLength) {
    if (BIP39_WORDLIST_COUNT != wordListLength)
        return 0;

    sharedWordList = wordList;

    return 1;
}

//
// Address
//

// And then there is this:
// TODO: Figure out Ethereum Address
/*
         var md32 = [UInt8](repeating: 0, count: 32)
        BRKeccak256(&md32, ethPubKey, ethPubKey.count)
        self.address = GethAddress(fromBytes: Data(md32[12...]))
 */

struct BREthereumAddressRecord {

    /**
     * The BRKey representing this address.  This is derived from the BIP39 'paper key' using
     * a BIP44 Ethereum path.  From this key both the privateKey and publicKey can be derived.
     */
    BRKey key;
};

/**
 * Create a BIP32 Ethereum address with the provided 'index'.
 *
 * @param seed The 'paper key' derived seed
 * @param index the address index
 * @return An ethereum address
 */
static BREthereumAddress
addressCreate (UInt512 seed, uint32_t index) {

    BREthereumAddress address = (BREthereumAddress) calloc (1, sizeof (struct BREthereumAddressRecord));

    // The BIP32 privateKey for m/44'/60'/0'/0/index
    BRBIP32PrivKeyPath (&address->key, &seed, sizeof(UInt512), 5,
                        44 | BIP32_HARD,          // purpose  : BIP-44
                        60 | BIP32_HARD,          // coin_type: Ethereum
                        0  | BIP32_HARD,          // account  : <n/a>
                        0,                        // change   : not change
                        index);                   // index    :
    return address;
}

extern const char *
addressGetPrivateKeyString (BREthereumAddress address) {
    size_t stringLen = BRKeyPrivKey(&address->key, NULL, 0);
    char *string = malloc(stringLen);

    return 0 == BRKeyPrivKey(&address->key, string, stringLen)
           ? NULL
           : string;
}

extern char *
addressGetPublicKeyString (BREthereumAddress address) {
    size_t stringLen = BRKeyPubKey(&address->key, NULL, 0);
    char *string = malloc(stringLen);

    return 0 == BRKeyPubKey(&address->key, string, stringLen)
           ? NULL
           : string;
}

extern const UInt160
addressGetPublicKeyHash160 (BREthereumAddress address) {
    return BRKeyHash160(&address->key);
}

extern char *
addressGetPublicKeyKeccak256 (BREthereumAddress address) {
    // https://ethereum.stackexchange.com/a/3619/33128

    uint8_t md[32];

    BRKeccak256 (md, address->key.pubKey, sizeof (address->key.pubKey));

    // Take the last 20 bytes, convert to hex, prefix with '0x' -> 42
    char *string = malloc (43);



}

//
// Account
//
struct BREthereumAccountRecord {
    // TODO: Probably MasterPubKey

    /**
     * The BIP39-Derived 512-bit key/seed.  This will be used a a BIP32 seed so as to
     * generate addresses.
     */
    UInt512 privateKey;

    /**
     * The primary address for this account - aka address[0].
     */
    BREthereumAddress primaryAddress;
};

extern BREthereumAccount
accountCreate(const char *paperKey) {
    return accountCreateDetailed(paperKey, sharedWordList, BIP39_WORDLIST_COUNT);
}

extern BREthereumAccount
accountCreateDetailed(const char *paperKey, const char *wordList[], const int wordListLength) {

    // Validate arguments
    if (NULL == paperKey || NULL == wordList || BIP39_WORDLIST_COUNT != wordListLength)
        return NULL;

    // Validate paperKey
    if (0 == BRBIP39Decode(NULL, 0, wordList, paperKey))
        return NULL;

    // Generate the 512bit private key using a BIP39 paperKey
    UInt512 key = UINT512_ZERO;
    BRBIP39DeriveKey(key.u8, paperKey, NULL); // no passphrase


    // Generate the MasterPubKey
    // ...

    // Create the actual account
    BREthereumAccount account = (BREthereumAccount) calloc (1, sizeof (struct BREthereumAccountRecord));

    // Assign the key; create the primary address.
    account->privateKey     = key;
    account->primaryAddress = accountCreatePrimaryAddress (account);

    return account;
}

extern BREthereumAddress
accountGetPrimaryAddress (BREthereumAccount account) {
    return account->primaryAddress;
}

static BREthereumAddress
accountCreateAddress(BREthereumAccount account) {
    int nextIndex = 0; // NO

    return addressCreate(account->privateKey, nextIndex);

}

static BREthereumAddress
accountCreatePrimaryAddress (BREthereumAccount account) {
    return addressCreate (account->privateKey, 0);
}

//
// Signature
//
extern BREthereumSignature
accountSignBytes(BREthereumAccount account,
                 BREthereumAddress address,
                 BREthereumSignatureType type,
                 uint8_t *bytes,
                 size_t bytesCount) {
    BREthereumSignature signature;

    signature.type = type;

    // TODO: Implement
    switch (type) {
        case SIGNATURE_TYPE_FOO:
            break;
        case SIGNATURE_TYPE_VRS:
            break;
    }
    return signature;
}

