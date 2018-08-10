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
#include "BREthereumWallet.h"
#include "BREthereumTransfer.h"
#include "blockchain/BREthereumBlock.h"
#include "ewm/BREthereumEWMPrivate.h" // ewmClientSignal*

//
//
//
extern BREthereumEWM
ethereumCreate(BREthereumNetwork network,
               const char *paperKey,
               BREthereumType type,
               BREthereumSyncMode syncMode,
               BREthereumClient client,
               BRArrayOf(BREthereumPersistData) peers,
               BRArrayOf(BREthereumPersistData) blocks,
               BRArrayOf(BREthereumPersistData) transactions,
               BRArrayOf(BREthereumPersistData) logs) {
    return createEWM (network, createAccount(paperKey), type, syncMode, client,
                      peers,
                      blocks,
                      transactions,
                      logs);
}

extern BREthereumEWM
ethereumCreateWithPublicKey(BREthereumNetwork network,
                            const BRKey publicKey,      // 65 byte, 0x04-prefixed, uncompressed public key
                            BREthereumType type,
                            BREthereumSyncMode syncMode,
                            BREthereumClient client,
                            BRArrayOf(BREthereumPersistData) peers,
                            BRArrayOf(BREthereumPersistData) blocks,
                            BRArrayOf(BREthereumPersistData) transactions,
                            BRArrayOf(BREthereumPersistData) logs) {
    return createEWM (network, createAccountWithPublicKey (publicKey), type, syncMode, client,
                      peers,
                      blocks,
                      transactions,
                      logs);
}

extern BREthereumBoolean
ethereumConnect(BREthereumEWM ewm) {
    return ewmConnect(ewm);
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

extern BREthereumWalletId *
ethereumGetWallets(BREthereumEWM ewm) {
    return ewmGetWallets (ewm);
}

extern unsigned int
ethereumGetWalletsCount (BREthereumEWM ewm) {
    return ewmGetWalletsCount (ewm);
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
                             BREthereumTransferId tid) {
    //  BREthereumWallet wallet = lightewmLookupWallet(ewm, wid);
    BREthereumTransfer transfer = ewmLookupTransfer(ewm, tid);
    return transferGetGasEstimate(transfer).amountOfGas;
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
ethereumWalletEstimateTransferFee(BREthereumEWM ewm,
                                     BREthereumWalletId wid,
                                     BREthereumAmount amount,
                                     int *overflow) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    return walletEstimateTransferFee(wallet, amount, overflow);
}

extern BREthereumTransferId
ethereumWalletCreateTransfer(BREthereumEWM ewm,
                                BREthereumWalletId wid,
                                const char *recvAddress,
                                BREthereumAmount amount) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    return ewmWalletCreateTransfer(ewm, wallet, recvAddress, amount);
}

extern void // status, error
ethereumWalletSignTransfer(BREthereumEWM ewm,
                              BREthereumWalletId wid,
                              BREthereumTransferId tid,
                              const char *paperKey) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    ewmWalletSignTransferWithPaperKey(ewm, wallet, transaction, paperKey);
}

extern void // status, error
ethereumWalletSignTransferWithPrivateKey(BREthereumEWM ewm,
                                            BREthereumWalletId wid,
                                            BREthereumTransferId tid,
                                            BRKey privateKey) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    ewmWalletSignTransfer(ewm, wallet, transaction, privateKey);
}

extern void // status, error
ethereumWalletSubmitTransfer(BREthereumEWM ewm,
                                BREthereumWalletId wid,
                                BREthereumTransferId tid) {
    ewmWalletSubmitTransfer(ewm, wid, tid);
}

//
//
//
extern BREthereumTransferId *
ethereumWalletGetTransfers(BREthereumEWM ewm,
                              BREthereumWalletId wid) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    return ewmWalletGetTransfers(ewm, wallet);
}

extern int
ethereumWalletGetTransferCount(BREthereumEWM ewm,
                                  BREthereumWalletId wid) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    return ewmWalletGetTransferCount(ewm, wallet);
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
// Transfer
//
extern char *
ethereumTransferGetRecvAddress(BREthereumEWM ewm,
                                  BREthereumTransferId tid) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    return addressGetEncodedString(transferGetTargetAddress(transaction), 1);
}

extern char * // sender, source
ethereumTransferGetSendAddress(BREthereumEWM ewm,
                                  BREthereumTransferId tid) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    return addressGetEncodedString(transferGetSourceAddress(transaction), 1);
}

extern char *
ethereumTransferGetHash(BREthereumEWM ewm,
                           BREthereumTransferId tid) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    return hashAsString (transferGetHash(transaction));
}

extern char *
ethereumTransferGetAmountEther(BREthereumEWM ewm,
                                  BREthereumTransferId tid,
                                  BREthereumEtherUnit unit) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    BREthereumAmount amount = transferGetAmount(transaction);
    return (AMOUNT_ETHER == amountGetType(amount)
            ? etherGetValueString(amountGetEther(amount), unit)
            : "");
}

extern char *
ethereumTransferGetAmountTokenQuantity(BREthereumEWM ewm,
                                          BREthereumTransferId tid,
                                          BREthereumTokenQuantityUnit unit) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    BREthereumAmount amount = transferGetAmount(transaction);
    return (AMOUNT_TOKEN == amountGetType(amount)
            ? tokenQuantityGetValueString(amountGetTokenQuantity(amount), unit)
            : "");
}

extern BREthereumAmount
ethereumTransferGetAmount(BREthereumEWM ewm,
                             BREthereumTransferId tid) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    return transferGetAmount(transaction);
}

//extern BREthereumAmount
//ethereumTransferGetGasPriceToo(BREthereumEWM ewm,
//                                  BREthereumTransferId tid) {
//    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
//    BREthereumGasPrice gasPrice = transferGetGasPrice(transaction);
//    return amountCreateEther (gasPrice.etherPerGas);
//}

extern char *
ethereumTransferGetGasPrice(BREthereumEWM ewm,
                               BREthereumTransferId tid,
                               BREthereumEtherUnit unit) {
    BREthereumTransfer transfer = ewmLookupTransfer(ewm, tid);
    BREthereumFeeBasis feeBasis = transferGetFeeBasis(transfer);
    return etherGetValueString(feeBasisGetGasPrice(feeBasis).etherPerGas, unit);
}

extern uint64_t
ethereumTransferGetGasLimit(BREthereumEWM ewm,
                               BREthereumTransferId tid) {
    BREthereumTransfer transfer = ewmLookupTransfer(ewm, tid);
    BREthereumFeeBasis feeBasis = transferGetFeeBasis(transfer);
    return feeBasisGetGasLimit(feeBasis).amountOfGas;
}

extern uint64_t
ethereumTransferGetNonce(BREthereumEWM ewm,
                         BREthereumTransferId tid) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    return transferGetNonce(transaction);
}

#if 0
extern uint64_t
ethereumTransferGetGasUsed(BREthereumEWM ewm,
                              BREthereumTransferId tid) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    BREthereumTransferStatus status = transferGetStatus(transaction);
    BREthereumGas gasUsed;
    return (transactionStatusExtractIncluded(&status, &gasUsed, NULL, NULL, NULL)
            ? gasUsed.amountOfGas
            : 0);
}

extern BREthereumHash
ethereumTransferGetBlockHash(BREthereumEWM ewm,
                                BREthereumTransferId tid) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    BREthereumTransferStatus status = transferGetStatus(transaction);
    BREthereumHash blockHash;
    return (transactionStatusExtractIncluded(&status, NULL, &blockHash, NULL, NULL)
            ? blockHash
            : hashCreateEmpty());
}

extern uint64_t
ethereumTransferGetBlockNumber(BREthereumEWM ewm,
                                  BREthereumTransferId tid) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    BREthereumTransferStatus status = transferGetStatus(transaction);
    uint64_t blockNumber;
    return (transactionStatusExtractIncluded(&status, NULL, NULL, &blockNumber, NULL)
            ? blockNumber
            : 0);
}

extern uint64_t
ethereumTransferGetBlockConfirmations(BREthereumEWM ewm,
                                         BREthereumTransferId tid) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    BREthereumTransferStatus status = transferGetStatus(transaction);
    uint64_t blockNumber = 0;
    return (transactionStatusExtractIncluded(&status, NULL, NULL, &blockNumber, NULL)
            ? (ewmGetBlockHeight(ewm) - blockNumber)
            : 0);
}
#endif
extern BREthereumBoolean
ethereumTransferIsConfirmed(BREthereumEWM ewm,
                               BREthereumTransferId tid) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    return transferHasStatusType (transaction, TRANSFER_STATUS_INCLUDED);
}

extern BREthereumBoolean
ethereumTransferIsSubmitted(BREthereumEWM ewm,
                               BREthereumTransferId tid) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    return AS_ETHEREUM_BOOLEAN(ETHEREUM_BOOLEAN_IS_TRUE(transferHasStatusType(transaction, TRANSFER_STATUS_SUBMITTED)) ||
                               ETHEREUM_BOOLEAN_IS_TRUE(transferHasStatusTypeOrTwo(transaction,
                                                                                   TRANSFER_STATUS_INCLUDED,
                                                                                   TRANSFER_STATUS_ERRORED)));
}

extern BREthereumBoolean
ethereumTransferHoldsToken(BREthereumEWM ewm,
                              BREthereumTransferId tid,
                              BREthereumToken token) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    assert (NULL != transaction);
    return (token == transferGetToken(transaction)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern BREthereumToken
ethereumTransferGetToken(BREthereumEWM ewm,
                            BREthereumTransferId tid) {
    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    assert (NULL != transaction);
    return transferGetToken(transaction);
}

extern BREthereumEther
ethereumTransferGetFee(BREthereumEWM ewm,
                          BREthereumTransferId tid,
                          int *overflow) {
    BREthereumTransfer transfer = ewmLookupTransfer(ewm, tid);
    assert (NULL != transfer);
    return transferGetFee(transfer, overflow);
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

//
// Announce - Called by Client to announce BRD endpoint data.
//
// We parse the strings to get valid values and then pass it own to EWM.

//
// Block Number
//
extern void
ethereumClientUpdateBlockNumber (BREthereumEWM ewm) {
    ewmUpdateBlockNumber(ewm);
}
extern BREthereumStatus
ethereumClientAnnounceBlockNumber (BREthereumEWM ewm,
                                   const char *strBlockNumber,
                                   int rid) {
    uint64_t blockNumber = strtoull(strBlockNumber, NULL, 0);
    ewmClientSignalAnnounceBlockNumber (ewm, blockNumber, rid);
    return SUCCESS;
}

//
// Nonce
//
extern void
ethereumUpdateNonce (BREthereumEWM ewm) {
    ewmUpdateNonce(ewm);
}

extern BREthereumStatus
ethereumClientAnnounceNonce (BREthereumEWM ewm,
                             const char *strAddress,
                             const char *strNonce,
                             int rid) {
    BREthereumAddress address = addressCreate(strAddress);
    uint64_t nonce = strtoull (strNonce, NULL, 0);
    ewmClientSignalAnnounceNonce(ewm, address, nonce, rid);
    return SUCCESS;
}

//
// Balance
//
extern void
ethereumUpdateWalletBalance (BREthereumEWM ewm,
                             BREthereumWalletId wid) {
    ewmUpdateWalletBalance(ewm, wid);
}

extern BREthereumStatus
ethereumClientAnnounceBalance (BREthereumEWM ewm,
                               BREthereumWalletId wid,
                               const char *balance,
                               int rid) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    if (NULL == wallet) { return ERROR_UNKNOWN_WALLET; }

    // Passed in `balance` can be base 10 or 16.  Let UInt256Prase decide.
    BRCoreParseStatus parseStatus;
    UInt256 value = createUInt256Parse(balance, 0, &parseStatus);
    if (CORE_PARSE_OK != parseStatus) { return ERROR_NUMERIC_PARSE; }

    ewmClientSignalAnnounceBalance(ewm, wallet, value, rid);
    return SUCCESS;
}

//
// Gas Price
//
extern void
ethereumUpdateWalletDefaultGasPrice (BREthereumEWM ewm,
                                     BREthereumWalletId wid) {
    ewmUpdateWalletDefaultGasPrice(ewm, wid);
}

extern BREthereumStatus
ethereumClientAnnounceGasPrice(BREthereumEWM ewm,
                               BREthereumWalletId wid,
                               const char *gasPrice,
                               int rid) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    if (NULL == wallet) { return ERROR_UNKNOWN_WALLET; }

    BRCoreParseStatus parseStatus;
    UInt256 amount = createUInt256Parse(gasPrice, 0, &parseStatus);
    if (CORE_PARSE_OK != parseStatus) { return ERROR_NUMERIC_PARSE; }

    ewmClientSignalAnnounceGasPrice(ewm, wallet, amount, rid);
    return SUCCESS;
}

//
// Gas Estimate
//
extern void
ethereumUpdateTransferGasEstimate (BREthereumEWM ewm,
                                   BREthereumWalletId wid,
                                   BREthereumTransferId tid) {
    ewmUpdateTransferGasEstimate(ewm, wid, tid);
}

extern BREthereumStatus
ethereumClientAnnounceGasEstimate (BREthereumEWM ewm,
                                   BREthereumWalletId wid,
                                   BREthereumTransferId tid,
                                   const char *gasEstimate,
                                   int rid) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    if (NULL == wallet) { return ERROR_UNKNOWN_WALLET; }

    BREthereumTransfer transaction = ewmLookupTransfer(ewm, tid);
    if (NULL == transaction) { return ERROR_UNKNOWN_TRANSACTION; }

    BRCoreParseStatus parseStatus;
    UInt256 gas = createUInt256Parse(gasEstimate, 0, &parseStatus);

    if (CORE_PARSE_OK != parseStatus ||
        0 != gas.u64[1] || 0 != gas.u64[2] || 0 != gas.u64[3]) { return ERROR_NUMERIC_PARSE; }


    ewmClientSignalAnnounceGasEstimate(ewm, wallet, transaction, gas, rid);
    return SUCCESS;
}

//
// Submit
//
extern BREthereumStatus
ethereumClientAnnounceSubmitTransfer(BREthereumEWM ewm,
                                        BREthereumWalletId wid,
                                        BREthereumTransferId tid,
                                        const char *strHash,
                                        int id) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    if (NULL == wallet) { return ERROR_UNKNOWN_WALLET; }

    BREthereumTransfer transfer = ewmLookupTransfer(ewm, tid);
    if (NULL == transfer) { return ERROR_UNKNOWN_TRANSACTION; }

    BREthereumHash hash = hashCreate(strHash);
    if (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(transferGetHash(transfer), hashCreateEmpty()))
        || ETHEREUM_BOOLEAN_IS_FALSE (hashEqual(transferGetHash(transfer), hash)))
        return ERROR_TRANSACTION_HASH_MISMATCH;

    ewmClientSignalAnnounceSubmitTransfer (ewm, wallet, transfer, id);
    return SUCCESS;
}

//
// Transactions
//
extern void
ethereumUpdateTransactions (BREthereumEWM ewm) {
    ewmUpdateTransactions(ewm);
}

extern BREthereumStatus
ethereumClientAnnounceTransaction(BREthereumEWM ewm,
                                  int id,
                                  const char *hashString,
                                  const char *from,
                                  const char *to,
                                  const char *contract,
                                  const char *strAmount, // value
                                  const char *strGasLimit,
                                  const char *strGasPrice,
                                  const char *data,
                                  const char *strNonce,
                                  const char *strGasUsed,
                                  const char *strBlockNumber,
                                  const char *strBlockHash,
                                  const char *strBlockConfirmations,
                                  const char *strBlockTransactionIndex,
                                  const char *strBlockTimestamp,
                                  const char *isError) {
    BRCoreParseStatus parseStatus;
    BREthereumEWMClientAnnounceTransactionBundle *bundle = malloc(sizeof (BREthereumEWMClientAnnounceTransactionBundle));

    bundle->hash = hashCreate(hashString);

    bundle->from = addressCreate(from);
    bundle->to = addressCreate(to);
    bundle->contract = addressCreate(contract);

    bundle->amount = createUInt256Parse(strAmount, 0, &parseStatus);

    bundle->gasLimit = strtoull(strGasLimit, NULL, 0);
    bundle->gasPrice = createUInt256Parse(strGasPrice, 0, &parseStatus);
    bundle->data = strdup(data);

    bundle->nonce = strtoull(strNonce, NULL, 0); // TODO: Assumes `nonce` is uint64_t; which it is for now
    bundle->gasUsed = strtoull(strGasUsed, NULL, 0);

    bundle->blockNumber = strtoull(strBlockNumber, NULL, 0);
    bundle->blockHash = hashCreate (strBlockHash);
    bundle->blockConfirmations = strtoull(strBlockConfirmations, NULL, 0);
    bundle->blockTransactionIndex = (unsigned int) strtoul(strBlockTransactionIndex, NULL, 0);
    bundle->blockTimestamp = strtoull(strBlockTimestamp, NULL, 0);

    bundle->isError = AS_ETHEREUM_BOOLEAN(0 != strcmp (isError, "0"));

    ewmClientSignalAnnounceTransaction(ewm, bundle, id);
    return SUCCESS;
}

//  {
//    "blockNumber":"1627184",
//    "timeStamp":"1516477482",
//    "hash":     "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
//    "blockHash":"0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
//    "transactionIndex":"3",
//    "from":"0x0ea166deef4d04aaefd0697982e6f7aa325ab69c",
//    "to":"0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
//    "nonce":"118",
//    "value":"11113000000000",
//    "gas":"21000",
//    "gasPrice":"21000000000",
//    "isError":"0",
//    "txreceipt_status":"1",
//    "input":"0x",
//    "contractAddress":"",
//    "cumulativeGasUsed":"106535",
//    "gasUsed":"21000",
//    "confirmations":"339050"}

//
// Logs
//
extern void
ethereumUpdateLogs (BREthereumEWM ewm,
                    BREthereumWalletId wid,
                    BREthereumContractEvent event) {
    ewmUpdateLogs(ewm, wid, event);
}

extern BREthereumStatus
ethereumClientAnnounceLog (BREthereumEWM ewm,
                           int id,
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
                           const char *strBlockTimestamp) {

    BRCoreParseStatus parseStatus;
    BREthereumEWMClientAnnounceLogBundle *bundle = malloc(sizeof (BREthereumEWMClientAnnounceLogBundle));

    bundle->hash = hashCreate(strHash);
    bundle->contract = addressCreate(strContract);
    bundle->topicCount = topicCount;
    bundle->arrayTopics = calloc (topicCount, sizeof (char *));
    for (int i = 0; i < topicCount; i++)
        bundle->arrayTopics[i] = strdup (arrayTopics[i]);
    bundle->data = strdup (strData);
    bundle->gasPrice = createUInt256Parse(strGasPrice, 0, &parseStatus);
    bundle->gasUsed = strtoull(strGasUsed, NULL, 0);
    bundle->logIndex = strtoull(strLogIndex, NULL, 0);
    bundle->blockNumber = strtoull(strBlockNumber, NULL, 0);
    bundle->blockTransactionIndex = strtoull(strBlockTransactionIndex, NULL, 0);
    bundle->blockTimestamp = strtoull(strBlockTimestamp, NULL, 0);

    ewmClientSignalAnnounceLog(ewm, bundle, id);
    return SUCCESS;
}

//http://ropsten.etherscan.io/api?module=logs&action=getLogs&fromBlock=0&toBlock=latest&address=0x722dd3f80bac40c951b51bdd28dd19d435762180&topic0=0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef&topic1=0x000000000000000000000000bDFdAd139440D2Db9BA2aa3B7081C2dE39291508&topic1_2_opr=or&topic2=0x000000000000000000000000bDFdAd139440D2Db9BA2aa3B7081C2dE39291508
//{
//    "status":"1",
//    "message":"OK",
//    "result":[
//              {
//              "address":"0x722dd3f80bac40c951b51bdd28dd19d435762180",
//              "topics":["0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
//                        "0x0000000000000000000000000000000000000000000000000000000000000000",
//                        "0x000000000000000000000000bdfdad139440d2db9ba2aa3b7081c2de39291508"],
//              "data":"0x0000000000000000000000000000000000000000000000000000000000002328",
//              "blockNumber":"0x1e487e",
//              "timeStamp":"0x59fa1ac9",
//              "gasPrice":"0xba43b7400",
//              "gasUsed":"0xc64e",
//              "logIndex":"0x",
//              "transactionHash":"0xa37bd8bd8b1fa2838ef65aec9f401f56a6279f99bb1cfb81fa84e923b1b60f2b",
//              "transactionIndex":"0x"},
//
//              {
//              "address":"0x722dd3f80bac40c951b51bdd28dd19d435762180",
//              "topics":["0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
//                        "0x000000000000000000000000bdfdad139440d2db9ba2aa3b7081c2de39291508",
//                        "0x0000000000000000000000006c0fe9f8f018e68e2f0bee94ab41b75e71df094d"],
//              "data":"0x00000000000000000000000000000000000000000000000000000000000003e8",
//              "blockNumber":"0x1e54a5",
//              "timeStamp":"0x59fac771",
//              "gasPrice":"0x4a817c800",
//              "gasUsed":"0xc886",
//              "logIndex":"0x",
//              "transactionHash":"0x393927b491208dd8c7415cd749a2559b345d47c800a5adfa8e3bd5307acb7de0",
//              "transactionIndex":"0x1"},
//
//
//              ...
//              }

//
// Tokens
//
extern void
ethereumUpdateTokens (BREthereumEWM ewm) {
    ewmUpdateTokens (ewm);
}

extern BREthereumStatus
ethereumClientAnnounceToken (BREthereumEWM ewm,
                             int id,
                             const char *address,
                             const char *symbol,
                             const char *name,
                             const char *description,
                             unsigned int decimals) {
    BREthereumEWMClientAnnounceTokenBundle *bundle = malloc (sizeof (BREthereumEWMClientAnnounceTokenBundle));

    bundle->address = strdup (address);
    bundle->symbol = strdup (symbol);
    bundle->name = strdup (name);
    bundle->description = strdup(description);
    bundle->decimals = decimals;

    ewmClientSignalAnnounceToken(ewm, bundle, id);
    return SUCCESS;
}


