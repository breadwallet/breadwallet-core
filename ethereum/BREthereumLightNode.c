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

//
// Light Node Configuration
//
extern BREthereumLightNodeConfiguration
lightNodeConfigurationCreateLES (BREthereumNetwork network /* ... */, int foo) {
  BREthereumLightNodeConfiguration configuration;
  configuration.network = network;
  configuration.type = NODE_TYPE_LES;
  configuration.u.les.foo = foo;
  return configuration;
}

extern BREthereumLightNodeConfiguration
lightNodeConfigurationCreateJSON_RPC(BREthereumNetwork network,
                                     JsonRpcContext context,
                                     JsonRpcGetBalance funcGetBalance,
                                     JsonRpcGetGasPrice functGetGasPrice,
                                     JsonRpcEstimateGas funcEstimateGas,
                                     JsonRpcSubmitTransaction funcSubmitTransaction) {
  BREthereumLightNodeConfiguration configuration;
  configuration.network = network;
  configuration.type = NODE_TYPE_JSON_RPC;
  configuration.u.json_rpc.funcContext = context;
  configuration.u.json_rpc.funcGetBalance = funcGetBalance;
  configuration.u.json_rpc.functGetGasPrice = functGetGasPrice;
  configuration.u.json_rpc.funcEstimateGas = funcEstimateGas;
  configuration.u.json_rpc.funcSubmitTransaction = funcSubmitTransaction;
  return configuration;
}

//
// Light Node
//
struct BREthereumLightNodeRecord {
    BREthereumLightNodeConfiguration configuration;
    BREthereumAccount account;
    BREthereumWallet  wallets[10];  // for now
    BREthereumTransaction transactions[1000]; // for now
    unsigned int requestId;
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
                       BREthereumLightNodeAccountId accountId,
                       BREthereumNetwork network) {

    BREthereumAccount account = (BREthereumAccount) accountId;
    if (node->account != (BREthereumAccount) account) return NULL;

    BREthereumWallet wallet = walletCreate(account, network);

    lightNodeInsertWallet(node, wallet);
    return (void *) wallet;
}

extern BREthereumLightNodeWalletId
lightNodeCreateWalletHoldingToken (BREthereumLightNode node,
                                   BREthereumLightNodeAccountId accountId,
                                   BREthereumNetwork network,
                                   BREthereumToken token) {
  BREthereumAccount account = (BREthereumAccount) accountId;
  if (node->account != (BREthereumAccount) account) return NULL;

  BREthereumWallet wallet = walletCreateHoldingToken (account, network, token);

  lightNodeInsertWallet(node, wallet);
  return (void *) wallet;

}

// Token

//
// Holding / Ether
//
extern BREthereumHolding
lightNodeCreateEtherAmountString (BREthereumLightNode node,
                                  const char *number,
                                  BREthereumEtherUnit unit,
                                  BREthereumEtherParseStatus *status) {
  return holdingCreateEther (etherCreateString(number, unit, status));
}

extern BREthereumHolding
lightNodeCreateEtherAmountUnit (BREthereumLightNode node,
                                uint64_t amountInUnit,
                                BREthereumEtherUnit unit) {
  return holdingCreateEther (etherCreateNumber(amountInUnit, unit));
}

extern char *
lightNodeCoerceEtherAmountToString (BREthereumLightNode node,
                                    BREthereumEther ether,
                                    BREthereumEtherUnit unit) {
  return etherGetValueString(ether, unit);
}

//
// Wallet
//
extern BREthereumEther
lightNodeUpdateWalletBalance (BREthereumLightNode node,
                              BREthereumLightNodeWalletId walletId) {
  BREthereumWallet wallet = (BREthereumWallet) walletId;

  switch (node->configuration.type) {
    case NODE_TYPE_JSON_RPC: {
      const char *address = addressAsString(walletGetAddress(wallet));

      int error = 0;
      const char *result = node->configuration.u.json_rpc.funcGetBalance
        (node->configuration.u.json_rpc.funcContext, ++node->requestId, address);
      assert (0 == strcmp (result, "0x"));
      UInt256 amount = createUInt256Parse(&result[2], 16, &error);
      return etherCreate(amount);
    }
    case NODE_TYPE_LES:
      assert (0);
  }
}

extern BREthereumGas
lightNodeUpdateWalletEstimatedGas (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId wallet) {
  switch (node->configuration.type) {
    case NODE_TYPE_JSON_RPC: {
      int error = 0;
      const char *result = node->configuration.u.json_rpc.functGetGasPrice
        (node->configuration.u.json_rpc.funcContext, ++node->requestId);
      assert (0 == strcmp (result, "0x"));
      UInt256 amount = createUInt256Parse(&result[2], 16, &error);
      assert (0 == amount.u64[1] && 0 == amount.u64[2] && 0 == amount.u64[3]);
      return gasCreate(amount.u64[0]);
    }
    case NODE_TYPE_LES:
      assert (0);
  }
}

extern BREthereumGasPrice
lightNodeUpdateWalletEstimatedGasPrice (BREthereumLightNode node,
                                        BREthereumLightNodeWalletId wallet) {
  switch (node->configuration.type) {
    case NODE_TYPE_JSON_RPC: {
      int error = 0;
      const char *result = node->configuration.u.json_rpc.functGetGasPrice
        (node->configuration.u.json_rpc.funcContext, ++node->requestId);
      assert (0 == strcmp (result, "0x"));
      UInt256 amount = createUInt256Parse(&result[2], 16, &error);
      return gasPriceCreate(etherCreate(amount));
    }
    case NODE_TYPE_LES:
      assert (0);
  }
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

extern BREthereumBoolean // status, error
lightNodeWalletSubmitTransaction (BREthereumLightNode node,
                                  BREthereumLightNodeWalletId walletId,
                                  BREthereumLightNodeTransactionId transactionId) {
  BREthereumWallet wallet = (BREthereumWallet) walletId;
  BREthereumTransaction transaction = (BREthereumTransaction) transactionId;

  switch (node->configuration.type) {
    case NODE_TYPE_JSON_RPC: {
      const char *rawTransaction = walletGetRawTransactionHexEncoded(wallet, transaction, "0x");

      // Result is the 'transaction hash'
      const char *result = node->configuration.u.json_rpc.funcSubmitTransaction
        (node->configuration.u.json_rpc.funcContext, ++node->requestId, rawTransaction);

      int success = (strlen(result) > 2
                     ? ETHEREUM_BOOLEAN_TRUE
                     : ETHEREUM_BOOLEAN_FALSE);

      free ((char *) rawTransaction);
      free ((char *) result);

      return success;
    }
    case NODE_TYPE_LES:
      assert (0);
  }
}

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
