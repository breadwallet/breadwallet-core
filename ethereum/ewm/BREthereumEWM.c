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

/* Forward Declaration */
static void
ewmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);

/* Forward Implementation */
static char *
directoryPathAppend (const char *base, const char *offset) {
    size_t length = strlen(base) + 1 + strlen(offset) + 1;
    char *path = malloc (length);

    strlcpy (path, base, length);
    strlcat (path, "/", length);
    strlcat (path, offset, length);

    return path;
}

static void directoryClear (const char *base, const char *offset) {
    char path [strlen(base) + 1 + strlen(offset) + 1];
    sprintf (path, "%s/%s", base, offset);

    struct dirent *dirEntry;
    DIR  *dir     = opendir(path);
    if (NULL == dir) {
        return;
    }
    while (NULL != (dirEntry = readdir(dir)))
        if (dirEntry->d_type == DT_REG)
            remove (dirEntry->d_name);

    closedir(dir);
}

static int directoryMake (const char *path) {
    struct stat dirStat;
    if (0 == stat  (path, &dirStat)) return 0;
    if (0 == mkdir (path, 0700)) return 0;
    return -1;
}

static char *
directoryForEthereum (const char *base) {
    return directoryPathAppend(base, "ETH");
}

static char *
filePathFromHash (const char *base, const char *offset, const UInt256 *hash) {

    size_t filenameSize = 2 * sizeof(UInt256) + 1;
    char filename [filenameSize];
    encodeHex (filename, filenameSize, hash->u8, sizeof(UInt256));

    size_t length = strlen(base) + 1 + strlen(offset) + 1 + filenameSize + 1;
    char *path = malloc (length);

    sprintf (path, "%s/%s", base, offset);
    if (-1 == directoryMake(path)) { free (path); return NULL; }

    sprintf (path, "%s/%s/%s", base, offset, filename);
    return path;
}

static char *
directoryForNetwork (const char *base, BREthereumNetwork network) {
    //    const char *net = (8333 == port ? "main" : (18333 == port ? "test" : "none"));

    if (-1 == directoryMake(base)) return NULL;

    char *forkPath = directoryForEthereum(base);
    if (-1 == directoryMake(forkPath)) { free (forkPath); return NULL; }

    char *path = directoryPathAppend (forkPath, networkGetName(network));
    if (-1 == directoryMake(path)) { free (path); free (forkPath); return NULL; }

    free (forkPath);
    return path;
}

static DIR *
directoryOpenWithSize (const char *dirPath, size_t *size, int *error) {

    DIR *dir;

    if (-1 == directoryMake(dirPath) || NULL == (dir = opendir(dirPath))) {
        *error = errno;
        return NULL;
    }

    if (NULL != size) {
        struct dirent *dirEntry;

        long dirStart = telldir(dir);

        // Determine the number of entries
        while (NULL != (dirEntry = readdir(dir)))
            if (dirEntry->d_type == DT_REG)
                *size += 1;

        // Recover each block
        seekdir(dir, dirStart);
    }

    return dir;
}

static DIR *
directoryOpen (const char *dirPath, int *error) {
    return directoryOpenWithSize(dirPath, NULL, error);
}

static void
directoryRemoveItem (DIR *dir,
                     const char *dirPath,
                     BREthereumHash hash) {
    BREthereumHashString filename;
    hashFillString (hash, filename);

    char *filePath = directoryPathAppend (dirPath, filename);

    remove (filePath);
    free (filePath);
}

static void
directorySaveItem (DIR *dir,
                   const char *dirPath,
                   BREthereumHash hash,
                   OwnershipGiven BRRlpItem rlpItem,
                   BRRlpCoder coder) {
    BREthereumHashString filename;
    hashFillString(hash, filename);

    char *filePath = directoryPathAppend (dirPath, filename);
    FILE *file = fopen (filePath, "w");

    BRRlpData rlpData = rlpGetDataSharedDontRelease(coder, rlpItem);

    // Write the number of bytes - don't use `size_t`; use fixed-size `uint64_t`
    uint64_t count = rlpData.bytesCount;

    fwrite (&count, sizeof (uint64_t), 1, file);

    // Write the bytes themselves
    fwrite (rlpData.bytes, 1, count, file);

    rlpReleaseItem (coder, rlpItem);
    free (filePath);
    fclose (file);
}

static BRRlpData
directoryReadData (const char *dirPath,
                   const char *fileName) {
    char *filepath = directoryPathAppend (dirPath, fileName);
    FILE *file = fopen (filepath, "r");

    uint64_t count;
    fread (&count, sizeof (uint64_t), 1, file);

    BRRlpData data = { count, malloc (count) };
    fread (data.bytes, 1, count, file);

    free (filepath);
    fclose (file);

    return data;
}

///
/// MARK: - Ensure / Restore
///


static BRSetOf(BREthereumBlock)
createEWMEnsureBlocks (OwnershipGiven BRSetOf(BREthereumHashDataPair) blocksPersistData,
                       BREthereumNetwork network,
                       BREthereumTimestamp timestamp,
                       BRRlpCoder coder) {
    size_t blocksCount = (NULL == blocksPersistData ? 0 : BRSetCount (blocksPersistData));

    BRSetOf(BREthereumBlock) blocks = BRSetNew (blockHashValue,
                                                blockHashEqual,
                                                blocksCount);

    if (0 == blocksCount) {
        const BREthereumBlockCheckpoint *checkpoint = blockCheckpointLookupByTimestamp (network, timestamp);
        BREthereumBlock block = blockCreate (blockCheckpointCreatePartialBlockHeader (checkpoint));
        blockSetTotalDifficulty (block, checkpoint->u.td);
        BRSetAdd (blocks, block);
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

static BRSetOf(BREthereumBlock)
createEWMRestoreBlocks (const char *storagePath,
                        BREthereumNetwork network,
                        BREthereumTimestamp timestamp,
                        BRRlpCoder coder) {
    int error = 0;
    size_t size = 0;
    struct dirent *dirEntry;

    char *dirPath = directoryPathAppend(storagePath, "blocks");
    DIR *dir = directoryOpenWithSize(dirPath, &size, &error);

    if (NULL == dir ) {
        return NULL;
    }

    BRSetOf(BREthereumBlock) blocks = BRSetNew (blockHashValue,
                                                blockHashEqual,
                                                size);

    if (0 == size) {
        const BREthereumBlockCheckpoint *checkpoint = blockCheckpointLookupByTimestamp (network, timestamp);
        BREthereumBlock block = blockCreate (blockCheckpointCreatePartialBlockHeader (checkpoint));
        blockSetTotalDifficulty (block, checkpoint->u.td);
        BRSetAdd (blocks, block);
    }
    else {
        while (NULL != (dirEntry = readdir(dir)))
            if (dirEntry->d_type == DT_REG) {
                BRRlpData data = directoryReadData(dirPath, dirEntry->d_name);
                BRRlpItem item = rlpGetItem(coder, data);

                BREthereumBlock block = blockRlpDecode(item, network, RLP_TYPE_ARCHIVE, coder);
                BRSetAdd(blocks, block);

                rlpReleaseItem (coder, item);
                rlpDataRelease(data);
            }

        free (dirPath);
        closedir (dir);
    }
    return blocks;
}

static BRSetOf(BREthereumNodeConfig)
createEWMEnsureNodes (OwnershipGiven BRSetOf(BREthereumHashDataPair) nodesPersistData,
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

static BRSetOf(BREthereumNodeConfig)
createEWMRestoreNodes (const char *storagePath,
                       BRRlpCoder coder) {
    int error = 0;
    size_t size = 0;
    struct dirent *dirEntry;

    char *dirPath = directoryPathAppend(storagePath, "nodes");
    DIR *dir = directoryOpenWithSize(dirPath, &size, &error);

    if (NULL == dir ) {
        return NULL;
    }

    BRSetOf (BREthereumNodeConfig) nodes = BRSetNew (nodeConfigHashValue,
                                                     nodeConfigHashEqual,
                                                     size);
    while (NULL != (dirEntry = readdir(dir)))
        if (dirEntry->d_type == DT_REG) {
            BRRlpData data = directoryReadData(dirPath, dirEntry->d_name);
            BRRlpItem item = rlpGetItem(coder, data);

            BREthereumNodeConfig node = nodeConfigDecode(item, coder);
            BRSetAdd (nodes, node);

            rlpReleaseItem (coder, item);
            rlpDataRelease(data);
        }

    free (dirPath);
    closedir (dir);

    return nodes;
}


static BRSetOf(BREthereumTransaction)
createEWMEnsureTransactions (OwnershipGiven BRSetOf(BREthereumHashDataPair) transactionsPersistData,
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

static BRSetOf(BREthereumTransaction)
createEWMRestoreTransactions (const char *storagePath,
                              BREthereumNetwork network,
                              BRRlpCoder coder) {
    int error = 0;
    size_t size = 0;
    struct dirent *dirEntry;

    char *dirPath = directoryPathAppend(storagePath, "transactions");
    DIR *dir = directoryOpenWithSize(dirPath, &size, &error);

    if (NULL == dir ) {
        return NULL;
    }

    BRSetOf(BREthereumTransaction) transactions = BRSetNew (transactionHashValue,
                                                            transactionHashEqual,
                                                            size);

    while (NULL != (dirEntry = readdir(dir)))
        if (dirEntry->d_type == DT_REG) {
            BRRlpData data = directoryReadData(dirPath, dirEntry->d_name);
            BRRlpItem item = rlpGetItem(coder, data);

            BREthereumTransaction transaction = transactionRlpDecode (item, network, RLP_TYPE_ARCHIVE, coder);
            BRSetAdd (transactions, transaction);

            rlpReleaseItem (coder, item);
            rlpDataRelease(data);
        }

    free (dirPath);
    closedir (dir);

   return transactions;
}

static BRSetOf(BREthereumLog)
createEWMEnsureLogs (OwnershipGiven BRSetOf(BREthereumHashDataPair) logsPersistData,
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

static BRSetOf(BREthereumLog)
createEWMRestoreLogs (const char *storagePath,
                      BREthereumNetwork network,
                      BRRlpCoder coder) {
    int error = 0;
    size_t size = 0;
    struct dirent *dirEntry;

    char *dirPath = directoryPathAppend(storagePath, "logs");
    DIR *dir = directoryOpenWithSize(dirPath, &size, &error);

    if (NULL == dir ) {
        return NULL;
    }

    BRSetOf(BREthereumLog) logs = BRSetNew (logHashValue,
                                            logHashEqual,
                                            size);
    while (NULL != (dirEntry = readdir(dir)))
        if (dirEntry->d_type == DT_REG) {
            BRRlpData data = directoryReadData(dirPath, dirEntry->d_name);
            BRRlpItem item = rlpGetItem(coder, data);

            BREthereumLog log = logRlpDecode(item, RLP_TYPE_ARCHIVE, coder);
            BRSetAdd (logs, log);

            rlpReleaseItem (coder, item);
            rlpDataRelease(data);
        }

    free (dirPath);
    closedir (dir);

    return logs;
}

///
/// MARK: Ethereum Wallet Manager
///

static BREthereumEWM
ewmCreateInternal (BREthereumNetwork network,
                   BREthereumAccount account,
                   BREthereumTimestamp accountTimestamp,
                   BREthereumMode mode,
                   BREthereumClient client,
                   OwnershipGiven BRSetOf(BREthereumNodeConfig)  nodes,
                   OwnershipGiven BRSetOf(BREthereumBlock)       blocks,
                   OwnershipGiven BRSetOf(BREthereumTransaction) transactions,
                   OwnershipGiven BRSetOf(BREthereumLog)         logs,
                   OwnershipGiven BRRlpCoder coder,
                   const char *storagePath) {
    BREthereumEWM ewm = (BREthereumEWM) calloc (1, sizeof (struct BREthereumEWMRecord));

    ewm->state = LIGHT_NODE_CREATED;
    ewm->mode = mode;
    ewm->network = network;
    ewm->account = account;
    ewm->bcs = NULL;

    // Get the client assigned early; callbacks as EWM/BCS state is re-establish, regarding
    // blocks, peers, transactions and logs, will be invoked.
    ewm->client = client;

    // Our one and only coder
    ewm->coder = coder;

    // The storage path, if it exists
    ewm->storagePath = (NULL == storagePath ? NULL : strdup (storagePath));

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

            // This will hit the BRD Services which will update us with everything after lastBlock.
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

            // Add ewmPeriodicDispatcher to handlerForMain.  We'll only add this if the
            // mode is BRD_ONLY - on all other modes the EWM state will be updated from a) full
            // peer-to-peer processing or b) calling ewmPeriodicDispatcher when a new block is
            // announced.
            if (BRD_ONLY == ewm->mode)
                eventHandlerSetTimeoutDispatcher(ewm->handler,
                                                 1000 * EWM_SLEEP_SECONDS,
                                                 (BREventDispatcher)ewmPeriodicDispatcher,
                                                 (void*) ewm);

            break;
        }

        case P2P_WITH_BRD_SYNC:
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
ewmCreate (BREthereumNetwork network,
           BREthereumAccount account,
           BREthereumTimestamp accountTimestamp,
           BREthereumMode mode,
           BREthereumClient client,
           BRSetOf(BREthereumHashDataPair) nodesPersistData,
           BRSetOf(BREthereumHashDataPair) blocksPersistData,
           BRSetOf(BREthereumHashDataPair) transactionsPersistData,
           BRSetOf(BREthereumHashDataPair) logsPersistData) {
    BRRlpCoder coder = rlpCoderCreate();

    return ewmCreateInternal (network,
                              account,
                              accountTimestamp,
                              mode,
                              client,
                              createEWMEnsureNodes(nodesPersistData, coder),
                              createEWMEnsureBlocks (blocksPersistData, network, accountTimestamp, coder),
                              createEWMEnsureTransactions(transactionsPersistData, network, coder),
                              createEWMEnsureLogs(logsPersistData, network, coder),
                              coder,
                              NULL);
}

extern BREthereumEWM
ewmCreateWithPaperKey (BREthereumNetwork network,
                       const char *paperKey,
                       BREthereumTimestamp accountTimestamp,
                       BREthereumMode mode,
                       BREthereumClient client,
                       BRSetOf(BREthereumHashDataPair) nodesPersistData,
                       BRSetOf(BREthereumHashDataPair) blocksPersistData,
                       BRSetOf(BREthereumHashDataPair) transactionsPersistData,
                       BRSetOf(BREthereumHashDataPair) logsPersistData) {
    return ewmCreate (network,
                      createAccount (paperKey),
                      accountTimestamp,
                      mode,
                      client,
                      nodesPersistData,
                      blocksPersistData,
                      transactionsPersistData,
                      logsPersistData);
}

extern BREthereumEWM
ewmCreateWithPublicKey (BREthereumNetwork network,
                        BRKey publicKey,
                        BREthereumTimestamp accountTimestamp,
                        BREthereumMode mode,
                        BREthereumClient client,
                        BRSetOf(BREthereumHashDataPair) nodesPersistData,
                        BRSetOf(BREthereumHashDataPair) blocksPersistData,
                        BRSetOf(BREthereumHashDataPair) transactionsPersistData,
                        BRSetOf(BREthereumHashDataPair) logsPersistData) {
    return ewmCreate (network,
                      createAccountWithPublicKey(publicKey),
                      accountTimestamp,
                      mode,
                      client,
                      nodesPersistData,
                      blocksPersistData,
                      transactionsPersistData,
                      logsPersistData);
}

extern BREthereumEWM
ewmCreateWithStoragePath (BREthereumNetwork network,
                          BREthereumAccount account,
                          BREthereumTimestamp accountTimestamp,
                          BREthereumMode mode,
                          BREthereumClient client,
                          const char *storagePath) {
    BRRlpCoder coder = rlpCoderCreate();
    const char *fullStoragePath = directoryForNetwork (storagePath, network);
    return ewmCreateInternal (network,
                              account,
                              accountTimestamp,
                              mode,
                              client,
                              createEWMRestoreNodes(fullStoragePath, coder),
                              createEWMRestoreBlocks (fullStoragePath, network, accountTimestamp, coder),
                              createEWMRestoreTransactions(fullStoragePath, network, coder),
                              createEWMRestoreLogs(fullStoragePath, network, coder),
                              coder,
                              fullStoragePath);
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


/**
 * Update the EWM state using the BRD services
 *
 * @param ewm the ewm
 */
static void
ewmUpdateState (BREthereumEWM ewm) {
    if (ewm->state != LIGHT_NODE_CONNECTED) return;

    // We don't use this in P2P modes, so skip out.
    if (P2P_ONLY == ewm->mode || P2P_WITH_BRD_SYNC == ewm->mode) return;

    ewmUpdateBlockNumber(ewm);
    ewmUpdateNonce(ewm);

    // We'll query all transactions for this ewm's account.  That will give us a shot at
    // getting the nonce for the account's address correct.  We'll save all the transactions and
    // then process them into wallet as wallets exist.
    ewmUpdateTransactions(ewm);

    // Similarly, we'll query all logs for this ewm's account.  We'll process these into
    // (token) transactions and associate with their wallet.
    ewmUpdateLogs(ewm, NULL, eventERC20Transfer);

    // For all the known wallets, get their balance.
    for (int i = 0; i < array_count(ewm->wallets); i++)
        ewmUpdateWalletBalance (ewm, ewm->wallets[i]);
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
ewmWalletSignTransfer(BREthereumEWM ewm,
                      BREthereumWallet wallet,
                      BREthereumTransfer transfer,
                      BRKey privateKey) {
    walletSignTransferWithPrivateKey (wallet, transfer, privateKey);
    ewmWalletSignTransferAnnounce (ewm, wallet, transfer);
}

extern void // status, error
ewmWalletSignTransferWithPaperKey(BREthereumEWM ewm,
                                  BREthereumWallet wallet,
                                  BREthereumTransfer transfer,
                                  const char *paperKey) {
    walletSignTransfer (wallet, transfer, paperKey);
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
    // Don't report during BCS sync but always report during an BRD mode.
    if (BRD_ONLY == ewm->mode || BRD_WITH_P2P_SEND == ewm->mode ||
        ETHEREUM_BOOLEAN_IS_FALSE(bcsSyncInProgress (ewm->bcs)))
        eth_log ("EWM", "BlockChain: %" PRIu64 "%s", headBlockNumber,
                 (BRD_WITH_P2P_SEND == ewm->mode ? " via BRD Services" : ""));

    // 
    uint64_t lastBlockNumber = ewm->blockHeight;

    // At least this - allows for: ewmGetBlockHeight
    ewm->blockHeight = headBlockNumber;

    // If we are in BRD_WITH_P2P_SEND mode, then trigger a EWM state update using BRD services.
    if (BRD_WITH_P2P_SEND == ewm->mode)
        ewmUpdateState (ewm);

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
ewmHandleTransactionOriginatingLog (BREthereumEWM ewm,
                                     BREthereumBCSCallbackTransactionType type,
                                    OwnershipKept BREthereumTransaction transaction) {
    BREthereumHash hash = transactionGetHash(transaction);
    for (size_t wid = 0; wid < array_count(ewm->wallets); wid++) {
        BREthereumWallet wallet = ewm->wallets[wid];
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

            if (ETHEREUM_BOOLEAN_IS_TRUE (transferHasStatus (transfer, TRANSFER_STATUS_INCLUDED)))
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
            }
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
    eth_log ("EWM", "Transaction: \"%s\", Change: %s",
             hashString, BCS_CALLBACK_TRANSACTION_TYPE_NAME(type));

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

    if (ETHEREUM_BOOLEAN_IS_TRUE (transferHasStatus (transfer, TRANSFER_STATUS_INCLUDED)))
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
        // free (reason)
    }

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

    BREthereumHashString hashString;
    hashFillString(transactionHash, hashString);
    eth_log ("EWM", "Log: \"%s\" @ %zu, Change: %s",
             hashString, logIndex, BCS_CALLBACK_TRANSACTION_TYPE_NAME(type));

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

    if (ETHEREUM_BOOLEAN_IS_TRUE (transferHasStatus (transfer, TRANSFER_STATUS_INCLUDED)))
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
        // free (reason)
    }
}

extern void
ewmHandleSaveBlocks (BREthereumEWM ewm,
                     OwnershipGiven BRArrayOf(BREthereumBlock) blocks) {
    size_t count = array_count(blocks);
    if (NULL != ewm->storagePath) {
        eth_log("EWM", "Save Blocks (Storage): %zu", count);
        directoryClear(ewm->storagePath, "blocks");

        int error = 0;
        char *dirPath = directoryPathAppend(ewm->storagePath, "blocks");
        DIR *dir = directoryOpen(dirPath, &error);

        for (size_t index = 0; index < count; index++) {
            BREthereumBlock block = blocks[index];
            directorySaveItem (dir, dirPath,
                               blockGetHash(block),
                               blockRlpEncode(block, ewm->network, RLP_TYPE_ARCHIVE, ewm->coder),
                               ewm->coder);
        }
        free (dirPath);
        closedir (dir);
    }
    else {
        eth_log("EWM", "Save Blocks (Client): %zu", array_count(blocks));

        BRSetOf(BREthereumHashDataPair) blocksToSave = hashDataPairSetCreateEmpty (array_count (blocks));

        for (size_t index = 0; index < array_count(blocks); index++) {
            BRRlpItem item = blockRlpEncode(blocks[index], ewm->network, RLP_TYPE_ARCHIVE, ewm->coder);
            BRSetAdd (blocksToSave,
                      hashDataPairCreate (blockGetHash(blocks[index]), // notice '1'; don't relese data
                                          dataCreateFromRlpData (rlpGetData (ewm->coder, item), 1)));
            rlpReleaseItem(ewm->coder, item);
        }

        // TODO: ewmSignalSaveBlocks(ewm, blocks)
        ewm->client.funcSaveBlocks (ewm->client.context,
                                    ewm,
                                    blocksToSave);
    }
    array_free (blocks);
}

extern void
ewmHandleSaveNodes (BREthereumEWM ewm,
                    OwnershipGiven BRArrayOf(BREthereumNodeConfig) nodes) {
    size_t count = array_count(nodes);
    if (NULL != ewm->storagePath) {
        eth_log("EWM", "Save Nodes (Storage): %zu", count);
        directoryClear(ewm->storagePath, "nodes");

        int error = 0;
        char *dirPath = directoryPathAppend(ewm->storagePath, "nodes");
        DIR *dir = directoryOpen(dirPath, &error);

        for (size_t index = 0; index < count; index++) {
            BREthereumNodeConfig config = nodes[index];
            directorySaveItem (dir, dirPath,
                               nodeConfigGetHash(config),
                               nodeConfigEncode(config, ewm->coder),
                               ewm->coder);
        }
        free (dirPath);
        closedir (dir);
    }
    else {
        eth_log("EWM", "Save nodes (client): %zu", count);

        BRSetOf(BREthereumHashDataPair) nodesToSave = hashDataPairSetCreateEmpty (count);

        for (size_t index = 0; index < count; index++) {
            BRRlpItem item = nodeConfigEncode(nodes[index], ewm->coder);

            BRSetAdd (nodesToSave,
                      hashDataPairCreate (nodeConfigGetHash(nodes[index]), // notice '1'; don't relese data
                                          dataCreateFromRlpData (rlpGetData (ewm->coder, item), 1)));

            rlpReleaseItem (ewm->coder, item);

            nodeConfigRelease(nodes[index]);
        }

        // TODO: ewmSignalSavenodes(ewm, nodes);
        ewm->client.funcSaveNodes (ewm->client.context,
                                   ewm,
                                   nodesToSave);

        array_free (nodes);
    }
}

extern void
ewmHandleSaveTransaction (BREthereumEWM ewm,
                          BREthereumTransaction transaction,
                          BREthereumClientChangeType type) {
    BREthereumHash hash = transactionGetHash(transaction);
    BREthereumHashString fileName;
    hashFillString(hash, fileName);

    if (NULL != ewm->storagePath) {
        eth_log("EWM", "Save Transaction (Storage): %s", fileName);

        int error = 0;
        char *dirPath = directoryPathAppend(ewm->storagePath, "transactions");
        DIR *dir = directoryOpen(dirPath, &error);

        if (CLIENT_CHANGE_REM == type || CLIENT_CHANGE_UPD == type)
            directoryRemoveItem (dir, dirPath, hash);

        if (CLIENT_CHANGE_ADD == type || CLIENT_CHANGE_UPD == type)
            directorySaveItem (dir, dirPath, hash,
                               transactionRlpEncode(transaction, ewm->network, RLP_TYPE_ARCHIVE, ewm->coder),
                               ewm->coder);

        free (dirPath);
        closedir(dir);
    }
    else {
        BRRlpItem item = transactionRlpEncode (transaction, ewm->network, RLP_TYPE_ARCHIVE, ewm->coder);

        // Notice the final '1' - don't release `data`...
        BREthereumHashDataPair pair =
        hashDataPairCreate (hash, dataCreateFromRlpData(rlpGetData(ewm->coder, item), 1));

        rlpReleaseItem(ewm->coder, item);

        ewm->client.funcChangeTransaction (ewm->client.context, ewm, type, pair);
    }
}

extern void
ewmHandleSaveLog (BREthereumEWM ewm,
                  BREthereumLog log,
                  BREthereumClientChangeType type) {
    BREthereumHash hash = logGetHash(log);
    BREthereumHashString filename;
    hashFillString(hash, filename);

    if (NULL != ewm->storagePath) {
        eth_log("EWM", "Save Log (Storage): %s", filename);

        int error = 0;
        char *dirPath = directoryPathAppend(ewm->storagePath, "logs");
        DIR *dir = directoryOpen(dirPath, &error);

        if (CLIENT_CHANGE_REM == type || CLIENT_CHANGE_UPD == type)
            directoryRemoveItem (dir, dirPath, hash);

        if (CLIENT_CHANGE_ADD == type || CLIENT_CHANGE_UPD == type)
            directorySaveItem (dir, dirPath, hash,
                               logRlpEncode(log, RLP_TYPE_ARCHIVE, ewm->coder),
                               ewm->coder);

        free (dirPath);
        closedir(dir);
    }
    else {
        BRRlpItem item = logRlpEncode(log, RLP_TYPE_ARCHIVE, ewm->coder);

        // Notice the final '1' - don't release `data`...
        BREthereumHashDataPair pair =
        hashDataPairCreate (hash, dataCreateFromRlpData(rlpGetData(ewm->coder, item), 1));

        rlpReleaseItem(ewm->coder, item);

        ewm->client.funcChangeLog (ewm->client.context, ewm, type, pair);
    }
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
    ewmUpdateState ((BREthereumEWM) event->context);
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
                                (transactionIsSigned(transaction)
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


