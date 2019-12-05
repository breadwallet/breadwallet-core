//
//  BRWalletManager.h
//  BRCore
//
//  Created by Ed Gamble on 11/21/18.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#ifndef BRWalletManager_h
#define BRWalletManager_h

#include <stdio.h>
#include "BRFileService.h"
#include "BRBase.h"                 // Ownership
#include "BRBIP32Sequence.h"        // BRMasterPubKey
#include "BRChainParams.h"          // BRChainParams (*NOT THE STATIC DECLARATIONS*)
#include "BRTransaction.h"
#include "BRWallet.h"

#include "BRCryptoTransfer.h"
#include "BRCryptoWalletManager.h"

#ifdef __cplusplus
extern "C" {
#endif

/// MARK: - Forward Declarations

typedef struct BRWalletSweeperStruct *BRWalletSweeper;

typedef struct BRWalletManagerStruct *BRWalletManager;

// Cookies are used as markers to match up an asynchronous operation
// request with its corresponding event.
typedef void *BRCookie;

typedef void *BRWalletManagerClientContext;

/// MARK: - Callbacks

typedef void
(*BRGetBlockNumberCallback) (BRWalletManagerClientContext context,
                             BRWalletManager manager,
                             int rid);

extern int
bwmAnnounceBlockNumber (BRWalletManager manager,
                        int rid,
                        uint64_t blockNumber);

typedef void
(*BRGetTransactionsCallback) (BRWalletManagerClientContext context,
                              BRWalletManager manager,
                              OwnershipKept const char **addresses,
                              size_t addressCount,
                              uint64_t begBlockNumber,
                              uint64_t endBlockNumber,
                              int rid);

extern int // success - data is valid
bwmAnnounceTransaction (BRWalletManager manager,
                        int id,
                        OwnershipKept uint8_t *transaction,
                        size_t transactionLength,
                        uint64_t timestamp,
                        uint64_t blockHeight);

extern void
bwmAnnounceTransactionComplete (BRWalletManager manager,
                                int id,
                                int success);

typedef void
(*BRSubmitTransactionCallback) (BRWalletManagerClientContext context,
                                BRWalletManager manager,
                                BRWallet *wallet,
                                OwnershipKept uint8_t *transaction,
                                size_t transactionLength,
                                UInt256 transactionHash,
                                int rid);

extern void
bwmAnnounceSubmit (BRWalletManager manager,
                   int rid,
                   UInt256 txHash,
                   int error);

///
/// Transaction Event
///
typedef enum {
    // NOTE: The transaction is NOT owned by the event handler.
    /**
     * For P2P and API, this event occurs once a transaction has been created for a
     * wallet, but is not yet signed or added.
     */
    BITCOIN_TRANSACTION_CREATED,

    /**
     * For P2P and API, this event occurs once a transaction has been signed for a
     * wallet, but is not yet added.
     */
    BITCOIN_TRANSACTION_SIGNED,

    /**
     * For P2P and API, this event occurs once a transaction has been added to a
     * wallet (via BRWalletRegisterTransaction).
     */
    BITCOIN_TRANSACTION_ADDED,

    /**
     * For P2P and API, this event occurs once a transaction has been added to a vallet
     * (via BRWalletRegisterTransaction) or once it has been marked as CONFIRMED or
     * UNCONFIRMED (via BRWalletUpdateTransactions).
     */
    BITCOIN_TRANSACTION_UPDATED,

    /**
     * For P2P, this event occurs once a transaction has been deleted from a wallet
     * (via BRWalletRemoveTransaction) as a result of an UNCONFIRMED transaction no longer
     * being visible in the mempools of any of connected P2P peers.
     *
     * For API, this event does not occur at present.
     */
    BITCOIN_TRANSACTION_DELETED,
} BRTransactionEventType;

typedef struct {
    BRTransactionEventType type;
    union {
        struct {
            uint64_t blockHeight;
            uint32_t timestamp;
        } updated;
    } u;
} BRTransactionEvent;

typedef void
(*BRTransactionEventCallback) (BRWalletManagerClientContext context,
                               OwnershipKept BRWalletManager manager,
                               OwnershipKept BRWallet *wallet,
                               OwnershipKept BRTransaction *transaction,
                               BRTransactionEvent event);

extern const char *
BRTransactionEventTypeString (BRTransactionEventType t);

///
/// Wallet Event
///

typedef enum {
    BITCOIN_WALLET_CREATED,
    BITCOIN_WALLET_BALANCE_UPDATED,
    BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED,
    BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED,
    BITCOIN_WALLET_FEE_PER_KB_UPDATED,
    BITCOIN_WALLET_FEE_ESTIMATED,
    BITCOIN_WALLET_DELETED,
} BRWalletEventType;

typedef struct {
    BRWalletEventType type;
    union {
        struct {
            uint64_t satoshi;
        } balance;

        struct {
            BRTransaction *transaction;
        } submitSucceeded;

        struct {
            BRTransaction *transaction;
            BRCryptoTransferSubmitError error;
        } submitFailed;

        struct {
            uint64_t value;
        } feePerKb;

        struct {
            BRCookie cookie;
            uint64_t feePerKb;
            uint32_t sizeInByte;
        } feeEstimated;
    } u;
} BRWalletEvent;

typedef void
(*BRWalletEventCallback) (BRWalletManagerClientContext context,
                          OwnershipKept BRWalletManager manager,
                          OwnershipKept BRWallet *wallet,
                          BRWalletEvent event);

extern const char *
BRWalletEventTypeString (BRWalletEventType t);

extern int
BRWalletEventTypeIsValidPair (BRWalletEventType t1, BRWalletEventType t2);

///
/// WalletManager Event
///
typedef enum {
    BITCOIN_WALLET_MANAGER_CREATED,
    BITCOIN_WALLET_MANAGER_CONNECTED,
    BITCOIN_WALLET_MANAGER_DISCONNECTED,
    BITCOIN_WALLET_MANAGER_SYNC_STARTED,
    BITCOIN_WALLET_MANAGER_SYNC_PROGRESS,
    BITCOIN_WALLET_MANAGER_SYNC_STOPPED,
    BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED,
    BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED
} BRWalletManagerEventType;

typedef struct {
    BRWalletManagerEventType type;
    union {
        struct {
            BRCryptoSyncTimestamp timestamp;
            BRCryptoSyncPercentComplete percentComplete;
        } syncProgress;
        struct {
            BRCryptoSyncStoppedReason reason;
        } syncStopped;
        struct {
            BRCryptoSyncDepth depth;
        } syncRecommended;
        struct {
            BRCryptoWalletManagerDisconnectReason reason;
        } disconnected;
        struct {
            uint64_t value;
        } blockHeightUpdated;
    } u;
} BRWalletManagerEvent;

typedef void
(*BRWalletManagerEventCallback) (BRWalletManagerClientContext context,
                                 OwnershipKept BRWalletManager manager,
                                 BRWalletManagerEvent event);

extern const char *
BRWalletManagerEventTypeString (BRWalletManagerEventType t);

extern int
BRWalletManagerEventTypeIsValidPair (BRWalletManagerEventType t1, BRWalletManagerEventType t2);

///
/// WalletManager
///
typedef struct {
    BRWalletManagerClientContext context;

    BRGetBlockNumberCallback  funcGetBlockNumber;
    BRGetTransactionsCallback funcGetTransactions;
    BRSubmitTransactionCallback funcSubmitTransaction;

    BRTransactionEventCallback funcTransactionEvent;
    BRWalletEventCallback  funcWalletEvent;
    BRWalletManagerEventCallback funcWalletManagerEvent;
} BRWalletManagerClient;

extern BRWalletManager
BRWalletManagerNew (BRWalletManagerClient client,
                    BRMasterPubKey mpk,
                    const BRChainParams *params,
                    uint32_t earliestKeyTime,
                    BRCryptoSyncMode mode,
                    const char *storagePath,
                    uint64_t blockHeight,
                    uint64_t confirmationsUntilFinal);

extern void
BRWalletManagerFree (BRWalletManager manager);

extern void
BRWalletManagerStart (BRWalletManager manager);

extern void
BRWalletManagerStop (BRWalletManager manager);

extern void
BRWalletManagerConnect (BRWalletManager manager);

extern void
BRWalletManagerDisconnect (BRWalletManager manager);

extern void
BRWalletManagerSetFixedPeer (BRWalletManager manager,
                             UInt128 address,
                             uint16_t port);

extern void
BRWalletManagerScan (BRWalletManager manager);

extern void
BRWalletManagerScanToDepth (BRWalletManager manager, BRCryptoSyncDepth depth);

extern void
BRWalletManagerSetMode (BRWalletManager manager, BRCryptoSyncMode mode);

extern BRCryptoSyncMode
BRWalletManagerGetMode (BRWalletManager manager);

extern void
BRWalletManagerSetNetworkReachable (BRWalletManager manager,
                                    int isNetworkReachable);

//
// These should not be needed if the events are sufficient
//
extern BRWallet *
BRWalletManagerGetWallet (BRWalletManager manager);

/**
 * Return `1` if `manager` handles BTC; otherwise `0` if BCH.  Note: the `BRChainParams` determine
 * BTC vs BCH.
 */
extern int
BRWalletManagerHandlesBTC (BRWalletManager manager);

/**
 * Creates an unsigned transaction that sends the specified amount from the wallet to the given
 * address.  The address must satisfy BRAddressIsValid() using the provided BRWalletManager
 * address parameters.  In particular, a BCH address (either mainnet or testnet) *does not*
 * satisfy BRAddressIsValid() - the BCH address needs to be decoded into a valid BTC address first.
 *
 * @returns NULL on failure, or a transaction on success; the returned transaction must be freed
 *     using BRTransactionFree()
 */
extern BRTransaction *
BRWalletManagerCreateTransaction (BRWalletManager manager,
                                  BRWallet *wallet,
                                  uint64_t amount,
                                  BRAddress addr,
                                  uint64_t feePerKb);

extern BRTransaction *
BRWalletManagerCreateTransactionForSweep (BRWalletManager manager,
                                          BRWallet *wallet,
                                          BRWalletSweeper sweeper,
                                          uint64_t feePerKb);

extern BRTransaction *
BRWalletManagerCreateTransactionForOutputs (BRWalletManager manager,
                                            BRWallet *wallet,
                                            BRTxOutput *outputs,
                                            size_t outputsLen,
                                            uint64_t feePerKb);

/**
 * Signs any inputs in transaction that can be signed using private keys from the wallet.
 *
 * @seed the master private key (wallet seed) corresponding to the master public key given when the wallet was created
 *
 * @return true if all inputs were signed, or false if there was an error or not all inputs were able to be signed
 */
extern int
BRWalletManagerSignTransaction (BRWalletManager manager,
                                BRWallet *wallet,
                                OwnershipKept BRTransaction *transaction,
                                const void *seed,
                                size_t seedLen);

extern int
BRWalletManagerSignTransactionForKey (BRWalletManager manager,
                                      BRWallet *wallet,
                                      OwnershipKept BRTransaction *transaction,
                                      BRKey *key);

extern void
BRWalletManagerSubmitTransaction (BRWalletManager manager,
                                  BRWallet *wallet,
                                  OwnershipKept BRTransaction *transaction);

extern void
BRWalletManagerUpdateFeePerKB (BRWalletManager manager,
                               BRWallet *wallet,
                               uint64_t feePerKb);

extern void
BRWalletManagerEstimateFeeForTransfer (BRWalletManager manager,
                                       BRWallet *wallet,
                                       BRCookie cookie,
                                       uint64_t transferAmount,
                                       uint64_t feePerKb);

extern void
BRWalletManagerEstimateFeeForSweep (BRWalletManager manager,
                                    BRWallet *wallet,
                                    BRCookie cookie,
                                    BRWalletSweeper sweeper,
                                    uint64_t feePerKb);

extern void
BRWalletManagerEstimateFeeForOutputs (BRWalletManager manager,
                                      BRWallet *wallet,
                                      BRCookie cookie,
                                      BRTxOutput *outputs,
                                      size_t outputsLen,
                                      uint64_t feePerKb);

extern BRFileService
BRWalletManagerCreateFileService (const BRChainParams *params,
                                  const char *storagePath,
                                  BRFileServiceContext context,
                                  BRFileServiceErrorHandler handler);

extern void
BRWalletManagerExtractFileServiceTypes (BRFileService fileService,
                                        const char **transactions,
                                        const char **blocks,
                                        const char **peers);

extern void
BRWalletManagerWipe (const BRChainParams *params,
                     const char *baseStoragePath);

//
// Mark: Wallet Sweeper
//

typedef enum {
    WALLET_SWEEPER_SUCCESS,
    WALLET_SWEEPER_INVALID_TRANSACTION,
    WALLET_SWEEPER_INVALID_SOURCE_WALLET,
    WALLET_SWEEPER_NO_TRANSACTIONS_FOUND,
    WALLET_SWEEPER_INSUFFICIENT_FUNDS,
    WALLET_SWEEPER_UNABLE_TO_SWEEP,
} BRWalletSweeperStatus;

extern BRWalletSweeperStatus
BRWalletSweeperValidateSupported (BRKey *key,
                                  BRAddressParams addrParams,
                                  BRWallet *wallet);

extern BRWalletSweeper // NULL on error
BRWalletSweeperNew (BRKey *key,
                    BRAddressParams addrParams,
                    uint8_t isSegwit);

extern void
BRWalletSweeperFree (BRWalletSweeper sweeper);

extern BRWalletSweeperStatus
BRWalletSweeperHandleTransaction (BRWalletSweeper sweeper,
                                  OwnershipKept uint8_t *transaction,
                                  size_t transactionLen);

extern char *
BRWalletSweeperGetLegacyAddress (BRWalletSweeper sweeper);

extern uint64_t
BRWalletSweeperGetBalance (BRWalletSweeper sweeper);

extern BRWalletSweeperStatus
BRWalletSweeperValidate (BRWalletSweeper sweeper);

#ifdef __cplusplus
}
#endif


#endif /* BRWalletManager_h */
