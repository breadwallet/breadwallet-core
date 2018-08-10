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
// Ethereum Wallet Manager
//

///
/// MARK: - Create EWM
///
static BRArrayOf(BREthereumBlock)
createEWMEnsureBlocks (BRArrayOf(BREthereumPersistData) blocksPersistData,
                       BREthereumNetwork network,
                       BRRlpCoder coder) {
    BRArrayOf(BREthereumBlock) blocks;

    if (NULL == blocksPersistData || array_count(blocksPersistData) == 0) {
        array_new(blocks, 1);
        BREthereumBlockHeader lastCheckpointHeader = blockCheckpointCreatePartialBlockHeader(blockCheckpointLookupLatest(network));
        array_add(blocks, blockCreate(lastCheckpointHeader));
    }
    else {
        array_new(blocks, array_count(blocksPersistData));

        for (size_t index = 0; index < array_count(blocksPersistData); index++) {
            BRRlpItem item = rlpGetItem(coder, blocksPersistData[index].blob);
            BREthereumBlock block = blockRlpDecode(item, network, RLP_TYPE_ARCHIVE, coder);
            rlpReleaseItem(coder, item);
            array_insert (blocks, index, block);
        }
    }

    if (NULL != blocksPersistData) {
        array_free(blocksPersistData);
    }

    return blocks;
}

static BRArrayOf(BREthereumLESPeerConfig)
createEWMEnsurePeers (BRArrayOf(BREthereumPersistData) peersPersistData,
                      BRRlpCoder coder) {
    BRArrayOf(BREthereumLESPeerConfig) peers;

    size_t peersCount = (NULL == peersPersistData ? 0 : array_count(peersPersistData));
    array_new(peers, peersCount);

    for (size_t index = 0; index < peersCount; index++) {
        //BREthereumPersistData persistData = peersPersistData[index];
        ; // Create PeerConfig from PersistData; then array_add
    }

    if (NULL != peersPersistData) {
        array_free (peersPersistData);
    }

    return peers;
}

static BRArrayOf(BREthereumTransaction)
createEWMEnsureTransactions (BRArrayOf(BREthereumPersistData) transactionsPersistData,
                             BREthereumNetwork network,
                             BRRlpCoder coder) {
    BRArrayOf(BREthereumTransaction) transactions;

    size_t transactionsCount = (NULL == transactionsPersistData ? 0 : array_count(transactionsPersistData));
    array_new(transactions, transactionsCount);

    for (size_t index = 0; index < transactionsCount; index++) {
        fprintf (stdout, "ETH: TST: EnsureTrans @ %p\n", transactionsPersistData[index].blob.bytes);

        BRRlpItem item = rlpGetItem(coder, transactionsPersistData[index].blob);
        BREthereumTransaction transaction = transactionRlpDecode(item, network, RLP_TYPE_ARCHIVE, coder);
        rlpReleaseItem(coder, item);
        array_insert (transactions, index, transaction);
    }

    if (NULL != transactionsPersistData) {
        array_free(transactionsPersistData);
    }

    return transactions;
}

static BRArrayOf(BREthereumLog)
createEWMEnsureLogs(BRArrayOf(BREthereumPersistData) logsPersistData,
                    BREthereumNetwork network,
                    BRRlpCoder coder) {
    BRArrayOf(BREthereumLog) logs;

    size_t logsCount = (NULL == logsPersistData ? 0 : array_count(logsPersistData));
    array_new(logs, logsCount);

    for (size_t index = 0; index < logsCount; index++) {
        fprintf (stdout, "ETH: TST: EnsureLogs @ %p\n", logsPersistData[index].blob.bytes);

        BRRlpItem item = rlpGetItem(coder, logsPersistData[index].blob);
        BREthereumLog log = logRlpDecode(item, RLP_TYPE_ARCHIVE, coder);
        rlpReleaseItem(coder, item);
        array_insert (logs, index, log);
    }

    if (NULL != logsPersistData) {
        array_free(logsPersistData);
    }

    return logs;
}

extern BREthereumEWM
createEWM (BREthereumNetwork network,
           BREthereumAccount account,
           BREthereumType type,
           // serialized: headers, transactions, logs
           BREthereumSyncMode syncMode,
           BREthereumClient client,
           BRArrayOf(BREthereumPersistData) peersPersistData,
           BRArrayOf(BREthereumPersistData) blocksPersistData,
           BRArrayOf(BREthereumPersistData) transactionsPersistData,
           BRArrayOf(BREthereumPersistData) logsPersistData) {
    BREthereumEWM ewm = (BREthereumEWM) calloc (1, sizeof (struct BREthereumEWMRecord));

    ewm->state = LIGHT_NODE_CREATED;
    ewm->type = type;
    ewm->syncMode = syncMode;
    ewm->network = network;
    ewm->account = account;

    // Get the client assigned early; callbacks as EWM/BCS state is re-establish, regarding
    // blocks, peers, transactions and logs, will be invoked.
    ewm->client = client;

    // Our one and only coder
    ewm->coder = rlpCoderCreate();

    // Create the `listener` and `main` event handlers.  Do this early so that queues exist
    // for any events/callbacks generated during initialization.  The queues won't be handled
    // until ewmConnect().
    ewm->handlerForClient = eventHandlerCreate(handlerForClientEventTypes, handlerForClientEventTypesCount);

    // The `main` event handler has a periodic wake-up.  Used, perhaps, if the syncMode indicates
    // that we should/might query the BRD backend services.
    ewm->handlerForMain = eventHandlerCreate(handlerForMainEventTypes, handlerForMainEventTypesCount);
    eventHandlerSetTimeoutDispatcher(ewm->handlerForMain,
                                     1000 * EWM_SLEEP_SECONDS,
                                     (BREventDispatcher)ewmPeriodicDispatcher,
                                     (void*) ewm);

    array_new(ewm->wallets, DEFAULT_WALLET_CAPACITY);
    array_new(ewm->transfers, DEFAULT_TRANSACTION_CAPACITY);
    array_new(ewm->blocks, DEFAULT_BLOCK_CAPACITY);

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

    // Create the BCS listener - allows EWM to handle block, peer, transaction and log events.
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

    // Create BCS - note: when BCS processes blocks, peers, transactions, and logs, callbacks will
    // be made to the EWM client.
    
    // Might need an argument related to `syncMode` - telling BCS, for example, to use LES,
    // or not to use LES and instead rely on `client` (or some manifestation of `client`).
    ewm->bcs = bcsCreate(network,
                         accountGetPrimaryAddress (account),
                         listener,
                         createEWMEnsurePeers(peersPersistData, ewm->coder),
                         createEWMEnsureBlocks (blocksPersistData, network, ewm->coder),
                         createEWMEnsureTransactions(transactionsPersistData, network, ewm->coder),
                         createEWMEnsureLogs(logsPersistData, network, ewm->coder));
    
    return ewm;
}

extern void
ewmDestroy (BREthereumEWM ewm) {
    ewmDisconnect(ewm);
    bcsDestroy(ewm->bcs);

    for (size_t index = 0; index < array_count(ewm->wallets); index++)
        walletRelease (ewm->wallets[index]);
    array_free(ewm->wallets);

    array_free(ewm->transfers);
    array_free(ewm->blocks);

    eventHandlerDestroy(ewm->handlerForClient);
    eventHandlerDestroy(ewm->handlerForMain);
    rlpCoderRelease(ewm->coder);
    
    free (ewm);
}

///
/// MARK: - Connect / Disconnect
///
extern BREthereumBoolean
ewmConnect(BREthereumEWM ewm) {
    if (ETHEREUM_BOOLEAN_IS_TRUE(bcsIsStarted(ewm->bcs)))
        return ETHEREUM_BOOLEAN_FALSE;

    // Set ewm {client,state} prior to bcs/event start.  Avoid race conditions, particularly
    // with `ewmPeriodicDispatcher`.
    ewm->state = LIGHT_NODE_CONNECTED;
    bcsStart(ewm->bcs);
    eventHandlerStart(ewm->handlerForClient);
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
        eventHandlerStop(ewm->handlerForClient);
    }
    return ETHEREUM_BOOLEAN_TRUE;
}


extern BREthereumAccount
ewmGetAccount (BREthereumEWM ewm) {
    return ewm->account;
}

extern BREthereumNetwork
ewmGetNetwork (BREthereumEWM ewm) {
    return ewm->network;
}

///
/// MARK: - Blocks
///

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
    ewmClientSignalBlockEvent(ewm, bid, BLOCK_EVENT_CREATED, SUCCESS, NULL);
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

///
/// MARK: - Transfers
///

extern BREthereumTransfer
ewmLookupTransfer (BREthereumEWM ewm,
                   BREthereumTransferId tid) {
    BREthereumTransfer transfer = NULL;

    pthread_mutex_lock(&ewm->lock);
    transfer = (0 <= tid && tid < array_count(ewm->transfers)
                ? ewm->transfers[tid]
                : NULL);
    pthread_mutex_unlock(&ewm->lock);
    return transfer;
}

extern BREthereumTransfer
ewmLookupTransferByHash (BREthereumEWM ewm,
                         const BREthereumHash hash) {
    BREthereumTransfer transfer = NULL;

    pthread_mutex_lock(&ewm->lock);
    for (int i = 0; i < array_count(ewm->transfers); i++)
        if (ETHEREUM_COMPARISON_EQ == hashCompare(hash, transferGetHash(ewm->transfers[i]))) {
            transfer = ewm->transfers[i];
            break;
        }
    pthread_mutex_unlock(&ewm->lock);
    return transfer;
}

extern BREthereumTransferId
ewmLookupTransferId (BREthereumEWM ewm,
                     BREthereumTransfer transfer) {
    BREthereumTransferId tid = -1;

    pthread_mutex_lock(&ewm->lock);
    for (int i = 0; i < array_count(ewm->transfers); i++)
        if (transfer == ewm->transfers[i]) {
            tid = i;
            break;
        }
    pthread_mutex_unlock(&ewm->lock);
    return tid;
}

extern BREthereumTransferId
ewmInsertTransfer (BREthereumEWM ewm,
                   BREthereumTransfer transfer) {
    BREthereumTransferId tid;

    pthread_mutex_lock(&ewm->lock);
    array_add (ewm->transfers, transfer);
    tid = (BREthereumTransferId) (array_count(ewm->transfers) - 1);
    pthread_mutex_unlock(&ewm->lock);

    return tid;
}

extern void
ewmDeleteTransfer (BREthereumEWM ewm,
                   BREthereumTransferId tid) {
    BREthereumTransfer transfer = ewm->transfers[tid];
    if (NULL == transfer) return;

    // Remove from any (and all - should be but one) wallet
    for (int wid = 0; wid < array_count(ewm->wallets); wid++)
        if (walletHasTransfer(ewm->wallets[wid], transfer)) {
            walletUnhandleTransfer(ewm->wallets[wid], transfer);
            ewmClientSignalTransferEvent(ewm, wid, tid, TRANSFER_EVENT_DELETED, SUCCESS, NULL);
        }

    // Null the ewm's `tid` - MUST NOT array_rm() as all `tid` holders will be dead.
    ewm->transfers[tid] = NULL;
    transferRelease(transfer);
}

///
/// MARK: Wallets
///

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
ewmLookupWalletByTransfer (BREthereumEWM ewm,
                           BREthereumTransfer transfer) {
    BREthereumWallet wallet = NULL;
    pthread_mutex_lock(&ewm->lock);
    for (int i = 0; i < array_count (ewm->wallets); i++)
        if (walletHasTransfer(ewm->wallets[i], transfer)) {
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
    ewmClientSignalWalletEvent(ewm, wid, WALLET_EVENT_CREATED, SUCCESS, NULL);
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


extern BREthereumTransferId
ewmWalletCreateTransfer(BREthereumEWM ewm,
                        BREthereumWallet wallet,
                        const char *recvAddress,
                        BREthereumAmount amount) {
    BREthereumTransferId tid = -1;
    BREthereumWalletId wid = -1;

    pthread_mutex_lock(&ewm->lock);

    BREthereumTransfer transaction = walletCreateTransfer(wallet, addressCreate(recvAddress), amount);

    tid = ewmInsertTransfer(ewm, transaction);
    wid = ewmLookupWalletId(ewm, wallet);

    pthread_mutex_unlock(&ewm->lock);

    ewmClientSignalTransferEvent(ewm, wid, tid, TRANSFER_EVENT_CREATED, SUCCESS, NULL);

    return tid;
}

extern void // status, error
ewmWalletSignTransfer(BREthereumEWM ewm,
                      BREthereumWallet wallet,
                      BREthereumTransfer transfer,
                      BRKey privateKey) {
    walletSignTransferWithPrivateKey (wallet, transfer, privateKey);
    ewmClientSignalTransferEvent (ewm,
                                  ewmLookupWalletId(ewm, wallet),
                                  ewmLookupTransferId(ewm, transfer),
                                  TRANSFER_EVENT_SIGNED,
                                  SUCCESS,
                                  NULL);
}

extern void // status, error
ewmWalletSignTransferWithPaperKey(BREthereumEWM ewm,
                                  BREthereumWallet wallet,
                                  BREthereumTransfer transfer,
                                  const char *paperKey) {
    walletSignTransfer (wallet, transfer, paperKey);
    ewmClientSignalTransferEvent (ewm,
                                  ewmLookupWalletId(ewm, wallet),
                                  ewmLookupTransferId(ewm, transfer),
                                  TRANSFER_EVENT_SIGNED,
                                  SUCCESS,
                                  NULL);
}

extern BREthereumTransferId *
ewmWalletGetTransfers(BREthereumEWM ewm,
                      BREthereumWallet wallet) {
    pthread_mutex_lock(&ewm->lock);

    unsigned long count = walletGetTransferCount(wallet);
    BREthereumTransferId *transactions = calloc (count + 1, sizeof (BREthereumTransferId));

    for (unsigned long index = 0; index < count; index++) {
        transactions [index] = ewmLookupTransferId(ewm, walletGetTransferByIndex(wallet, index));
    }
    transactions[count] = -1;

    pthread_mutex_unlock(&ewm->lock);
    return transactions;
}

extern int
ewmWalletGetTransferCount(BREthereumEWM ewm,
                          BREthereumWallet wallet) {
    int count = -1;

    pthread_mutex_lock(&ewm->lock);
    if (NULL != wallet) count = (int) walletGetTransferCount(wallet);
    pthread_mutex_unlock(&ewm->lock);

    return count;
}

extern void
ewmWalletSetDefaultGasLimit(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumGas gasLimit) {
    walletSetDefaultGasLimit(wallet, gasLimit);
    ewmClientSignalWalletEvent(ewm,
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
    ewmClientSignalWalletEvent(ewm,
                               ewmLookupWalletId(ewm, wallet),
                               WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
                               SUCCESS,
                               NULL);
}


///
/// MARK: Handlers
///
/**
 * Handle a default `gasPrice` for `wallet`
 *
 * @param ewm
 * @param wallet
 * @param gasPrice
 */
extern void
ewmHandleGasPrice (BREthereumEWM ewm,
                   BREthereumWallet wallet,
                   BREthereumGasPrice gasPrice) {
    pthread_mutex_lock(&ewm->lock);
    
    walletSetDefaultGasPrice(wallet, gasPrice);
    
    ewmClientSignalWalletEvent(ewm,
                                 ewmLookupWalletId(ewm, wallet),
                                 WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
                                 SUCCESS, NULL);
    
    pthread_mutex_unlock(&ewm->lock);
}


/**
 * Handle a `gasEstimate` for `transaction` in `wallet`
 *
 * @param ewm
 * @param wallet
 * @param transaction
 * @param gasEstimate
 */
extern void
ewmHandleGasEstimate (BREthereumEWM ewm,
                      BREthereumWallet wallet,
                      BREthereumTransfer transfer,
                      BREthereumGas gasEstimate) {
    pthread_mutex_lock(&ewm->lock);
    
    transferSetGasEstimate(transfer, gasEstimate);
    
    ewmClientSignalTransferEvent(ewm,
                                      ewmLookupWalletId(ewm, wallet),
                                      ewmLookupTransferId (ewm, transfer),
                                      TRANSFER_EVENT_GAS_ESTIMATE_UPDATED,
                                      SUCCESS, NULL);
    
    pthread_mutex_unlock(&ewm->lock);
    
}

// ==============================================================================================
//
// LES(BCS)/JSON_RPC Handlers
//


/**
 * Handle the BCS BlockChain callback.  This should result in a 'client block event' callback.
 * However, that callback accepts a `bid` and we don't have one (in the same sense as a tid or
 * wid); perhaps the blockNumber is the `bid`?
 *
 * Additionally, this handler has no indication of the type of BCS data.  E.g is this block chained
 * or orphaned.
 *
 * @param ewm
 * @param headBlockHash
 * @param headBlockNumber
 * @param headBlockTimestamp
 */
extern void
ewmHandleBlockChain (BREthereumEWM ewm,
                     BREthereumHash headBlockHash,
                     uint64_t headBlockNumber,
                     uint64_t headBlockTimestamp) {
    // Don't rebort during sync.
    if (ETHEREUM_BOOLEAN_IS_FALSE(bcsSyncInProgress(ewm->bcs)))
        eth_log ("EWM", "BlockChain: %llu", headBlockNumber);

    // TODO: Need a 'block id' - or axe the need of 'block id'?
    //
    // ewmClientSignalBlockEvent(<#BREthereumEWM ewm#>, <#BREthereumBlockId bid#>, <#BREthereumBlockEvent event#>, <#BREthereumStatus status#>, <#const char *errorDescription#>)
}


/**
 * Handle the BCS AccountState callback.
 *
 * @param ewm
 * @param accountState
 */
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

    ewmClientSignalWalletEvent(ewm, wid, WALLET_EVENT_BALANCE_UPDATED,
                                 SUCCESS,
                                 NULL);

    pthread_mutex_unlock(&ewm->lock);
}

extern void
ewmHandleTransaction (BREthereumEWM ewm,
                      BREthereumBCSCallbackTransactionType type,
                      BREthereumTransaction transaction) {
    

    BREthereumHash hash = transactionGetHash(transaction);

    BREthereumHashString hashString;
    hashFillString(hash, hashString);
    eth_log ("EWM", "Transaction: \"%s\", Change: %s",
             hashString, BCS_CALLBACK_TRANSACTION_TYPE_NAME(type));

    // Find the wallet
    BREthereumWalletId wid = ewmGetWallet(ewm);
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    assert (NULL != wallet);

    // Find a preexisting transfer
    BREthereumTransfer transfer = walletGetTransferByHash(wallet, hash);
    BREthereumTransferId tid = -1;

    if (NULL == transfer) {
        transfer = transferCreateWithTransaction(transaction);
        tid      = ewmInsertTransfer(ewm, transfer);

        walletHandleTransfer(wallet, transfer);

        ewmClientSignalTransferEvent(ewm, wid, tid,
                                     TRANSFER_EVENT_CREATED,
                                     SUCCESS, NULL);
    }
    else
        tid = ewmLookupTransferId(ewm, transfer);

    // TODO: Not quite
    ewmClientSignalTransferEvent(ewm, wid, tid,
                                 TRANSFER_EVENT_INCLUDED,
                                 SUCCESS, NULL);
}

extern void
ewmHandleLog (BREthereumEWM ewm,
              BREthereumBCSCallbackLogType type,
              BREthereumLog log) {
    BREthereumHash logHash = logGetHash(log);

    BREthereumHash transactionHash;
    size_t logIndex;
    logExtractIdentifier(log, &transactionHash, &logIndex);

    BREthereumHashString hashString;
    hashFillString(transactionHash, hashString);
    eth_log ("EWM", "Log: \"%s\" @ %zu, Change: %s",
             hashString, logIndex, BCS_CALLBACK_TRANSACTION_TYPE_NAME(type));

    BREthereumToken token = tokenLookupByAddress(logGetAddress(log));
    if (NULL == token) return;

    BREthereumWalletId wid = ewmGetWalletHoldingToken(ewm, token);
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    assert (NULL != wallet);

    BREthereumTransfer transfer = walletGetTransferByHash(wallet, logHash);
    BREthereumTransferId tid = -1;

    if (NULL == transfer) {
        transfer = transferCreateWithLog (log, token);
        tid      = ewmInsertTransfer(ewm, transfer);

        walletHandleTransfer(wallet, transfer);

        ewmClientSignalTransferEvent(ewm, wid, tid,
                                     TRANSFER_EVENT_CREATED,
                                     SUCCESS, NULL);
    }
    else
        tid = ewmLookupTransferId(ewm, transfer);

    // TODO: Not quite
    ewmClientSignalTransferEvent(ewm, wid, tid,
                                 TRANSFER_EVENT_INCLUDED,
                                 SUCCESS, NULL);
}

extern void
ewmHandleSaveBlocks (BREthereumEWM ewm,
                     BRArrayOf(BREthereumBlock) blocks) {
    eth_log("EWM", "Save Blocks: %zu", array_count(blocks));

    BRArrayOf(BREthereumPersistData) blocksToSave;
    array_new(blocksToSave, array_count(blocks));
    for (size_t index = 0; index < array_count(blocks); index++) {
        BRRlpItem item = blockRlpEncode(blocks[index], ewm->network, RLP_TYPE_ARCHIVE, ewm->coder);
        BREthereumPersistData persistData = {
            blockGetHash(blocks[index]),
            rlpGetData(ewm->coder, item)
        };
        rlpReleaseItem(ewm->coder, item);
        array_add (blocksToSave, persistData);
    }

    // TODO: ewmSignalSaveBlocks(ewm, blocks)
    ewm->client.funcSaveBlocks (ewm->client.context, ewm,
                                blocksToSave);

    array_free (blocks);
}

extern void
ewmHandleSavePeers (BREthereumEWM ewm,
                    BRArrayOf(BREthereumLESPeerConfig) peers) {
    size_t peersCount = array_count(peers);

    // Serialize BREthereumPeerConfig
    BRArrayOf(BREthereumPersistData) peersToSave;
    array_new(peersToSave, 0);
    for (size_t index = 0; index < array_count(peers); index++) {
        // Add to peersToSave
    }

    // TODO: ewmSignalSavePeers(ewm, peers);
    ewm->client.funcSavePeers (ewm->client.context, ewm,
                               peersToSave);

    eth_log("EWM", "Save Peers: %zu", peersCount);

    array_free (peers);
}

extern void
ewmHandleSync (BREthereumEWM ewm,
               BREthereumBCSCallbackSyncType type,
               uint64_t blockNumberStart,
               uint64_t blockNumberCurrent,
               uint64_t blockNumberStop) {
    BREthereumEWMEvent event = (blockNumberCurrent == blockNumberStart
                                ? EWM_EVENT_SYNC_STARTED
                                : (blockNumberCurrent == blockNumberStop
                                   ? EWM_EVENT_SYNC_STARTED
                                   : EWM_EVENT_SYNC_CONTINUES));
    double syncCompletePercent = 100.0 * (blockNumberCurrent - blockNumberStart) / (blockNumberStop - blockNumberStart);

    ewmClientSignalEWMEvent (ewm, event, SUCCESS, NULL);

    eth_log ("EWM", "Sync: %d, %.2f%%", type, syncCompletePercent);
}

//
// Periodic Dispatcher
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

#if defined(SUPPORT_JSON_RPC)

extern void
ethereumTransferFillRawData (BREthereumEWM ewm,
                             BREthereumWalletId wid,
                             BREthereumTransferId transferId,
                             uint8_t **bytesPtr, size_t *bytesCountPtr) {
    assert (NULL != bytesCountPtr && NULL != bytesPtr);

    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    BREthereumTransfer transfer = ewmLookupTransfer(ewm, transferId);
    assert (walletHasTransfer(wallet, transfer));

    BREthereumTransaction transaction = transferGetOriginatingTransaction (transfer);
    assert (NULL != transaction);
    assert (ETHEREUM_BOOLEAN_IS_TRUE (transactionIsSigned(transaction)));

    BRRlpItem item = transactionRlpEncode(transaction,
                                          ewm->network,
                                          (transactionIsSigned(transaction)
                                           ? RLP_TYPE_TRANSACTION_SIGNED
                                           : RLP_TYPE_TRANSACTION_UNSIGNED),
                                          ewm->coder);
    BRRlpData data = rlpGetData (ewm->coder, item);

    *bytesCountPtr = data.bytesCount;
    *bytesPtr = data.bytes;

    rlpReleaseItem(ewm->coder, item);
}

extern const char *
ethereumTransferGetRawDataHexEncoded(BREthereumEWM ewm,
                                        BREthereumWalletId wid,
                                        BREthereumTransferId transferId,
                                        const char *prefix) {
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    BREthereumTransfer transfer = ewmLookupTransfer(ewm, transferId);
    assert (walletHasTransfer(wallet, transfer));

    BREthereumTransaction transaction = transferGetOriginatingTransaction (transfer);
    assert (NULL != transaction);

    return transactionGetRlpHexEncoded(transaction,
                                ewm->network,
                                (transactionIsSigned(transaction)
                                 ? RLP_TYPE_TRANSACTION_SIGNED
                                 : RLP_TYPE_TRANSACTION_UNSIGNED),
                                prefix);
}

#endif // ETHEREUM_LIGHT_NODE_USE_JSON_RPC
