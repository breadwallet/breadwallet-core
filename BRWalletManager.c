//
//  BRWalletManager.c
//  BRCore
//
//  Created by Ed Gamble on 11/21/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "BRArray.h"
#include "BRWalletManager.h"
#include "BRPeerManager.h"
#include "BRMerkleBlock.h"
#include "BRBase58.h"
#include "bcash/BRBCashParams.h"

#define BRArrayOf(type)      type*

/* Forward Declarations */

static void _BRWalletManagerBalanceChanged (void *info, uint64_t balanceInSatoshi);
static void _BRWalletManagerTxAdded   (void *info, BRTransaction *tx);
static void _BRWalletManagerTxUpdated (void *info, const UInt256 *hashes, size_t count, uint32_t blockHeight, uint32_t timestamp);
static void _BRWalletManagerTxDeleted (void *info, UInt256 hash, int notifyUser, int recommendRescan);

static void _BRWalletManagerSyncStarted (void *info);
static void _BRWalletManagerSyncStopped (void *info, int reason);
static void _BRWalletManagerTxStatusUpdate (void *info);
static void _BRWalletManagerSaveBlocks (void *info, int replace, BRMerkleBlock **blocks, size_t count);
static void _BRWalletManagerSavePeers  (void *info, int replace, const BRPeer *peers, size_t count);
static int  _BRWalletManagerNetworkIsReachabele (void *info);
static void _BRWalletManagerThreadCleanup (void *info);

static BRArrayOf(BRTransaction*) _BRWalletManagerLoadTransactions (BRWalletManager manager, int *error);
static BRArrayOf(BRMerkleBlock*) _BRWalletManagerLoadBlocks (BRWalletManager manager, int *error);
static BRArrayOf(BRPeer)         _BRWalletManagerLoadPeers  (BRWalletManager manager, int *error);

extern void
decodeHex (uint8_t *target, size_t targetLen, const char *source, size_t sourceLen) {
    //
    assert (0 == sourceLen % 2);
    assert (2 * targetLen == sourceLen);

    for (int i = 0; i < targetLen; i++) {
        target[i] = (uint8_t) ((_hexu(source[2*i]) << 4) | _hexu(source[(2*i)+1]));
    }
}

extern void
encodeHex (char *target, size_t targetLen, const uint8_t *source, size_t sourceLen) {
    assert (targetLen == 2 * sourceLen  + 1);

    for (int i = 0; i < sourceLen; i++) {
        target[2*i] = (uint8_t) _hexc (source[i] >> 4);
        target[2*i + 1] = (uint8_t) _hexc (source[i]);
    }
    target[2*sourceLen] = '\0';
}

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
}

static int directoryMake (const char *path) {
    struct stat dirStat;
    if (0 == stat  (path, &dirStat)) return 0;
    if (0 == mkdir (path, 0700)) return 0;
    return -1;
}

static const char *
directoryOffsetForWalletFork (BRWalletForkId fork) {
    if (fork == WALLET_FORKID_BITCOIN) return "btc";
    if (fork == WALLET_FORKID_BITCASH) return "bch";
    return "unk";
}

static char *
directoryForWalletFork (const char *base, BRWalletForkId fork) {
    return directoryPathAppend(base, directoryOffsetForWalletFork(fork));
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
directoryForWallet (const char *base, BRWalletForkId fork, uint16_t port) {
    const char *net = (8333 == port ? "main" : (18333 == port ? "test" : "none"));

    if (-1 == directoryMake(base)) return NULL;

    char *forkPath = directoryForWalletFork(base, fork);
    if (-1 == directoryMake(forkPath)) { free (forkPath); return NULL; }

    char *path = directoryPathAppend (forkPath, net);
    if (-1 == directoryMake(path)) { free (path); free (forkPath); return NULL; }

    free (forkPath);
    return path;
}

///
/// MARK: BRWalletManager
///
struct BRWalletManagerStruct {
    BRWalletForkId walletForkId;
    BRWallet *wallet;
    BRPeerManager  *peerManager;

    BRWalletManagerClient client;

    char *storagePath;

};

extern BRWalletManager
BRWalletManagerNew (BRWalletManagerClient client,
                    BRWalletForkId fork,
                    BRMasterPubKey mpk,
                    const BRChainParams *params,
                    uint32_t earliestKeyTime,
                    const char *baseStoragePath) {
    BRWalletManager manager = malloc (sizeof (struct BRWalletManagerStruct));
    if (NULL == manager) return NULL;

    manager->walletForkId = fork;
    manager->client = client;

    manager->storagePath = directoryForWallet(baseStoragePath, fork,  params->standardPort);
    if (NULL == manager->storagePath) return NULL;

    DIR *test = opendir(manager->storagePath);
    if (NULL == test) return NULL;
    closedir(test);

    int error = 0;

    BRArrayOf(BRTransaction*) transactions = _BRWalletManagerLoadTransactions(manager, &error);
    if (error) return NULL;

    manager->wallet = BRWalletNew (transactions, array_count(transactions), mpk);
    BRWalletSetCallbacks (manager->wallet, manager,
                          _BRWalletManagerBalanceChanged,
                          _BRWalletManagerTxAdded,
                          _BRWalletManagerTxUpdated,
                          _BRWalletManagerTxDeleted);
    array_free (transactions);

    client.funcWalletEvent (manager,
                            manager->wallet,
                            (BRWalletEvent) {
                                BITCOIN_WALLET_CREATED
                            });

    BRArrayOf(BRMerkleBlock*) blocks = _BRWalletManagerLoadBlocks (manager, &error);
    if (error) return NULL;

    BRArrayOf(BRPeer) peers = _BRWalletManagerLoadPeers (manager, &error);
    if (error) return NULL;

    manager->peerManager = BRPeerManagerNew (params, manager->wallet, earliestKeyTime,
                                             blocks, array_count(blocks),
                                             peers,  array_count(peers));
    BRPeerManagerSetCallbacks (manager->peerManager, manager,
                               _BRWalletManagerSyncStarted,
                               _BRWalletManagerSyncStopped,
                               _BRWalletManagerTxStatusUpdate,
                               _BRWalletManagerSaveBlocks,
                               _BRWalletManagerSavePeers,
                               _BRWalletManagerNetworkIsReachabele,
                               _BRWalletManagerThreadCleanup);

    array_free (blocks); array_free (peers);

    return manager;
}

extern void
BRWalletManagerFree (BRWalletManager manager) {
    free (manager->storagePath);

    BRPeerManagerFree(manager->peerManager);
    BRWalletFree(manager->wallet);
    free (manager);
}

extern BRWallet *
BRWalletManagerGetWallet (BRWalletManager manager) {
    return manager->wallet;
}

extern BRPeerManager *
BRWalletManagerGetPeerManager (BRWalletManager manager) {
    return manager->peerManager;
}


///
/// MARK: Storage
///

///
/// Transactions
///
static BRArrayOf(BRTransaction*)
_BRWalletManagerLoadTransactions (BRWalletManager manager, int *error) {
    assert (NULL != error);

    BRArrayOf(BRTransaction*) transactions = NULL;
    size_t transactionsCount = 0;

    DIR *dir;
    struct dirent *dirEntry;
    char *dirPath = directoryPathAppend(manager->storagePath, "transactions");

    if (-1 == directoryMake(dirPath) || NULL == (dir = opendir(dirPath))) {
        *error = errno;
        return NULL;
    }

    long dirStart = telldir(dir);

    // Determine the number of transactions
    while (NULL != (dirEntry = readdir(dir)))
        if (dirEntry->d_type == DT_REG)
            transactionsCount += 1;

    array_new (transactions, transactionsCount);

    // Allocate some storage for transaction data
    size_t bufferSize = 8 * 1024;
    uint8_t *buffer = malloc (bufferSize);

    // Recover each transaction
    seekdir(dir, dirStart);
    while (NULL != (dirEntry = readdir(dir)))
        if (dirEntry->d_type == DT_REG) {
            char *filepath = directoryPathAppend (dirPath, dirEntry->d_name);
            FILE *file = fopen (filepath, "r");

            // Read the serialized transaction size
            uint32_t count;
            fread (&count, sizeof (uint32_t), 1, file);

            // Ensure `buffer` is large enough
            if (count > bufferSize) {
                bufferSize = count;
                buffer = realloc (buffer, bufferSize);
            }

            // Read the serialized data and parse into a transaction
            fread (buffer, 1, count, file);
            BRTransaction *transaction = BRTransactionParse(buffer, count);
            if (NULL == transaction) {
            }

            // Read the block height
            fread (&transaction->blockHeight, sizeof (transaction->blockHeight), 1, file);

            // Read the timestamp
            fread(&transaction->timestamp, sizeof (transaction->timestamp), 1, file);

            fclose(file);
            free (filepath);

            // Confirm the file name as the hash of the data...

            // ... if confirmed, add to transactions
            array_add (transactions, transaction);
        }

    closedir(dir);
    free (dirPath);

    return transactions;
}

static void
_BRWalletManagerSaveTransaction (BRWalletManager manager,
                                 BRTransaction *transaction,
                                 uint32_t blockHeight,
                                 uint32_t timestamp) {
    uint32_t count = (uint32_t) BRTransactionSerialize(transaction, NULL, 0);
    uint8_t buffer[count];

    BRTransactionSerialize (transaction, buffer, count);

    char *filePath = filePathFromHash(manager->storagePath, "transactions", &transaction->txHash);
    FILE *file = fopen (filePath, "w");

    // Write: the serialized count;
    if (sizeof(uint32_t) != fwrite (&count, sizeof(uint32_t), 1, file)) {
    }

    // ... the serialized buffer
    if (count != fwrite (buffer, 1, count, file)) {
    }

    // ... the block height
    if (sizeof(uint32_t) != fwrite (&blockHeight, sizeof(uint32_t), 1, file)) {

    }

    // ... the timestamp
    if (sizeof(uint32_t) != fwrite(&timestamp, sizeof(uint32_t), 1, file)) {

    }

    fclose (file);
    free (filePath);
}

static void
_BRWalletManagerRemoveTransaction (BRWalletManager manager,
                                   UInt256 hash) {
    char *filePath = filePathFromHash(manager->storagePath, "transactions", &hash);
    remove (filePath);
    free (filePath);
}

///
/// Blocks
///
static BRArrayOf(BRMerkleBlock*)
_BRWalletManagerLoadBlocks (BRWalletManager manager, int *error) {
    assert (NULL != error);

    BRArrayOf(BRMerkleBlock*) blocks = NULL;
    size_t blocksCount = 0;

    DIR *dir;
    struct dirent *dirEntry;
    char *dirPath = directoryPathAppend(manager->storagePath, "blocks");

    if (-1 == directoryMake(dirPath) || NULL == (dir = opendir(dirPath))) {
        *error = errno;
        return NULL;
    }
    long dirStart = telldir(dir);

    // Determine the number of blocks
    while (NULL != (dirEntry = readdir(dir)))
        if (dirEntry->d_type == DT_REG)
            blocksCount += 1;

    array_new (blocks, blocksCount);

    // Allocate some storage for block data
    size_t bufferSize = 8 * 1024;
    uint8_t *buffer = malloc (bufferSize);

    // Recover each block
    seekdir(dir, dirStart);
    while (NULL != (dirEntry = readdir(dir)))
        if (dirEntry->d_type == DT_REG) {
            char *filepath = directoryPathAppend (dirPath, dirEntry->d_name);
            FILE *file = fopen (filepath, "r");

            // Read the serialized block size
            uint32_t count;
            fread (&count, sizeof (uint32_t), 1, file);

            // Ensure `buffer` is large enough
            if (count > bufferSize) {
                bufferSize = count;
                buffer = realloc (buffer, bufferSize);
            }

            // Read the serialized data and parse into a block
            fread (buffer, 1, count, file);
            BRMerkleBlock *block = BRMerkleBlockParse (buffer, count);
            if (NULL == block) {
            }

            // Read the block height
            fread (&block->height, sizeof (block->height), 1, file);

            fclose (file);
            free (filepath);

            // Confirm the file name as the hash of the data...

            // ... if confirmed, add to blocks
            array_add (blocks, block);
        }

    free (dirPath);
    closedir(dir);

    return blocks;
}


static void
_BRWalletManagerSaveBlock (BRWalletManager manager,
                           BRMerkleBlock *block) {
    uint32_t count = (uint32_t) BRMerkleBlockSerialize (block, NULL, 0);
    uint8_t buffer[count];

    BRMerkleBlockSerialize (block, buffer, count);

    char *filePath = filePathFromHash(manager->storagePath, "blocks", &block->blockHash);
    FILE *file = fopen (filePath, "w");

    // Write: the serialized count;
    if (sizeof(uint32_t) != fwrite (&count, sizeof(uint32_t), 1, file)) {
    }

    // ... the serialized buffer
    if (count != fwrite (buffer, 1, count, file)) {
    }

    // ... the block height
    if (sizeof(block->height) != fwrite (&block->height, sizeof(block->height), 1, file )) {
    }

    fclose (file);
    free (filePath);
}

static void
_BRWalletManagerReplaceBlocks (BRWalletManager manager) {
    directoryClear(manager->storagePath, "blocks");
}

///
/// Peers
///

static BRArrayOf(BRPeer)
_BRWalletManagerLoadPeers  (BRWalletManager manager, int *error) {
    assert (NULL != error);

    BRArrayOf(BRPeer) peers = NULL;
    size_t peersCount = 0;

    DIR *dir;
    struct dirent *dirEntry;
    char *dirPath = directoryPathAppend(manager->storagePath, "peers");

    if (-1 == directoryMake(dirPath) || NULL == (dir = opendir(dirPath))) {
        *error = errno;
        return NULL;
    }
    long dirStart = telldir(dir);

    // Determine the number of blocks
    while (NULL != (dirEntry = readdir(dir)))
        if (dirEntry->d_type == DT_REG)
            peersCount += 1;

    array_new (peers, peersCount);

    // Recover each peer
    seekdir(dir, dirStart);
    while (NULL != (dirEntry = readdir(dir)))
        if (dirEntry->d_type == DT_REG) {
            char *filepath = directoryPathAppend (dirPath, dirEntry->d_name);
            FILE *file = fopen (filepath, "r");

            // Read the peer
            BRPeer peer;
            fread (&peer, sizeof (BRPeer), 1, file);

            fclose (file);
            free (filepath);

            // Confirm the file name as the hash of the data...

            // ... if confirmed, add to blocks
            array_add (peers, peer);
        }

    closedir(dir);
    free (dirPath);

    return peers;
}

static void
_BRWalletManagerSavePeer (BRWalletManager manager,
                          BRPeer peer) {
    UInt256 hash;
    BRSHA256 (&hash, &peer, sizeof(BRPeer));

    char *filePath = filePathFromHash(manager->storagePath, "peers", &hash);
    FILE *file = fopen (filePath, "w");

    // Write: the serialized count;
    if (sizeof(BRPeer) != fwrite (&peer, sizeof(BRPeer), 1, file)) {
    }

    free (filePath);
    fclose (file);
}

static void
_BRWalletManagerReplacePeers (BRWalletManager manager) {
    directoryClear(manager->storagePath, "peers");
}

///
/// MARK: Wallet Callbacks
///

static void
_BRWalletManagerBalanceChanged (void *info, uint64_t balanceInSatoshi) {
    BRWalletManager manager = (BRWalletManager) info;
    manager->client.funcWalletEvent (manager,
                                     manager->wallet,
                                     (BRWalletEvent) {
                                         BITCOIN_WALLET_BALANCE_UPDATED,
                                         { .balance = { balanceInSatoshi }}
                                     });
}

static void
_BRWalletManagerTxAdded   (void *info, BRTransaction *tx) {
    BRWalletManager manager = (BRWalletManager) info;
    _BRWalletManagerSaveTransaction (manager, tx, tx->blockHeight, tx->timestamp);
    manager->client.funcTransactionEvent (manager,
                                          manager->wallet,
                                          tx,
                                          (BRTransactionEvent) {
                                              BITCOIN_TRANSACTION_ADDED
                                          });
}

static void
_BRWalletManagerTxUpdated (void *info, const UInt256 *hashes, size_t count, uint32_t blockHeight, uint32_t timestamp) {
    BRWalletManager manager = (BRWalletManager) info;

    for (size_t index = 0; index < count; index++) {
        UInt256 hash = hashes[index];
        BRTransaction *transaction = BRWalletTransactionForHash(manager->wallet, hash);
        _BRWalletManagerSaveTransaction (manager, transaction, blockHeight, timestamp);
        manager->client.funcTransactionEvent (manager,
                                              manager->wallet,
                                              transaction,
                                              (BRTransactionEvent) {
                                                  BITCOIN_TRANSACTION_UPDATED,
                                                  { .updated = { blockHeight, timestamp }}
                                              });
    }
}

static void
_BRWalletManagerTxDeleted (void *info, UInt256 hash, int notifyUser, int recommendRescan) {
    BRWalletManager manager = (BRWalletManager) info;
    _BRWalletManagerRemoveTransaction (manager, hash);
    BRTransaction *transaction = BRWalletTransactionForHash(manager->wallet, hash);
    manager->client.funcTransactionEvent (manager,
                                          manager->wallet,
                                          transaction,
                                          (BRTransactionEvent) {
                                              BITCOIN_TRANSACTION_DELETED
                                          });
}

///
/// MARK: Peer Manager Callbacks
///
static void
_BRWalletManagerSyncStarted (void *info) {
    BRWalletManager manager = (BRWalletManager) info;
    manager->client.funcWalletManagerEvent (manager,
                                            (BRWalletManagerEvent) {
                                                BITCOIN_WALLET_MANAGER_SYNC_STARTED
                                            });
}

static void
_BRWalletManagerSyncStopped (void *info, int reason) {
    BRWalletManager manager = (BRWalletManager) info;
    manager->client.funcWalletManagerEvent (manager,
                                            (BRWalletManagerEvent) {
                                                BITCOIN_WALLET_MANAGER_SYNC_STOPPED,
                                                { .syncStopped = { reason }}
                                            });
}

static void
_BRWalletManagerTxStatusUpdate (void *info) {
//    BRWalletManager manager = (BRWalletManager) info;

    // event

}

static void
_BRWalletManagerSaveBlocks (void *info, int replace, BRMerkleBlock **blocks, size_t count) {
    BRWalletManager manager = (BRWalletManager) info;

    if (replace) _BRWalletManagerReplaceBlocks(manager);
    for (size_t index = 0; index < count; index++)
        _BRWalletManagerSaveBlock (manager, blocks[index]);
}

static void
_BRWalletManagerSavePeers  (void *info, int replace, const BRPeer *peers, size_t count) {
    BRWalletManager manager = (BRWalletManager) info;

    if (replace) _BRWalletManagerReplacePeers(manager);
    for (size_t index = 0; index < count; index++)
        _BRWalletManagerSavePeer (manager, peers[index]);
}

static int
_BRWalletManagerNetworkIsReachabele (void *info) {
//    BRWalletManager manager = (BRWalletManager) info;

    // event
   return 1;
}

static void
_BRWalletManagerThreadCleanup (void *info) {
//    BRWalletManager manager = (BRWalletManager) info;

    // event
}
