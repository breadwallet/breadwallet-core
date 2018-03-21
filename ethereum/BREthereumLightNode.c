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
#include <BRBIP39Mnemonic.h>
#include <BRBIP39WordsEn.h>
#include "BREthereumPrivate.h"

// Forward declaration
static BREthereumTransaction
lightNodeLookupTransaction (BREthereumLightNode node,
                            BREthereumLightNodeTransactionId transactionId);

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
                                     JsonRpcSubmitTransaction funcSubmitTransaction,
                                     JsonRpcGetTransactions funcGetTransactions) {
  BREthereumLightNodeConfiguration configuration;
  configuration.network = network;
  configuration.type = NODE_TYPE_JSON_RPC;
  configuration.u.json_rpc.funcContext = context;
  configuration.u.json_rpc.funcGetBalance = funcGetBalance;
  configuration.u.json_rpc.functGetGasPrice = functGetGasPrice;
  configuration.u.json_rpc.funcEstimateGas = funcEstimateGas;
  configuration.u.json_rpc.funcSubmitTransaction = funcSubmitTransaction;
  configuration.u.json_rpc.funcGetTransactions = funcGetTransactions;
  return configuration;
}

//
// Light Node
//
#define HACK_NUMBER_OF_WALLETS 10
#define HACK_NUMBER_OF_TRANSACTIONS 1000

struct BREthereumLightNodeRecord {
    BREthereumLightNodeConfiguration configuration;
    BREthereumAccount account;
    BREthereumWallet  wallets[HACK_NUMBER_OF_WALLETS];  // for now
    BREthereumTransaction transactions[HACK_NUMBER_OF_TRANSACTIONS]; // for now
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

  // TODO: Move somewhere.
  installSharedWordList(BRBIP39WordsEn, BIP39_WORDLIST_COUNT);

  node->account = createAccount(paperKey);
  return (void *) node->account;
}

extern char *
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
    for (int i = 0; i < HACK_NUMBER_OF_WALLETS; i++)
        if (NULL == node->wallets[i]) {
            node->wallets[i] = wallet;
            return;
        }
    assert (0);
}

static BREthereumWallet
lightNodeLookupWallet (BREthereumLightNode node,
                       BREthereumLightNodeWalletId walletId) {
  for (int i = 0; i < HACK_NUMBER_OF_WALLETS; i++)
    if (walletId == node->wallets[i])
      return node->wallets[i];
  return NULL;
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
extern BREthereumAmount
lightNodeCreateEtherAmountString (BREthereumLightNode node,
                                  const char *number,
                                  BREthereumEtherUnit unit,
                                  BRCoreParseStatus *status) {
  return amountCreateEther (etherCreateString(number, unit, status));
}

extern BREthereumAmount
lightNodeCreateEtherAmountUnit (BREthereumLightNode node,
                                uint64_t amountInUnit,
                                BREthereumEtherUnit unit) {
  return amountCreateEther (etherCreateNumber(amountInUnit, unit));
}

extern char *
lightNodeCoerceEtherAmountToString (BREthereumLightNode node,
                                    BREthereumEther ether,
                                    BREthereumEtherUnit unit) {
  return etherGetValueString(ether, unit);
}

extern BREthereumAmount
lightNodeCreateTokenAmountString (BREthereumLightNode node,
                                  BREthereumToken token,
                                  const char *number,
                                  BREthereumTokenQuantityUnit unit,
                                  BRCoreParseStatus *status) {
  return amountCreateTokenQuantityString(token, number, unit, status);
}

//
// Wallet
//
//
// Wallet Defaults
//
extern uint64_t
lightNodeWalletGetDefaultGasLimit (BREthereumLightNode node,
                            BREthereumLightNodeWalletId walletId) {
  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
  return walletGetDefaultGasLimit(wallet).amountOfGas;
}

extern void
lightNodeWalletSetDefaultGasLimit (BREthereumLightNode node,
                            BREthereumLightNodeWalletId walletId,
                            uint64_t gasLimit) {
  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
  walletSetDefaultGasLimit(wallet, gasCreate(gasLimit));
}

extern uint64_t
lightNodeWalletGetGasEstimate (BREthereumLightNode node,
                               BREthereumLightNodeWalletId walletId,
                               BREthereumLightNodeTransactionId transactionId) {
//  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
  BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
  return transactionGetGasEstimate(transaction).amountOfGas;
}

extern void
lightNodeWalletSetDefaultGasPrice (BREthereumLightNode node,
                            BREthereumLightNodeWalletId walletId,
                            BREthereumEtherUnit unit,
                            uint64_t value) {
  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
  walletSetDefaultGasPrice (wallet, gasPriceCreate(etherCreateNumber (value, unit)));
}

extern uint64_t
lightNodeWalletGetDefaultGasPrice (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId walletId) {
  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
  BREthereumGasPrice gasPrice = walletGetDefaultGasPrice(wallet);
  return (gtUInt256 (gasPrice.etherPerGas.valueInWEI, createUInt256(UINT64_MAX))
          ? 0
          : gasPrice.etherPerGas.valueInWEI.u64[0]);
}

extern BREthereumAmount
lightNodeWalletGetBalance (BREthereumLightNode node,
                           BREthereumLightNodeWalletId walletId) {
  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
  return walletGetBalance(wallet);
}

//
// Transactions
//

static void
lightNodeInsertTransaction (BREthereumLightNode node,
                             BREthereumTransaction transaction) {
    for (int i = 0; i < HACK_NUMBER_OF_TRANSACTIONS; i++)
        if (NULL == node->transactions[i]) {
            node->transactions[i] = transaction;
            return;
        }
    assert (0);
}

static BREthereumTransaction
lightNodeLookupTransaction (BREthereumLightNode node,
                           BREthereumLightNodeTransactionId transactionId) {
  for (int i = 0; i < HACK_NUMBER_OF_TRANSACTIONS; i++)
    if (transactionId == node->transactions[i])
      return node->transactions[i];
  return NULL;
}

extern BREthereumLightNodeTransactionId
lightNodeWalletCreateTransaction(BREthereumLightNode node,
                                 BREthereumLightNodeWalletId walletId,
                                 const char *recvAddress,
                                 BREthereumAmount amount) {
  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
  BREthereumTransaction transaction = walletCreateTransaction (wallet,
                                                               createAddress(recvAddress),
                                                               amount);

  lightNodeInsertTransaction(node, transaction);
  return (void *) transaction;
}

extern void // status, error
lightNodeWalletSignTransaction (BREthereumLightNode node,
                                BREthereumLightNodeWalletId walletId,
                                BREthereumLightNodeTransactionId transactionId,
                                const char *paperKey) {
  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
  BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
  walletSignTransaction(wallet, transaction, paperKey);
}

extern BREthereumBoolean // status, error
lightNodeWalletSubmitTransaction (BREthereumLightNode node,
                                  BREthereumLightNodeWalletId walletId,
                                  BREthereumLightNodeTransactionId transactionId) {
  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
  BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);

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

/**
 *
 * @param node
 * @param walletId
 */
extern void
lightNodeWalletUpdateTransactions (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId walletId) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);

    switch (node->configuration.type) {
        case NODE_TYPE_JSON_RPC: {
            char *address = addressAsString(walletGetAddress(wallet));

            node->configuration.u.json_rpc.funcGetTransactions
                    (node->configuration.u.json_rpc.funcContext, ++node->requestId, address);

            free (address);
        }
            break;
        case NODE_TYPE_LES:
            assert (0);
    }
}

#if ETHEREUM_LIGHT_NODE_USE_JSON_RPC
extern void
lightNodeUpdateWalletBalance (BREthereumLightNode node,
                              BREthereumLightNodeWalletId walletId,
                              BRCoreParseStatus *status) {
  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);

  switch (node->configuration.type) {
    case NODE_TYPE_JSON_RPC: {
      char *address = addressAsString(walletGetAddress(wallet));
      const char *number = node->configuration.u.json_rpc.funcGetBalance
      (node->configuration.u.json_rpc.funcContext, ++node->requestId, address);

      assert (0 == strncmp (number, "0x", 2));
      UInt256 value = createUInt256Parse(number, 16, status);

      BREthereumAmount amount = (AMOUNT_ETHER == walletGetAmountType(wallet)
                                 ? amountCreateEther(etherCreate(value))
                                 : amountCreateToken(createTokenQuantity(walletGetToken(wallet), value)));

      walletSetBalance (wallet, amount);
      free (address);
      break;
    }
    case NODE_TYPE_LES:
      assert (0);
  }
}

extern void
lightNodeUpdateTransactionGasEstimate (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId walletId,
                                   BREthereumLightNodeTransactionId transactionId,
                                   BRCoreParseStatus *status) {
  //  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
  BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);

  switch (node->configuration.type) {
    case NODE_TYPE_JSON_RPC: {
      // This will be ZERO if transaction amount is in TOKEN.
      BREthereumEther amountInEther = transactionGetEffectiveAmountInEther (transaction);
      char *to = (char *) addressAsString (transactionGetTargetAddress(transaction));
      char *amount = coerceString(amountInEther.valueInWEI, 16);
      char *data = (char *) transactionGetData(transaction);

      const char *result = node->configuration.u.json_rpc.funcEstimateGas
      (node->configuration.u.json_rpc.funcContext, ++node->requestId,
       to, amount, data);
      free (to); free (amount), free(data);

      assert (0 == strcmp (result, "0x"));
      UInt256 gas = createUInt256Parse(&result[2], 16, status);
      assert (0 == gas.u64[1] && 0 == gas.u64[2] && 0 == gas.u64[3]);

      transactionSetGasEstimate(transaction, gasCreate(gas.u64[0]));
      break;
    }
    case NODE_TYPE_LES:
      assert (0);
  }
}

extern void
lightNodeUpdateWalletDefaultGasPrice (BREthereumLightNode node,
                                        BREthereumLightNodeWalletId wallet,
                                        BRCoreParseStatus *status) {
  switch (node->configuration.type) {
    case NODE_TYPE_JSON_RPC: {
      const char *result = node->configuration.u.json_rpc.functGetGasPrice
      (node->configuration.u.json_rpc.funcContext, ++node->requestId);
      assert (0 == strcmp (result, "0x"));
      UInt256 amount = createUInt256Parse(&result[2], 16, status);

      walletSetDefaultGasPrice(wallet, gasPriceCreate(etherCreate(amount)));
      break;
    }
    case NODE_TYPE_LES:
      assert (0);
  }
}


extern void
lightNodeFillTransactionRawData(BREthereumLightNode node,
                                BREthereumLightNodeWalletId walletId,
                                BREthereumLightNodeTransactionId transactionId,
                                uint8_t **bytesPtr, size_t *bytesCountPtr) {
  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
  BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);

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
  BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
  BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);

  return walletGetRawTransactionHexEncoded(wallet, transaction, prefix);
}

extern void
lightNodeAnnounceTransaction(BREthereumLightNode node,
                             const char *from,
                             const char *to,
                             const char *contract,
                             const char *amount, // value
                             const char *gasLimit,
                             const char *gasPrice,
                             const char *data,
                             const char *nonce) {
    // Figure out the wallet....
    // create a transaction
    return;
}

//  {
//    "blockNumber":"1627184",
//    "timeStamp":"1516477482",
//    "hash":"0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
//    "nonce":"118",
//    "blockHash":"0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
//    "transactionIndex":"3",
//    "from":"0x0ea166deef4d04aaefd0697982e6f7aa325ab69c",
//    "to":"0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
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



#endif // ETHEREUM_LIGHT_NODE_USE_JSON_RPC

extern const char *
lightNodeGetTransactionRecvAddress (BREthereumLightNode node,
                                    BREthereumLightNodeTransactionId transactionId) {
  BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
  return addressAsString(transactionGetTargetAddress(transaction));
}

extern BREthereumEther
lightNodeGetTransactionAmount (BREthereumLightNode node,
                               BREthereumLightNodeTransactionId transactionId) {
  BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
  BREthereumAmount holding = transactionGetAmount(transaction);
  return amountGetEther(holding); // TODO: Fatal if TOKEN
}
