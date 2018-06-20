//
//  BREthereum
//  breadwallet-core Ethereum
//
//  Created by ebg on 4/17/18.
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

#include <string.h>
#include <assert.h>
#include "BREthereum.h"
#include "blockchain/BREthereumTransaction.h"
#include "blockchain/BREthereumBlock.h"
#include "ewm/BREthereumEWM.h"
#include "BREthereumWallet.h"

//
//
//
extern BREthereumEWM
ethereumCreate(BREthereumNetwork network,
               const char *paperKey,
               BREthereumType type,
               BREthereumSyncMode syncMode) {
    return createEWM (network, createAccount(paperKey), type, syncMode);
}

extern BREthereumEWM
ethereumCreateWithPublicKey(BREthereumNetwork network,
                            const BRKey publicKey,      // 65 byte, 0x04-prefixed, uncompressed public key
                            BREthereumType type,
                            BREthereumSyncMode syncMode) {
    return createEWM (network, createAccountWithPublicKey (publicKey), type, syncMode);
}

extern BREthereumBoolean
ethereumConnect(BREthereumEWM ewm,
                BREthereumClient client) {
    return ewmConnect(ewm, client);
}

extern BREthereumBoolean
ethereumDisconnect (BREthereumEWM ewm) {
    return ewmDisconnect(ewm);
}

extern void
ethereumDestroy (BREthereumEWM ewm) {
    ewmDestroy(ewm);
}

extern BREthereumAccountId
ethereumGetAccount(BREthereumEWM ewm) {
    return 0;
}

extern char *
ethereumGetAccountPrimaryAddress(BREthereumEWM ewm) {
    return accountGetPrimaryAddressString(ewmGetAccount(ewm));
}

extern BRKey // key.pubKey
ethereumGetAccountPrimaryAddressPublicKey(BREthereumEWM ewm) {
    return accountGetPrimaryAddressPublicKey(ewmGetAccount(ewm));
}

extern BRKey
ethereumGetAccountPrimaryAddressPrivateKey(BREthereumEWM ewm,
                                           const char *paperKey) {
    return accountGetPrimaryAddressPrivateKey (ewmGetAccount(ewm), paperKey);
    
}

extern BREthereumNetwork
ethereumGetNetwork (BREthereumEWM ewm) {
    return ewmGetNetwork(ewm);
}


extern BREthereumWalletId
ethereumGetWallet(BREthereumEWM ewm) {
    return ewmGetWallet(ewm);
}

extern BREthereumWalletId
ethereumGetWalletHoldingToken(BREthereumEWM ewm,
                              BREthereumToken token) {
    return ewmGetWalletHoldingToken(ewm, token);
}

extern uint64_t
ethereumWalletGetDefaultGasLimit(BREthereumEWM ewm,
                                 BREthereumWalletId wid) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    return walletGetDefaultGasLimit(wallet).amountOfGas;
}

extern void
ethereumWalletSetDefaultGasLimit(BREthereumEWM ewm,
                                 BREthereumWalletId wid,
                                 uint64_t gasLimit) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    ewmWalletSetDefaultGasLimit(ewm, wallet, gasCreate(gasLimit));
}

extern uint64_t
ethereumWalletGetGasEstimate(BREthereumEWM ewm,
                             BREthereumWalletId wid,
                             BREthereumTransactionId tid) {
    //  BREthereumWallet wallet = lightewmLookupWallet(ewm, wid);
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    return transactionGetGasEstimate(transaction).amountOfGas;
}

extern void
ethereumWalletSetDefaultGasPrice(BREthereumEWM ewm,
                                 BREthereumWalletId wid,
                                 BREthereumEtherUnit unit,
                                 uint64_t value) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    ewmWalletSetDefaultGasPrice (ewm, wallet, gasPriceCreate(etherCreateNumber (value, unit)));
}

extern uint64_t
ethereumWalletGetDefaultGasPrice(BREthereumEWM ewm,
                                 BREthereumWalletId wid) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    BREthereumGasPrice gasPrice = walletGetDefaultGasPrice(wallet);
    return (gtUInt256 (gasPrice.etherPerGas.valueInWEI, createUInt256(UINT64_MAX))
            ? 0
            : gasPrice.etherPerGas.valueInWEI.u64[0]);
}

extern BREthereumAmount
ethereumWalletGetBalance(BREthereumEWM ewm,
                         BREthereumWalletId wid) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    return walletGetBalance(wallet);
}

extern char *
ethereumWalletGetBalanceEther(BREthereumEWM ewm,
                              BREthereumWalletId wid,
                              BREthereumEtherUnit unit) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    BREthereumAmount balance = walletGetBalance(wallet);
    return (AMOUNT_ETHER == amountGetType(balance)
            ? etherGetValueString(balance.u.ether, unit)
            : NULL);
}

extern char *
ethereumWalletGetBalanceTokenQuantity(BREthereumEWM ewm,
                                      BREthereumWalletId wid,
                                      BREthereumTokenQuantityUnit unit) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    BREthereumAmount balance = walletGetBalance(wallet);
    return (AMOUNT_TOKEN == amountGetType(balance)
            ? tokenQuantityGetValueString(balance.u.tokenQuantity, unit)
            : NULL);
}

extern BREthereumEther
ethereumWalletEstimateTransactionFee(BREthereumEWM ewm,
                                     BREthereumWalletId wid,
                                     BREthereumAmount amount,
                                     int *overflow) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    return walletEstimateTransactionFee(wallet, amount, overflow);
}

extern BREthereumTransactionId
ethereumWalletCreateTransaction(BREthereumEWM ewm,
                                BREthereumWalletId wid,
                                const char *recvAddress,
                                BREthereumAmount amount) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    return ewmWalletCreateTransaction(ewm, wallet, recvAddress, amount);
}

extern void // status, error
ethereumWalletSignTransaction(BREthereumEWM ewm,
                              BREthereumWalletId wid,
                              BREthereumTransactionId tid,
                              const char *paperKey) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    ewmWalletSignTransactionWithPaperKey(ewm, wallet, transaction, paperKey);
}

extern void // status, error
ethereumWalletSignTransactionWithPrivateKey(BREthereumEWM ewm,
                                            BREthereumWalletId wid,
                                            BREthereumTransactionId tid,
                                            BRKey privateKey) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    ewmWalletSignTransaction(ewm, wallet, transaction, privateKey);
}

extern void // status, error
ethereumWalletSubmitTransaction(BREthereumEWM ewm,
                                BREthereumWalletId wid,
                                BREthereumTransactionId tid) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    ewmWalletSubmitTransaction(ewm, wallet, transaction);
}

//
//
//
extern BREthereumTransactionId *
ethereumWalletGetTransactions(BREthereumEWM ewm,
                              BREthereumWalletId wid) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    return ewmWalletGetTransactions(ewm, wallet);
}

extern int
ethereumWalletGetTransactionCount(BREthereumEWM ewm,
                                  BREthereumWalletId wid) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    return ewmWalletGetTransactionCount(ewm, wallet);
}

extern BREthereumBoolean
ethereumWalletHoldsToken(BREthereumEWM ewm,
                         BREthereumWalletId wid,
                         BREthereumToken token) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    return (NULL != wallet && token == walletGetToken(wallet)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern BREthereumToken
ethereumWalletGetToken(BREthereumEWM ewm,
                       BREthereumWalletId wid) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    return (NULL != wallet
            ? walletGetToken(wallet)
            : NULL);
}

//
// Block
//
extern uint64_t
ethereumGetBlockHeight (BREthereumEWM ewm) {
    return ewmGetBlockHeight(ewm);
}


extern uint64_t
ethereumBlockGetNumber (BREthereumEWM ewm,
                        BREthereumBlockId bid) {
    BREthereumBlock block = ewmLookupBlock(ewm, bid);
    return blockGetNumber(block);
}

extern uint64_t
ethereumBlockGetTimestamp (BREthereumEWM ewm,
                           BREthereumBlockId bid) {
    BREthereumBlock block = ewmLookupBlock(ewm, bid);
    return blockGetTimestamp(block);
}

extern char *
ethereumBlockGetHash (BREthereumEWM ewm,
                      BREthereumBlockId bid) {
    BREthereumBlock block = ewmLookupBlock(ewm, bid);
    return hashAsString (blockGetHash(block));
}

//
// Transaction
//
extern char *
ethereumTransactionGetRecvAddress(BREthereumEWM ewm,
                                  BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    return addressGetEncodedString(transactionGetTargetAddress(transaction), 1);
}

extern char * // sender, source
ethereumTransactionGetSendAddress(BREthereumEWM ewm,
                                  BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    return addressGetEncodedString(transactionGetSourceAddress(transaction), 1);
}

extern char *
ethereumTransactionGetHash(BREthereumEWM ewm,
                           BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    return hashAsString (transactionGetHash(transaction));
}

extern char *
ethereumTransactionGetAmountEther(BREthereumEWM ewm,
                                  BREthereumTransactionId tid,
                                  BREthereumEtherUnit unit) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    BREthereumAmount amount = transactionGetAmount(transaction);
    return (AMOUNT_ETHER == amountGetType(amount)
            ? etherGetValueString(amountGetEther(amount), unit)
            : "");
}

extern char *
ethereumTransactionGetAmountTokenQuantity(BREthereumEWM ewm,
                                          BREthereumTransactionId tid,
                                          BREthereumTokenQuantityUnit unit) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    BREthereumAmount amount = transactionGetAmount(transaction);
    return (AMOUNT_TOKEN == amountGetType(amount)
            ? tokenQuantityGetValueString(amountGetTokenQuantity(amount), unit)
            : "");
}

extern BREthereumAmount
ethereumTransactionGetAmount(BREthereumEWM ewm,
                             BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    return transactionGetAmount(transaction);
}

extern BREthereumAmount
ethereumTransactionGetGasPriceToo(BREthereumEWM ewm,
                                  BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    BREthereumGasPrice gasPrice = transactionGetGasPrice(transaction);
    return amountCreateEther (gasPrice.etherPerGas);
}

extern char *
ethereumTransactionGetGasPrice(BREthereumEWM ewm,
                               BREthereumTransactionId tid,
                               BREthereumEtherUnit unit) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    BREthereumGasPrice gasPrice = transactionGetGasPrice(transaction);
    return etherGetValueString(gasPrice.etherPerGas, unit);
}

extern uint64_t
ethereumTransactionGetGasLimit(BREthereumEWM ewm,
                               BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    return transactionGetGasLimit(transaction).amountOfGas;
}

extern uint64_t
ethereumTransactionGetGasUsed(BREthereumEWM ewm,
                              BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    BREthereumGas gasUsed;
    return (transactionExtractIncluded(transaction, &gasUsed, NULL, NULL, NULL)
            ? gasUsed.amountOfGas
            : 0);
}

extern uint64_t
ethereumTransactionGetNonce(BREthereumEWM ewm,
                            BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    return transactionGetNonce(transaction);
}

extern BREthereumHash
ethereumTransactionGetBlockHash(BREthereumEWM ewm,
                                BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    BREthereumHash blockHash;
    return (transactionExtractIncluded(transaction, NULL, &blockHash, NULL, NULL)
            ? blockHash
            : hashCreateEmpty());
}

extern uint64_t
ethereumTransactionGetBlockNumber(BREthereumEWM ewm,
                                  BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    uint64_t blockNumber;
    return (transactionExtractIncluded(transaction, NULL, NULL, &blockNumber, NULL)
            ? blockNumber
            : 0);
}

extern uint64_t
ethereumTransactionGetBlockConfirmations(BREthereumEWM ewm,
                                         BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    
    uint64_t blockNumber = 0;
    return (transactionExtractIncluded(transaction, NULL, NULL, &blockNumber, NULL)
            ? (ewmGetBlockHeight(ewm) - blockNumber)
            : 0);
}

extern BREthereumBoolean
ethereumTransactionIsConfirmed(BREthereumEWM ewm,
                               BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    return transactionIsConfirmed(transaction);
}

extern BREthereumBoolean
ethereumTransactionIsSubmitted(BREthereumEWM ewm,
                               BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    return transactionIsSubmitted(transaction);
}

extern BREthereumBoolean
ethereumTransactionHoldsToken(BREthereumEWM ewm,
                              BREthereumTransactionId tid,
                              BREthereumToken token) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    assert (NULL != transaction);
    return (token == transactionGetToken(transaction)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern BREthereumToken
ethereumTransactionGetToken(BREthereumEWM ewm,
                            BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    assert (NULL != transaction);
    return transactionGetToken(transaction);
}

extern BREthereumEther
ethereumTransactionGetFee(BREthereumEWM ewm,
                          BREthereumTransactionId tid,
                          int *overflow) {
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, tid);
    assert (NULL != transaction);
    return transactionGetFee(transaction, overflow);
}

//
// Amount
//
extern BREthereumAmount
ethereumCreateEtherAmountString(BREthereumEWM ewm,
                                const char *number,
                                BREthereumEtherUnit unit,
                                BRCoreParseStatus *status) {
    return amountCreateEther (etherCreateString(number, unit, status));
}

extern BREthereumAmount
ethereumCreateEtherAmountUnit(BREthereumEWM ewm,
                              uint64_t amountInUnit,
                              BREthereumEtherUnit unit) {
    return amountCreateEther (etherCreateNumber(amountInUnit, unit));
}

extern BREthereumAmount
ethereumCreateTokenAmountString(BREthereumEWM ewm,
                                BREthereumToken token,
                                const char *number,
                                BREthereumTokenQuantityUnit unit,
                                BRCoreParseStatus *status) {
    return amountCreateTokenQuantityString(token, number, unit, status);
}

extern char *
ethereumCoerceEtherAmountToString(BREthereumEWM ewm,
                                  BREthereumEther ether,
                                  BREthereumEtherUnit unit) {
    return etherGetValueString(ether, unit);
}

extern char *
ethereumCoerceTokenAmountToString(BREthereumEWM ewm,
                                  BREthereumTokenQuantity token,
                                  BREthereumTokenQuantityUnit unit) {
    return tokenQuantityGetValueString(token, unit);
}
