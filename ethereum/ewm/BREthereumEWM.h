//
//  BREthereumEWM
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/5/18.
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

#ifndef BR_Ethereum_EWM_H
#define BR_Ethereum_EWM_H

#include <stdint.h>
#include "BREthereum.h"
#include "../blockchain/BREthereumTransaction.h"
#include "../blockchain/BREthereumBlock.h"
#include "../blockchain/BREthereumLog.h"
#include "BREthereumAccount.h"
#include "BREthereumWallet.h"

#ifdef __cplusplus
extern "C" {
#endif

extern BREthereumEWM
createEWM (BREthereumNetwork network,
           BREthereumAccount account,
           BREthereumType type,
           BREthereumSyncMode syncMode);

extern void
ewmDestroy (BREthereumEWM ewm);

extern BREthereumAccount
ewmGetAccount (BREthereumEWM ewm);

extern BREthereumNetwork
ewmGetNetwork (BREthereumEWM ewm);

//
// Connect & Disconnect
//
extern BREthereumBoolean
ewmConnect(BREthereumEWM ewm,
           BREthereumClient client);

extern BREthereumBoolean
ewmDisconnect (BREthereumEWM ewm);

//
// {Wallet, Block, Transaction} Lookup
//
extern BREthereumWallet
ewmLookupWallet(BREthereumEWM ewm,
                BREthereumWalletId wid);

extern BREthereumBlock
ewmLookupBlock(BREthereumEWM ewm,
               BREthereumBlockId bid);

extern BREthereumBlock
ewmLookupBlockByHash(BREthereumEWM ewm,
                     const BREthereumHash hash);

extern BREthereumTransaction
ewmLookupTransaction(BREthereumEWM ewm,
                     BREthereumTransactionId tid);

extern BREthereumTransaction
ewmLookupTransactionByHash(BREthereumEWM ewm,
                           const BREthereumHash hash);

//
// Wallet
//
extern BREthereumWalletId
ewmGetWallet(BREthereumEWM ewm);

extern BREthereumWalletId
ewmGetWalletHoldingToken(BREthereumEWM ewm,
                         BREthereumToken token);

extern BREthereumTransactionId
ewmWalletCreateTransaction(BREthereumEWM ewm,
                           BREthereumWallet wallet,
                           const char *recvAddress,
                           BREthereumAmount amount);

extern void // status, error
ewmWalletSignTransaction(BREthereumEWM ewm,
                         BREthereumWallet wallet,
                         BREthereumTransaction transaction,
                         BRKey privateKey);

extern void // status, error
ewmWalletSignTransactionWithPaperKey(BREthereumEWM ewm,
                                     BREthereumWallet wallet,
                                     BREthereumTransaction transaction,
                                     const char *paperKey);

extern void // status, error
ewmWalletSubmitTransaction(BREthereumEWM ewm,
                           BREthereumWallet wallet,
                           BREthereumTransaction transaction);

extern BREthereumTransactionId *
ewmWalletGetTransactions(BREthereumEWM ewm,
                         BREthereumWallet wallet);

extern int
ewmWalletGetTransactionCount(BREthereumEWM ewm,
                             BREthereumWallet wallet);

extern void
ewmWalletSetDefaultGasLimit(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumGas gasLimit);

extern void
ewmWalletSetDefaultGasPrice(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumGasPrice gasPrice);

//
// Block
//
extern uint64_t
ewmGetBlockHeight(BREthereumEWM ewm);

extern void
ewmUpdateBlockHeight(BREthereumEWM ewm,
                     uint64_t blockHeight);

#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_EWM_H
