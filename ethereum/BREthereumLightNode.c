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
#include <pthread.h>
#include <unistd.h>
#include "BREthereumPrivate.h"
#include "BRArray.h"

// Forward declaration
static BREthereumTransaction
lightNodeLookupTransaction (BREthereumLightNode node,
                            BREthereumLightNodeTransactionId transactionId);

static BREthereumWallet
lightNodeLookupWallet(BREthereumLightNode node,
                      BREthereumLightNodeWalletId wid);

static void
lightNodeInsertWallet (BREthereumLightNode node,
                       BREthereumWallet wallet);

//
// Light Node Listener
//
typedef struct  {
    BREthereumLightNodeListenerContext context;
    BREthereumLightNodeListenerWalletEventHandler walletEventHandler;
    BREthereumLightNodeListenerBlockEventHandler blockEventHandler;
    BREthereumLightNodeListenerTransactionEventHandler transactionEventHandler;
} BREthereumLightNodeListener;


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
                                     JsonRpcGetGasPrice funcGetGasPrice,
                                     JsonRpcEstimateGas funcEstimateGas,
                                     JsonRpcSubmitTransaction funcSubmitTransaction,
                                     JsonRpcGetTransactions funcGetTransactions,
                                     JsonRpcGetLogs funcGetLogs) {
    BREthereumLightNodeConfiguration configuration;
    configuration.network = network;
    configuration.type = NODE_TYPE_JSON_RPC;
    configuration.u.json_rpc.funcContext = context;
    configuration.u.json_rpc.funcGetBalance = funcGetBalance;
    configuration.u.json_rpc.funcGetGasPrice = funcGetGasPrice;
    configuration.u.json_rpc.funcEstimateGas = funcEstimateGas;
    configuration.u.json_rpc.funcSubmitTransaction = funcSubmitTransaction;
    configuration.u.json_rpc.funcGetTransactions = funcGetTransactions;
    configuration.u.json_rpc.funcGetLogs = funcGetLogs;
    return configuration;
}

//
// Light Node
//
#define DEFAULT_LISTENER_CAPACITY 3
#define DEFAULT_WALLET_CAPACITY 10
#define DEFAULT_BLOCK_CAPACITY 100
#define DEFAULT_TRANSACTION_CAPACITY 1000

typedef enum {
    LIGHT_NODE_CREATED,
    LIGHT_NODE_CONNECTING,
    LIGHT_NODE_CONNECTED,
    LIGHT_NODE_DISCONNECTING,
    LIGHT_NODE_DISCONNECTED,
    LIGHT_NODE_ERRORED
} BREthereumLightNodeState;

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
     * The transactions seen/handled by this node.  These are used *solely* for the TransactionId
     * interface in LightNode.  *All* transactions must be accesses through their wallet.
     */
    BREthereumTransaction *transactions; // BRSet
    
    /**
     * The blocks seen/handled by this node.
     */
    BREthereumBlock *blocks; // BRSet
    
    //
    unsigned int requestId;

    BREthereumLightNodeState state;

    BREthereumLightNodeListener *listeners; // BRArray

    pthread_t thread;
    pthread_mutex_t lock;
};

static BREthereumLightNode
createLightNodeInternal (BREthereumLightNodeConfiguration configuration,
                         BREthereumAccount account) {
    BREthereumLightNode node = (BREthereumLightNode) calloc (1, sizeof (struct BREthereumLightNodeRecord));
    node->state = LIGHT_NODE_CREATED;
    node->configuration = configuration;
    node->account = account;
    array_new(node->wallets, DEFAULT_WALLET_CAPACITY);
    array_new(node->transactions, DEFAULT_TRANSACTION_CAPACITY);
    array_new(node->blocks, DEFAULT_BLOCK_CAPACITY);
    array_new(node->listeners, DEFAULT_LISTENER_CAPACITY);

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&node->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // Create a default ETH wallet.
    node->walletHoldingEther = walletCreate(node->account,
                                            node->configuration.network);
    lightNodeInsertWallet(node, node->walletHoldingEther);

    // Create other wallets for each TOKEN?

    return node;

}

extern BREthereumLightNode
createLightNode (BREthereumLightNodeConfiguration configuration,
                 const char *paperKey) {
    return createLightNodeInternal(configuration, createAccount(paperKey));
}

extern BREthereumLightNode
createLightNodeWithPublicKey (BREthereumLightNodeConfiguration configuration,
                              const BRKey publicKey) { // 65 byte, 0x04-prefixed, uncompressed public key
    return createLightNodeInternal(configuration, createAccountWithPublicKey (publicKey));
}

extern BREthereumLightNodeAccountId
lightNodeGetAccount (BREthereumLightNode node) {
    return 0;
}

extern char *
lightNodeGetAccountPrimaryAddress (BREthereumLightNode node) {
    return addressAsString (accountGetPrimaryAddress(node->account));
}

extern BRKey // key.pubKey
lightNodeGetAccountPrimaryAddressPublicKey (BREthereumLightNode node) {
    return accountGetPrimaryAddressPublicKey(node->account);
}

extern BRKey
lightNodeGetAccountPrimaryAddressPrivateKey (BREthereumLightNode node,
                                             const char *paperKey) {
    return accountGetPrimaryAddressPrivateKey (node->account, paperKey);

}

//
// Listener
//
extern BREthereumLightNodeListenerId
lightNodeAddListener (BREthereumLightNode node,
                      BREthereumLightNodeListenerContext context,
                      BREthereumLightNodeListenerWalletEventHandler walletEventHandler,
                      BREthereumLightNodeListenerBlockEventHandler blockEventHandler,
                      BREthereumLightNodeListenerTransactionEventHandler transactionEventHandler) {
    BREthereumLightNodeListenerId lid = -1;
    BREthereumLightNodeListener listener;

    listener.context = context;
    listener.walletEventHandler = walletEventHandler;
    listener.blockEventHandler = blockEventHandler;
    listener.transactionEventHandler = transactionEventHandler;

    pthread_mutex_lock(&node->lock);
    array_add (node->listeners, listener);
    lid = (BREthereumLightNodeListenerId) (array_count (node->listeners) - 1);
    pthread_mutex_unlock(&node->lock);

    return lid;
}

extern BREthereumBoolean
lightNodeHasListener (BREthereumLightNode node,
                      BREthereumLightNodeListenerId lid) {
    return (0 <= lid && lid < array_count(node->listeners)
        && NULL != node->listeners[lid].context
        && (NULL != node->listeners[lid].walletEventHandler ||
            NULL != node->listeners[lid].blockEventHandler  ||
            NULL != node->listeners[lid].transactionEventHandler)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern BREthereumBoolean
lightNodeRemoveListener (BREthereumLightNode node,
                         BREthereumLightNodeListenerId lid) {
    if (0 <= lid && lid < array_count(node->listeners)) {
        memset (&node->listeners[lid], 0, sizeof (BREthereumLightNodeListener));
        return ETHEREUM_BOOLEAN_TRUE;
    }
    return ETHEREUM_BOOLEAN_FALSE;
}

extern void
lightNodeListenerAnnounceWalletEvent (BREthereumLightNode node,
                                      BREthereumLightNodeWalletId wid,
                                      BREthereumLightNodeWalletEvent event) {
    int count = (int) array_count(node->listeners);
    for (int i = 0; i < count; i++) {
        if (NULL != node->listeners[i].walletEventHandler)
            node->listeners[i].walletEventHandler
            (node->listeners[i].context,
             node,
             wid,
             event);
    }
}

extern void
lightNodeListenerAnnounceBlockEvent (BREthereumLightNode node,
                                     BREthereumLightNodeBlockId bid,
                                     BREthereumLightNodeBlockEvent event) {
    int count = (int) array_count(node->listeners);
    for (int i = 0; i < count; i++) {
        if (NULL != node->listeners[i].blockEventHandler)
            node->listeners[i].blockEventHandler
            (node->listeners[i].context,
             node,
             bid,
             event);
    }
}

extern void
lightNodeListenerAnnounceTransactionEvent (BREthereumLightNode node,
                                           BREthereumLightNodeWalletId wid,
                                           BREthereumLightNodeTransactionId tid,
                                           BREthereumLightNodeTransactionEvent event) {
    int count = (int) array_count(node->listeners);
    for (int i = 0; i < count; i++) {
        if (NULL != node->listeners[i].transactionEventHandler)
            node->listeners[i].transactionEventHandler
            (node->listeners[i].context,
             node,
             wid,
             tid,
             event);
    }
}

//
// Connect // Disconnect
//
#define PTHREAD_STACK_SIZE (512 * 1024)
#define PTHREAD_SLEEP_SECONDS (15)

typedef void* (*ThreadRoutine) (void*);

static void *
lightNodeThreadRoutine (BREthereumLightNode node) {
    node->state = LIGHT_NODE_CONNECTED;

    while (1) {
        if (LIGHT_NODE_DISCONNECTING == node->state) break;
        pthread_mutex_lock(&node->lock);

        // We'll query all transactions for this node's account.  That will give us a shot at
        // getting the nonce for the account's address correct.  We'll save all the transactions and
        // then process them into wallet as wallets exist.
        lightNodeUpdateTransactions(node);

        // Similarly, we'll query all logs for this node's account.  We'll process these into
        // (token) transactions and associate with their wallet.
        lightNodeUpdateLogs(node, eventERC20Transfer);

//        for (int i = 0; i < array_count(node->wallets); i++) {
//            BREthereumWallet wallet = lightNodeLookupWallet (node, i);
//            BREthereumToken token = walletGetToken(wallet);
//            if (NULL != token)
//                lightNodeUpdateLogs(node, tokenGetAddress(token));
//        }

        // For all the known wallets, get their balance.
        for (int i = 0; i < array_count(node->wallets); i++)
            lightNodeUpdateWalletBalance (node, i);

        pthread_mutex_unlock(&node->lock);
        if (1 == sleep (PTHREAD_SLEEP_SECONDS)) {
        }
    }

    node->state = LIGHT_NODE_DISCONNECTED;
    pthread_detach(node->thread);
    return NULL;
}

extern BREthereumBoolean
lightNodeConnect (BREthereumLightNode node) {
    pthread_attr_t attr;

    switch (node->state) {
        case LIGHT_NODE_CONNECTING:
        case LIGHT_NODE_CONNECTED:
        case LIGHT_NODE_DISCONNECTING:
            return ETHEREUM_BOOLEAN_FALSE;

        case LIGHT_NODE_CREATED:
        case LIGHT_NODE_DISCONNECTED:
        case LIGHT_NODE_ERRORED: {
            if (0 != pthread_attr_init(&attr)) {
                // Unable to initialize attr
                node->state = LIGHT_NODE_ERRORED;
                return ETHEREUM_BOOLEAN_FALSE;
            } else if (0 != pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) ||
                       0 != pthread_attr_setstacksize(&attr, PTHREAD_STACK_SIZE) ||
                       0 != pthread_create(&node->thread, &attr, (ThreadRoutine) lightNodeThreadRoutine, node)) {
                // Unable to fully create the thread w/ task
                node->state = LIGHT_NODE_ERRORED;
                pthread_attr_destroy(&attr);
                return ETHEREUM_BOOLEAN_FALSE;
            }

            node->state = LIGHT_NODE_CONNECTING;
            return ETHEREUM_BOOLEAN_TRUE;
        }
    }
}

extern BREthereumBoolean
lightNodeDisconnect (BREthereumLightNode node) {
    node->state = LIGHT_NODE_DISCONNECTING;
    return ETHEREUM_BOOLEAN_TRUE;
}

//
// Wallet
//

static void
lightNodeInsertWallet (BREthereumLightNode node,
                       BREthereumWallet wallet) {
    BREthereumLightNodeWalletId wid = -1;
    pthread_mutex_lock(&node->lock);
    array_add (node->wallets, wallet);
    wid = (BREthereumLightNodeWalletId) (array_count(node->wallets) - 1);
    pthread_mutex_unlock(&node->lock);
    lightNodeListenerAnnounceWalletEvent(node, wid, WALLET_EVENT_CREATED);
}

static BREthereumWallet
lightNodeLookupWallet(BREthereumLightNode node,
                      BREthereumLightNodeWalletId wid) {
    BREthereumWallet wallet = NULL;

    pthread_mutex_lock(&node->lock);
    wallet = (0 <= wid && wid < array_count(node->wallets)
              ? node->wallets[wid]
              : NULL);
    pthread_mutex_unlock(&node->lock);
    return wallet;
}

static BREthereumLightNodeWalletId
lightNodeLookupWalletId(BREthereumLightNode node,
                        BREthereumWallet wallet) {
    BREthereumLightNodeWalletId wid = -1;

    pthread_mutex_lock(&node->lock);
    for (int i = 0; i < array_count (node->wallets); i++)
        if (wallet == node->wallets[i]) {
            wid = i;
            break;
        }
    pthread_mutex_unlock(&node->lock);
    return wid;
}

extern BREthereumLightNodeWalletId
lightNodeGetWallet (BREthereumLightNode node) {
    return lightNodeLookupWalletId (node, node->walletHoldingEther);
}

extern BREthereumLightNodeWalletId
lightNodeGetWalletHoldingToken (BREthereumLightNode node,
                                BREthereumToken token) {
    BREthereumLightNodeWalletId wid = -1;

    pthread_mutex_lock(&node->lock);
    for (int i = 0; i < array_count(node->wallets); i++)
        if (token == walletGetToken(node->wallets[i])) {
            wid = i;
            break;
        }
    pthread_mutex_unlock(&node->lock);
    return wid;
}

extern BREthereumLightNodeWalletId
lightNodeCreateWalletHoldingToken(BREthereumLightNode node,
                                  BREthereumToken token) {
    pthread_mutex_lock(&node->lock);
    BREthereumLightNodeWalletId wid = lightNodeGetWalletHoldingToken(node, token);

    if (-1 == wid) {
        BREthereumWallet wallet = walletCreateHoldingToken(node->account,
                                                           node->configuration.network,
                                                           token);
        lightNodeInsertWallet(node, wallet);
        wid = lightNodeLookupWalletId(node, wallet);
    }

    pthread_mutex_unlock(&node->lock);
    return wid;
}

extern BREthereumBoolean
lightNodeWalletHoldsToken (BREthereumLightNode node,
                           BREthereumLightNodeWalletId wid,
                           BREthereumToken token) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    return (NULL != wallet && token == walletGetToken(wallet)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern BREthereumToken
lightNodeWalletGetToken(BREthereumLightNode node,
                        BREthereumLightNodeWalletId wid) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    return (NULL != wallet
            ? walletGetToken(wallet)
            : NULL);
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
                                   BREthereumLightNodeWalletId wid) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    return walletGetDefaultGasLimit(wallet).amountOfGas;
}

extern void
lightNodeWalletSetDefaultGasLimit (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId wid,
                                   uint64_t gasLimit) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    walletSetDefaultGasLimit(wallet, gasCreate(gasLimit));
    lightNodeListenerAnnounceWalletEvent(node, wid, WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED);

}

extern uint64_t
lightNodeWalletGetGasEstimate (BREthereumLightNode node,
                               BREthereumLightNodeWalletId wid,
                               BREthereumLightNodeTransactionId tid) {
    //  BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, tid);
    return transactionGetGasEstimate(transaction).amountOfGas;
}

extern void
lightNodeWalletSetDefaultGasPrice (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId wid,
                                   BREthereumEtherUnit unit,
                                   uint64_t value) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    walletSetDefaultGasPrice (wallet, gasPriceCreate(etherCreateNumber (value, unit)));
    lightNodeListenerAnnounceWalletEvent(node, wid, WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED);

}

extern uint64_t
lightNodeWalletGetDefaultGasPrice (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId wid) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    BREthereumGasPrice gasPrice = walletGetDefaultGasPrice(wallet);
    return (gtUInt256 (gasPrice.etherPerGas.valueInWEI, createUInt256(UINT64_MAX))
            ? 0
            : gasPrice.etherPerGas.valueInWEI.u64[0]);
}

extern BREthereumAmount
lightNodeWalletGetBalance (BREthereumLightNode node,
                           BREthereumLightNodeWalletId wid) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    return walletGetBalance(wallet);
}

extern char *
lightNodeWalletGetBalanceEther (BREthereumLightNode node,
                                BREthereumLightNodeWalletId wid,
                                BREthereumEtherUnit unit) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    BREthereumAmount balance = walletGetBalance(wallet);
    return (AMOUNT_ETHER == amountGetType(balance)
            ? etherGetValueString(balance.u.ether, unit)
            : NULL);
}

extern char *
lightNodeWalletGetBalanceTokenQuantity (BREthereumLightNode node,
                                        BREthereumLightNodeWalletId wid,
                                        BREthereumTokenQuantityUnit unit) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    BREthereumAmount balance = walletGetBalance(wallet);
    return (AMOUNT_TOKEN == amountGetType(balance)
            ? tokenQuantityGetValueString(balance.u.tokenQuantity, unit)
            : NULL);
}

extern BREthereumEther
lightNodeWalletEstimateTransactionFee (BREthereumLightNode node,
                                       BREthereumLightNodeWalletId wid,
                                       BREthereumAmount amount,
                                       int *overflow) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    return walletEstimateTransactionFee(wallet, amount, overflow);
}

//
// Blocks
//
static BREthereumBlock
lightNodeLookupBlock (BREthereumLightNode node,
                      const BREthereumHash hash) {
    BREthereumBlock block = NULL;

    pthread_mutex_lock(&node->lock);
    for (int i = 0; i < array_count(node->blocks); i++)
        if (ETHEREUM_COMPARISON_EQ == hashCompare(hash, blockGetHash(node->blocks[i]))) {
            block = node->blocks[i];
            break;
        }
    pthread_mutex_unlock(&node->lock);
    return block;
}

static void
lightNodeInsertBlock (BREthereumLightNode node,
                      BREthereumBlock block) {
    BREthereumLightNodeBlockId bid = -1;
    pthread_mutex_lock(&node->lock);
    array_add(node->blocks, block);
    bid = (BREthereumLightNodeBlockId) (array_count(node->blocks) - 1);
    pthread_mutex_unlock(&node->lock);
    lightNodeListenerAnnounceBlockEvent(node, bid, BLOCK_EVENT_CREATED);
}

//
// Transactions
//

static void
lightNodeInsertTransaction (BREthereumLightNode node,
                            BREthereumTransaction transaction) {
    pthread_mutex_lock(&node->lock);
    array_add (node->transactions, transaction);
    pthread_mutex_unlock(&node->lock);
}

static BREthereumTransaction
lightNodeLookupTransaction(BREthereumLightNode node,
                           BREthereumLightNodeTransactionId tid) {
    BREthereumTransaction transaction = NULL;

    pthread_mutex_lock(&node->lock);
    transaction = (0 <= tid && tid < array_count(node->transactions)
                   ? node->transactions[tid]
                   : NULL);
    pthread_mutex_unlock(&node->lock);
    return transaction;
}

static BREthereumLightNodeTransactionId
lightNodeLookupTransactionId(BREthereumLightNode node,
                           BREthereumTransaction transaction) {
    BREthereumLightNodeTransactionId tid = -1;

    pthread_mutex_lock(&node->lock);
    for (int i = 0; i < array_count(node->transactions); i++)
        if (transaction == node->transactions[i]) {
            tid = i;
            break;
        }
    pthread_mutex_unlock(&node->lock);
    return tid;
}

extern BREthereumLightNodeTransactionId
lightNodeWalletCreateTransaction(BREthereumLightNode node,
                                 BREthereumLightNodeWalletId wid,
                                 const char *recvAddress,
                                 BREthereumAmount amount) {
    BREthereumLightNodeTransactionId tid = -1;

    pthread_mutex_lock(&node->lock);

    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    BREthereumTransaction transaction = walletCreateTransaction (wallet,
                                                                 createAddress(recvAddress),
                                                                 amount);
    lightNodeInsertTransaction(node, transaction);
    tid = lightNodeLookupTransactionId(node, transaction);

    pthread_mutex_unlock(&node->lock);

    lightNodeListenerAnnounceTransactionEvent(node, wid, tid, TRANSACTION_EVENT_CREATED);
    return tid;
}

extern void // status, error
lightNodeWalletSignTransaction (BREthereumLightNode node,
                                BREthereumLightNodeWalletId wid,
                                BREthereumLightNodeTransactionId tid,
                                const char *paperKey) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, tid);
    walletSignTransaction(wallet, transaction, paperKey);
    lightNodeListenerAnnounceTransactionEvent(node, wid, tid, TRANSACTION_EVENT_SIGNED);
}

extern void // status, error
lightNodeWalletSignTransactionWithPrivateKey (BREthereumLightNode node,
                                BREthereumLightNodeWalletId wid,
                                BREthereumLightNodeTransactionId tid,
                                BRKey privateKey) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, tid);
    walletSignTransactionWithPrivateKey(wallet, transaction, privateKey);
    lightNodeListenerAnnounceTransactionEvent(node, wid, tid, TRANSACTION_EVENT_SIGNED);
}

extern void // status, error
lightNodeWalletSubmitTransaction(BREthereumLightNode node,
                                 BREthereumLightNodeWalletId wid,
                                 BREthereumLightNodeTransactionId tid) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, tid);

    switch (node->configuration.type) {
        case NODE_TYPE_JSON_RPC: {
            char *rawTransaction = walletGetRawTransactionHexEncoded(wallet, transaction, "0x");

            node->configuration.u.json_rpc.funcSubmitTransaction
                    (node->configuration.u.json_rpc.funcContext,
                     node,
                     wid,
                     tid,
                     rawTransaction,
                     ++node->requestId);

            free(rawTransaction);
            break;
        }
        case NODE_TYPE_LES:
            assert (0);
    }
}

//
//
//
static void
assignToTransactions (BREthereumLightNodeTransactionId *transactions,
                      BREthereumTransaction transaction,
                      unsigned int index) {
    transactions[index] = index;
}

extern BREthereumLightNodeTransactionId *
lightNodeWalletGetTransactions (BREthereumLightNode node,
                                BREthereumLightNodeWalletId wid) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    pthread_mutex_lock(&node->lock);

    unsigned long count = walletGetTransactionCount(wallet);
    BREthereumLightNodeTransactionId *transactions = calloc (count + 1, sizeof (BREthereumLightNodeTransactionId));

    walletWalkTransactions(wallet, transactions,
                           transactionPredicateAny,
                           (BREthereumTransactionWalker) assignToTransactions);
    transactions[count] = -1;

    pthread_mutex_unlock(&node->lock);
    return transactions;
}

extern int
lightNodeWalletGetTransactionCount (BREthereumLightNode node,
                                    BREthereumLightNodeWalletId wid) {
    int count = -1;

    pthread_mutex_lock(&node->lock);
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    if (NULL != wallet) count = (int) walletGetTransactionCount(wallet);
    pthread_mutex_unlock(&node->lock);

    return count;
}

//
// Transactions
//

/**
 *
 * @param node
 */
extern void
lightNodeUpdateTransactions (BREthereumLightNode node) {
    switch (node->configuration.type) {
        case NODE_TYPE_JSON_RPC: {
            char *address = addressAsString(accountGetPrimaryAddress(node->account));
            
            node->configuration.u.json_rpc.funcGetTransactions
            (node->configuration.u.json_rpc.funcContext,
             node,
             address,
             ++node->requestId);
            
            free (address);
            break;
        }
        case NODE_TYPE_LES:
            //      assert (0);
            break;
    }
}

//
// Logs
//
/**
 *
 * @param node
 */
extern void
lightNodeUpdateLogs (BREthereumLightNode node, BREthereumEvent event) {
    switch (node->configuration.type) {
        case NODE_TYPE_JSON_RPC: {
            char *address = addressAsString(accountGetPrimaryAddress(node->account));

            node->configuration.u.json_rpc.funcGetLogs
            (node->configuration.u.json_rpc.funcContext,
             node,
             address,
             eventGetSelector(event),
             ++node->requestId);

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
lightNodeUpdateWalletBalance(BREthereumLightNode node,
                             BREthereumLightNodeWalletId wid) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);

    switch (node->configuration.type) {
        case NODE_TYPE_JSON_RPC: {
            char *address = addressAsString(walletGetAddress(wallet));

            node->configuration.u.json_rpc.funcGetBalance
                    (node->configuration.u.json_rpc.funcContext,
                     node,
                     wid,
                     address,
                     ++node->requestId);

            free(address);
            break;
        }
        case NODE_TYPE_LES:
            assert (0);
    }
}

extern void
lightNodeUpdateTransactionGasEstimate (BREthereumLightNode node,
                                       BREthereumLightNodeWalletId wid,
                                       BREthereumLightNodeTransactionId tid) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, tid);
    
    switch (node->configuration.type) {
        case NODE_TYPE_JSON_RPC: {
            // This will be ZERO if transaction amount is in TOKEN.
            BREthereumEther amountInEther = transactionGetEffectiveAmountInEther (transaction);
            char *to = (char *) addressAsString (transactionGetTargetAddress(transaction));
            char *amount = coerceString(amountInEther.valueInWEI, 16);
            char *data = (char *) transactionGetData(transaction);
            
            node->configuration.u.json_rpc.funcEstimateGas
            (node->configuration.u.json_rpc.funcContext,
             node,
             wid,
             tid,
             to,
             amount,
             data,
             ++node->requestId);

            free (to); free (amount);

            if (NULL != data && '\0' != data[0])
                free(data);
            
            break;
        }
        case NODE_TYPE_LES:
            assert (0);
    }
}

extern void
lightNodeUpdateWalletDefaultGasPrice (BREthereumLightNode node,
                                      BREthereumLightNodeWalletId wid) {
    switch (node->configuration.type) {
        case NODE_TYPE_JSON_RPC: {
            node->configuration.u.json_rpc.funcGetGasPrice
            (node->configuration.u.json_rpc.funcContext,
             node,
             wid,
             ++node->requestId);
            break;
        }
        case NODE_TYPE_LES:
            assert (0);
    }
}

extern void
lightNodeFillTransactionRawData(BREthereumLightNode node,
                                BREthereumLightNodeWalletId wid,
                                BREthereumLightNodeTransactionId transactionId,
                                uint8_t **bytesPtr, size_t *bytesCountPtr) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
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
                                         BREthereumLightNodeWalletId wid,
                                         BREthereumLightNodeTransactionId transactionId,
                                         const char *prefix) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    
    return walletGetRawTransactionHexEncoded(wallet, transaction, prefix);
}

static BREthereumBlock
lightNodeAnnounceBlock(BREthereumLightNode node,
                       const char *strBlockNumber,
                       const char *strBlockHash,
                       const char *strBlockConfirmations,
                       const char *strBlockTimestamp) {
    // Build a block-ish
    BREthereumHash blockHash = hashCreate (strBlockHash);
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
    BREthereumLightNodeWalletId wid = (NULL == token
                                       ? lightNodeGetWallet(node)
                                       : lightNodeCreateWalletHoldingToken(node, token));
    return lightNodeLookupWallet(node, wid);
}

extern void
lightNodeAnnounceTransaction(BREthereumLightNode node,
                             int id,
                             const char *hashString,
                             const char *from,
                             const char *to,
                             const char *contract,
                             const char *amountString, // value
                             const char *gasLimitString,
                             const char *gasPriceString,
                             const char *data,
                             const char *strNonce,
                             const char *strGasUsed,
                             const char *blockNumber,
                             const char *blockHash,
                             const char *blockConfirmations,
                             const char *strBlockTransactionIndex,
                             const char *blockTimestamp,
                             const char *isError) {
    BREthereumTransaction transaction = NULL;
    BREthereumAddress primaryAddress = accountGetPrimaryAddress(node->account);
    
    assert (ETHEREUM_BOOLEAN_IS_TRUE(addressHasString(primaryAddress, from))
            || ETHEREUM_BOOLEAN_IS_TRUE(addressHasString(primaryAddress, to)));
    
    // primaryAddress is either the transaction's `source` or `target`.
    BREthereumBoolean isSource = addressHasString(primaryAddress, from);
    
    // Get the nonceValue
    uint64_t nonce = strtoull(strNonce, NULL, 10); // TODO: Assumes `nonce` is uint64_t; which it is for now

    pthread_mutex_lock(&node->lock);

    // If `isSource` then the nonce is 'ours'.
    if (ETHEREUM_BOOLEAN_IS_TRUE(isSource) && nonce >= addressGetNonce(primaryAddress))
        addressSetNonce(primaryAddress, nonce + 1);  // next

    // Find or create a block.  No point in doing this until we have a transaction of interest
    BREthereumBlock block = lightNodeAnnounceBlock(node, blockNumber, blockHash, blockConfirmations, blockTimestamp);
    assert (NULL != block);
    
    // The transaction's index within the block.
    unsigned int blockTransactionIndex = (unsigned int) strtoul (strBlockTransactionIndex, NULL, 10);
    
    // All transactions apply to the ETH wallet.
    BREthereumWallet wallet = node->walletHoldingEther;
    
    // Get the transaction's hash.
    BREthereumHash hash = hashCreate(hashString);
    
    // Look for a pre-existing transaction
    transaction =  walletGetTransactionByHash(wallet, hash);
    
    // If we did not have a transaction for 'hash' it might be (might likely be) a newly submitted
    // transaction that we are holding but that doesn't have a hash yet.  This will *only* apply
    // if we are the source.
    if (NULL == transaction && ETHEREUM_BOOLEAN_IS_TRUE(isSource))
        transaction = walletGetTransactionByNonce(wallet, nonce);
    
    // If we still don't have a transaction (with 'hash' or 'nonce'); then create one.
    if (NULL == transaction) {
        // TODO: Handle Status Error
        BRCoreParseStatus status;
        
        BREthereumAddress sourceAddr =
        (ETHEREUM_BOOLEAN_IS_TRUE(isSource) ? primaryAddress : createAddress(from));

        BREthereumAddress targetAddr =
        (ETHEREUM_BOOLEAN_IS_TRUE(isSource) ? createAddress(to) : primaryAddress);

        // Get the amount; this will be '0' if this is a token transfer
        BREthereumAmount amount =
        amountCreateEther(etherCreate(createUInt256Parse(amountString, 10, &status)));

        // Easily extract the gasPrice and gasLimit.
        BREthereumGasPrice gasPrice =
        gasPriceCreate(etherCreate(createUInt256Parse(gasPriceString, 10, &status)));
        
        BREthereumGas gasLimit =
        gasCreate(strtoull(gasLimitString, NULL, 0));
        
        // Finally, get ourselves a transaction.
        transaction = transactionCreate(sourceAddr,
                                        targetAddr,
                                        amount,
                                        gasPrice,
                                        gasLimit,
                                        nonce);

        // With a new transaction:
        //
        //   a) add to the light node
        lightNodeInsertTransaction(node, transaction);
        //
        //  b) add to the wallet
        walletHandleTransaction(wallet, transaction);
        //
        //  c) anounce as submitted.
        walletTransactionSubmitted(wallet, transaction, hash);
    }

    BREthereumGas gasUsed = gasCreate(strtoull(strGasUsed, NULL, 0));

    // TODO: Process 'state' properly - errors?
    
    // Get the current status.
    BREthereumTransactionStatus status = transactionGetStatus(transaction);

    // Update the status as blocked
    walletTransactionBlocked(wallet, transaction,
                             gasUsed,
                             blockGetNumber(block),
                             blockGetTimestamp(block),
                             blockTransactionIndex);

    if (TRANSACTION_BLOCKED != status) {
        BREthereumLightNodeWalletId wid = lightNodeLookupWalletId(node, wallet);
        BREthereumLightNodeTransactionId tid = lightNodeLookupTransactionId(node, transaction);
        lightNodeListenerAnnounceTransactionEvent(node, wid, tid, TRANSACTION_EVENT_BLOCKED);
    }

    // Hmmm...
    pthread_mutex_unlock(&node->lock);
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

/**
 *
 *
 */
extern void
lightNodeAnnounceLog (BREthereumLightNode node,
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

    pthread_mutex_lock(&node->lock);

    // Token of interest
    BREthereumToken token = tokenLookup(strContract);
    if (NULL == token) { pthread_mutex_unlock(&node->lock); return; } // uninteresting token

    // Event of interest
    BREthereumEvent event = contractLookupEventForTopic (contractERC20, arrayTopics[0]);
    if (NULL == event || event != eventERC20Transfer) { pthread_mutex_unlock(&node->lock); return; }; // uninteresting event

    // Wallet for token
    BREthereumWallet wallet = lightNodeAnnounceWallet(node, token);

    // Existing transaction
    BREthereumHash hash = hashCreate(strHash);
    BREthereumTransaction transaction = walletGetTransactionByHash(wallet, hash);

    BREthereumGas gasUsed = gasCreate(strtoull(strGasUsed, NULL, 0));

    // Create a token transaction
    if (NULL == transaction) {

        // Parse the topic data - we fake it becasue we 'know' topics indices
        BREthereumAddress sourceAddr = createAddress (eventERC20TransferDecodeAddress(event, arrayTopics[1]));
        BREthereumAddress targetAddr = createAddress (eventERC20TransferDecodeAddress(event, arrayTopics[2]));

        BRCoreParseStatus status = CORE_PARSE_OK;

        BREthereumAmount amount =
        amountCreateToken (createTokenQuantity(token, eventERC20TransferDecodeUInt256(event, strData, &status)));

        BREthereumGasPrice gasPrice =
        gasPriceCreate(etherCreate(createUInt256Parse(strGasPrice, 0, &status)));

        transaction = transactionCreate(sourceAddr, targetAddr, amount, gasPrice, gasUsed, 0);

        // With a new transaction:
        //
        //   a) add to the light node
        lightNodeInsertTransaction(node, transaction);
        //
        //  b) add to the wallet
        walletHandleTransaction(wallet, transaction);
        //
        //  c) anounce as submitted.
        walletTransactionSubmitted(wallet, transaction, hash);

    }

    // TODO: Process 'state' properly - errors?

    // Announce this transaction as blocked.
    // TODO: Handle already blocked: repeated transaction (most common).

    // Get the current status.
    BREthereumTransactionStatus status = transactionGetStatus(transaction);

    uint64_t blockNumber = strtoull(strBlockNumber, NULL, 0);
    uint64_t blockTimestamp = strtoull(strBlockTimestamp, NULL, 0);
    unsigned int blockTransactionIndex = (unsigned int) strtoul (strBlockTransactionIndex, NULL, 0);

    // Update the status as blocked
    walletTransactionBlocked(wallet, transaction,
                             gasUsed,
                             blockNumber,
                             blockTimestamp,
                             blockTransactionIndex);

    if (TRANSACTION_BLOCKED != status) {
        BREthereumLightNodeWalletId wid = lightNodeLookupWalletId(node, wallet);
        BREthereumLightNodeTransactionId tid = lightNodeLookupTransactionId(node, transaction);
        lightNodeListenerAnnounceTransactionEvent(node, wid, tid, TRANSACTION_EVENT_BLOCKED);
    }

    // Hmmmm...
    pthread_mutex_unlock(&node->lock);
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

extern void
lightNodeAnnounceBalance (BREthereumLightNode node,
                          BREthereumLightNodeWalletId wid,
                          const char *balance,
                          int rid) {
    BRCoreParseStatus status;

    assert (0 == strncmp (balance, "0x", 2));
    UInt256 value = createUInt256Parse(balance, 16, &status);

    pthread_mutex_lock(&node->lock);
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    BREthereumAmount amount = (AMOUNT_ETHER == walletGetAmountType(wallet)
                               ? amountCreateEther(etherCreate(value))
                               : amountCreateToken(createTokenQuantity(walletGetToken(wallet), value)));

    walletSetBalance (wallet, amount);
    pthread_mutex_unlock(&node->lock);
    lightNodeListenerAnnounceWalletEvent(node, wid, WALLET_EVENT_BALANCE_UPDATED);
}

extern void
lightNodeAnnounceGasPrice (BREthereumLightNode node,
                           BREthereumLightNodeWalletId wid,
                           const char *gasPrice,
                           int rid) {
    BRCoreParseStatus status;

    assert (0 == strncmp (gasPrice, "0x", 2));
    UInt256 amount = createUInt256Parse(&gasPrice[2], 16, &status);

    pthread_mutex_lock(&node->lock);
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    walletSetDefaultGasPrice(wallet, gasPriceCreate(etherCreate(amount)));
    pthread_mutex_unlock(&node->lock);

    lightNodeListenerAnnounceWalletEvent(node, wid, WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED);
}

extern void
lightNodeAnnounceGasEstimate (BREthereumLightNode node,
                              BREthereumLightNodeWalletId wid,
                              BREthereumLightNodeTransactionId tid,
                              const char *gasEstimate,
                              int rid) {
    BRCoreParseStatus status = CORE_PARSE_OK;
    assert (0 == strncmp (gasEstimate, "0x", 2));
    UInt256 gas = createUInt256Parse(&gasEstimate[2], 16, &status);
    assert (0 == gas.u64[1] && 0 == gas.u64[2] && 0 == gas.u64[3]);

    pthread_mutex_lock(&node->lock);
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, tid);
    transactionSetGasEstimate(transaction, gasCreate(gas.u64[0]));
    pthread_mutex_unlock(&node->lock);

    lightNodeListenerAnnounceTransactionEvent(node, wid, tid, TRANSACTION_EVENT_GAS_ESTIMATE_UPDATED);
}

extern void
lightNodeAnnounceSubmitTransaction(BREthereumLightNode node,
                                   BREthereumLightNodeWalletId wid,
                                   BREthereumLightNodeTransactionId tid,
                                   const char *strHash,
                                   int id) {
    pthread_mutex_lock(&node->lock);
    BREthereumWallet  wallet = lightNodeLookupWallet(node, wid);
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, tid);
    if (NULL != wallet && NULL != transaction) {
        BREthereumHash hash = hashCreate(strHash);
        walletTransactionSubmitted(wallet, transaction, hash);
        free (hash);
    }
    pthread_mutex_unlock(&node->lock);

    lightNodeListenerAnnounceTransactionEvent(node, wid, tid, TRANSACTION_EVENT_SUBMITTED);
}

#endif // ETHEREUM_LIGHT_NODE_USE_JSON_RPC

extern char *
lightNodeTransactionGetRecvAddress(BREthereumLightNode node,
                                   BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    return addressAsString(transactionGetTargetAddress(transaction));
}

extern char * // sender, source
lightNodeTransactionGetSendAddress (BREthereumLightNode node,
                                    BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    return addressAsString(transactionGetSourceAddress(transaction));
}

extern char *
lightNodeTransactionGetHash (BREthereumLightNode node,
                             BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    return strdup (transactionGetHash(transaction));
}

extern char *
lightNodeTransactionGetAmountEther(BREthereumLightNode node,
                                   BREthereumLightNodeTransactionId transactionId,
                                   BREthereumEtherUnit unit) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    BREthereumAmount amount = transactionGetAmount(transaction);
    return (AMOUNT_ETHER == amountGetType(amount)
            ? etherGetValueString(amountGetEther(amount), unit)
            : "");
}

extern char *
lightNodeTransactionGetAmountTokenQuantity(BREthereumLightNode node,
                                           BREthereumLightNodeTransactionId transactionId,
                                           BREthereumTokenQuantityUnit unit) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    BREthereumAmount amount = transactionGetAmount(transaction);
    return (AMOUNT_TOKEN == amountGetType(amount)
            ? tokenQuantityGetValueString(amountGetTokenQuantity(amount), unit)
            : "");
}

extern BREthereumAmount
lightNodeTransactionGetAmount(BREthereumLightNode node,
                              BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    return transactionGetAmount(transaction);
}

extern BREthereumAmount
lightNodeTransactionGetGasPriceToo(BREthereumLightNode node,
                                BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    BREthereumGasPrice gasPrice = transactionGetGasPrice(transaction);
    return amountCreateEther (gasPrice.etherPerGas);
}

extern char *
lightNodeTransactionGetGasPrice(BREthereumLightNode node,
                                BREthereumLightNodeTransactionId transactionId,
                                BREthereumEtherUnit unit) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    BREthereumGasPrice gasPrice = transactionGetGasPrice(transaction);
    return etherGetValueString(gasPrice.etherPerGas, unit);
}

extern uint64_t
lightNodeTransactionGetGasLimit(BREthereumLightNode node,
                                BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    return transactionGetGasLimit(transaction).amountOfGas;
}

extern uint64_t
lightNodeTransactionGetGasUsed (BREthereumLightNode node,
                                BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    BREthereumGas gasUsed;
    return (transactionExtractBlocked(transaction, &gasUsed, NULL, NULL, NULL)
            ? gasUsed.amountOfGas
            : 0);
}

extern uint64_t
lightNodeTransactionGetNonce (BREthereumLightNode node,
                              BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    return transactionGetNonce(transaction);
}

extern uint64_t
lightNodeTransactionGetBlockNumber (BREthereumLightNode node,
                                    BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    uint64_t blockNumber;
    return (transactionExtractBlocked(transaction, NULL, &blockNumber, NULL, NULL)
            ? blockNumber
            : 0);
}

extern uint64_t
lightNodeTransactionGetBlockTimestamp (BREthereumLightNode node,
                                       BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    uint64_t blockTimestamp;
    return (transactionExtractBlocked(transaction, NULL, NULL, &blockTimestamp, NULL)
            ? blockTimestamp
            : 0);
}

extern BREthereumBoolean
lightNodeTransactionIsConfirmed(BREthereumLightNode node,
                                BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    return transactionIsConfirmed(transaction);
}

extern BREthereumBoolean
lightNodeTransactionIsSubmitted (BREthereumLightNode node,
                                 BREthereumLightNodeTransactionId transactionId) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    return transactionIsSubmitted(transaction);
}

extern BREthereumBoolean
lightNodeTransactionHoldsToken (BREthereumLightNode node,
                                 BREthereumLightNodeTransactionId tid,
                                 BREthereumToken token) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, tid);
    assert (NULL != transaction);
    return (token == transactionGetToken(transaction)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern BREthereumToken
lightNodeTransactionGetToken (BREthereumLightNode node,
                              BREthereumLightNodeTransactionId tid) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, tid);
    assert (NULL != transaction);
    return transactionGetToken(transaction);
}

extern BREthereumEther
lightNodeTransactionGetFee (BREthereumLightNode node,
                            BREthereumLightNodeTransactionId tid,
                            int *overflow) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, tid);
    assert (NULL != transaction);
    return transactionGetFee(transaction, overflow);
}

