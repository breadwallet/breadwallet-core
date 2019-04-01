//
//  BREthereumEWM
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/5/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_EWM_H
#define BR_Ethereum_EWM_H

#include "ethereum/blockchain/BREthereumNetwork.h"
#include "ethereum/contract/BREthereumContract.h"
#include "BREthereumBase.h"
#include "BREthereumAmount.h"
#include "BREthereumClient.h"

#ifdef __cplusplus
extern "C" {
#endif

/// MARK: - Ethereum Wallet Manager

extern BREthereumEWM
ewmCreate (BREthereumNetwork network,
           BREthereumAccount account,
           BREthereumTimestamp accountTimestamp,
           BREthereumMode mode,
           BREthereumClient client,
           const char *storagePath);

extern BREthereumEWM
ewmCreateWithPaperKey (BREthereumNetwork network,
                       const char *paperKey,
                       BREthereumTimestamp accountTimestamp,
                       BREthereumMode mode,
                       BREthereumClient client,
                       const char *storagePath);

extern BREthereumEWM
ewmCreateWithPublicKey (BREthereumNetwork network,
                        BRKey publicKey,
                        BREthereumTimestamp accountTimestamp,
                        BREthereumMode mode,
                        BREthereumClient client,
                        const char *storagePath);

extern void
ewmDestroy (BREthereumEWM ewm);

extern BREthereumNetwork
ewmGetNetwork (BREthereumEWM ewm);

extern BREthereumAccount
ewmGetAccount (BREthereumEWM ewm);

/**
 * Get the primary address for `account`.  This is the '0x'-prefixed, 40-char, hex encoded
 * string.  The returned char* is newly allocated, on each call - you MUST free() it.
 */
extern char *
ewmGetAccountPrimaryAddress(BREthereumEWM ewm);

/**
 * Get the public key for `account`.  This is a 0x04-prefixed, 65-byte array.  You own this
 * memory and you MUST free() it.
 */
extern BRKey
ewmGetAccountPrimaryAddressPublicKey(BREthereumEWM ewm);

/**
 * Get the private key for `account`.
 */
extern BRKey
ewmGetAccountPrimaryAddressPrivateKey(BREthereumEWM ewm,
                                      const char *paperKey);

/// MARK: - Connect

extern BREthereumBoolean
ewmConnect(BREthereumEWM ewm);

extern BREthereumBoolean
ewmDisconnect (BREthereumEWM ewm);

extern BREthereumBoolean
ewmIsConnected (BREthereumEWM ewm);

extern BREthereumBoolean
ewmSync (BREthereumEWM ewm);

extern void
ewmLock (BREthereumEWM ewm);

extern void
ewmUnlock (BREthereumEWM ewm);

extern uint64_t
ewmGetBlockHeight (BREthereumEWM ewm);

extern void
ewmUpdateBlockHeight(BREthereumEWM ewm,
                     uint64_t blockHeight);

/// MARK: - Wallets

extern BREthereumWallet *
ewmGetWallets (BREthereumEWM ewm);

extern unsigned int
ewmGetWalletsCount (BREthereumEWM ewm);

extern BREthereumWallet
ewmGetWallet(BREthereumEWM ewm);

extern BREthereumWallet
ewmGetWalletHoldingToken(BREthereumEWM ewm,
                         BREthereumToken token);

/// MARK: - Wallet

extern BREthereumToken
ewmWalletGetToken (BREthereumEWM ewm,
                   BREthereumWallet wallet);

extern BREthereumAmount
ewmWalletGetBalance(BREthereumEWM ewm,
                    BREthereumWallet wallet);


extern BREthereumGas
ewmWalletGetGasEstimate(BREthereumEWM ewm,
                        BREthereumWallet wallet,
                        BREthereumTransfer transfer);

extern BREthereumGas
ewmWalletGetDefaultGasLimit(BREthereumEWM ewm,
                            BREthereumWallet wallet);

extern void
ewmWalletSetDefaultGasLimit(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumGas gasLimit);

extern BREthereumGasPrice
ewmWalletGetDefaultGasPrice(BREthereumEWM ewm,
                            BREthereumWallet wallet);

extern void
ewmWalletSetDefaultGasPrice(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumGasPrice gasPrice);


/**
 * Return a newly allocated array of transfers in wallet.  (You own the array and  must call free
 * on it - but not on its elements).
 *
 * @param ewm
 * @param wallet
 *
 * @return array of transfers, will be NULL terminated.
 */
extern BREthereumTransfer *
ewmWalletGetTransfers(BREthereumEWM ewm,
                      BREthereumWallet wallet);

extern int
ewmWalletGetTransferCount(BREthereumEWM ewm,
                          BREthereumWallet wallet);


extern BREthereumTransfer
ewmWalletCreateTransfer(BREthereumEWM ewm,
                        BREthereumWallet wallet,
                        const char *recvAddress,
                        BREthereumAmount amount);

extern BREthereumTransfer
ewmWalletCreateTransferGeneric(BREthereumEWM ewm,
                               BREthereumWallet wallet,
                               const char *recvAddress,
                               BREthereumEther amount,
                               BREthereumGasPrice gasPrice,
                               BREthereumGas gasLimit,
                               const char *data);

extern BREthereumTransfer
ewmWalletCreateTransferWithFeeBasis (BREthereumEWM ewm,
                                     BREthereumWallet wallet,
                                     const char *recvAddress,
                                     BREthereumAmount amount,
                                     BREthereumFeeBasis feeBasis);
extern BREthereumEther
ewmWalletEstimateTransferFee(BREthereumEWM ewm,
                             BREthereumWallet wallet,
                             BREthereumAmount amount,
                             int *overflow);

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
                        BREthereumWallet wid,
                        BREthereumTransfer tid);

extern BREthereumBoolean
ewmWalletCanCancelTransfer (BREthereumEWM ewm,
                            BREthereumWallet wid,
                            BREthereumTransfer tid);

extern BREthereumTransfer // status, error
ewmWalletCreateTransferToCancel(BREthereumEWM ewm,
                                BREthereumWallet wid,
                                BREthereumTransfer tid);

extern BREthereumBoolean
ewmWalletCanReplaceTransfer (BREthereumEWM ewm,
                             BREthereumWallet wid,
                             BREthereumTransfer tid);

extern BREthereumTransfer // status, error
ewmWalletCreateTransferToReplace(BREthereumEWM ewm,
                                 BREthereumWallet wid,
                                 BREthereumTransfer tid,
                                 // ...
                                 BREthereumBoolean updateGasPrice,
                                 BREthereumBoolean updateGasLimit,
                                 BREthereumBoolean updateNonce);

/// MARK: - Transfer

extern void
ewmTransferDelete (BREthereumEWM ewm,
                   BREthereumTransfer transfer);

extern BREthereumAddress
ewmTransferGetTarget (BREthereumEWM ewm,
                      BREthereumTransfer transfer);


extern BREthereumAddress
ewmTransferGetSource (BREthereumEWM ewm,
                      BREthereumTransfer transfer);

extern BREthereumHash
ewmTransferGetIdentifier (BREthereumEWM ewm,
                          BREthereumTransfer transfer);

extern BREthereumHash
ewmTransferGetOriginatingTransactionHash (BREthereumEWM ewm,
                                          BREthereumTransfer transfer);


extern BREthereumAmount
ewmTransferGetAmount(BREthereumEWM ewm,
                     BREthereumTransfer transfer);

extern BREthereumGasPrice
ewmTransferGetGasPrice(BREthereumEWM ewm,
                       BREthereumTransfer transfer,
                       BREthereumEtherUnit unit);

extern BREthereumGas
ewmTransferGetGasLimit(BREthereumEWM ewm,
                       BREthereumTransfer transfer);

extern uint64_t
ewmTransferGetNonce(BREthereumEWM ewm,
                    BREthereumTransfer transfer);


extern BREthereumGas
ewmTransferGetGasUsed(BREthereumEWM ewm,
                      BREthereumTransfer transfer);

extern uint64_t
ewmTransferGetTransactionIndex(BREthereumEWM ewm,
                               BREthereumTransfer transfer);

extern BREthereumHash
ewmTransferGetBlockHash(BREthereumEWM ewm,
                        BREthereumTransfer transfer);

extern uint64_t
ewmTransferGetBlockNumber(BREthereumEWM ewm,
                          BREthereumTransfer transfer);

extern uint64_t
ewmTransferGetBlockTimestamp (BREthereumEWM ewm,
                              BREthereumTransfer transfer);

extern uint64_t
ewmTransferGetBlockConfirmations(BREthereumEWM ewm,
                                 BREthereumTransfer transfer);

extern BREthereumTransferStatus
ewmTransferGetStatus (BREthereumEWM ewm,
                      BREthereumTransfer transfer);

extern BREthereumBoolean
ewmTransferIsConfirmed(BREthereumEWM ewm,
                       BREthereumTransfer transfer);

extern BREthereumBoolean
ewmTransferIsSubmitted(BREthereumEWM ewm,
                       BREthereumTransfer transfer);

extern char *
ewmTransferStatusGetError (BREthereumEWM ewm,
                           BREthereumTransfer transfer);

extern int
ewmTransferStatusGetErrorType (BREthereumEWM ewm,
                               BREthereumTransfer transfer);

extern BREthereumBoolean
ewmTransferHoldsToken(BREthereumEWM ewm,
                      BREthereumTransfer transfer,
                      BREthereumToken token);

extern BREthereumToken
ewmTransferGetToken(BREthereumEWM ewm,
                    BREthereumTransfer transfer);

extern BREthereumEther
ewmTransferGetFee(BREthereumEWM ewm,
                  BREthereumTransfer transfer,
                  int *overflow);

/// MARK: - Amount

extern BREthereumAmount
ewmCreateEtherAmountString(BREthereumEWM ewm,
                           const char *number,
                           BREthereumEtherUnit unit,
                           BRCoreParseStatus *status);

extern BREthereumAmount
ewmCreateEtherAmountUnit(BREthereumEWM ewm,
                         uint64_t amountInUnit,
                         BREthereumEtherUnit unit);

extern BREthereumAmount
ewmCreateTokenAmountString(BREthereumEWM ewm,
                           BREthereumToken token,
                           const char *number,
                           BREthereumTokenQuantityUnit unit,
                           BRCoreParseStatus *status);

extern char *
ewmCoerceEtherAmountToString(BREthereumEWM ewm,
                             BREthereumEther ether,
                             BREthereumEtherUnit unit) ;

extern char *
ewmCoerceTokenAmountToString(BREthereumEWM ewm,
                             BREthereumTokenQuantity token,
                             BREthereumTokenQuantityUnit unit);

/// MARK: - Gas Price / Limit

extern BREthereumGasPrice
ewmCreateGasPrice (uint64_t value,
                   BREthereumEtherUnit unit);

extern BREthereumGas
ewmCreateGas (uint64_t value);

/// MARK: - Block Number

extern void
ethereumClientUpdateBlockNumber (BREthereumEWM ewm);

extern BREthereumStatus
ethereumClientAnnounceBlockNumber (BREthereumEWM ewm,
                                   const char *strBlockNumber,
                                   int rid);

///
//extern void // status, error
//ewmWalletSubmitTransferCancel(BREthereumEWM ewm,
//                              BREthereumWalletId wid,
//                              BREthereumTransfer transfer,
//                              const char *paperKey);
//
//extern void // status, error
//ewmWalletSubmitTransferAgain(BREthereumEWM ewm,
//                             BREthereumWalletId wid,
//                             BREthereumTransfer transfer,
//                             const char *paperKey);

//
// Block
//
extern uint64_t
ewmGetBlockHeight(BREthereumEWM ewm);

extern void
ewmUpdateBlockHeight(BREthereumEWM ewm,
                     uint64_t blockHeight);

extern const char *
ewmTransferGetRawDataHexEncoded(BREthereumEWM ewm,
                                BREthereumWallet wallet,
                                BREthereumTransfer transfer,
                                const char *prefix);


#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_EWM_H
