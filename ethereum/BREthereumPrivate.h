//
//  BREthereum
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/24/18.
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

#ifndef BR_Ethereum_Private_H
#define BR_Ethereum_Private_H

#import "BREthereum.h"

// Returns Ether appropriate for encoding a transaction.  If the transaction is for a TOKEN,
// then the returned Ether is zero (because the amount of a TOKEN transfer is encoded in the
// contract's function call, in the transaction.data field).
private_extern BREthereumEther
transactionGetEffectiveAmountInEther (BREthereumTransaction transaction);

private_extern BREthereumTransaction
walletGetTransactionById (BREthereumWallet wallet,
                          BREthereumLightNodeTransactionId transactionId);

private_extern void
walletSetBalance (BREthereumWallet wallet,
                  BREthereumAmount balance);

private_extern void
walletTransactionSubmitted (BREthereumWallet wallet,
                            BREthereumTransaction transaction,
                            BREthereumHash hash); // ....

private_extern void
walletTransactionBlocked (BREthereumWallet wallet,
                          BREthereumTransaction transaction,
                          BREthereumHash blockHash,
                          unsigned int blockIndex);

private_extern void
walletTransactionDropped (BREthereumWallet wallet,
                          BREthereumTransaction transaction);

private_extern void
walletHandleTransaction (BREthereumWallet wallet,
                         BREthereumTransaction transaction);
//
// Address
//

private_extern void
addressSetNonce(BREthereumAddress address,
                uint64_t nonce);

private_extern uint64_t
addressGetThenIncrementNonce(BREthereumAddress address);

//
// Token Lookup
//
private_extern BREthereumToken
tokenLookup (const char *address);

//
// Block
//
private_extern void
blockFree (BREthereumBlock block);

#endif /* BR_Ethereum_Private_H */
