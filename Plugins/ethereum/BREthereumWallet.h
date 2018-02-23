//
//  BBREthereumWallet.h
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

#ifndef BR_Ethereum_Wallet_H
#define BR_Ethereum_Wallet_H

#include "BREthereum.h"
#include "BREthereumAddress.h"
#include "BREthereumTransaction.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREthereumWalletRecord *BREthereumWallet;

extern BREthereumWallet
createEthereumWallet ();

extern BREthereumTransaction
createTransaction (BREthereumWallet wallet,
                   BREthereumAddress recvAddress,
                   BREthereumEther amount);

extern BREthereumTransaction
createTransactionDetailed(BREthereumWallet wallet,
                          BREthereumAddress recvAddress,
                          BREthereumEther amount,
                          int gasPrice,
                          int gasLimit,
                          int nonce);

extern void
signTransaction (BREthereumWallet wallet,
                 BREthereumTransaction transaction);

/**
 * For `transaction`, get the 'signed transaction data' suitable for use in the RPC-JSON Ethereum
 * method `eth_sendRawTransaction`.
 *
 * @param wallet
 * @param transaction
 * @return
 */
extern char *
getRawTransaction (BREthereumWallet wallet,
                   BREthereumTransaction transaction);


extern int
getDefaultGasLimit (BREthereumWallet wallet);

extern int
getDefaultGasPrice (BREthereumWallet wallet);

extern int
getNonce (BREthereumWallet wallet);

extern int
incrementNonce (BREthereumWallet wallet);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Wallet_H */
