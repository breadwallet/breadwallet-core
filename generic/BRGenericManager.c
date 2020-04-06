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
#include <stdbool.h>

#include "BRGenericPrivate.h"

#include "support/BRFileService.h"
#include "ethereum/event/BREvent.h"
#include "ethereum/event/BREventAlarm.h"
#include "ethereum/rlp/BRRlp.h"
#include "ethereum/util/BRUtil.h"

static void
genManagerPeriodicDispatcher (BREventHandler handler,
                              BREventTimeout *event);

extern const BREventType *gwmEventTypes[];
extern const unsigned int gwmEventTypesCount;

#define GWM_BRD_SYNC_START_BLOCK_OFFSET     1000

#if !defined (MAX)
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

static BRRlpItem
genFeeBasisEncode (BRGenericFeeBasis feeBasis,
                   BRRlpCoder coder);

static BRGenericFeeBasis
genFeeBasisDecode (BRRlpItem item,
                   BRRlpCoder coder);

static OwnershipGiven BRArrayOf(BRGenericTransferAttribute)
genTransferAttributesDecode (BRRlpItem item,
                             BRRlpCoder coder);

static BRRlpItem
genTransferAttributesEncode (OwnershipKept BRArrayOf(BRGenericTransferAttribute) attributes,
                             BRRlpCoder coder);
///
///
///
struct BRGenericManagerRecord {
    BRGenericHandlers handlers;
    BRGenericNetwork network;
    BRGenericAccount account;
    BRGenericClient client;
    char *storagePath;

    /** The primary wallet */
    BRGenericWallet wallet;

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

        bool completed;
        bool success;
    } brdSync;

    BRGenericManagerSyncContext  syncContext;
    BRGenericManagerSyncCallback syncCallback;
};


/// MARK: - File Service

#define fileServiceTypeTransactions      "transactions"

typedef enum {
    GENERIC_TRANSFER_VERSION_1,
    GENERIC_TRANSFER_VERSION_2,
} BRGenericFileServiceTransferVersion;

static UInt256
fileServiceTypeTransferV1Identifier (BRFileServiceContext context,
                                     BRFileService fs,
                                     const void *entity) {
    BRGenericTransfer transfer = (BRGenericTransfer) entity;
    return genTransferGetHash (transfer).value;
}

static void *
fileServiceTypeTransferV1Reader (BRFileServiceContext context,
                                 BRFileService fs,
                                 uint8_t *bytes,
                                 uint32_t bytesCount) {
    BRGenericManager gwm = (BRGenericManager) context;

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpData  data  = (BRRlpData) { bytesCount, bytes };
    BRRlpItem  item  = rlpGetItem (coder, data);

    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);
    assert (9 == itemsCount);

    BRRlpData hashData = rlpDecodeBytes(coder, items[0]);
    char *strUids   = rlpDecodeString  (coder, items[1]);
    char *strSource = rlpDecodeString  (coder, items[2]);
    char *strTarget = rlpDecodeString  (coder, items[3]);
    UInt256 amount  = rlpDecodeUInt256 (coder, items[4], 0);
    char *currency  = rlpDecodeString  (coder, items[5]);
    BRGenericFeeBasis feeBasis = genFeeBasisDecode (items[6], coder);
    BRGenericTransferState state = genTransferStateDecode (items[7], coder);
    BRArrayOf(BRGenericTransferAttribute) attributes = genTransferAttributesDecode(items[8], coder);

    BRGenericHash *hash = (BRGenericHash*) hashData.bytes;
    char *strHash   = genericHashAsString (*hash);

    char *strAmount = coerceString (amount, 10);

    int overflow = 0;
    UInt256 fee = genFeeBasisGetFee (&feeBasis, &overflow);
    assert (!overflow);
    char *strFee = coerceString (fee,    10);

    uint64_t timestamp = (GENERIC_TRANSFER_STATE_INCLUDED == state.type
                          ? state.u.included.timestamp
                          : GENERIC_TRANSFER_TIMESTAMP_UNKNOWN);

    uint64_t blockHeight = (GENERIC_TRANSFER_STATE_INCLUDED == state.type
                            ? state.u.included.blockNumber
                            : GENERIC_TRANSFER_BLOCK_NUMBER_UNKNOWN);

    // Derive `wallet` from currency
    BRGenericWallet  wallet = genManagerGetPrimaryWallet (gwm);

    BRGenericTransfer transfer = genManagerRecoverTransfer (gwm, wallet, strHash,
                                                            strUids,
                                                            strSource,
                                                            strTarget,
                                                            strAmount,
                                                            currency,
                                                            strFee,
                                                            timestamp,
                                                            blockHeight,
                                                            GENERIC_TRANSFER_STATE_ERRORED == state.type);

    // Set the transfer's `state` and `attributes` from the read values.  For`state`, this will
    // overwrite what `genManagerRecoverTransfer()` assigned but will be correct with the saved
    // values.  Later, perhaps based on a BlocksetDB query, the state change to 'included error'.
    genTransferSetState (transfer, state);
    genTransferSetAttributes (transfer, attributes);

    genTransferAttributeReleaseAll(attributes);
    free (strFee);
    free (strAmount);
    free (strHash);
    free (currency);
    free (strTarget);
    free (strSource);
    free (strUids);

    rlpReleaseItem (coder, item);
    rlpCoderRelease(coder);

    return transfer;
}

static uint8_t *
fileServiceTypeTransferWriter (BRFileServiceContext context,
                               BRFileService fs,
                               const void* entity,
                               uint32_t *bytesCount,
                               BRGenericFileServiceTransferVersion version) {
    BRGenericTransfer transfer = (BRGenericTransfer) entity;

    BRGenericHash    hash   = genTransferGetHash (transfer);
    BRGenericAddress source = genTransferGetSourceAddress (transfer);
    BRGenericAddress target = genTransferGetTargetAddress (transfer);
    UInt256 amount = genTransferGetAmount (transfer);
    BRGenericFeeBasis feeBasis = genTransferGetFeeBasis(transfer);
    BRGenericTransferState state = genTransferGetState (transfer);

    // Code it Up!
    BRRlpCoder coder = rlpCoderCreate();

    char *strSource = genAddressAsString(source);
    char *strTarget = genAddressAsString(target);

    BRGenericTransferStateEncodeVersion stateEncodeVersion =
        (GENERIC_TRANSFER_VERSION_1 == version ? GEN_TRANSFER_STATE_ENCODE_V1
         : (GENERIC_TRANSFER_VERSION_2 == version ? GEN_TRANSFER_STATE_ENCODE_V2
            : GEN_TRANSFER_STATE_ENCODE_V1));

    BRRlpItem item = rlpEncodeList (coder, 9,
                                    rlpEncodeBytes (coder, hash.value.u8, sizeof (hash.value.u8)),
                                    rlpEncodeString (coder, transfer->uids),
                                    rlpEncodeString (coder, strSource),
                                    rlpEncodeString (coder, strTarget),
                                    rlpEncodeUInt256 (coder, amount, 0),
                                    rlpEncodeString (coder, transfer->type),
                                    genFeeBasisEncode (feeBasis, coder),
                                    genTransferStateEncode (state, stateEncodeVersion, coder),
                                    genTransferAttributesEncode (transfer->attributes, coder));

    BRRlpData data = rlpGetData (coder, item);

    rlpReleaseItem (coder, item);
    rlpCoderRelease (coder);

    free (strSource); genAddressRelease (source);
    free (strTarget); genAddressRelease (target);

    *bytesCount = (uint32_t) data.bytesCount;
    return data.bytes;
}

static uint8_t *
fileServiceTypeTransferV1Writer (BRFileServiceContext context,
                                 BRFileService fs,
                                 const void* entity,
                                 uint32_t *bytesCount) {
    return fileServiceTypeTransferWriter (context, fs, entity, bytesCount, GENERIC_TRANSFER_VERSION_1);
}

static uint8_t *
fileServiceTypeTransferV2Writer (BRFileServiceContext context,
                                 BRFileService fs,
                                 const void* entity,
                                 uint32_t *bytesCount) {
    return fileServiceTypeTransferWriter (context, fs, entity, bytesCount, GENERIC_TRANSFER_VERSION_2);
}

static BRFileServiceTypeSpecification fileServiceSpecifications[] = {
    {
        fileServiceTypeTransactions,
        GENERIC_TRANSFER_VERSION_2, // current version
        2,
        {
            {
                GENERIC_TRANSFER_VERSION_1,
                fileServiceTypeTransferV1Identifier,
                fileServiceTypeTransferV1Reader,
                fileServiceTypeTransferV1Writer
            },

            {
                GENERIC_TRANSFER_VERSION_2,
                fileServiceTypeTransferV1Identifier,
                fileServiceTypeTransferV1Reader,
                fileServiceTypeTransferV2Writer
            },
        }
    }
};
static size_t fileServiceSpecificationsCount = (sizeof (fileServiceSpecifications) / sizeof (BRFileServiceTypeSpecification));

/// MARK: - Manager

extern BRGenericManager
genManagerCreate (BRGenericClient client,
                  const char *type,
                  BRGenericNetwork network,
                  BRGenericAccount account,
                  uint64_t accountTimestamp,
                  const char *storagePath,
                  uint32_t syncPeriodInSeconds,
                  BRGenericManagerSyncContext  syncContext,
                  BRGenericManagerSyncCallback syncCallback,
                  uint64_t blockHeight) {
    BRGenericManager gwm = calloc (1, sizeof (struct BRGenericManagerRecord));

    gwm->handlers = genHandlerLookup (type);
    assert (NULL != gwm->handlers);

    gwm->network = network;
    gwm->account = account;
    gwm->client  = client;
    gwm->wallet  = genWalletCreate (account);
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
    const char *networkName  = (genNetworkIsMainnet (gwm->network) ? "mainnet" : "testnet");
    const char *currencyCode = type;

    gwm->fileService =  fileServiceCreateFromTypeSpecfications (storagePath, currencyCode, networkName,
                                                   gwm,
                                                   NULL,
                                                   fileServiceSpecificationsCount,
                                                   fileServiceSpecifications);

    // Wallet ??

    // Earliest blockHeight from accountTimestamp.
    uint64_t earliestBlockNumber = 0;

    // Initialize the `brdSync` struct
    gwm->brdSync.rid = -1;
    gwm->brdSync.begBlockNumber = earliestBlockNumber;
    gwm->brdSync.endBlockNumber = MAX (earliestBlockNumber, blockHeight);
    gwm->brdSync.completed = false;
    gwm->brdSync.success = false;

    gwm->syncContext  = syncContext;
    gwm->syncCallback = syncCallback;
    
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
    genWalletRelease (gwm->wallet);

    fileServiceRelease (gwm->fileService);
    eventHandlerDestroy (gwm->handler);
    free (gwm->storagePath);
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

extern BRGenericWallet
genManagerGetPrimaryWallet (BRGenericManager gwm) {
   return gwm->wallet;
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

extern int
genManagerIsConnected (BRGenericManager gwm) {
    return eventHandlerIsRunning (gwm->handler);
}

extern void
genManagerSync (BRGenericManager gwm) {
    genManagerSyncToDepth (gwm, CRYPTO_SYNC_DEPTH_FROM_CREATION);
}

extern void
genManagerSyncToDepth (BRGenericManager gwm,
                       BRCryptoSyncDepth depth) {
    pthread_mutex_lock (&gwm->lock);

    // Abort an ongoing sync by incrementing the `rid` - any callback for an in-progress
    // client callback will be ignored.
    gwm->brdSync.rid += 1;

    // Avoid the periodic dispatcher from mucking with the 'begBlockNumber' - which it might
    // do if success is `true`.
    gwm->brdSync.completed = false;
    gwm->brdSync.success = false;

    // For a GEN sync, just start at 0 always.
    switch (depth) {
        case CRYPTO_SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND:
        case CRYPTO_SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK:
        case CRYPTO_SYNC_DEPTH_FROM_CREATION:
            gwm->brdSync.begBlockNumber = 0;
            break;
    }
    pthread_mutex_unlock (&gwm->lock);
}

extern BRGenericAddress
genManagerGetAccountAddress (BRGenericManager gwm) {
    return genAccountGetAddress (gwm->account);
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
    free (tx);
}

extern BRGenericTransfer
genManagerRecoverTransfer (BRGenericManager gwm,
                           BRGenericWallet wallet,
                           const char *hash,
                           const char *uids,
                           const char *from,
                           const char *to,
                           const char *amount,
                           const char *currency,
                           const char *fee,
                           uint64_t timestamp,
                           uint64_t blockHeight,
                           int error) {
    BRGenericTransfer transfer = genTransferAllocAndInit (gwm->handlers->type,
                                                          gwm->handlers->manager.transferRecover (hash, from, to, amount, currency, fee, timestamp, blockHeight, error));

    transfer->uids = strdup (uids);

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
                                                         feeBasis,
                                                         0 == error,
                                                         NULL));

    genAddressRelease (source);
    genAddressRelease (target);
    return transfer;
}

extern BRArrayOf(BRGenericTransfer)
genManagerRecoverTransfersFromRawTransaction (BRGenericManager gwm,
                                              uint8_t *bytes,
                                              size_t   bytesCount,
                                              uint64_t timestamp,
                                              uint64_t blockHeight,
                                              int error) {
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
                                                             genTransferGetFeeBasis (transfer),
                                                             0 == error,
                                                             NULL));
        array_add (transfers, transfer);
    }
    pthread_mutex_unlock (&gwm->lock);
    return transfers;
}

extern BRArrayOf(BRGenericTransfer)
genManagerLoadTransfers (BRGenericManager gwm) {
    BRArrayOf (BRGenericTransfer) transfers;
    BRSetOf   (BRGenericTransfer) transferSet = genTransferSetCreate (25);

    fileServiceLoad (gwm->fileService, transferSet, fileServiceTypeTransactions, 1);

    pthread_mutex_lock (&gwm->lock);
    array_new (transfers, BRSetCount(transferSet));
    FOR_SET (BRGenericTransfer, transfer, transferSet)
        array_add (transfers, transfer);
    pthread_mutex_unlock (&gwm->lock);

    BRSetFree(transferSet);
    return transfers;
}

extern void
genManagerSaveTransfer (BRGenericManager gwm,
                        BRGenericTransfer transfer) {
    fileServiceSave (gwm->fileService, fileServiceTypeTransactions, transfer);
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

    // 1) check if the prior sync has completed successfully
    if (gwm->brdSync.completed && gwm->brdSync.success) {
        // 1a) if so, advance the sync range by updating `begBlockNumber`
        gwm->brdSync.begBlockNumber = (gwm->brdSync.endBlockNumber >=  GWM_BRD_SYNC_START_BLOCK_OFFSET
                                       ? gwm->brdSync.endBlockNumber - GWM_BRD_SYNC_START_BLOCK_OFFSET
                                       : 0);
    }

    // 2) completed or not, update the `endBlockNumber` to the current block height.
    gwm->brdSync.endBlockNumber = MAX (gwm->blockHeight, gwm->brdSync.begBlockNumber);

    // 3) we'll update transactions if there are more blocks to examine
    if (gwm->brdSync.begBlockNumber != gwm->brdSync.endBlockNumber) {
        BRGenericAddress accountAddress = genManagerGetAccountAddress(gwm);
        char *address = genAddressAsString (accountAddress);
        
        // 3a) Save the current requestId and mark as not completed.
        gwm->brdSync.rid = gwm->requestId;
        gwm->brdSync.completed = false;
        gwm->brdSync.success = false;

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
                                      gwm->brdSync.rid);
        } else {
            gwm->client.getTransactions (gwm->client.context,
                                         gwm,
                                         address,
                                         gwm->brdSync.begBlockNumber,
                                         gwm->brdSync.endBlockNumber,
                                         gwm->brdSync.rid);
        }

        // 3c) On to the next rid
        gwm->requestId += 1;

        free (address);
        genAddressRelease(accountAddress);
    }

    if (NULL != gwm->syncCallback)
        gwm->syncCallback (gwm->syncContext,
                           gwm,
                           gwm->brdSync.begBlockNumber,
                           gwm->brdSync.endBlockNumber,
                           // lots of incremental sync 'slop'
                           2 * GWM_BRD_SYNC_START_BLOCK_OFFSET);

    // End handling a BRD Sync
}

extern void
genManagerWipe (BRGenericNetwork network,
                const char *storagePath) {
        fileServiceWipe (storagePath,
                         genNetworkGetType(network),
                         genNetworkIsMainnet (network) ? "mainnet" : "testnet");
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
    if (rid == manager->brdSync.rid) {
        manager->brdSync.completed = true;
        manager->brdSync.success = (bool) success;
    }
    pthread_mutex_unlock (&manager->lock);
}

extern void
genManagerAnnounceSubmit (BRGenericManager manager,
                          int rid,
                          BRGenericTransfer transfer,
                          int error) {
    // Event
}

/// MARK: - Transfer State, Fee Basis & Attribute Encode/Decode

static BRRlpItem
genFeeBasisEncode (BRGenericFeeBasis feeBasis,
                   BRRlpCoder coder) {
    return rlpEncodeList2 (coder,
                           rlpEncodeUInt256 (coder, feeBasis.pricePerCostFactor, 0),
                           rlpEncodeDouble  (coder, feeBasis.costFactor));
}

static BRGenericFeeBasis
genFeeBasisDecode (BRRlpItem item,
                   BRRlpCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);
    assert (itemsCount == 2);

    return (BRGenericFeeBasis) {
        rlpDecodeUInt256 (coder, items[0], 0),
        rlpDecodeDouble  (coder, items[1])
    };
}

extern BRRlpItem
genTransferStateEncode (BRGenericTransferState state,
                        BRGenericTransferStateEncodeVersion version,
                        BRRlpCoder coder) {
    switch (state.type) {
        case GENERIC_TRANSFER_STATE_INCLUDED:
            switch (version) {
                case GEN_TRANSFER_STATE_ENCODE_V1:
                    return rlpEncodeList (coder, 5,
                                          rlpEncodeUInt64  (coder, state.type, 0),
                                          rlpEncodeUInt64  (coder, state.u.included.blockNumber, 0),
                                          rlpEncodeUInt64  (coder, state.u.included.transactionIndex, 0),
                                          rlpEncodeUInt64  (coder, state.u.included.timestamp, 0),
                                          genFeeBasisEncode (state.u.included.feeBasis, coder));
                case GEN_TRANSFER_STATE_ENCODE_V2:
                    return rlpEncodeList (coder, 7,
                                          rlpEncodeUInt64  (coder, state.type, 0),
                                          rlpEncodeUInt64  (coder, state.u.included.blockNumber, 0),
                                          rlpEncodeUInt64  (coder, state.u.included.transactionIndex, 0),
                                          rlpEncodeUInt64  (coder, state.u.included.timestamp, 0),
                                          genFeeBasisEncode (state.u.included.feeBasis, coder),
                                          rlpEncodeUInt64  (coder, state.u.included.success, 0),
                                          rlpEncodeString  (coder, state.u.included.error));
            }

        case GENERIC_TRANSFER_STATE_ERRORED:
            return rlpEncodeList2 (coder,
                                   rlpEncodeUInt64 (coder, state.type, 0),
                                   rlpEncodeUInt64 (coder, state.u.errored, 0));

        case GENERIC_TRANSFER_STATE_CREATED:
        case GENERIC_TRANSFER_STATE_SIGNED:
        case GENERIC_TRANSFER_STATE_SUBMITTED:
        case GENERIC_TRANSFER_STATE_DELETED:
            return rlpEncodeList1 (coder,
                                   rlpEncodeUInt64 (coder, state.type, 0));

    }
}

extern BRGenericTransferState
genTransferStateDecode (BRRlpItem item,
                        BRRlpCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);
    assert (itemsCount >= 1);

    BRGenericTransferStateType type = (BRGenericTransferStateType) rlpDecodeUInt64 (coder, items[0], 0);
    switch (type) {
        case GENERIC_TRANSFER_STATE_INCLUDED: {
            assert (5 == itemsCount || 7 == itemsCount);

            BRGenericTransferState state = (BRGenericTransferState) {
                type,
                { .included = {
                    rlpDecodeUInt64  (coder, items[1], 0),
                    rlpDecodeUInt64  (coder, items[2], 0),
                    rlpDecodeUInt64  (coder, items[3], 0),
                    genFeeBasisDecode (items[4], coder),
                    (5 == itemsCount ? CRYPTO_TRUE : ((BRCryptoBoolean) rlpDecodeUInt64(coder, items[5], 0)))
                }}
            };

            memset (state.u.included.error, 0, sizeof(state.u.included.error));
            if (7 == itemsCount) {
                char *error = rlpDecodeString(coder, items[6]);
                strlcpy (state.u.included.error, error, sizeof(state.u.included.error));
                free (error);
            }

            return state;
        }
            
        case GENERIC_TRANSFER_STATE_ERRORED: {
            assert (2 == itemsCount);
            return (BRGenericTransferState) {
                type,
                { .errored = (BRGenericTransferSubmitError) rlpDecodeUInt64 (coder, items[1], 0) }
            };
        }

        case GENERIC_TRANSFER_STATE_CREATED:
        case GENERIC_TRANSFER_STATE_SIGNED:
        case GENERIC_TRANSFER_STATE_SUBMITTED:
        case GENERIC_TRANSFER_STATE_DELETED:
            return (BRGenericTransferState) { type };
    }
}

static OwnershipGiven BRArrayOf(BRGenericTransferAttribute)
genTransferAttributesDecode (BRRlpItem item,
                             BRRlpCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);

    BRArrayOf(BRGenericTransferAttribute) attributes;
    array_new(attributes, itemsCount);
    for (size_t index = 0; index < itemsCount; index++) {
        size_t fieldCount = 0;
        const BRRlpItem *fields = rlpDecodeList (coder, items[index], &fieldCount);
        assert (3 == fieldCount);

        char *key = rlpDecodeString (coder, fields[0]);
        char *val = rlpDecodeString (coder, fields[1]);
        int  isRequired = (int) rlpDecodeUInt64 (coder, fields[2], 1);

        array_add (attributes, genTransferAttributeCreate (key, (0 == strlen(val) ? NULL : val), isRequired));

        free (key);
        free (val);
    }

    return attributes;
}

static BRRlpItem
genTransferAttributesEncode (OwnershipKept BRArrayOf(BRGenericTransferAttribute) attributes,
                             BRRlpCoder coder) {
    size_t itemsCount = array_count(attributes);
    BRRlpItem items[itemsCount];
    for (size_t index = 0; index < itemsCount; index++) {
        BRGenericTransferAttribute attribute = attributes[index];

        const char *key = genTransferAttributeGetKey (attribute);
        const char *val = genTransferAttributeGetVal (attribute);
        int isRequired = genTransferAttributeIsRequired (attribute);

        items[index] = rlpEncodeList (coder, 3,
                                      rlpEncodeString (coder, key),
                                      rlpEncodeString (coder, (NULL == val ? "" : val)),
                                      rlpEncodeUInt64 (coder, isRequired, 1));
    }
    return rlpEncodeListItems(coder, items, itemsCount);
}

/// MARK: - Events

const BREventType *gwmEventTypes[] = {
    // ...
};

const unsigned int
gwmEventTypesCount = (sizeof (gwmEventTypes) / sizeof (BREventType*));
