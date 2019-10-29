//
//  BRGenericManager.c
//  BRCore
//
//  Created by Ed Gamble on 6/20/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "BRGenericPrivate.h"

#include "support/BRFileService.h"
#include "ethereum/event/BREvent.h"
#include "ethereum/event/BREventAlarm.h"

static void
genManagerPeriodicDispatcher (BREventHandler handler,
                              BREventTimeout *event);

extern const BREventType *gwmEventTypes[];
extern const unsigned int gwmEventTypesCount;

#define GWM_BRD_SYNC_START_BLOCK_OFFSET     1000

#if !defined (MAX)
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

static void
genManagerInstallFileService (BRGenericManager gwm,
                              const char *storagePath,
                              const char *currencyName,
                              const char *networkName);

///
///
///
struct BRGenericManagerRecord {
    BRGenericHandlers handlers;
    BRGenericNetwork network;
    BRGenericAccount account;
    BRGenericClient client;
    char *storagePath;

    /** The file service */
    BRFileService fileService;

    /**
     * The BlockHeight is the largest block number seen
     */
    uint32_t blockHeight;

    /**
     * An identiifer for a BRD Request
     */
    unsigned int requestId;

    /**
     * An EventHandler for Main.  All 'announcements' (via PeerManager (or BRD) hit here.
     */
    BREventHandler handler;

    /**
     * The Lock ensuring single thread access to BWM state.
     */
    pthread_mutex_t lock;

    /**
     * If we are syncing with BRD, instead of as P2P with PeerManager, then we'll keep a record to
     * ensure we've successfully completed the getTransactions() callbacks to the client.
     */
    struct {
        uint64_t begBlockNumber;
        uint64_t endBlockNumber;

        int rid;

        int completed:1;
    } brdSync;
};

extern BRGenericManager
genManagerCreate (BRGenericClient client,
                  const char *type,
                  BRGenericNetwork network,
                  BRGenericAccount account,
                  uint64_t accountTimestamp,
                  const char *storagePath,
                  uint32_t syncPeriodInSeconds,
                  uint64_t blockHeight) {
    BRGenericManager gwm = calloc (1, sizeof (struct BRGenericManagerRecord));

    gwm->handlers = genHandlerLookup (type);
    assert (NULL != gwm->handlers);

    gwm->network = network;
    gwm->account = account;
    gwm->client  = client;
    gwm->storagePath = strdup (storagePath);
    gwm->blockHeight = (uint32_t) blockHeight;
    gwm->requestId = 0;

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&gwm->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // Create the alarm clock, but don't start it.
    alarmClockCreateIfNecessary(0);

    char handlerName[5 + strlen(type) + 1], *hp = &handlerName[4]; // less 1
    sprintf (handlerName, "Core %s", type);
    while (*++hp) *hp = toupper (*hp);

    // The `main` event handler has a periodic wake-up.  Used, perhaps, if the mode indicates
    // that we should/might query the BRD backend services.
    gwm->handler = eventHandlerCreate (handlerName,
                                       gwmEventTypes,
                                       gwmEventTypesCount,
                                       &gwm->lock);

    // File Service
    genManagerInstallFileService (gwm, storagePath, type, "mainnet");

    // Wallet ??

    // Earliest blockHeight from accountTimestamp.
    uint64_t earliestBlockNumber = 0;

    // Initialize the `brdSync` struct
    gwm->brdSync.rid = -1;
    gwm->brdSync.begBlockNumber = earliestBlockNumber;
    gwm->brdSync.endBlockNumber = MAX (earliestBlockNumber, blockHeight);
    gwm->brdSync.completed = 0;

    eventHandlerSetTimeoutDispatcher (gwm->handler,
                                      1000 * syncPeriodInSeconds,
                                      (BREventDispatcher) genManagerPeriodicDispatcher,
                                      (void*) gwm);

    // Events ...

    return gwm;
}

extern void
genManagerRelease (BRGenericManager gwm) {
    genManagerDisconnect (gwm);
    free (gwm);
}

extern void
genManagerStop (BRGenericManager gwm) {
    eventHandlerStop (gwm->handler);
    fileServiceClose (gwm->fileService);
}

extern BRGenericNetwork
genManagerGetNetwork (BRGenericManager gwm) {
    return gwm->network;
}

extern BRGenericAccount
genManagerGetAccount (BRGenericManager gwm) {
    return gwm->account;
}

extern BRGenericClient
genManagerGetClient (BRGenericManager gwm) {
    return gwm->client;
}

extern void
genManagerConnect (BRGenericManager gwm) {
    eventHandlerStart (gwm->handler);
    // Event
}

extern void
genManagerDisconnect (BRGenericManager gwm) {
    genManagerStop (gwm);  // This is questionable.
                           // Event
}

extern void
genManagerSync (BRGenericManager gwm) {
    return;
}

extern BRGenericAddress
genManagerGetAccountAddress (BRGenericManager gwm) {
    return genAccountGetAddress (gwm->account);
}

extern BRGenericWallet
genManagerCreatePrimaryWallet (BRGenericManager gwm) {
    return genWalletCreate(gwm->account);
}

extern int
genManagerSignTransfer (BRGenericManager gwm,
                        BRGenericWallet wid,
                        BRGenericTransfer transfer,
                        UInt512 seed) {
    genAccountSignTransferWithSeed (gwm->account, transfer, seed);
    return 1;
}

extern int
genManagerSignTransferWithKey (BRGenericManager gwm,
                               BRGenericWallet wid,
                               BRGenericTransfer transfer,
                               BRKey *key) {
    genAccountSignTransferWithKey (gwm->account, transfer, key);
    return 1;
}

extern void
genManagerSubmitTransfer (BRGenericManager gwm,
                          BRGenericWallet wid,
                          BRGenericTransfer transfer) {
    // Get the serialization, as raw bytes', for the transfer.  We assert if the raw bytes
    // don't exist which implies that transfer was not signed.
    size_t txSize = 0;
    uint8_t * tx = genTransferSerialize(transfer, &txSize);
    assert (NULL != tx);

    // Get the hash
    BRGenericHash hash = genTransferGetHash(transfer);

    // Submit the raw bytes to the client.
    BRGenericClient client = genManagerGetClient(gwm);
    client.submitTransaction (client.context, gwm, wid, transfer, tx, txSize, hash, 0);
}

extern BRGenericTransfer
genManagerRecoverTransfer (BRGenericManager gwm,
                           BRGenericWallet wallet,
                           const char *hash,
                           const char *from,
                           const char *to,
                           const char *amount,
                           const char *currency,
                           const char *fee,
                           uint64_t timestamp,
                           uint64_t blockHeight) {
    BRGenericTransfer transfer = genTransferAllocAndInit (gwm->handlers->type,
                                                          gwm->handlers->manager.transferRecover (hash, from, to, amount, currency, fee, timestamp, blockHeight));

    BRGenericAddress  source   = genTransferGetSourceAddress (transfer);
    BRGenericAddress  target   = genTransferGetTargetAddress (transfer);
    BRGenericFeeBasis feeBasis = genTransferGetFeeBasis (transfer);

    int isSource = genWalletHasAddress (wallet, source);
    int isTarget = genWalletHasAddress (wallet, target);

    transfer->direction = (isSource && isTarget
                           ? GENERIC_TRANSFER_RECOVERED
                           : (isSource
                              ? GENERIC_TRANSFER_SENT
                              : GENERIC_TRANSFER_RECEIVED));

    genTransferSetState (transfer,
                         genTransferStateCreateIncluded (blockHeight,
                                                         GENERIC_TRANSFER_TRANSACTION_INDEX_UNKNOWN,
                                                         timestamp,
                                                         feeBasis));

    genAddressRelease (source);
    genAddressRelease (target);
    return transfer;
}

extern BRArrayOf(BRGenericTransfer)
genManagerRecoverTransfersFromRawTransaction (BRGenericManager gwm,
                                              uint8_t *bytes,
                                              size_t   bytesCount,
                                              uint64_t timestamp,
                                              uint64_t blockHeight) {
    pthread_mutex_lock (&gwm->lock);
    BRArrayOf(BRGenericTransferRef) refs = gwm->handlers->manager.transfersRecoverFromRawTransaction (bytes, bytesCount);
    BRArrayOf(BRGenericTransfer) transfers;
    array_new (transfers, array_count(refs));
    for (size_t index = 0; index < array_count(refs); index++) {
        BRGenericTransfer transfer = genTransferAllocAndInit (gwm->handlers->type, refs[index]);
        genTransferSetState (transfer,
                             genTransferStateCreateIncluded (blockHeight,
                                                             GENERIC_TRANSFER_TRANSACTION_INDEX_UNKNOWN,
                                                             timestamp,
                                                             genTransferGetFeeBasis (transfer)));
        array_add (transfers, transfer);
    }
    pthread_mutex_unlock (&gwm->lock);
    return transfers;
}

extern BRArrayOf(BRGenericTransfer)
genManagerLoadTransfers (BRGenericManager gwm) {
    pthread_mutex_lock (&gwm->lock);
    BRArrayOf(BRGenericTransferRef) refs = gwm->handlers->manager.fileServiceLoadTransfers (gwm, gwm->fileService);
    BRArrayOf(BRGenericTransfer) objs;
    array_new (objs, array_count(refs));
    for (size_t index = 0; index < array_count(refs); index++) {
        BRGenericTransfer obj = genTransferAllocAndInit (gwm->handlers->type, refs[index]);
        array_add (objs, obj);
    }
    pthread_mutex_unlock (&gwm->lock);
    return objs;

}

/// MARK: Periodic Dispatcher

static void
genManagerPeriodicDispatcher (BREventHandler handler,
                              BREventTimeout *event) {
    BRGenericManager gwm = (BRGenericManager) event->context;

    gwm->client.getBlockNumber (gwm->client.context,
                                gwm,
                                gwm->requestId++);

    // Handle a BRD Sync:

    // 1) check if the prior sync has completed.
    if (gwm->brdSync.completed) {
        // 1a) if so, advance the sync range by updating `begBlockNumber`
        gwm->brdSync.begBlockNumber = (gwm->brdSync.endBlockNumber >=  GWM_BRD_SYNC_START_BLOCK_OFFSET
                                       ? gwm->brdSync.endBlockNumber - GWM_BRD_SYNC_START_BLOCK_OFFSET
                                       : 0);
    }

    // 2) completed or not, update the `endBlockNumber` to the current block height.
    gwm->brdSync.endBlockNumber = MAX (gwm->blockHeight, gwm->brdSync.begBlockNumber);

    // 3) we'll update transactions if there are more blocks to examine
    if (gwm->brdSync.begBlockNumber != gwm->brdSync.endBlockNumber) {
        char *address = genAddressAsString (genManagerGetAccountAddress(gwm));
        
        // 3a) Save the current requestId
        gwm->brdSync.rid = gwm->requestId;

        // 3b) Query all transactions; each one found will have bwmAnnounceTransaction() invoked
        // which will process the transaction into the wallet.

        // Callback to 'client' to get all transactions (for all wallet addresses) between
        // a {beg,end}BlockNumber.  The client will gather the transactions and then call
        // bwmAnnounceTransaction()  (for each one or with all of them).
        if (gwm->handlers->manager.apiSyncType() == GENERIC_SYNC_TYPE_TRANSFER) {
            gwm->client.getTransfers (gwm->client.context,
                                      gwm,
                                      address,
                                      gwm->brdSync.begBlockNumber,
                                      gwm->brdSync.endBlockNumber,
                                      gwm->requestId++);
        } else {
            gwm->client.getTransactions (gwm->client.context,
                                         gwm,
                                         address,
                                         gwm->brdSync.begBlockNumber,
                                         gwm->brdSync.endBlockNumber,
                                         gwm->requestId++);
        }

        // TODO: Handle address
        // free (address);

        // 3c) Mark as not completed
        gwm->brdSync.completed = 0;
    }

    // End handling a BRD Sync
}

/// MARK: - Announce

// handle transfer
// signal transfer

extern int
genManagerAnnounceBlockNumber (BRGenericManager manager,
                               int rid,
                               uint64_t height) {
    pthread_mutex_lock (&manager->lock);
    if (height != manager->blockHeight) {
        manager->blockHeight = (uint32_t) height;
        // event
    }
    pthread_mutex_unlock (&manager->lock);
    return 1;
}

extern int // success - data is valid
genManagerAnnounceTransfer (BRGenericManager manager,
                            int rid,
                            BRGenericTransfer transfer) {
    // Add transfer ?? EVent
    return 1;
}

extern void
genManagerAnnounceTransferComplete (BRGenericManager manager,
                                    int rid,
                                    int success) {
    pthread_mutex_lock (&manager->lock);
    if (rid == manager->brdSync.rid)
        manager->brdSync.completed = success;
    pthread_mutex_unlock (&manager->lock);
}

extern void
genManagerAnnounceSubmit (BRGenericManager manager,
                          int rid,
                          BRGenericTransfer transfer,
                          int error) {
    // Event
}

static void
genManagerInstallFileService(BRGenericManager gwm,
                             const char *storagePath,
                             const char *currencyName,
                             const char *networkName) {
    gwm->fileService = fileServiceCreate (storagePath, currencyName, networkName, gwm, NULL);

    gwm->handlers->manager.fileServiceInit (gwm, gwm->fileService);
}

/// MARK: - Events

const BREventType *gwmEventTypes[] = {
    // ...
};

const unsigned int
gwmEventTypesCount = (sizeof (gwmEventTypes) / sizeof (BREventType*));
