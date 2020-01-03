//
//  BREthereumEWM
//  Core Ethereum
//
//  Created by Ed Gamble on 3/5/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "support/BRArray.h"
#include "support/BRBIP39Mnemonic.h"
#include "support/BRAssert.h"
#include "ethereum/event/BREvent.h"
#include "ethereum/event/BREventAlarm.h"
#include "BREthereumEWMPrivate.h"

#define EWM_SLEEP_SECONDS (10)

// When using a BRD sync, offset the start block by N days of Ethereum blocks
#define EWM_BRD_SYNC_START_BLOCK_OFFSET        (3 * 24 * 60 * 4)   /* 4 per minute (every 15 seconds) */

// An ongoing sync is one that has a `end - beg` block difference of
// EWM_BRD_SYNC_START_BLOCK_OFFSET or so (with some slop).  If the block difference is large
// enough we'll transition to an EWM state of SYNCING; otherwise we'll consider the sync as an
// ongoing sync (as when the blockchain is extended).
static int
ewmIsNotAnOngoingSync (BREthereumEWM ewm) {
    return ewm->brdSync.endBlockNumber - ewm->brdSync.begBlockNumber >
    (EWM_BRD_SYNC_START_BLOCK_OFFSET + EWM_BRD_SYNC_START_BLOCK_OFFSET/10);
}


#define EWM_INITIAL_SET_SIZE_DEFAULT         (25)

/* Forward Declaration */
static void
ewmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);

/* Forward Implementation */

/// MARK: - File Service, Initial Load

static BRSetOf(BREthereumTransaction)
initialTransactionsLoad (BREthereumEWM ewm) {
    BRSetOf(BREthereumTransaction) transactions = BRSetNew(transactionHashValue, transactionHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);
    if (NULL != transactions && 1 != fileServiceLoad (ewm->fs, transactions, ewmFileServiceTypeTransactions, 1)) {
        BRSetFreeAll (transactions, (void (*) (void*)) transactionRelease);
        return NULL;
    }
    return transactions;
}

static BRSetOf(BREthereumLog)
initialLogsLoad (BREthereumEWM ewm) {
    BRSetOf(BREthereumLog) logs = BRSetNew(logHashValue, logHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);
    if (NULL != logs && 1 != fileServiceLoad (ewm->fs, logs, ewmFileServiceTypeLogs, 1)) {
        BRSetFreeAll (logs, (void (*) (void*)) logRelease);
        return NULL;
    }
    return logs;
}


static BRSetOf(BREthereumBlock)
initialBlocksLoad (BREthereumEWM ewm) {
    BRSetOf(BREthereumBlock) blocks = BRSetNew(blockHashValue, blockHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);
    if (NULL != blocks && 1 != fileServiceLoad (ewm->fs, blocks, ewmFileServiceTypeBlocks, 1)) {
        BRSetFreeAll (blocks,  (void (*) (void*)) blockRelease);
        return NULL;
    }
    return blocks;
}

static BRSetOf(BREthereumNodeConfig)
initialNodesLoad (BREthereumEWM ewm) {
    BRSetOf(BREthereumNodeConfig) nodes = BRSetNew(nodeConfigHashValue, nodeConfigHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);
    if (NULL != nodes && 1 != fileServiceLoad (ewm->fs, nodes, ewmFileServiceTypeNodes, 1)) {
        BRSetFreeAll (nodes, (void (*) (void*)) nodeConfigRelease);
        return NULL;
    }
    return nodes;
}

static BRSetOf(BREthereumToken)
initialTokensLoad (BREthereumEWM ewm) {
    BRSetOf(BREthereumToken) tokens = tokenSetCreate (EWM_INITIAL_SET_SIZE_DEFAULT);
    if (NULL != tokens && 1 != fileServiceLoad (ewm->fs, tokens, ewmFileServiceTypeTokens, 1)) {
        BRSetFreeAll (tokens, (void (*) (void*)) tokenRelease);
        return NULL;
    }
    return tokens;
}

static BRSetOf(BREthereumWalletState)
initialWalletsLoad (BREthereumEWM ewm) {
    BRSetOf(BREthereumWalletState) states = walletStateSetCreate (EWM_INITIAL_SET_SIZE_DEFAULT);
    if (NULL != states && 1 != fileServiceLoad (ewm->fs, states, ewmFileServiceTypeWallets, 1)) {
        BRSetFreeAll (states, (void (*) (void*)) walletStateRelease);
        return NULL;
    }
    return states;
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
        case FILE_SERVICE_SDB:
            eth_log ("EWM", "FileService Error: SDB: (%d): %s",
                     error.u.sdb.code,
                     error.u.sdb.reason);
            break;
    }
    eth_log ("EWM", "FileService Error: FORCED SYNC%s", "");

    // TODO: Actually force a resync.
}

/// MARK: - Ethereum Wallet Manager

static BREthereumEWM
ewmCreateErrorHandler (BREthereumEWM ewm, int fileService, const char* reason) {
    if (NULL != ewm) free (ewm);
    if (fileService)
        eth_log ("EWM", "on ewmCreate: FileService Error: %s", reason);
    else
        eth_log ("EWM", "on ewmCreate: Error: %s", reason);

    return NULL;
}

static void
ewmAssertRecovery (BREthereumEWM ewm);

static BREthereumBCSListener
ewmCreateBCSListener (BREthereumEWM ewm) {
    return (BREthereumBCSListener) {
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
}

static void
ewmCreateInitialSets (BREthereumEWM ewm,
                      BREthereumNetwork network,
                      BREthereumTimestamp accountTimestamp,
                      BRSetOf(BREthereumTransaction) *transactions,
                      BRSetOf(BREthereumLog) *logs,
                      BRSetOf(BREthereumNodeConfig) *nodes,
                      BRSetOf(BREthereumBlock) *blocks,
                      BRSetOf(BREthereumToken) *tokens,
                      BRSetOf(BREthereumWalletState) *states) {

    *transactions = initialTransactionsLoad(ewm);
    *logs = initialLogsLoad(ewm);
    *nodes = initialNodesLoad(ewm);
    *blocks = initialBlocksLoad(ewm);
    *tokens = initialTokensLoad(ewm);
    *states = initialWalletsLoad(ewm);

    // If any are NULL, then we have an error and a full sync is required.  The sync will be
    // started automatically, as part of the normal processing, of 'blocks' (we'll use a checkpoint,
    // before the `accountTimestamp, which will be well in the past and we'll sync up to the
    // head of the blockchain).
    if (NULL == *transactions || NULL == *logs || NULL == *nodes || NULL == *blocks || NULL == *tokens || NULL == *states) {
        // If the set exists, clear it out completely and then create another one.  Note, since
        // we have `BRSetFreeAll()` we'll use that even though it frees the set and then we
        // create one again, minimally wastefully.
        if (NULL != *transactions) { BRSetFreeAll (*transactions, (void (*) (void*)) transactionRelease); }
        *transactions = BRSetNew (transactionHashValue, transactionHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);

        if (NULL != *logs) { BRSetFreeAll (*logs, (void (*) (void*)) logRelease); }
        *logs = BRSetNew (logHashValue, logHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);

        if (NULL != *blocks) { BRSetFreeAll (*blocks,  (void (*) (void*)) blockRelease); }
        *blocks = BRSetNew (blockHashValue, blockHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);

        if (NULL != *nodes) { BRSetFreeAll (*nodes, (void (*) (void*)) nodeConfigRelease); }
        *nodes = BRSetNew (nodeConfigHashValue, nodeConfigHashEqual, EWM_INITIAL_SET_SIZE_DEFAULT);

        if (NULL != *tokens) { BRSetFreeAll (*tokens, (void (*) (void*)) tokenRelease); }
        *tokens = tokenSetCreate (EWM_INITIAL_SET_SIZE_DEFAULT);

        if (NULL != *states) { BRSetFreeAll (*states, (void (*) (void*)) walletStateRelease); }
        *states = walletStateSetCreate(EWM_INITIAL_SET_SIZE_DEFAULT);
    }

    // If we have no blocks; then add a checkpoint
    if (0 == BRSetCount(*blocks)) {
        const BREthereumBlockCheckpoint *checkpoint = blockCheckpointLookupByTimestamp (network, accountTimestamp);
        BREthereumBlock block = blockCreate (blockCheckpointCreatePartialBlockHeader (checkpoint));
        blockSetTotalDifficulty (block, checkpoint->u.td);
        BRSetAdd (*blocks, block);
    }
}

extern BREthereumEWM
ewmCreate (BREthereumNetwork network,
           BREthereumAccount account,
           BREthereumTimestamp accountTimestamp,
           BRCryptoSyncMode mode,
           BREthereumClient client,
           const char *storagePath,
           uint64_t blockHeight,
           uint64_t confirmationsUntilFinal) {
    BREthereumEWM ewm = (BREthereumEWM) calloc (1, sizeof (struct BREthereumEWMRecord));

    ewm->state = EWM_STATE_CREATED;
    ewm->mode = mode;
    ewm->network = network;
    ewm->account = account;
    ewm->accountTimestamp = accountTimestamp;
    ewm->bcs = NULL;
    ewm->blockHeight = blockHeight;
    ewm->confirmationsUntilFinal = confirmationsUntilFinal;
    ewm->requestId = 0;

    {
        char address [ADDRESS_ENCODED_CHARS];
        addressFillEncodedString (accountGetPrimaryAddress(account), 1, address);
        eth_log ("EWM", "Account: %s", address);
    }

    // Initialize the `brdSync` struct
    ewm->brdSync.ridTransaction = EWM_REQUEST_ID_UNKNOWN;
    ewm->brdSync.ridLog = EWM_REQUEST_ID_UNKNOWN;
    ewm->brdSync.begBlockNumber = 0;
    ewm->brdSync.endBlockNumber = ewm->blockHeight;
    ewm->brdSync.completedTransaction = 1;
    ewm->brdSync.completedLog = 1;

    // Get the client assigned early; callbacks as EWM/BCS state is re-establish, regarding
    // blocks, peers, transactions and logs, will be invoked.
    ewm->client = client;

    // Our one and only coder
    ewm->coder = rlpCoderCreate();

    // Create the EWM lock - do this early in case any `init` functions use it.
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&ewm->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // The file service.  Initialize {nodes, blocks, transactions and logs} from the FileService

    ewm->fs = fileServiceCreateFromTypeSpecfications (storagePath, "eth", networkGetName(network),
                                                      ewm,
                                                      ewmFileServiceErrorHandler,
                                                      ewmFileServiceSpecificationsCount,
                                                      ewmFileServiceSpecifications);
    if (NULL == ewm->fs) return ewmCreateErrorHandler(ewm, 1, "create");

    // Load all the persistent entities
    BRSetOf(BREthereumTransaction) transactions;
    BRSetOf(BREthereumLog) logs;
    BRSetOf(BREthereumNodeConfig) nodes;
    BRSetOf(BREthereumBlock) blocks;
    BRSetOf(BREthereumToken) tokens;
    BRSetOf(BREthereumWalletState) walletStates;

    ewmCreateInitialSets (ewm, ewm->network, ewm->accountTimestamp,
                          &transactions, &logs, &nodes, &blocks, &tokens, &walletStates);

    // Save the recovered tokens
    ewm->tokens = tokens;

    // Create the alarm clock, but don't start it.
    alarmClockCreateIfNecessary(0);

    // The `main` event handler has a periodic wake-up.  Used, perhaps, if the mode indicates
    // that we should/might query the BRD backend services.
    ewm->handler = eventHandlerCreate ("Core Ethereum EWM",
                                       ewmEventTypes,
                                       ewmEventTypesCount,
                                       &ewm->lock);

    array_new (ewm->wallets, DEFAULT_WALLET_CAPACITY);

    // Queue the CREATED event so that it is the first event delievered to the BREthereumClient
    ewmSignalEWMEvent (ewm, (BREthereumEWMEvent) {
        EWM_EVENT_CREATED,
        SUCCESS
    });

    // Create a default ETH wallet; other wallets will be created 'on demand'.  This will signal
    // a WALLET_EVENT_CREATED event.
    ewm->walletHoldingEther = walletCreate (ewm->account, ewm->network);
    ewmInsertWallet (ewm, ewm->walletHoldingEther);

    // Create the BCS listener - allows EWM to handle block, peer, transaction and log events.
    BREthereumBCSListener listener = ewmCreateBCSListener (ewm);

    BRAssertDefineRecovery ((BRAssertRecoveryInfo) ewm,
                            (BRAssertRecoveryHandler) ewmAssertRecovery);

    // Having restored the WalletState. restore that Wallet (balance).
    FOR_SET (BREthereumWalletState, state, walletStates) {
        BREthereumAddress address = walletStateGetAddress (state);

        // If the WalletState address is EMPTY_ADDRESS_INIT, then the state is for ETHER
        BREthereumBoolean addressIsForEther = addressEqual (EMPTY_ADDRESS_INIT, address);

        // See if we have a token.
        BREthereumToken token = (ETHEREUM_BOOLEAN_IS_TRUE (addressIsForEther)
                                 ? NULL
                                 : ewmLookupToken (ewm, address));

        // If we don't have a token and the address is not for ether, then ignore the state
        // This would occur if we have a state for a token that is no longer supported.
        if (NULL == token && ETHEREUM_BOOLEAN_IS_FALSE(addressIsForEther)) continue;

         // Get the balance
        BREthereumAmount balance = (NULL == token
                                    ? amountCreateEther (etherCreate (walletStateGetAmount (state)))
                                    : amountCreateToken (createTokenQuantity (token, walletStateGetAmount (state))));

        // Finally, update the balance; this will create TOK wallets as required.
        ewmHandleBalance (ewm, balance);

        if (NULL == token) {
            accountSetAddressNonce (ewm->account,
                                    accountGetPrimaryAddress(ewm->account),
                                    walletStateGetNonce(state),
                                    ETHEREUM_BOOLEAN_TRUE);
        }
    }

    // Create BCS - note: when BCS processes blocks, peers, transactions, and logs there
    // will be callbacks made to the EWM client.  Because we've defined `handlerForMain`
    // any callbacks will be queued and then handled when EWM actually starts
    //

    // Support the requested mode
    switch (ewm->mode) {
        case CRYPTO_SYNC_MODE_API_ONLY:
        case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND: {
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

            // ... as well as the provided logs... however, in handling an announced log we perform
            //   ```
            //    BREthereumToken token = tokenLookupByAddress(logGetAddress(log));
            //    if (NULL == token) { logRelease(log); return;}
            //   ```
            // and thus very single log is discared immediately.  They only come back with the
            // first sync.  We have no choice but to discard (until tokens are persistently stored)
            FOR_SET (BREthereumLog, log, logs)
                ewmSignalLog (ewm, BCS_CALLBACK_LOG_ADDED, log);

            // Previously both `ewmSignalTransaction()` and `ewmSignalLog` would iterate over
            // all the transfers to compute the wallet's balance.  (see `walletUpdateBalance()`
            // and its call sites (commented out currently)).  The balance was updated for each
            // and every added transaction and an 'BALANCE_UPDATED' event was generated for each.
            //
            // But, now, we do not rely on summing transfers amounts - instead, since Ethereum is
            // 'account based' we use the account state (ETH or ERC20) to get the wallet's
            // balance.  Note, this might need to change as it is not currently clear to me
            // how to get an ERC20 balance (execute a (free) transaction for 'balance'??); this
            // later case applies for `bcsCreate()` below in P2P modes.
            //
            // In addition, because we DO NOT handle 'internal transactions' the sum of transfers
            // cannot reliably produce the account balance.
            //
            // So, we have all the logs and transactions and we won't modify the wallet balances
            // that have been restored from persistent state.

#if 0
            // ... and then the latest block.
            BREthereumBlock lastBlock = NULL;
            FOR_SET (BREthereumBlock, block, blocks)
                if (NULL == lastBlock || blockGetNumber(lastBlock) < blockGetNumber(block))
                    lastBlock = block;
            ewmSignalBlockChain (ewm,
                                 blockGetHash( lastBlock),
                                 blockGetNumber (lastBlock),
                                 blockGetTimestamp (lastBlock));
#endif
            // Use the provided `blockHeight` in API modes.  We only have the `blockHeight` as a
            // parameter, thus calling `ewmSignalBlockChain()` with 'empty' values for `blockHash`
            // and `blockTimestamp` works because we know that `ewmHandleBlockChain` doesn't use
            // those parameters.
            //
            // What was the expectation of the above `lastBlock` code in API modes?  It might have
            // been from a time when `blockHeight` was not passed in as a parameters and thus we
            // need some block height and the best we could do was the saved blocks (like from a
            // P2P mode) or the checkpoint which is always there, at least.
            //
            // Our current API appraoch is to use BlockSet where a) we have the current block height
            // or b) if BlockSet, is down we hardcode the block height at the time of the software
            // distribution (See System.supportedBlockchains) - either way we have a better
            // current block height than any checkpoint.
            
            ewmSignalBlockChain (ewm, EMPTY_HASH_INIT, ewm->blockHeight, 0);

            // ... and then just ignore nodes

            // Free sets... BUT DO NOT free 'nodes' as those had 'OwnershipGiven' in bcsCreate()
            BRSetFreeAll(blocks, (void (*) (void*)) blockRelease);

            // We must not free the individual `transactions` and `logs` as they were OwnershipGiven
            // in the above `ewmSignalTransaction()` and `ewmSignalLog()` calls.
            BRSetFree (transactions);
            BRSetFree (logs);

            // Add ewmPeriodicDispatcher to handlerForMain.  Note that a 'timeout' is handled by
            // an OOB (out-of-band) event whereby the event is pushed to the front of the queue.
            // This may not be the right thing to do.  Imagine that EWM is blocked somehow (doing
            // a time consuming calculation) and two 'timeout events' occur - the events will be
            // queued in the wrong order (second before first).
            //
            // The function `ewmPeriodcDispatcher()` will be installed as a periodic alarm
            // on the event handler.  It will only trigger when the event handler is running (
            // the time between `eventHandlerStart()` and `eventHandlerStop()`)

            eventHandlerSetTimeoutDispatcher(ewm->handler,
                                             1000 * EWM_SLEEP_SECONDS,
                                             (BREventDispatcher) ewmPeriodicDispatcher,
                                             (void*) ewm);

            break;
        }

        case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC:  //
        case CRYPTO_SYNC_MODE_P2P_ONLY: {
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

    BRSetFreeAll (walletStates, (void (*) (void*)) walletStateRelease);

    // mark as 'sync in progress' - we can't sent transactions until we have the nonce.
    return ewm;

}

extern BREthereumEWM
ewmCreateWithPaperKey (BREthereumNetwork network,
                       const char *paperKey,
                       BREthereumTimestamp accountTimestamp,
                       BRCryptoSyncMode mode,
                       BREthereumClient client,
                       const char *storagePath,
                       uint64_t blockHeight,
                       uint64_t confirmationsUntilFinal) {
    return ewmCreate (network,
                      createAccount (paperKey),
                      accountTimestamp,
                      mode,
                      client,
                      storagePath,
                      blockHeight,
                      confirmationsUntilFinal);
}

extern BREthereumEWM
ewmCreateWithPublicKey (BREthereumNetwork network,
                        BRKey publicKey,
                        BREthereumTimestamp accountTimestamp,
                        BRCryptoSyncMode mode,
                        BREthereumClient client,
                        const char *storagePath,
                        uint64_t blockHeight,
                        uint64_t confirmationsUntilFinal) {
    return ewmCreate (network,
                      createAccountWithPublicKey(publicKey),
                      accountTimestamp,
                      mode,
                      client,
                      storagePath,
                      blockHeight,
                      confirmationsUntilFinal);
}

extern void
ewmDestroy (BREthereumEWM ewm) {
    // Stop, including disconnect.  This WILL take `ewm->lock` and it MUST be available.
    ewmStop (ewm);

    pthread_mutex_lock(&ewm->lock);

    //
    // Begin destroy
    //

    bcsDestroy(ewm->bcs);

    walletsRelease (ewm->wallets);
    ewm->wallets = NULL;

    BRSetFreeAll (ewm->tokens, (void (*) (void*)) tokenRelease);
    ewm->tokens = NULL;

    fileServiceRelease (ewm->fs);
    eventHandlerDestroy(ewm->handler);
    rlpCoderRelease(ewm->coder);

    // Finally remove the assert recovery handler
    BRAssertRemoveRecovery((BRAssertRecoveryInfo) ewm);

    pthread_mutex_unlock (&ewm->lock);
    pthread_mutex_destroy (&ewm->lock);

    memset (ewm, 0, sizeof(*ewm));
    free (ewm);
}

/// MARK: - Start/Stop

extern void
ewmStart (BREthereumEWM ewm) {
    // TODO: Check on a current state before starting.

    // Start the alarm clock.
    alarmClockStart(alarmClock);

    // Start the EWM thread
    eventHandlerStart(ewm->handler);
}

extern void
ewmStop (BREthereumEWM ewm) {
    // TODO: Check on a current state before stopping.

    // Disconnect
    ewmDisconnect(ewm);
    // TODO: Are their disconnect events that we need to process before stopping the handler?

    // Stop the alarm clock
    alarmClockStop (alarmClock);

    // Stop the EWM thread
    eventHandlerStop(ewm->handler);

    // Close the file service.
    fileServiceClose (ewm->fs);
}

/// MARK: - Connect / Disconnect

/**
 * ewmConnect() - Start EWM.  Returns TRUE if started, FALSE if is currently stated (TRUE
 * is action taken).
 *
 * Note that connecting *does not necessarily* start with empty queues.  Once EWM is created it
 * can have events signaled.  (This happens routinely as 'tokens' are announced; the token
 * announcements are queued and then handled as the first events once EWM is connected).
 */
extern BREthereumBoolean
ewmConnect(BREthereumEWM ewm) {
    BREthereumBoolean result = ETHEREUM_BOOLEAN_FALSE;

    pthread_mutex_lock(&ewm->lock);

    BREthereumEWMState oldState = ewm->state;
    BREthereumEWMState newState = ewm->state;

    // Nothing to do if already connected
    if (ETHEREUM_BOOLEAN_IS_FALSE (ewmIsConnected(ewm))) {

         // Set ewm {client,state} prior to bcs/event start.  Avoid race conditions, particularly
        // with `ewmPeriodicDispatcher`.
        ewm->state = EWM_STATE_CONNECTED;
        newState = ewm->state;

        switch (ewm->mode) {
            case CRYPTO_SYNC_MODE_API_ONLY:
                // Immediately start an API sync
                ewmSignalSyncAPI (ewm, ETHEREUM_BOOLEAN_TRUE);
                break;
            case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND:
                ewmSignalSyncAPI (ewm, ETHEREUM_BOOLEAN_TRUE);
                // fall-through
            case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC:
            case CRYPTO_SYNC_MODE_P2P_ONLY:
                bcsStart(ewm->bcs);
                break;
        }

        result = ETHEREUM_BOOLEAN_TRUE;
    }

    if (oldState != newState)
        ewmSignalEWMEvent (ewm, (BREthereumEWMEvent) {
            EWM_EVENT_CHANGED,
            SUCCESS,
            { .changed = { oldState, newState }}
        });

    pthread_mutex_unlock (&ewm->lock);

    return result;
}

/**
 * Stop EWM.  Returns TRUE if stopped, FALSE if currently stopped.
 *
 * @param ewm EWM
 * @return TRUE if action needed.
 */
extern BREthereumBoolean
ewmDisconnect (BREthereumEWM ewm) {
    BREthereumBoolean result = ETHEREUM_BOOLEAN_FALSE;

    pthread_mutex_lock(&ewm->lock);

    BREthereumEWMState oldState = ewm->state;
    BREthereumEWMState newState = ewm->state;

    if (ETHEREUM_BOOLEAN_IS_TRUE (ewmIsConnected(ewm))) {
        // Set ewm->state thereby stopping handlers (in a race with bcs/event calls).
        ewm->state = EWM_STATE_DISCONNECTED;
        newState = ewm->state;

        // Stop an ongoing sync
        switch (ewm->mode) {
            case CRYPTO_SYNC_MODE_API_ONLY:
            case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND:
                // If we are in the middle of a sync, the end it.
                if (!ewm->brdSync.completedTransaction || !ewm->brdSync.completedLog) {

                    // but only announce if it is not an 'ongoing' sync
                    if (ewmIsNotAnOngoingSync(ewm)) {
                        ewmSignalEWMEvent (ewm, (BREthereumEWMEvent) {
                            EWM_EVENT_CHANGED,
                            SUCCESS,
                            { .changed = { EWM_STATE_SYNCING, EWM_STATE_CONNECTED }}
                        });
                        oldState = EWM_STATE_CONNECTED;
                    }

                    ewm->brdSync.begBlockNumber = 0;
                    ewm->brdSync.endBlockNumber = ewm->blockHeight;
                    ewm->brdSync.completedTransaction = 1;
                    ewm->brdSync.completedLog = 1;
                }
                break;
            default: break;
        }

        // Stop BCS
        switch (ewm->mode) {
            case CRYPTO_SYNC_MODE_API_ONLY:
                break;
            case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND:
            case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC:
            case CRYPTO_SYNC_MODE_P2P_ONLY:
                bcsStop(ewm->bcs);
                break;
        }

        result = ETHEREUM_BOOLEAN_TRUE;
    }

    if (oldState != newState)
        ewmSignalEWMEvent (ewm, (BREthereumEWMEvent) {
            EWM_EVENT_CHANGED,
            SUCCESS,
            { .changed = { oldState, newState }}
        });

    pthread_mutex_unlock (&ewm->lock);

    return result;
}

extern BREthereumBoolean
ewmIsConnected (BREthereumEWM ewm) {
    BREthereumBoolean result = ETHEREUM_BOOLEAN_FALSE;

    pthread_mutex_lock(&ewm->lock);

    if (EWM_STATE_CONNECTED == ewm->state || EWM_STATE_SYNCING == ewm->state) {
        switch (ewm->mode) {
            case CRYPTO_SYNC_MODE_API_ONLY:
                result = ETHEREUM_BOOLEAN_TRUE;
                break;

            case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND:
            case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC:
            case CRYPTO_SYNC_MODE_P2P_ONLY:
                result = bcsIsStarted (ewm->bcs);
                break;
        }
    }

    pthread_mutex_unlock (&ewm->lock);
    return result;
}

static void
ewmAssertRecovery (BREthereumEWM ewm) {
    eth_log ("EWM", "Recovery%s", "");
    ewmDisconnect (ewm);
}

extern BREthereumNetwork
ewmGetNetwork (BREthereumEWM ewm) {
    return ewm->network; // constant
}

extern BREthereumAccount
ewmGetAccount (BREthereumEWM ewm) {
    return ewm->account; // constant
}

extern char *
ewmGetAccountPrimaryAddress(BREthereumEWM ewm) {
    return accountGetPrimaryAddressString(ewmGetAccount(ewm)); // constant
}

extern BRKey // key.pubKey
ewmGetAccountPrimaryAddressPublicKey(BREthereumEWM ewm) {
    return accountGetPrimaryAddressPublicKey(ewmGetAccount(ewm)); // constant
}

extern BRKey
ewmGetAccountPrimaryAddressPrivateKey(BREthereumEWM ewm,
                                           const char *paperKey) {
    return accountGetPrimaryAddressPrivateKey (ewmGetAccount(ewm), paperKey); // constant

}

/// MARK: - Sync

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
ewmSync (BREthereumEWM ewm,
         BREthereumBoolean pendExistingTransfers) {
    return ewmSyncToDepth (ewm, pendExistingTransfers, CRYPTO_SYNC_DEPTH_FROM_CREATION);
}

typedef struct {
    BREthereumEWM ewm;
    uint64_t transferBlockHeight;
    uint64_t confirmedBlockHeight;
} ewmSyncToDepthGetLastConfirmedSendTransferHeightContext;

static int
ewmSyncToDepthGetLastConfirmedSendTransferHeightPredicate (ewmSyncToDepthGetLastConfirmedSendTransferHeightContext *context,
                                                           BREthereumTransfer transfer,
                                                           unsigned int index) {
    BREthereumAccount account = ewmGetAccount (context->ewm);
    BREthereumAddress accountAddress = accountGetPrimaryAddress (account);

    BREthereumAddress source = transferGetSourceAddress (transfer);
    BREthereumAddress target = transferGetTargetAddress (transfer);

    BREthereumBoolean accountIsSource = addressEqual (source, accountAddress);
    BREthereumBoolean accountIsTarget = addressEqual (target, accountAddress);

    uint64_t blockNumber = 0;
    // check that the transfer has been included, is a send and has been confirmed as final
    return (transferExtractStatusIncluded (transfer, NULL, &blockNumber, NULL, NULL, NULL) &&
            accountIsSource == ETHEREUM_BOOLEAN_TRUE && accountIsTarget == ETHEREUM_BOOLEAN_FALSE &&
            blockNumber < (context->confirmedBlockHeight));
}

static void
ewmSyncToDepthGetLastConfirmedSendTransferHeightWalker (ewmSyncToDepthGetLastConfirmedSendTransferHeightContext *context,
                                                        BREthereumTransfer transfer,
                                                        unsigned int index) {
    uint64_t blockNumber = 0;
    transferExtractStatusIncluded (transfer, NULL, &blockNumber, NULL, NULL, NULL);
    context->transferBlockHeight = (blockNumber > context->transferBlockHeight ?
                                    blockNumber : context->transferBlockHeight);
    return;
}

static uint64_t
ewmSyncToDepthCalculateBlockHeight (BREthereumEWM ewm,
                                    BRCryptoSyncDepth depth) {
    uint64_t blockHeight = 0;

    pthread_mutex_lock(&ewm->lock);
    switch (depth) {
        case CRYPTO_SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND: {
            if (ewm->blockHeight >= ewm->confirmationsUntilFinal) {
                ewmSyncToDepthGetLastConfirmedSendTransferHeightContext context = { ewm, 0, ewm->blockHeight - ewm->confirmationsUntilFinal };
                BREthereumWallet *wallets = ewmGetWallets(ewm);

                for (size_t wid = 0; NULL != wallets[wid]; wid++)
                    walletWalkTransfers (wallets[wid], &context,
                                        (BREthereumTransferPredicate) ewmSyncToDepthGetLastConfirmedSendTransferHeightPredicate,
                                        (BREthereumTransferWalker)    ewmSyncToDepthGetLastConfirmedSendTransferHeightWalker);

                free (wallets);

                blockHeight = context.transferBlockHeight;
            }
            break;
        }
        case CRYPTO_SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK: {
            const BREthereumBlockCheckpoint *checkpoint = blockCheckpointLookupByNumber (ewm->network, ewm->blockHeight);
            blockHeight = NULL == checkpoint ? 0 : checkpoint->number;
            break;
        }
        case CRYPTO_SYNC_DEPTH_FROM_CREATION: {
            // Start a sync from block 0
            blockHeight = 0;
            break;
        }
    }
    pthread_mutex_unlock(&ewm->lock);

    return blockHeight;
}

extern BREthereumBoolean
ewmSyncToDepth (BREthereumEWM ewm,
                BREthereumBoolean pendExistingTransfers,
                BRCryptoSyncDepth depth) {
    if (EWM_STATE_CONNECTED != ewm->state) return ETHEREUM_BOOLEAN_FALSE;

    uint64_t blockHeight = ewmSyncToDepthCalculateBlockHeight (ewm, depth);

    switch (ewm->mode) {
        case CRYPTO_SYNC_MODE_API_ONLY:
        case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND: {
            pthread_mutex_lock(&ewm->lock);

            // set the beginning block number to the minimum between the calculated height and
            // the known block height
            blockHeight = blockHeight < ewm->blockHeight ? blockHeight : ewm->blockHeight;

            if (ETHEREUM_BOOLEAN_IS_TRUE (pendExistingTransfers)) {
                BREthereumSyncTransferContext context = { ewm, blockHeight, ewm->blockHeight };
                BREthereumWallet *wallets = ewmGetWallets(ewm);

                // Walk each wallet, set all transfers to 'pending'
                for (size_t wid = 0; NULL != wallets[wid]; wid++)
                    walletWalkTransfers (wallets[wid], &context,
                                         (BREthereumTransferPredicate) ewmSyncUpdateTransferPredicate,
                                         (BREthereumTransferWalker)    ewmSyncUpdateTransfer);

                free (wallets);
            }

            // Abort an in progress sync
            if (!ewm->brdSync.completedTransaction || !ewm->brdSync.completedLog) {
                // With the following, any callback to `ewmHandleAnnounceComplete` WILL skip out
                // before changing `brdSync` state.  Of course, the call to ewmHandleSyncAPI()
                // (that is below) will immediately reassign new rids thereby mooting the need for
                // assignin -1 here.  However, assigning -1 here does no harm and allows replacing
                // of `ewmHandleSyncAPI` with `ewmSignalSyncAPI` if need be.
                ewm->brdSync.ridLog = EWM_REQUEST_ID_UNKNOWN;
                ewm->brdSync.ridTransaction = EWM_REQUEST_ID_UNKNOWN;

                // If this was not an 'ongoing' sync, then signal back to 'connected'.  This will
                // appear as syncEnded(success) - that is okay.
                if (ewmIsNotAnOngoingSync (ewm))
                    ewmSignalEWMEvent (ewm, (BREthereumEWMEvent) {
                        EWM_EVENT_CHANGED,
                        SUCCESS,
                        { .changed = { EWM_STATE_SYNCING, EWM_STATE_CONNECTED }}
                    });

                // Start anew as 'completed'
                ewm->brdSync.completedTransaction = 1;
                ewm->brdSync.completedLog = 1;
            }

            // don't allow the sync to jump forward so that we don't have a situation where
            // we miss transfers
            ewm->brdSync.begBlockNumber = (ewm->brdSync.begBlockNumber < blockHeight ?
                                           ewm->brdSync.begBlockNumber : blockHeight);

            // Immediately sync - inline call (not via 'signal'; directly 'handle')
            ewmHandleSyncAPI (ewm);

            pthread_mutex_unlock(&ewm->lock);
            return ETHEREUM_BOOLEAN_TRUE;
        }
        case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC:
        case CRYPTO_SYNC_MODE_P2P_ONLY:
            bcsSync (ewm->bcs, blockHeight);
            return ETHEREUM_BOOLEAN_TRUE;
    }
}

/// MARK: - Mode

extern BRCryptoSyncMode
ewmGetMode (BREthereumEWM ewm) {
    pthread_mutex_lock (&ewm->lock);
    BRCryptoSyncMode mode = ewm->mode;
    pthread_mutex_unlock (&ewm->lock);
    return mode;
}

extern void
ewmUpdateMode (BREthereumEWM ewm,
               BRCryptoSyncMode mode) {
    pthread_mutex_lock (&ewm->lock);

    BRCryptoSyncMode oldMode = ewm->mode;
    BRCryptoSyncMode newMode = mode;


    if (oldMode != newMode) {

        // Disconnect if connected; reconnect if connected.
         if (ETHEREUM_BOOLEAN_IS_TRUE(ewmIsConnected(ewm)))
            ewmDisconnect(ewm); // Stops periodic dispatch too.

        BRSetOf(BREthereumToken) tokens;
        BRSetOf(BREthereumNodeConfig) nodes;
        BRSetOf(BREthereumBlock) blocks;
        BRSetOf(BREthereumTransaction) transactions;
        BRSetOf(BREthereumLog) logs;
        BRSetOf(BREthereumWalletState) states;

        // We have BCS in all modes but in BRD_ONLY mode it is never started.


        //
        // This `bcsStop()` is going a) to call `lesStop()` and b) then stop *and clear*
        // the BCS event handler.  When LES stops, the current LES nodes will be saved by
        // calling `bcsSignalNodes()` which, when handled, will then call
        // `ewmSignalSaveNodes()`.  Note that `lesStop()` will block until `lesThread()`
        // actually completes.  Thus upon completion there might be at leasat ONE `bcsSignalNodes()`
        // event in the BCS event queue...  and then the BCS event handler is stopped
        // and cleared.
        //
        // Will the nodes actually get written?  There is quite a bit of computation that happens
        // in `lesThread()` after `bcsSignalNodes()` is called - including printing to log and
        // deactivating TCP/UDP sockets - so it is possible that the ONE event will get dispatched
        // and nodes written to file.
        //
        bcsStop (ewm->bcs);

        // Everything gone at this point.  Should not be any references to BCS still using
        // BCS at this point.  Surely none.
        bcsDestroy (ewm->bcs);

        // Get some current state that we'll use when recreating BCS.
        BREthereumAddress primaryAddress = accountGetPrimaryAddress(ewm->account);
        BREthereumBCSListener listener   = ewmCreateBCSListener (ewm);

        // Set the new mode
        ewm->mode = newMode;

        //
        // We'll create a node-specific BCS here; this parallels how BCS is created in ewmCreat().
        // The pimary difference being that in ewmCreate() we announce newly-recovered transactions
        // and logs (recovered from persistent storage).  We don't need to reannounce those here
        // as they are already in EWM.
        //

        switch (newMode) {
            case CRYPTO_SYNC_MODE_API_ONLY:
            case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND:
                ewm->bcs = bcsCreate (ewm->network,
                                      primaryAddress,
                                      listener,
                                      newMode,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL);
                break;

            case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC:
            case CRYPTO_SYNC_MODE_P2P_ONLY:
                ewmCreateInitialSets (ewm, ewm->network, ewm->accountTimestamp,
                                      &transactions, &logs, &nodes, &blocks, &tokens, &states);

                ewm->bcs = bcsCreate (ewm->network,
                                      primaryAddress,
                                      listener,
                                      newMode,
                                      nodes,
                                      blocks,
                                      transactions,
                                      logs);

                BRSetFreeAll (states, (void (*) (void*)) walletStateRelease);
                break;
         }

        // Don't reestablish a connection
    }
    pthread_mutex_unlock (&ewm->lock);
}

extern void
ewmWipe (BREthereumNetwork network,
         const char *storagePath) {
    fileServiceWipe (storagePath, "eth", networkGetName(network));
}

/// MARK: - Blocks

extern uint64_t
ewmGetBlockHeight(BREthereumEWM ewm) {
    pthread_mutex_lock(&ewm->lock);
    uint64_t height = ewm->blockHeight;
    pthread_mutex_unlock(&ewm->lock);
    return height;
}

extern void
ewmUpdateBlockHeight(BREthereumEWM ewm,
                     uint64_t blockHeight) {
    pthread_mutex_lock(&ewm->lock);
    if (blockHeight != ewm->blockHeight) {
        ewm->blockHeight = blockHeight;
        ewmSignalEWMEvent (ewm, ((BREthereumEWMEvent) {
            EWM_EVENT_BLOCK_HEIGHT_UPDATED,
            SUCCESS,
            { .blockHeight = { blockHeight }}
        }));
    }
    pthread_mutex_unlock(&ewm->lock);
}

/// MARK: - Transfers
/// MARK: - Wallets

extern void
ewmInsertWallet (BREthereumEWM ewm,
                 BREthereumWallet wallet) {
    pthread_mutex_lock(&ewm->lock);
    array_add (ewm->wallets, wallet);
    ewmSignalWalletEvent (ewm, wallet, (BREthereumWalletEvent) {
        WALLET_EVENT_CREATED,
        SUCCESS
    });
    pthread_mutex_unlock(&ewm->lock);
}

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

extern size_t
ewmGetWalletsCount (BREthereumEWM ewm) {
    pthread_mutex_lock(&ewm->lock);
    size_t count = array_count(ewm->wallets);
    pthread_mutex_unlock(&ewm->lock);
    return count;
}

extern BREthereumWallet
ewmGetWallet(BREthereumEWM ewm) {
    return ewm->walletHoldingEther; // constant
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
    ewmSignalTransferEvent (ewm, wallet, transfer, (BREthereumTransferEvent) {
        TRANSFER_EVENT_CREATED,
        SUCCESS
    });

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
    ewmSignalTransferEvent(ewm, wallet, transfer, (BREthereumTransferEvent) {
        TRANSFER_EVENT_CREATED,
        SUCCESS
    });

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
    transfer = walletCreateTransferWithFeeBasis (wallet, addressCreate(recvAddress), amount, feeBasis);
    pthread_mutex_unlock(&ewm->lock);

    // Transfer DOES NOT have a hash yet because it is not signed; but it is inserted in the
    // wallet and can be display, in order, w/o the hash
    ewmSignalTransferEvent (ewm, wallet, transfer, (BREthereumTransferEvent) {
        TRANSFER_EVENT_CREATED,
        SUCCESS
    });

    return transfer;
}

extern BREthereumEther
ewmWalletEstimateTransferFee(BREthereumEWM ewm,
                             BREthereumWallet wallet,
                             BREthereumAmount amount,
                             int *overflow) {
    pthread_mutex_lock(&ewm->lock);
    BREthereumEther fee = walletEstimateTransferFee(wallet, amount, overflow);
    pthread_mutex_unlock(&ewm->lock);
    return fee;
}

extern BREthereumEther
ewmWalletEstimateTransferFeeForBasis(BREthereumEWM ewm,
                                     BREthereumWallet wallet,
                                     BREthereumAmount amount,
                                     BREthereumGasPrice price,
                                     BREthereumGas gas,
                                     int *overflow) {
    pthread_mutex_lock(&ewm->lock);
    BREthereumEther fee = walletEstimateTransferFeeDetailed (wallet, amount, price, gas, overflow);
    pthread_mutex_unlock(&ewm->lock);
    return fee;
}

extern void
ewmWalletEstimateTransferFeeForTransfer (BREthereumEWM ewm,
                                         BREthereumWallet wallet,
                                         BREthereumCookie cookie,
                                         BREthereumAddress source,
                                         BREthereumAddress target,
                                         BREthereumAmount amount,
                                         BREthereumGasPrice gasPrice,
                                         BREthereumGas gasLimit) {
    BREthereumToken  ethToken  = ewmWalletGetToken (ewm, wallet);

    // use transfer, instead of transaction, due to the need to fill out the transaction data based on if
    // it is a token transfer or not
    BREthereumTransfer transfer = transferCreate (source,
                                                  target,
                                                  amount,
                                                  (BREthereumFeeBasis) {FEE_BASIS_GAS, {.gas = {gasLimit, gasPrice}}},
                                                  (NULL == ethToken ? TRANSFER_BASIS_TRANSACTION : TRANSFER_BASIS_LOG));

    ewmGetGasEstimate (ewm, wallet, transfer, cookie);

    transferRelease (transfer);
}

extern BREthereumBoolean
ewmWalletCanCancelTransfer (BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumTransfer oldTransfer) {
    pthread_mutex_lock(&ewm->lock);
    BREthereumTransaction oldTransaction = transferGetOriginatingTransaction(oldTransfer);
    pthread_mutex_unlock(&ewm->lock);

    // TODO: Something about the 'status' (not already cancelled, etc)
    return AS_ETHEREUM_BOOLEAN (NULL != oldTransaction);
}

extern BREthereumTransfer // status, error
ewmWalletCreateTransferToCancel(BREthereumEWM ewm,
                                BREthereumWallet wallet,
                                BREthereumTransfer oldTransfer) {
    pthread_mutex_lock(&ewm->lock);
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
                       transactionGetData(oldTransaction),
                       transactionGetNonce(oldTransaction));

    transferSetStatus(oldTransfer, TRANSFER_STATUS_REPLACED);

    // Delete transfer??  Update transfer??
    BREthereumTransfer transfer = transferCreateWithTransactionOriginating (transaction,
                                                                            (NULL == walletGetToken(wallet)
                                                                             ? TRANSFER_BASIS_TRANSACTION
                                                                             : TRANSFER_BASIS_LOG));
    walletHandleTransfer(wallet, transfer);
    pthread_mutex_unlock(&ewm->lock);

    return transfer;
}

extern BREthereumBoolean
ewmWalletCanReplaceTransfer (BREthereumEWM ewm,
                             BREthereumWallet wid,
                             BREthereumTransfer oldTransfer) {
    pthread_mutex_lock(&ewm->lock);
    BREthereumTransaction oldTransaction = transferGetOriginatingTransaction(oldTransfer);
    pthread_mutex_unlock(&ewm->lock);

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
    pthread_mutex_lock(&ewm->lock);
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
                       transactionGetData(oldTransaction),
                       nonce);

    transferSetStatus(oldTransfer, TRANSFER_STATUS_REPLACED);

    // Delete transfer??  Update transfer??
    BREthereumTransfer transfer = transferCreateWithTransactionOriginating (transaction,
                                                                            (NULL == walletGetToken(wallet)
                                                                             ? TRANSFER_BASIS_TRANSACTION
                                                                             : TRANSFER_BASIS_LOG));
    walletHandleTransfer(wallet, transfer);
    pthread_mutex_unlock(&ewm->lock);
    return transfer;
}


static void
ewmWalletSignTransferAnnounce (BREthereumEWM ewm,
                               BREthereumWallet wallet,
                               BREthereumTransfer transfer) {
    ewmSignalTransferEvent (ewm, wallet, transfer, (BREthereumTransferEvent) {
        TRANSFER_EVENT_SIGNED,
        SUCCESS
    });
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

extern BREthereumAddress
ewmWalletGetAddress (BREthereumEWM ewm,
                     BREthereumWallet wallet) {
    return walletGetAddress(wallet);
}

extern BREthereumBoolean
ewmWalletHasAddress (BREthereumEWM ewm,
                     BREthereumWallet wallet,
                     BREthereumAddress address) {
    return addressEqual(address, walletGetAddress(wallet));
}

extern BREthereumToken
ewmWalletGetToken (BREthereumEWM ewm,
                   BREthereumWallet wallet) {
    return walletGetToken(wallet); // constant
}

extern BREthereumAmount
ewmWalletGetBalance(BREthereumEWM ewm,
                    BREthereumWallet wallet) {
    pthread_mutex_lock(&ewm->lock);
    BREthereumAmount balance = walletGetBalance(wallet);
    pthread_mutex_unlock(&ewm->lock);
    return balance;
}


extern BREthereumGas
ewmWalletGetGasEstimate(BREthereumEWM ewm,
                        BREthereumWallet wallet,
                        BREthereumTransfer transfer) {
    pthread_mutex_lock(&ewm->lock);
    BREthereumGas gas = transferGetGasEstimate(transfer);
    pthread_mutex_unlock(&ewm->lock);
    return gas;

}

extern BREthereumGas
ewmWalletGetDefaultGasLimit(BREthereumEWM ewm,
                            BREthereumWallet wallet) {
    pthread_mutex_lock(&ewm->lock);
    BREthereumGas gas = walletGetDefaultGasLimit(wallet);
    pthread_mutex_unlock(&ewm->lock);
    return gas;
}

extern void
ewmWalletSetDefaultGasLimit(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumGas gasLimit) {
    pthread_mutex_lock(&ewm->lock);
    walletSetDefaultGasLimit(wallet, gasLimit);
    pthread_mutex_unlock(&ewm->lock);

    ewmSignalWalletEvent(ewm,
                         wallet,
                         (BREthereumWalletEvent) {
                             WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED,
                             SUCCESS
                         });
}

extern BREthereumGasPrice
ewmWalletGetDefaultGasPrice(BREthereumEWM ewm,
                            BREthereumWallet wallet) {
    pthread_mutex_lock(&ewm->lock);
    BREthereumGasPrice price = walletGetDefaultGasPrice(wallet);
    pthread_mutex_unlock(&ewm->lock);
    return price;
}

extern void
ewmWalletSetDefaultGasPrice(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumGasPrice gasPrice) {
    pthread_mutex_lock(&ewm->lock);
    walletSetDefaultGasPrice(wallet, gasPrice);
    pthread_mutex_unlock(&ewm->lock);

    ewmSignalWalletEvent(ewm,
                         wallet,
                         (BREthereumWalletEvent) {
                             WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
                             SUCCESS
                         });
}


/// MARK: - Handlers

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
    walletSetDefaultGasPrice(wallet, gasPrice);

    ewmSignalWalletEvent(ewm,
                         wallet,
                         (BREthereumWalletEvent) {
                             WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
                             SUCCESS
                         });
}

#if defined (NEVER_DEFINED)
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
    transferSetGasEstimate(transfer, gasEstimate);

    ewmSignalTransferEvent(ewm,
                           wallet,
                           transfer,
                           (BREthereumTransferEvent) {
                               TRANSFER_EVENT_GAS_ESTIMATE_UPDATED,
                               SUCCESS
                           });
}
#endif
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
    if (CRYPTO_SYNC_MODE_API_ONLY == ewm->mode || ETHEREUM_BOOLEAN_IS_FALSE(bcsSyncInProgress (ewm->bcs)))
        eth_log ("EWM", "BlockChain: %" PRIu64, headBlockNumber);

    // At least this - allows for: ewmGetBlockHeight
    ewmUpdateBlockHeight (ewm, headBlockNumber);

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
    eth_log("EWM", "AccountState: Nonce: %" PRIu64, accountState.nonce);
    ewmHandleAnnounceNonce (ewm, accountGetPrimaryAddress(ewm->account), accountState.nonce, 0);
    ewmSignalBalance(ewm, amountCreateEther(accountState.balance));
}

extern void
ewmHandleBalance (BREthereumEWM ewm,
                  BREthereumAmount amount) {
    BREthereumWallet wallet = (AMOUNT_ETHER == amountGetType(amount)
                               ? ewmGetWallet(ewm)
                               : ewmGetWalletHoldingToken(ewm, amountGetToken (amount)));

    int amountTypeMismatch;

    if (ETHEREUM_COMPARISON_EQ != amountCompare(amount, walletGetBalance(wallet), &amountTypeMismatch)) {
        walletSetBalance(wallet, amount);
        ewmSignalWalletEvent (ewm,
                              wallet,
                              (BREthereumWalletEvent) {
                                  WALLET_EVENT_BALANCE_UPDATED,
                                  SUCCESS
                              });

        {
            char *amountAsString = (AMOUNT_ETHER == amountGetType(amount)
                                    ? etherGetValueString (amountGetEther(amount), WEI)
                                    : tokenQuantityGetValueString (amountGetTokenQuantity(amount), TOKEN_QUANTITY_TYPE_INTEGER));
            eth_log("EWM", "Balance: %s %s (%s)", amountAsString,
                    (AMOUNT_ETHER == amountGetType(amount) ? "ETH" : tokenGetName(amountGetToken(amount))),
                    (AMOUNT_ETHER == amountGetType(amount) ? "WEI"    : "INTEGER"));
            free (amountAsString);
        }
    }
}

static int
ewmReportTransferStatusAsEventIsNeeded (BREthereumEWM ewm,
                                        BREthereumWallet wallet,
                                        BREthereumTransfer transfer,
                                        BREthereumTransactionStatus status) {
    return (// If the status differs from the transfer's basis status...
            ETHEREUM_BOOLEAN_IS_FALSE (transactionStatusEqual (status, transferGetStatusForBasis(transfer))) ||
            // Otherwise, if the transfer's status differs.
            ETHEREUM_BOOLEAN_IS_FALSE (transferHasStatus (transfer, transferStatusCreate(status))));
}

static void
ewmReportTransferStatusAsEvent (BREthereumEWM ewm,
                                BREthereumWallet wallet,
                                BREthereumTransfer transfer) {
    if (ETHEREUM_BOOLEAN_IS_TRUE (transferHasStatus (transfer, TRANSFER_STATUS_SUBMITTED)))
        ewmSignalTransferEvent(ewm, wallet, transfer, (BREthereumTransferEvent) {
            TRANSFER_EVENT_SUBMITTED,
            SUCCESS
        });

    else if (ETHEREUM_BOOLEAN_IS_TRUE (transferHasStatus (transfer, TRANSFER_STATUS_INCLUDED)))
        ewmSignalTransferEvent(ewm, wallet, transfer, (BREthereumTransferEvent) {
            TRANSFER_EVENT_INCLUDED,
            SUCCESS
        });

    else if (ETHEREUM_BOOLEAN_IS_TRUE (transferHasStatus (transfer, TRANSFER_STATUS_ERRORED))) {
        char *reason = NULL;
        transferExtractStatusError (transfer, &reason);
        ewmSignalTransferEvent (ewm, wallet, transfer,
                                transferEventCreateError (TRANSFER_EVENT_ERRORED,
                                                          ERROR_TRANSACTION_SUBMISSION,
                                                          reason));

        if (NULL != reason) free (reason);
    }
}

//
// We have `transaction` but we don't know if it originated a log.  If it did originate a log then
// we need to update that log's status.  We don't know what logs the transaction originated so
// we'll look through all wallets and all their transfers for any one transfer that matches the
// provided transaction.
//
// Note: that `transaction` is owned by another; thus we won't hold it.
//
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

static void
ewmHandleLogFeeBasis (BREthereumEWM ewm,
                      BREthereumHash hash,
                      BREthereumTransfer transferTransaction,
                      BREthereumTransfer transferLog) {

    // Find the ETH transfer, if needed
    if (NULL == transferTransaction)
        transferTransaction = walletGetTransferByIdentifier (ewmGetWallet(ewm), hash);

    // If none exists, then the transaction hasn't been 'synced' yet.
    if (NULL == transferTransaction) return;

    // If we have a TOK transfer, set the fee basis.
    if (NULL != transferLog)
        transferSetFeeBasis(transferLog, transferGetFeeBasis(transferTransaction));

    // but if we don't have a TOK transfer, find every transfer referencing `hash` and set the basis.
    else
        for (size_t wid = 0; wid < array_count(ewm->wallets); wid++) {
            BREthereumWallet wallet = ewm->wallets[wid];

            // We are only looking for TOK transfers (non-ETH).
            if (wallet == ewm->walletHoldingEther) continue;

            size_t tidLimit = walletGetTransferCount (wallet);
            for (size_t tid = 0; tid < tidLimit; tid++) {
                transferLog = walletGetTransferByIndex (wallet, tid);

                // Look for a log that has a matching transaction hash
                BREthereumLog log = transferGetBasisLog(transferLog);
                if (NULL != log) {
                    BREthereumHash transactionHash;
                    if (ETHEREUM_BOOLEAN_TRUE == logExtractIdentifier (log, &transactionHash, NULL) &&
                        ETHEREUM_BOOLEAN_TRUE == hashEqual (transactionHash, hash))
                        ewmHandleLogFeeBasis (ewm, hash, transferTransaction, transferLog);
                }
            }
        }
}

extern void
ewmHandleTransaction (BREthereumEWM ewm,
                      BREthereumBCSCallbackTransactionType type,
                      OwnershipGiven BREthereumTransaction transaction) {
    BREthereumHash hash = transactionGetHash(transaction);

    // Find the wallet
    BREthereumWallet wallet = ewmGetWallet(ewm);
    assert (NULL != wallet);

    ///
    ///  What hash to use:
    ///     originating -> expecting a result
    ///     identifier  -> seen already.
    ///
    ///     originating should be good in one wallet?  [no, multiple logs?]
    ///        wallet will have transfers w/o a basis.
    ///        does a transfer with an ERC20 transfer fit in one wallet?
    ///        does a transfer with some smart contract fit in one wallet (no?)
    ///

    // Find a preexisting transfer
    BREthereumTransfer transfer = walletGetTransferByIdentifier (wallet, hash);
    if (NULL == transfer)
        transfer = walletGetTransferByOriginatingHash (wallet, hash);

    int needStatusEvent = 0;

    // If we've no transfer, then create one and save `transaction` as the basis
    if (NULL == transfer) {
        transfer = transferCreateWithTransaction (transaction); // transaction ownership given

        walletHandleTransfer (wallet, transfer);

        // We've added a transfer and arguably we should update the wallet's balance.  But don't.
        // Ethereum is 'account based'; we'll only update the balance based on a account state
        // change (based on a P2P or API callback).
        //
        // walletUpdateBalance (wallet);

        ewmSignalTransferEvent (ewm, wallet, transfer, (BREthereumTransferEvent) {
            TRANSFER_EVENT_CREATED,
            SUCCESS
        });

         // If this transfer is referenced by a log, fill out the log's fee basis.
        ewmHandleLogFeeBasis (ewm, hash, transfer, NULL);

        needStatusEvent = 1;
    }
    else {
        needStatusEvent = ewmReportTransferStatusAsEventIsNeeded (ewm, wallet, transfer,
                                                                  transactionGetStatus(transaction));


        // If this transaction is the transfer's originatingTransaction, then update the
        // originatingTransaction's status.
        BREthereumTransaction original = transferGetOriginatingTransaction (transfer);
        if (NULL != original && ETHEREUM_BOOLEAN_IS_TRUE(hashEqual (transactionGetHash(original),
                                                                    transactionGetHash(transaction))))
            transactionSetStatus (original, transactionGetStatus(transaction));

        transferSetBasisForTransaction (transfer, transaction); // transaction ownership given
    }

    if (needStatusEvent) {
        BREthereumHashString hashString;
        hashFillString(hash, hashString);
        eth_log ("EWM", "Transaction: \"%s\", Change: %s, Status: %d", hashString,
                 BCS_CALLBACK_TRANSACTION_TYPE_NAME(type),
                 transactionGetStatus(transaction).type);

        ewmReportTransferStatusAsEvent(ewm, wallet, transfer);
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

    // Assert that we always have an identifier for `log`.
    BREthereumBoolean extractedIdentifier = logExtractIdentifier (log, &transactionHash, &logIndex);
    assert (ETHEREUM_BOOLEAN_IS_TRUE (extractedIdentifier));
    
    BREthereumToken token = ewmLookupToken (ewm, logGetAddress(log));
    if (NULL == token) { logRelease(log); return;}

    // TODO: Confirm LogTopic[0] is 'transfer'
    if (3 != logGetTopicsCount(log)) { logRelease(log); return; }

    BREthereumWallet wallet = ewmGetWalletHoldingToken (ewm, token);
    assert (NULL != wallet);

    BREthereumTransfer transfer = walletGetTransferByIdentifier (wallet, logHash);
    if (NULL == transfer)
        transfer = walletGetTransferByOriginatingHash (wallet, transactionHash);

    int needStatusEvent = 0;

    // If we've no transfer, then create one and save `log` as the basis
    if (NULL == transfer) {
        transfer = transferCreateWithLog (log, token, ewm->coder); // log ownership given

        walletHandleTransfer (wallet, transfer);

        // We've added a transfer and arguably we should update the wallet's balance.  But don't.
        // Ethereum is 'account based'; we'll only update the balance based on a account state
        // change (based on a P2P or API callback).
        //
        // walletUpdateBalance (wallet);

        ewmSignalTransferEvent (ewm, wallet, transfer, (BREthereumTransferEvent) {
            TRANSFER_EVENT_CREATED,
            SUCCESS
        });

        // If this transfer references a transaction, fill out this log's fee basis
        ewmHandleLogFeeBasis (ewm, transactionHash, NULL, transfer);

        needStatusEvent = 1;
    }

    // We've got a transfer for log.  We'll update the transfer's basis and check if we need
    // to report a transfer status event.  We'll strive to only report events when the status has
    // actually changed.
    else {
        needStatusEvent = ewmReportTransferStatusAsEventIsNeeded (ewm, wallet, transfer,
                                                                  logGetStatus (log));

        // Log becomes the new basis for transfer
        transferSetBasisForLog (transfer, log);  // log ownership given
    }

    if (needStatusEvent) {
        BREthereumHashString logHashString;
        hashFillString(logHash, logHashString);

        BREthereumHashString transactionHashString;
        hashFillString(transactionHash, transactionHashString);

        eth_log ("EWM", "Log: %s { %8s @ %zu }, Change: %s, Status: %d",
                 logHashString, transactionHashString, logIndex,
                 BCS_CALLBACK_TRANSACTION_TYPE_NAME(type),
                 logGetStatus(log).type);

        ewmReportTransferStatusAsEvent (ewm, wallet, transfer);
    }
}

extern void
ewmHandleSaveBlocks (BREthereumEWM ewm,
                     OwnershipGiven BRArrayOf(BREthereumBlock) blocks) {
    size_t count = array_count(blocks);

    eth_log("EWM", "Save Blocks (Storage): %zu", count);
    fileServiceReplace (ewm->fs, ewmFileServiceTypeBlocks,
                        (const void **) blocks,
                        count);

    array_free (blocks);
}

extern void
ewmHandleSaveNodes (BREthereumEWM ewm,
                    OwnershipGiven BRArrayOf(BREthereumNodeConfig) nodes) {
    size_t count = array_count(nodes);

    eth_log("EWM", "Save Nodes (Storage): %zu", count);
    fileServiceReplace (ewm->fs, ewmFileServiceTypeNodes,
                        (const void **) nodes,
                        count);

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
        fileServiceRemove (ewm->fs, ewmFileServiceTypeTransactions,
                           fileServiceGetIdentifier(ewm->fs, ewmFileServiceTypeTransactions, transaction));

    if (CLIENT_CHANGE_ADD == type || CLIENT_CHANGE_UPD == type)
        fileServiceSave (ewm->fs, ewmFileServiceTypeTransactions, transaction);
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
        fileServiceRemove (ewm->fs, ewmFileServiceTypeLogs,
                           fileServiceGetIdentifier (ewm->fs, ewmFileServiceTypeLogs, log));

    if (CLIENT_CHANGE_ADD == type || CLIENT_CHANGE_UPD == type)
        fileServiceSave (ewm->fs, ewmFileServiceTypeLogs, log);
}

extern void
ewmHandleSaveWallet (BREthereumEWM ewm,
                     BREthereumWallet wallet,
                     BREthereumClientChangeType type) {
    BREthereumWalletState state = walletStateCreate (wallet);

    // If this is the primaryWallet, hack in the nonce
    if (wallet == ewm->walletHoldingEther) {
        walletStateSetNonce (state,
                             accountGetAddressNonce (ewm->account,
                                                     accountGetPrimaryAddress(ewm->account)));
    }

    BREthereumHash hash = walletStateGetHash(state);
    BREthereumHashString filename;
    hashFillString(hash, filename);

    eth_log ("EWM", "Wallet: Save: %s: %s",
             CLIENT_CHANGE_TYPE_NAME (type),
             filename);

    switch (type) {
        case CLIENT_CHANGE_REM:
            fileServiceRemove (ewm->fs, ewmFileServiceTypeWallets,
                               fileServiceGetIdentifier (ewm->fs, ewmFileServiceTypeWallets, state));
            break;

        case CLIENT_CHANGE_ADD:
        case CLIENT_CHANGE_UPD:
            fileServiceSave (ewm->fs, ewmFileServiceTypeWallets, state);
            break;
    }

    walletStateRelease (state);
}

extern void
ewmHandleSync (BREthereumEWM ewm,
               BREthereumBCSCallbackSyncType type,
               uint64_t blockNumberStart,
               uint64_t blockNumberCurrent,
               uint64_t blockNumberStop) {
    assert (CRYPTO_SYNC_MODE_P2P_ONLY == ewm->mode || CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC == ewm->mode);

    BRCryptoSyncPercentComplete syncCompletePercent = AS_CRYPTO_SYNC_PERCENT_COMPLETE (100.0 * (blockNumberCurrent - blockNumberStart) / (blockNumberStop - blockNumberStart));
    // We do not have blockTimestampCurrent

    BREthereumEWMEvent event;

    if (blockNumberCurrent == blockNumberStart) {
        event = (BREthereumEWMEvent) {
            EWM_EVENT_CHANGED,
            SUCCESS,
            { .changed = { ewm->state, EWM_STATE_SYNCING }}
        };
    }
    else if (blockNumberCurrent == blockNumberStop) {
        event = (BREthereumEWMEvent) {
            EWM_EVENT_CHANGED,
            SUCCESS,
            { .changed = { ewm->state, EWM_STATE_CONNECTED }}
        };
    }
    else {
        event = (BREthereumEWMEvent) {
            EWM_EVENT_SYNC_PROGRESS,
            SUCCESS,
            { .syncProgress = {
                NO_CRYPTO_SYNC_TIMESTAMP, // We do not have a timestamp
                syncCompletePercent }}
        };
    }

    ewmSignalEWMEvent (ewm, event);

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
ewmUpdateWalletBalance(BREthereumEWM ewm,
                       BREthereumWallet wallet) {

    if (NULL == wallet) {
        ewmSignalWalletEvent (ewm, wallet,
                              walletEventCreateError (WALLET_EVENT_BALANCE_UPDATED,
                                                      ERROR_UNKNOWN_WALLET,
                                                      NULL));

    } else if (ETHEREUM_BOOLEAN_IS_FALSE(ewmIsConnected(ewm))) {
        ewmSignalWalletEvent(ewm, wallet,
                             walletEventCreateError (WALLET_EVENT_BALANCE_UPDATED,
                                                     ERROR_NODE_NOT_CONNECTED,
                                                     NULL));
    } else {
        switch (ewm->mode) {
            case CRYPTO_SYNC_MODE_API_ONLY:
            case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND: {
                char *address = addressGetEncodedString(walletGetAddress(wallet), 0);

                ewm->client.funcGetBalance (ewm->client.context,
                                            ewm,
                                            wallet,
                                            address,
                                            ++ewm->requestId);

                free(address);
                break;
            }

            case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC:
            case CRYPTO_SYNC_MODE_P2P_ONLY:
                // TODO: LES Update Wallet Balance
                break;
        }
    }
}

static void
ewmUpdateBlockNumber (BREthereumEWM ewm) {
    if (ETHEREUM_BOOLEAN_IS_FALSE(ewmIsConnected(ewm))) return;
    switch (ewm->mode) {
        case CRYPTO_SYNC_MODE_API_ONLY:
        case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND: {
            ewm->client.funcGetBlockNumber (ewm->client.context,
                                            ewm,
                                            ++ewm->requestId);
            break;
        }

        case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC:
        case CRYPTO_SYNC_MODE_P2P_ONLY:
            // TODO: LES Update Wallet Balance
            break;
    }
}

static void
ewmUpdateNonce (BREthereumEWM ewm) {
    if (ETHEREUM_BOOLEAN_IS_FALSE(ewmIsConnected(ewm))) return;
    switch (ewm->mode) {
        case CRYPTO_SYNC_MODE_API_ONLY:
        case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND: {
            char *address = addressGetEncodedString(accountGetPrimaryAddress(ewm->account), 0);

            ewm->client.funcGetNonce (ewm->client.context,
                                      ewm,
                                      address,
                                      ++ewm->requestId);

            free (address);
            break;
        }

        case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC:
        case CRYPTO_SYNC_MODE_P2P_ONLY:
            // TODO: LES Update Wallet Balance
            break;
    }
}

/**
 * Update the transactions for the ewm's account.  A JSON_RPC EWM will call out to
 * BREthereumClientHandlerGetTransactions which is expected to query all transactions associated with the
 * accounts address and then the call out is to call back the 'announce transaction' callback.
 */
static void
ewmUpdateTransactions (BREthereumEWM ewm) {
    // Nothing to update if not connected.
    if (ETHEREUM_BOOLEAN_IS_FALSE(ewmIsConnected(ewm))) return;

    switch (ewm->mode) {
        case CRYPTO_SYNC_MODE_API_ONLY:
        case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND: {
            char *address = addressGetEncodedString(accountGetPrimaryAddress(ewm->account), 0);

            ewm->client.funcGetTransactions (ewm->client.context,
                                             ewm,
                                             address,
                                             ewm->brdSync.begBlockNumber,
                                             ewm->brdSync.endBlockNumber,
                                             ++ewm->requestId);

            free (address);
            break;
        }

        case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC:
        case CRYPTO_SYNC_MODE_P2P_ONLY:
            // TODO: LES Update Wallet Balance
            break;
    }
}

static const char *
ewmGetWalletContractAddress (BREthereumEWM ewm, BREthereumWallet wallet) {
    if (NULL == wallet) return NULL;

    BREthereumToken token = walletGetToken(wallet);
    return (NULL == token ? NULL : tokenGetAddress(token));
}

static void
ewmUpdateLogs (BREthereumEWM ewm,
               BREthereumWallet wid,
               BREthereumContractEvent event) {
    // Nothing to update if not connected.
    if (ETHEREUM_BOOLEAN_IS_FALSE(ewmIsConnected(ewm))) return;

    switch (ewm->mode) {
        case CRYPTO_SYNC_MODE_API_ONLY:
        case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND: {
            char *address = addressGetEncodedString(accountGetPrimaryAddress(ewm->account), 0);
            char *encodedAddress =
            eventERC20TransferEncodeAddress (event, address);
            const char *contract = ewmGetWalletContractAddress(ewm, wid);

            ewm->client.funcGetLogs (ewm->client.context,
                                     ewm,
                                     contract,
                                     encodedAddress,
                                     eventGetSelector(event),
                                     ewm->brdSync.begBlockNumber,
                                     ewm->brdSync.endBlockNumber,
                                     ++ewm->requestId);

            free (encodedAddress);
            free (address);
            break;
        }

        case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC:
        case CRYPTO_SYNC_MODE_P2P_ONLY:
            // TODO: LES Update Logs
            break;
    }
}

// ==============================================================================================
//
// Announce {Transactions, Logs} Complete
//
extern void
ewmHandleAnnounceComplete (BREthereumEWM ewm,
                           BREthereumBoolean isTransaction,
                           BREthereumBoolean success,
                           int rid) {
    if (ETHEREUM_BOOLEAN_IS_TRUE(isTransaction)) {
        // skip out if `rid` doesn't match
        if (rid != ewm->brdSync.ridTransaction) return;
        ewm->brdSync.completedTransaction = 1; // completed, no matter success or failure
    }
    else /* isLog */ {
        // skip out if `rid` doesn't match
        if (rid != ewm->brdSync.ridLog) return;
        ewm->brdSync.completedLog = 1;         // completed, no matter success or failure
    }

    // If both transaction and log are completed we'll signal complete and then, on
    // success, update `begBlockNumber`
    if (ewm->brdSync.completedTransaction && ewm->brdSync.completedLog) {
        // If this was not an 'ongoing' sync, then signal back to 'connected'
        if (ewmIsNotAnOngoingSync(ewm))
            ewmSignalEWMEvent (ewm, (BREthereumEWMEvent) {
                EWM_EVENT_CHANGED,
                SUCCESS,
                { .changed = { EWM_STATE_SYNCING, EWM_STATE_CONNECTED }}
            });

        // On success, advance the begBlockNumber
        if (ETHEREUM_BOOLEAN_IS_TRUE(success))
            ewm->brdSync.begBlockNumber = (ewm->brdSync.endBlockNumber >=  EWM_BRD_SYNC_START_BLOCK_OFFSET
                                           ? ewm->brdSync.endBlockNumber - EWM_BRD_SYNC_START_BLOCK_OFFSET
                                           : 0);
    }
}

extern void
ewmHandleSyncAPI (BREthereumEWM ewm) {
    if (ewm->state != EWM_STATE_CONNECTED) return;
    if (CRYPTO_SYNC_MODE_P2P_ONLY == ewm->mode || CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC == ewm->mode) return;

    // Get this always and early.
    ewmUpdateBlockNumber(ewm);
    ewmUpdateNonce(ewm);

    // Handle a BRD Sync:

    // 1) if a prior sync has not completed, skip out
    if (!ewm->brdSync.completedTransaction || !ewm->brdSync.completedLog) return;

    // 2) Update the `endBlockNumber` to the current block height.
    ewm->brdSync.endBlockNumber = ewmGetBlockHeight(ewm);

    // 3) if the `endBlockNumber` differs from the `begBlockNumber` then perform a 'sync'
    if (ewm->brdSync.begBlockNumber != ewm->brdSync.endBlockNumber) {

        // If this is not an 'ongoing' sync, then signal 'syncing'
        if (ewmIsNotAnOngoingSync(ewm))
            ewmSignalEWMEvent (ewm, (BREthereumEWMEvent) {
                EWM_EVENT_CHANGED,
                SUCCESS,
                { .changed = { EWM_STATE_CONNECTED, EWM_STATE_SYNCING }}
            });

        // 3a) For all the registered (aka 'known') wallets, get each balance.
        for (int i = 0; i < array_count(ewm->wallets); i++)
            ewmUpdateWalletBalance (ewm, ewm->wallets[i]);

        // If this is not an 'ongoing' sync, then arbitrarily report progress - half way
        // between transactions and logs
        if (ewmIsNotAnOngoingSync(ewm))
            ewmSignalEWMEvent (ewm, (BREthereumEWMEvent) {
                EWM_EVENT_SYNC_PROGRESS,
                SUCCESS,
                { .syncProgress = {
                    NO_CRYPTO_SYNC_TIMESTAMP, // We do not have a timestamp
                    AS_CRYPTO_SYNC_PERCENT_COMPLETE(33.33) }}
            });


        // 3b) We'll query all transactions for this ewm's account.  That will give us a shot at
        // getting the nonce for the account's address correct.  We'll save all the transactions and
        // then process them into wallet as wallets exist.
        ewmUpdateTransactions(ewm);

        // Record an 'update transaction' as in progress
        ewm->brdSync.ridTransaction = ewm->requestId;
        ewm->brdSync.completedTransaction = 0;

        // If this is not an 'ongoing' sync, then arbitrarily report progress - half way
        // between transactions and logs
        if (ewmIsNotAnOngoingSync(ewm))
            ewmSignalEWMEvent (ewm, (BREthereumEWMEvent) {
                EWM_EVENT_SYNC_PROGRESS,
                SUCCESS,
                { .syncProgress = {
                    NO_CRYPTO_SYNC_TIMESTAMP, // We do not have a timestamp
                    AS_CRYPTO_SYNC_PERCENT_COMPLETE(66.67) }}
            });

        // 3c) Similarly, we'll query all logs for this ewm's account.  We'll process these into
        // (token) transactions and associate with their wallet.
        ewmUpdateLogs(ewm, NULL, eventERC20Transfer);

        // Record an 'update log' as in progress
        ewm->brdSync.ridLog = ewm->requestId;
        ewm->brdSync.completedLog = 0;
    }

    // End handling a BRD Sync

    if (NULL != ewm->bcs) bcsClean (ewm->bcs);
}

//
// Periodicaly query the BRD backend to get current status (block number, nonce, balances,
// transactions and logs) The event will be NULL (as specified for a 'period dispatcher' - See
// `eventHandlerSetTimeoutDispatcher()`)
//
static void
ewmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event) {
    BREthereumEWM ewm = (BREthereumEWM) event->context;
    ewmHandleSyncAPI(ewm);
}

extern void
ewmTransferFillRawData (BREthereumEWM ewm,
                        BREthereumWallet wallet,
                        BREthereumTransfer transfer,
                        uint8_t **bytesPtr, size_t *bytesCountPtr) {
    assert (NULL != bytesCountPtr && NULL != bytesPtr);

    pthread_mutex_lock (&ewm->lock);
    assert (walletHasTransfer(wallet, transfer));

    BREthereumTransaction transaction = transferGetOriginatingTransaction (transfer);
    assert (NULL != transaction);
    assert (ETHEREUM_BOOLEAN_IS_TRUE (transactionIsSigned(transaction)));
    pthread_mutex_unlock (&ewm->lock);

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
    
    pthread_mutex_lock (&ewm->lock);
    BREthereumTransaction transaction = transferGetOriginatingTransaction (transfer);
    pthread_mutex_unlock (&ewm->lock);
    
    return (NULL == transaction ? NULL
            : transactionGetRlpHexEncoded (transaction,
                                           ewm->network,
                                           (ETHEREUM_BOOLEAN_IS_TRUE (transactionIsSigned(transaction))
                                            ? RLP_TYPE_TRANSACTION_SIGNED
                                            : RLP_TYPE_TRANSACTION_UNSIGNED),
                                           prefix));
}

/// MARK: - Transfer

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
ewmTransferGetIdentifier(BREthereumEWM ewm,
                         BREthereumTransfer transfer) {
    pthread_mutex_lock (&ewm->lock);
    BREthereumHash identifier = transferGetIdentifier (transfer);
    pthread_mutex_unlock (&ewm->lock);
    return identifier;
}

extern BREthereumHash
ewmTransferGetOriginatingTransactionHash(BREthereumEWM ewm,
                                         BREthereumTransfer transfer) {
    pthread_mutex_lock (&ewm->lock);
    BREthereumHash hash = transferGetOriginatingTransactionHash(transfer);
    pthread_mutex_unlock (&ewm->lock);
    return hash;
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

extern BREthereumFeeBasis
ewmTransferGetFeeBasis (BREthereumEWM ewm,
                        BREthereumTransfer transfer) {
    return transferGetFeeBasis (transfer);
}

extern uint64_t
ewmTransferGetNonce(BREthereumEWM ewm,
                    BREthereumTransfer transfer) {
    pthread_mutex_lock (&ewm->lock);
    uint64_t nonce = transferGetNonce(transfer);
    pthread_mutex_unlock (&ewm->lock);
    return nonce;
}

extern BREthereumBoolean
ewmTransferExtractStatusIncluded (BREthereumEWM ewm,
                                  BREthereumTransfer transfer,
                                  BREthereumHash *blockHash,
                                  uint64_t *blockNumber,
                                  uint64_t *blockTransactionIndex,
                                  uint64_t *blockTimestamp,
                                  BREthereumGas *gasUsed) {
    pthread_mutex_lock (&ewm->lock);
    int included = transferExtractStatusIncluded (transfer,
                                                  blockHash,
                                                  blockNumber,
                                                  blockTransactionIndex,
                                                  blockTimestamp,
                                                  gasUsed);
    pthread_mutex_unlock (&ewm->lock);

    return AS_ETHEREUM_BOOLEAN (included);
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
ewmTransferGetTransactionIndex(BREthereumEWM ewm,
                               BREthereumTransfer transfer) {
    uint64_t transactionIndex;
    return (transferExtractStatusIncluded(transfer, NULL, NULL, &transactionIndex, NULL, NULL)
            ? transactionIndex
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

extern BREthereumGas
ewmTransferGetGasUsed(BREthereumEWM ewm,
                      BREthereumTransfer transfer) {
    BREthereumGas gasUsed;
    return (transferExtractStatusIncluded(transfer, NULL, NULL, NULL, NULL, &gasUsed)
            ? gasUsed
            : gasCreate(0));
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
    return AS_ETHEREUM_BOOLEAN (ETHEREUM_BOOLEAN_IS_TRUE (transferHasStatus (transfer, TRANSFER_STATUS_SUBMITTED)) ||
                                ETHEREUM_BOOLEAN_IS_TRUE (transferHasStatusOrTwo (transfer,
                                                                                  TRANSFER_STATUS_INCLUDED,
                                                                                  TRANSFER_STATUS_ERRORED)));
}

extern char *
ewmTransferStatusGetError (BREthereumEWM ewm,
                           BREthereumTransfer transfer) {
    char *reason = NULL;

    pthread_mutex_lock (&ewm->lock);
    if (TRANSFER_STATUS_ERRORED == transferGetStatus(transfer))
        transferExtractStatusError (transfer, &reason);
    pthread_mutex_unlock (&ewm->lock);

    return reason;
}

extern int
ewmTransferStatusGetErrorType (BREthereumEWM ewm,
                               BREthereumTransfer transfer) {
    BREthereumTransactionErrorType type = (BREthereumTransactionErrorType) -1;

    pthread_mutex_lock (&ewm->lock);
    transferExtractStatusErrorType (transfer, &type);
    pthread_mutex_unlock (&ewm->lock);

    return type;
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

    pthread_mutex_lock (&ewm->lock);
    BREthereumEther fee = transferGetFee(transfer, overflow);
    pthread_mutex_unlock (&ewm->lock);

    return fee;
}

/// MARK: - Amount

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

/// MARK: - Gas Price / Limit

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
    pthread_mutex_lock (&ewm->lock);
    for (int wid = 0; wid < array_count(ewm->wallets); wid++) {
        BREthereumWallet wallet = ewm->wallets[wid];
        if (walletHasTransfer(wallet, transfer)) {
            walletUnhandleTransfer(wallet, transfer);
            ewmSignalTransferEvent(ewm, wallet, transfer, (BREthereumTransferEvent) {
                TRANSFER_EVENT_DELETED,
                SUCCESS
            });
        }
    }
    // Null the ewm's `tid` - MUST NOT array_rm() as all `tid` holders will be dead.
    transferRelease(transfer);
    pthread_mutex_unlock (&ewm->lock);
}

extern BREthereumToken
ewmLookupToken (BREthereumEWM ewm,
                BREthereumAddress address) {
    pthread_mutex_lock (&ewm->lock);
    BREthereumToken token = (BREthereumToken) BRSetGet (ewm->tokens, &address);
    pthread_mutex_unlock (&ewm->lock);
    return token;
}

extern BREthereumToken
ewmCreateToken (BREthereumEWM ewm,
                const char *address,
                const char *symbol,
                const char *name,
                const char *description,
                int decimals,
                BREthereumGas defaultGasLimit,
                BREthereumGasPrice defaultGasPrice) {
    if (NULL == address || 0 == strlen(address)) return NULL;
    if (ETHEREUM_BOOLEAN_FALSE == addressValidateString(address)) return NULL;

    // This function is called in potentially two threads.  One in EWM event handler (on
    // `ewmHandleAnnounceToken()`) and one in `cryptoWalletManagerInstall...()` (on some App
    // listener thread).  Such a description, used here, is troubling in and of itself.

    BREthereumAddress addr = addressCreate(address);

    // Lock over BRSetGet(), BRSetAdd() and tokenUpdate()
    pthread_mutex_lock (&ewm->lock);
    BREthereumToken token = (BREthereumToken) BRSetGet (ewm->tokens, &addr);
    if (NULL == token) {
        token = tokenCreate (address,
                             symbol,
                             name,
                             description,
                             decimals,
                             defaultGasLimit,
                             defaultGasPrice);
        BRSetAdd (ewm->tokens, token);
    }
    else {
        tokenUpdate (token,
                     symbol,
                     name,
                     description,
                     decimals,
                     defaultGasLimit,
                     defaultGasPrice);
    }
    pthread_mutex_unlock (&ewm->lock);

    fileServiceSave (ewm->fs, ewmFileServiceTypeTokens, token);
    return token;
}
