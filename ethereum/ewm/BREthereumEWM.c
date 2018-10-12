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
static BRSetOf(BREthereumBlock)
createEWMEnsureBlocks (BRSetOf(BREthereumHashDataPair) blocksPersistData,
                       BREthereumNetwork network,
                       BRRlpCoder coder) {
    size_t blocksCount = (NULL == blocksPersistData ? 0 : BRSetCount (blocksPersistData));

    BRSetOf(BREthereumBlock) blocks = BRSetNew (blockHashValue,
                                                blockHashEqual,
                                                blocksCount);

    if (0 == blocksCount) {
        BREthereumBlockHeader lastCheckpointHeader = blockCheckpointCreatePartialBlockHeader(blockCheckpointLookupLatest(network));
        BRSetAdd (blocks, blockCreate(lastCheckpointHeader));
    }
    else {
        FOR_SET (BREthereumHashDataPair, pair, blocksPersistData) {
            BRRlpItem item = rlpGetItem (coder, dataAsRlpData (hashDataPairGetData (pair)));
            BREthereumBlock block = blockRlpDecode (item, network, RLP_TYPE_ARCHIVE, coder);
            rlpReleaseItem(coder, item);
            BRSetAdd (blocks, block);
        }

        if (NULL != blocksPersistData)
            hashDataPairSetRelease(blocksPersistData);
    }

    return blocks;
}

static BRSetOf(BREthereumNodeConfig)
createEWMEnsureNodes (BRSetOf(BREthereumHashDataPair) nodesPersistData,
                      BRRlpCoder coder) {

    BRSetOf (BREthereumNodeConfig) nodes =
    BRSetNew (nodeConfigHashValue,
              nodeConfigHashEqual,
              (NULL == nodesPersistData ? 0 : BRSetCount(nodesPersistData)));

    if (NULL != nodesPersistData) {
        FOR_SET (BREthereumHashDataPair, pair, nodesPersistData) {
            BRRlpItem item = rlpGetItem (coder, dataAsRlpData (hashDataPairGetData (pair)));
            BREthereumNodeConfig node = nodeConfigDecode(item, coder);
            rlpReleaseItem(coder, item);
            BRSetAdd (nodes, node);
        }

        hashDataPairSetRelease(nodesPersistData);
    }

    return nodes;
}

static BRSetOf(BREthereumTransaction)
createEWMEnsureTransactions (BRSetOf(BREthereumHashDataPair) transactionsPersistData,
                             BREthereumNetwork network,
                             BRRlpCoder coder) {
    BRSetOf(BREthereumTransaction) transactions =
    BRSetNew (transactionHashValue,
              transactionHashEqual,
              (NULL == transactionsPersistData ? 0 : BRSetCount (transactionsPersistData)));

    if (NULL != transactionsPersistData) {
        FOR_SET (BREthereumHashDataPair, pair, transactionsPersistData) {
            fprintf (stdout, "ETH: TST: EnsureTrans @ %p\n", hashDataPairGetData(pair).bytes);

            BRRlpItem item = rlpGetItem (coder, dataAsRlpData (hashDataPairGetData (pair)));
            BREthereumTransaction transaction = transactionRlpDecode(item, network, RLP_TYPE_ARCHIVE, coder);
            rlpReleaseItem (coder, item);
            BRSetAdd (transactions, transaction);
        }

        hashDataPairSetRelease(transactionsPersistData);
    }

    return transactions;
}

static BRSetOf(BREthereumLog)
createEWMEnsureLogs(BRSetOf(BREthereumHashDataPair) logsPersistData,
                    BREthereumNetwork network,
                    BRRlpCoder coder) {
    BRSetOf(BREthereumLog) logs =
    BRSetNew (logHashValue,
              logHashEqual,
              (NULL == logsPersistData ? 0 : BRSetCount(logsPersistData)));

    if (NULL != logsPersistData) {

        FOR_SET (BREthereumHashDataPair, pair, logsPersistData) {
            fprintf (stdout, "ETH: TST: EnsureLogs @ %p\n", hashDataPairGetData(pair).bytes);
            BRRlpItem item = rlpGetItem (coder, dataAsRlpData (hashDataPairGetData (pair)));
            BREthereumLog log = logRlpDecode(item, RLP_TYPE_ARCHIVE, coder);
            rlpReleaseItem(coder, item);

            BRSetAdd (logs, log);
        }

        hashDataPairSetRelease (logsPersistData);
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
           BRSetOf(BREthereumHashDataPair) nodesPersistData,
           BRSetOf(BREthereumHashDataPair) blocksPersistData,
           BRSetOf(BREthereumHashDataPair) transactionsPersistData,
           BRSetOf(BREthereumHashDataPair) logsPersistData) {
    BREthereumEWM ewm = (BREthereumEWM) calloc (1, sizeof (struct BREthereumEWMRecord));

    ewm->state = LIGHT_NODE_CREATED;
    ewm->type = type;
    ewm->network = network;
    ewm->account = account;
    ewm->bcs = NULL;

    // Get the client assigned early; callbacks as EWM/BCS state is re-establish, regarding
    // blocks, peers, transactions and logs, will be invoked.
    ewm->client = client;

    // Our one and only coder
    ewm->coder = rlpCoderCreate();

    // Create the `listener` and `main` event handlers.  Do this early so that queues exist
    // for any events/callbacks generated during initialization.  The queues won't be handled
    // until ewmConnect().
    ewm->handlerForClient = eventHandlerCreate ("Core Ethereum EWM Client",
                                                handlerForClientEventTypes,
                                                handlerForClientEventTypesCount);

    // The `main` event handler has a periodic wake-up.  Used, perhaps, if the syncMode indicates
    // that we should/might query the BRD backend services.
    ewm->handlerForMain = eventHandlerCreate ("Core Ethereum EWM",
                                              handlerForMainEventTypes,
                                              handlerForMainEventTypesCount);

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

    // Extract the provided persist data.  We'll use these when configuring BCS (if EWM_USE_LES)
    // or EWM (if EWM_USE_BRD).
    BRSetOf(BREthereumNodeConfig)  nodes        = createEWMEnsureNodes(nodesPersistData, ewm->coder);
    BRSetOf(BREthereumBlock)       blocks       = createEWMEnsureBlocks (blocksPersistData, network, ewm->coder);
    BRSetOf(BREthereumTransaction) transactions = createEWMEnsureTransactions(transactionsPersistData, network, ewm->coder);
    BRSetOf(BREthereumLog)         logs         = createEWMEnsureLogs(logsPersistData, network, ewm->coder);

    // Support the requested type
    switch (ewm->type) {
        case EWM_USE_LES: {
            // Create BCS - note: when BCS processes blocks, peers, transactions, and logs there
            // will be callbacks made to the EWM client.  Because we've defined `handlerForMain`
            // any callbacks will be queued and then handled when EWM actually starts

            // Create the BCS listener - allows EWM to handle block, peer, transaction and log events.
            BREthereumBCSListener listener = {
                (BREthereumBCSCallbackContext) ewm,
                (BREthereumBCSCallbackBlockchain) ewmSignalBlockChain,
                (BREthereumBCSCallbackAccountState) ewmSignalAccountState,
                (BREthereumBCSCallbackTransaction) ewmSignalTransaction,
                (BREthereumBCSCallbackLog) ewmSignalLog,
                (BREthereumBCSCallbackSaveBlocks) ewmSignalSaveBlocks,
                (BREthereumBCSCallbackSavePeers) ewmSignalSaveNodes,
                (BREthereumBCSCallbackSync) ewmSignalSync,
                (BREthereumBCSCallbackGetBlocks) ewmSignalGetBlocks
            };

            ewm->bcs = bcsCreate (network,
                                  accountGetPrimaryAddress (account),
                                  listener,
                                  syncMode,
                                  nodes,
                                  blocks,
                                  transactions,
                                  logs);
            break;
        }

        case EWM_USE_BRD: {
            // Announce all the provided transactions...
            FOR_SET (BREthereumTransaction, transaction, transactions)
                ewmSignalTransaction (ewm, BCS_CALLBACK_TRANSACTION_ADDED, transaction);

            // ... as well as the provided logs...
            FOR_SET (BREthereumLog, log, logs)
                ewmSignalLog (ewm, BCS_CALLBACK_LOG_ADDED, log);

            // ... and then the latest block.
            BREthereumBlock lastBlock = NULL;
            FOR_SET (BREthereumBlock, block, blocks)
                if (NULL == lastBlock || blockGetNumber(lastBlock) < blockGetNumber(block))
                    lastBlock = block;
            ewmSignalBlockChain (ewm,
                                 blockGetHash( lastBlock),
                                 blockGetNumber (lastBlock),
                                 blockGetTimestamp (lastBlock));

            // ... and then ignore nodes

            // TODO: What items to free?
            BRSetFree (nodes);
            BRSetFree (blocks);
            BRSetFree (transactions);
            BRSetFree (logs);

            // Add ewmPeriodicDispatcher to handlerForMain.
            eventHandlerSetTimeoutDispatcher(ewm->handlerForMain,
                                             1000 * EWM_SLEEP_SECONDS,
                                             (BREventDispatcher)ewmPeriodicDispatcher,
                                             (void*) ewm);

            break;
        }
    }

    return ewm;
}

extern void
ewmDestroy (BREthereumEWM ewm) {
    assert (ewm->type == (NULL == ewm->bcs ? EWM_USE_BRD : EWM_USE_LES));

    ewmDisconnect(ewm);

    if (NULL != ewm->bcs)
        bcsDestroy(ewm->bcs);

    walletsRelease (ewm->wallets);
    ewm->wallets = NULL;

    array_free (ewm->transfers);   // released by wallets: transfersRelease(ewm->transfers);
    blocksRelease(ewm->blocks);

    eventHandlerDestroy(ewm->handlerForClient);
    eventHandlerDestroy(ewm->handlerForMain);
    rlpCoderRelease(ewm->coder);
    
    free (ewm);
}

///
/// MARK: - Connect / Disconnect
///
static BREthereumBoolean
ewmIsConnected (BREthereumEWM ewm) {
    switch (ewm->type) {
        case EWM_USE_LES: return bcsIsStarted (ewm->bcs);
        case EWM_USE_BRD: return AS_ETHEREUM_BOOLEAN (LIGHT_NODE_CONNECTED == ewm->state);
    }
}

/**
 * ewmConnect() - Start EWM.  Returns TRUE if started, FALSE if is currently stated (TRUE
 * is action taken).
 */
extern BREthereumBoolean
ewmConnect(BREthereumEWM ewm) {
    assert (ewm->type == (NULL == ewm->bcs ? EWM_USE_BRD : EWM_USE_LES));

    // Nothing to do if already connected
    if (ETHEREUM_BOOLEAN_IS_TRUE (ewmIsConnected(ewm)))
        return ETHEREUM_BOOLEAN_FALSE;

    // Set ewm {client,state} prior to bcs/event start.  Avoid race conditions, particularly
    // with `ewmPeriodicDispatcher`.
    ewm->state = LIGHT_NODE_CONNECTED;

    if (NULL != ewm->bcs)
        bcsStart(ewm->bcs);

    eventHandlerStart(ewm->handlerForClient);
    eventHandlerStart(ewm->handlerForMain);

    return ETHEREUM_BOOLEAN_TRUE;
}

/**
 * Stop EWM.  Returns TRUE if stopped, FALSE if currently stopped.
 *
 * @param ewm EWM
 * @return TRUE if action needed.
 */
extern BREthereumBoolean
ewmDisconnect (BREthereumEWM ewm) {
    assert (ewm->type == (NULL == ewm->bcs ? EWM_USE_BRD : EWM_USE_LES));

    if (ETHEREUM_BOOLEAN_IS_FALSE (ewmIsConnected(ewm)))
        return ETHEREUM_BOOLEAN_FALSE;

    // Set ewm->state thereby stopping handlers (in a race with bcs/event calls).
    ewm->state = LIGHT_NODE_DISCONNECTED;

    if (NULL != ewm->bcs)
        bcsStop(ewm->bcs);

    eventHandlerStop(ewm->handlerForMain);
    eventHandlerStop(ewm->handlerForClient);

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
extern BREthereumWalletId *
ewmGetWallets (BREthereumEWM ewm) {
    pthread_mutex_lock(&ewm->lock);

    unsigned long count = array_count(ewm->wallets);
    BREthereumWalletId *wallets = calloc (count + 1, sizeof (BREthereumWalletId));

    for (BREthereumWalletId index = 0; index < count; index++) {
        wallets [index] = index;
    }
    wallets[count] = -1;

    pthread_mutex_unlock(&ewm->lock);
    return wallets;
}

extern unsigned int
ewmGetWalletsCount (BREthereumEWM ewm) {
    return (unsigned int) array_count(ewm->wallets);
}

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
// LES(BCS)/BRD Handlers
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
    assert (ewm->type == (NULL == ewm->bcs ? EWM_USE_BRD : EWM_USE_LES));

    // Don't report during BCS sync.
    if (NULL == ewm->bcs || ETHEREUM_BOOLEAN_IS_FALSE(bcsSyncInProgress (ewm->bcs)))
        eth_log ("EWM", "BlockChain: %llu", headBlockNumber);

    // At least this - allows for: ewmGetBlockHeight
    ewm->blockHeight = headBlockNumber;

    // TODO: Need a 'block id' - or axe the need of 'block id'?
    ewmClientSignalBlockEvent (ewm,
                               (BREthereumBlockId) 0,
                               BLOCK_EVENT_CHAINED,
                               SUCCESS,
                               NULL);
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
        walletUpdateBalance (wallet);

        ewmClientSignalTransferEvent(ewm, wid, tid,
                                     TRANSFER_EVENT_CREATED,
                                     SUCCESS, NULL);

        ewmClientSignalWalletEvent(ewm, wid, WALLET_EVENT_BALANCE_UPDATED,
                                   SUCCESS,
                                   NULL);

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
              OwnershipGiven BREthereumLog log) {
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

    // TODO: Confirm LogTopic[0] is 'transfer'
    if (3 != logGetTopicsCount(log)) return;

    BREthereumWalletId wid = ewmGetWalletHoldingToken(ewm, token);
    BREthereumWallet wallet = ewmLookupWallet(ewm, wid);
    assert (NULL != wallet);

    BREthereumTransfer transfer = walletGetTransferByHash(wallet, logHash);
    BREthereumTransferId tid = -1;

    if (NULL == transfer) {
        transfer = transferCreateWithLog (log, token);
        tid      = ewmInsertTransfer(ewm, transfer);

        walletHandleTransfer(wallet, transfer);
        walletUpdateBalance (wallet);

        ewmClientSignalTransferEvent(ewm, wid, tid,
                                     TRANSFER_EVENT_CREATED,
                                     SUCCESS, NULL);

        ewmClientSignalWalletEvent(ewm, wid, WALLET_EVENT_BALANCE_UPDATED,
                                   SUCCESS,
                                   NULL);
    }
    else {
        tid = ewmLookupTransferId(ewm, transfer);
        if (log != transferGetBasisLog(transfer))
            logRelease(log);
    }

    // TODO: Not quite
    ewmClientSignalTransferEvent(ewm, wid, tid,
                                 TRANSFER_EVENT_INCLUDED,
                                 SUCCESS, NULL);
}

extern void
ewmHandleSaveBlocks (BREthereumEWM ewm,
                     BRArrayOf(BREthereumBlock) blocks) {
    eth_log("EWM", "Save Blocks: %zu", array_count(blocks));

    BRSetOf(BREthereumHashDataPair) blocksToSave = hashDataPairSetCreateEmpty (array_count (blocks));

    for (size_t index = 0; index < array_count(blocks); index++) {
        BRRlpItem item = blockRlpEncode(blocks[index], ewm->network, RLP_TYPE_ARCHIVE, ewm->coder);
        BRSetAdd (blocksToSave,
                  hashDataPairCreate (blockGetHash(blocks[index]),
                                      dataCreateFromRlpData (rlpGetData (ewm->coder, item), 1)));
        rlpReleaseItem(ewm->coder, item);
    }

    // TODO: ewmSignalSaveBlocks(ewm, blocks)
    ewm->client.funcSaveBlocks (ewm->client.context,
                                ewm,
                                blocksToSave);

    array_free (blocks);
}

extern void
ewmHandleSaveNodes (BREthereumEWM ewm,
                    BRArrayOf(BREthereumNodeConfig) nodes) {
    size_t nodesCount = array_count(nodes);

    BRSetOf(BREthereumHashDataPair) nodesToSave = hashDataPairSetCreateEmpty (array_count (nodes));

    for (size_t index = 0; index < array_count(nodes); index++) {
        BRRlpItem item = nodeConfigEncode(nodes[index], ewm->coder);
        BRSetAdd (nodesToSave,
                  hashDataPairCreate (nodeConfigGetHash(nodes[index]),
                                      dataCreateFromRlpData (rlpGetData (ewm->coder, item), 1)));
        rlpReleaseItem (ewm->coder, item);
    }

    // TODO: ewmSignalSavenodes(ewm, nodes);
    ewm->client.funcSaveNodes (ewm->client.context,
                               ewm,
                               nodesToSave);

    eth_log("EWM", "Save nodes: %zu", nodesCount);

    array_free (nodes);
}

extern void
ewmHandleSync (BREthereumEWM ewm,
               BREthereumBCSCallbackSyncType type,
               uint64_t blockNumberStart,
               uint64_t blockNumberCurrent,
               uint64_t blockNumberStop) {
    assert (EWM_USE_LES == ewm->type);

    BREthereumEWMEvent event = (blockNumberCurrent == blockNumberStart
                                ? EWM_EVENT_SYNC_STARTED
                                : (blockNumberCurrent == blockNumberStop
                                   ? EWM_EVENT_SYNC_STOPPED
                                   : EWM_EVENT_SYNC_CONTINUES));
    double syncCompletePercent = 100.0 * (blockNumberCurrent - blockNumberStart) / (blockNumberStop - blockNumberStart);
    
    ewmClientSignalEWMEvent (ewm, event, SUCCESS, NULL);

    eth_log ("EWM", "Sync: %d, %.2f%%", type, syncCompletePercent);
}

extern void
ewmHandleGetBlocks (BREthereumEWM ewm,
                    BREthereumAddress address,
                    BREthereumSyncInterestSet interests,
                    uint64_t blockStart,
                    uint64_t blockStop) {

    char *strAddress = addressGetEncodedString(address, 0);

    ewm->client.funcGetBlocks (ewm->client.context,
                               ewm,
                               strAddress,
                               interests,
                               blockStart,
                               blockStop,
                               ++ewm->requestId);

    free (strAddress);
}

//
// Periodic Dispatcher
//

static void
ewmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event) {
    BREthereumEWM ewm = (BREthereumEWM) event->context;
    
    if (ewm->state != LIGHT_NODE_CONNECTED) return;
    if (ewm->type  == EWM_USE_LES) return;
    
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

#if 1 // defined(SUPPORT_JSON_RPC)

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
