//
//  BBREthereumAddress.h
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

#ifndef BR_Ethereum_Account_H
#define BR_Ethereum_Account_H

#ifdef __cplusplus
extern "C" {
#endif

#include "BRInt.h"
#include "rlp/BRRlpCoder.h"
#include "BREthereumEther.h"

//
// BIP39 Word List
//

/**
 * Install 'wordList' as the default.  THIS SHARED MEMORY; DO NOT FREE wordList.
 *
 * @param wordList
 * @param wordListLength
 * @return
 */
extern int
installSharedWordList (const char *wordList[], int wordListLength);

//
// Address
//

/**
 *
 */
typedef struct BREthereumAddressRecord *BREthereumAddress;

/**
 * Create an address from the external representation of an address.  The provided address *must*
 * include a prefix of "Ox" and pass the validateAddressString() function; otherwise NULL is
 * returned.
 *
 * @param string
 * @return
 */
extern BREthereumAddress
createAddress (const char *string);

/**
 * Validate `string` as an Ethereum address.  The validation is minimal - based solely on the
 * `string` content.  Said another way, the Ethereum Network is not used for validation.
 *
 * At a minimum `string` must start with "0x", have a total of 42 characters and by a 'hex' string
 * (as if a result of encodeHex(); containing characters [0-9,a-f])
 *
 * @param string
 * @return
 */
extern BREthereumBoolean
validateAddressString(const char *string);

extern uint64_t
addressGetNonce(BREthereumAddress address);

extern void
addressFree (BREthereumAddress address);

/**
 * Returns a string representation of the address, newly allocated.  YOU OWN THIS.
 */
extern char *
addressAsString (BREthereumAddress address);

#if defined (DEBUG)
extern const char *
addressPublicKeyAsString (BREthereumAddress address, int compressed);
#endif

extern BRRlpItem
addressRlpEncode (BREthereumAddress address, BRRlpCoder coder);

//
// Account
//

/**
 * The Bread App will have a single EthereumAccount for both Ether and all ERC20 tokens.  This
 * account is conceptually identical to the App's 'private key' derived from the User's 'paper
 * key'.  An EthereumAccount uses BIP32 (probably not BIP44) to generate addresses; and thus
 * the provided 'private key' must be suitable for BIP32.  [The 'private key` argument is likely
 * a BRMasterPubKey thingy]
 *
 * An EthereumAccount can generate an essentially arbitrary number of EthereumAddress-es.  However,
 * in Ethereum addresses are not a factor in privacy; therefore, we'll use one EthereumAddress per
 * EthereumWallet - all transactions for that wallet will use the same address.
 *
 */
typedef struct BREthereumAccountRecord *BREthereumAccount;

/**
 * Create a new account using paperKey and the sharedWordList (see installSharedWordList).
 *
 * @param paperKey
 * @return
 */
extern BREthereumAccount
createAccount(const char *paperKey);

extern void
accountFree (BREthereumAccount account);

/**
 * Create a new account using paperKey and the provided wordList
 *
 * @param paperKey
 * @param wordList
 * @param wordListLength
 * @return
 */
extern BREthereumAccount
accountCreateDetailed(const char *paperKey, const char *wordList[], const int wordListLength);

/**
 * The account's primary address (aka 'address[0]').
 *
 * TODO: Copy or not
 *
 * @param account
 * @return
 */
extern BREthereumAddress
accountGetPrimaryAddress (BREthereumAccount account);

//
// Signature
//

typedef enum {
    SIGNATURE_TYPE_FOO,
    SIGNATURE_TYPE_RECOVERABLE
} BREthereumSignatureType;

typedef struct {
    BREthereumSignatureType type;
    union {
        struct {
            int ignore;
        } foo;

        struct {
            uint8_t v;
            uint8_t r[32];
            uint8_t s[32];
        } recoverable;
    } sig;
} BREthereumSignature;


/**
 * Sign an arbitrary array of bytes with the account's private key using the signature algorithm
 * specified by `type`.
 *
 * @param account
 * @param type
 * @param bytes
 * @param bytesCount
 * @return
 */
extern BREthereumSignature
accountSignBytes(BREthereumAccount account,
                 BREthereumAddress address,
                 BREthereumSignatureType type,
                 uint8_t *bytes,
                 size_t bytesCount,
                 const char *paperKey);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Account_H */
