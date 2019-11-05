//
//  BRGenericWalletManager.c
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

#include "BRGeneric.h"

#include "support/BRFileService.h"
#include "ethereum/event/BREvent.h"
#include "ethereum/event/BREventAlarm.h"

static void
gwmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event);

extern const BREventType *gwmEventTypes[];
extern const unsigned int gwmEventTypesCount;

#define GWM_BRD_SYNC_START_BLOCK_OFFSET     1000

#if !defined (MAX)
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

static void
gwmInstallFileService (BRGenericWalletManager gwm,
                       const char *storagePath,
                       const char *currencyName,
                       const char *networkName);

///
///
///
struct BRGenericWalletManagerRecord {
    BRGenericHandlers handlers;
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

extern BRGenericWalletManager
gwmCreate (BRGenericClient client,
           const char *type,
           BRGenericAccount account,
           uint64_t accountTimestamp,
           const char *storagePath,
           uint32_t syncPeriodInSeconds,
           uint64_t blockHeight) {
    BRGenericWalletManager gwm = calloc (1, sizeof (struct BRGenericWalletManagerRecord));

    gwm->handlers = genericHandlerLookup (type);
    assert (NULL != gwm->handlers);

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
    gwmInstallFileService (gwm, storagePath, type, "mainnet");

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
                                      (BREventDispatcher) gwmPeriodicDispatcher,
                                      (void*) gwm);

    // Events ...

    return gwm;
}

extern void
gwmRelease (BRGenericWalletManager gwm) {
    gwmDisconnect (gwm);
    free (gwm);
}

extern void
gwmStop (BRGenericWalletManager gwm) {
    eventHandlerStop (gwm->handler);
    fileServiceClose (gwm->fileService);
}

extern BRGenericHandlers
gwmGetHandlers (BRGenericWalletManager gwm) {
    return gwm->handlers;
}

extern BRGenericAccount
gwmGetAccount (BRGenericWalletManager gwm) {
    return gwm->account;
}

extern void
gwmConnect (BRGenericWalletManager gwm) {
    eventHandlerStart (gwm->handler);
    // Event
}

extern void
gwmDisconnect (BRGenericWalletManager gwm) {
    gwmStop (gwm);  // This is questionable.
    // Event

}

extern void
gwmSync (BRGenericWalletManager gwm) {
    return;
}


extern BRGenericAddress
gwmGetAccountAddress (BRGenericWalletManager gwm) {
    return gwmAccountGetAddress (gwm->account);
}

extern BRGenericWallet
gwmCreatePrimaryWallet (BRGenericWalletManager gwm) {
    return gwmWalletCreate(gwm);
}

extern BRGenericTransfer
gwmRecoverTransfer (BRGenericWalletManager gwm,
                    uint8_t *bytes,
                    size_t   bytesCount) {
    return gwmGetHandlers(gwm)->manager.transferRecover (bytes, bytesCount);
}

extern BRArrayOf(BRGenericTransfer)
gwmLoadTransfers (BRGenericWalletManager gwm) {
    return gwm->handlers->manager.fileServiceLoadTransfers (gwm, gwm->fileService);
}

/// MARK: Periodic Dispatcher

static void
gwmPeriodicDispatcher (BREventHandler handler,
                       BREventTimeout *event) {
    BRGenericWalletManager gwm = (BRGenericWalletManager) event->context;

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
        BRGenericAddress addressGen = gwmGetAccountAddress(gwm);
        char *address = gwmAddressAsString (gwm, addressGen);

        // 3a) Save the current requestId
        gwm->brdSync.rid = gwm->requestId;

        // 3b) Query all transactions; each one found will have bwmAnnounceTransaction() invoked
        // which will process the transaction into the wallet.

        // Callback to 'client' to get all transactions (for all wallet addresses) between
        // a {beg,end}BlockNumber.  The client will gather the transactions and then call
        // bwmAnnounceTransaction()  (for each one or with all of them).
        gwm->client.getTransactions (gwm->client.context,
                                     gwm,
                                     address,
                                     gwm->brdSync.begBlockNumber,
                                     gwm->brdSync.endBlockNumber,
                                     gwm->requestId++);

        // TODO: Handle address
        // free (address);

        // 3c) Mark as not completed
        gwm->brdSync.completed = 0;
    }

    // End handling a BRD Sync
}

extern void
gwmWipe (const char *type,
         const char *storagePath) {
    fileServiceWipe (storagePath, type, "mainnet");
}

/// MARK: - Announce

// handle transfer
// signal transfer

extern int
gwmAnnounceBlockNumber (BRGenericWalletManager manager,
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
gwmAnnounceTransfer (BRGenericWalletManager manager,
                     int rid,
                     BRGenericTransfer transfer) {
    // Add transfer ?? EVent
    return 1;
}

extern void
gwmAnnounceTransferComplete (BRGenericWalletManager manager,
                             int rid,
                             int success) {
    pthread_mutex_lock (&manager->lock);
    if (rid == manager->brdSync.rid)
        manager->brdSync.completed = success;
    pthread_mutex_unlock (&manager->lock);
}

extern void
gwmAnnounceSubmit (BRGenericWalletManager manager,
                   int rid,
                   BRGenericTransfer transfer,
                   int error) {
    // Event
}

static void
gwmInstallFileService(BRGenericWalletManager gwm,
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

