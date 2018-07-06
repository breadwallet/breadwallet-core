//
//  BREthereumEWM
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
#include "BREthereumEWMPrivate.h"
#include "../event/BREvent.h"

#define EWM_SLEEP_SECONDS (5)

/* Forward Declaration */
static void
ewmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);


//
// EWM Client
//
extern BREthereumClient
ethereumClientCreate(BREthereumClientContext context,
                     BREthereumClientHandlerGetBalance funcGetBalance,
                     BREthereumClientHandlerGetGasPrice funcGetGasPrice,
                     BREthereumClientHandlerEstimateGas funcEstimateGas,
                     BREthereumClientHandlerSubmitTransaction funcSubmitTransaction,
                     BREthereumClientHandlerGetTransactions funcGetTransactions,
                     BREthereumClientHandlerGetLogs funcGetLogs,
                     BREthereumClientHandlerGetBlockNumber funcGetBlockNumber,
                     BREthereumClientHandlerGetNonce funcGetNonce) {
    
    BREthereumClient client;
    client.funcContext = context;
    client.funcGetBalance = funcGetBalance;
    client.funcGetGasPrice = funcGetGasPrice;
    client.funcEstimateGas = funcEstimateGas;
    client.funcSubmitTransaction = funcSubmitTransaction;
    client.funcGetTransactions = funcGetTransactions;
    client.funcGetLogs = funcGetLogs;
    client.funcGetBlockNumber = funcGetBlockNumber;
    client.funcGetNonce = funcGetNonce;
    return client;
}

//
// Ethereum Wallet Manager
//
extern BREthereumEWM
createEWM (BREthereumNetwork network,
           BREthereumAccount account,
           BREthereumType type,
           // serialized: headers, transactions, logs
           BREthereumSyncMode syncMode) {
    BREthereumEWM ewm = (BREthereumEWM) calloc (1, sizeof (struct BREthereumEWMRecord));
    ewm->state = LIGHT_NODE_CREATED;
    ewm->type = type;
    ewm->syncMode = syncMode;
    ewm->network = network;
    ewm->account = account;
    
    // Create the `listener` and `main` event handlers.  Do this early so that queues exist
    // for any events/callbacks generated during initialization.  The queues won't be handled
    // until ewmConnect().
    ewm->handlerForListener = eventHandlerCreate(listenerEventTypes, listenerEventTypesCount);

    ewm->handlerForMain = eventHandlerCreate(handlerEventTypes, handlerEventTypesCount);
    eventHandlerSetTimeoutDispatcher(ewm->handlerForMain,
                                     1000 * EWM_SLEEP_SECONDS,
                                     (BREventDispatcher)ewmPeriodicDispatcher,
                                     (void*) ewm);

    array_new(ewm->wallets, DEFAULT_WALLET_CAPACITY);
    array_new(ewm->transactions, DEFAULT_TRANSACTION_CAPACITY);
    array_new(ewm->blocks, DEFAULT_BLOCK_CAPACITY);
    array_new(ewm->listeners, DEFAULT_LISTENER_CAPACITY);

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&ewm->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // Create a default ETH wallet; other wallets will be created 'on demand'
    ewm->walletHoldingEther = walletCreate(ewm->account,
                                           ewm->network);
    ewmInsertWallet(ewm, ewm->walletHoldingEther);

    BREthereumBCSListener listener = {
        (BREthereumBCSCallbackContext) ewm,
        (BREthereumBCSCallbackBlockchain) ewmSignalBlockChain,
        (BREthereumBCSCallbackAccountState) ewmSignalAccountState,
        (BREthereumBCSCallbackTransaction) ewmSignalTransaction,
        (BREthereumBCSCallbackLog) ewmSignalLog,
        (BREthereumBCSCallbackSaveBlocks) ewmSignalSaveBlocks,
        (BREthereumBCSCallbackSavePeers) ewmSignalSavePeers,
        (BREthereumBCSCallbackSync) ewmSignalSync
    };

    // Create BCS - note: when BCS processes headers, transactions, etc callbacks will be made to
    // the EWM listener.
    {
        BREthereumBlockHeader *headers;
        BREthereumBlockHeader lastCheckpointHeader = blockCheckpointCreatePartialBlockHeader(blockCheckpointLookupLatest(network));

        array_new(headers, 1);
        array_add(headers, lastCheckpointHeader);

        ewm->bcs = bcsCreate(network,
                             accountGetPrimaryAddress (account),
                             listener,
                             headers,
                             NULL,
                             NULL);

        array_free(headers);
    }

    return ewm;
}

extern void
ewmDestroy (BREthereumEWM ewm) {
    ewmDisconnect(ewm);
    bcsDestroy(ewm->bcs);

    for (size_t index = 0; index < array_count(ewm->wallets); index++)
        walletRelease (ewm->wallets[index]);
    array_free(ewm->wallets);

    array_free(ewm->transactions);
    array_free(ewm->blocks);
    array_free(ewm->listeners);

    eventHandlerDestroy(ewm->handlerForListener);
    eventHandlerDestroy(ewm->handlerForMain);
    
    free (ewm);
}

extern BREthereumAccount
ewmGetAccount (BREthereumEWM ewm) {
    return ewm->account;
}

extern BREthereumNetwork
ewmGetNetwork (BREthereumEWM ewm) {
    return ewm->network;
}

// ==============================================================================================
//
// Listener
//
extern BREthereumListenerId
ewmAddListener (BREthereumEWM ewm,
                BREthereumListenerContext context,
                BREthereumListenerEWMEventHandler ewmEventHandler,
                BREthereumListenerPeerEventHandler peerEventHandler,
                BREthereumListenerWalletEventHandler walletEventHandler,
                BREthereumListenerBlockEventHandler blockEventHandler,
                BREthereumListenerTransactionEventHandler transactionEventHandler) {
    BREthereumListenerId lid = -1;
    BREthereumEWMListener listener;
    
    listener.context = context;
    listener.ewmEventHandler = ewmEventHandler;
    listener.peerEventHandler = peerEventHandler;
    listener.walletEventHandler = walletEventHandler;
    listener.blockEventHandler = blockEventHandler;
    listener.transactionEventHandler = transactionEventHandler;
    
    pthread_mutex_lock(&ewm->lock);
    array_add (ewm->listeners, listener);
    lid = (BREthereumListenerId) (array_count (ewm->listeners) - 1);
    pthread_mutex_unlock(&ewm->lock);
    
    return lid;
}

extern BREthereumBoolean
ewmHasListener (BREthereumEWM ewm,
                BREthereumListenerId lid) {
    return (0 <= lid && lid < array_count(ewm->listeners)
            && NULL != ewm->listeners[lid].context
            && (NULL != ewm->listeners[lid].walletEventHandler ||
                NULL != ewm->listeners[lid].blockEventHandler  ||
                NULL != ewm->listeners[lid].transactionEventHandler)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern BREthereumBoolean
ewmRemoveListener (BREthereumEWM ewm,
                   BREthereumListenerId lid) {
    if (0 <= lid && lid < array_count(ewm->listeners)) {
        memset (&ewm->listeners[lid], 0, sizeof (BREthereumEWMListener));
        return ETHEREUM_BOOLEAN_TRUE;
    }
    return ETHEREUM_BOOLEAN_FALSE;
}

extern void
ewmListenerHandleWalletEvent(BREthereumEWM ewm,
                             BREthereumWalletId wid,
                             BREthereumWalletEvent event,
                             BREthereumStatus status,
                             const char *errorDescription) {
    int count = (int) array_count(ewm->listeners);
    for (int i = 0; i < count; i++) {
        if (NULL != ewm->listeners[i].walletEventHandler)
            ewm->listeners[i].walletEventHandler
            (ewm->listeners[i].context,
             ewm,
             wid,
             event,
             status,
             errorDescription);
    }
}

extern void
ewmListenerHandleBlockEvent(BREthereumEWM ewm,
                            BREthereumBlockId bid,
                            BREthereumBlockEvent event,
                            BREthereumStatus status,
                            const char *errorDescription) {
    int count = (int) array_count(ewm->listeners);
    for (int i = 0; i < count; i++) {
        if (NULL != ewm->listeners[i].blockEventHandler)
            ewm->listeners[i].blockEventHandler
            (ewm->listeners[i].context,
             ewm,
             bid,
             event,
             status,
             errorDescription);
    }
}

extern void
ewmListenerHandleTransactionEvent(BREthereumEWM ewm,
                                  BREthereumWalletId wid,
                                  BREthereumTransactionId tid,
                                  BREthereumTransactionEvent event,
                                  BREthereumStatus status,
                                  const char *errorDescription) {
    int count = (int) array_count(ewm->listeners);
    for (int i = 0; i < count; i++) {
        if (NULL != ewm->listeners[i].transactionEventHandler)
            ewm->listeners[i].transactionEventHandler
            (ewm->listeners[i].context,
             ewm,
             wid,
             tid,
             event,
             status,
             errorDescription);
    }
}

extern void
ewmListenerHandlePeerEvent(BREthereumEWM ewm,
                           // BREthereumWalletId wid,
                           // BREthereumTransactionId tid,
                           BREthereumPeerEvent event,
                           BREthereumStatus status,
                           const char *errorDescription) {
    int count = (int) array_count(ewm->listeners);
    for (int i = 0; i < count; i++) {
        if (NULL != ewm->listeners[i].peerEventHandler)
            ewm->listeners[i].peerEventHandler
            (ewm->listeners[i].context,
             ewm,
             // event->wid,
             // event->tid,
             event,
             status,
             errorDescription);
    }
}

extern void
ewmListenerHandleEWMEvent(BREthereumEWM ewm,
                          // BREthereumWalletId wid,
                          // BREthereumTransactionId tid,
                          BREthereumEWMEvent event,
                          BREthereumStatus status,
                          const char *errorDescription) {
    int count = (int) array_count(ewm->listeners);
    for (int i = 0; i < count; i++) {
        if (NULL != ewm->listeners[i].ewmEventHandler)
            ewm->listeners[i].ewmEventHandler
            (ewm->listeners[i].context,
             ewm,
             //event->wid,
             // event->tid,
             event,
             status,
             errorDescription);
    }
}

extern void
ewmHandleGasPrice (BREthereumEWM ewm,
                   BREthereumWallet wallet,
                   BREthereumGasPrice gasPrice) {
    pthread_mutex_lock(&ewm->lock);
    
    walletSetDefaultGasPrice(wallet, gasPrice);
    
    ewmListenerSignalWalletEvent(ewm,
                                 ewmLookupWalletId(ewm, wallet),
                                 WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
                                 SUCCESS, NULL);
    
    pthread_mutex_unlock(&ewm->lock);
}

extern void
ewmHandleGasEstimate (BREthereumEWM ewm,
                      BREthereumWallet wallet,
                      BREthereumTransaction transaction,
                      BREthereumGas gasEstimate) {
    pthread_mutex_lock(&ewm->lock);
    
    transactionSetGasEstimate(transaction, gasEstimate);
    
    ewmListenerSignalTransactionEvent(ewm,
                                      ewmLookupWalletId(ewm, wallet),
                                      ewmLookupTransactionId (ewm, transaction),
                                      TRANSACTION_EVENT_GAS_ESTIMATE_UPDATED,
                                      SUCCESS, NULL);
    
    pthread_mutex_unlock(&ewm->lock);
    
}

// ==============================================================================================
//
// LES(BCS)/JSON_RPC Handlers
//
extern void
ewmHandleBlockChain (BREthereumEWM ewm,
                     BREthereumHash headBlockHash,
                     uint64_t headBlockNumber,
                     uint64_t headBlockTimestamp) {
    // Don't rebort during sync.
    if (ETHEREUM_BOOLEAN_IS_FALSE(bcsSyncInProgress(ewm->bcs)))
        eth_log ("EWM", "BlockChain: %llu", headBlockNumber);
}

extern void
ewmHandleAccountState (BREthereumEWM ewm,
                       BREthereumAccountState accountState) {
    pthread_mutex_lock(&ewm->lock);

    eth_log("EWM", "AccountState: Nonce: %llu", accountState.nonce);

    accountSetAddressNonce(ewm->account, accountGetPrimaryAddress(ewm->account),
                           accountState.nonce,
                           ETHEREUM_BOOLEAN_FALSE);

    ewmSignalBalance(ewm, amountCreateEther(accountState.balance));
    pthread_mutex_unlock(&ewm->lock);
}

extern void
ewmHandleBalance (BREthereumEWM ewm,
                  BREthereumAmount amount) {
    pthread_mutex_lock(&ewm->lock);

    BREthereumWalletId wid = (AMOUNT_ETHER == amountGetType(amount)
                              ? ewmGetWallet(ewm)
                              : ewmGetWalletHoldingToken(ewm, amountGetToken (amount)));
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);

    walletSetBalance(wallet, amount);

    ewmListenerSignalWalletEvent(ewm, wid, WALLET_EVENT_BALANCE_UPDATED,
                                 SUCCESS,
                                 NULL);

    pthread_mutex_unlock(&ewm->lock);
}

extern void
ewmHandleTransaction (BREthereumEWM ewm,
                      BREthereumBCSCallbackTransactionType type,
                      BREthereumTransaction transaction) {
    
    pthread_mutex_lock(&ewm->lock);

    eth_log("EWM", "Transaction: %d", 0);

    // Find the wallet
    BREthereumAmount amount = transactionGetAmount(transaction);
    BREthereumToken token =  (AMOUNT_TOKEN == amountGetType(amount) ? amountGetToken(amount) : NULL);
    
    BREthereumWalletId wid = (NULL == token ? 0 : ewmGetWalletHoldingToken(ewm, token));
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    assert (NULL != wallet);
    
    BREthereumTransactionId tid = ewmLookupTransactionId(ewm, transaction);
    
    // If `transaction` is new, then add it to the EWM
    if (-1 == tid)
        tid = ewmInsertTransaction(ewm, transaction);
    
    // We have a hash?  We had a hash all along?
    // walletTransactionSubmitted(wallet, transaction, transactionGetHash(transaction));
    
    // If `transaction` is not yet held in `wallet`
    if (!walletHasTransaction(wallet, transaction)) {
        //
        //  a) add to the wallet
        walletHandleTransaction(wallet, transaction);
        //
        //  b) announce the wallet update
        //  TODO: Need a hash here?
        ewmListenerSignalTransactionEvent(ewm, wid, tid,
                                          TRANSACTION_EVENT_ADDED,
                                          SUCCESS, NULL);
    }
    
    BREthereumTransactionStatus status = transactionGetStatus(transaction);
    switch (status.type) {
        case TRANSACTION_STATUS_UNKNOWN:
            break;
        case TRANSACTION_STATUS_QUEUED:
        case TRANSACTION_STATUS_PENDING:
            break;
        case TRANSACTION_STATUS_INCLUDED:
            ewmListenerSignalTransactionEvent(ewm, wid, tid,
                                              (ewmGetBlockHeight(ewm) == status.u.included.blockNumber
                                               ? TRANSACTION_EVENT_BLOCKED
                                               : TRANSACTION_EVENT_BLOCK_CONFIRMATIONS_UPDATED),
                                              SUCCESS, NULL);
            break;
        case TRANSACTION_STATUS_ERRORED:
            ewmListenerSignalTransactionEvent(ewm, wid, tid,
                                              (ewmGetBlockHeight(ewm) == status.u.included.blockNumber
                                               ? TRANSACTION_EVENT_BLOCKED
                                               : TRANSACTION_EVENT_BLOCK_CONFIRMATIONS_UPDATED),
                                              ERROR_TRANSACTION_SUBMISSION,
                                              status.u.errored.reason);
            break;
            
        case TRANSACTION_STATUS_CREATED:
        case TRANSACTION_STATUS_SIGNED:
        case TRANSACTION_STATUS_SUBMITTED:
            break;
    }
    /*
     //
     // TODO: Do we get 'included' before or after we see transaction, in the block body?
     //  If we get 'included' we should ignore because a) we'll see the transaction
     //  later and b) we don't have any block information to provide in transaction anyway.
     //
     switch (status.type) {
     case TRANSACTION_STATUS_UNKNOWN:
     break;
     case TRANSACTION_STATUS_QUEUED:
     break;
     case TRANSACTION_STATUS_PENDING:
     transactionAnnounceSubmitted(transaction, transactionHash);
     // TRANSACTION_EVENT_SUBMITTED, SUCCESS
     break;
     
     case TRANSACTION_STATUS_INCLUDED: {
     BREthereumBlockHeader header = NULL; // status.u.included.blockHash
     transactionAnnounceBlocked(transaction,
     gasCreate(0),
     status.u.included.blockHash,
     status.u.included.blockNumber,
     status.u.included.transactionIndex);
     }
     break;
     
     case TRANSACTION_STATUS_ERROR:
     transactionAnnounceDropped(transaction, 0);
     // TRANSACTION_EVENT_SUBMITTED, ERROR_TRANSACTION_SUBMISSION, event->status.u.error.message
     break;
     
     case TRANSACTION_STATUS_CREATED:
     case TRANSACTION_STATUS_SIGNED:
     case TRANSACTION_STATUS_SUBMITTED:
     // TODO: DO
     break;
     }
     */
}

extern void
ewmHandleLog (BREthereumEWM ewm,
              BREthereumBCSCallbackLogType type,
              BREthereumLog log) {
    eth_log("EWM", "Log: %d", 0);
}

extern void
ewmHandleSaveBlocks (BREthereumEWM ewm,
                     BRArrayOf(BREthereumBlock) blocks) {
    eth_log("EWM", "Save Blocks: %zu", array_count(blocks));
    array_free(blocks);
}

extern void
ewmHandleSavePeers (BREthereumEWM ewm) {
    eth_log("EWM", "Save Peers: %d",0);
}

extern void
ewmHandleSync (BREthereumEWM ewm,
               BREthereumBCSCallbackSyncType type,
               uint64_t blockNumberStart,
               uint64_t blockNumberCurrent,
               uint64_t blockNumberStop) {
    eth_log ("EWM", "Sync: %d, %llu%%", type, (100 * (blockNumberCurrent - blockNumberStart) / (blockNumberStop - blockNumberStart)));
}
//
// Connect // Disconnect
//

static void
ewmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event) {
    BREthereumEWM ewm = (BREthereumEWM) event->context;
    
    if (ewm->state != LIGHT_NODE_CONNECTED) return;
    if (ewm->type  == NODE_TYPE_LES) return;
    
    ewmUpdateBlockNumber(ewm);
    ewmUpdateNonce(ewm);
    
    // We'll query all transactions for this ewm's account.  That will give us a shot at
    // getting the nonce for the account's address correct.  We'll save all the transactions and
    // then process them into wallet as wallets exist.
    ewmUpdateTransactions(ewm);
    
    // Similarly, we'll query all logs for this ewm's account.  We'll process these into
    // (token) transactions and associate with their wallet.
    ewmUpdateLogs(ewm, -1, eventERC20Transfer);
    
    // For all the known wallets, get their balance.
    for (int i = 0; i < array_count(ewm->wallets); i++)
        ewmUpdateWalletBalance (ewm, i);
}

//static BREthereumClient nullClient;

extern BREthereumBoolean
ewmConnect(BREthereumEWM ewm,
           BREthereumClient client) {
    if (ETHEREUM_BOOLEAN_IS_TRUE(bcsIsStarted(ewm->bcs)))
        return ETHEREUM_BOOLEAN_FALSE;
    
    // Set ewm {client,state} prior to bcs/event start.  Avoid race conditions, particularly
    // with `ewmPeriodicDispatcher`.
    ewm->client = client;
    ewm->state = LIGHT_NODE_CONNECTED;
    bcsStart(ewm->bcs);
    eventHandlerStart(ewm->handlerForListener);
    eventHandlerStart(ewm->handlerForMain);
    return ETHEREUM_BOOLEAN_TRUE;
}

extern BREthereumBoolean
ewmDisconnect (BREthereumEWM ewm) {
    if (ETHEREUM_BOOLEAN_IS_TRUE(bcsIsStarted(ewm->bcs))) {
        // Set ewm->state thereby stopping handlers (in a race with bcs/event calls).
        ewm->state = LIGHT_NODE_DISCONNECTED;
        bcsStop(ewm->bcs);
        eventHandlerStop(ewm->handlerForMain);
        eventHandlerStop(ewm->handlerForListener);
    }
    return ETHEREUM_BOOLEAN_TRUE;
}

//
// Wallet Lookup & Insert
//
extern BREthereumWallet
ewmLookupWallet(BREthereumEWM ewm,
                BREthereumWalletId wid) {
    BREthereumWallet wallet = NULL;
    
    pthread_mutex_lock(&ewm->lock);
    wallet = (0 <= wid && wid < array_count(ewm->wallets)
              ? ewm->wallets[wid]
              : NULL);
    pthread_mutex_unlock(&ewm->lock);
    return wallet;
}

extern BREthereumWalletId
ewmLookupWalletId(BREthereumEWM ewm,
                  BREthereumWallet wallet) {
    BREthereumWalletId wid = -1;
    
    pthread_mutex_lock(&ewm->lock);
    for (int i = 0; i < array_count (ewm->wallets); i++)
        if (wallet == ewm->wallets[i]) {
            wid = i;
            break;
        }
    pthread_mutex_unlock(&ewm->lock);
    return wid;
}

extern BREthereumWallet
ewmLookupWalletByTransaction (BREthereumEWM ewm,
                              BREthereumTransaction transaction) {
    BREthereumWallet wallet = NULL;
    pthread_mutex_lock(&ewm->lock);
    for (int i = 0; i < array_count (ewm->wallets); i++)
        if (walletHasTransaction(ewm->wallets[i], transaction)) {
            wallet = ewm->wallets[i];
            break;
        }
    pthread_mutex_unlock(&ewm->lock);
    return wallet;
}

extern BREthereumWalletId
ewmInsertWallet (BREthereumEWM ewm,
                 BREthereumWallet wallet) {
    BREthereumWalletId wid = -1;
    pthread_mutex_lock(&ewm->lock);
    array_add (ewm->wallets, wallet);
    wid = (BREthereumWalletId) (array_count(ewm->wallets) - 1);
    pthread_mutex_unlock(&ewm->lock);
    ewmListenerSignalWalletEvent(ewm, wid, WALLET_EVENT_CREATED, SUCCESS, NULL);
    return wid;
}

//
// Wallet (Actions)
//
extern BREthereumWalletId
ewmGetWallet(BREthereumEWM ewm) {
    return ewmLookupWalletId (ewm, ewm->walletHoldingEther);
}

extern BREthereumWalletId
ewmGetWalletHoldingToken(BREthereumEWM ewm,
                         BREthereumToken token) {
    BREthereumWalletId wid = -1;
    
    pthread_mutex_lock(&ewm->lock);
    for (int i = 0; i < array_count(ewm->wallets); i++)
        if (token == walletGetToken(ewm->wallets[i])) {
            wid = i;
            break;
        }
    
    if (-1 == wid) {
        BREthereumWallet wallet = walletCreateHoldingToken(ewm->account,
                                                           ewm->network,
                                                           token);
        wid = ewmInsertWallet(ewm, wallet);
    }
    
    pthread_mutex_unlock(&ewm->lock);
    return wid;
}


extern BREthereumTransactionId
ewmWalletCreateTransaction(BREthereumEWM ewm,
                           BREthereumWallet wallet,
                           const char *recvAddress,
                           BREthereumAmount amount) {
    BREthereumTransactionId tid = -1;
    BREthereumWalletId wid = -1;
    
    pthread_mutex_lock(&ewm->lock);
    
    BREthereumTransaction transaction =
    walletCreateTransaction(wallet, addressCreate(recvAddress), amount);
    
    tid = ewmInsertTransaction(ewm, transaction);
    wid = ewmLookupWalletId(ewm, wallet);
    
    pthread_mutex_unlock(&ewm->lock);
    
    ewmListenerSignalTransactionEvent(ewm, wid, tid, TRANSACTION_EVENT_CREATED, SUCCESS, NULL);
    ewmListenerSignalTransactionEvent(ewm, wid, tid, TRANSACTION_EVENT_ADDED, SUCCESS, NULL);
    
    return tid;
}

extern void // status, error
ewmWalletSignTransaction(BREthereumEWM ewm,
                         BREthereumWallet wallet,
                         BREthereumTransaction transaction,
                         BRKey privateKey) {
    walletSignTransactionWithPrivateKey(wallet, transaction, privateKey);
    ewmListenerSignalTransactionEvent(ewm,
                                      ewmLookupWalletId(ewm, wallet),
                                      ewmLookupTransactionId(ewm, transaction),
                                      TRANSACTION_EVENT_SIGNED,
                                      SUCCESS,
                                      NULL);
}

extern void // status, error
ewmWalletSignTransactionWithPaperKey(BREthereumEWM ewm,
                                     BREthereumWallet wallet,
                                     BREthereumTransaction transaction,
                                     const char *paperKey) {
    walletSignTransaction(wallet, transaction, paperKey);
    ewmListenerSignalTransactionEvent(ewm,
                                      ewmLookupWalletId(ewm, wallet),
                                      ewmLookupTransactionId(ewm, transaction),
                                      TRANSACTION_EVENT_SIGNED,
                                      SUCCESS,
                                      NULL);
}

extern void // status, error
ewmWalletSubmitTransaction(BREthereumEWM ewm,
                           BREthereumWallet wallet,
                           BREthereumTransaction transaction) {
    char *rawTransaction = walletGetRawTransactionHexEncoded(wallet, transaction, "0x");
    
    switch (ewm->type) {
        case NODE_TYPE_LES:
            bcsSendTransaction(ewm->bcs, transaction);
            // TODO: Fall-through on error, perhaps
            break;
            
        case NODE_TYPE_JSON_RPC: {
            ewm->client.funcSubmitTransaction
            (ewm->client.funcContext,
             ewm,
             ewmLookupWalletId(ewm, wallet),
             ewmLookupTransactionId(ewm, transaction),
             rawTransaction,
             ++ewm->requestId);
            
            break;
        }
            
        case NODE_TYPE_NONE:
            break;
    }
    free(rawTransaction);
}

extern BREthereumTransactionId *
ewmWalletGetTransactions(BREthereumEWM ewm,
                         BREthereumWallet wallet) {
    pthread_mutex_lock(&ewm->lock);
    
    unsigned long count = walletGetTransactionCount(wallet);
    BREthereumTransactionId *transactions = calloc (count + 1, sizeof (BREthereumTransactionId));
    
    for (unsigned long index = 0; index < count; index++) {
        transactions [index] = ewmLookupTransactionId(ewm, walletGetTransactionByIndex(wallet, index));
    }
    transactions[count] = -1;
    
    pthread_mutex_unlock(&ewm->lock);
    return transactions;
}

extern int
ewmWalletGetTransactionCount(BREthereumEWM ewm,
                             BREthereumWallet wallet) {
    int count = -1;
    
    pthread_mutex_lock(&ewm->lock);
    if (NULL != wallet) count = (int) walletGetTransactionCount(wallet);
    pthread_mutex_unlock(&ewm->lock);
    
    return count;
}

extern void
ewmWalletSetDefaultGasLimit(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumGas gasLimit) {
    walletSetDefaultGasLimit(wallet, gasLimit);
    ewmListenerSignalWalletEvent(ewm,
                                 ewmLookupWalletId(ewm, wallet),
                                 WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED,
                                 SUCCESS,
                                 NULL);
}

extern void
ewmWalletSetDefaultGasPrice(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumGasPrice gasPrice) {
    walletSetDefaultGasPrice(wallet, gasPrice);
    ewmListenerSignalWalletEvent(ewm,
                                 ewmLookupWalletId(ewm, wallet),
                                 WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
                                 SUCCESS,
                                 NULL);
}

//
// Blocks
//
extern BREthereumBlock
ewmLookupBlockByHash(BREthereumEWM ewm,
                     const BREthereumHash hash) {
    BREthereumBlock block = NULL;
    
    pthread_mutex_lock(&ewm->lock);
    for (int i = 0; i < array_count(ewm->blocks); i++)
        if (ETHEREUM_COMPARISON_EQ == hashCompare(hash, blockGetHash(ewm->blocks[i]))) {
            block = ewm->blocks[i];
            break;
        }
    pthread_mutex_unlock(&ewm->lock);
    return block;
}

extern BREthereumBlock
ewmLookupBlock(BREthereumEWM ewm,
               BREthereumBlockId bid) {
    BREthereumBlock block = NULL;
    
    pthread_mutex_lock(&ewm->lock);
    block = (0 <= bid && bid < array_count(ewm->blocks)
             ? ewm->blocks[bid]
             : NULL);
    pthread_mutex_unlock(&ewm->lock);
    return block;
}

extern BREthereumBlockId
ewmLookupBlockId (BREthereumEWM ewm,
                  BREthereumBlock block) {
    BREthereumBlockId bid = -1;
    
    pthread_mutex_lock(&ewm->lock);
    for (int i = 0; i < array_count(ewm->blocks); i++)
        if (block == ewm->blocks[i]) {
            bid = i;
            break;
        }
    pthread_mutex_unlock(&ewm->lock);
    return bid;
}

extern BREthereumBlockId
ewmInsertBlock (BREthereumEWM ewm,
                BREthereumBlock block) {
    BREthereumBlockId bid = -1;
    pthread_mutex_lock(&ewm->lock);
    array_add(ewm->blocks, block);
    bid = (BREthereumBlockId) (array_count(ewm->blocks) - 1);
    pthread_mutex_unlock(&ewm->lock);
    ewmListenerSignalBlockEvent(ewm, bid, BLOCK_EVENT_CREATED, SUCCESS, NULL);
    return bid;
}

extern uint64_t
ewmGetBlockHeight(BREthereumEWM ewm) {
    return ewm->blockHeight;
}

extern void
ewmUpdateBlockHeight(BREthereumEWM ewm,
                     uint64_t blockHeight) {
    if (blockHeight > ewm->blockHeight)
        ewm->blockHeight = blockHeight;
}

//
// Transactions Lookup & Insert
//
extern BREthereumTransaction
ewmLookupTransaction(BREthereumEWM ewm,
                     BREthereumTransactionId tid) {
    BREthereumTransaction transaction = NULL;
    
    pthread_mutex_lock(&ewm->lock);
    transaction = (0 <= tid && tid < array_count(ewm->transactions)
                   ? ewm->transactions[tid]
                   : NULL);
    pthread_mutex_unlock(&ewm->lock);
    return transaction;
}

extern BREthereumTransaction
ewmLookupTransactionByHash(BREthereumEWM ewm,
                           const BREthereumHash hash) {
    BREthereumTransaction transaction = NULL;
    
    pthread_mutex_lock(&ewm->lock);
    for (int i = 0; i < array_count(ewm->transactions); i++)
        if (ETHEREUM_COMPARISON_EQ == hashCompare(hash, transactionGetHash(ewm->transactions[i]))) {
            transaction = ewm->transactions[i];
            break;
        }
    pthread_mutex_unlock(&ewm->lock);
    return transaction;
}

extern BREthereumTransactionId
ewmLookupTransactionId(BREthereumEWM ewm,
                       BREthereumTransaction transaction) {
    BREthereumTransactionId tid = -1;
    
    pthread_mutex_lock(&ewm->lock);
    for (int i = 0; i < array_count(ewm->transactions); i++)
        if (transaction == ewm->transactions[i]) {
            tid = i;
            break;
        }
    pthread_mutex_unlock(&ewm->lock);
    return tid;
}

extern BREthereumTransactionId
ewmInsertTransaction (BREthereumEWM ewm,
                      BREthereumTransaction transaction) {
    BREthereumTransactionId tid;
    
    pthread_mutex_lock(&ewm->lock);
    array_add (ewm->transactions, transaction);
    tid = (BREthereumTransactionId) (array_count(ewm->transactions) - 1);
    pthread_mutex_unlock(&ewm->lock);
    
    return tid;
}

extern void
ewmDeleteTransaction (BREthereumEWM ewm,
                      BREthereumTransactionId tid) {
    BREthereumTransaction transaction = ewm->transactions[tid];
    if (NULL == transaction) return;

    // Remove from any (and all - should be but one) wallet
    for (int wid = 0; wid < array_count(ewm->wallets); wid++)
        if (walletHasTransaction(ewm->wallets[wid], transaction)) {
            walletUnhandleTransaction(ewm->wallets[wid], transaction);
            ewmListenerSignalTransactionEvent(ewm, wid, tid, TRANSACTION_EVENT_REMOVED, SUCCESS, NULL);
        }
    
    // Null the ewm's `tid` - MUST NOT array_rm() as all `tid` holders will be dead.
    ewm->transactions[tid] = NULL;
    transactionRelease(transaction);
}


#if defined(SUPPORT_JSON_RPC)

extern void
ewmFillTransactionRawData(BREthereumEWM ewm,
                          BREthereumWalletId wid,
                          BREthereumTransactionId transactionId,
                          uint8_t **bytesPtr, size_t *bytesCountPtr) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, transactionId);
    
    assert (NULL != bytesCountPtr && NULL != bytesPtr);
    assert (ETHEREUM_BOOLEAN_IS_TRUE (transactionIsSigned(transaction)));
    
    BRRlpData rawTransactionData =
    walletGetRawTransaction(wallet, transaction);
    
    *bytesCountPtr = rawTransactionData.bytesCount;
    *bytesPtr = (uint8_t *) malloc (*bytesCountPtr);
    memcpy (*bytesPtr, rawTransactionData.bytes, *bytesCountPtr);
}

extern const char *
ewmGetTransactionRawDataHexEncoded(BREthereumEWM ewm,
                                   BREthereumWalletId wid,
                                   BREthereumTransactionId transactionId,
                                   const char *prefix) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    BREthereumTransaction transaction = ewmLookupTransaction(ewm, transactionId);
    
    return walletGetRawTransactionHexEncoded(wallet, transaction, prefix);
}

#endif // ETHEREUM_LIGHT_NODE_USE_JSON_RPC


//
// BCS Listener Callbacks
//
static void
ewmBCSListenerTransactionCallback (BREthereumEWM ewm,
                                   BREthereumBCS bcs,
                                   BREthereumTransaction transaction) {
    
}
