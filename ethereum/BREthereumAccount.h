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

#include "BRKey.h"
#include "BRInt.h"
#include "base/BREthereumBase.h"
#include "BREthereumEncodedAddress.h"

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

/**
 * Create a new account using the 65 bytes, 0x04-prefixed, uncompressed public key (as returned
 * by addressGetPublicKey())
 */
extern BREthereumAccount
createAccountWithPublicKey (const BRKey publicKey);

/**
 * Create a new account using paperKey and the provided wordList
 *
 * @param paperKey
 * @param wordList
 * @param wordListLength
 * @return
 */
extern BREthereumAccount
createAccountDetailed(const char *paperKey, const char *wordList[], const int wordListLength);

extern void
accountFree (BREthereumAccount account);

/**
 * The account's primary address (aka 'address[0]').
 *
 * @param account
 * @return
 */
extern BREthereumAddress
accountGetPrimaryAddress(BREthereumAccount account);

extern char *
accountGetPrimaryAddressString (BREthereumAccount account);

/**
 * the public key for the account's primary address
 */
extern BRKey
accountGetPrimaryAddressPublicKey (BREthereumAccount account);

/**
 * the privateKey for the account's primary address
 */
extern BRKey
accountGetPrimaryAddressPrivateKey (BREthereumAccount account,
                                    const char *paperKey);

extern BREthereumBoolean
accountHasAddress(BREthereumAccount account,
                  BREthereumAddress address);
    
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
accountSignBytesWithPrivateKey(BREthereumAccount account,
                               BREthereumAddress address,
                               BREthereumSignatureType type,
                               uint8_t *bytes,
                               size_t bytesCount,
                               BRKey privateKey);

extern BREthereumSignature
accountSignBytes(BREthereumAccount account,
                 BREthereumAddress address,
                 BREthereumSignatureType type,
                 uint8_t *bytes,
                 size_t bytesCount,
                 const char *paperKey);

//
// Support (quasi-private)
//
extern UInt512
deriveSeedFromPaperKey (const char *paperKey);

extern BRKey
derivePrivateKeyFromSeed (UInt512 seed, uint32_t index);


//
// New
//
extern uint32_t
accountGetAddressIndex (BREthereumAccount account,
                        BREthereumAddress address);

extern uint32_t
accountGetAddressNonce (BREthereumAccount account,
                        BREthereumAddress address);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Account_H */
