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
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include "BRArray.h"
#include "BRBIP39Mnemonic.h"

#include "../event/BREvent.h"
#include "BREthereumEWMPrivate.h"

#define EWM_SLEEP_SECONDS (5)

// When using a BRD sync offset the start block by 3 days of Ethereum blocks
#define EWM_BRD_SYNC_START_BLOCK_OFFSET        (3 * 24 * 60 * 4)   /* 4 per minute, every 15 seconds */

#define EWM_INITIAL_SET_SIZE_DEFAULT         (25)

/* Forward Declaration */
static void
ewmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);

/* Forward Implementation */

///
/// MARK: Transaction File Service
///
static const char *fileServiceTypeTransactions = "transactions";

enum {
    EWM_TRANSACTION_VERSION_1
};

static UInt256
fileServiceTypeTransactionV1Identifier (BRFileServiceContext context,
                                        BRFileService fs,
                                        const void *entity) {
    BREthereumTransaction transaction = (BREthereumTransaction) entity;
    BREthereumHash hash = transactionGetHash(transaction);

    UInt256 result;
    memcpy (result.u8, hash.bytes, ETHEREUM_HASH_BYTES);
    return result;
}

static uint8_t *
fileServiceTypeTransactionV1Writer (BRFileServiceContext context,
                                    BRFileService fs,
                                    const void* entity,
                                    uint32_t *bytesCount) {
    BREthereumEWM ewm = context;
    BREthereumTransaction transaction = (BREthereumTransaction) entity;

    BRRlpItem item = transactionRlpEncode(transaction, ewm->network, RLP_TYPE_ARCHIVE, ewm->coder);
    BRRlpData data = rlpGetData (ewm->coder, item);
    rlpReleaseItem (ewm->coder, item);

    *bytesCount = (uint32_t) data.bytesCount;
    return data.bytes;
}

static void *
fileServiceTypeTransactionV1Reader (BRFileServiceContext context,
                                    BRFileService fs,
                                    uint8_t *bytes,
                                    uint32_t bytesCount) {
    BREthereumEWM ewm = context;

    BRRlpData data = { bytesCount, bytes };
    BRRlpItem item = rlpGetItem (ewm->coder, data);

    BREthereumTransaction transaction = transactionRlpDecode(item, ewm->network, RLP_TYPE_ARCHIVE, ewm->coder);
    rlpReleaseItem (ewm->coder, item);

    return transaction;
}

static BRSetOf(BREthereumTransaction)
initialTransactionsLoad (BREthereumEWM ewm) {
    BRSetOf(BREthereumTransaction) transactions = BRSetNew(transactionHashValue, transactionHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);
    if (1 != fileServiceLoad (ewm->fs, transactions, fileServiceTypeTransactions, 1)) {
        BRSetFree(transactions);
        return NULL;
    }
    return transactions;
}

///
/// MARK: Log File Service
///
static const char *fileServiceTypeLogs = "logs";

enum {
    EWM_LOG_VERSION_1
};

static UInt256
fileServiceTypeLogV1Identifier (BRFileServiceContext context,
                                        BRFileService fs,
                                        const void *entity) {
    const BREthereumLog log = (BREthereumLog) entity;
    BREthereumHash hash = logGetHash( log);

    UInt256 result;
    memcpy (result.u8, hash.bytes, ETHEREUM_HASH_BYTES);
    return result;
}

static uint8_t *
fileServiceTypeLogV1Writer (BRFileServiceContext context,
                                    BRFileService fs,
                                    const void* entity,
                                    uint32_t *bytesCount) {
    BREthereumEWM ewm = context;
    BREthereumLog log = (BREthereumLog) entity;

    BRRlpItem item = logRlpEncode (log, RLP_TYPE_ARCHIVE, ewm->coder);
    BRRlpData data = rlpGetData (ewm->coder, item);
    rlpReleaseItem (ewm->coder, item);

    *bytesCount = (uint32_t) data.bytesCount;
    return data.bytes;
}

static void *
fileServiceTypeLogV1Reader (BRFileServiceContext context,
                                    BRFileService fs,
                                    uint8_t *bytes,
                                    uint32_t bytesCount) {
    BREthereumEWM ewm = context;

    BRRlpData data = { bytesCount, bytes };
    BRRlpItem item = rlpGetItem (ewm->coder, data);

    BREthereumLog log = logRlpDecode(item, RLP_TYPE_ARCHIVE, ewm->coder);
    rlpReleaseItem (ewm->coder, item);

    return log;
}

static BRSetOf(BREthereumLog)
initialLogsLoad (BREthereumEWM ewm) {
    BRSetOf(BREthereumLog) logs = BRSetNew(logHashValue, logHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);
    if (1 != fileServiceLoad (ewm->fs, logs, fileServiceTypeLogs, 1)) {
        BRSetFree(logs);
        return NULL;
    }
    return logs;
}


///
/// MARK: Block File Service
///
static const char *fileServiceTypeBlocks = "blocks";
enum {
    EWM_BLOCK_VERSION_1
};

static UInt256
fileServiceTypeBlockV1Identifier (BRFileServiceContext context,
                                  BRFileService fs,
                                  const void *entity) {
    const BREthereumBlock block = (BREthereumBlock) entity;
    BREthereumHash hash = blockGetHash(block);

    UInt256 result;
    memcpy (result.u8, hash.bytes, ETHEREUM_HASH_BYTES);
    return result;
}

static uint8_t *
fileServiceTypeBlockV1Writer (BRFileServiceContext context,
                              BRFileService fs,
                              const void* entity,
                              uint32_t *bytesCount) {
    BREthereumEWM ewm = context;
    BREthereumBlock block = (BREthereumBlock) entity;

    BRRlpItem item = blockRlpEncode(block, ewm->network, RLP_TYPE_ARCHIVE, ewm->coder);
    BRRlpData data = rlpGetData (ewm->coder, item);
    rlpReleaseItem (ewm->coder, item);

    *bytesCount = (uint32_t) data.bytesCount;
    return data.bytes;
}

static void *
fileServiceTypeBlockV1Reader (BRFileServiceContext context,
                              BRFileService fs,
                              uint8_t *bytes,
                              uint32_t bytesCount) {
    BREthereumEWM ewm = context;

    BRRlpData data = { bytesCount, bytes };
    BRRlpItem item = rlpGetItem (ewm->coder, data);

    BREthereumBlock block = blockRlpDecode (item, ewm->network, RLP_TYPE_ARCHIVE, ewm->coder);
    rlpReleaseItem (ewm->coder, item);

    return block;
}

static BRSetOf(BREthereumBlock)
initialBlocksLoad (BREthereumEWM ewm) {
    BRSetOf(BREthereumBlock) blocks = BRSetNew(blockHashValue, blockHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);
    if (1 != fileServiceLoad (ewm->fs, blocks, fileServiceTypeBlocks, 1)) {
        BRSetFree(blocks);
        return NULL;
    }
    return blocks;
}

///
/// MARK: Node File Service
///
static const char *fileServiceTypeNodes = "nodes";
enum {
    EWM_NODE_VERSION_1
};

static UInt256
fileServiceTypeNodeV1Identifier (BRFileServiceContext context,
                                 BRFileService fs,
                                 const void *entity) {
    const BREthereumNodeConfig node = (BREthereumNodeConfig) entity;

    BREthereumHash hash = nodeConfigGetHash(node);

    UInt256 result;
    memcpy (result.u8, hash.bytes, ETHEREUM_HASH_BYTES);
    return result;
}

static uint8_t *
fileServiceTypeNodeV1Writer (BRFileServiceContext context,
                             BRFileService fs,
                             const void* entity,
                             uint32_t *bytesCount) {
    BREthereumEWM ewm = context;
    const BREthereumNodeConfig node = (BREthereumNodeConfig) entity;

    BRRlpItem item = nodeConfigEncode (node, ewm->coder);
    BRRlpData data = rlpGetData (ewm->coder, item);
    rlpReleaseItem (ewm->coder, item);

    *bytesCount = (uint32_t) data.bytesCount;
    return data.bytes;
}

static void *
fileServiceTypeNodeV1Reader (BRFileServiceContext context,
                             BRFileService fs,
                             uint8_t *bytes,
                             uint32_t bytesCount) {
    BREthereumEWM ewm = context;

    BRRlpData data = { bytesCount, bytes };
    BRRlpItem item = rlpGetItem (ewm->coder, data);

    BREthereumNodeConfig node = nodeConfigDecode (item, ewm->coder);
    rlpReleaseItem (ewm->coder, item);

    return node;
}

static BRSetOf(BREthereumNodeConfig)
initialNodesLoad (BREthereumEWM ewm) {
    BRSetOf(BREthereumNodeConfig) nodes = BRSetNew(nodeConfigHashValue, nodeConfigHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);
    if (1 != fileServiceLoad (ewm->fs, nodes, fileServiceTypeNodes, 1)) {
        BRSetFree(nodes);
        return NULL;
    }
    return nodes;
}


/**
 *
 *
 * @param context The EthereumWalletManager (EWM)
 * @param fs the FileSerice
 * @param error A BRFileServiceError
 */
static void
ewmFileServiceErrorHandler (BRFileServiceContext context,
                            BRFileService fs,
                            BRFileServiceError error) {
    //BREthereumEWM ewm = (BREthereumEWM) context;

    switch (error.type) {
        case FILE_SERVICE_IMPL:
            // This actually a FATAL - an unresolvable coding error.
            eth_log ("EWM", "FileService Error: IMPL: %s", error.u.impl.reason);
            break;
        case FILE_SERVICE_UNIX:
            eth_log ("EWM", "FileService Error: UNIX: %s", strerror(error.u.unix.error));
            break;
        case FILE_SERVICE_ENTITY:
            // This is likely a coding error too.
            eth_log ("EWM", "FileService Error: ENTITY (%s): %s",
                     error.u.entity.type,
                     error.u.entity.reason);
            break;
    }
    eth_log ("EWM", "FileService Error: FORCED SYNC%s", "");

    // TODO: Actually force a resync.
}
///
/// MARK: Ethereum Wallet Manager
///
static BREthereumEWM
ewmCreateErrorHandler (BREthereumEWM ewm, int fileService, const char* reason) {
    if (NULL != ewm) free (ewm);
    if (fileService)
        eth_log ("EWM", "on ewmCreate: FileService Error: %s", reason);
    else
        eth_log ("EWM", "on ewmCreate: Error: %s", reason);

    return NULL;
}

extern BREthereumEWM
ewmCreate (BREthereumNetwork network,
           BREthereumAccount account,
           BREthereumTimestamp accountTimestamp,
           BREthereumMode mode,
           BREthereumClient client,
           const char *storagePath) {
    BREthereumEWM ewm = (BREthereumEWM) calloc (1, sizeof (struct BREthereumEWMRecord));

    ewm->state = LIGHT_NODE_CREATED;
    ewm->mode = mode;
    ewm->network = network;
    ewm->account = account;
    ewm->bcs = NULL;
    ewm->blockHeight = 0;

    {
        char address [ADDRESS_ENCODED_CHARS];
        addressFillEncodedString (accountGetPrimaryAddress(account), 1, address);
        eth_log ("EWM", "Account: %s", address);
    }

    // Initialize the `brdSync` struct
    ewm->brdSync.ridTransaction = -1;
    ewm->brdSync.ridLog = -1;
    ewm->brdSync.begBlockNumber = 0;
    ewm->brdSync.endBlockNumber = 0;
    ewm->brdSync.completedTransaction = 0;
    ewm->brdSync.completedLog = 0;

    // Get the client assigned early; callbacks as EWM/BCS state is re-establish, regarding
    // blocks, peers, transactions and logs, will be invoked.
    ewm->client = client;

    // Our one and only coder
    ewm->coder = rlpCoderCreate();

    // The file service.  Initialize {nodes, blocks, transactions and logs} from the FileService

    ewm->fs = fileServiceCreate (storagePath, "eth", networkGetName(network),
                                 ewm,
                                 ewmFileServiceErrorHandler);
    if (NULL == ewm->fs) return ewmCreateErrorHandler(ewm, 1, "create");

    /// Transaction
    if (1 != fileServiceDefineType (ewm->fs, fileServiceTypeTransactions, EWM_TRANSACTION_VERSION_1,
                                    (BRFileServiceContext) ewm,
                                    fileServiceTypeTransactionV1Identifier,
                                    fileServiceTypeTransactionV1Reader,
                                    fileServiceTypeTransactionV1Writer) ||
        1 != fileServiceDefineCurrentVersion (ewm->fs, fileServiceTypeTransactions,
                                              EWM_TRANSACTION_VERSION_1))
        return ewmCreateErrorHandler(ewm, 1, fileServiceTypeTransactions);

    /// Log
    if (1 != fileServiceDefineType (ewm->fs, fileServiceTypeLogs, EWM_LOG_VERSION_1,
                                    (BRFileServiceContext) ewm,
                                    fileServiceTypeLogV1Identifier,
                                    fileServiceTypeLogV1Reader,
                                    fileServiceTypeLogV1Writer) ||
        1 != fileServiceDefineCurrentVersion (ewm->fs, fileServiceTypeLogs,
                                              EWM_LOG_VERSION_1))
        return ewmCreateErrorHandler(ewm, 1, fileServiceTypeLogs);

    /// Peer
    if (1 != fileServiceDefineType (ewm->fs, fileServiceTypeNodes, EWM_NODE_VERSION_1,
                                    (BRFileServiceContext) ewm,
                                    fileServiceTypeNodeV1Identifier,
                                    fileServiceTypeNodeV1Reader,
                                    fileServiceTypeNodeV1Writer) ||
        1 != fileServiceDefineCurrentVersion (ewm->fs, fileServiceTypeNodes,
                                              EWM_NODE_VERSION_1))
        return ewmCreateErrorHandler(ewm, 1, fileServiceTypeNodes);


   /// Block
    if (1 != fileServiceDefineType (ewm->fs, fileServiceTypeBlocks, EWM_BLOCK_VERSION_1,
                                    (BRFileServiceContext) ewm,
                                    fileServiceTypeBlockV1Identifier,
                                    fileServiceTypeBlockV1Reader,
                                    fileServiceTypeBlockV1Writer) ||
        1 != fileServiceDefineCurrentVersion (ewm->fs, fileServiceTypeBlocks,
                                              EWM_BLOCK_VERSION_1))
        return ewmCreateErrorHandler(ewm, 1, fileServiceTypeBlocks);

    // Load all the persistent entities
    BRSetOf(BREthereumTransaction) transactions = initialTransactionsLoad(ewm);
    BRSetOf(BREthereumLog) logs = initialLogsLoad(ewm);
    BRSetOf(BREthereumNodeConfig) nodes = initialNodesLoad(ewm);
    BRSetOf(BREthereumBlock) blocks = initialBlocksLoad(ewm);

    // If any are NULL, then we have an error and a full sync is required.  The sync will be
    // started automatically, as part of the normal processing, of 'blocks' (we'll use a checkpoint,
    // before the `accountTimestamp, which will be well in the past and we'll sync up to the
    // head of the blockchain).
    if (NULL == transactions || NULL == logs || NULL == nodes || NULL == blocks) {
        if (NULL == transactions) transactions = BRSetNew(transactionHashValue, transactionHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);
        else BRSetClear(transactions);

        if (NULL == logs) logs = BRSetNew(logHashValue, logHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);
        else BRSetClear(logs);

        if (NULL == blocks) blocks = BRSetNew(blockHashValue, blockHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);
        else BRSetClear(blocks);

        if (NULL == nodes) nodes = BRSetNew(nodeConfigHashValue, nodeConfigHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);
        else BRSetClear(nodes);
    }

    // If we have no blocks; then add a checkpoint
    if (0 == BRSetCount(blocks)) {
        const BREthereumBlockCheckpoint *checkpoint = blockCheckpointLookupByTimestamp (network, accountTimestamp);
        BREthereumBlock block = blockCreate (blockCheckpointCreatePartialBlockHeader (checkpoint));
        blockSetTotalDifficulty (block, checkpoint->u.td);
        BRSetAdd (blocks, block);
    }


    // The `main` event handler has a periodic wake-up.  Used, perhaps, if the mode indicates
    // that we should/might query the BRD backend services.
    ewm->handler = eventHandlerCreate ("Core Ethereum EWM",
                                       ewmEventTypes,
                                       ewmEventTypesCount);

    array_new(ewm->wallets, DEFAULT_WALLET_CAPACITY);

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
        (BREthereumBCSCallbackSavePeers) ewmSignalSaveNodes,
        (BREthereumBCSCallbackSync) ewmSignalSync,
        (BREthereumBCSCallbackGetBlocks) ewmSignalGetBlocks
    };

    // Create BCS - note: when BCS processes blocks, peers, transactions, and logs there
    // will be callbacks made to the EWM client.  Because we've defined `handlerForMain`
    // any callbacks will be queued and then handled when EWM actually starts
    //

    // Support the requested mode
    switch (ewm->mode) {
        case BRD_ONLY:
        case BRD_WITH_P2P_SEND: {
            // Note: We'll create BCS even for the mode where we don't use it (BRD_ONLY).
            ewm->bcs = bcsCreate (network,
                                  accountGetPrimaryAddress (account),
                                  listener,
                                  mode,
                                  nodes,
                                  NULL,
                                  NULL,
                                  NULL);

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

            // ... and then just ignore nodes

            // Free sets... BUT DO NOT free 'nodes' as those had 'OwnershipGiven' in bcsCreate()
            BRSetFree (blocks);
            BRSetFree (transactions);
            BRSetFree (logs);

            // Add ewmPeriodicDispatcher to handlerForMain.
            eventHandlerSetTimeoutDispatcher(ewm->handler,
                                             1000 * EWM_SLEEP_SECONDS,
                                             (BREventDispatcher)ewmPeriodicDispatcher,
                                             (void*) ewm);

            break;
        }

        case P2P_WITH_BRD_SYNC:  // 
        case P2P_ONLY: {
            ewm->bcs = bcsCreate (network,
                                  accountGetPrimaryAddress (account),
                                  listener,
                                  mode,
                                  nodes,
                                  blocks,
                                  transactions,
                                  logs);
            break;
        }
    }

    // mark as 'sync in progress' - we can't sent transactions until we have the nonce.

    return ewm;

}

extern BREthereumEWM
ewmCreateWithPaperKey (BREthereumNetwork network,
                       const char *paperKey,
                       BREthereumTimestamp accountTimestamp,
                       BREthereumMode mode,
                       BREthereumClient client,
                       const char *storagePath) {
    return ewmCreate (network,
                      createAccount (paperKey),
                      accountTimestamp,
                      mode,
                      client,
                      storagePath);
}

extern BREthereumEWM
ewmCreateWithPublicKey (BREthereumNetwork network,
                        BRKey publicKey,
                        BREthereumTimestamp accountTimestamp,
                        BREthereumMode mode,
                        BREthereumClient client,
                        const char *storagePath) {
    return ewmCreate (network,
                      createAccountWithPublicKey(publicKey),
                      accountTimestamp,
                      mode,
                      client,
                      storagePath);
}

extern void
ewmDestroy (BREthereumEWM ewm) {
    ewmDisconnect(ewm);

    bcsDestroy(ewm->bcs);

    walletsRelease (ewm->wallets);
    ewm->wallets = NULL;

    eventHandlerDestroy(ewm->handler);
    rlpCoderRelease(ewm->coder);
    
    free (ewm);
}

///
/// MARK: - Connect / Disconnect
///

/**
 * ewmConnect() - Start EWM.  Returns TRUE if started, FALSE if is currently stated (TRUE
 * is action taken).
 */
extern BREthereumBoolean
ewmConnect(BREthereumEWM ewm) {

    // Nothing to do if already connected
    if (ETHEREUM_BOOLEAN_IS_TRUE (ewmIsConnected(ewm)))
        return ETHEREUM_BOOLEAN_FALSE;

    // Set ewm {client,state} prior to bcs/event start.  Avoid race conditions, particularly
    // with `ewmPeriodicDispatcher`.
    ewm->state = LIGHT_NODE_CONNECTED;

    switch (ewm->mode) {
        case BRD_ONLY:
            break;
        case BRD_WITH_P2P_SEND:
        case P2P_WITH_BRD_SYNC:
        case P2P_ONLY:
            bcsStart(ewm->bcs);
            break;
    }

    eventHandlerStart(ewm->handler);

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

    if (ETHEREUM_BOOLEAN_IS_FALSE (ewmIsConnected(ewm)))
        return ETHEREUM_BOOLEAN_FALSE;

    // Set ewm->state thereby stopping handlers (in a race with bcs/event calls).
    ewm->state = LIGHT_NODE_DISCONNECTED;

    switch (ewm->mode) {
        case BRD_ONLY:
            break;
        case BRD_WITH_P2P_SEND:
        case P2P_WITH_BRD_SYNC:
        case P2P_ONLY:
            bcsStop(ewm->bcs);
            break;
    }

    eventHandlerStop(ewm->handler);

    return ETHEREUM_BOOLEAN_TRUE;
}

extern BREthereumBoolean
ewmIsConnected (BREthereumEWM ewm) {
    if (LIGHT_NODE_CONNECTED != ewm->state) return ETHEREUM_BOOLEAN_FALSE;

    switch (ewm->mode) {
        case BRD_ONLY:
            return ETHEREUM_BOOLEAN_TRUE;

        case BRD_WITH_P2P_SEND:
        case P2P_WITH_BRD_SYNC:
        case P2P_ONLY:
            return bcsIsStarted (ewm->bcs);
    }
}

extern BREthereumNetwork
ewmGetNetwork (BREthereumEWM ewm) {
    return ewm->network;
}

extern BREthereumAccount
ewmGetAccount (BREthereumEWM ewm) {
    return ewm->account;
}

extern char *
ewmGetAccountPrimaryAddress(BREthereumEWM ewm) {
    return accountGetPrimaryAddressString(ewmGetAccount(ewm));
}

extern BRKey // key.pubKey
ewmGetAccountPrimaryAddressPublicKey(BREthereumEWM ewm) {
    return accountGetPrimaryAddressPublicKey(ewmGetAccount(ewm));
}

extern BRKey
ewmGetAccountPrimaryAddressPrivateKey(BREthereumEWM ewm,
                                           const char *paperKey) {
    return accountGetPrimaryAddressPrivateKey (ewmGetAccount(ewm), paperKey);

}

///
/// Sync
///
typedef struct {
    BREthereumEWM ewm;
    uint64_t begBlockNumber;
    uint64_t endBlockNumber;
} BREthereumSyncTransferContext;

static int
ewmSyncUpdateTransferPredicate (BREthereumSyncTransferContext *context,
                                BREthereumTransfer transfer,
                                unsigned int index) {
    uint64_t blockNumber = 0;

    // Do we return true for anything besides 'included' - like for 'error'.  For 'error' the
    // answer is 'no - because the blockchain has no information about non-included transactios
    // and logs'.  The other status types (created, submitted, etc) will either be resolved by
    // another sync or won't matter.
    
    return (transferExtractStatusIncluded (transfer, NULL, &blockNumber, NULL, NULL, NULL) &&
            context->begBlockNumber <= blockNumber && blockNumber <= context->endBlockNumber);
}

static void
ewmSyncUpdateTransfer (BREthereumSyncTransferContext *context,
                       BREthereumTransfer transfer,
                       unsigned int index) {
    BREthereumTransaction transaction = transferGetBasisTransaction (transfer);
    BREthereumLog log = transferGetBasisLog (transfer);
    BREthereumTransactionStatus status = transactionStatusCreate (TRANSACTION_STATUS_PENDING);

    // Assert only one of transfer or log exists.
    assert (NULL == transaction || NULL == log);

    if (NULL != transaction) {
        transaction = transactionCopy (transaction);
        transactionSetStatus (transaction, status);
        ewmSignalTransaction (context->ewm, BCS_CALLBACK_TRANSACTION_UPDATED, transaction);
    }

    if (NULL != log) {
        log = logCopy(log);
        logSetStatus (log, status);
        ewmSignalLog (context->ewm, BCS_CALLBACK_LOG_UPDATED, log);
    }

    return;
}

extern BREthereumBoolean
ewmSync (BREthereumEWM ewm) {
    if (LIGHT_NODE_CONNECTED != ewm->state) return ETHEREUM_BOOLEAN_FALSE;

    switch (ewm->mode) {
        case BRD_ONLY:
        case BRD_WITH_P2P_SEND: {
            pthread_mutex_lock(&ewm->lock);

            BREthereumSyncTransferContext context = { ewm, 0, ewm->blockHeight };
            BREthereumWallet *wallets = ewmGetWallets(ewm);

            // Walk each wallet, set all transfers to 'pending'
            for (size_t wid = 0; NULL != wallets[wid]; wid++)
                walletWalkTransfers (wallets[wid], &context,
                                     (BREthereumTransferPredicate) ewmSyncUpdateTransferPredicate,
                                     (BREthereumTransferWalker)    ewmSyncUpdateTransfer);

            free (wallets);

            // Start a sync from block 0
            ewm->brdSync.begBlockNumber = 0;

            // Try to avoid letting a nearly completed sync from continuing.
            ewm->brdSync.completedTransaction = 0;
            ewm->brdSync.completedLog = 0;
            pthread_mutex_unlock(&ewm->lock);
            return ETHEREUM_BOOLEAN_TRUE;
        }
        case P2P_WITH_BRD_SYNC:
        case P2P_ONLY:
            bcsSync (ewm->bcs, 0);
            return ETHEREUM_BOOLEAN_TRUE;
    }
}

///
/// MARK: - Blocks
///
#if defined (NEVER_DEFINED)
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
    ewmSignalBlockEvent(ewm, bid, BLOCK_EVENT_CREATED, SUCCESS, NULL);
    return bid;
}
#endif

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
#if defined (NEVER_DEFINED)
extern BREthereumTransfer
ewmLookupTransfer (BREthereumEWM ewm,
                   BREthereumTransfer transfer) {
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
    BREthereumTransfer transfer = -1;

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
    BREthereumTransfer transfer;

    pthread_mutex_lock(&ewm->lock);
    array_add (ewm->transfers, transfer);
    tid = (BREthereumTransferId) (array_count(ewm->transfers) - 1);
    pthread_mutex_unlock(&ewm->lock);

    return tid;
}

extern void
ewmDeleteTransfer (BREthereumEWM ewm,
                   BREthereumTransfer transfer) {
    BREthereumTransfer transfer = ewm->transfers[tid];
    if (NULL == transfer) return;

    // Remove from any (and all - should be but one) wallet
    for (int wid = 0; wid < array_count(ewm->wallets); wid++)
        if (walletHasTransfer(ewm->wallets[wid], transfer)) {
            walletUnhandleTransfer(ewm->wallets[wid], transfer);
            ewmSignalTransferEvent(ewm, wid, tid, TRANSFER_EVENT_DELETED, SUCCESS, NULL);
        }

    // Null the ewm's `tid` - MUST NOT array_rm() as all `tid` holders will be dead.
    ewm->transfers[tid] = NULL;
    transferRelease(transfer);
}
#endif

///
/// MARK: Wallets
///
#if defined (NEVER_DEFINED)
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
#endif
extern void
ewmInsertWallet (BREthereumEWM ewm,
                 BREthereumWallet wallet) {
    pthread_mutex_lock(&ewm->lock);
    array_add (ewm->wallets, wallet);
    pthread_mutex_unlock(&ewm->lock);
    ewmSignalWalletEvent(ewm, wallet, WALLET_EVENT_CREATED, SUCCESS, NULL);
}

//
// Wallet (Actions)
//
extern BREthereumWallet *
ewmGetWallets (BREthereumEWM ewm) {
    pthread_mutex_lock(&ewm->lock);

    unsigned long count = array_count(ewm->wallets);
    BREthereumWallet *wallets = calloc (count + 1, sizeof (BREthereumWallet));

    for (size_t index = 0; index < count; index++) {
        wallets [index] = ewm->wallets[index];
    }
    wallets[count] = NULL;

    pthread_mutex_unlock(&ewm->lock);
    return wallets;
}

extern unsigned int
ewmGetWalletsCount (BREthereumEWM ewm) {
    return (unsigned int) array_count(ewm->wallets);
}

extern BREthereumWallet
ewmGetWallet(BREthereumEWM ewm) {
    return ewm->walletHoldingEther;
}

extern BREthereumWallet
ewmGetWalletHoldingToken(BREthereumEWM ewm,
                         BREthereumToken token) {
    BREthereumWallet wallet = NULL;

    pthread_mutex_lock(&ewm->lock);
    for (int i = 0; i < array_count(ewm->wallets); i++)
        if (token == walletGetToken(ewm->wallets[i])) {
            wallet = ewm->wallets[i];
            break;
        }

    if (NULL == wallet) {
        wallet = walletCreateHoldingToken(ewm->account,
                                          ewm->network,
                                          token);
        ewmInsertWallet(ewm, wallet);
    }
    pthread_mutex_unlock(&ewm->lock);
    return wallet;
}


extern BREthereumTransfer
ewmWalletCreateTransfer(BREthereumEWM ewm,
                        BREthereumWallet wallet,
                        const char *recvAddress,
                        BREthereumAmount amount) {
    BREthereumTransfer transfer = NULL;

    pthread_mutex_lock(&ewm->lock);

    transfer = walletCreateTransfer(wallet, addressCreate(recvAddress), amount);

    pthread_mutex_unlock(&ewm->lock);

    // Transfer DOES NOT have a hash yet because it is not signed; but it is inserted in the
    // wallet and can be display, in order, w/o the hash
    ewmSignalTransferEvent (ewm, wallet, transfer, TRANSFER_EVENT_CREATED, SUCCESS, NULL);

    return transfer;
}

extern BREthereumTransfer
ewmWalletCreateTransferGeneric(BREthereumEWM ewm,
                               BREthereumWallet wallet,
                               const char *recvAddress,
                               BREthereumEther amount,
                               BREthereumGasPrice gasPrice,
                               BREthereumGas gasLimit,
                               const char *data) {
    BREthereumTransfer transfer = NULL;

    pthread_mutex_lock(&ewm->lock);

    transfer = walletCreateTransferGeneric(wallet,
                                              addressCreate(recvAddress),
                                              amount,
                                              gasPrice,
                                              gasLimit,
                                              data);

    pthread_mutex_unlock(&ewm->lock);

    // Transfer DOES NOT have a hash yet because it is not signed; but it is inserted in the
    // wallet and can be display, in order, w/o the hash
    ewmSignalTransferEvent(ewm, wallet, transfer, TRANSFER_EVENT_CREATED, SUCCESS, NULL);

    return transfer;
}

extern BREthereumTransfer
ewmWalletCreateTransferWithFeeBasis (BREthereumEWM ewm,
                                     BREthereumWallet wallet,
                                     const char *recvAddress,
                                     BREthereumAmount amount,
                                     BREthereumFeeBasis feeBasis) {
    BREthereumTransfer transfer = NULL;

    pthread_mutex_lock(&ewm->lock);
    {
        transfer = walletCreateTransferWithFeeBasis (wallet, addressCreate(recvAddress), amount, feeBasis);
    }
    pthread_mutex_unlock(&ewm->lock);

    // Transfer DOES NOT have a hash yet because it is not signed; but it is inserted in the
    // wallet and can be display, in order, w/o the hash
    ewmSignalTransferEvent (ewm, wallet, transfer, TRANSFER_EVENT_CREATED, SUCCESS, NULL);

    return transfer;
}

extern BREthereumEther
ewmWalletEstimateTransferFee(BREthereumEWM ewm,
                             BREthereumWallet wallet,
                             BREthereumAmount amount,
                             int *overflow) {
    return walletEstimateTransferFee(wallet, amount, overflow);
}

extern BREthereumBoolean
ewmWalletCanCancelTransfer (BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumTransfer oldTransfer) {
    BREthereumTransaction oldTransaction = transferGetOriginatingTransaction(oldTransfer);

    // TODO: Something about the 'status' (not already cancelled, etc)
    return AS_ETHEREUM_BOOLEAN (NULL != oldTransaction);
}

extern BREthereumTransfer // status, error
ewmWalletCreateTransferToCancel(BREthereumEWM ewm,
                                BREthereumWallet wallet,
                                BREthereumTransfer oldTransfer) {
    BREthereumTransaction oldTransaction = transferGetOriginatingTransaction(oldTransfer);

    assert (NULL != oldTransaction);

    int overflow;
    BREthereumEther oldGasPrice = transactionGetGasPrice(oldTransaction).etherPerGas;
    BREthereumEther newGasPrice = etherAdd (oldGasPrice, oldGasPrice, &overflow);

    // Create a new transaction with: a) targetAddress to self (sourceAddress), b) 0 ETH, c)
    // gasPrice increased (to replacement value).
    BREthereumTransaction transaction =
    transactionCreate (transactionGetSourceAddress(oldTransaction),
                       transactionGetSourceAddress(oldTransaction),
                       etherCreateZero(),
                       gasPriceCreate(newGasPrice),
                       transactionGetGasLimit(oldTransaction),
                       strdup (transactionGetData(oldTransaction)),
                       transactionGetNonce(oldTransaction));

    transferSetStatus(oldTransfer, TRANSFER_STATUS_REPLACED);

    // Delete transfer??  Update transfer??
    BREthereumTransfer transfer = transferCreateWithTransactionOriginating (transaction,
                                                                            (NULL == walletGetToken(wallet)
                                                                             ? TRANSFER_BASIS_TRANSACTION
                                                                             : TRANSFER_BASIS_LOG));
    walletHandleTransfer(wallet, transfer);
    return transfer;
}

extern BREthereumBoolean
ewmWalletCanReplaceTransfer (BREthereumEWM ewm,
                             BREthereumWallet wid,
                             BREthereumTransfer oldTransfer) {
    BREthereumTransaction oldTransaction = transferGetOriginatingTransaction(oldTransfer);

    // TODO: Something about the 'status' (not already replaced, etc)
    return AS_ETHEREUM_BOOLEAN (NULL != oldTransaction);
}

extern BREthereumTransfer // status, error
ewmWalletCreateTransferToReplace (BREthereumEWM ewm,
                                  BREthereumWallet wallet,
                                  BREthereumTransfer oldTransfer,
                                  // ...
                                  BREthereumBoolean updateGasPrice,
                                  BREthereumBoolean updateGasLimit,
                                  BREthereumBoolean updateNonce) {
    BREthereumTransaction oldTransaction = transferGetOriginatingTransaction(oldTransfer);

    assert (NULL != oldTransaction);

    BREthereumAccount account =  ewmGetAccount(ewm);
    BREthereumAddress address = transactionGetSourceAddress(oldTransaction);

    int overflow = 0;

    // The old nonce
    uint64_t nonce = transactionGetNonce(oldTransaction);
    if (ETHEREUM_BOOLEAN_IS_TRUE(updateNonce)) {
        // Nonce is 100% low.  Update the account's nonce to be at least nonce.
        if (nonce <= accountGetAddressNonce (account, address))
            accountSetAddressNonce (account, address, nonce + 1, ETHEREUM_BOOLEAN_TRUE);

        // Nonce is surely 1 larger or more (if nonce was behind the account's nonce)
        nonce = accountGetThenIncrementAddressNonce (account, address);
    }

    BREthereumGasPrice gasPrice = transactionGetGasPrice(oldTransaction);
    if (ETHEREUM_BOOLEAN_IS_TRUE (updateGasPrice)) {
        gasPrice = gasPriceCreate (etherAdd (gasPrice.etherPerGas, gasPrice.etherPerGas, &overflow)); // double
        assert (0 == overflow);
    }

    BREthereumGas gasLimit = transactionGetGasLimit (oldTransaction);
    if (ETHEREUM_BOOLEAN_IS_TRUE (updateGasLimit))
        gasLimit = gasCreate (gasLimit.amountOfGas + gasLimit.amountOfGas); // double

    BREthereumTransaction transaction =
    transactionCreate (transactionGetSourceAddress(oldTransaction),
                       transactionGetTargetAddress(oldTransaction),
                       transactionGetAmount(oldTransaction),
                       gasPrice,
                       gasLimit,
                       strdup (transactionGetData(oldTransaction)),
                       nonce);

    transferSetStatus(oldTransfer, TRANSFER_STATUS_REPLACED);

    // Delete transfer??  Update transfer??
    BREthereumTransfer transfer = transferCreateWithTransactionOriginating (transaction,
                                                                            (NULL == walletGetToken(wallet)
                                                                             ? TRANSFER_BASIS_TRANSACTION
                                                                             : TRANSFER_BASIS_LOG));
    walletHandleTransfer(wallet, transfer);
    return transfer;
}


static void
ewmWalletSignTransferAnnounce (BREthereumEWM ewm,
                               BREthereumWallet wallet,
                               BREthereumTransfer transfer) {
    ewmSignalTransferEvent (ewm, wallet, transfer, TRANSFER_EVENT_SIGNED,  SUCCESS, NULL);
}

extern void // status, error
ewmWalletSignTransfer (BREthereumEWM ewm,
                       BREthereumWallet wallet,
                       BREthereumTransfer transfer,
                       BRKey privateKey) {
    pthread_mutex_lock(&ewm->lock);
    walletSignTransferWithPrivateKey (wallet, transfer, privateKey);
    pthread_mutex_unlock(&ewm->lock);
    ewmWalletSignTransferAnnounce (ewm, wallet, transfer);
}

extern void // status, error
ewmWalletSignTransferWithPaperKey(BREthereumEWM ewm,
                                  BREthereumWallet wallet,
                                  BREthereumTransfer transfer,
                                  const char *paperKey) {
    pthread_mutex_lock(&ewm->lock);
    walletSignTransfer (wallet, transfer, paperKey);
    pthread_mutex_unlock(&ewm->lock);
    ewmWalletSignTransferAnnounce (ewm, wallet, transfer);
}

extern BREthereumTransfer *
ewmWalletGetTransfers(BREthereumEWM ewm,
                      BREthereumWallet wallet) {
    pthread_mutex_lock(&ewm->lock);

    unsigned long count = walletGetTransferCount(wallet);
    BREthereumTransfer *transfers = calloc (count + 1, sizeof (BREthereumTransfer));

    for (unsigned long index = 0; index < count; index++)
        transfers [index] = walletGetTransferByIndex (wallet, index);
    transfers[count] = NULL;

    pthread_mutex_unlock(&ewm->lock);
    return transfers;
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

extern BREthereumToken
ewmWalletGetToken (BREthereumEWM ewm,
                   BREthereumWallet wallet) {
    return walletGetToken(wallet);
}

extern BREthereumAmount
ewmWalletGetBalance(BREthereumEWM ewm,
                    BREthereumWallet wallet) {
    return walletGetBalance(wallet);
}


extern BREthereumGas
ewmWalletGetGasEstimate(BREthereumEWM ewm,
                        BREthereumWallet wallet,
                        BREthereumTransfer transfer) {
    return transferGetGasEstimate(transfer);

}

extern BREthereumGas
ewmWalletGetDefaultGasLimit(BREthereumEWM ewm,
                            BREthereumWallet wallet) {
    return walletGetDefaultGasLimit(wallet);
}

extern void
ewmWalletSetDefaultGasLimit(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumGas gasLimit) {
    walletSetDefaultGasLimit(wallet, gasLimit);
    ewmSignalWalletEvent(ewm,
                               wallet,
                               WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED,
                               SUCCESS,
                               NULL);
}

extern BREthereumGasPrice
ewmWalletGetDefaultGasPrice(BREthereumEWM ewm,
                                 BREthereumWallet wallet) {
    return walletGetDefaultGasPrice(wallet);
}

extern void
ewmWalletSetDefaultGasPrice(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumGasPrice gasPrice) {
    walletSetDefaultGasPrice(wallet, gasPrice);
    ewmSignalWalletEvent(ewm,
                               wallet,
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
    
    ewmSignalWalletEvent(ewm,
                                 wallet,
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
    
    ewmSignalTransferEvent(ewm,
                                      wallet,
                                      transfer,
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
    // Don't report during BCS sync.
    if (BRD_ONLY == ewm->mode || ETHEREUM_BOOLEAN_IS_FALSE(bcsSyncInProgress (ewm->bcs)))
        eth_log ("EWM", "BlockChain: %" PRIu64, headBlockNumber);

    // At least this - allows for: ewmGetBlockHeight
    ewm->blockHeight = headBlockNumber;

#if defined (NEVER_DEFINED)
    // TODO: Need a 'block id' - or axe the need of 'block id'?
    ewmSignalBlockEvent (ewm,
                               (BREthereumBlockId) 0,
                               BLOCK_EVENT_CHAINED,
                               SUCCESS,
                               NULL);
#endif
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

    eth_log("EWM", "AccountState: Nonce: %" PRIu64, accountState.nonce);

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

    BREthereumWallet wallet = (AMOUNT_ETHER == amountGetType(amount)
                               ? ewmGetWallet(ewm)
                               : ewmGetWalletHoldingToken(ewm, amountGetToken (amount)));
    walletSetBalance(wallet, amount);

    ewmSignalWalletEvent(ewm,
                               wallet,
                               WALLET_EVENT_BALANCE_UPDATED,
                               SUCCESS,
                               NULL);

    pthread_mutex_unlock(&ewm->lock);
}

static void
ewmReportTransferStatusAsEvent (BREthereumEWM ewm,
                                BREthereumWallet wallet,
                                BREthereumTransfer transfer) {
    if (ETHEREUM_BOOLEAN_IS_TRUE (transferHasStatus (transfer, TRANSFER_STATUS_SUBMITTED)))
        ewmSignalTransferEvent(ewm, wallet, transfer,
                               TRANSFER_EVENT_SUBMITTED,
                               SUCCESS, NULL);

    else if (ETHEREUM_BOOLEAN_IS_TRUE (transferHasStatus (transfer, TRANSFER_STATUS_INCLUDED)))
        ewmSignalTransferEvent(ewm, wallet, transfer,
                               TRANSFER_EVENT_INCLUDED,
                               SUCCESS, NULL);

    else if (ETHEREUM_BOOLEAN_IS_TRUE (transferHasStatus (transfer, TRANSFER_STATUS_ERRORED))) {
        char *reason = NULL;
        transferExtractStatusError (transfer, &reason);
        ewmSignalTransferEvent(ewm, wallet, transfer,
                               TRANSFER_EVENT_ERRORED,
                               ERROR_TRANSACTION_SUBMISSION,
                               (NULL == reason ? "" : reason));
        // TODO: free(reason)?
        // Note: ewmSignalTransferEvent expects the 'reason' to stick around an never frees it.
        // If we free here, the string will be gone by the time it is handled.
    }
}

static void
ewmHandleTransactionOriginatingLog (BREthereumEWM ewm,
                                     BREthereumBCSCallbackTransactionType type,
                                    OwnershipKept BREthereumTransaction transaction) {
    BREthereumHash hash = transactionGetHash(transaction);
    for (size_t wid = 0; wid < array_count(ewm->wallets); wid++) {
        BREthereumWallet wallet = ewm->wallets[wid];

        // We already handle the ETH wallet.  See ewmHandleTransaction.
        if (wallet == ewm->walletHoldingEther) continue;

        BREthereumTransfer transfer = walletGetTransferByOriginatingHash (wallet, hash);
        if (NULL != transfer) {
            // If this transaction is the transfer's originatingTransaction, then update the
            // originatingTransaction's status.
            BREthereumTransaction original = transferGetOriginatingTransaction (transfer);
            if (NULL != original && ETHEREUM_BOOLEAN_IS_TRUE(hashEqual (transactionGetHash(original),
                                                                        transactionGetHash(transaction))))
                transactionSetStatus (original, transactionGetStatus(transaction));

            //
            transferSetStatusForBasis (transfer, transactionGetStatus(transaction));

            // NOTE: So `transaction` applies to `transfer`.  If the transfer's basis is 'log'
            // then we'd like to update the log's identifier.... alas, we cannot because we need
            // the 'logIndex' and no way to get that from the originating transaction's status.

            ewmReportTransferStatusAsEvent(ewm, wallet, transfer);
        }
    }
}

extern void
ewmHandleTransaction (BREthereumEWM ewm,
                      BREthereumBCSCallbackTransactionType type,
                      OwnershipGiven BREthereumTransaction transaction) {
    BREthereumHash hash = transactionGetHash(transaction);

    BREthereumHashString hashString;
    hashFillString(hash, hashString);
    eth_log ("EWM", "Transaction: \"%s\", Change: %s, Status: %d", hashString,
             BCS_CALLBACK_TRANSACTION_TYPE_NAME(type),
             transactionGetStatus(transaction).type);

    // Find the wallet
    BREthereumWallet wallet = ewmGetWallet(ewm);
    assert (NULL != wallet);

    // Find a preexisting transfer
    BREthereumTransfer transfer = walletGetTransferByHash(wallet, hash);

    // If we've no transfer, then create one and save `transaction` as the basis
    if (NULL == transfer) {
        transfer = transferCreateWithTransaction(transaction);

        walletHandleTransfer(wallet, transfer);
        walletUpdateBalance (wallet);

        ewmSignalTransferEvent(ewm, wallet, transfer,
                                     TRANSFER_EVENT_CREATED,
                                     SUCCESS, NULL);

        ewmSignalWalletEvent(ewm, wallet, WALLET_EVENT_BALANCE_UPDATED,
                                   SUCCESS,
                                   NULL);

    }
    else {
        // If this transaction is the transfer's originatingTransaction, then update the
        // originatingTransaction's status.
        BREthereumTransaction original = transferGetOriginatingTransaction (transfer);
        if (NULL != original && ETHEREUM_BOOLEAN_IS_TRUE(hashEqual (transactionGetHash(original),
                                                                    transactionGetHash(transaction))))
            transactionSetStatus (original, transactionGetStatus(transaction));

        transferSetBasisForTransaction (transfer, transaction);
    }

    ewmReportTransferStatusAsEvent(ewm, wallet, transfer);

    ewmHandleTransactionOriginatingLog (ewm, type, transaction);
}

extern void
ewmHandleLog (BREthereumEWM ewm,
              BREthereumBCSCallbackLogType type,
              OwnershipGiven BREthereumLog log) {
    BREthereumHash logHash = logGetHash(log);

    BREthereumHash transactionHash;
    size_t logIndex;
    logExtractIdentifier(log, &transactionHash, &logIndex);

    BREthereumHashString logHashString;
    hashFillString(logHash, logHashString);

    BREthereumHashString transactionHashString;
    hashFillString(transactionHash, transactionHashString);

    eth_log ("EWM", "Log: %s { %8s @ %zu }, Change: %s",
             logHashString, transactionHashString, logIndex, BCS_CALLBACK_TRANSACTION_TYPE_NAME(type));

    BREthereumToken token = tokenLookupByAddress(logGetAddress(log));
    if (NULL == token) return;

    // TODO: Confirm LogTopic[0] is 'transfer'
    if (3 != logGetTopicsCount(log)) return;

    BREthereumWallet wallet = ewmGetWalletHoldingToken(ewm, token);
    assert (NULL != wallet);

    BREthereumTransfer transfer = walletGetTransferByHash(wallet, logHash);

    // If we've no transfer, then create one and save `log` as the basis
    if (NULL == transfer) {
        transfer = transferCreateWithLog (log, token, ewm->coder);

        walletHandleTransfer(wallet, transfer);
        walletUpdateBalance (wallet);

        ewmSignalTransferEvent(ewm, wallet, transfer,
                                     TRANSFER_EVENT_CREATED,
                                     SUCCESS, NULL);

        ewmSignalWalletEvent(ewm, wallet, WALLET_EVENT_BALANCE_UPDATED,
                                   SUCCESS,
                                   NULL);
    }
    else {
        transferSetBasisForLog (transfer, log);
    }

    ewmReportTransferStatusAsEvent(ewm, wallet, transfer);
}

extern void
ewmHandleSaveBlocks (BREthereumEWM ewm,
                     OwnershipGiven BRArrayOf(BREthereumBlock) blocks) {
    size_t count = array_count(blocks);

    eth_log("EWM", "Save Blocks (Storage): %zu", count);
    fileServiceClear(ewm->fs, fileServiceTypeBlocks);

    for (size_t index = 0; index < count; index++)
        fileServiceSave (ewm->fs, fileServiceTypeBlocks, blocks[index]);
    array_free (blocks);
}

extern void
ewmHandleSaveNodes (BREthereumEWM ewm,
                    OwnershipGiven BRArrayOf(BREthereumNodeConfig) nodes) {
    size_t count = array_count(nodes);

    eth_log("EWM", "Save Nodes (Storage): %zu", count);
    fileServiceClear(ewm->fs, fileServiceTypeNodes);

    for (size_t index = 0; index < count; index++)
        fileServiceSave(ewm->fs, fileServiceTypeNodes, nodes[index]);

    array_free (nodes);
}

extern void
ewmHandleSaveTransaction (BREthereumEWM ewm,
                          BREthereumTransaction transaction,
                          BREthereumClientChangeType type) {
    BREthereumHash hash = transactionGetHash(transaction);
    BREthereumHashString fileName;
    hashFillString(hash, fileName);

    eth_log("EWM", "Transaction: Save: %s: %s",
            CLIENT_CHANGE_TYPE_NAME (type),
            fileName);

    if (CLIENT_CHANGE_REM == type || CLIENT_CHANGE_UPD == type)
        fileServiceRemove (ewm->fs, fileServiceTypeTransactions,
                           fileServiceTypeTransactionV1Identifier (ewm, ewm->fs, transaction));

    if (CLIENT_CHANGE_ADD == type || CLIENT_CHANGE_UPD == type)
        fileServiceSave (ewm->fs, fileServiceTypeTransactions, transaction);
}

extern void
ewmHandleSaveLog (BREthereumEWM ewm,
                  BREthereumLog log,
                  BREthereumClientChangeType type) {
    BREthereumHash hash = logGetHash(log);
    BREthereumHashString filename;
    hashFillString(hash, filename);

    eth_log("EWM", "Log: Save: %s: %s",
            CLIENT_CHANGE_TYPE_NAME (type),
            filename);

    if (CLIENT_CHANGE_REM == type || CLIENT_CHANGE_UPD == type)
        fileServiceRemove (ewm->fs, fileServiceTypeLogs,
                           fileServiceTypeLogV1Identifier(ewm, ewm->fs, log));

    if (CLIENT_CHANGE_ADD == type || CLIENT_CHANGE_UPD == type)
        fileServiceSave (ewm->fs, fileServiceTypeLogs, log);
}

extern void
ewmHandleSync (BREthereumEWM ewm,
               BREthereumBCSCallbackSyncType type,
               uint64_t blockNumberStart,
               uint64_t blockNumberCurrent,
               uint64_t blockNumberStop) {
    assert (P2P_ONLY == ewm->mode || P2P_WITH_BRD_SYNC == ewm->mode);

    BREthereumEWMEvent event = (blockNumberCurrent == blockNumberStart
                                ? EWM_EVENT_SYNC_STARTED
                                : (blockNumberCurrent == blockNumberStop
                                   ? EWM_EVENT_SYNC_STOPPED
                                   : EWM_EVENT_SYNC_CONTINUES));
    double syncCompletePercent = 100.0 * (blockNumberCurrent - blockNumberStart) / (blockNumberStop - blockNumberStart);
    
    ewmSignalEWMEvent (ewm, event, SUCCESS, NULL);

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
    if (P2P_ONLY == ewm->mode || P2P_WITH_BRD_SYNC == ewm->mode) return;

    ewmUpdateBlockNumber(ewm);
    ewmUpdateNonce(ewm);

    // Handle a BRD Sync:

    // 1) check if the prior sync has completed.
    if (ewm->brdSync.completedTransaction && ewm->brdSync.completedLog) {
        // 1a) if so, advance the sync range by updating `begBlockNumber`
        ewm->brdSync.begBlockNumber = (ewm->brdSync.endBlockNumber >=  EWM_BRD_SYNC_START_BLOCK_OFFSET
                                       ? ewm->brdSync.endBlockNumber - EWM_BRD_SYNC_START_BLOCK_OFFSET
                                       : 0);
    }
    // 2) completed or not, update the `endBlockNumber` to the current block height.
    ewm->brdSync.endBlockNumber = ewmGetBlockHeight(ewm);

    // 3) We'll query all transactions for this ewm's account.  That will give us a shot at
    // getting the nonce for the account's address correct.  We'll save all the transactions and
    // then process them into wallet as wallets exist.
    ewmUpdateTransactions(ewm);
    // Note: we don't do the following  `ewmUpdateTransactions` because that is `extern`
    ewm->brdSync.ridTransaction = ewm->requestId;
    ewm->brdSync.completedTransaction = 0;
    
    // 4) Similarly, we'll query all logs for this ewm's account.  We'll process these into
    // (token) transactions and associate with their wallet.
    ewmUpdateLogs(ewm, NULL, eventERC20Transfer);
    // Note: we don't do the following  `ewmUpdateLogs because that is `extern`
    ewm->brdSync.ridLog = ewm->requestId;
    ewm->brdSync.completedLog = 0;

    // End handling a BRD Sync
    
    // For all the known wallets, get their balance.
    for (int i = 0; i < array_count(ewm->wallets); i++)
        ewmUpdateWalletBalance (ewm, ewm->wallets[i]);
}

#if 1 // defined(SUPPORT_JSON_RPC)

extern void
ewmTransferFillRawData (BREthereumEWM ewm,
                             BREthereumWallet wallet,
                             BREthereumTransfer transfer,
                             uint8_t **bytesPtr, size_t *bytesCountPtr) {
    assert (NULL != bytesCountPtr && NULL != bytesPtr);

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
ewmTransferGetRawDataHexEncoded(BREthereumEWM ewm,
                                        BREthereumWallet wallet,
                                        BREthereumTransfer transfer,
                                        const char *prefix) {
    assert (walletHasTransfer(wallet, transfer));

    BREthereumTransaction transaction = transferGetOriginatingTransaction (transfer);
    assert (NULL != transaction);

    return transactionGetRlpHexEncoded(transaction,
                                ewm->network,
                                (ETHEREUM_BOOLEAN_IS_TRUE (transactionIsSigned(transaction))
                                 ? RLP_TYPE_TRANSACTION_SIGNED
                                 : RLP_TYPE_TRANSACTION_UNSIGNED),
                                prefix);
}


///
/// MARK: - Transfer
///
extern BREthereumAddress
ewmTransferGetTarget (BREthereumEWM ewm,
                           BREthereumTransfer transfer) {
    return transferGetTargetAddress(transfer);
}

extern BREthereumAddress
ewmTransferGetSource (BREthereumEWM ewm,
                           BREthereumTransfer transfer) {
    return transferGetSourceAddress(transfer);
}

extern BREthereumHash
ewmTransferGetHash(BREthereumEWM ewm,
                        BREthereumTransfer transfer) {
    return transferGetHash(transfer);
}

extern char *
ewmTransferGetAmountEther(BREthereumEWM ewm,
                               BREthereumTransfer transfer,
                               BREthereumEtherUnit unit) {
    BREthereumAmount amount = transferGetAmount(transfer);
    return (AMOUNT_ETHER == amountGetType(amount)
            ? etherGetValueString(amountGetEther(amount), unit)
            : "");
}

extern char *
ewmTransferGetAmountTokenQuantity(BREthereumEWM ewm,
                                       BREthereumTransfer transfer,
                                       BREthereumTokenQuantityUnit unit) {
    BREthereumAmount amount = transferGetAmount(transfer);
    return (AMOUNT_TOKEN == amountGetType(amount)
            ? tokenQuantityGetValueString(amountGetTokenQuantity(amount), unit)
            : "");
}

extern BREthereumAmount
ewmTransferGetAmount(BREthereumEWM ewm,
                          BREthereumTransfer transfer) {
    return transferGetAmount(transfer);
}

extern BREthereumGasPrice
ewmTransferGetGasPrice(BREthereumEWM ewm,
                            BREthereumTransfer transfer,
                            BREthereumEtherUnit unit) {
    return feeBasisGetGasPrice (transferGetFeeBasis(transfer));
}

extern BREthereumGas
ewmTransferGetGasLimit(BREthereumEWM ewm,
                            BREthereumTransfer transfer) {
    return feeBasisGetGasLimit(transferGetFeeBasis(transfer));
}

extern uint64_t
ewmTransferGetNonce(BREthereumEWM ewm,
                         BREthereumTransfer transfer) {
    return transferGetNonce(transfer);
}

extern BREthereumGas
ewmTransferGetGasUsed(BREthereumEWM ewm,
                           BREthereumTransfer transfer) {
    BREthereumGas gasUsed;
    return (transferExtractStatusIncluded(transfer, NULL, NULL, NULL, NULL, &gasUsed)
            ? gasUsed
            : gasCreate(0));
}

extern uint64_t
ewmTransferGetTransactionIndex(BREthereumEWM ewm,
                                    BREthereumTransfer transfer) {
    uint64_t transactionIndex;
    return (transferExtractStatusIncluded(transfer, NULL, NULL, &transactionIndex, NULL, NULL)
            ? transactionIndex
            : 0);
}

extern BREthereumHash
ewmTransferGetBlockHash(BREthereumEWM ewm,
                             BREthereumTransfer transfer) {
    BREthereumHash blockHash;
    return (transferExtractStatusIncluded(transfer, &blockHash, NULL, NULL, NULL, NULL)
            ? blockHash
            : hashCreateEmpty());
}

extern uint64_t
ewmTransferGetBlockNumber(BREthereumEWM ewm,
                               BREthereumTransfer transfer) {
    uint64_t blockNumber;
    return (transferExtractStatusIncluded(transfer, NULL, &blockNumber, NULL, NULL, NULL)
            ? blockNumber
            : 0);
}

extern uint64_t
ewmTransferGetBlockTimestamp (BREthereumEWM ewm,
                              BREthereumTransfer transfer) {
    uint64_t blockTimestamp;
    return (transferExtractStatusIncluded(transfer, NULL, NULL, NULL, &blockTimestamp, NULL)
            ? blockTimestamp
            : TRANSACTION_STATUS_BLOCK_TIMESTAMP_UNKNOWN);
}

extern uint64_t
ewmTransferGetBlockConfirmations(BREthereumEWM ewm,
                                      BREthereumTransfer transfer) {
    uint64_t blockNumber = 0;
    return (transferExtractStatusIncluded(transfer, NULL, &blockNumber, NULL, NULL, NULL)
            ? (ewmGetBlockHeight(ewm) - blockNumber)
            : 0);
}

extern BREthereumTransferStatus
ewmTransferGetStatus (BREthereumEWM ewm,
                           BREthereumTransfer transfer) {
    return transferGetStatus (transfer);
}

extern BREthereumBoolean
ewmTransferIsConfirmed(BREthereumEWM ewm,
                            BREthereumTransfer transfer) {
    return transferHasStatus (transfer, TRANSFER_STATUS_INCLUDED);
}

extern BREthereumBoolean
ewmTransferIsSubmitted(BREthereumEWM ewm,
                            BREthereumTransfer transfer) {
    return AS_ETHEREUM_BOOLEAN(ETHEREUM_BOOLEAN_IS_TRUE(transferHasStatus(transfer, TRANSFER_STATUS_SUBMITTED)) ||
                               ETHEREUM_BOOLEAN_IS_TRUE(transferHasStatusOrTwo(transfer,
                                                                               TRANSFER_STATUS_INCLUDED,
                                                                               TRANSFER_STATUS_ERRORED)));
}

extern char *
ewmTransferStatusGetError (BREthereumEWM ewm,
                                BREthereumTransfer transfer) {
    if (TRANSFER_STATUS_ERRORED == transferGetStatus(transfer)) {
        char *reason;
        transferExtractStatusError (transfer, &reason);
        return reason;
    }
    else return NULL;
}

extern int
ewmTransferStatusGetErrorType (BREthereumEWM ewm,
                                    BREthereumTransfer transfer) {
    BREthereumTransactionErrorType type;

    return (transferExtractStatusErrorType (transfer, &type)
            ? type
            : (int ) -1);
}

extern BREthereumBoolean
ewmTransferHoldsToken(BREthereumEWM ewm,
                           BREthereumTransfer transfer,
                           BREthereumToken token) {
    assert (NULL != transfer);
    return (token == transferGetToken(transfer)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern BREthereumToken
ewmTransferGetToken(BREthereumEWM ewm,
                         BREthereumTransfer transfer) {
    assert (NULL !=  transfer);
    return transferGetToken(transfer);
}

extern BREthereumEther
ewmTransferGetFee(BREthereumEWM ewm,
                       BREthereumTransfer transfer,
                       int *overflow) {
    assert (NULL != transfer);
    return transferGetFee(transfer, overflow);
}

///
/// MARK: - Amount
///
extern BREthereumAmount
ewmCreateEtherAmountString(BREthereumEWM ewm,
                                const char *number,
                                BREthereumEtherUnit unit,
                                BRCoreParseStatus *status) {
    return amountCreateEther (etherCreateString(number, unit, status));
}

extern BREthereumAmount
ewmCreateEtherAmountUnit(BREthereumEWM ewm,
                              uint64_t amountInUnit,
                              BREthereumEtherUnit unit) {
    return amountCreateEther (etherCreateNumber(amountInUnit, unit));
}

extern BREthereumAmount
ewmCreateTokenAmountString(BREthereumEWM ewm,
                                BREthereumToken token,
                                const char *number,
                                BREthereumTokenQuantityUnit unit,
                                BRCoreParseStatus *status) {
    return amountCreateTokenQuantityString(token, number, unit, status);
}

extern char *
ewmCoerceEtherAmountToString(BREthereumEWM ewm,
                                  BREthereumEther ether,
                                  BREthereumEtherUnit unit) {
    return etherGetValueString(ether, unit);
}

extern char *
ewmCoerceTokenAmountToString(BREthereumEWM ewm,
                                  BREthereumTokenQuantity token,
                                  BREthereumTokenQuantityUnit unit) {
    return tokenQuantityGetValueString(token, unit);
}

///
/// MARK: - Gas Price / Limit
///
extern BREthereumGasPrice
ewmCreateGasPrice (uint64_t value,
                        BREthereumEtherUnit unit) {
    return gasPriceCreate(etherCreateNumber(value, unit));
}

extern BREthereumGas
ewmCreateGas (uint64_t value) {
    return gasCreate(value);
}






extern void
ewmTransferDelete (BREthereumEWM ewm,
                   BREthereumTransfer transfer) {
    if (NULL == transfer) return;

    // Remove from any (and all - should be but one) wallet
    for (int wid = 0; wid < array_count(ewm->wallets); wid++) {
        BREthereumWallet wallet = ewm->wallets[wid];
        if (walletHasTransfer(wallet, transfer)) {
            walletUnhandleTransfer(wallet, transfer);
            ewmSignalTransferEvent(ewm, wallet, transfer, TRANSFER_EVENT_DELETED, SUCCESS, NULL);
        }
    }
    // Null the ewm's `tid` - MUST NOT array_rm() as all `tid` holders will be dead.
    transferRelease(transfer);
}

extern BREthereumFeeBasis
feeBasisCreate (BREthereumGas limit,
                BREthereumGasPrice price) {
    return (BREthereumFeeBasis) {
        FEE_BASIS_GAS,
        { .gas = { limit, price }}
    };
}

#endif // ETHEREUM_LIGHT_NODE_USE_JSON_RPC

///
/// MARK: EWM Persistent Storage


