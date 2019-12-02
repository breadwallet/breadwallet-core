//
//  BREthereumEWMPrivate.h
//  BRCore
//
//  Created by Ed Gamble on 5/7/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_EWM_Private_H
#define BR_Ethereum_EWM_Private_H

#include <stdint.h>
#include <pthread.h>
#include "support/BRFileService.h"
#include "ethereum/blockchain/BREthereumBlockChain.h"
#include "ethereum/les/BREthereumLES.h"
#include "ethereum/bcs/BREthereumBCS.h"
#include "ethereum/event/BREvent.h"
#include "BREthereumTransfer.h"
#include "BREthereumWallet.h"
#include "BREthereumEWM.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CLIENT_CHANGE_ADD,
    CLIENT_CHANGE_REM,
    CLIENT_CHANGE_UPD
} BREthereumClientChangeType;

#define CLIENT_CHANGE_TYPE_NAME( ev ) \
(CLIENT_CHANGE_ADD == (ev) ? "Add" \
: (CLIENT_CHANGE_REM == (ev) ? "Rem" : "Upd"))

#define EWM_REQUEST_ID_UNKNOWN    (UINT_MAX)

//
// EWM
//

#define DEFAULT_LISTENER_CAPACITY 3
#define DEFAULT_WALLET_CAPACITY 10
#define DEFAULT_BLOCK_CAPACITY 100
#define DEFAULT_TRANSACTION_CAPACITY 1000

/// MISPLACED
extern void
ewmInsertWallet (BREthereumEWM ewm,
                 BREthereumWallet wallet);

extern void
ewmUpdateBlockHeight(BREthereumEWM ewm,
                     uint64_t blockHeight);

//
// EWM
//
struct BREthereumEWMRecord {
    /**
     * The State
     */
    BREthereumEWMState state;

    /**
     * The Mode of this EWM
     */
    BRCryptoSyncMode mode;

    /**
     * The network
     */
    BREthereumNetwork network;

    /**
     * The Client supporting this EWM
     */
    BREthereumClient client;

    /**
     * The account
     */
    BREthereumAccount account;
    BREthereumTimestamp accountTimestamp;

    /**
     * The wallets 'managed/handled' by this ewm.  There can be only one wallet holding ETHER;
     * all the other wallets hold TOKENs and only one wallet per TOKEN.
     */
    BREthereumWallet *wallets;
    BREthereumWallet  walletHoldingEther;

    /**
     * ERC20 Tokens
     */
    BRSetOf(BREthereumToken) tokens;

    /**
     * The BCS Interface
     */
    BREthereumBCS bcs;

    /**
     * The BlockHeight is the largest block number seen or computed.  [Note: the blockHeight may
     * be computed from a Log event as (log block number + log confirmations).  This is the block
     * number for the block at the head of the blockchain.
     *
     * This gets initialized from ewmCreate() based on the 'known' block height of the network or
     * with zero if the network's block height is unknown.
     */
    uint64_t blockHeight;

    /**
     * The number of blocks required to be mined before until a transfer can be considered final
     */
    uint64_t confirmationsUntilFinal;

    /**
     * An identiifer for a LES/BRD Request
     */
    unsigned int requestId;

    /**
     * An EventHandler for Main.  All 'announcements' (via LES (or BRD) hit here.
     */
    BREventHandler handler;

    /**
     * The Lock ensuring single thread access to EWM state.
     */
    pthread_mutex_t lock;

    /**
     * The RLP Coder
     */
    BRRlpCoder coder;

    /**
     * The File Service
     */
    BRFileService fs;

    /**
     * If we are syncing with BRD, instead of as P2P with BCS, then we'll keep a record to
     * ensure we've successfully completed the getTransactions() and getLogs() callbacks to
     * the client.
     */
    struct {
        uint64_t begBlockNumber;
        uint64_t endBlockNumber;

        unsigned int ridTransaction;
        unsigned int ridLog;

        unsigned int completedTransaction:1;
        unsigned int completedLog:1;
    } brdSync;
};

/// MARK: - BCS Callback Interfaces

//
// Signal/Handle Block Chain (BCS Callback)
//
extern void
ewmHandleBlockChain (BREthereumEWM ewm,
                     BREthereumHash headBlockHash,
                     uint64_t headBlockNumber,
                     uint64_t headBlockTimestamp);

extern void
ewmSignalBlockChain (BREthereumEWM ewm,
                     BREthereumHash headBlockHash,
                     uint64_t headBlockNumber,
                     uint64_t headBlockTimestamp);

//
// Signal/Handle Account State (BCS Callback)
//
extern void
ewmHandleAccountState (BREthereumEWM ewm,
                       BREthereumAccountState accountState);

extern void
ewmSignalAccountState (BREthereumEWM ewm,
                       BREthereumAccountState accountState);

//
// Signal/Handle Balance (BCS Callback)
//
extern void
ewmHandleBalance (BREthereumEWM ewm,
                  BREthereumAmount balance);

extern void
ewmSignalBalance (BREthereumEWM ewm,
                  BREthereumAmount balance);

//
// Signal/Handle GasPrice (BCS Callback)
//
extern void
ewmHandleGasPrice (BREthereumEWM ewm,
                   BREthereumWallet wallet,
                   BREthereumGasPrice gasPrice);
extern void
ewmSignalGasPrice (BREthereumEWM ewm,
                   BREthereumWallet wallet,
                   BREthereumGasPrice gasPrice);

//
// Signal/Handle GasEstimate (BCS Callback)
//
extern void
ewmHandlGasEstimateSuccess (BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumCookie cookie,
                            BREthereumGas gasEstimate,
                            BREthereumGasPrice gasPrice);

extern void
ewmHandlGasEstimateFailure (BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumCookie cookie,
                            BREthereumStatus status);

extern void
ewmSignalGasEstimateSuccess(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumCookie cookie,
                            BREthereumGas gasEstimate,
                            BREthereumGasPrice gasPrice);

extern void
ewmSignalGasEstimateFailure(BREthereumEWM ewm,
                            BREthereumWallet wallet,
                            BREthereumCookie cookie,
                            BREthereumStatus status);

//
// Signal/Handle Transaction (BCS Callback)
//
// This *MUST BE* the single point-of-entry for injecting a transaction into EWM.  It will be
// called by BCS as the P2P modes find blocks w/ transactions and it will be called by EWM as the
// BRD modes query transactions from the BRD endpoing.
//
extern void
ewmHandleTransaction (BREthereumEWM ewm,
                      BREthereumBCSCallbackTransactionType type,
                      OwnershipGiven BREthereumTransaction transaction);

extern void
ewmSignalTransaction (BREthereumEWM ewm,
                      BREthereumBCSCallbackTransactionType type,
                      OwnershipGiven BREthereumTransaction transaction);

//
// Signal/Handle Log (BCS Callback)
//
// See the comments for ewm{Handle,Signal}Transaction() above
//
extern void
ewmHandleLog (BREthereumEWM ewm,
              BREthereumBCSCallbackLogType type,
              OwnershipGiven BREthereumLog log);

extern void
ewmSignalLog (BREthereumEWM ewm,
              BREthereumBCSCallbackLogType type,
              OwnershipGiven BREthereumLog log);

//
// Signal/Handle Save Blocks (BCS Callback)
//
extern void
ewmHandleSaveBlocks (BREthereumEWM ewm,
                     OwnershipGiven BRArrayOf(BREthereumBlock) blocks);

extern void
ewmSignalSaveBlocks (BREthereumEWM ewm,
                     OwnershipGiven BRArrayOf(BREthereumBlock) blocks);

//
// Signal/Handle Save Peers (BCS Callback)
//
extern void
ewmHandleSaveNodes (BREthereumEWM ewm,
                    OwnershipGiven BRArrayOf(BREthereumNodeConfig) peers);

extern void
ewmSignalSaveNodes (BREthereumEWM ewm,
                    OwnershipGiven BRArrayOf(BREthereumNodeConfig) peers);

//
// Handle Save Transaction
//
extern void
ewmHandleSaveTransaction (BREthereumEWM ewm,
                          BREthereumTransaction transaction,
                          BREthereumClientChangeType type);

//
// Handle Save Log
//
extern void
ewmHandleSaveLog (BREthereumEWM ewm,
                  BREthereumLog log,
                  BREthereumClientChangeType type);

extern void
ewmHandleSaveWallet (BREthereumEWM ewm,
                     BREthereumWallet wallet,
                     BREthereumClientChangeType type);

//
// Signal/Handle Sync (BCS Callback)
//
extern void
ewmHandleSync (BREthereumEWM ewm,
               BREthereumBCSCallbackSyncType type,
               uint64_t blockNumberStart,
               uint64_t blockNumberCurrent,
               uint64_t blockNumberStop);

extern void
ewmSignalSync (BREthereumEWM ewm,
               BREthereumBCSCallbackSyncType type,
               uint64_t blockNumberStart,
               uint64_t blockNumberCurrent,
               uint64_t blockNumberStop);

//
// Signal/Handle Sync (BCS Callback)
//
extern void
ewmHandleSyncAPI (BREthereumEWM ewm);

extern void
ewmSignalSyncAPI (BREthereumEWM ewm,
                  BREthereumBoolean OOB);

//
// Signal/Handle Get Blocks (BCS Callback)
//
extern void
ewmHandleGetBlocks (BREthereumEWM ewm,
                    BREthereumAddress address,
                    BREthereumSyncInterestSet interests,
                    uint64_t blockStart,
                    uint64_t blockStop);
extern void
ewmSignalGetBlocks (BREthereumEWM ewm,
                    BREthereumAddress address,
                    BREthereumSyncInterestSet interests,
                    uint64_t blockStart,
                    uint64_t blockStop);

/// MARK: - (Wallet) Balance

extern void
ewmHandleAnnounceBalance (BREthereumEWM ewm,
                          BREthereumWallet wallet,
                          UInt256 amount,
                          int rid);

extern void
ewmSignalAnnounceBalance (BREthereumEWM ewm,
                          BREthereumWallet wallet,
                          UInt256 amount,
                          int rid);

extern void
ewmHandleUpdateWalletBalances (BREthereumEWM ewm);

extern void
ewmSignalUpdateWalletBalances (BREthereumEWM ewm);

/// MARK: - GasPrice

extern void
ewmSignalAnnounceGasPrice (BREthereumEWM ewm,
                           BREthereumWallet wallet,
                           UInt256 value,
                           int rid);

extern void
ewmHandleAnnounceGasPrice (BREthereumEWM ewm,
                           BREthereumWallet wallet,
                           UInt256 value,
                           int rid);

/// MARK: - Submit Transaction

extern void
ewmSignalAnnounceSubmitTransfer (BREthereumEWM ewm,
                                 BREthereumWallet wallet,
                                 BREthereumTransfer transfer,
                                 int errorCode,
                                 const char *errorMessage,
                                 int rid);

extern void
ewmHandleAnnounceSubmitTransfer (BREthereumEWM ewm,
                                 BREthereumWallet wallet,
                                 BREthereumTransfer transfer,
                                 int errorCode,
                                 const char *errorMessage,
                                 int rid);

/// MARK: - Transactions

typedef struct {
    BREthereumHash hash;
    BREthereumAddress from;
    BREthereumAddress to;
    BREthereumAddress contract;
    UInt256 amount;
    uint64_t gasLimit;
    UInt256 gasPrice;
    char *data;
    uint64_t nonce;
    uint64_t gasUsed;
    uint64_t blockNumber;
    BREthereumHash blockHash;
    uint64_t blockConfirmations;
    uint64_t blockTransactionIndex;
    uint64_t blockTimestamp;
    BREthereumBoolean isError;
} BREthereumEWMClientAnnounceTransactionBundle;

static inline void
ewmClientAnnounceTransactionBundleRelease (BREthereumEWMClientAnnounceTransactionBundle *bundle) {
    free (bundle->data);
    free (bundle);
}

extern void
ewmHandleAnnounceTransaction(BREthereumEWM ewm,
                                   BREthereumEWMClientAnnounceTransactionBundle *bundle,
                                   int id);

extern void
ewmSignalAnnounceTransaction(BREthereumEWM ewm,
                                   BREthereumEWMClientAnnounceTransactionBundle *bundle,
                                   int id);

/// MARK: - Logs

typedef struct {
    BREthereumHash hash;
    BREthereumAddress contract;
    int topicCount;
    char **arrayTopics;
    char *data;
    UInt256 gasPrice;
    uint64_t gasUsed;
    uint64_t logIndex;
    uint64_t blockNumber;
    uint64_t blockTransactionIndex;
    uint64_t blockTimestamp;
} BREthereumEWMClientAnnounceLogBundle;

static inline void
ewmClientAnnounceLogBundleRelease (BREthereumEWMClientAnnounceLogBundle *bundle) {
    for (int i = 0; i < bundle->topicCount; i++)
        free (bundle->arrayTopics[i]);
    free (bundle->arrayTopics);
    free (bundle->data);
    free (bundle);
}

extern void
ewmSignalAnnounceLog (BREthereumEWM ewm,
                            BREthereumEWMClientAnnounceLogBundle *bundle,
                            int id);

extern void
ewmHandleAnnounceLog (BREthereumEWM ewm,
                            BREthereumEWMClientAnnounceLogBundle *bundle,
                            int id);

/// MARK: - Account Complete

extern void
ewmSignalAnnounceComplete (BREthereumEWM ewm,
                           BREthereumBoolean isTransaction,
                           BREthereumBoolean success,
                           int rid);

extern void
ewmHandleAnnounceComplete (BREthereumEWM ewm,
                           BREthereumBoolean isTransaction,
                           BREthereumBoolean success,
                           int rid);
/// MARK: - Tokens

typedef struct {
    char *address;
    char *symbol;
    char *name;
    char *description;
    unsigned int decimals;
    BREthereumGas gasLimit;
    BREthereumGasPrice gasPrice;
} BREthereumEWMClientAnnounceTokenBundle;

static inline void
ewmClientAnnounceTokenBundleRelease (BREthereumEWMClientAnnounceTokenBundle *bundle) {
    free (bundle->address);
    free (bundle->symbol);
    free (bundle->name);
    free (bundle->description);
    free (bundle);
}

extern void
ewmHandleAnnounceToken (BREthereumEWM ewm,
                              BREthereumEWMClientAnnounceTokenBundle *bundle,
                              int id);
extern void
ewmSignalAnnounceToken (BREthereumEWM ewm,
                              BREthereumEWMClientAnnounceTokenBundle *bundle,
                              int id);

extern void
ewmHandleAnnounceTokenComplete (BREthereumEWM ewm,
                                int rid,
                                BREthereumBoolean success);
extern void
ewmSignalAnnounceTokenComplete (BREthereumEWM ewm,
                                int rid,
                                BREthereumBoolean success);

/// MARK: - BlockNumber

extern void
ewmHandleAnnounceBlockNumber (BREthereumEWM ewm,
                                    uint64_t blockNumber,
                                    int rid);

extern void
ewmSignalAnnounceBlockNumber (BREthereumEWM ewm,
                                    uint64_t blockNumber,
                                    int rid);

/// MARK: - Nonce

extern void
ewmHandleAnnounceNonce (BREthereumEWM ewm,
                              BREthereumAddress address,
                              uint64_t nonce,
                              int rid);

extern void
ewmSignalAnnounceNonce (BREthereumEWM ewm,
                              BREthereumAddress address,
                              uint64_t nonce,
                              int rid);

///
// Save Sync (and other) State
//
//
// Wallet Event
//

extern void
ewmHandleWalletEvent(BREthereumEWM ewm,
                           BREthereumWallet wid,
                           BREthereumWalletEvent event);

extern void
ewmSignalWalletEvent(BREthereumEWM ewm,
                           BREthereumWallet wid,
                           BREthereumWalletEvent event);

//
// Block Event
//
#if defined (NEVER_DEFINED)
extern void
ewmSignalBlockEvent(BREthereumEWM ewm,
                          BREthereumBlock bid,
                          BREthereumBlockEvent event);

extern void
ewmHandleBlockEvent(BREthereumEWM ewm,
                          BREthereumBlock bid,
                          BREthereumBlockEvent event);
#endif
//
// Transfer Event
//

extern void
ewmSignalTransferEvent(BREthereumEWM ewm,
                       BREthereumWallet wid,
                       BREthereumTransfer tid,
                       BREthereumTransferEvent event);

extern void
ewmHandleTransferEvent(BREthereumEWM ewm,
                       BREthereumWallet wid,
                       BREthereumTransfer tid,
                       BREthereumTransferEvent event);
//
// Peer Event
//
extern void
ewmSignalPeerEvent(BREthereumEWM ewm,
                   BREthereumPeerEvent event);

extern void
ewmHandlePeerEvent(BREthereumEWM ewm,
                   BREthereumPeerEvent event);

//
// EWM Event
//
extern void
ewmSignalEWMEvent(BREthereumEWM ewm,
                  BREthereumEWMEvent event);

extern void
ewmHandleEWMEvent(BREthereumEWM ewm,
                  BREthereumEWMEvent event);

/// MARK: - Handler For Main

extern const BREventType *ewmEventTypes[];
extern const unsigned int ewmEventTypesCount;

/// MARK: - File Service

extern const char *ewmFileServiceTypeTransactions;
extern const char *ewmFileServiceTypeLogs;
extern const char *ewmFileServiceTypeBlocks;
extern const char *ewmFileServiceTypeNodes;
extern const char *ewmFileServiceTypeTokens;
extern const char *ewmFileServiceTypeWallets;

extern size_t ewmFileServiceSpecificationsCount;
extern BRFileServiceTypeSpecification *ewmFileServiceSpecifications;

#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_EWM_Private_H
