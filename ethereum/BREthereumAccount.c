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
#include <regex.h>
#include "BRBIP32Sequence.h"
#include "BRBIP39Mnemonic.h"
#include "BRCrypto.h"
#include "BRBase58.h"

#include "BRUtil.h"
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

#define PRIMARY_ADDRESS_BIP44_INDEX 0

/* Forward Declarations */
static BREthereumAddress
accountCreateAddress (BREthereumAccount account, UInt512 seed, uint32_t index);

static UInt512
deriveSeedFromPaperKey (const char *paperKey);

static BRKey
derivePrivateKeyFromSeed (UInt512 seed, uint32_t index);

//
// Locale-Based BIP-39 Word List
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
// Address
//

// And then there is this:
// TODO: Figure out Ethereum Address
/*
         var md32 = [UInt8](repeating: 0, count: 32)
        BRKeccak256(&md32, ethPubKey, ethPubKey.count)
        self.address = GethAddress(fromBytes: Data(md32[12...]))
 */

typedef enum {
    ADDRESS_PROVIDED,   // target,
    ADDRESS_DERIVED,    // from BIP44
} BREthereumAddressType;


struct BREthereumAddressRecord {

    /**
     * The 'official' ethereum address string for (the external representation of) this
     * BREthereum address.
     *
     * THIS IS NOT A SIMPLE STRING; this is a hex encoded (with encodeHex) string prefixed with
     * "0x".  Generally, when using this string, for example when RLP encoding, one needs to
     * convert back to the byte array (use rlpEncodeItemHexString())
     */
    char string[43];    // '0x' + <40 chars> + '\0'

    /**
     * Identify the type of this address record - created with a provided string or
     * with a provided publicKey.
     */
    BREthereumAddressType type;

    /**
     * The public key.  This started out as a BIP44 264 bits (65 bytes) array with a value of
     * 0x04 at byte 0; we strip off that first byte and are left with 64.  Go figure.
     */
    uint8_t publicKey [64];  // BIP44: 'Master Public Key 'M' (264 bits) - 8

    /**
     * The BIP-44 Index used for this key.
     */
    uint32_t index;
};

extern BREthereumAddress
createAddress (const char *string) {
    if (ETHEREUM_BOOLEAN_IS_FALSE(validateAddressString(string))) return NULL;

    BREthereumAddress address = malloc (sizeof (struct BREthereumAddressRecord));

    address->type = ADDRESS_PROVIDED;
    strncpy (address->string, string, 42);
    address->string[42] = '\0';

    return address;
}

extern BREthereumBoolean
validateAddressString(const char *string) {
    return 42 == strlen(string)
           && '0' == string[0]
           && 'x' == string[1]
           && encodeHexValidate (&string[2])
           ? ETHEREUM_BOOLEAN_TRUE
           : ETHEREUM_BOOLEAN_FALSE;
}

extern void
addressFree (BREthereumAddress address) {
    free (address);
}

/**
 * Create an address given a 65 byte publicKey (derived from a BIP-44 public key).
 *
 * Details: publicKey[0] must be '0x04';
 *
 * @param publicKey
 * @return
 */
static BREthereumAddress
createAddressDerived (const uint8_t *publicKey, uint32_t index) {
    BREthereumAddress address = malloc (sizeof (struct BREthereumAddressRecord));

    address->type = ADDRESS_DERIVED;  // painfully
    address->index = index;

    // Seriously???
    //
    // https://kobl.one/blog/create-full-ethereum-keypair-and-address/#derive-the-ethereum-address-from-the-public-key
    //
    // "The public key is what we need in order to derive its Ethereum address. Every EC public key
    // begins with the 0x04 prefix before giving the location of the two point on the curve. You
    // should remove this leading 0x04 byte in order to hash it correctly. ...

    assert (publicKey[0] == 0x04);

    // Strip off byte 0
    memcpy(address->publicKey, &publicKey[1], sizeof (address->publicKey));

    // We interrupt your regularly scheduled programming...

    // "Use any method you like to get it in the form of an hexadecimal string
    // "The <pub file> now contains the hexadecimal value of the public key without the 0x04 prefix.

    // "An Ethereum address is made of 20 bytes (40 hex characters long), it is commonly
    // represented by adding the 0x prefix. In order to derive it, one should take the keccak-256
    // hash of the hexadecimal form of a public key, then keep only the last 20 bytes (aka get
    // rid of the first 12 bytes).
    //
    // "Simply pass the file containing the public key in hexadecimal format to the keccak-256sum
    // command. Do not forget to use the ‘-x’ option in order to interpret it as hexadecimal and
    // not a simple string.
    //
    // WTF is a 'simple string'.  Seriously??

    // Back to our regularly scheduled programming...
    //
    // We'll assume our BRKeccak256 takes an array of bytes (sure, the argument is void*); NOT
    // a hexadecimal format of a 0x04 stripped public key...

    // Fill in string
    address->string[0] = '0';
    address->string[1] = 'x';

    // Hash the public key (64 bytes, 0x04 prefix axed) and then hex encode the last 20 values
    uint8_t hash[32];
    BRKeccak256(hash, address->publicKey, sizeof (address->publicKey));
    // Offset '2' into address->string and account for the '\0' terminator.
    encodeHex(&address->string[2], 40 + 1, &hash[12], 20);

    // And now the 'checksum after thought'

    // https://ethereum.stackexchange.com/a/19048/33128
    //
    // Ethereum wallet addresses are in hex [0-9A-F]*. While the address itself is case-insensitive
    // (A is the same as a to the network), the case sensitivity is used as a (optional) checksum.
    // It was built as an after-thought to an addressing scheme that lacked basic checksum
    // validation.  https://github.com/ethereum/EIPs/issues/55#issuecomment-187159063
    //
    // The checksum works like so:
    //
    // 1) lowercase address and remove 0x prefix
    // 2) sha3 hash result from #1
    // 3) change nth letter of address according to the nth letter of the hash:
    //      0,1,2,3,4,5,6,7 → Lowercase
    //      8, 9, a, b, c, d, e, f → Uppercase
    //
    // So, you sha3 hash the address, and look at each Nth character of the sha result. If it's 7
    // or below, the Nth character in the address is lowercase. If it is 8 or above, that character
    // is uppercase.

    // We'll skip it.
    return address;
}

extern char *
addressAsString (BREthereumAddress address) {
    char *result = malloc (43);
    strncpy (result, address->string, 43);
    return result;
}

#if defined (DEBUG)
extern const char *
addressPublicKeyAsString (BREthereumAddress address, int compressed) {
  // The byte array at address->publicKey has the '04' 'uncompressed' prefix removed.  Thus
  // the value in publicKey is uncompressed and 64 bytes.  As a string, this result will have
  // an 0x0<n> prefix where 'n' is in { 4: uncompressed, 2: compressed even, 3: compressed odd }.

  // Default, uncompressed
  char *prefix = "0x04";
  size_t sourceLen = sizeof (address->publicKey);           // 64 bytes: { x y }

  if (compressed) {
    sourceLen /= 2;  // use 'x'; skip 'y'
    prefix = (0 == address->publicKey[63] % 2 ? "0x02" : "0x03");
  }

  char *result = malloc (4 + 2 * sourceLen + 1);
  strcpy (result, prefix);  // encode properly...
  encodeHex(&result[4], 2 * sourceLen + 1, address->publicKey, sourceLen);
  
  return result;
}
#endif

extern BRRlpItem
addressRlpEncode (BREthereumAddress address, BRRlpCoder coder) {
  return rlpEncodeItemHexString(coder, address->string);
}

//
// Account
//
struct BREthereumAccountRecord {

    BRMasterPubKey masterPubKey;

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
createAccount(const char *paperKey) {
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
    UInt512 seed = deriveSeedFromPaperKey(paperKey);

    // Create the actual account
    BREthereumAccount account = (BREthereumAccount) calloc (1, sizeof (struct BREthereumAccountRecord));

    // Assign the key; create the primary address.
    account->masterPubKey = BRBIP32MasterPubKey(&seed, sizeof(seed));
    account->primaryAddress = accountCreateAddress(account, seed, PRIMARY_ADDRESS_BIP44_INDEX);

    return account;
}

extern void
accountFree (BREthereumAccount account) {
    addressFree(account->primaryAddress);
    free (account);
}

extern BREthereumAddress
accountGetPrimaryAddress (BREthereumAccount account) {
    return account->primaryAddress;
}

static BREthereumAddress
accountCreateAddress (BREthereumAccount account, UInt512 seed, uint32_t index) {
    BRKey privateKey = derivePrivateKeyFromSeed (seed, index);

    // Seriously???
    //
    // https://kobl.one/blog/create-full-ethereum-keypair-and-address/#derive-the-ethereum-address-from-the-public-key
    //
    // "The private key must be 32 bytes and not begin with 0x00 and the public one must be
    // uncompressed and 64 bytes long or 65 with the constant 0x04 prefix. More on that in the
    // next section. ...

    uint8_t publicKey[65];
    size_t pubKeyLength = BRKeyPubKey(&privateKey, NULL, 0);
    assert (pubKeyLength == 65);

    // "The public key is what we need in order to derive its Ethereum address. Every EC public key
    // begins with the 0x04 prefix before giving the location of the two point on the curve. You
    // should remove this leading 0x04 byte in order to hash it correctly. ...

    BRKeyPubKey(&privateKey, publicKey, 65);
    assert (publicKey[0] = 0x04);

    return createAddressDerived(publicKey, index);
}

//
// Signature
//
extern BREthereumSignature
accountSignBytes(BREthereumAccount account,
                 BREthereumAddress address,
                 BREthereumSignatureType type,
                 uint8_t *bytes,
                 size_t bytesCount,
                 const char *paperKey) {
    BREthereumSignature signature;

    // Save the type.
    signature.type = type;

    // Recreate the seed; then recreate the PrivateKey (for the address with 'index')
    UInt512 seed = deriveSeedFromPaperKey(paperKey);
    BRKey privateKeyUncompressed = derivePrivateKeyFromSeed(seed, address->index);

    // Hash with the required Keccak-256
    UInt256 messageDigest;
    BRKeccak256(&messageDigest, bytes, bytesCount);

    switch (type) {
        case SIGNATURE_TYPE_FOO:
            break;

        case SIGNATURE_TYPE_RECOVERABLE: {
            // Determine the signature length
            size_t signatureLen = BRKeyCompactSign(&privateKeyUncompressed,
                                                   NULL, 0,
                                                   messageDigest);

            // Fill the signature
            uint8_t signatureBytes[signatureLen];
            signatureLen = BRKeyCompactSign(&privateKeyUncompressed,
                                            signatureBytes, signatureLen,
                                            messageDigest);
            assert (65 == signatureLen);

            // The actual 'signature' is one byte added to secp256k1_ecdsa_recoverable_signature
            // and secp256k1_ecdsa_recoverable_signature is 64 bytes as {r[32], s32]}

            // Extract V, R, and S
            signature.sig.recoverable.v = signatureBytes[0];
            memcpy(signature.sig.recoverable.r, &signatureBytes[ 1], 32);
            memcpy(signature.sig.recoverable.s, &signatureBytes[33], 32);

            // TODO: Confirm signature
            // assigns pubKey recovered from compactSig to key and returns true on success
            // int BRKeyRecoverPubKey(BRKey *key, UInt256 md, const void *compactSig, size_t sigLen)

            break;
        }
    }

    return signature;
}

//
// Support
//

static UInt512
deriveSeedFromPaperKey (const char *paperKey) {
    // Generate the 512bit private key using a BIP39 paperKey
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, paperKey, NULL); // no passphrase
    return seed;
}

static BRKey
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


/*
extern const char *
addressEncodePrivateKey(BREthereumAddress address, BREthereumAddressEncodeType type) {
    switch (type) {
        case ADDRESS_ENCODE_HEX: {
            size_t stringLen = 1 + 2 * sizeof(UInt256);

            char *string = malloc (stringLen);

            encodeHex(string, stringLen, (uint8_t *) &address->key.secret, sizeof (UInt256));

            return string;
        }

        case ADDRESS_ENCODE_WIF: {
            // BRKeyPrivKey produces the WIF encoding.

            address->key.compressed = 0;

            // Get the string length
            size_t stringLen = BRKeyPrivKey(&address->key, NULL, 0);

            // Allocate a string
            char *string = malloc(stringLen);


            // Fill it
            return 0 == BRKeyPrivKey(&address->key, string, stringLen)
                   ? NULL
                   : string;
        }

        case ADDRESS_ENCODE_WIF_COMPRESSED:{
            // BRKeyPrivKey produces the WIF encoding.

            address->key.compressed = 1;

            // Get the string length
            size_t stringLen = BRKeyPrivKey(&address->key, NULL, 0);

            // Allocate a string
            char *string = malloc(stringLen);


            // Fill it
            return 0 == BRKeyPrivKey(&address->key, string, stringLen)
                   ? NULL
                   : string;
        }

        case ADDRESS_ENCODE_ETH:
            return NULL;
    }
}

extern char *
addressEncodePublicKey(BREthereumAddress address, BREthereumAddressEncodeType type) {
    switch (type) {
        case ADDRESS_ENCODE_HEX: {
            size_t stringLen = 1 + 2 * sizeof(address->key.pubKey);

            char *string = malloc(stringLen);

            encodeHex (string, stringLen, (uint8_t *) &address->key.pubKey,
                       sizeof(address->key.pubKey));

            return string;
        }
        case ADDRESS_ENCODE_WIF: {
            size_t stringLen = BRBase58CheckEncode(NULL, 0, (const uint8_t *) &address->key.pubKey, sizeof (address->key.pubKey));

            char *string = malloc (stringLen);

            BRBase58CheckEncode(string, stringLen, (const uint8_t *) &address->key.pubKey, sizeof (address->key.pubKey));

            return string;
        }

        case ADDRESS_ENCODE_WIF_COMPRESSED:
            return NULL;

        case ADDRESS_ENCODE_ETH: {
            // https://kobl.one/blog/create-full-ethereum-keypair-and-address/#derive-the-ethereum-address-from-the-public-key
            // The private key must be 32 bytes and not begin with 0x00 and the public one must be
            // uncompressed and 64 bytes long or 65 with the constant 0x04 prefix.
            // More on that in the next section.

            // 1. Start with the public key (128 characters / 64 bytes)
            // 2. Take the Keccak-256 hash of the public key. You should now have a string that is
            //     64 characters / 32 bytes. (note: SHA3-256 eventually became the standard,
            //     but Ethereum uses Keccak)
            // 3. Take the last 40 characters / 20 bytes of this public key (Keccak-256). Or, in
            //     other words, drop the first 24 characters / 12 bytes. These 40 characters / 20
            //     bytes are the address. When prefixed with 0x it becomes 42 characters long.

            uint8_t hash[32];

            BRKeccak256(hash, address->key.pubKey, 64);

            size_t stringLen = 2 + 2 * 20;   // '0x' + 20 bytes + terminator;

            char *string = malloc(1 + stringLen);

            string[0] = '0';
            string[1] = 'x';
            encodeHex(&string[2], stringLen - 2 + 1, &hash[12], 20);

            return string;
        }
    }
}
*/
