//
//  BRWalletManager.c
//  BRCore
//
//  Created by Ed Gamble on 11/21/18.
//  Copyright © 2018-2019 Breadwallet AG. All rights reserved.
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

// default to TRUE in case client's don't bother updating this value
#define DEFAULT_NETWORK_IS_REACHABLE             (1)

#if defined (DEBUG)
#define static_on_release
#else
#define static_on_release   static
#endif

/* Forward Declarations */
static void
bwmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);

static void
bwmGenerateAddedEvents (BRWalletManager manager,
                        BRTransactionWithState txnWithState);

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

    // this should never happen!
    assert (0);
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

    // this should never happen!
    assert (0);
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
 * A Word About Ownership
 *
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
 *
 *
 * A Word About State
 *
 * Some of the other currency impls (ex: ethereum, generic crypto) have the concept of
 * transaction state that is persisted alongside the raw transaction bytes. We do not
 * track that information in the Bitcoin logic, to the same degree, as we are leveraging
 * the behaviour of the existing BRWallet code, as well as the behaviour of the
 * BRSyncManager/BRPeerManager. Specifically, we are leveraging the fact that a transaction
 * is only added to the wallet if it has been broadcast to the Bitcoin network (see:
 * usage of BRWalletRegisterTransaction).
 *
 * To understand why, here is a breakdown of each conceptual state present in the Generic
 * Crypto codebase:
 *
 *  - DELETED: Do not persist; by definition, we don't persist something that is deleted.
 *  - CREATED: Do not persist; no value to it. If a user wants to create many transactions
 *             but not submit them, this would clutter the persistent store.
 *  - SIGNED: Do not persist; no value to it. Like created, transactions that have yet to
 *            to be submitted don't need to be persisted.
 *  - ERRORED: Do not persist. This one is a tricky one. For now, the decision has been
 *             made not to persist these. One could argue that there is value in having
 *             a persistent record of transactions that have been attempted to submit but
 *             failed. Ultimately, in the interest of simplicity, these are NOT persisted as
 *             for both API and P2P mode, these will not be registered to the wallet and
 *             thus do not need to be stored for the wallet to be logically consistent on
 *             re-instantiation.
 *  - INCLUDED: Do persist!
 *  - SUBMITTED: Do persist!
 *
 * As you can see from the above possible states, only INCLUDED and SUBMITTED provide value.
 * Since both of these are derived from the existing BRTransaction fields (namely height and
 * tiemstamp), we do NOT need to persist a state field alongside the BRTransaction data.
 */

struct BRTransactionWithStateStruct {
    uint8_t isDeleted;
    uint8_t isResolved;
    BRTransaction *refedTransaction;
    BRTransaction *ownedTransaction;
};

static BRTransactionWithState BRTransactionWithStateNew(BRTransaction *ownedTransaction,
                                                        BRTransaction *refedTransaction) {
    BRTransactionWithState txnWithState = calloc (1, sizeof(struct BRTransactionWithStateStruct));
    txnWithState->isDeleted = 0;
    txnWithState->isResolved = 0;
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

static BRTransactionWithState BRTransactionWithStateSetResolved(BRTransactionWithState txnWithState) {
    txnWithState->isResolved = 1;
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
 * Find the tracked transaction that corresponds to the confirmed send with the highest
 * block height. Deleted transactions are not checked (i.e. they are skipped).
 */
static BRTransactionWithState
BRWalletManagerFindTransactionWithLastConfirmedSend(BRWalletManager manager,
                                                    uint64_t lastBlockHeight,
                                                    uint64_t confirmationsUntilFinal) {
    BRTransactionWithState txnWithState = NULL;

    if (lastBlockHeight >= confirmationsUntilFinal) {
        for (size_t index = 0; index < array_count (manager->transactions); index++) {
            // ensure:
            // - tx is not deleted
            // - tx is valid (i.e. no previous transaction spend any of utxos, and no inputs are invalid)
            // - AND the transaction was a SEND
            // - AND the transaction has been confirmed
            if (!manager->transactions[index]->isDeleted &&
                BRTransactionIsSigned (manager->transactions[index]->ownedTransaction) &&
                BRWalletTransactionIsValid (manager->wallet, manager->transactions[index]->ownedTransaction) &&
                0 != BRWalletAmountSentByTx (manager->wallet, manager->transactions[index]->ownedTransaction) &&
                TX_UNCONFIRMED != manager->transactions[index]->ownedTransaction->blockHeight &&
                manager->transactions[index]->ownedTransaction->blockHeight < (lastBlockHeight - confirmationsUntilFinal)) {

                if (NULL == txnWithState ||
                    txnWithState->ownedTransaction->blockHeight < manager->transactions[index]->ownedTransaction->blockHeight) {
                    txnWithState =  manager->transactions[index];
                }
            }
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
                                      UInt256 hash,
                                      int ignoreIsDeleted) {
    BRTransactionWithState txnWithState = NULL;

    for (size_t index = 0; index < array_count(manager->transactions); index++) {
        if ((ignoreIsDeleted || !manager->transactions[index]->isDeleted) &&
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
    if (bytesCount < (txTimestampSize + txBlockHeightSize)) return NULL;

    BRTransaction *transaction = BRTransactionParse (bytes, bytesCount - txTimestampSize - txBlockHeightSize);
    if (NULL == transaction) return NULL;

    transaction->blockHeight = UInt32GetLE (&bytes[bytesCount - txTimestampSize - txBlockHeightSize]);
    transaction->timestamp   = UInt32GetLE (&bytes[bytesCount - txTimestampSize]);

    return transaction;
}

static BRArrayOf(BRTransaction*)
initialTransactionsLoad (BRWalletManager manager) {
    BRSetOf(BRTransaction*) transactionSet = BRSetNew(BRTransactionHash, BRTransactionEq, 100);
    if (1 != fileServiceLoad (manager->fileService, transactionSet, fileServiceTypeTransactions, 1)) {
        BRSetFreeAll(transactionSet, (void (*) (void*)) BRTransactionFree);
        _peer_log ("BWM: failed to load transactions");
        return NULL;
    }

    size_t transactionsCount = BRSetCount(transactionSet);

    BRArrayOf(BRTransaction*) transactions;
    array_new (transactions, transactionsCount);
    array_set_count(transactions, transactionsCount);

    BRSetAll(transactionSet, (void**) transactions, transactionsCount);
    BRSetFree(transactionSet);

    _peer_log ("BWM: loaded %zu transactions", transactionsCount);
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
    if (bytesCount < blockHeightSize) return NULL;

    BRMerkleBlock *block = BRMerkleBlockParse (bytes, bytesCount - blockHeightSize);
    if (NULL == block) return NULL;

    block->height = UInt32GetLE(&bytes[bytesCount - blockHeightSize]);

    return block;
}

static BRArrayOf(BRMerkleBlock*)
initialBlocksLoad (BRWalletManager manager) {
    BRSetOf(BRMerkleBlock*) blockSet = BRSetNew(BRMerkleBlockHash, BRMerkleBlockEq, 100);
    if (1 != fileServiceLoad (manager->fileService, blockSet, fileServiceTypeBlocks, 1)) {
        BRSetFreeAll(blockSet, (void (*) (void*)) BRMerkleBlockFree);
        _peer_log ("BWM: failed to load blocks");
        return NULL;
    }

    size_t blocksCount = BRSetCount(blockSet);

    BRArrayOf(BRMerkleBlock*) blocks;
    array_new (blocks, blocksCount);
    array_set_count(blocks, blocksCount);

    BRSetAll(blockSet, (void**) blocks, blocksCount);
    BRSetFree(blockSet);

    _peer_log ("BWM: loaded %zu blocks", blocksCount);
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
    size_t offset = 0;

    *bytesCount = sizeof (BRPeer);
    uint8_t *bytes = malloc (*bytesCount);

    memcpy (&bytes[offset], peer->address.u8, sizeof (UInt128));
    offset += sizeof (UInt128);

    UInt16SetBE (&bytes[offset], peer->port);
    offset += sizeof (uint16_t);

    UInt64SetBE (&bytes[offset], peer->services);
    offset += sizeof (uint64_t);

    UInt64SetBE (&bytes[offset], peer->timestamp);
    offset += sizeof (uint64_t);

    bytes[offset] = peer->flags;
    offset += sizeof(uint8_t); (void) offset;

    return bytes;
}

static void *
fileServiceTypePeerV1Reader (BRFileServiceContext context,
                             BRFileService fs,
                             uint8_t *bytes,
                             uint32_t bytesCount) {
    assert (bytesCount == sizeof (BRPeer));

    size_t offset = 0;

    BRPeer *peer = malloc (bytesCount);

    memcpy (peer->address.u8, &bytes[offset], sizeof (UInt128));
    offset += sizeof (UInt128);

    peer->port = UInt16GetBE (&bytes[offset]);
    offset += sizeof (uint16_t);

    peer->services = UInt64GetBE(&bytes[offset]);
    offset += sizeof (uint64_t);

    peer->timestamp = UInt64GetBE(&bytes[offset]);
    offset += sizeof (uint64_t);

    peer->flags = bytes[offset];
    offset += sizeof(uint8_t); (void) offset;

    return peer;
}

static BRArrayOf(BRPeer)
initialPeersLoad (BRWalletManager manager) {
    /// Load peers for the wallet manager.
    BRSetOf(BRPeer*) peerSet = BRSetNew(BRPeerHash, BRPeerEq, 100);
    if (1 != fileServiceLoad (manager->fileService, peerSet, fileServiceTypePeers, 1)) {
        BRSetFreeAll(peerSet, free);
        _peer_log ("BWM: failed to load peers");
        return NULL;
    }

    size_t peersCount = BRSetCount(peerSet);

    BRArrayOf(BRPeer) peers;
    array_new (peers, peersCount);

    FOR_SET (BRPeer*, peer, peerSet) array_add (peers, *peer);
    BRSetFreeAll(peerSet, free);

    _peer_log ("BWM: loaded %zu peers", peersCount);
    return peers;
}

static void
bwmFileServiceErrorHandler (BRFileServiceContext context,
                            BRFileService fs,
                            BRFileServiceError error) {
    switch (error.type) {
        case FILE_SERVICE_IMPL:
            // This actually a FATAL - an unresolvable coding error.
            _peer_log ("BWM: FileService Error: IMPL: %s", error.u.impl.reason);
            break;
        case FILE_SERVICE_UNIX:
            _peer_log ("BWM: FileService Error: UNIX: %s", strerror(error.u.unix.error));
            break;
        case FILE_SERVICE_ENTITY:
            // This is likely a coding error too.
            _peer_log ("BWM: FileService Error: ENTITY (%s): %s",
                     error.u.entity.type,
                     error.u.entity.reason);
            break;
        case FILE_SERVICE_SDB:
            _peer_log ("BWM: FileService Error: SDB: (%d): %s",
                       error.u.sdb.code,
                       error.u.sdb.reason);
            break;
    }
    _peer_log ("BWM: FileService Error: FORCED SYNC%s", "");

    // BRWalletManager bwm = (BRWalletManager) context;
    // TODO(fix): What do we actually want to happen here?
    // if (NULL != bwm->peerManager)
    //     BRPeerManagerRescan (bwm->peerManager);
}

static_on_release BRFileServiceTypeSpecification fileServiceSpecifications[] = {
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
static_on_release size_t fileServiceSpecificationsCount = (sizeof (fileServiceSpecifications) / sizeof (BRFileServiceTypeSpecification));



/// MARK: - Wallet Manager

static BRWalletManager
bwmCreateErrorHandler (BRWalletManager bwm, int fileService, const char* reason) {
    if (fileService) {
        _peer_log ("BWM: on bwmCreate: FileService Error: %s", reason);
    } else {
        _peer_log ("BWM: on bwmCreate: Error: %s", reason);
    }

    if (NULL != bwm) {
        if (NULL != bwm->handler) {
            eventHandlerDestroy (bwm->handler);
        }

        if (NULL != bwm->fileService) {
            fileServiceRelease (bwm->fileService);
        }

        if (NULL != bwm->syncManager) {
            BRSyncManagerFree (bwm->syncManager);
        }

        if (NULL != bwm->transactions) {
            BRWalletManagerFreeTransactions (bwm);
        }

        if (NULL != bwm->wallet) {
            BRWalletFree (bwm->wallet);
        }

        pthread_mutex_destroy (&bwm->lock);

        memset (bwm, 0, sizeof(*bwm));
        free (bwm);
    }

    return NULL;
}

extern BRWalletManager
BRWalletManagerNew (BRWalletManagerClient client,
                    BRMasterPubKey mpk,
                    const BRChainParams *params,
                    uint32_t earliestKeyTime,
                    BRCryptoSyncMode mode,
                    const char *baseStoragePath,
                    uint64_t blockHeight,
                    uint64_t confirmationsUntilFinal) {
    assert (mode == CRYPTO_SYNC_MODE_API_ONLY || CRYPTO_SYNC_MODE_P2P_ONLY);

    BRWalletManager bwm = calloc (1, sizeof (struct BRWalletManagerStruct));
    if (NULL == bwm) {
        return bwmCreateErrorHandler (NULL, 0, "allocate");
    }

    bwm->mode = mode;
    bwm->client = client;
    bwm->chainParams = params;
    bwm->earliestKeyTime = earliestKeyTime;
    bwm->sleepWakeupsForSyncTickTock = 0;

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
    const char *networkName  = getNetworkName  (params);
    const char *currencyName = getCurrencyName (params);
    bwm->fileService = fileServiceCreateFromTypeSpecfications (baseStoragePath, currencyName, networkName,
                                                               bwm,
                                                               bwmFileServiceErrorHandler,
                                                               fileServiceSpecificationsCount,
                                                               fileServiceSpecifications);
    if (NULL == bwm->fileService) {
        return bwmCreateErrorHandler (bwm, 1, "create");
    }

    /// Load transactions for the wallet manager.
    BRArrayOf(BRTransaction*) transactions = initialTransactionsLoad(bwm);
    /// Load blocks and peers for the peer manager.
    BRArrayOf(BRMerkleBlock*) blocks = initialBlocksLoad(bwm);
    BRArrayOf(BRPeer) peers = initialPeersLoad(bwm);

    // If any of these are NULL, then there was a failure; on a failure they all need to be cleared
    // which will cause a *FULL SYNC*
    if (NULL == transactions || NULL == blocks || NULL == peers) {
        if (NULL != transactions) array_free_all(transactions, BRTransactionFree);
        array_new (transactions, 1);

        if (NULL != blocks) array_free_all(blocks, BRMerkleBlockFree);
        array_new (blocks, 1);

        if (NULL != peers) array_free(peers);
        array_new (peers, 1);
    }

    // Create the transaction array with enough initial capacity to hold all the loaded transactions
    array_new(bwm->transactions, array_count(transactions));

    // Create the Wallet being managed and populate with the loaded transactions
    _peer_log ("BWM: initializing wallet with %zu transactions", array_count(transactions));
    bwm->wallet = BRWalletNew (params->addrParams, transactions, array_count(transactions), mpk);
    if (NULL == bwm->wallet) {
        array_free(transactions); array_free(blocks); array_free(peers);
        return bwmCreateErrorHandler (bwm, 0, "wallet");
    }

    // Set the callbacks if the wallet has been created successfully
    BRWalletSetCallbacks (bwm->wallet, bwm,
                          _BRWalletManagerBalanceChanged,
                          _BRWalletManagerTxAdded,
                          _BRWalletManagerTxUpdated,
                          _BRWalletManagerTxDeleted);

    // Create the SyncManager responsible for interacting with the P2P network or delegating to a client
    // for retrieving blockchain data
    _peer_log ("BWM: initializing sync manager with %zu blocks and %zu peers",
               array_count(blocks), array_count(peers));
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
                                                confirmationsUntilFinal,
                                                DEFAULT_NETWORK_IS_REACHABLE,
                                                blocks, array_count(blocks),
                                                peers,  array_count(peers));

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

    // Populate the WalletManager's list transaction based on what the Wallet contains
    // See the comment in section "Transaction Tracking" for more details on how this
    // operates.
    //
    // For each of these initially loaded transactions, add an event indicating that
    // the transaction was added and updated with its corresponding height and timestamp.

    size_t txCount = BRWalletTransactions (bwm->wallet, NULL, 0);
    if (txCount) {
        BRArrayOf(BRTransaction *) txns = calloc (txCount, sizeof(BRTransaction*));
        // Fills txns with direct references to BRWallet transactions; caution warranted.
        BRWalletTransactions (bwm->wallet, txns, txCount);

        for (size_t i = 0; i < txCount; i++) {
            // Create a new wallet-based transaction. No lock is held here because we
            // are still in the "ctor" and callbacks are not being made until the
            // event handler is started in a later call.
            BRTransactionWithState txnWithState = BRWalletManagerAddTransaction (bwm,
                                                                                 BRTransactionCopy (txns[i]),
                                                                                 txns[i]);
            if (BRWalletTransactionIsResolved (bwm->wallet, txnWithState->ownedTransaction))
                BRTransactionWithStateSetResolved (txnWithState);

            if (txnWithState->isResolved)
                bwmGenerateAddedEvents (bwm, txnWithState);
        }

        free (txns);
    }

    // Add bwmPeriodicDispatcher to handlerForMain.  Note that a 'timeout' is handled by
    // an OOB (out-of-band) event whereby the event is pushed to the front of the queue.
    // This may not be the right thing to do.  Imagine that BWM is blocked somehow (doing
    // a time consuming calculation) and two 'timeout events' occur - the events will be
    // queued in the wrong order (second before first).
    //
    // The function `bwmPeriodicDispatcher()` will be installed as a periodic alarm
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
    fileServiceClose (manager->fileService);
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
BRWalletManagerSetFixedPeer (BRWalletManager manager,
                             UInt128 address,
                             uint16_t port) {
    pthread_mutex_lock (&manager->lock);
    BRSyncManagerSetFixedPeer (manager->syncManager, address, port);
    pthread_mutex_unlock (&manager->lock);
}

extern void
BRWalletManagerWipe (const BRChainParams *params,
                      const char *baseStoragePath) {
    const char *networkName  = getNetworkName  (params);
    const char *currencyName = getCurrencyName (params);
    fileServiceWipe (baseStoragePath, currencyName, networkName);
}

extern void
BRWalletManagerScan (BRWalletManager manager) {
    BRWalletManagerScanToDepth (manager, CRYPTO_SYNC_DEPTH_FROM_CREATION);
}

extern void
BRWalletManagerScanToDepth (BRWalletManager manager,
                            BRCryptoSyncDepth depth) {
    // The BRSyncManager has no safe way to get transactions (BRWalletTransactions isn't safe as one of
    // the returned transactions could be deleted at any moment). To work around that fact, and the fact
    // that the BRWalletManager needs to know the block height of the last confirmed send when the mode
    // is CRYPTO_SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND, get the last transaction up front (if available).

    pthread_mutex_lock (&manager->lock);

    BRTransaction *lastConfirmedSendTxn = NULL;
    if (CRYPTO_SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND == depth) {
        uint64_t lastBlockHeight = BRSyncManagerGetBlockHeight (manager->syncManager);
        uint64_t confirmationsUntilFinal = BRSyncManagerGetConfirmationsUntilFinal (manager->syncManager);
        BRTransactionWithState txnWithState = BRWalletManagerFindTransactionWithLastConfirmedSend (manager,
                                                                                                   lastBlockHeight,
                                                                                                   confirmationsUntilFinal);

        lastConfirmedSendTxn = NULL == txnWithState ? NULL : BRTransactionWithStateGetOwned (txnWithState);
    }

    BRSyncManagerScanToDepth (manager->syncManager, depth, lastConfirmedSendTxn);

    pthread_mutex_unlock (&manager->lock);
}

extern void
BRWalletManagerSetMode (BRWalletManager manager, BRCryptoSyncMode mode) {
    pthread_mutex_lock (&manager->lock);
    if (mode != manager->mode) {
        // get the sync manager values that need to be preserved on mode change
        uint64_t blockHeight = BRSyncManagerGetBlockHeight (manager->syncManager);
        uint64_t confirmationsUntilFinal = BRSyncManagerGetConfirmationsUntilFinal (manager->syncManager);
        int isNetworkReachable = BRSyncManagerGetNetworkReachable (manager->syncManager);

        // kill the sync manager to prevent any additional callbacks from occuring
        BRSyncManagerDisconnect (manager->syncManager);
        BRSyncManagerFree (manager->syncManager);

        // load up the blocks/peers saved to disk
        BRArrayOf(BRMerkleBlock*) blocks = initialBlocksLoad(manager);
        BRArrayOf(BRPeer) peers = initialPeersLoad(manager);

        // If any of these are NULL, then there was a failure; on a failure they all need to be cleared
        // which will cause a *FULL SYNC*
        if (NULL == blocks || NULL == peers) {
            if (NULL != blocks) array_free_all(blocks, BRMerkleBlockFree);
            array_new (blocks, 1);

            if (NULL != peers) array_free(peers);
            array_new (peers, 1);
        }

        // set the new mode
        manager->mode = mode;

        // create the new sync manager
        _peer_log ("BWM: initializing sync manager with %zu blocks and %zu peers",
                   array_count(blocks), array_count(peers));
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
                                                        confirmationsUntilFinal,
                                                        isNetworkReachable,
                                                        blocks, array_count (blocks),
                                                        peers, array_count (peers));

        // No longer need the loaded blocks/peers
        array_free(blocks); array_free(peers);
    }
    pthread_mutex_unlock (&manager->lock);
}

extern BRCryptoSyncMode
BRWalletManagerGetMode (BRWalletManager manager) {
    pthread_mutex_lock (&manager->lock);
    BRCryptoSyncMode mode = manager->mode;
    pthread_mutex_unlock (&manager->lock);
    return mode;
}

extern void
BRWalletManagerSetNetworkReachable (BRWalletManager manager,
                                    int isNetworkReachable) {
    pthread_mutex_lock (&manager->lock);
    BRSyncManagerSetNetworkReachable (manager->syncManager, isNetworkReachable);
    pthread_mutex_unlock (&manager->lock);
}

extern BRTransaction *
BRWalletManagerCreateTransaction (BRWalletManager manager,
                                  BRWallet *wallet,
                                  uint64_t amount,
                                  BRAddress addr,
                                  uint64_t feePerKb) {
    assert (wallet == manager->wallet);

    pthread_mutex_lock (&manager->lock);
    BRTransaction *transaction = BRWalletCreateTransactionWithFeePerKb (wallet, feePerKb, amount, addr.s);
    BRTransactionWithState txnWithState = (NULL != transaction) ? BRWalletManagerAddTransaction (manager, transaction, NULL) : NULL;

    BRTransaction *ownedTransaction = NULL;
    if (NULL != txnWithState) {
        BRTransactionWithStateSetResolved (txnWithState);  // Always resolved if created
        ownedTransaction = BRTransactionWithStateGetOwned (txnWithState);
    }
    pthread_mutex_unlock (&manager->lock);

    if (NULL != ownedTransaction)
        bwmSignalTransactionEvent(manager,
                                  wallet,
                                  ownedTransaction,
                                  (BRTransactionEvent) {
                                      BITCOIN_TRANSACTION_CREATED
                                  });

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

    BRTransaction *ownedTransaction = NULL;
    if (NULL != txnWithState) {
        BRTransactionWithStateSetResolved (txnWithState);  // Always resolved if created
        ownedTransaction = BRTransactionWithStateGetOwned (txnWithState);
    }
    pthread_mutex_unlock (&manager->lock);

    if (NULL != ownedTransaction)
        bwmSignalTransactionEvent(manager,
                                  wallet,
                                  ownedTransaction,
                                  (BRTransactionEvent) {
                                      BITCOIN_TRANSACTION_CREATED
                                  });

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
    BRTransaction *transaction = BRWalletCreateTxForOutputsWithFeePerKb (wallet, feePerKb, outputs, outputsLen);
    BRTransactionWithState txnWithState = (NULL != transaction) ? BRWalletManagerAddTransaction (manager, transaction, NULL) : NULL;

    BRTransaction *ownedTransaction = NULL;
    if (NULL != txnWithState) {
        BRTransactionWithStateSetResolved (txnWithState);  // Always resolved if created
        ownedTransaction = BRTransactionWithStateGetOwned (txnWithState);
    }
    pthread_mutex_unlock (&manager->lock);

    if (NULL != ownedTransaction)
        bwmSignalTransactionEvent(manager,
                                  wallet,
                                  ownedTransaction,
                                  (BRTransactionEvent) {
                                      BITCOIN_TRANSACTION_CREATED
                                  });

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
    BRTransaction *ownedTransaction = NULL == txnWithState ? NULL : BRTransactionWithStateGetOwned (txnWithState);
    pthread_mutex_unlock (&manager->lock);

    int success = 0;
    if (NULL != ownedTransaction &&
        1 == BRWalletSignTransaction (wallet,
                                      ownedTransaction,
                                      manager->chainParams->forkId,
                                      seed,
                                      seedLen)) {
        success = 1;
        bwmSignalTransactionEvent(manager,
                                  wallet,
                                  ownedTransaction,
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
    BRTransaction *ownedTransaction = NULL == txnWithState ? NULL : BRTransactionWithStateGetOwned (txnWithState);
    pthread_mutex_unlock (&manager->lock);

    int success = 0;
    if (NULL != ownedTransaction &&
        1 == BRTransactionSign (ownedTransaction,
                                manager->chainParams->forkId,
                                key,
                                1)) {
        success = 1;
        bwmSignalTransactionEvent(manager,
                                  wallet,
                                  ownedTransaction,
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
    uint64_t fee  = (0 == transferAmount ? 0 : BRWalletFeeForTxAmountWithFeePerKb (wallet, feePerKb, transferAmount));
    uint32_t sizeInByte = (uint32_t) ((1000 * fee)/ feePerKb);
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
    BRTransaction *transaction = BRWalletCreateTxForOutputsWithFeePerKb (wallet, feePerKb, outputs, outputsLen);

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
 * These callbacks come from the BRWallet. That component has no inherent thread mode of its own. As
 * such, these callbacks can occur on any thread. That includes that of the caller that
 * triggered the callback, as well as the threads created for each BRPeer created by the
 * BRPeerManager (by way of the BRSyncManager).
 *
 * As an example, `BRWalletUpdateTransactions` is called by `_peerRelayedBlock`. That
 * `_peerRelayedBlock` function holds the BRPeerManager's lock and `BRWalletUpdateTransactions`
 * will call into `_BRWalletManagerTxUpdated`. If we were to acquire the BRWalletManager::lock
 * in `_BRWalletManagerTxUpdated`, we would have a lock inversion situation (the lock flow
 * should ONLY be BRWalletManager::lock -> BRPeerManager::lock, NEVER BRPeerManager::lock
 * -> BRWalletManager::lock).
 *
 * It is hypothesized that the other callbacks can occur way, via the BRPeerManager. Thus
 * these callbacks avoid taking the BRWalletManager::lock, to be safe.
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

    bwmSignalTxDeleted (manager, hash, recommendRescan);
}

/// MARK: Wallet Callback Event Handlers

/**
 * These handlers are called by the event handler thread. They are free to acquire
 * locks as needed.
 */
static void
bwmGenerateAddedEvents (BRWalletManager manager,
                        BRTransactionWithState txnWithState) {
    bwmSignalTransactionEvent(manager,
                              manager->wallet,
                              BRTransactionWithStateGetOwned (txnWithState),
                              (BRTransactionEvent) {
        BITCOIN_TRANSACTION_ADDED
    });

    bwmSignalTransactionEvent (manager,
                               manager->wallet,
                               BRTransactionWithStateGetOwned (txnWithState),
                               (BRTransactionEvent) {
        BITCOIN_TRANSACTION_UPDATED,
        { .updated = {
            BRTransactionWithStateGetOwned (txnWithState)->blockHeight,
            BRTransactionWithStateGetOwned (txnWithState)->timestamp }}
    });
}

extern void
bwmHandleTxAdded (BRWalletManager manager,
                  OwnershipGiven BRTransaction *ownedTransaction,
                  OwnershipKept BRTransaction *refedTransaction) {
    // RefedTransaction and OwnTransaction are guaranteed identical; don't access the
    // content of refedTransaction; it is OwnershipKept and could be gone by now.

    pthread_mutex_lock (&manager->lock);
    int needEvents = 1;

    // Find the transaction by `hash` but in the lookup ignore the deleted flag.
    BRTransactionWithState txnWithState = BRWalletManagerFindTransactionByHash (manager, ownedTransaction->txHash, 1);
    if (NULL == txnWithState) {
        // first we've seen it, so it came from the network; add it to our list
        txnWithState = BRWalletManagerAddTransaction (manager, ownedTransaction, refedTransaction);
        if (BRWalletTransactionIsResolved (manager->wallet, txnWithState->ownedTransaction)) {
            BRTransactionWithStateSetResolved (txnWithState);
        }
    } else {
        if (txnWithState->isDeleted) {
            // We've seen it before but has already been deleted, somewhow?  We are quietly going
            // to skip out and avoid signalling any events. Perhaps should assert(0) here.
            needEvents = 0;
        }
        else {
            // this is a transaction we've submitted; set the reference transaction from the wallet
            BRTransactionWithStateSetReferenced (txnWithState, refedTransaction);
            BRTransactionWithStateSetBlock (txnWithState, ownedTransaction->blockHeight, ownedTransaction->timestamp);
        }

        // we already have an owned copy of this transaction; free up the passed one
        BRTransactionFree (ownedTransaction);
    }
    assert (NULL != txnWithState);

    // Check 'IsResolved' based on the 'old' ownedTransaction; resolution does not depend on
    // any fields that might have changed in the ownedTransaction argument to this function.  Thus
    // is is okay to use `txnWithState->ownedTransaction`
    if (BRWalletTransactionIsResolved (manager->wallet, txnWithState->ownedTransaction))
        BRTransactionWithStateSetResolved (txnWithState);

    // Find other transactions that are now resolved.

    size_t transactionsCount = array_count (manager->transactions);

    size_t resolvedTransactionIndex = 0;
    BRTransactionWithState *resolvedTransactions = calloc (transactionsCount, sizeof (BRTransactionWithState*));

    for (size_t index = 0; index < transactionsCount; index++) {
        BRTransactionWithState txnWithState  = manager->transactions[index];
        uint8_t nowResolved = BRWalletTransactionIsResolved (manager->wallet, txnWithState->ownedTransaction);

        if (!txnWithState->isResolved && nowResolved) {
            BRTransactionWithStateSetResolved (txnWithState);
            resolvedTransactions[resolvedTransactionIndex++] = txnWithState;
        }
    }

    pthread_mutex_unlock (&manager->lock);

    if (txnWithState->isResolved && needEvents)
        bwmGenerateAddedEvents (manager, txnWithState);

    for (size_t index = 0; index < resolvedTransactionIndex; index++)
        bwmGenerateAddedEvents (manager, resolvedTransactions[index]);

    free (resolvedTransactions);
}

extern void
bwmHandleTxUpdated (BRWalletManager manager,
                    UInt256 hash,
                    uint32_t blockHeight,
                    uint32_t timestamp) {
    pthread_mutex_lock (&manager->lock);

    // We anticapte a case whereby a transaction has seen `txDeleted` but then `txUpdated`.  At
    // least we postulate such a case because IOS crashes seem to indicate that it has occurred.
    //
    // A txDeleted is currently only produced in a BRSyncMode of P2P; but this might change as
    // BlockSet deletes transactions from the mempool.
    int needEvents = 1;

    // Find the transaction by `hash` but in the lookup ignore the deleted flag.
    BRTransactionWithState txnWithState = BRWalletManagerFindTransactionByHash (manager, hash, 1);

    // A transaction must have been found, even if it is flagged as deleted.
    assert (NULL != txnWithState);

    // The transaction must be signed to be updated
    assert (BRTransactionIsSigned (BRTransactionWithStateGetOwned (txnWithState)));

    // If the transaction is deleted.... we'll avoid setting the block and the subsequent event.
    if (txnWithState->isDeleted) needEvents = 0;
    else {
        BRTransactionWithStateSetBlock (txnWithState, blockHeight, timestamp);
    }
    pthread_mutex_unlock (&manager->lock);

    if (txnWithState->isResolved && needEvents) {
        bwmSignalTransactionEvent(manager,
                                  manager->wallet,
                                  BRTransactionWithStateGetOwned (txnWithState),
                                  (BRTransactionEvent) {
            BITCOIN_TRANSACTION_UPDATED,
            { .updated = { blockHeight, timestamp }}
        });
    }
}

extern void
bwmHandleTxDeleted (BRWalletManager manager,
                    UInt256 hash,
                    int recommendRescan) {
    pthread_mutex_lock (&manager->lock);

    // We anticapte a case whereby a transaction has seen `txDeleted` but then `txDeleted` again.
    // At least we postulate such a case because IOS crashes seem to indicate that it has occurred.
    //
    // See comment above in `bwmHandleTxUpdated()`
    int needEvents = 1;

    // Find the transaction by `hash` but in the lookup ignore the deleted flag.
    BRTransactionWithState txnWithState = BRWalletManagerFindTransactionByHash (manager, hash, 1);

    // A transaction must have been found, even if it is flagged as deleted.
    assert (NULL != txnWithState);

    // The transaction must be signed to be deleted
    assert (BRTransactionIsSigned (BRTransactionWithStateGetOwned (txnWithState)));

    // If the transaction is deleted.... we'll avoid setting the deleted flag (again) and also
    // avoid the subsequent events.
    if (txnWithState->isDeleted) needEvents = 0;
    else {
        BRTransactionWithStateSetDeleted (txnWithState);
    }
    pthread_mutex_unlock (&manager->lock);

    if (needEvents) {
        if (txnWithState->isResolved)
            bwmSignalTransactionEvent(manager,
                                      manager->wallet,
                                      BRTransactionWithStateGetOwned (txnWithState),
                                      (BRTransactionEvent) {
                BITCOIN_TRANSACTION_DELETED
            });

        if (recommendRescan) {
            // When that happens it's because the wallet believes its missing spend tx, causing new tx to get
            // rejected as a double spend because of how the input selection works. You should only have to
            // scan from the most recent successful spend.
            bwmSignalWalletManagerEvent(manager,
                                        (BRWalletManagerEvent) {
                BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED,
                { .syncRecommended = { CRYPTO_SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND } }
            });
        }
    }
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
            fileServiceReplace (bwm->fileService, fileServiceTypeBlocks,
                                (const void **) event.u.blocks.blocks,
                                event.u.blocks.count);
            break;
        }
        case SYNC_MANAGER_ADD_BLOCKS: {
            // filesystem changes are NOT queued; they are acted upon immediately
            for (size_t index = 0; index < event.u.blocks.count; index++)
                fileServiceSave (bwm->fileService, fileServiceTypeBlocks, event.u.blocks.blocks[index]);
            break;
        }
        case SYNC_MANAGER_SET_PEERS: {
            // filesystem changes are NOT queued; they are acted upon immediately
            if (0 == event.u.peers.count) {
                // no peers to set, just do a clear
                fileServiceClear (bwm->fileService, fileServiceTypePeers);
            } else {
                // fileServiceReplace expects an array of pointers to entities, instead of an array of
                // structures so let's do the conversion here
                BRPeer **peers = calloc (event.u.peers.count,
                                         sizeof(BRPeer *));

                for (size_t i = 0; i < event.u.peers.count; i++) {
                    peers[i] = &event.u.peers.peers[i];
                }

                fileServiceReplace (bwm->fileService, fileServiceTypePeers,
                                    (const void **) peers,
                                    event.u.peers.count);
                free (peers);
            }
            break;
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
                                            BITCOIN_WALLET_MANAGER_DISCONNECTED,
                                            { .disconnected = { event.u.disconnected.reason } }
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
                                            { .syncStopped = { event.u.syncStopped.reason } }
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

        case SYNC_MANAGER_TXN_SUBMIT_SUCCEEDED: {
            bwmSignalWalletEvent(bwm,
                                 bwm->wallet,
                                 (BRWalletEvent) {
                                     BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED,
                                     { .submitSucceeded = { event.u.submitSucceeded.transaction }}
                                 });
            break;
        }

        case SYNC_MANAGER_TXN_SUBMIT_FAILED: {
            bwmSignalWalletEvent(bwm,
                                 bwm->wallet,
                                 (BRWalletEvent) {
                                     BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED,
                                     { .submitFailed = { event.u.submitFailed.transaction, event.u.submitFailed.error }}
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

static void
_BRWalletManagerGetBlockNumber(void * context,
                               BRSyncManager manager,
                               int rid) {
    BRWalletManager bwm = (BRWalletManager) context;

    assert  (NULL != bwm->client.funcGetBlockNumber);
    bwm->client.funcGetBlockNumber (bwm->client.context,
                                    bwm,
                                    rid);
}

static void
_BRWalletManagerGetTransactions(void * context,
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
 * model of a client is unknown. They are free to acquire locks as needed.
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

///
/// MARK: BRWalletManagerClient Completion Event Handlers
//

/**
 * These handlers are called by the event handler thread. They are free to acquire
 * locks as needed.
 */

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

    // We'll lookup based on `txHash` while ignoring if the transaction has been deleted.
    BRTransactionWithState txnWithState = BRWalletManagerFindTransactionByHash (manager, txHash, 1);
    if (NULL != txnWithState) {
        // Can't possibly have been deleted.
        assert (!txnWithState->isDeleted);

        // Do the actual submit via the API or P2P interface.
        BRSyncManagerAnnounceSubmitTransaction (manager->syncManager,
                                                rid,
                                                BRTransactionWithStateGetOwned (txnWithState),
                                                error);
    }
    pthread_mutex_unlock (&manager->lock);
}

///
/// MARK: BRWalletManager Event Handlers
//

/**
 * These handlers are called by the event handler thread. They are free to acquire
 * locks as needed.
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

        case BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED:
        return "BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED";

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
                case BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED:
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
                case BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED:
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
                case BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED:
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
                case BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED:
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
                case BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED:
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
                case BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED:
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
        case BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED:
            switch (t2) {
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS:
                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                case BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED:
                isValid = 1;
                break;

                case BITCOIN_WALLET_MANAGER_CREATED:
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
                case BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED:
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
        case BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED:
        return "BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED";
        case BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED:
        return "BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED";
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
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED:
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED:
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
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED:
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED:
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
        case BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED:
            switch (t2) {
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED:
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED:
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
        case BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED:
            switch (t2) {
                case BITCOIN_WALLET_BALANCE_UPDATED:
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED:
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED:
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
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED:
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED:
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
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED:
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED:
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
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED:
                case BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED:
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
