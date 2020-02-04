//
//  BRCryptoWalletManagerClient.h
//  BRCrypto
//
//  Created by Michael Carrara on 6/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoWalletManagerClient_h
#define BRCryptoWalletManagerClient_h

#include "BRCryptoBase.h"
#include "BRCryptoNetwork.h"
#include "BRCryptoAccount.h"
#include "BRCryptoStatus.h"
#include "BRCryptoTransfer.h"
#include "BRCryptoWallet.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *BRCryptoClientContext;

typedef struct BRCryptoClientCallbackStateRecord *BRCryptoClientCallbackState;

// MARK: - Get Balance

typedef void
(*BRCryptoClientGetBalanceCallback) (BRCryptoClientContext context,
                                     OwnershipGiven BRCryptoWalletManager manager,
                                     OwnershipGiven BRCryptoClientCallbackState callbackState,
                                     OwnershipKept const char **addresses,
                                     size_t addressesCount,
                                     OwnershipKept const char *issuer); // NULLABLE

extern void
cwmAnnounceGetBalanceSuccess (OwnershipKept BRCryptoWalletManager cwm,
                              OwnershipGiven BRCryptoClientCallbackState callbackState,
                              const char *balance);

extern void
cwmAnnounceGetBalanceFailure (OwnershipKept BRCryptoWalletManager cwm,
                              OwnershipGiven BRCryptoClientCallbackState callbackState);


// MARK: - Get Block Number

typedef void
(*BRCryptoClientGetBlockNumberCallback) (BRCryptoClientContext context,
                                         OwnershipGiven BRCryptoWalletManager manager,
                                         OwnershipGiven BRCryptoClientCallbackState callbackState);

extern void
cwmAnnounceGetBlockNumberSuccessAsInteger (OwnershipKept BRCryptoWalletManager cwm,
                                           OwnershipGiven BRCryptoClientCallbackState callbackState,
                                           uint64_t blockNumber);

extern void
cwmAnnounceGetBlockNumberSuccessAsString (OwnershipKept BRCryptoWalletManager cwm,
                                          OwnershipGiven BRCryptoClientCallbackState callbackState,
                                          OwnershipKept const char *blockNumber);

extern void
cwmAnnounceGetBlockNumberFailure (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoClientCallbackState callbackState);

// MARK: - Get Transactions

typedef void
(*BRCryptoClientGetTransactionsCallback) (BRCryptoClientContext context,
                                          OwnershipGiven BRCryptoWalletManager manager,
                                          OwnershipGiven BRCryptoClientCallbackState callbackState,
                                          OwnershipKept const char **addresses,
                                          size_t addressCount,
                                          OwnershipKept const char *currency,
                                          uint64_t begBlockNumber,
                                          uint64_t endBlockNumber);

extern void
cwmAnnounceGetTransactionsItem (OwnershipKept BRCryptoWalletManager cwm,
                                OwnershipGiven BRCryptoClientCallbackState callbackState,
                                BRCryptoTransferStateType status,
                                OwnershipKept uint8_t *transaction,
                                size_t transactionLength,
                                uint64_t timestamp,
                                uint64_t blockHeight);

extern void
cwmAnnounceGetTransactionsComplete (OwnershipKept BRCryptoWalletManager cwm,
                                    OwnershipGiven BRCryptoClientCallbackState callbackState,
                                    BRCryptoBoolean success);

// MARK: - Get Transfers

typedef void
(*BRCryptoClientGetTransfersCallback) (BRCryptoClientContext context,
                                       OwnershipGiven BRCryptoWalletManager manager,
                                       OwnershipGiven BRCryptoClientCallbackState callbackState,
                                       OwnershipKept const char **addresses,
                                       size_t addressCount,
                                       OwnershipKept const char *currency,
                                       uint64_t begBlockNumber,
                                       uint64_t endBlockNumber);

extern void
cwmAnnounceGetTransferItemGEN (BRCryptoWalletManager cwm,
                               BRCryptoClientCallbackState callbackState,
                               BRCryptoTransferStateType status,
                               OwnershipKept const char *hash,
                               OwnershipKept const char *uids,
                               OwnershipKept const char *from,
                               OwnershipKept const char *to,
                               OwnershipKept const char *amount,
                               OwnershipKept const char *currency,
                               OwnershipKept const char *fee,
                               uint64_t timestamp,
                               uint64_t blockHeight,
                               size_t attributesCount,
                               OwnershipKept const char **attributeKeys,
                               OwnershipKept const char **attributeVals);

extern void
cwmAnnounceGetTransactionsItemETH (OwnershipKept BRCryptoWalletManager cwm,
                                   OwnershipGiven BRCryptoClientCallbackState callbackState,
                                   OwnershipKept const char *hash,
                                   OwnershipKept const char *from,
                                   OwnershipKept const char *to,
                                   OwnershipKept const char *contract,
                                   OwnershipKept const char *amount, // value
                                   OwnershipKept const char *gasLimit,
                                   OwnershipKept const char *gasPrice,
                                   OwnershipKept const char *data,
                                   OwnershipKept const char *nonce,
                                   OwnershipKept const char *gasUsed,
                                   OwnershipKept const char *blockNumber,
                                   OwnershipKept const char *blockHash,
                                   OwnershipKept const char *blockConfirmations,
                                   OwnershipKept const char *blockTransactionIndex,
                                   OwnershipKept const char *blockTimestamp,
                                   // cumulative gas used,
                                   // confirmations
                                   // txreceipt_status
                                   OwnershipKept const char *isError);

extern void
cwmAnnounceGetLogsItemETH (OwnershipKept BRCryptoWalletManager cwm,
                           OwnershipGiven BRCryptoClientCallbackState callbackState,
                           OwnershipKept const char *strHash,
                           OwnershipKept const char *strContract,
                           int topicCount,
                           OwnershipKept const char **arrayTopics,
                           OwnershipKept const char *strData,
                           OwnershipKept const char *strGasPrice,
                           OwnershipKept const char *strGasUsed,
                           OwnershipKept const char *strLogIndex,
                           OwnershipKept const char *strBlockNumber,
                           OwnershipKept const char *strBlockTransactionIndex,
                           OwnershipKept const char *strBlockTimestamp);

extern void
cwmAnnounceGetTransfersComplete (OwnershipKept BRCryptoWalletManager cwm,
                                 OwnershipGiven BRCryptoClientCallbackState callbackState,
                                 BRCryptoBoolean success);

extern void
cwmAnnounceSubmitTransferSuccess (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoClientCallbackState callbackState);

extern void
cwmAnnounceGetLogsComplete(OwnershipKept BRCryptoWalletManager cwm,
                           OwnershipGiven BRCryptoClientCallbackState callbackState,
                           BRCryptoBoolean success);

// MARK: - Submit Transaction

typedef void
(*BRCryptoClientSubmitTransactionCallback) (BRCryptoClientContext context,
                                            OwnershipGiven BRCryptoWalletManager manager,
                                            OwnershipGiven BRCryptoClientCallbackState callbackState,
                                            OwnershipKept const uint8_t *transaction,
                                            size_t transactionLength,
                                            OwnershipKept const char *hashAsHex);

extern void
cwmAnnounceSubmitTransferSuccessForHash (OwnershipKept BRCryptoWalletManager cwm,
                                         OwnershipGiven BRCryptoClientCallbackState callbackState,
                                         const char *hash);

extern void
cwmAnnounceSubmitTransferFailure (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoClientCallbackState callbackState);

// MARK: - (ETH) Get Gas Price

typedef void
(*BRCryptoClientETHGetGasPriceCallback) (BRCryptoClientContext context,
                                         OwnershipGiven BRCryptoWalletManager manager,
                                         OwnershipGiven BRCryptoClientCallbackState callbackState,
                                         OwnershipKept const char *network);

extern void
cwmAnnounceGetGasPriceSuccess (OwnershipKept BRCryptoWalletManager cwm,
                               OwnershipGiven BRCryptoClientCallbackState callbackState,
                               const char *gasPrice);

extern void
cwmAnnounceGetGasPriceFailure (OwnershipKept BRCryptoWalletManager cwm,
                               OwnershipGiven BRCryptoClientCallbackState callbackState);

// MARK: - (ETH) Get Gas Estimate

typedef void
(*BRCryptoClientETHEstimateGasCallback) (BRCryptoClientContext context,
                                         OwnershipGiven BRCryptoWalletManager manager,
                                         OwnershipGiven BRCryptoClientCallbackState callbackState,
                                         OwnershipKept const char *network,
                                         OwnershipKept const char *from,
                                         OwnershipKept const char *to,
                                         OwnershipKept const char *amount,
                                         OwnershipKept const char *price,
                                         OwnershipKept const char *data);
extern void
cwmAnnounceGetGasEstimateSuccess (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoClientCallbackState callbackState,
                                  const char *gasEstimate,
                                  const char *gasPrice);

extern void
cwmAnnounceGetGasEstimateFailure (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoClientCallbackState callbackState,
                                  BRCryptoStatus status);

// MARK: - (ETH) Get Blocks

typedef void
(*BRCryptoClientETHGetBlocksCallback) (BRCryptoClientContext context,
                                       OwnershipGiven BRCryptoWalletManager manager,
                                       OwnershipGiven BRCryptoClientCallbackState callbackState,
                                       OwnershipKept const char *network,
                                       OwnershipKept const char *address, // disappears immediately
                                       unsigned int interests,
                                       uint64_t blockNumberStart,
                                       uint64_t blockNumberStop);
extern void
cwmAnnounceGetBlocksSuccess (OwnershipKept BRCryptoWalletManager cwm,
                             OwnershipGiven BRCryptoClientCallbackState callbackState,
                             int blockNumbersCount,
                             uint64_t *blockNumbers);

extern void
cwmAnnounceGetBlocksFailure (OwnershipKept BRCryptoWalletManager cwm,
                             OwnershipGiven BRCryptoClientCallbackState callbackState);

// MARK: - (ETH) Get Tokens

typedef void
(*BRCryptoClientETHGetTokensCallback) (BRCryptoClientContext context,
                                       OwnershipGiven BRCryptoWalletManager manager,
                                       OwnershipGiven BRCryptoClientCallbackState callbackState);
extern void
cwmAnnounceGetTokensItem (OwnershipKept BRCryptoWalletManager cwm,
                          OwnershipGiven BRCryptoClientCallbackState callbackState,
                          OwnershipKept const char *address,
                          OwnershipKept const char *symbol,
                          OwnershipKept const char *name,
                          OwnershipKept const char *description,
                          unsigned int decimals,
                          OwnershipKept const char *strDefaultGasLimit,
                          OwnershipKept const char *strDefaultGasPrice);

extern void
cwmAnnounceGetTokensComplete (OwnershipKept BRCryptoWalletManager cwm,
                              OwnershipGiven BRCryptoClientCallbackState callbackState,
                              BRCryptoBoolean success);

// MARK: - (ETH) Get Block Number

typedef void
(*BRCryptoClientETHGetBlockNumberCallback) (BRCryptoClientContext context,
                                            OwnershipGiven BRCryptoWalletManager manager,
                                            OwnershipGiven BRCryptoClientCallbackState callbackState,
                                            OwnershipKept const char *network);


// MARK: (ETH) Get Nonce

typedef void
(*BRCryptoClientETHGetNonceCallback) (BRCryptoClientContext context,
                                      OwnershipGiven BRCryptoWalletManager manager,
                                      OwnershipGiven BRCryptoClientCallbackState callbackState,
                                      OwnershipKept const char *network,
                                      OwnershipKept const char *address);

extern void
cwmAnnounceGetNonceSuccess (OwnershipKept BRCryptoWalletManager cwm,
                            OwnershipGiven BRCryptoClientCallbackState callbackState,
                            OwnershipKept const char *address,
                            OwnershipKept const char *nonce);

extern void
cwmAnnounceGetNonceFailure (BRCryptoWalletManager cwm,
                            OwnershipGiven BRCryptoClientCallbackState callbackState);

typedef struct {
} BRCryptoCWMClientETH;


typedef struct {
    BRCryptoClientContext context;
    BRCryptoClientGetBalanceCallback funcGetBalance;
    BRCryptoClientGetBlockNumberCallback  funcGetBlockNumber;
    BRCryptoClientGetTransactionsCallback funcGetTransactions;
    BRCryptoClientGetTransfersCallback funcGetTransfers;
    BRCryptoClientSubmitTransactionCallback funcSubmitTransaction;

    BRCryptoClientETHGetGasPriceCallback funcGetGasPriceETH;
    BRCryptoClientETHEstimateGasCallback funcEstimateGasETH;
    BRCryptoClientETHGetBlocksCallback funcGetBlocksETH;
    BRCryptoClientETHGetTokensCallback funcGetTokensETH; // announce one-by-one
    BRCryptoClientETHGetNonceCallback funcGetNonceETH;
} BRCryptoClient;

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoWalletManagerClient_h */
