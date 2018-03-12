//
//  BREthereumLightNode
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

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "BREthereumLightNode.h"
#include "BREthereumAccount.h"
#include "BREthereumWallet.h"
#include "BREthereumHolding.h"

struct BREthereumLightNodeRecord {
    BREthereumLightNodeConfiguration configuration;
    BREthereumAccount account;
    BREthereumWallet  wallets[10];  // for now
    BREthereumTransaction transactions[1000]; // for now
};

extern BREthereumLightNode
createLightNode (BREthereumLightNodeConfiguration configuration) {
    BREthereumLightNode node = (BREthereumLightNode) calloc (1, sizeof (struct BREthereumLightNodeRecord));
    node->configuration = configuration;
    return node;
}

extern BREthereumLightNodeAccountId
lightNodeCreateAccount (BREthereumLightNode node,
                        const char *paperKey) {
    if (NULL != node->account) return NULL;

    node->account = createAccount (paperKey);
    return (void *) node->account;
}

extern const char *
lightNodeGetAccountPrimaryAddress (BREthereumLightNode node,
                                   BREthereumLightNodeAccountId account) {
    if (node->account != (BREthereumAccount) account) return NULL;

    // TODO: Copy address
    return addressAsString (accountGetPrimaryAddress(node->account));
}

//
// Wallet
//

static void
lightNodeInsertWallet (BREthereumLightNode node,
                       BREthereumWallet wallet) {
    for (int i = 0; i < 10; i++)
        if (NULL == node->wallets[i]) {
            node->wallets[i] = wallet;
            return;
        }
    assert (0);
}

static int
ligthNodeHasWallet (BREthereumLightNode node,
                    BREthereumWallet wallet) {
    for (int i = 0; i < 10; i++)
        if (wallet == node->wallets[i])
            return 1;
    return 0;
}

extern BREthereumLightNodeWalletId
lightNodeCreateWallet (BREthereumLightNode node,
                       BREthereumLightNodeAccountId accountId) {

    BREthereumAccount account = (BREthereumAccount) accountId;
    if (node->account != (BREthereumAccount) account) return NULL;

    BREthereumAddress address = accountGetPrimaryAddress(account);

    BREthereumWallet wallet = walletCreateWithAddress(account, address);

    lightNodeInsertWallet(node, wallet);
    return (void *) wallet;
}

// Token

//
// Holding / Ether
//
extern BREthereumEther
lightNodeCreateEtherAmountString (BREthereumLightNode node,
                                  const char *number,
                                  BREthereumEtherUnit unit,
                                  BREthereumEtherParseStatus *status) {
  return etherCreateString(number, unit, status);
}

extern BREthereumEther
lightNodeCreateEtherAmountUnit (BREthereumLightNode node,
                                uint64_t amountInUnit,
                                BREthereumEtherUnit unit) {
  return etherCreateNumber(amountInUnit, unit);
}

//
// Wallet Defaults
//
extern uint64_t
lightNodeGetWalletGasLimit (BREthereumLightNode node,
                            BREthereumLightNodeWalletId walletId) {
    BREthereumWallet wallet = (BREthereumWallet) walletId;

    return walletGetDefaultGasLimit(wallet).amountOfGas;
}

extern void
lightNodeSetWalletGasLimit (BREthereumLightNode node,
                            BREthereumLightNodeWalletId walletId,
                            uint64_t gasLimit) {
  BREthereumWallet wallet = (BREthereumWallet) walletId;

  walletSetDefaultGasLimit(wallet, gasCreate(gasLimit));
}

extern void
lightNodeSetWalletGasPrice (BREthereumLightNode node,
                            BREthereumLightNodeWalletId walletId,
                            BREthereumEtherUnit unit,
                            uint64_t value) {
  BREthereumWallet wallet = (BREthereumWallet) walletId;

  walletSetDefaultGasPrice (wallet, gasPriceCreate(etherCreateNumber (value, unit)));
}

//
// Transactions
//

static void
lightNodeInsertTransaction (BREthereumLightNode node,
                             BREthereumTransaction transaction) {
    for (int i = 0; i < 1000; i++)
        if (NULL == node->transactions[i]) {
            node->transactions[i] = transaction;
            return;
        }
    assert (0);
}

extern BREthereumLightNodeTransactionId
lightNodeWalletCreateTransaction(BREthereumLightNode node,
                                 BREthereumLightNodeWalletId walletId,
                                 const char *recvAddress,
                                 BREthereumEther amount) {
    BREthereumWallet wallet = (BREthereumWallet) walletId;

    BREthereumTransaction transaction = walletCreateTransaction
            (wallet,
             createAddress(recvAddress),
             holdingCreateEther (amount));

    lightNodeInsertTransaction(node, transaction);
    return (void *) transaction;
}

extern void // status, error
lightNodeWalletSignTransaction (BREthereumLightNode node,
                                BREthereumLightNodeWalletId walletId,
                                BREthereumLightNodeTransactionId transactionId,
                                const char *paperKey) {
    BREthereumWallet wallet = (BREthereumWallet) walletId;
    BREthereumTransaction transaction = (BREthereumTransaction) transactionId;

    walletSignTransaction(wallet, transaction, paperKey);
}

#if !ETHEREUM_LIGHT_NODE_USE_JSON_RPC
extern void // status, error
lightNodeWalletSubmitTransaction (BREthereumLightNode node,
                                BREthereumLightNodeWalletId walletId,
                                BREthereumLightNodeTransactionId transactionId,
                                const char *paperKey) {
    BREthereumWallet wallet = (BREthereumWallet) walletId;
    BREthereumTransaction transaction = (BREthereumTransaction) transactionId;

    // TODO: Submit if not using JSON_RPC
}
#endif

//
// Transactions
//

#if ETHEREUM_LIGHT_NODE_USE_JSON_RPC
extern void
lightNodeFillTransactionRawData(BREthereumLightNode node,
                                BREthereumLightNodeWalletId walletId,
                                BREthereumLightNodeTransactionId transactionId,
                                uint8_t **bytesPtr, size_t *bytesCountPtr) {
    BREthereumWallet wallet = (BREthereumWallet) walletId;
    BREthereumTransaction transaction = (BREthereumTransaction) transactionId;

    assert (NULL != bytesCountPtr && NULL != bytesPtr);
    assert (ETHEREUM_BOOLEAN_IS_TRUE (transactionIsSigned(transaction)));

    BRRlpData rawTransactionData =
            walletGetRawTransaction(wallet, transaction);

    *bytesCountPtr = rawTransactionData.bytesCount;
    *bytesPtr = (uint8_t *) malloc (*bytesCountPtr);
    memcpy (*bytesPtr, rawTransactionData.bytes, *bytesCountPtr);
}

extern const char *
lightNodeGetTransactionRawDataHexEncoded(BREthereumLightNode node,
                                         BREthereumLightNodeWalletId walletId,
                                         BREthereumLightNodeTransactionId transactionId,
                                         const char *prefix) {
  BREthereumWallet wallet = (BREthereumWallet) walletId;
  BREthereumTransaction transaction = (BREthereumTransaction) transactionId;

  return walletGetRawTransactionHexEncoded(wallet, transaction, prefix);
}

#endif

extern const char *
lightNodeGetTransactionRecvAddress (BREthereumLightNode node,
                                    BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = (BREthereumTransaction) transactionId;

    return addressAsString(transactionGetTargetAddress(transaction));
}

extern BREthereumEther
lightNodeGetTransactionAmount (BREthereumLightNode node,
                               BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = (BREthereumTransaction) transactionId;
    BREthereumHolding holding = transactionGetAmount(transaction);
    return holding.holding.ether.amount;
}

