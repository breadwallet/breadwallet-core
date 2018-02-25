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

// Value type?
typedef struct BREthereumAddressRecord *BREthereumAddress;

typedef struct BREthereumAccountRecord *BREthereumAccount;

/**
 * The Bread App will have a single EthereumAccount for both Ether and all ERC20 tokens.  This
 * account is conceptually identical to the App's 'private key' derived from the User's 'paper
 * key'.  An EthereumAccount uses BIP32 (probably not BIP44) to generate addresses; and thus
 * the provided 'private key' must be suitable for BIP32.  [The 'private key` argument is likely
 * to be a BRMasterPubKey thingy]
 *
 * An EthereumAccount can generate an essentially arbitrary number of EthereumAddress-es.  However,
 * in Ethereum addresses are not a factor in privacy; therefore, we'll use one EthereumAddress per
 * EthereumWallet - all transactions for that wallet will use the same address.
 *
 * @return
 */
extern BREthereumAccount
accountCreate(/* private key - derived from paper key - can create BIP32 addresses */);

/**
 * Create an EthereumAddress for the provided `account`
 * @param account
 * @return
 */

extern BREthereumAddress
accountCreateAddress(BREthereumAccount account);


// sign a byte array

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Account_H */
