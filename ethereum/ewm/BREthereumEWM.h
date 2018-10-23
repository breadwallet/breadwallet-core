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
#include "../blockchain/BREthereumTransaction.h"
#include "../blockchain/BREthereumBlock.h"
#include "../blockchain/BREthereumLog.h"
#include "../BREthereum.h"
#include "BREthereumAccount.h"
#include "BREthereumWallet.h"

#ifdef __cplusplus
extern "C" {
#endif

extern BREthereumEWM
createEWM (BREthereumNetwork network,
           BREthereumAccount account,
           BREthereumTimestamp accountTimestamp,
           BREthereumType type,
           BREthereumSyncMode syncMode,
           BREthereumClient client,
           BRSetOf(BREthereumPersistData) peers,
           BRSetOf(BREthereumPersistData) blocks,
           BRSetOf(BREthereumPersistData) transactions,
           BRSetOf(BREthereumPersistData) logs);

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
ewmConnect(BREthereumEWM ewm);

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

extern BREthereumTransfer
ewmLookupTransfer(BREthereumEWM ewm,
                  BREthereumTransferId tid);

extern BREthereumTransfer
ewmLookupTransferByHash(BREthereumEWM ewm,
                        const BREthereumHash hash);

//
// Wallet
//
extern BREthereumWalletId *
ewmGetWallets (BREthereumEWM ewm);

extern unsigned int
ewmGetWalletsCount (BREthereumEWM ewm);

extern BREthereumWalletId
ewmGetWallet(BREthereumEWM ewm);

extern BREthereumWalletId
ewmGetWalletHoldingToken(BREthereumEWM ewm,
                         BREthereumToken token);

extern BREthereumTransferId
ewmWalletCreateTransfer(BREthereumEWM ewm,
                        BREthereumWallet wallet,
                        const char *recvAddress,
                        BREthereumAmount amount);

extern BREthereumTransferId
ewmWalletCreateTransferWithFeeBasis (BREthereumEWM ewm,
                                     BREthereumWallet wallet,
                                     const char *recvAddress,
                                     BREthereumAmount amount,
                                     BREthereumFeeBasis feeBasis);

extern void // status, error
ewmWalletSignTransfer(BREthereumEWM ewm,
                      BREthereumWallet wallet,
                      BREthereumTransfer transfer,
                      BRKey privateKey);

extern void // status, error
ewmWalletSignTransferWithPaperKey(BREthereumEWM ewm,
                                  BREthereumWallet wallet,
                                  BREthereumTransfer transfer,
                                  const char *paperKey);

extern void // status, error
ewmWalletSubmitTransfer(BREthereumEWM ewm,
                        BREthereumWalletId wid,
                        BREthereumTransferId tid);

extern BREthereumTransferId *
ewmWalletGetTransfers(BREthereumEWM ewm,
                      BREthereumWallet wallet);

extern int
ewmWalletGetTransferCount(BREthereumEWM ewm,
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





//
// Temporary (Perhaps) - Initiate Callbacks to Client
//
//
extern void
ewmUpdateBlockNumber (BREthereumEWM ewm);

extern void
ewmUpdateNonce (BREthereumEWM ewm);

/**
 * Update the transactions for the ewm's account.  A JSON_RPC EWM will call out to
 * BREthereumClientHandlerGetTransactions which is expected to query all transactions associated with the
 * accounts address and then the call out is to call back the 'announce transaction' callback.
 */
extern void
ewmUpdateTransactions (BREthereumEWM ewm);

extern void
ewmUpdateLogs (BREthereumEWM ewm,
               BREthereumWalletId wid,
               BREthereumContractEvent event);

extern void
ewmUpdateTokens (BREthereumEWM ewm);

//
// Wallet Updates
//
extern void
ewmUpdateWalletBalance (BREthereumEWM ewm,
                        BREthereumWalletId wid);

extern void
ewmUpdateTransferGasEstimate (BREthereumEWM ewm,
                              BREthereumWalletId wid,
                              BREthereumTransferId tid);

extern void
ewmUpdateWalletDefaultGasPrice (BREthereumEWM ewm,
                                BREthereumWalletId wid);


#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_EWM_H
