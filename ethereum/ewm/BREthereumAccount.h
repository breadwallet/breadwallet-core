//
//  BBREthereumAddress.h
//  Core Ethereum
//
//  Created by Ed Gamble on 2/21/2018.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Account_H
#define BR_Ethereum_Account_H

#include "support/BRInt.h"
#include "support/BRKey.h"
#include "BREthereumBase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a new account using paperKey and the sharedWordList (see installSharedWordList).
 *
 * @param paperKey
 * @return
 */
extern BREthereumAccount
ethAccountCreate (const char *paperKey);

/**
 * Create a new account using the 65 bytes, 0x04-prefixed, uncompressed public key (as returned
 * by addressGetPublicKey())
 */
extern BREthereumAccount
ethAccountCreateWithPublicKey (const BRKey publicKey);

extern BREthereumAccount
ethAccountCreateWithBIP32Seed (UInt512 seed);

/**
 * Create a new account using paperKey and the provided wordList
 *
 * @param paperKey
 * @param wordList
 * @param wordListLength
 * @return
 */
extern BREthereumAccount
ethAccountCreateDetailed(const char *paperKey, const char *wordList[], const int wordListLength);

extern void
ethAccountRelease (BREthereumAccount account);

/**
 * The account's primary address (aka 'address[0]').
 *
 * @param account
 * @return
 */
extern BREthereumAddress
ethAccountGetPrimaryAddress(BREthereumAccount account);

extern char *
ethAccountGetPrimaryAddressString (BREthereumAccount account);

/**
 * the public key for the account's primary address
 */
extern BRKey
ethAccountGetPrimaryAddressPublicKey (BREthereumAccount account);

/**
 * the privateKey for the account's primary address
 */
extern BRKey
ethAccountGetPrimaryAddressPrivateKey (BREthereumAccount account,
                                       const char *paperKey);

#if defined (DEBUG)
extern const char *
ethAccountGetPrimaryAddressPublicKeyString (BREthereumAccount account, int compressed);
#endif

extern BREthereumBoolean
ethAccountHasAddress(BREthereumAccount account,
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
ethAccountSignBytesWithPrivateKey(BREthereumAccount account,
                                  BREthereumAddress address,
                                  BREthereumSignatureType type,
                                  uint8_t *bytes,
                                  size_t bytesCount,
                                  BRKey privateKey);

extern BREthereumSignature
ethAccountSignBytes(BREthereumAccount account,
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
ethAccountGetAddressIndex (BREthereumAccount account,
                           BREthereumAddress address);

extern uint64_t
ethAccountGetAddressNonce (BREthereumAccount account,
                           BREthereumAddress address);

//
// Private
//
private_extern void
ethAccountSetAddressNonce(BREthereumAccount account,
                          BREthereumAddress address,
                          uint64_t nonce,
                          BREthereumBoolean force);

private_extern uint64_t
ethAccountGetThenIncrementAddressNonce(BREthereumAccount account,
                                       BREthereumAddress address);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Account_H */
