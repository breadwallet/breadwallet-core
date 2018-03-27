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
#include <stdarg.h>
#include <stdio.h>  // sprintf
#include <BRBIP39Mnemonic.h>
#include "BREthereumPrivate.h"
#include "BRArray.h"

// Forward declaration
static BREthereumTransaction
lightNodeLookupTransaction (BREthereumLightNode node,
                            BREthereumLightNodeWalletId walletId,
                            BREthereumLightNodeTransactionId transactionId);

static void
lightNodeInsertWallet (BREthereumLightNode node,
                       BREthereumWallet wallet);
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
#define DEFAULT_WALLET_CAPACITY 10
#define DEFAULT_BLOCK_CAPACITY 100
#define DEFAULT_TRANSACTION_CAPACITY 1000

/**
 *
 */
struct BREthereumLightNodeRecord {
    /**
     * The Configuration defining this Light Node
     */
    BREthereumLightNodeConfiguration configuration;
    
    /**
     * The account
     */
    BREthereumAccount account;
    
    /**
     * The wallets 'managed/handled' by this node.  There can be only one wallet holding ETHER;
     * all the other wallets hold TOKENs and only one wallet per TOKEN.
     */
    BREthereumWallet *wallets;  // for now
    BREthereumWallet  walletHoldingEther;
    
    /**
     * The transactions seen/handled by this node.  These will generally be parcelled out to their
     * associaed wallets.
     */
    BREthereumTransaction *transactions; // BRSet
    
    /**
     * The blocks seen/handled by this node.
     */
    BREthereumBlock *blocks; // BRSet
    
    //
    unsigned int requestId;
};

extern BREthereumLightNode
createLightNode (BREthereumLightNodeConfiguration configuration,
                 const char *paperKey) {
    BREthereumLightNode node = (BREthereumLightNode) calloc (1, sizeof (struct BREthereumLightNodeRecord));
    node->configuration = configuration;
    node->account = createAccount(paperKey);
    array_new(node->wallets, DEFAULT_WALLET_CAPACITY);
    array_new(node->transactions, DEFAULT_TRANSACTION_CAPACITY);
    array_new(node->blocks, DEFAULT_BLOCK_CAPACITY);
    
    // Create a default ETH wallet.
    node->walletHoldingEther = walletCreate(node->account,
                                            node->configuration.network);
    lightNodeInsertWallet(node, node->walletHoldingEther);
    
    // Create other wallets for each TOKEN?
    
    return node;
}

extern BREthereumLightNodeAccountId
lightNodeGetAccount (BREthereumLightNode node) {
    return node->account;
}

extern char *
lightNodeGetAccountPrimaryAddress (BREthereumLightNode node) {
    return addressAsString (accountGetPrimaryAddress(node->account));
}

//
// Connect // Disconnect
//
extern BREthereumBoolean
lightNodeConnect (BREthereumLightNode node) {
    // We'll query all transactions for this node's account.  That will give us a shot at
    // getting the nonce for the account's address correct.  We'll save all the transactions and
    // then process them into wallet as wallets exist.
    lightNodeUpdateTransactions(node);
    
    // For all the known wallets, get their balance.
    BRCoreParseStatus status;
    for (int i = 0; i < array_count(node->wallets); i++) {
        lightNodeUpdateWalletBalance (node, (BREthereumLightNodeWalletId) node->wallets[i], &status);
        if (CORE_PARSE_OK != status) {
            
        }
    }
    
    return ETHEREUM_BOOLEAN_TRUE;
    
}

extern BREthereumBoolean
lightNodeDisconnect (BREthereumLightNode node) {
    return ETHEREUM_BOOLEAN_TRUE;
}

//
// Wallet
//

static void
lightNodeInsertWallet (BREthereumLightNode node,
                       BREthereumWallet wallet) {
    array_add (node->wallets, wallet);
}

static BREthereumWallet
lightNodeLookupWallet (BREthereumLightNode node,
                       BREthereumLightNodeWalletId walletId) {
    for (int i = 0; i < array_count(node->wallets); i++)
        if (walletId == node->wallets[i])
            return node->wallets[i];
    return NULL;
}

extern BREthereumLightNodeWalletId
lightNodeGetWallet (BREthereumLightNode node) {
    return (BREthereumLightNodeWalletId) node->walletHoldingEther;
}

extern BREthereumLightNodeWalletId
lightNodeCreateWalletHoldingToken (BREthereumLightNode node,
                                   BREthereumToken token) {
    BREthereumWallet wallet = walletCreateHoldingToken (node->account,
                                                        node->configuration.network,
                                                        token);
    lightNodeInsertWallet(node, wallet);
    return (BREthereumLightNodeWalletId) wallet;
    
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
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, walletId, transactionId);
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
// Blocks
//
static BREthereumBlock
lightNodeLookupBlock (BREthereumLightNode node,
                      const BREthereumHash hash) {
    for (int i = 0; i < array_count(node->blocks); i++)
        if (ETHEREUM_COMPARISON_EQ == hashCompare(hash, blockGetHash(node->blocks[i])))
            return node->blocks[i];
    return NULL;
}

static void
lightNodeInsertBlock (BREthereumLightNode node,
                      BREthereumBlock block) {
    array_add(node->blocks, block);
}

//
// Transactions
//

//static void
//lightNodeInsertTransaction (BREthereumLightNode node,
//                             BREthereumTransaction transaction) {
//    for (int i = 0; i < HACK_NUMBER_OF_TRANSACTIONS; i++)
//        if (NULL == node->transactions[i]) {
//            node->transactions[i] = transaction;
//            return;
//        }
//    assert (0);
//}
//
static BREthereumTransaction
lightNodeLookupTransaction (BREthereumLightNode node,
                            BREthereumLightNodeWalletId walletId,
                            BREthereumLightNodeTransactionId transactionId) {
    for (int i = 0; i < array_count(node->wallets); i++)
        if (walletId == node->wallets[i]) {
            return walletGetTransactionById(node->wallets[i], transactionId);
        }
    return NULL;
}

static BREthereumTransaction
lightNodeLookupTransactionByHash (BREthereumLightNode node,
                                  BREthereumHash hash) {
    for (int i = 0; i < array_count(node->transactions); i++)
        if (ETHEREUM_COMPARISON_EQ == hashCompare(hash, transactionGetHash(node->transactions[i])))
            return node->transactions[i];
    return NULL;
}

static BREthereumTransaction
lightNodeLookupTransactionByNone (BREthereumLightNode node,
                                  uint64_t nonce) {
    for (int i = 0; i < array_count(node->transactions); i++)
        if (ETHEREUM_COMPARISON_EQ == hashCompare(hashCreateEmpty(), transactionGetHash(node->transactions[i]))
            && nonce == transactionGetNonce(node->transactions[i]))
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
    
    //  lightNodeInsertTransaction(node, transaction);
    return (BREthereumLightNodeTransactionId) transaction;
}

extern void // status, error
lightNodeWalletSignTransaction (BREthereumLightNode node,
                                BREthereumLightNodeWalletId walletId,
                                BREthereumLightNodeTransactionId transactionId,
                                const char *paperKey) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, walletId, transactionId);
    walletSignTransaction(wallet, transaction, paperKey);
}

extern BREthereumBoolean // status, error
lightNodeWalletSubmitTransaction (BREthereumLightNode node,
                                  BREthereumLightNodeWalletId walletId,
                                  BREthereumLightNodeTransactionId transactionId) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, walletId, transactionId);
    
    switch (node->configuration.type) {
        case NODE_TYPE_JSON_RPC: {
            const char *rawTransaction = walletGetRawTransactionHexEncoded(wallet, transaction, "0x");
            
            // Result is the 'transaction hash'
            const char *result = node->configuration.u.json_rpc.funcSubmitTransaction
            (node->configuration.u.json_rpc.funcContext, node, ++node->requestId, rawTransaction);
            
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
lightNodeUpdateTransactions (BREthereumLightNode node) {
    switch (node->configuration.type) {
        case NODE_TYPE_JSON_RPC: {
            char *address = addressAsString(accountGetPrimaryAddress(node->account));
            
            node->configuration.u.json_rpc.funcGetTransactions
            (node->configuration.u.json_rpc.funcContext, node, ++node->requestId, address);
            
            free (address);
            break;
        }
        case NODE_TYPE_LES:
            //      assert (0);
            break;
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
            (node->configuration.u.json_rpc.funcContext, node, ++node->requestId, address);
            
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
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, walletId, transactionId);
    
    switch (node->configuration.type) {
        case NODE_TYPE_JSON_RPC: {
            // This will be ZERO if transaction amount is in TOKEN.
            BREthereumEther amountInEther = transactionGetEffectiveAmountInEther (transaction);
            char *to = (char *) addressAsString (transactionGetTargetAddress(transaction));
            char *amount = coerceString(amountInEther.valueInWEI, 16);
            char *data = (char *) transactionGetData(transaction);
            
            const char *result = node->configuration.u.json_rpc.funcEstimateGas
            (node->configuration.u.json_rpc.funcContext, node, ++node->requestId,
             to, amount, data);

            free (to); free (amount);

            if (NULL != data && '\0' != data[0])
                free(data);
            
            assert (0 == strncmp (result, "0x", 2));
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
            (node->configuration.u.json_rpc.funcContext, node, ++node->requestId);
            assert (0 == strncmp (result, "0x", 2));
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
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, walletId, transactionId);
    
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
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, walletId, transactionId);
    
    return walletGetRawTransactionHexEncoded(wallet, transaction, prefix);
}

static BREthereumBlock
lightNodeAnnounceBlock(BREthereumLightNode node,
                       const char *strBlockNumber,
                       const char *blockHashString,
                       const char *strBlockConfirmations,
                       const char *strBlockTimestamp) {
    // Build a block-ish
    BREthereumHash blockHash = hashCreate (blockHashString);
    BREthereumBlock block = lightNodeLookupBlock(node, blockHash);
    if (NULL == block) {
        uint64_t blockNumber = strtoull(strBlockNumber, NULL, 0);
        uint64_t blockTimestamp = strtoull(strBlockTimestamp, NULL, 0);
        uint64_t blockConfirmations = strtoull (strBlockConfirmations, NULL, 0);

        block = createBlock(blockHash, blockNumber, blockConfirmations, blockTimestamp);
        lightNodeInsertBlock(node, block);
    }
    else {
        // confirm
    }
    free (blockHash);
    return block;
}

static int
lightNodeDataIsEmpty (BREthereumLightNode node, const char *data) {
    return NULL == data || 0 == strcmp ("", data) || 0 == strcmp ("0x", data);
}

static BREthereumToken
lightNodeAnnounceToken (BREthereumLightNode node,
                        const char *target,
                        const char *contract,
                        const char *data) {
    // If `data` is anything besides "0x", then we have a contract function call.  At that point
    // it seems we need to process `data` to extract the 'function + args' and then, if the
    // function is 'transfer() token' we can then and only then conclude that we have a token
    
    if (lightNodeDataIsEmpty(node, data)) return NULL;
    
    // There is contract data; see if it is a ERC20 function.
    BREthereumFunction function = contractLookupFunctionForEncoding(contractERC20, data);
    
    // Not an ERC20 token
    if (NULL == function) return NULL;
    
    // See if we have an existing token.
    BREthereumToken token = tokenLookup(target);
    if (NULL == token) token = tokenLookup(contract);
    
    // We found a token...
    if (NULL != token) return token;
    
    // ... we didn't find a token - we should create is dynamically.
    fprintf (stderr, "Ignoring transaction for unknown ERC20 token at '%s'", target);
    return NULL;
}

static BREthereumWallet
lightNodeAnnounceWallet(BREthereumLightNode node,
                        BREthereumToken token) {
    // Find a wallet.
    for (int i = 0; i < array_count(node->wallets); i++) {
        // If we have a token and it matches wallet's token OR we don't have a
        // token and wallet doesn't either.
        if ((NULL != token && token == walletGetToken(node->wallets[i]))
            || (NULL == token && NULL == walletGetToken(node->wallets[i]))) {
            return node->wallets[i];
        }
    }
    
    // We always assume a wallet holding ETHER; so only here for non-NULL token.
    assert (NULL != token);
    
    // Create a wallet for token.
    BREthereumLightNodeWalletId walletId = lightNodeCreateWalletHoldingToken (node, token);
    return lightNodeLookupWallet(node, walletId);
}

extern void
lightNodeAnnounceTransaction(BREthereumLightNode node,
                             const char *hashString,
                             const char *from,
                             const char *to,
                             const char *contract,
                             const char *amountString, // value
                             const char *gasLimitString,
                             const char *gasPriceString,
                             const char *data,
                             const char *nonce,
                             const char *strGasUsed,
                             const char *blockNumber,
                             const char *blockHash,
                             const char *blockConfirmations,
                             const char *strBlockTransactionIndex,
                             const char *blockTimestamp,
                             const char *isError) {
    BREthereumTransaction transaction = NULL;
    BREthereumAddress primaryAddress = accountGetPrimaryAddress(node->account);
    int newTransaction = 0;
    
    assert (ETHEREUM_BOOLEAN_IS_TRUE(addressHasString(primaryAddress, from))
            || ETHEREUM_BOOLEAN_IS_TRUE(addressHasString(primaryAddress, to)));
    
    // primaryAddress is either the transaction's `source` or `target`.
    BREthereumBoolean isSource = addressHasString(primaryAddress, from);
    
    // Get the nonceValue
    uint64_t nonceValue = strtoull(nonce, NULL, 10); // TODO: Assumes `nonce` is uint64_t; which it is for now
    
    // If `isSource` then the nonce is 'ours'.
    if (ETHEREUM_BOOLEAN_IS_TRUE(isSource) && nonceValue >= addressGetNonce(primaryAddress))
        addressSetNonce(primaryAddress, nonceValue + 1);  // next
    
    // Find a token.
    BREthereumToken token = lightNodeAnnounceToken(node,
                                                   (ETHEREUM_BOOLEAN_IS_TRUE(isSource) ? to : from),
                                                   contract,
                                                   data);
    
    // If we have data, but did not identify a token; then we are processing a transaction for
    // a contract that we know nothing about - likely not even a wallet.
    if (!lightNodeDataIsEmpty(node, data) && NULL == token) {
        fprintf (stderr, "Ignoring transaction with '%s' having data '%s'",
                 (ETHEREUM_BOOLEAN_IS_TRUE(isSource) ? to : from),
                 data);
        return;
    }
    
    // Find or create a block.  No point is doing this until we have a transaction of interest
    BREthereumBlock block = lightNodeAnnounceBlock(node, blockNumber, blockHash, blockConfirmations, blockTimestamp);
    assert (NULL != block);
    
    // The transaction's index within the block.
    unsigned int blockTransactionIndex = (unsigned int) strtoul (strBlockTransactionIndex, NULL, 10);
    
    // Find or create a wallet for this transaction
    BREthereumWallet wallet = lightNodeAnnounceWallet (node, token);
    
    // Get the transaction's hash.
    BREthereumHash hash = hashCreate(hashString);
    
    // Look for a pre-existing transaction
    transaction = lightNodeLookupTransactionByHash(node, hash);
    
    // If we did not have a transaction for 'hash' it might be (might likely be) a newly submitted
    // transaction that we are holding but that doesn't have a hash yet.  This will *only* apply
    // if we are the source.
    if (NULL == transaction && ETHEREUM_BOOLEAN_IS_TRUE(isSource))
        transaction = lightNodeLookupTransactionByNone(node, nonceValue);
    
    // If we still don't have a transaction (with 'hash' or 'nonce'); then create one.
    if (NULL == transaction) {
        BRCoreParseStatus status;
        
        // If we have an ERC20 token, then the {source,target}Address comes from data.
        char *tokenTarget = NULL;
        if (NULL != token) {
            tokenTarget = functionERC20TransferDecodeAddress(functionERC20Transfer, data);
            if (ETHEREUM_BOOLEAN_IS_TRUE(isSource))
                to = tokenTarget;
            else
                from = tokenTarget;
        }
        
        BREthereumAddress sourceAddr =
        (ETHEREUM_BOOLEAN_IS_TRUE(isSource) ? primaryAddress : createAddress(from));
        BREthereumAddress targetAddr =
        (ETHEREUM_BOOLEAN_IS_TRUE(isSource) ? createAddress(to) : primaryAddress);
        
        // TODO: What to do with `contractAddr` - find an example in returned JSON
        // TODO: What to do with non-Ether, non-Token-Transfer transactions
        
        // Get the amount; this can be tricky if we have a token
        BREthereumAmount amount =
        (NULL == token
         ? amountCreateEther(etherCreate(createUInt256Parse(amountString, 10, &status)))
         : amountCreateToken(createTokenQuantity(token,
                                                 functionERC20TransferDecodeAmount(
                                                                                   functionERC20Transfer,
                                                                                   data,
                                                                                   &status))));
        // TODO: Handle Status Error
        
        // Easily extract the gasPrice and gasLimit.
        BREthereumGasPrice gasPrice =
        gasPriceCreate(etherCreate(createUInt256Parse(gasPriceString, 16, &status)));
        
        BREthereumGas gasLimit =
        gasCreate(strtoull(gasLimitString, NULL, 0));
        
        // Finally, get ourselves a transaction.
        transaction = transactionCreate(sourceAddr,
                                        targetAddr,
                                        amount,
                                        gasPrice,
                                        gasLimit,
                                        nonceValue);
        // With transaction defined - including with a properly typed amount; we should be able
        // to validate based, in part, on strcmp(data, transaction->data)
        
        array_add (node->transactions, transaction);
        newTransaction = 1;
        
        if (NULL != tokenTarget) free(tokenTarget);
    }

    BREthereumGas gasUsed = gasCreate(strtoull(strGasUsed, NULL, 0));

    // Other properties
    // hash
    // state (block, etc)
    
    // For a new transaction, add it to wallet and then transition through the transaction
    // states.  We do extra work here; but we ensure that the transactions happen in order and
    // consistently with the normal flow - which is created (added to end of wallet transactions);
    // submitted (assigned hash) and then blocked (resorted to proper position, perhaps).
    if (1 == newTransaction) {
        // This transaction is in wallet.
        walletHandleTransaction(wallet, transaction);
        // When in this callback, a new transaction implies submitted.  So announce.
        walletTransactionSubmitted(wallet, transaction, hash);
    }
    
    
    // TODO: Process 'state' properly - errors?
    
    // Announce this transaction as blocked.
    // TODO: Handle already blocked: repeated transaction (most common).
    walletTransactionBlocked(wallet, transaction,
                             gasUsed,
                             blockGetNumber(block),
                             blockGetTimestamp(block),
                             blockTransactionIndex);
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



#endif // ETHEREUM_LIGHT_NODE_USE_JSON_RPC

extern const char *
lightNodeGetTransactionRecvAddress (BREthereumLightNode node,
                                    BREthereumLightNodeWalletId walletId,
                                    BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, walletId, transactionId);
    return addressAsString(transactionGetTargetAddress(transaction));
}

extern BREthereumEther
lightNodeGetTransactionAmount (BREthereumLightNode node,
                               BREthereumLightNodeWalletId walletId,
                               BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, walletId, transactionId);
    BREthereumAmount holding = transactionGetAmount(transaction);
    return amountGetEther(holding); // TODO: Fatal if TOKEN
}

static void
assignToTransactions (BREthereumLightNodeTransactionId *transactions,
                      BREthereumTransaction transaction,
                      unsigned int index) {
    transactions[index] = (BREthereumLightNodeTransactionId) transaction;
}

extern BREthereumLightNodeTransactionId *
lightNodeWalletGetTransactions (BREthereumLightNode node,
                                BREthereumLightNodeWalletId walletId) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, walletId);
    unsigned long count = walletGetTransactionCount(wallet);
    BREthereumLightNodeTransactionId *transactions = calloc (count + 1, sizeof (BREthereumLightNodeTransactionId));
    
    walletWalkTransactions(wallet, transactions,
                           transactionPredicateAny,
                           (BREthereumTransactionWalker) assignToTransactions);
    transactions[count] = NULL;
    
    return transactions;
}

static char *
ullToStr (uint64_t value) {
    char buf[100];
    sprintf(buf, "%" PRIu64, value);
    return strdup (buf);
}

extern char *
lightNodeGetTransactionProperty (BREthereumLightNode node,
                                 BREthereumLightNodeWalletId wallet,
                                 BREthereumLightNodeTransactionId transactionId,
                                 BREthereumTransactionProperty property) {
    BREthereumTransaction transaction = (BREthereumTransaction) transactionId; // lightNodeLookupTransaction(node, wallet, transactionId);
    if (NULL == transaction) return NULL;
    
    switch (property) {
        case TRANSACTION_PROPERTY_TO_ADDR:
            return addressAsString(transactionGetTargetAddress(transaction));
        case TRANSACTION_PROPERTY_FROM_ADDR:
            return addressAsString(transactionGetSourceAddress(transaction));
        case TRANSACTION_PROPERTY_AMOUNT: {
            BREthereumAmount amount = transactionGetAmount(transaction);
            return (AMOUNT_ETHER == amountGetType(amount)
                    ? etherGetValueString(amountGetEther(amount), WEI)
                    : tokenQuantityGetValueString(amountGetTokenQuantity(amount),
                                                  TOKEN_QUANTITY_TYPE_INTEGER));
        }
        case TRANSACTION_PROPERTY_GAS_PRICE:
            return etherGetValueString(transactionGetGasPrice(transaction).etherPerGas, WEI);
        case TRANSACTION_PROPERTY_GAS_LIMIT:
            return ullToStr(transactionGetGasLimit(transaction).amountOfGas);
        case TRANSACTION_PROPERTY_GAS_USED:  {
            BREthereumGas gasUsed;
            return (transactionExtractBlocked(transaction, &gasUsed, NULL, NULL, NULL)
                    ? ""
                    : ullToStr(gasUsed.amountOfGas));
        }
        case TRANSACTION_PROPERTY_NONCE:
            return ullToStr(transactionGetNonce(transaction));
        case TRANSACTION_PROPERTY_HASH: {
            BREthereumHash hash = transactionGetHash(transaction);
            return hashExists(hash) ? hashCopy(hash) : "";
        }
        case TRANSACTION_PROPERTY_BLOCK_NUMBER: {
            uint64_t blockNumber;
            return (transactionExtractBlocked(transaction, NULL, &blockNumber, NULL, NULL)
                    ? ""
                    : ullToStr(blockNumber));
        }
        case TRANSACTION_PROPERTY_BLOCK_TIMESTAMP: {
            uint64_t blockTimestamp;
            return (transactionExtractBlocked(transaction, NULL, NULL, &blockTimestamp, NULL)
                    ? ""
                    : ullToStr(blockTimestamp));
        }
    }
}

extern char **
lightNodeGetTransactionProperties (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId wallet,
                                   BREthereumLightNodeTransactionId transactionId,
                                   unsigned int propertyCount,
                                   ...) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, wallet, transactionId);
    if (NULL == transaction || 0 == propertyCount) return NULL;
    
    char **result = (char **) calloc (propertyCount, sizeof (char*));
    
    va_list args;
    va_start (args, propertyCount);
    for (int i = 0; i < propertyCount; i++)
        result[i] = lightNodeGetTransactionProperty (node, wallet, transaction,
                                                     va_arg (args, BREthereumTransactionProperty));
    va_end(args);
    return result;
}

