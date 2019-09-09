//
//  BRWalletManager.c
//  BRCore
//
//  Created by Ed Gamble on 11/21/18.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "BRArray.h"
#include "BRBase.h"
#include "BRSet.h"
#include "BRWalletManager.h"
#include "BRWalletManagerPrivate.h"
#include "BRPeerManager.h"
#include "BRMerkleBlock.h"
#include "BRBase58.h"
#include "BRChainParams.h"
#include "bcash/BRBCashParams.h"

#include "support/BRFileService.h"
#include "ethereum/event/BREvent.h"
#include "ethereum/event/BREventAlarm.h"

#define BWM_SLEEP_SECONDS                        (1)
#define BWM_SYNC_AFTER_WAKEUPS                   (60)

#define DEFAULT_TRANSACTION_CAPACITY     20

/* Forward Declarations */
static void
bwmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);

static void _BRWalletManagerGetBlockNumber(void * context, BRSyncManager manager, int rid);
static void _BRWalletManagerGetTransactions(void * context, BRSyncManager manager, const char **addresses, size_t addressCount, uint64_t begBlockNumber, uint64_t endBlockNumber, int rid);
static void _BRWalletManagerSubmitTransaction(void * context, BRSyncManager manager, uint8_t *tx, size_t txLength, UInt256 txHash, int rid);
static void _BRWalletManagerSyncEvent(void * context, BRSyncManager manager, BRSyncManagerEvent event);

static void _BRWalletManagerBalanceChanged (void *info, uint64_t balanceInSatoshi);
static void _BRWalletManagerTxAdded   (void *info, BRTransaction *tx);
static void _BRWalletManagerTxUpdated (void *info, const UInt256 *hashes, size_t count, uint32_t blockHeight, uint32_t timestamp);
static void _BRWalletManagerTxDeleted (void *info, UInt256 hash, int notifyUser, int recommendRescan);

static const char *
getNetworkName (const BRChainParams *params) {
    if (params->magicNumber == BRMainNetParams->magicNumber ||
        params->magicNumber == BRBCashParams->magicNumber)
        return "mainnet";

    if (params->magicNumber == BRTestNetParams->magicNumber ||
        params->magicNumber == BRBCashTestNetParams->magicNumber)
        return "testnet";

    return NULL;
}

static const char *
getCurrencyName (const BRChainParams *params) {
    if (params->magicNumber == BRMainNetParams->magicNumber ||
        params->magicNumber == BRTestNetParams->magicNumber)
        return "btc";

    if (params->magicNumber == BRBCashParams->magicNumber ||
        params->magicNumber == BRBCashTestNetParams->magicNumber)
        return "bch";

    return NULL;
}

//
// Mark: Wallet Sweeper
//

struct BRWalletSweeperStruct {
    BRAddressParams addrParams;
    uint8_t isSegwit;
    char * sourceAddress;
    BRArrayOf(BRTransaction *) txns;
};

extern BRWalletSweeperStatus
BRWalletSweeperValidateSupported (BRKey *key,
                                  BRAddressParams addrParams,
                                  BRWallet *wallet) {
    // encode using legacy format (only supported method for BTC)
    size_t addrLength = BRKeyLegacyAddr (key, NULL, 0, addrParams);
    char  *addr = malloc (addrLength + 1);
    BRKeyLegacyAddr (key, addr, addrLength, addrParams);
    addr[addrLength] = '\0';

    // check if we are trying to sweep ourselves
    int containsAddr = BRWalletContainsAddress (wallet, addr);
    free (addr);

    if (containsAddr) {
        return WALLET_SWEEPER_INVALID_SOURCE_WALLET;
    }

    return WALLET_SWEEPER_SUCCESS;
}

extern BRWalletSweeper // NULL on error
BRWalletSweeperNew (BRKey *key,
                    BRAddressParams addrParams,
                    uint8_t isSegwit) {
    size_t addressLength = BRKeyLegacyAddr (key, NULL, 0, addrParams);
    char  *address = malloc (addressLength + 1);
    BRKeyLegacyAddr (key, address, addressLength, addrParams);
    address[addressLength] = '\0';

    BRWalletSweeper sweeper = calloc (1, sizeof(struct BRWalletSweeperStruct));
    sweeper->addrParams = addrParams;
    sweeper->isSegwit = isSegwit;
    sweeper->sourceAddress = address;
    array_new (sweeper->txns, 100);
    return sweeper;
}

extern void
BRWalletSweeperFree (BRWalletSweeper sweeper) {
    free (sweeper->sourceAddress);
    for (size_t index = 0; index < array_count(sweeper->txns); index++) {
        BRTransactionFree (sweeper->txns[index]);
    }
    array_free (sweeper->txns);
    memset (sweeper, 0, sizeof(struct BRWalletSweeperStruct));
    free (sweeper);
}

typedef struct {
    UInt256 txHash;
    uint32_t utxoIndex;
    uint8_t *script;
    size_t scriptLen;
    uint64_t amount;
} BRWalletSweeperUTXO;

inline static size_t BRWalletSweeperUTXOHash(const void *utxo)
{
    // (hash xor n)*FNV_PRIME, lifted from BRWallet's BRUTXOHash
    return (size_t)((((const BRWalletSweeperUTXO *)utxo)->txHash.u32[0] ^ ((const BRWalletSweeperUTXO *)utxo)->utxoIndex)*0x01000193);
}

inline static int BRWalletSweeperUTXOEq(const void *utxo, const void *otherUtxo)
{
    // lifted from BRWallet's BRUTXOEq
    return (utxo == otherUtxo || (UInt256Eq(((const BRWalletSweeperUTXO *)utxo)->txHash, ((const BRWalletSweeperUTXO *)otherUtxo)->txHash) &&
                                  ((const BRWalletSweeperUTXO *)utxo)->utxoIndex == ((const BRWalletSweeperUTXO *)otherUtxo)->utxoIndex));
}

inline static uint64_t BRWalletSweeperCalculateFee(uint64_t feePerKb, size_t size)
{
    // lifted from BRWallet's _txFee
    uint64_t standardFee = size*TX_FEE_PER_KB/1000,       // standard fee based on tx size
             fee = (((size*feePerKb/1000) + 99)/100)*100; // fee using feePerKb, rounded up to nearest 100 satoshi

    return (fee > standardFee) ? fee : standardFee;
}

inline static uint64_t BRWalletSweeperCalculateMinOutputAmount(uint64_t feePerKb)
{
    // lifted from BRWallet's BRWalletMinOutputAmount
    uint64_t amount = (TX_MIN_OUTPUT_AMOUNT*feePerKb + MIN_FEE_PER_KB - 1)/MIN_FEE_PER_KB;
    return (amount > TX_MIN_OUTPUT_AMOUNT) ? amount : TX_MIN_OUTPUT_AMOUNT;
}

inline static int BRWalletSweeperIsSourceInput(BRTxInput *input, BRAddressParams addrParams, char * sourceAddress) {
    size_t addressLength = BRTxInputAddress (input, NULL, 0, addrParams);
    char * address = malloc (addressLength + 1);
    BRTxInputAddress (input, address, addressLength, addrParams);
    address[addressLength] = '\0';

    int match = 0 == strcmp (sourceAddress, address);

    free (address);
    return match;
}

inline static int BRWalletSweeperIsSourceOutput(BRTxOutput *output, BRAddressParams addrParams, char * sourceAddress) {
    size_t addressLength = BRTxOutputAddress (output, NULL, 0, addrParams);
    char * address = malloc (addressLength + 1);
    BRTxOutputAddress (output, address, addressLength, addrParams);
    address[addressLength] = '\0';

    int match = 0 == strcmp (sourceAddress, address);

    free (address);
    return match;
}

static BRSetOf(BRWalletSweeperUTXO *)
BRWalletSweeperGetUTXOs (BRWalletSweeper sweeper) {
    BRSet * utxos = BRSetNew(BRWalletSweeperUTXOHash, BRWalletSweeperUTXOEq, 100);

    // TODO(fix): This is horrible; we should be building up this knowledge as transactions are added

    // loop through and add all the unspent outputs
    for (size_t index = 0; index < array_count (sweeper->txns); index++) {
        BRTransaction *txn = sweeper->txns[index];

        for (uint32_t i = 0; i < txn->outCount; i++) {
            if (BRWalletSweeperIsSourceOutput (&txn->outputs[i], sweeper->addrParams, sweeper->sourceAddress)) {
                BRWalletSweeperUTXO * utxo = malloc (sizeof(BRWalletSweeperUTXO));
                utxo->txHash = txn->txHash;
                utxo->utxoIndex = i;
                utxo->amount = txn->outputs[i].amount;
                utxo->script = txn->outputs[i].script;
                utxo->scriptLen = txn->outputs[i].scriptLen;

                utxo = BRSetAdd (utxos, utxo);
                if (NULL != utxo) {
                    free (utxo);
                }
            }
        }
    }

    // loop through and remove all the unspent outputs
    for (size_t index = 0; index < array_count (sweeper->txns); index++) {
        BRTransaction *txn = sweeper->txns[index];

        for (uint32_t i = 0; i < txn->inCount; i++) {
            if (BRWalletSweeperIsSourceInput (&txn->inputs[i], sweeper->addrParams, sweeper->sourceAddress)) {
                BRWalletSweeperUTXO value = {0};
                BRWalletSweeperUTXO * utxo = &value;
                value.txHash = txn->inputs[i].txHash;
                value.utxoIndex = txn->inputs[i].index;
                // other values are not used during lookup

                utxo = BRSetRemove (utxos, utxo);
                if (NULL != utxo) {
                    free (utxo);
                }
            }
        }
    }

    return utxos;
}

static BRWalletSweeperStatus
BRWalletSweeperBuildTransaction (BRWalletSweeper sweeper,
                                 BRWallet * wallet,
                                 uint64_t feePerKb,
                                 BRTransaction **transactionOut,
                                 uint64_t *feeAmountOut,
                                 uint64_t *balanceAmountOut) {
    uint64_t balanceAmount = 0;
    BRTransaction *transaction = BRTransactionNew ();

    // based on BRWallet's BRWalletCreateTxForOutputs

    BRSetOf(BRWalletSweeperUTXO *) outputs = BRWalletSweeperGetUTXOs (sweeper);
    FOR_SET (BRWalletSweeperUTXO *, utxo, outputs) {
        BRTransactionAddInput(transaction,
                              utxo->txHash,
                              utxo->utxoIndex,
                              utxo->amount,
                              utxo->script,
                              utxo->scriptLen,
                              NULL,
                              0,
                              NULL,
                              0,
                              TXIN_SEQUENCE);
        balanceAmount += utxo->amount;
    }
    BRSetFreeAll (outputs, free);

    size_t txnSize = BRTransactionVSize(transaction) + TX_OUTPUT_SIZE;
    if (txnSize > TX_MAX_SIZE) {
        BRTransactionFree (transaction);
        if (transactionOut) *transactionOut = NULL;
        if (feeAmountOut) *feeAmountOut = 0;
        if (balanceAmountOut) *balanceAmountOut = 0;
        return WALLET_SWEEPER_UNABLE_TO_SWEEP;
    }

    if (0 == balanceAmount) {
        BRTransactionFree (transaction);
        if (transactionOut) *transactionOut = NULL;
        if (feeAmountOut) *feeAmountOut = 0;
        if (balanceAmountOut) *balanceAmountOut = 0;
        return WALLET_SWEEPER_INSUFFICIENT_FUNDS;
    }

    uint64_t feeAmount = BRWalletSweeperCalculateFee(feePerKb, txnSize);
    uint64_t minAmount = BRWalletSweeperCalculateMinOutputAmount(feePerKb);
    if ((feeAmount + minAmount) > balanceAmount) {
        BRTransactionFree (transaction);
        if (transactionOut) *transactionOut = NULL;
        if (feeAmountOut) *feeAmountOut = 0;
        if (balanceAmountOut) *balanceAmountOut = 0;
        return WALLET_SWEEPER_INSUFFICIENT_FUNDS;
    }

    BRAddress addr = sweeper->isSegwit ? BRWalletReceiveAddress(wallet) : BRWalletLegacyAddress (wallet);
    BRTxOutput o = BR_TX_OUTPUT_NONE;
    BRTxOutputSetAddress(&o, sweeper->addrParams, addr.s);
    BRTransactionAddOutput (transaction, balanceAmount - feeAmount, o.script, o.scriptLen);

    if (transactionOut) {
        *transactionOut = transaction;
    } else {
        BRTransactionFree (transaction);
    }

    if (feeAmountOut) {
        *feeAmountOut = feeAmount;
    }

    if (balanceAmountOut) {
        *balanceAmountOut = balanceAmount;
    }

    return WALLET_SWEEPER_SUCCESS;
}

static BRWalletSweeperStatus
BRWalletSweeperCreateTransaction (BRWalletSweeper sweeper,
                                  BRWallet * wallet,
                                  uint64_t feePerKb,
                                  BRTransaction **transaction) {
    return BRWalletSweeperBuildTransaction (sweeper, wallet, feePerKb, transaction, NULL, NULL);
}

static BRWalletSweeperStatus
BRWalletSweeperEstimateFee (BRWalletSweeper sweeper,
                            BRWallet * wallet,
                            uint64_t feePerKb,
                            uint64_t *feeEstimate) {
    return BRWalletSweeperBuildTransaction (sweeper, wallet, feePerKb, NULL, feeEstimate, NULL);
}

extern BRWalletSweeperStatus
BRWalletSweeperHandleTransaction (BRWalletSweeper sweeper,
                                  OwnershipKept uint8_t *transaction,
                                  size_t transactionLen) {
    BRWalletSweeperStatus status = WALLET_SWEEPER_SUCCESS;

    BRTransaction * txn = BRTransactionParse (transaction, transactionLen);
    if (NULL != txn) {
        array_add (sweeper->txns, txn);
    } else {
        status = WALLET_SWEEPER_INVALID_TRANSACTION;
    }

    return status;
}

extern char *
BRWalletSweeperGetLegacyAddress (BRWalletSweeper sweeper) {
    return strdup (sweeper->sourceAddress);
}

extern uint64_t
BRWalletSweeperGetBalance (BRWalletSweeper sweeper) {
    uint64_t balance = 0;

    BRSetOf(BRWalletSweeperUTXO *) outputs = BRWalletSweeperGetUTXOs (sweeper);
    FOR_SET (BRWalletSweeperUTXO *, utxo, outputs) {
        balance += utxo->amount;
    }
    BRSetFreeAll (outputs, free);

    return balance;
}

extern BRWalletSweeperStatus
BRWalletSweeperValidate (BRWalletSweeper sweeper) {
    if (0 == array_count (sweeper->txns)) {
        return WALLET_SWEEPER_NO_TRANSACTIONS_FOUND;
    }

    if (0 == BRWalletSweeperGetBalance (sweeper)) {
        return WALLET_SWEEPER_INSUFFICIENT_FUNDS;
    }

    return WALLET_SWEEPER_SUCCESS;
}

/// MARK: - Transaction Tracking

/**
 * The BRWallet has ownership over its set of transactions. As a result, it is not safe
 * for the BRWalletManager to access a transaction owned by its wrapped BRWallet, as that
 * transaction may be deleted at any time by the wallet.
 *
 * To work around this limitation, the BRWalletManager maintains a list of all transactions,
 * including those that have been deleted. This is done using the
 * `BRTransactionWithStateStruct` data structure. It contains an pointer to an owned
 * copy of the transaction (`ownedTransaction`), as well as a pointer to the referenced
 * transaction owned by the BRWallet (`refedTransaction`).
 *
 * The key to this design is that the `refedTransaction` is NEVER, EVER dereferenced. It is
 * used solely for lookups based on identify in callbacks from the BRWallet and
 * BRPeerManager (via the BRSyncManager).
 */

struct BRTransactionWithStateStruct {
    uint8_t isDeleted;
    BRTransaction *refedTransaction;
    BRTransaction *ownedTransaction;
};

static BRTransactionWithState BRTransactionWithStateNew(BRTransaction *ownedTransaction,
                                                        BRTransaction *refedTransaction) {
    BRTransactionWithState txnWithState = calloc (1, sizeof(struct BRTransactionWithStateStruct));
    txnWithState->isDeleted = 0;
    txnWithState->ownedTransaction = ownedTransaction;
    txnWithState->refedTransaction = refedTransaction;
    return txnWithState;
}

static BRTransaction * BRTransactionWithStateGetOwned(BRTransactionWithState txnWithState) {
    return txnWithState->ownedTransaction;
}

static BRTransactionWithState BRTransactionWithStateSetReferenced(BRTransactionWithState txnWithState,
                                                                  BRTransaction *transaction) {
    assert (txnWithState->refedTransaction == NULL);
    txnWithState->refedTransaction = transaction;
    return txnWithState;
}

static BRTransactionWithState BRTransactionWithStateSetBlock(BRTransactionWithState txnWithState,
                                                             uint32_t height,
                                                             uint32_t timestamp) {
    txnWithState->ownedTransaction->blockHeight = height;
    txnWithState->ownedTransaction->timestamp = timestamp;
    return txnWithState;
}

static BRTransactionWithState BRTransactionWithStateSetDeleted(BRTransactionWithState txnWithState) {
    txnWithState->isDeleted = 1;
    return txnWithState;
}

static void BRTransactionWithStateFree(BRTransactionWithState txn) {
    BRTransactionFree (txn->ownedTransaction);
    free (txn);
}

static BRTransactionWithState
BRWalletManagerAddTransaction(BRWalletManager manager,
                              BRTransaction *ownedTransaction,
                              BRTransaction *refedTransaction) {
    BRTransactionWithState txnWithState = BRTransactionWithStateNew (ownedTransaction, refedTransaction);
    array_add (manager->transactions, txnWithState);
    return txnWithState;
}

/**
 * Find the tracked transaction using the `owned` transaction pointer. Deleted transactions
 * are not checked (i.e. they are skipped).
 */
static BRTransactionWithState
BRWalletManagerFindTransactionByOwned (BRWalletManager manager,
                                       BRTransaction *transaction) {
    BRTransactionWithState txnWithState = NULL;

    for (size_t index = 0; index < array_count(manager->transactions); index++) {
        if (!manager->transactions[index]->isDeleted &&
            manager->transactions[index]->ownedTransaction == transaction) {
            txnWithState = manager->transactions[index];
            break;
        }
    }

    return txnWithState;
}

/**
 * Find the tracked transaction using the `owned` transaction's hash. Deleted transactions
 * are not checked (i.e. they are skipped).
 */
static BRTransactionWithState
BRWalletManagerFindTransactionByHash (BRWalletManager manager,
                                      UInt256 hash) {
    BRTransactionWithState txnWithState = NULL;

    for (size_t index = 0; index < array_count(manager->transactions); index++) {
        if (!manager->transactions[index]->isDeleted &&
            UInt256Eq (manager->transactions[index]->ownedTransaction->txHash, hash)) {
            txnWithState = manager->transactions[index];
            break;
        }
    }

    return txnWithState;
}

static void
BRWalletManagerFreeTransactions(BRWalletManager manager) {
    for (size_t index = 0; index < array_count(manager->transactions); index++) {
        BRTransactionWithStateFree (manager->transactions[index]);
    }
    array_free(manager->transactions);
}

/// MARK: - Transaction File Service

#define fileServiceTypeTransactions     "transactions"

enum {
    WALLET_MANAGER_TRANSACTION_VERSION_1
};

static UInt256
fileServiceTypeTransactionV1Identifier (BRFileServiceContext context,
                                        BRFileService fs,
                                        const void *entity) {
    const BRTransaction *transaction = entity;
    return transaction->txHash;
}

static uint8_t *
fileServiceTypeTransactionV1Writer (BRFileServiceContext context,
                                    BRFileService fs,
                                    const void* entity,
                                    uint32_t *bytesCount) {
    const BRTransaction *transaction = entity;

    size_t txTimestampSize  = sizeof (uint32_t);
    size_t txBlockHeightSize = sizeof (uint32_t);
    size_t txSize = BRTransactionSerialize (transaction, NULL, 0);

    assert (txTimestampSize   == sizeof(transaction->timestamp));
    assert (txBlockHeightSize == sizeof(transaction->blockHeight));

    *bytesCount = (uint32_t) (txSize + txBlockHeightSize + txTimestampSize);

    uint8_t *bytes = calloc (*bytesCount, 1);

    size_t bytesOffset = 0;

    BRTransactionSerialize (transaction, &bytes[bytesOffset], txSize);
    bytesOffset += txSize;

    UInt32SetLE (&bytes[bytesOffset], transaction->blockHeight);
    bytesOffset += txBlockHeightSize;

    UInt32SetLE(&bytes[bytesOffset], transaction->timestamp);

    return bytes;
}

static void *
fileServiceTypeTransactionV1Reader (BRFileServiceContext context,
                                    BRFileService fs,
                                    uint8_t *bytes,
                                    uint32_t bytesCount) {
    size_t txTimestampSize  = sizeof (uint32_t);
    size_t txBlockHeightSize = sizeof (uint32_t);

    BRTransaction *transaction = BRTransactionParse (bytes, bytesCount);
    if (NULL == transaction) return NULL;

    transaction->blockHeight = UInt32GetLE (&bytes[bytesCount - txTimestampSize - txBlockHeightSize]);
    transaction->timestamp   = UInt32GetLE (&bytes[bytesCount - txTimestampSize]);

    return transaction;
}

static BRArrayOf(BRTransaction*)
initialTransactionsLoad (BRWalletManager manager) {
    BRSetOf(BRTransaction*) transactionSet = BRSetNew(BRTransactionHash, BRTransactionEq, 100);
    if (1 != fileServiceLoad (manager->fileService, transactionSet, fileServiceTypeTransactions, 1)) {
        BRSetFree(transactionSet);
        return NULL;
    }

    size_t transactionsCount = BRSetCount(transactionSet);

    BRArrayOf(BRTransaction*) transactions;
    array_new (transactions, transactionsCount);
    array_set_count(transactions, transactionsCount);

    BRSetAll(transactionSet, (void**) transactions, transactionsCount);
    BRSetFree(transactionSet);

    return transactions;
}

/// MARK: - Block File Service

#define fileServiceTypeBlocks       "blocks"
enum {
    WALLET_MANAGER_BLOCK_VERSION_1
};

static UInt256
fileServiceTypeBlockV1Identifier (BRFileServiceContext context,
                                  BRFileService fs,
                                  const void *entity) {
    const BRMerkleBlock *block = (BRMerkleBlock*) entity;
    return block->blockHash;
}

static uint8_t *
fileServiceTypeBlockV1Writer (BRFileServiceContext context,
                              BRFileService fs,
                              const void* entity,
                              uint32_t *bytesCount) {
    const BRMerkleBlock *block = entity;

    // The serialization of a block does not include the block height.  Thus, we'll need to
    // append the height.

    // These are serialization sizes
    size_t blockHeightSize = sizeof (uint32_t);
    size_t blockSize = BRMerkleBlockSerialize(block, NULL, 0);

    // Confirm.
    assert (blockHeightSize == sizeof (block->height));

    // Update bytesCound with the total of what is written.
    *bytesCount = (uint32_t) (blockSize + blockHeightSize);

    // Get our bytes
    uint8_t *bytes = calloc (*bytesCount, 1);

    // We'll serialize the block itself first
    BRMerkleBlockSerialize(block, bytes, blockSize);

    // And then the height.
    UInt32SetLE(&bytes[blockSize], block->height);

    return bytes;
}

static void *
fileServiceTypeBlockV1Reader (BRFileServiceContext context,
                              BRFileService fs,
                              uint8_t *bytes,
                              uint32_t bytesCount) {
    size_t blockHeightSize = sizeof (uint32_t);

    BRMerkleBlock *block = BRMerkleBlockParse (bytes, bytesCount);
    if (NULL == block) return NULL;

    block->height = UInt32GetLE(&bytes[bytesCount - blockHeightSize]);

    return block;
}

static BRArrayOf(BRMerkleBlock*)
initialBlocksLoad (BRWalletManager manager) {
    BRSetOf(BRTransaction*) blockSet = BRSetNew(BRMerkleBlockHash, BRMerkleBlockEq, 100);
    if (1 != fileServiceLoad (manager->fileService, blockSet, fileServiceTypeBlocks, 1)) {
        BRSetFree (blockSet);
        return NULL;
    }

    size_t blocksCount = BRSetCount(blockSet);

    BRArrayOf(BRMerkleBlock*) blocks;
    array_new (blocks, blocksCount);
    array_set_count(blocks, blocksCount);

    BRSetAll(blockSet, (void**) blocks, blocksCount);
    BRSetFree(blockSet);

    return blocks;
}

/// MARK: - Peer File Service

#define fileServiceTypePeers        "peers"
enum {
    WALLET_MANAGER_PEER_VERSION_1
};

static UInt256
fileServiceTypePeerV1Identifier (BRFileServiceContext context,
                                 BRFileService fs,
                                 const void *entity) {
    const BRPeer *peer = entity;

    UInt256 hash;
    BRSHA256 (&hash, peer, sizeof(BRPeer));

    return hash;
}

static uint8_t *
fileServiceTypePeerV1Writer (BRFileServiceContext context,
                             BRFileService fs,
                             const void* entity,
                             uint32_t *bytesCount) {
    const BRPeer *peer = entity;

    // long term, this is wrong
    *bytesCount = sizeof (BRPeer);
    uint8_t *bytes = malloc (*bytesCount);
    memcpy (bytes, peer, *bytesCount);

    return bytes;
}

static void *
fileServiceTypePeerV1Reader (BRFileServiceContext context,
                             BRFileService fs,
                             uint8_t *bytes,
                             uint32_t bytesCount) {
    assert (bytesCount == sizeof (BRPeer));

    BRPeer *peer = malloc (bytesCount);;
    memcpy (peer, bytes, bytesCount);

    return peer;
}

static BRArrayOf(BRPeer)
initialPeersLoad (BRWalletManager manager) {
    /// Load peers for the wallet manager.
    BRSetOf(BRPeer*) peerSet = BRSetNew(BRPeerHash, BRPeerEq, 100);
    if (1 != fileServiceLoad (manager->fileService, peerSet, fileServiceTypePeers, 1)) {
        BRSetFree(peerSet);
        return NULL;
    }

    size_t peersCount = BRSetCount(peerSet);
    BRPeer *peersRefs[peersCount];

    BRSetAll(peerSet, (void**) peersRefs, peersCount);
    BRSetClear(peerSet);
    BRSetFree(peerSet);

    BRArrayOf(BRPeer) peers;
    array_new (peers, peersCount);

    for (size_t index = 0; index < peersCount; index++)
        array_add (peers, *peersRefs[index]);

    return peers;
}

static void
bwmFileServiceErrorHandler (BRFileServiceContext context,
                            BRFileService fs,
                            BRFileServiceError error) {
    switch (error.type) {
        case FILE_SERVICE_IMPL:
            // This actually a FATAL - an unresolvable coding error.
            _peer_log ("bread: FileService Error: IMPL: %s", error.u.impl.reason);
            break;
        case FILE_SERVICE_UNIX:
            _peer_log ("bread: FileService Error: UNIX: %s", strerror(error.u.unix.error));
            break;
        case FILE_SERVICE_ENTITY:
            // This is likely a coding error too.
            _peer_log ("bread: FileService Error: ENTITY (%s); %s",
                     error.u.entity.type,
                     error.u.entity.reason);
            break;
    }
    _peer_log ("bread: FileService Error: FORCED SYNC%s", "");

    // BRWalletManager bwm = (BRWalletManager) context;
    // TODO(fix): What do we actually want to happen here?
    // if (NULL != bwm->peerManager)
    //     BRPeerManagerRescan (bwm->peerManager);
}

#define fileServiceSpecificationsCount      (3)
static BRFileServiceTypeSpecification fileServiceSpecifications[] = {
    {
        fileServiceTypeTransactions,
        WALLET_MANAGER_TRANSACTION_VERSION_1,
        1,
        {
            {
                WALLET_MANAGER_TRANSACTION_VERSION_1,
                fileServiceTypeTransactionV1Identifier,
                fileServiceTypeTransactionV1Reader,
                fileServiceTypeTransactionV1Writer
            }
        }
    },

    {
        fileServiceTypeBlocks,
        WALLET_MANAGER_BLOCK_VERSION_1,
        1,
        {
            {
                WALLET_MANAGER_BLOCK_VERSION_1,
                fileServiceTypeBlockV1Identifier,
                fileServiceTypeBlockV1Reader,
                fileServiceTypeBlockV1Writer
            }
        }
    },

    {
        fileServiceTypePeers,
        WALLET_MANAGER_PEER_VERSION_1,
        1,
        {
            {
                WALLET_MANAGER_PEER_VERSION_1,
                fileServiceTypePeerV1Identifier,
                fileServiceTypePeerV1Reader,
                fileServiceTypePeerV1Writer
            }
        }
    }
};

/// MARK: - Wallet Manager

static BRWalletManager
bwmCreateErrorHandler (BRWalletManager bwm, int fileService, const char* reason) {
    if (NULL != bwm) free (bwm);
    if (fileService)
        _peer_log ("bread: on ewmCreate: FileService Error: %s", reason);
    else
        _peer_log ("bread: on ewmCreate: Error: %s", reason);

    return NULL;
}

extern BRWalletManager
BRWalletManagerNew (BRWalletManagerClient client,
                    BRMasterPubKey mpk,
                    const BRChainParams *params,
                    uint32_t earliestKeyTime,
                    BRSyncMode mode,
                    const char *baseStoragePath,
                    uint64_t blockHeight) {
    assert (mode == SYNC_MODE_BRD_ONLY || SYNC_MODE_P2P_ONLY);

    BRWalletManager bwm = calloc (1, sizeof (struct BRWalletManagerStruct));
    if (NULL == bwm) return bwmCreateErrorHandler (NULL, 0, "allocate");

    bwm->mode = mode;
    bwm->client = client;
    bwm->chainParams = params;
    bwm->earliestKeyTime = earliestKeyTime;
    bwm->sleepWakeupsForSyncTickTock = 0;

    const char *networkName  = getNetworkName  (params);
    const char *currencyName = getCurrencyName (params);

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&bwm->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // Create the alarm clock, but don't start it.
    alarmClockCreateIfNecessary(0);

    const char *handlerName = (BRChainParamsIsBitcoin (params)
                              ? "Core Bitcoin BWM"
                              : (BRChainParamsIsBitcash (params)
                                 ? "Core Bitcash BWM"
                                 : "Core BWM"));

    // The `main` event handler has a periodic wake-up.  Used, perhaps, if the mode indicates
    // that we should/might query the BRD backend services.
    bwm->handler = eventHandlerCreate (handlerName,
                                       bwmEventTypes,
                                       bwmEventTypesCount,
                                       &bwm->lock);

    //
    // Create the File Service w/ associated types.
    //
    bwm->fileService = fileServiceCreateFromTypeSpecfications (baseStoragePath, currencyName, networkName,
                                                               bwm,
                                                               bwmFileServiceErrorHandler,
                                                               fileServiceSpecificationsCount,
                                                               fileServiceSpecifications);
    if (NULL == bwm->fileService) return bwmCreateErrorHandler (bwm, 1, "create");

    /// Load transactions for the wallet manager.
    BRArrayOf(BRTransaction*) transactions = initialTransactionsLoad(bwm);
    /// Load blocks and peers for the peer manager.
    BRArrayOf(BRMerkleBlock*) blocks = initialBlocksLoad(bwm);
    BRArrayOf(BRPeer) peers = initialPeersLoad(bwm);

    // If any of these are NULL, then there was a failure; on a failure they all need to be cleared
    // which will cause a *FULL SYNC*
    if (NULL == transactions || NULL == blocks || NULL == peers) {
        if (NULL == transactions) array_new (transactions, 1);
        else array_clear(transactions);

        if (NULL == blocks) array_new (blocks, 1);
        else array_clear(blocks);

        if (NULL == peers) array_new (peers, 1);
        else array_clear(peers);
    }

    bwm->wallet = BRWalletNew (params->addrParams, transactions, array_count(transactions), mpk);
    BRWalletSetCallbacks (bwm->wallet, bwm,
                          _BRWalletManagerBalanceChanged,
                          _BRWalletManagerTxAdded,
                          _BRWalletManagerTxUpdated,
                          _BRWalletManagerTxDeleted);

    bwm->syncManager = BRSyncManagerNewForMode (mode,
                                                bwm,
                                                _BRWalletManagerSyncEvent,
                                                bwm,
                                                (BRSyncManagerClientCallbacks) {
                                                    _BRWalletManagerGetBlockNumber,
                                                    _BRWalletManagerGetTransactions,
                                                    _BRWalletManagerSubmitTransaction,
                                                },
                                                params,
                                                bwm->wallet,
                                                earliestKeyTime,
                                                blockHeight,
                                                blocks, array_count(blocks),
                                                peers,  array_count(peers));

    array_new(bwm->transactions, DEFAULT_TRANSACTION_CAPACITY);

    // No longer need the loaded txns/blocks/peers
    array_free(transactions); array_free(blocks); array_free(peers);

    // Create initial events for wallet manager creation, wallet addition and
    // events for any transactions loaded from disk.

    bwmSignalWalletManagerEvent(bwm,
                                (BRWalletManagerEvent) {
                                    BITCOIN_WALLET_MANAGER_CREATED
                                });

    bwmSignalWalletEvent (bwm,
                          bwm->wallet,
                          (BRWalletEvent) {
                              BITCOIN_WALLET_CREATED
                          });

    size_t txCount = BRWalletTransactions (bwm->wallet, NULL, 0);
    if (txCount) {
        BRArrayOf(BRTransaction *) txns = calloc (txCount, sizeof(BRTransaction*));
        BRWalletTransactions (bwm->wallet, txns, txCount);

        for (size_t i = 0; i < txCount; i++) {
            // Create a new wallet-based transaction. No lock is held here because we
            // are still in the "ctor" and callbacks are not being made until the
            // event handler is started in a later call.
            BRTransactionWithState txnWithState = BRWalletManagerAddTransaction (bwm,
                                                                                 BRTransactionCopy (txns[i]),
                                                                                 txns[i]);

            bwmSignalTransactionEvent (bwm,
                                    bwm->wallet,
                                    BRTransactionWithStateGetOwned (txnWithState),
                                    (BRTransactionEvent) {
                                        BITCOIN_TRANSACTION_ADDED
                                    });

            bwmSignalTransactionEvent (bwm,
                                    bwm->wallet,
                                    BRTransactionWithStateGetOwned (txnWithState),
                                    (BRTransactionEvent) {
                                    BITCOIN_TRANSACTION_UPDATED,
                                        { .updated = {
                                            BRTransactionWithStateGetOwned (txnWithState)->blockHeight,
                                            BRTransactionWithStateGetOwned (txnWithState)->timestamp }}
                                    });
        }

        free (txns);
    }

    // Add ewmPeriodicDispatcher to handlerForMain.  Note that a 'timeout' is handled by
    // an OOB (out-of-band) event whereby the event is pushed to the front of the queue.
    // This may not be the right thing to do.  Imagine that EWM is blocked somehow (doing
    // a time consuming calculation) and two 'timeout events' occur - the events will be
    // queued in the wrong order (second before first).
    //
    // The function `ewmPeriodcDispatcher()` will be installed as a periodic alarm
    // on the event handler.  It will only trigger when the event handler is running (
    // the time between `eventHandlerStart()` and `eventHandlerStop()`)

    eventHandlerSetTimeoutDispatcher (bwm->handler,
                                      1000 * BWM_SLEEP_SECONDS,
                                      (BREventDispatcher) bwmPeriodicDispatcher,
                                      (void*) bwm);

    return bwm;
}

extern void
BRWalletManagerFree (BRWalletManager manager) {
    // holding lock, tear down the manager
    pthread_mutex_lock (&manager->lock);
    {
        // stop, including disconnect.
        BRWalletManagerStop (manager);

        BRSyncManagerFree (manager->syncManager);
        BRWalletFree (manager->wallet);

        BRWalletManagerFreeTransactions (manager);
        eventHandlerDestroy (manager->handler);
        fileServiceRelease (manager->fileService);
    }
    pthread_mutex_unlock (&manager->lock);

    pthread_mutex_destroy (&manager->lock);
    memset (manager, 0, sizeof(*manager));
    free (manager);
}

extern void
BRWalletManagerStart (BRWalletManager manager) {
    eventHandlerStart (manager->handler);
}

extern void
BRWalletManagerStop (BRWalletManager manager) {
    BRWalletManagerDisconnect (manager);
    eventHandlerStop (manager->handler);
}

extern BRWallet *
BRWalletManagerGetWallet (BRWalletManager manager) {
    return manager->wallet;
}

extern int
BRWalletManagerHandlesBTC (BRWalletManager manager) {
    return BRChainParamsIsBitcoin (manager->chainParams);
}

extern void
BRWalletManagerConnect (BRWalletManager manager) {
    pthread_mutex_lock (&manager->lock);
    BRSyncManagerConnect (manager->syncManager);
    pthread_mutex_unlock (&manager->lock);
}

extern void
BRWalletManagerDisconnect (BRWalletManager manager) {
    pthread_mutex_lock (&manager->lock);
    BRSyncManagerDisconnect (manager->syncManager);
    pthread_mutex_unlock (&manager->lock);
}

extern void
BRWalletManagerScan (BRWalletManager manager) {
    pthread_mutex_lock (&manager->lock);
    BRSyncManagerScan (manager->syncManager);
    pthread_mutex_unlock (&manager->lock);
}

extern void
BRWalletManagerSetMode (BRWalletManager manager, BRSyncMode mode) {
    pthread_mutex_lock (&manager->lock);
    if (mode != manager->mode) {
        // get the currently known block height
        uint64_t blockHeight = BRSyncManagerGetBlockHeight (manager->syncManager);

        // kill the sync manager to prevent any additional callbacks from occuring
        BRSyncManagerDisconnect (manager->syncManager);
        BRSyncManagerFree (manager->syncManager);

        // load up the blocks/peers saved to disk
        BRArrayOf(BRMerkleBlock*) blocks = initialBlocksLoad(manager);
        BRArrayOf(BRPeer) peers = initialPeersLoad(manager);

        // If any of these are NULL, then there was a failure; on a failure they all need to be cleared
        // which will cause a *FULL SYNC*
        if (NULL == blocks || NULL == peers) {
            if (NULL == blocks) array_new (blocks, 1);
            else array_clear(blocks);

            if (NULL == peers) array_new (peers, 1);
            else array_clear(peers);
        }

        // set the new mode
        manager->mode = mode;

        // create the new sync manager
        manager->syncManager = BRSyncManagerNewForMode (mode,
                                                        manager,
                                                        _BRWalletManagerSyncEvent,
                                                        manager,
                                                        (BRSyncManagerClientCallbacks) {
                                                            _BRWalletManagerGetBlockNumber,
                                                            _BRWalletManagerGetTransactions,
                                                            _BRWalletManagerSubmitTransaction,
                                                        },
                                                        manager->chainParams,
                                                        manager->wallet,
                                                        manager->earliestKeyTime,
                                                        blockHeight,
                                                        blocks, array_count (blocks),
                                                        peers, array_count (peers));
    }
    pthread_mutex_unlock (&manager->lock);
}

extern BRSyncMode
BRWalletManagerGetMode (BRWalletManager manager) {
    pthread_mutex_lock (&manager->lock);
    BRSyncMode mode = manager->mode;
    pthread_mutex_unlock (&manager->lock);
    return mode;
}

extern BRTransaction *
BRWalletManagerCreateTransaction (BRWalletManager manager,
                                  BRWallet *wallet,
                                  uint64_t amount,
                                  BRAddress addr,
                                  uint64_t feePerKb) {
    assert (wallet == manager->wallet);

    pthread_mutex_lock (&manager->lock);
    uint64_t feePerKbSaved = BRWalletFeePerKb (wallet);

    BRWalletSetFeePerKb (wallet, feePerKb);
    BRTransaction *transaction = BRWalletCreateTransaction (wallet, amount, addr.s);
    BRWalletSetFeePerKb (wallet, feePerKbSaved);

    BRTransactionWithState txnWithState = (NULL != transaction) ? BRWalletManagerAddTransaction (manager, transaction, NULL) : NULL;
    pthread_mutex_unlock (&manager->lock);

    if (NULL != txnWithState) {
        bwmSignalTransactionEvent(manager,
                                  wallet,
                                  BRTransactionWithStateGetOwned (txnWithState),
                                  (BRTransactionEvent) {
                                      BITCOIN_TRANSACTION_CREATED
                                  });
    }

    return transaction;
}

extern BRTransaction *
BRWalletManagerCreateTransactionForSweep (BRWalletManager manager,
                                          BRWallet *wallet,
                                          BRWalletSweeper sweeper,
                                          uint64_t feePerKb) {
    assert (wallet == manager->wallet);

    pthread_mutex_lock (&manager->lock);

    // TODO(fix): We should move this, along with BRWalletManagerCreateTransaction, to
    //            a model where they return a status code. We are currently providing no
    //            context to the caller.
    BRTransaction *transaction = NULL;
    BRWalletSweeperCreateTransaction (sweeper, wallet, feePerKb, &transaction);

    BRTransactionWithState txnWithState = (NULL != transaction) ? BRWalletManagerAddTransaction (manager, transaction, NULL) : NULL;
    pthread_mutex_unlock (&manager->lock);

    if (NULL != txnWithState) {
        bwmSignalTransactionEvent(manager,
                                  wallet,
                                  BRTransactionWithStateGetOwned (txnWithState),
                                  (BRTransactionEvent) {
                                      BITCOIN_TRANSACTION_CREATED
                                  });
    }

    return transaction;
}

extern BRTransaction *
BRWalletManagerCreateTransactionForOutputs (BRWalletManager manager,
                                            BRWallet *wallet,
                                            BRTxOutput *outputs,
                                            size_t outputsLen,
                                            uint64_t feePerKb) {
    assert (wallet == manager->wallet);

    pthread_mutex_lock (&manager->lock);
    uint64_t feePerKbSaved = BRWalletFeePerKb (wallet);

    BRWalletSetFeePerKb (wallet, feePerKb);
    BRTransaction *transaction = BRWalletCreateTxForOutputs (wallet, outputs, outputsLen);
    BRWalletSetFeePerKb (wallet, feePerKbSaved);

    BRTransactionWithState txnWithState = (NULL != transaction) ? BRWalletManagerAddTransaction (manager, transaction, NULL) : NULL;
    pthread_mutex_unlock (&manager->lock);

    if (NULL != txnWithState) {
        bwmSignalTransactionEvent(manager,
                                  wallet,
                                  BRTransactionWithStateGetOwned (txnWithState),
                                  (BRTransactionEvent) {
                                      BITCOIN_TRANSACTION_CREATED
                                  });
    }

    return transaction;
}

extern int
BRWalletManagerSignTransaction (BRWalletManager manager,
                                BRWallet *wallet,
                                OwnershipKept BRTransaction *transaction,
                                const void *seed,
                                size_t seedLen) {
    assert (wallet == manager->wallet);

    pthread_mutex_lock (&manager->lock);
    BRTransactionWithState txnWithState = BRWalletManagerFindTransactionByOwned (manager, transaction);
    pthread_mutex_unlock (&manager->lock);

    int success = 0;
    if (NULL != txnWithState &&
        1 == BRWalletSignTransaction (wallet,
                                      BRTransactionWithStateGetOwned (txnWithState),
                                      manager->chainParams->forkId,
                                      seed,
                                      seedLen)) {
        success = 1;
        bwmSignalTransactionEvent(manager,
                                  wallet,
                                  BRTransactionWithStateGetOwned (txnWithState),
                                  (BRTransactionEvent) {
                                      BITCOIN_TRANSACTION_SIGNED
                                  });
    }

    return success;
}

extern int
BRWalletManagerSignTransactionForKey (BRWalletManager manager,
                                      BRWallet *wallet,
                                      OwnershipKept BRTransaction *transaction,
                                      BRKey *key) {
    assert (wallet == manager->wallet);

    pthread_mutex_lock (&manager->lock);
    BRTransactionWithState txnWithState = BRWalletManagerFindTransactionByOwned (manager, transaction);
    pthread_mutex_unlock (&manager->lock);

    int success = 0;
    if (NULL != txnWithState &&
        1 == BRTransactionSign (BRTransactionWithStateGetOwned (txnWithState),
                                manager->chainParams->forkId,
                                key,
                                1)) {
        success = 1;
        bwmSignalTransactionEvent(manager,
                                  wallet,
                                  BRTransactionWithStateGetOwned (txnWithState),
                                  (BRTransactionEvent) {
                                      BITCOIN_TRANSACTION_SIGNED
                                  });
    }

    return success;
}

extern void
BRWalletManagerSubmitTransaction (BRWalletManager manager,
                                  BRWallet *wallet,
                                  OwnershipKept BRTransaction *transaction) {
    assert (wallet == manager->wallet);

    pthread_mutex_lock (&manager->lock);
    BRTransactionWithState txnWithState = BRWalletManagerFindTransactionByOwned (manager, transaction);
    if (NULL != txnWithState) {
        BRSyncManagerSubmit (manager->syncManager, BRTransactionWithStateGetOwned (txnWithState));
    }
    pthread_mutex_unlock (&manager->lock);
}

extern void
BRWalletManagerUpdateFeePerKB (BRWalletManager manager,
                               BRWallet *wallet,
                               uint64_t feePerKb) {
    assert (wallet == manager->wallet);

    BRWalletSetFeePerKb (wallet, feePerKb);
    bwmSignalWalletEvent(manager,
                         wallet,
                         (BRWalletEvent) {
                             BITCOIN_WALLET_FEE_PER_KB_UPDATED,
                             { .feePerKb = { feePerKb }}
                         });
}

extern void
BRWalletManagerEstimateFeeForTransfer (BRWalletManager manager,
                                       BRWallet *wallet,
                                       BRCookie cookie,
                                       uint64_t transferAmount,
                                       uint64_t feePerKb) {
    assert (wallet == manager->wallet);

    pthread_mutex_lock (&manager->lock);
    uint64_t feePerKBSaved = BRWalletFeePerKb (wallet);

    BRWalletSetFeePerKb (wallet, feePerKb);
    uint64_t fee  = (0 == transferAmount ? 0 : BRWalletFeeForTxAmount (wallet, transferAmount));
    uint32_t sizeInByte = (uint32_t) ((1000 * fee)/ feePerKb);
    BRWalletSetFeePerKb (wallet, feePerKBSaved);
    pthread_mutex_unlock (&manager->lock);

    bwmSignalWalletEvent(manager,
                         wallet,
                         (BRWalletEvent) {
                             BITCOIN_WALLET_FEE_ESTIMATED,
                                { .feeEstimated = { cookie, feePerKb, sizeInByte }}
                         });
}

extern void
BRWalletManagerEstimateFeeForSweep (BRWalletManager manager,
                                    BRWallet *wallet,
                                    BRCookie cookie,
                                    BRWalletSweeper sweeper,
                                    uint64_t feePerKb) {
    assert (wallet == manager->wallet);

    // TODO(fix): We should move this, along with BRWalletManagerEstimateFeeForTransfer, to
    //            a model where they return a status code. We are currently providing no
    //            context to the caller.
    uint64_t fee = 0;
    BRWalletSweeperEstimateFee (sweeper, wallet, feePerKb, &fee);
    uint32_t sizeInByte = (uint32_t) ((1000 * fee)/ feePerKb);

    bwmSignalWalletEvent(manager,
                         wallet,
                         (BRWalletEvent) {
                             BITCOIN_WALLET_FEE_ESTIMATED,
                                { .feeEstimated = { cookie, feePerKb, sizeInByte }}
                         });
}

extern void
BRWalletManagerEstimateFeeForOutputs (BRWalletManager manager,
                                      BRWallet *wallet,
                                      BRCookie cookie,
                                      BRTxOutput *outputs,
                                      size_t outputsLen,
                                      uint64_t feePerKb) {
    assert (wallet == manager->wallet);

    pthread_mutex_lock (&manager->lock);
    uint64_t feePerKbSaved = BRWalletFeePerKb (wallet);

    BRWalletSetFeePerKb (wallet, feePerKb);
    BRTransaction *transaction = BRWalletCreateTxForOutputs (wallet, outputs, outputsLen);
    BRWalletSetFeePerKb (wallet, feePerKbSaved);

    uint64_t fee = 0;
    if (NULL != transaction) {
        fee = BRWalletFeeForTx(wallet, transaction);
        BRTransactionFree(transaction);
    }
    uint32_t sizeInByte = (uint32_t) ((1000 * fee)/ feePerKb);
    pthread_mutex_unlock (&manager->lock);

    bwmSignalWalletEvent(manager,
                         wallet,
                         (BRWalletEvent) {
                             BITCOIN_WALLET_FEE_ESTIMATED,
                                { .feeEstimated = { cookie, feePerKb, sizeInByte }}
                         });
}

/// MARK: Wallet Callbacks

/**
 * These callbacks come from the BRWallet. That component spawns multiple threads of its own. As
 * such, these callbacks can occur on any thread (including that of the caller that triggered the
 * callback).
 *
 * !!!! These callbacks do NOT (and should NEVER) acquire the BRWalletManager::lock. !!!!
 */

static void
_BRWalletManagerBalanceChanged (void *info,
                                uint64_t balanceInSatoshi) {
    BRWalletManager manager = (BRWalletManager) info;

    bwmSignalWalletEvent(manager,
                         manager->wallet,
                         (BRWalletEvent) {
                             BITCOIN_WALLET_BALANCE_UPDATED,
                             { .balance = { balanceInSatoshi }}
                         });
}

static void
_BRWalletManagerTxAdded (void *info,
                         OwnershipKept BRTransaction *tx) {
    BRWalletManager manager = (BRWalletManager) info;
    assert (BRTransactionIsSigned (tx));

    // filesystem changes are NOT queued; they are acted upon immediately
    fileServiceSave(manager->fileService, fileServiceTypeTransactions, tx);

    bwmSignalTxAdded (manager, BRTransactionCopy (tx), tx);
}

static void
_BRWalletManagerTxUpdated (void *info,
                           OwnershipKept const UInt256 *hashes,
                           size_t count,
                           uint32_t blockHeight,
                           uint32_t timestamp) {
    BRWalletManager manager = (BRWalletManager) info;

    for (size_t index = 0; index < count; index++) {
        BRTransaction *transaction = BRWalletTransactionCopyForHash(manager->wallet, hashes[index]);
        if (NULL != transaction) {
            // assert timestamp and blockHeight in transaction
            // filesystem changes are NOT queued; they are acted upon immediately
            fileServiceSave (manager->fileService, fileServiceTypeTransactions, transaction);

            bwmSignalTxUpdated (manager, hashes[index], blockHeight, timestamp);

            BRTransactionFree (transaction);
        }
    }
}

static void
_BRWalletManagerTxDeleted (void *info,
                           UInt256 hash,
                           int notifyUser,
                           int recommendRescan) {
    BRWalletManager manager = (BRWalletManager) info;

    // filesystem changes are NOT queued; they are acted upon immediately
    fileServiceRemove(manager->fileService, fileServiceTypeTransactions, hash);

    bwmSignalTxDeleted (manager, hash);
}

/// MARK: Wallet Callback Events

/**
 * These handlers are called by the event handler thread.
 */

extern void
bwmHandleTxAdded (BRWalletManager manager,
                  OwnershipGiven BRTransaction *ownedTransaction,
                  OwnershipKept BRTransaction *refedTransaction) {
    pthread_mutex_lock (&manager->lock);
    BRTransactionWithState txnWithState = BRWalletManagerFindTransactionByHash (manager, ownedTransaction->txHash);
    if (NULL == txnWithState) {
        // first we've seen it, so it came from the network; add it to our list
        txnWithState = BRWalletManagerAddTransaction (manager, ownedTransaction, refedTransaction);
    } else {
        // this is a transaction we've submitted; set the reference transaction from the wallet
        BRTransactionWithStateSetReferenced (txnWithState, refedTransaction);
        BRTransactionWithStateSetBlock (txnWithState, ownedTransaction->blockHeight, ownedTransaction->timestamp);

        // we already have an owned copy of this transaction; free up the passed one
        BRTransactionFree (ownedTransaction);
    }
    assert (NULL != txnWithState);
    pthread_mutex_unlock (&manager->lock);

    bwmSignalTransactionEvent(manager,
                              manager->wallet,
                              BRTransactionWithStateGetOwned (txnWithState),
                              (BRTransactionEvent) {
                                  BITCOIN_TRANSACTION_ADDED
                              });
}

extern void
bwmHandleTxUpdated (BRWalletManager manager,
                    UInt256 hash,
                    uint32_t blockHeight,
                    uint32_t timestamp) {
    pthread_mutex_lock (&manager->lock);
    BRTransactionWithState txnWithState = BRWalletManagerFindTransactionByHash (manager, hash);
    assert (NULL != txnWithState && BRTransactionIsSigned (BRTransactionWithStateGetOwned (txnWithState)));

    BRTransactionWithStateSetBlock (txnWithState, blockHeight, timestamp);
    pthread_mutex_unlock (&manager->lock);

    bwmSignalTransactionEvent(manager,
                                manager->wallet,
                                BRTransactionWithStateGetOwned (txnWithState),
                                (BRTransactionEvent) {
                                    BITCOIN_TRANSACTION_UPDATED,
                                    { .updated = { blockHeight, timestamp }}
                                });
}

extern void
bwmHandleTxDeleted (BRWalletManager manager,
                    UInt256 hash) {
    pthread_mutex_lock (&manager->lock);
    BRTransactionWithState txnWithState = BRWalletManagerFindTransactionByHash (manager, hash);
    assert (NULL != txnWithState && BRTransactionIsSigned (BRTransactionWithStateGetOwned (txnWithState)));

    BRTransactionWithStateSetDeleted (txnWithState);
    pthread_mutex_unlock (&manager->lock);

    bwmSignalTransactionEvent(manager,
                              manager->wallet,
                              BRTransactionWithStateGetOwned (txnWithState),
                              (BRTransactionEvent) {
                                  BITCOIN_TRANSACTION_DELETED
                              });
}

///
/// Mark: SyncManager Event Callback
///

/**
 * This callback comes from the BRSyncManager. That component has no inherent threading model
 * of its own. As such, these callbacks can occur on any thread (including that of the caller
 * that triggered the event).
 *
 * !!!! These callbacks do NOT (and should NEVER) acquire the BRWalletManager::lock. !!!!
 */

static void
_BRWalletManagerSyncEvent(void * context,
                          BRSyncManager manager,
                          OwnershipKept BRSyncManagerEvent event) {
    /**
     * The `BRSyncManagerEvent` event is `OwnershipKept`. That means that the
     * event needs to be handled inline or copies of the event data need to be
     * made.
     *
     * For BLOCKS and PEERS events, we handle them inline, rather than copy them
     * as filesystem changes are acted upon immediately.
     *
     * For CONNECTIVITY/SYNCING/HEIGHT events, we queue them, as they contain no out
     * of band data (i.e. pointers).
     *
     * For SYNC_MANAGER_TXN_SUBMITTED, we queue them, as all transactions are owned
     * by this wallet manager.
     */

    BRWalletManager bwm = (BRWalletManager) context;
    switch (event.type) {
        case SYNC_MANAGER_SET_BLOCKS: {
            // filesystem changes are NOT queued; they are acted upon immediately
            fileServiceClear(bwm->fileService, fileServiceTypeBlocks);

            // !!!!!!!!!
            // no break;
            // !!!!!!!!!
        }
        case SYNC_MANAGER_ADD_BLOCKS: {
            // filesystem changes are NOT queued; they are acted upon immediately
            for (size_t index = 0; index < event.u.blocks.count; index++)
                fileServiceSave (bwm->fileService, fileServiceTypeBlocks, event.u.blocks.blocks[index]);
            break;
        }

        case SYNC_MANAGER_SET_PEERS: {
            // filesystem changes are NOT queued; they are acted upon immediately
            fileServiceClear(bwm->fileService, fileServiceTypePeers);

            // !!!!!!!!!
            // no break;
            // !!!!!!!!!
        }
        case SYNC_MANAGER_ADD_PEERS: {
            // filesystem changes are NOT queued; they are acted upon immediately
            for (size_t index = 0; index < event.u.peers.count; index++)
                fileServiceSave (bwm->fileService, fileServiceTypePeers, &event.u.peers.peers[index]);
            break;
        }

        case SYNC_MANAGER_CONNECTED: {
            bwmSignalWalletManagerEvent(bwm,
                                        (BRWalletManagerEvent) {
                                            BITCOIN_WALLET_MANAGER_CONNECTED
                                        });
            break;
        }
        case SYNC_MANAGER_DISCONNECTED: {
            bwmSignalWalletManagerEvent(bwm,
                                        (BRWalletManagerEvent) {
                                            BITCOIN_WALLET_MANAGER_DISCONNECTED
                                        });
            break;
        }

        case SYNC_MANAGER_SYNC_STARTED: {
            bwmSignalWalletManagerEvent(bwm,
                                        (BRWalletManagerEvent) {
                                            BITCOIN_WALLET_MANAGER_SYNC_STARTED
                                        });
            break;
        }
        case SYNC_MANAGER_SYNC_PROGRESS: {
            bwmSignalWalletManagerEvent(bwm,
                                        (BRWalletManagerEvent) {
                                            BITCOIN_WALLET_MANAGER_SYNC_PROGRESS,
                                            { .syncProgress = {
                                                event.u.syncProgress.timestamp,
                                                event.u.syncProgress.percentComplete }}
                                        });
            break;
        }
        case SYNC_MANAGER_SYNC_STOPPED: {
            bwmSignalWalletManagerEvent(bwm,
                                        (BRWalletManagerEvent) {
                                            BITCOIN_WALLET_MANAGER_SYNC_STOPPED,
                                            { .syncStopped = { event.u.syncStopped.reason }}
                                        });
            break;
        }

        case SYNC_MANAGER_BLOCK_HEIGHT_UPDATED: {
            bwmSignalWalletManagerEvent (bwm,
                                         (BRWalletManagerEvent) {
                                             BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED,
                                             { .blockHeightUpdated = { event.u.blockHeightUpdated.value }}
                                         });
            break;
        }

        case SYNC_MANAGER_TXN_SUBMITTED: {
            bwmSignalWalletEvent(bwm,
                                 bwm->wallet,
                                 (BRWalletEvent) {
                                     BITCOIN_WALLET_TRANSACTION_SUBMITTED,
                                     { .submitted = { event.u.submitted.transaction, event.u.submitted.error }}
                                 });
            break;
        }

        case SYNC_MANAGER_TXNS_UPDATED: {
            // TODO(discuss): Do we want to do something here, once we track all transactions?
            break;
        }
    }
}

///
/// Mark: BRSyncManager Client Callbacks
///

/**
 * These callbacks come from the BRSyncManager. That component has no inherent threading model
 * of its own. As such, these callbacks can occur on any thread (including that of the caller
 * that triggered the event).
 *
 * !!!! These callbacks do NOT (and should NEVER) acquire the BRWalletManager::lock. !!!!
 */

static void _BRWalletManagerGetBlockNumber(void * context,
                                           BRSyncManager manager,
                                           int rid) {
    BRWalletManager bwm = (BRWalletManager) context;

    assert  (NULL != bwm->client.funcGetBlockNumber);
    bwm->client.funcGetBlockNumber (bwm->client.context,
                                    bwm,
                                    rid);
}
static void _BRWalletManagerGetTransactions(void * context,
                                            BRSyncManager manager,
                                            OwnershipKept const char **addresses,
                                            size_t addressCount,
                                            uint64_t begBlockNumber,
                                            uint64_t endBlockNumber,
                                            int rid) {
    BRWalletManager bwm = (BRWalletManager) context;

    assert  (NULL != bwm->client.funcGetTransactions);
    bwm->client.funcGetTransactions (bwm->client.context,
                                    bwm,
                                    addresses,
                                    addressCount,
                                    begBlockNumber,
                                    endBlockNumber,
                                    rid);
}

static void
_BRWalletManagerSubmitTransaction(void * context,
                                  BRSyncManager manager,
                                  OwnershipKept uint8_t *transaction,
                                  size_t transactionLength,
                                  UInt256 transactionHash,
                                  int rid) {
    BRWalletManager bwm = (BRWalletManager) context;

    assert  (NULL != bwm->client.funcSubmitTransaction);
    bwm->client.funcSubmitTransaction (bwm->client.context,
                                       bwm,
                                       bwm->wallet,
                                       transaction,
                                       transactionLength,
                                       transactionHash,
                                       rid);
}

///
/// MARK: BRWalletManagerClient Completion Routines
//

/**
 * These announcers are called by a client once it has completed a request. The threading
 * model of a client is unknown. As such, these callbacks can occur on any thread.
 */

extern int
bwmAnnounceBlockNumber (BRWalletManager manager,
                        int rid,
                        uint64_t blockNumber) {
    bwmSignalAnnounceBlockNumber (manager,
                                  rid,
                                  blockNumber);
    return 1;
}

extern int
bwmAnnounceTransaction (BRWalletManager manager,
                        int id,
                        OwnershipKept uint8_t *transaction,
                        size_t transactionLength,
                        uint64_t timestamp,
                        uint64_t blockHeight) {
    bwmSignalAnnounceTransaction (manager,
                                  id,
                                  transaction,
                                  transactionLength,
                                  timestamp,
                                  blockHeight);
    return 1;
}

extern void
bwmAnnounceTransactionComplete (BRWalletManager manager,
                                int rid,
                                int success) {
    bwmSignalAnnounceTransactionComplete (manager,
                                          rid,
                                          success);
}

extern void
bwmAnnounceSubmit (BRWalletManager manager,
                   int rid,
                   UInt256 txHash,
                   int error) {
    bwmSignalAnnounceSubmit (manager,
                             rid,
                             txHash,
                             error);
}

// These handlers are called by the event handler thread.

extern int
bwmHandleAnnounceBlockNumber (BRWalletManager manager,
                              int rid,
                              uint64_t blockNumber) {
    assert (eventHandlerIsCurrentThread (manager->handler));

    pthread_mutex_lock (&manager->lock);
    BRSyncManagerAnnounceGetBlockNumber (manager->syncManager,
                                         rid,
                                         (int32_t) blockNumber);
    pthread_mutex_unlock (&manager->lock);
    return 1;
}

extern int
bwmHandleAnnounceTransaction (BRWalletManager manager,
                              int id,
                              OwnershipKept uint8_t *transaction,
                              size_t transactionLength,
                              uint64_t timestamp,
                              uint64_t blockHeight) {
    assert (eventHandlerIsCurrentThread (manager->handler));

    pthread_mutex_lock (&manager->lock);
    BRSyncManagerAnnounceGetTransactionsItem (manager->syncManager,
                                              id,
                                              transaction,
                                              transactionLength,
                                              timestamp,
                                              blockHeight);
    pthread_mutex_unlock (&manager->lock);
    return 1;
}

extern void
bwmHandleAnnounceTransactionComplete (BRWalletManager manager,
                                      int rid,
                                      int success) {
    assert (eventHandlerIsCurrentThread (manager->handler));

    pthread_mutex_lock (&manager->lock);
    BRSyncManagerAnnounceGetTransactionsDone (manager->syncManager,
                                              rid,
                                              success);
    pthread_mutex_unlock (&manager->lock);
}

extern void
bwmHandleAnnounceSubmit (BRWalletManager manager,
                         int rid,
                         UInt256 txHash,
                         int error) {
    assert (eventHandlerIsCurrentThread (manager->handler));

    pthread_mutex_lock (&manager->lock);
    BRTransactionWithState txnWithState = BRWalletManagerFindTransactionByHash (manager, txHash);
    if (NULL != txnWithState) {
        BRSyncManagerAnnounceSubmitTransaction (manager->syncManager,
                                                rid,
                                                BRTransactionWithStateGetOwned (txnWithState),
                                                error);
    }
    pthread_mutex_unlock (&manager->lock);
}

///
/// MARK: BRWalletManager Events
//

/**
 * These handlers are called by the event handler thread.
 */

extern void
bwmHandleWalletManagerEvent(BRWalletManager bwm,
                            BRWalletManagerEvent event) {
    assert (eventHandlerIsCurrentThread (bwm->handler) && NULL != bwm->client.funcWalletManagerEvent);
    bwm->client.funcWalletManagerEvent (bwm->client.context,
                                        bwm,
                                        event);
}

extern void
bwmHandleWalletEvent(BRWalletManager bwm,
                     BRWallet *wallet,
                     BRWalletEvent event) {
    assert (eventHandlerIsCurrentThread (bwm->handler) && NULL != bwm->client.funcWalletEvent);
    bwm->client.funcWalletEvent (bwm->client.context,
                                 bwm,
                                 wallet,
                                 event);
}

extern void
bwmHandleTransactionEvent(BRWalletManager bwm,
                          BRWallet *wallet,
                          BRTransaction *transaction,
                          BRTransactionEvent event) {
    assert (eventHandlerIsCurrentThread (bwm->handler) && NULL != bwm->client.funcTransactionEvent);
    bwm->client.funcTransactionEvent (bwm->client.context,
                                      bwm,
                                      wallet,
                                      transaction,
                                      event);
}

//
// Periodicaly query the BRD backend to get current status (block number, nonce, balances,
// transactions and logs) The event will be NULL (as specified for a 'period dispatcher' - See
// `eventHandlerSetTimeoutDispatcher()`). This is called by the event handler thread.
//
static void
bwmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event) {
    BRWalletManager bwm = (BRWalletManager) event->context;

    assert (eventHandlerIsCurrentThread (bwm->handler));
    pthread_mutex_lock (&bwm->lock);
    if (0 == bwm->sleepWakeupsForSyncTickTock % BWM_SYNC_AFTER_WAKEUPS) {
        // If BWM_SYNC_AFTER_WAKEUPS have occurred, then 'tick tock'.
        BRSyncManagerTickTock (bwm->syncManager);
    }
    else {
        // Otherwise, if in P2P mode and in a full scan, report on progress
        BRSyncManagerP2PFullScanReport(bwm->syncManager);
    }

    bwm->sleepWakeupsForSyncTickTock += 1;
    bwm->sleepWakeupsForSyncTickTock %= BWM_SYNC_AFTER_WAKEUPS;
    pthread_mutex_unlock (&bwm->lock);
}

extern BRFileService
BRWalletManagerCreateFileService (const BRChainParams *params,
                                  const char *storagePath,
                                  BRFileServiceContext context,
                                  BRFileServiceErrorHandler handler) {
    const char *networkName  = getNetworkName  (params);
    const char *currencyName = getCurrencyName (params);

    return fileServiceCreateFromTypeSpecfications (storagePath, currencyName, networkName,
                                                   context,
                                                   handler,
                                                   fileServiceSpecificationsCount,
                                                   fileServiceSpecifications);
}

extern void
BRWalletManagerExtractFileServiceTypes (BRFileService fileService,
                                        const char **transactions,
                                        const char **blocks,
                                        const char **peers) {
    if (NULL != transactions) *transactions = fileServiceTypeTransactions;
    if (NULL != blocks)       *blocks       = fileServiceTypeBlocks;
    if (NULL != peers)        *peers        = fileServiceTypePeers;
}

///
/// Mark: Event Helper Routines
///

extern const char *
BRWalletManagerEventTypeString (BRWalletManagerEventType t) {
    switch (t) {
        case BITCOIN_WALLET_MANAGER_CONNECTED:
        return "BITCOIN_WALLET_MANAGER_CONNECTED";

        case BITCOIN_WALLET_MANAGER_CREATED:
        return "BITCOIN_WALLET_MANAGER_CREATED";

        case BITCOIN_WALLET_MANAGER_DISCONNECTED:
        return "BITCOIN_WALLET_MANAGER_DISCONNECTED";

        case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
        return "BITCOIN_WALLET_MANAGER_SYNC_STARTED";

        case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
        return "BITCOIN_WALLET_MANAGER_SYNC_PROGRESS";

        case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
        return "BITCOIN_WALLET_MANAGER_SYNC_STOPPED";

        case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
        return "BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED";
    }
    return "<BITCOIN_WALLET_MANAGER_EVENT_TYPE_UNKNOWN>";
}

extern int
BRWalletManagerEventTypeIsValidPair (BRWalletManagerEventType t1, BRWalletManagerEventType t2) {
    int isValid = 0;
    switch (t1) {
        case BITCOIN_WALLET_MANAGER_CREATED:
            switch (t2) {
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_MANAGER_CONNECTED:
            switch (t2) {
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_MANAGER_DISCONNECTED:
            switch (t2) {
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
            switch (t2) {
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
            switch (t2) {
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
            switch (t2) {
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
            switch (t2) {
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
                isValid = 0;
                break;
            }
        break;
    }
    return isValid;
}

extern const char *
BRWalletEventTypeString (BRWalletEventType t) {
    switch (t) {
        case BITCOIN_WALLET_CREATED:
        return "BITCOIN_WALLET_CREATED";
        case BITCOIN_WALLET_BALANCE_UPDATED:
        return "BITCOIN_WALLET_BALANCE_UPDATED";
        case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
        return "BITCOIN_WALLET_TRANSACTION_SUBMITTED";
        case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
        return "BITCOIN_WALLET_FEE_PER_KB_UPDATED";
        case BITCOIN_WALLET_FEE_ESTIMATED:
        return "BITCOIN_WALLET_FEE_ESTIMATED";
        case BITCOIN_WALLET_DELETED:
        return "BITCOIN_WALLET_DELETED";
    }
    return "<BITCOIN_WALLET_EVENT_TYPE_UNKNOWN>";
}

extern int
BRWalletEventTypeIsValidPair (BRWalletEventType t1, BRWalletEventType t2) {
    int isValid = 0;
    switch (t1) {
        case BITCOIN_WALLET_CREATED:
            switch (t2) {
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
                case BITCOIN_WALLET_FEE_ESTIMATED:
                case BITCOIN_WALLET_DELETED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_CREATED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_BALANCE_UPDATED:
            switch (t2) {
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
                case BITCOIN_WALLET_FEE_ESTIMATED:
                case BITCOIN_WALLET_DELETED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_CREATED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
            switch (t2) {
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
                case BITCOIN_WALLET_FEE_ESTIMATED:
                case BITCOIN_WALLET_DELETED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_CREATED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
            switch (t2) {
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
                case BITCOIN_WALLET_FEE_ESTIMATED:
                case BITCOIN_WALLET_DELETED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_CREATED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_FEE_ESTIMATED:
            switch (t2) {
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
                case BITCOIN_WALLET_FEE_ESTIMATED:
                case BITCOIN_WALLET_DELETED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_CREATED:
                isValid = 0;
                break;
            }
        break;
        case BITCOIN_WALLET_DELETED:
            switch (t2) {
                case BITCOIN_WALLET_CREATED:
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                case BITCOIN_WALLET_FEE_PER_KB_UPDATED:
                case BITCOIN_WALLET_FEE_ESTIMATED:
                case BITCOIN_WALLET_DELETED:
                isValid = 0;
                break;
            }
        break;
    }
    return isValid;
}

extern const char *
BRTransactionEventTypeString (BRTransactionEventType t) {
    switch (t) {
        case BITCOIN_TRANSACTION_CREATED:
        return "BITCOIN_TRANSACTION_CREATED";
        case BITCOIN_TRANSACTION_SIGNED:
        return "BITCOIN_TRANSACTION_SIGNED";
        case BITCOIN_TRANSACTION_ADDED:
        return "BITCOIN_TRANSACTION_ADDED";
        case BITCOIN_TRANSACTION_UPDATED:
        return "BITCOIN_TRANSACTION_UPDATED";
        case BITCOIN_TRANSACTION_DELETED:
        return "BITCOIN_TRANSACTION_DELETED";
    }
    return "<BITCOIN_TRANSACTION_EVENT_TYPE_UNKNOWN>";
}
