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
#include <pthread.h>
#include <unistd.h>
#include "BRArray.h"
#include "BRBIP39Mnemonic.h"

#include "BREthereumPrivate.h"
#include "BREthereumLightNodePrivate.h"
#include "../event/BREvent.h"

#define LIGHT_NODE_SLEEP_SECONDS (5)

/* Forward Declaration */
static void
lightNodePeriodicDispatcher (BREventHandler handler,
                             BRTimeoutEvent *event);

//
// Light Node Client
//
extern BREthereumClient
ethereumClientCreate(BREthereumClientContext context,
                     BREthereumClientHandlerGetBalance funcGetBalance,
                     BREthereumClientHandlerGetGasPrice funcGetGasPrice,
                     BREthereumClientHandlerEstimateGas funcEstimateGas,
                     BREthereumClientHandlerSubmitTransaction funcSubmitTransaction,
                     BREthereumClientHandlerGetTransactions funcGetTransactions,
                     BREthereumClientHandlerGetLogs funcGetLogs) {
    BREthereumClient client;
    client.funcContext = context;
    client.funcGetBalance = funcGetBalance;
    client.funcGetGasPrice = funcGetGasPrice;
    client.funcEstimateGas = funcEstimateGas;
    client.funcSubmitTransaction = funcSubmitTransaction;
    client.funcGetTransactions = funcGetTransactions;
    client.funcGetLogs = funcGetLogs;
    return client;
}

//
// Light Node
//
extern BREthereumLightNode
createLightNode (BREthereumNetwork network,
                 BREthereumAccount account) {
    BREthereumLightNode node = (BREthereumLightNode) calloc (1, sizeof (struct BREthereumLightNodeRecord));
    node->state = LIGHT_NODE_CREATED;
    node->type = FIXED_LIGHT_NODE_TYPE;
    node->network = network;
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

    // Create and then start the eventHandler
    node->handlerForListener = eventHandlerCreate(listenerEventTypes, listenerEventTypesCount);
    eventHandlerStart(node->handlerForListener);

    node->handlerForMain = eventHandlerCreate(handlerEventTypes, handlerEventTypesCount);
    eventHandlerSetTimeoutDispatcher(node->handlerForMain,
                                     1000 * LIGHT_NODE_SLEEP_SECONDS,
                                     (BREventDispatcher)lightNodePeriodicDispatcher,
                                     (void*) node);
    eventHandlerStart(node->handlerForMain);
    
    // Create a default ETH wallet; other wallets will be created 'on demand'
    node->walletHoldingEther = walletCreate(node->account,
                                            node->network);
    lightNodeInsertWallet(node, node->walletHoldingEther);

    return node;
}

extern BREthereumAccount
lightNodeGetAccount (BREthereumLightNode node) {
    return node->account;
}

extern BREthereumNetwork
lightNodeGetNetwork (BREthereumLightNode node) {
    return node->network;
}

//
// Listener
//
extern BREthereumListenerId
lightNodeAddListener (BREthereumLightNode node,
                      BREthereumListenerContext context,
                      BREthereumListenerLightNodeEventHandler lightNodeEventHandler,
                      BREthereumListenerPeerEventHandler peerEventHandler,
                      BREthereumListenerWalletEventHandler walletEventHandler,
                      BREthereumListenerBlockEventHandler blockEventHandler,
                      BREthereumListenerTransactionEventHandler transactionEventHandler) {
    BREthereumListenerId lid = -1;
    BREthereumLightNodeListener listener;

    listener.context = context;
    listener.lightNodeEventHandler = lightNodeEventHandler;
    listener.peerEventHandler = peerEventHandler;
    listener.walletEventHandler = walletEventHandler;
    listener.blockEventHandler = blockEventHandler;
    listener.transactionEventHandler = transactionEventHandler;

    pthread_mutex_lock(&node->lock);
    array_add (node->listeners, listener);
    lid = (BREthereumListenerId) (array_count (node->listeners) - 1);
    pthread_mutex_unlock(&node->lock);

    return lid;
}

extern BREthereumBoolean
lightNodeHasListener (BREthereumLightNode node,
                      BREthereumListenerId lid) {
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
                         BREthereumListenerId lid) {
    if (0 <= lid && lid < array_count(node->listeners)) {
        memset (&node->listeners[lid], 0, sizeof (BREthereumLightNodeListener));
        return ETHEREUM_BOOLEAN_TRUE;
    }
    return ETHEREUM_BOOLEAN_FALSE;
}

//
// Connect // Disconnect
//

static void
lightNodePeriodicDispatcher (BREventHandler handler,
                             BRTimeoutEvent *event) {
    BREthereumLightNode node = (BREthereumLightNode) event->context;

    if (node->state != LIGHT_NODE_CONNECTED) return;

    // We'll query all transactions for this node's account.  That will give us a shot at
    // getting the nonce for the account's address correct.  We'll save all the transactions and
    // then process them into wallet as wallets exist.
    lightNodeUpdateTransactions(node);

    // Similarly, we'll query all logs for this node's account.  We'll process these into
    // (token) transactions and associate with their wallet.
    lightNodeUpdateLogs(node, -1, eventERC20Transfer);

    // For all the known wallets, get their balance.
    for (int i = 0; i < array_count(node->wallets); i++)
        lightNodeUpdateWalletBalance (node, i);
}

//static BREthereumClient nullClient;

extern BREthereumBoolean
lightNodeConnect(BREthereumLightNode node,
                 BREthereumClient client) {
    switch (node->state) {
        case LIGHT_NODE_CONNECTING:
        case LIGHT_NODE_CONNECTED:
        case LIGHT_NODE_DISCONNECTING:
            return ETHEREUM_BOOLEAN_FALSE;

        case LIGHT_NODE_CREATED:
        case LIGHT_NODE_DISCONNECTED:
        case LIGHT_NODE_ERRORED: {
            node->client = client;
            node->state = LIGHT_NODE_CONNECTED;
            return ETHEREUM_BOOLEAN_TRUE;
        }
    }
}

extern BREthereumBoolean
lightNodeDisconnect (BREthereumLightNode node) {
    node->state = LIGHT_NODE_DISCONNECTED;
    return ETHEREUM_BOOLEAN_TRUE;
}

//
// Wallet Lookup & Insert
//
extern BREthereumWallet
lightNodeLookupWallet(BREthereumLightNode node,
                      BREthereumWalletId wid) {
    BREthereumWallet wallet = NULL;

    pthread_mutex_lock(&node->lock);
    wallet = (0 <= wid && wid < array_count(node->wallets)
              ? node->wallets[wid]
              : NULL);
    pthread_mutex_unlock(&node->lock);
    return wallet;
}

extern BREthereumWalletId
lightNodeLookupWalletId(BREthereumLightNode node,
                        BREthereumWallet wallet) {
    BREthereumWalletId wid = -1;

    pthread_mutex_lock(&node->lock);
    for (int i = 0; i < array_count (node->wallets); i++)
        if (wallet == node->wallets[i]) {
            wid = i;
            break;
        }
    pthread_mutex_unlock(&node->lock);
    return wid;
}

extern BREthereumWallet
lightNodeLookupWalletByTransaction (BREthereumLightNode node,
                                    BREthereumTransaction transaction) {
    BREthereumWallet wallet = NULL;
    pthread_mutex_lock(&node->lock);
    for (int i = 0; i < array_count (node->wallets); i++)
        if (walletHasTransaction(node->wallets[i], transaction)) {
            wallet = node->wallets[i];
            break;
        }
    pthread_mutex_unlock(&node->lock);
    return wallet;
}

extern BREthereumWalletId
lightNodeInsertWallet (BREthereumLightNode node,
                       BREthereumWallet wallet) {
    BREthereumWalletId wid = -1;
    pthread_mutex_lock(&node->lock);
    array_add (node->wallets, wallet);
    wid = (BREthereumWalletId) (array_count(node->wallets) - 1);
    pthread_mutex_unlock(&node->lock);
    lightNodeListenerAnnounceWalletEvent(node, wid, WALLET_EVENT_CREATED, SUCCESS, NULL);
    return wid;
}

//
// Wallet (Actions)
//
extern BREthereumWalletId
lightNodeGetWallet(BREthereumLightNode node) {
    return lightNodeLookupWalletId (node, node->walletHoldingEther);
}

extern BREthereumWalletId
lightNodeGetWalletHoldingToken(BREthereumLightNode node,
                               BREthereumToken token) {
    BREthereumWalletId wid = -1;

    pthread_mutex_lock(&node->lock);
    for (int i = 0; i < array_count(node->wallets); i++)
        if (token == walletGetToken(node->wallets[i])) {
            wid = i;
            break;
        }

    if (-1 == wid) {
        BREthereumWallet wallet = walletCreateHoldingToken(node->account,
                                                           node->network,
                                                           token);
        wid = lightNodeInsertWallet(node, wallet);
    }

    pthread_mutex_unlock(&node->lock);
    return wid;
}


extern BREthereumTransactionId
lightNodeWalletCreateTransaction(BREthereumLightNode node,
                                 BREthereumWallet wallet,
                                 const char *recvAddress,
                                 BREthereumAmount amount) {
    BREthereumTransactionId tid = -1;
    BREthereumWalletId wid = -1;

    pthread_mutex_lock(&node->lock);

    BREthereumTransaction transaction =
      walletCreateTransaction(wallet, createAddress(recvAddress), amount);

    tid = lightNodeInsertTransaction(node, transaction);
    wid = lightNodeLookupWalletId(node, wallet);

    pthread_mutex_unlock(&node->lock);

    lightNodeListenerAnnounceTransactionEvent(node, wid, tid, TRANSACTION_EVENT_CREATED, SUCCESS, NULL);
    lightNodeListenerAnnounceTransactionEvent(node, wid, tid, TRANSACTION_EVENT_ADDED, SUCCESS, NULL);

    return tid;
}

extern void // status, error
lightNodeWalletSignTransaction(BREthereumLightNode node,
                               BREthereumWallet wallet,
                               BREthereumTransaction transaction,
                               BRKey privateKey) {
    walletSignTransactionWithPrivateKey(wallet, transaction, privateKey);
    lightNodeListenerAnnounceTransactionEvent(node,
                                              lightNodeLookupWalletId(node, wallet),
                                              lightNodeLookupTransactionId(node, transaction),
                                              TRANSACTION_EVENT_SIGNED,
                                              SUCCESS,
                                              NULL);
}

extern void // status, error
lightNodeWalletSignTransactionWithPaperKey(BREthereumLightNode node,
                                           BREthereumWallet wallet,
                                           BREthereumTransaction transaction,
                                           const char *paperKey) {
    walletSignTransaction(wallet, transaction, paperKey);
    lightNodeListenerAnnounceTransactionEvent(node,
                                              lightNodeLookupWalletId(node, wallet),
                                              lightNodeLookupTransactionId(node, transaction),
                                              TRANSACTION_EVENT_SIGNED,
                                              SUCCESS,
                                              NULL);
}

extern void // status, error
lightNodeWalletSubmitTransaction(BREthereumLightNode node,
                                 BREthereumWallet wallet,
                                 BREthereumTransaction transaction) {
    char *rawTransaction = walletGetRawTransactionHexEncoded(wallet, transaction, "0x");

    switch (node->type) {
        case NODE_TYPE_LES:
            // TODO: Fall-through on error, perhaps

        case NODE_TYPE_JSON_RPC: {
            node->client.funcSubmitTransaction
                    (node->client.funcContext,
                     node,
                     lightNodeLookupWalletId(node, wallet),
                     lightNodeLookupTransactionId(node, transaction),
                     rawTransaction,
                     ++node->requestId);

            break;
        }

        case NODE_TYPE_NONE:
            break;
    }
    free(rawTransaction);
}

extern BREthereumTransactionId *
lightNodeWalletGetTransactions(BREthereumLightNode node,
                               BREthereumWallet wallet) {
    pthread_mutex_lock(&node->lock);

    unsigned long count = walletGetTransactionCount(wallet);
    BREthereumTransactionId *transactions = calloc (count + 1, sizeof (BREthereumTransactionId));

    for (unsigned long index = 0; index < count; index++) {
        transactions [index] = lightNodeLookupTransactionId(node, walletGetTransactionByIndex(wallet, index));
    }
    transactions[count] = -1;

    pthread_mutex_unlock(&node->lock);
    return transactions;
}

extern int
lightNodeWalletGetTransactionCount(BREthereumLightNode node,
                                   BREthereumWallet wallet) {
    int count = -1;

    pthread_mutex_lock(&node->lock);
    if (NULL != wallet) count = (int) walletGetTransactionCount(wallet);
    pthread_mutex_unlock(&node->lock);

    return count;
}

extern void
lightNodeWalletSetDefaultGasLimit(BREthereumLightNode node,
                                  BREthereumWallet wallet,
                                  BREthereumGas gasLimit) {
    walletSetDefaultGasLimit(wallet, gasLimit);
    lightNodeListenerAnnounceWalletEvent(node,
                                         lightNodeLookupWalletId(node, wallet),
                                         WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED,
                                         SUCCESS,
                                         NULL);
}

extern void
lightNodeWalletSetDefaultGasPrice(BREthereumLightNode node,
                                  BREthereumWallet wallet,
                                  BREthereumGasPrice gasPrice) {
    walletSetDefaultGasPrice(wallet, gasPrice);
    lightNodeListenerAnnounceWalletEvent(node,
                                         lightNodeLookupWalletId(node, wallet),
                                         WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
                                         SUCCESS,
                                         NULL);
}

//
// Blocks
//
extern BREthereumBlock
lightNodeLookupBlockByHash(BREthereumLightNode node,
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

extern BREthereumBlock
lightNodeLookupBlock(BREthereumLightNode node,
                     BREthereumBlockId bid) {
    BREthereumBlock block = NULL;

    pthread_mutex_lock(&node->lock);
    block = (0 <= bid && bid < array_count(node->blocks)
                   ? node->blocks[bid]
                   : NULL);
    pthread_mutex_unlock(&node->lock);
    return block;
}

extern BREthereumBlockId
lightNodeLookupBlockId (BREthereumLightNode node,
                        BREthereumBlock block) {
    BREthereumBlockId bid = -1;

    pthread_mutex_lock(&node->lock);
    for (int i = 0; i < array_count(node->blocks); i++)
        if (block == node->blocks[i]) {
            bid = i;
            break;
        }
    pthread_mutex_unlock(&node->lock);
    return bid;
}

extern BREthereumBlockId
lightNodeInsertBlock (BREthereumLightNode node,
                      BREthereumBlock block) {
    BREthereumBlockId bid = -1;
    pthread_mutex_lock(&node->lock);
    array_add(node->blocks, block);
    bid = (BREthereumBlockId) (array_count(node->blocks) - 1);
    pthread_mutex_unlock(&node->lock);
    lightNodeListenerAnnounceBlockEvent(node, bid, BLOCK_EVENT_CREATED, SUCCESS, NULL);
    return bid;
}

extern uint64_t
lightNodeGetBlockHeight(BREthereumLightNode node) {
    return node->blockHeight;
}

extern void
lightNodeUpdateBlockHeight(BREthereumLightNode node,
                           uint64_t blockHeight) {
    if (blockHeight > node->blockHeight)
        node->blockHeight = blockHeight;
}

//
// Transactions Lookup & Insert
//
extern BREthereumTransaction
lightNodeLookupTransaction(BREthereumLightNode node,
                           BREthereumTransactionId tid) {
    BREthereumTransaction transaction = NULL;

    pthread_mutex_lock(&node->lock);
    transaction = (0 <= tid && tid < array_count(node->transactions)
                   ? node->transactions[tid]
                   : NULL);
    pthread_mutex_unlock(&node->lock);
    return transaction;
}

extern BREthereumTransaction
lightNodeLookupTransactionByHash(BREthereumLightNode node,
                           const BREthereumHash hash) {
    BREthereumTransaction transaction = NULL;

    pthread_mutex_lock(&node->lock);
    for (int i = 0; i < array_count(node->transactions); i++)
        if (ETHEREUM_COMPARISON_EQ == hashCompare(hash, transactionGetHash(node->transactions[i]))) {
            transaction = node->transactions[i];
            break;
        }
    pthread_mutex_unlock(&node->lock);
    return transaction;
}

extern BREthereumTransactionId
lightNodeLookupTransactionId(BREthereumLightNode node,
                           BREthereumTransaction transaction) {
    BREthereumTransactionId tid = -1;

    pthread_mutex_lock(&node->lock);
    for (int i = 0; i < array_count(node->transactions); i++)
        if (transaction == node->transactions[i]) {
            tid = i;
            break;
        }
    pthread_mutex_unlock(&node->lock);
    return tid;
}

extern BREthereumTransactionId
lightNodeInsertTransaction (BREthereumLightNode node,
                            BREthereumTransaction transaction) {
    BREthereumTransactionId tid;

    pthread_mutex_lock(&node->lock);
    array_add (node->transactions, transaction);
    tid = (BREthereumTransactionId) (array_count(node->transactions) - 1);
    pthread_mutex_unlock(&node->lock);

    return tid;
}

static void
lightNodeDeleteTransaction (BREthereumLightNode node,
                             BREthereumTransaction transaction) {
    BREthereumTransactionId tid = lightNodeLookupTransactionId(node, transaction);

    // Remove from any (and all - should be but one) wallet
    for (int wid = 0; wid < array_count(node->wallets); wid++)
        if (walletHasTransaction(node->wallets[wid], transaction)) {
            walletUnhandleTransaction(node->wallets[wid], transaction);
            lightNodeListenerAnnounceTransactionEvent(node, wid, tid, TRANSACTION_EVENT_REMOVED, SUCCESS, NULL);
        }

    // Null the node's `tid` - MUST NOT array_rm() as all `tid` holders will be dead.
    node->transactions[tid] = NULL;
}


#if defined(SUPPORT_JSON_RPC)

extern void
lightNodeFillTransactionRawData(BREthereumLightNode node,
                                BREthereumWalletId wid,
                                BREthereumTransactionId transactionId,
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
                                         BREthereumWalletId wid,
                                         BREthereumTransactionId transactionId,
                                         const char *prefix) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, transactionId);
    
    return walletGetRawTransactionHexEncoded(wallet, transaction, prefix);
}

#endif // ETHEREUM_LIGHT_NODE_USE_JSON_RPC

