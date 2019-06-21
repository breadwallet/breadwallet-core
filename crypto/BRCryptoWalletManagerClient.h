//
//  BRCryptoWalletManagerClient.h
//  BRCrypto
//
//  Created by Michael Carrara on 6/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
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

#ifndef BRCryptoWalletManagerClient_h
#define BRCryptoWalletManagerClient_h

#include "BRCryptoBase.h"
#include "BRCryptoNetwork.h"
#include "BRCryptoAccount.h"
#include "BRCryptoTransfer.h"
#include "BRCryptoWallet.h"

#include "ethereum/BREthereum.h"
#include "bitcoin/BRWalletManager.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef void *BRCryptoCWMClientContext;

    typedef struct BRCryptoCWMClientCallbackStateRecord *BRCryptoCWMClientCallbackState;

    typedef void
        (*BRCryptoCWMEthGetEtherBalanceCallback) (BRCryptoCWMClientContext context,
                                                  BRCryptoWalletManager manager,
                                                  BRCryptoCWMClientCallbackState callbackState,
                                                  const char *network,
                                                  const char *address);

    typedef void
        (*BRCryptoCWMEthGetTokenBalanceCallback) (BRCryptoCWMClientContext context,
                                                  BRCryptoWalletManager manager,
                                                  BRCryptoCWMClientCallbackState callbackState,
                                                  const char *network,
                                                  const char *address,
                                                  const char *tokenAddress);

    typedef void
        (*BRCryptoCWMEthGetGasPriceCallback) (BRCryptoCWMClientContext context,
                                              BRCryptoWalletManager manager,
                                              BRCryptoCWMClientCallbackState callbackState,
                                              const char *network);

    typedef void
        (*BRCryptoCWMEthEstimateGasCallback) (BRCryptoCWMClientContext context,
                                              BRCryptoWalletManager manager,
                                              BRCryptoCWMClientCallbackState callbackState,
                                              const char *network,
                                              const char *from,
                                              const char *to,
                                              const char *amount,
                                              const char *data);

    typedef void
        (*BRCryptoCWMEthSubmitTransactionCallback) (BRCryptoCWMClientContext context,
                                                    BRCryptoWalletManager manager,
                                                    BRCryptoCWMClientCallbackState callbackState,
                                                    const char *network,
                                                    const char *transaction);

    typedef void
        (*BRCryptoCWMEthGetTransactionsCallback) (BRCryptoCWMClientContext context,
                                                  BRCryptoWalletManager manager,
                                                  BRCryptoCWMClientCallbackState callbackState,
                                                  const char *network,
                                                  const char *address,
                                                  uint64_t begBlockNumber,
                                                  uint64_t endBlockNumber);

    typedef void
        (*BRCryptoCWMEthGetLogsCallback) (BRCryptoCWMClientContext context,
                                          BRCryptoWalletManager manager,
                                          BRCryptoCWMClientCallbackState callbackState,
                                          const char *network,
                                          const char *contract,
                                          const char *address,
                                          const char *event,
                                          uint64_t begBlockNumber,
                                          uint64_t endBlockNumber);

    typedef void
        (*BRCryptoCWMEthGetBlocksCallback) (BRCryptoCWMClientContext context,
                                            BRCryptoWalletManager manager,
                                            BRCryptoCWMClientCallbackState callbackState,
                                            const char *network,
                                            const char *address, // disappears immediately
                                            BREthereumSyncInterestSet interests,
                                            uint64_t blockNumberStart,
                                            uint64_t blockNumberStop);

    typedef void
        (*BRCryptoCWMEthGetTokensCallback) (BRCryptoCWMClientContext context,
                                            BRCryptoWalletManager manager,
                                            BRCryptoCWMClientCallbackState callbackState);

    typedef void
        (*BRCryptoCWMEthGetBlockNumberCallback) (BRCryptoCWMClientContext context,
                                                 BRCryptoWalletManager manager,
                                                 BRCryptoCWMClientCallbackState callbackState,
                                                 const char *network);

    typedef void
        (*BRCryptoCWMEthGetNonceCallback) (BRCryptoCWMClientContext context,
                                           BRCryptoWalletManager manager,
                                           BRCryptoCWMClientCallbackState callbackState,
                                           const char *network,
                                           const char *address);

    typedef struct {
        BRCryptoCWMEthGetEtherBalanceCallback funcGetEtherBalance;
        BRCryptoCWMEthGetTokenBalanceCallback funcGetTokenBalance;
        BRCryptoCWMEthGetGasPriceCallback funcGetGasPrice;
        BRCryptoCWMEthEstimateGasCallback funcEstimateGas;
        BRCryptoCWMEthSubmitTransactionCallback funcSubmitTransaction;
        BRCryptoCWMEthGetTransactionsCallback funcGetTransactions; // announce one-by-one
        BRCryptoCWMEthGetLogsCallback funcGetLogs; // announce one-by-one
        BRCryptoCWMEthGetBlocksCallback funcGetBlocks;
        BRCryptoCWMEthGetTokensCallback funcGetTokens; // announce one-by-one
        BRCryptoCWMEthGetBlockNumberCallback funcGetBlockNumber;
        BRCryptoCWMEthGetNonceCallback funcGetNonce;
    } BRCryptoCWMClientETH;

    typedef void
    (*BRCryptoCWMBtcGetBlockNumberCallback) (BRCryptoCWMClientContext context,
                                             BRCryptoWalletManager manager,
                                             BRCryptoCWMClientCallbackState callbackState);

    typedef void
    (*BRCryptoCWMBtcGetTransactionsCallback) (BRCryptoCWMClientContext context,
                                              BRCryptoWalletManager manager,
                                              BRCryptoCWMClientCallbackState callbackState,
                                              char **addresses,
                                              size_t addressCount,
                                              uint64_t begBlockNumber,
                                              uint64_t endBlockNumber);

    typedef void
    (*BRCryptoCWMBtcSubmitTransactionCallback) (BRCryptoCWMClientContext context,
                                                BRCryptoWalletManager manager,
                                                BRCryptoCWMClientCallbackState callbackState,
                                                uint8_t *transaction,
                                                size_t transactionLength);

    typedef struct {
        BRCryptoCWMBtcGetBlockNumberCallback  funcGetBlockNumber;
        BRCryptoCWMBtcGetTransactionsCallback funcGetTransactions;
        BRCryptoCWMBtcSubmitTransactionCallback funcSubmitTransaction;
    } BRCryptoCWMClientBTC;

    typedef struct {
    } BRCryptoCWMClientGEN;

    typedef struct {
        BRCryptoCWMClientContext context;
        BRCryptoCWMClientBTC btc;
        BRCryptoCWMClientETH eth;
        BRCryptoCWMClientGEN gen;
    } BRCryptoCWMClient;

    extern BRWalletManagerClient
    cryptoWalletManagerClientCreateBTCClient (BRCryptoWalletManager cwm);

    extern BREthereumClient
    cryptoWalletManagerClientCreateETHClient (BRCryptoWalletManager cwm);

    extern void
    cwmAnnounceGetBlockNumberSuccessAsInteger (BRCryptoWalletManager cwm,
                                               BRCryptoCWMClientCallbackState callbackState,
                                               uint64_t blockNumber);

    extern void
    cwmAnnounceGetBlockNumberSuccessAsString (BRCryptoWalletManager cwm,
                                              BRCryptoCWMClientCallbackState callbackState,
                                              const char *blockNumber);

    extern void
    cwmAnnounceGetBlockNumberFailure (BRCryptoWalletManager cwm,
                                      BRCryptoCWMClientCallbackState callbackState);

    extern void
    cwmAnnounceGetTransactionsItemBTC (BRCryptoWalletManager cwm,
                                       BRCryptoCWMClientCallbackState callbackState,
                                       uint8_t *transaction,
                                       size_t transactionLength,
                                       uint64_t timestamp,
                                       uint64_t blockHeight);

    extern void
    cwmAnnounceGetTransactionsItemETH (BRCryptoWalletManager cwm,
                                       BRCryptoCWMClientCallbackState callbackState,
                                       const char *hash,
                                       const char *from,
                                       const char *to,
                                       const char *contract,
                                       const char *amount, // value
                                       const char *gasLimit,
                                       const char *gasPrice,
                                       const char *data,
                                       const char *nonce,
                                       const char *gasUsed,
                                       const char *blockNumber,
                                       const char *blockHash,
                                       const char *blockConfirmations,
                                       const char *blockTransactionIndex,
                                       const char *blockTimestamp,
                                       // cumulative gas used,
                                       // confirmations
                                       // txreceipt_status
                                       const char *isError);

    extern void
    cwmAnnounceGetTransactionsComplete (BRCryptoWalletManager cwm,
                                        BRCryptoCWMClientCallbackState callbackState,
                                        BRCryptoBoolean success);

    extern void
    cwmAnnounceSubmitTransferSuccess (BRCryptoWalletManager cwm,
                                      BRCryptoCWMClientCallbackState callbackState);

    extern void
    cwmAnnounceSubmitTransferSuccessForHash (BRCryptoWalletManager cwm,
                                             BRCryptoCWMClientCallbackState callbackState,
                                             const char *hash);

    extern void
    cwmAnnounceSubmitTransferFailure (BRCryptoWalletManager cwm,
                                      BRCryptoCWMClientCallbackState callbackState);

    extern void
    cwmAnnounceGetBalanceSuccess (BRCryptoWalletManager cwm,
                                  BRCryptoCWMClientCallbackState callbackState,
                                  const char *balance);

    extern void
    cwmAnnounceGetBalanceFailure (BRCryptoWalletManager cwm,
                                  BRCryptoCWMClientCallbackState callbackState);

    extern void
    cwmAnnounceGetBlocksSuccess (BRCryptoWalletManager cwm,
                                 BRCryptoCWMClientCallbackState callbackState,
                                 int blockNumbersCount,
                                 uint64_t *blockNumbers);

    extern void
    cwmAnnounceGetBlocksFailure (BRCryptoWalletManager cwm,
                                 BRCryptoCWMClientCallbackState callbackState);

    extern void
    cwmAnnounceGetGasPriceSuccess (BRCryptoWalletManager cwm,
                                   BRCryptoCWMClientCallbackState callbackState,
                                   const char *gasPrice);

    extern void
    cwmAnnounceGetGasPriceFailure (BRCryptoWalletManager cwm,
                                   BRCryptoCWMClientCallbackState callbackState);

    extern void
    cwmAnnounceGetGasEstimateSuccess (BRCryptoWalletManager cwm,
                                      BRCryptoCWMClientCallbackState callbackState,
                                      const char *gasEstimate);

    extern void
    cwmAnnounceGetGasEstimateFailure (BRCryptoWalletManager cwm,
                                      BRCryptoCWMClientCallbackState callbackState);

    extern void
    cwmAnnounceGetLogsItem(BRCryptoWalletManager cwm,
                           BRCryptoCWMClientCallbackState callbackState,
                           const char *strHash,
                           const char *strContract,
                           int topicCount,
                           const char **arrayTopics,
                           const char *strData,
                           const char *strGasPrice,
                           const char *strGasUsed,
                           const char *strLogIndex,
                           const char *strBlockNumber,
                           const char *strBlockTransactionIndex,
                           const char *strBlockTimestamp);

    extern void
    cwmAnnounceGetLogsComplete(BRCryptoWalletManager cwm,
                               BRCryptoCWMClientCallbackState callbackState,
                               BRCryptoBoolean success);

    extern void
    cwmAnnounceGetTokensItem (BRCryptoWalletManager cwm,
                              BRCryptoCWMClientCallbackState callbackState,
                              const char *address,
                              const char *symbol,
                              const char *name,
                              const char *description,
                              unsigned int decimals,
                              const char *strDefaultGasLimit,
                              const char *strDefaultGasPrice);

    extern void
    cwmAnnounceGetTokensComplete (BRCryptoWalletManager cwm,
                                  BRCryptoCWMClientCallbackState callbackState,
                                  BRCryptoBoolean success);

    extern void
    cwmAnnounceGetNonceSuccess (BRCryptoWalletManager cwm,
                                BRCryptoCWMClientCallbackState callbackState,
                                const char *address,
                                const char *nonce);

    extern void
    cwmAnnounceGetNonceFailure (BRCryptoWalletManager cwm,
                                BRCryptoCWMClientCallbackState callbackState);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoWalletManagerClient_h */
