//
//  BRCryptoWalletManager.h
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoWalletManager_h
#define BRCryptoWalletManager_h

#include "BRCryptoBase.h"
#include "BRCryptoKey.h"
#include "BRCryptoNetwork.h"
#include "BRCryptoPeer.h"
#include "BRCryptoAccount.h"
#include "BRCryptoTransfer.h"
#include "BRCryptoWallet.h"
#include "BRCryptoSync.h"
#include "BRCryptoWalletManagerClient.h"

#ifdef __cplusplus
extern "C" {
#endif

    /// MARK: - Wallet Manager Disconnect Reason

    typedef enum {
        CRYPTO_WALLET_MANAGER_DISCONNECT_REASON_REQUESTED,
        CRYPTO_WALLET_MANAGER_DISCONNECT_REASON_UNKNOWN,
        CRYPTO_WALLET_MANAGER_DISCONNECT_REASON_POSIX
    } BRCryptoWalletManagerDisconnectReasonType;

    typedef struct {
        BRCryptoWalletManagerDisconnectReasonType type;
        union {
            struct {
                int errnum;
            } posix;
        } u;
    } BRCryptoWalletManagerDisconnectReason;

    extern BRCryptoWalletManagerDisconnectReason
    cryptoWalletManagerDisconnectReasonRequested (void);

    extern BRCryptoWalletManagerDisconnectReason
    cryptoWalletManagerDisconnectReasonUnknown (void);

    extern BRCryptoWalletManagerDisconnectReason
    cryptoWalletManagerDisconnectReasonPosix (int errnum);

    /**
     * Return a descriptive message as to why the disconnect occurred.
     *
     *@return the detailed reason as a string or NULL
     */
    extern char *
    cryptoWalletManagerDisconnectReasonGetMessage (BRCryptoWalletManagerDisconnectReason *reason);

    /// MARK: Wallet Manager Event

    typedef enum {
        CRYPTO_WALLET_MANAGER_STATE_CREATED,
        CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED,
        CRYPTO_WALLET_MANAGER_STATE_CONNECTED,
        CRYPTO_WALLET_MANAGER_STATE_SYNCING,
        CRYPTO_WALLET_MANAGER_STATE_DELETED
    } BRCryptoWalletManagerStateType;

    typedef struct {
        BRCryptoWalletManagerStateType type;
        union {
            struct {
                BRCryptoWalletManagerDisconnectReason reason;
            } disconnected;
        } u;
    } BRCryptoWalletManagerState;

    extern const BRCryptoWalletManagerState CRYPTO_WALLET_MANAGER_STATE_CREATED_INIT;

    typedef enum {
        CRYPTO_WALLET_MANAGER_EVENT_CREATED,
        CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
        CRYPTO_WALLET_MANAGER_EVENT_DELETED,

        CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
        CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED,
        CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED,

        // wallet: added, ...
        CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED,
        CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES,
        CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED,
        CRYPTO_WALLET_MANAGER_EVENT_SYNC_RECOMMENDED,

        CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED,
    } BRCryptoWalletManagerEventType;

    extern const char *
    cryptoWalletManagerEventTypeString (BRCryptoWalletManagerEventType t);

    typedef struct {
        BRCryptoWalletManagerEventType type;
        union {
            struct {
                BRCryptoWalletManagerState oldValue;
                BRCryptoWalletManagerState newValue;
            } state;

            struct {
                /// Handler must 'give'
                BRCryptoWallet value;
            } wallet;

            struct {
                BRCryptoSyncTimestamp timestamp;
                BRCryptoSyncPercentComplete percentComplete;
            } syncContinues;

            struct {
                BRCryptoSyncStoppedReason reason;
            } syncStopped;

            struct {
                BRCryptoSyncDepth depth;
            } syncRecommended;

            struct {
                uint64_t value;
            } blockHeight;
        } u;
    } BRCryptoWalletManagerEvent;

    /// MARK: Listener

    typedef void *BRCryptoCWMListenerContext;

    /// Handler must 'give': manager, event.wallet.value
    typedef void (*BRCryptoCWMListenerWalletManagerEvent) (BRCryptoCWMListenerContext context,
                                                           BRCryptoWalletManager manager,
                                                           BRCryptoWalletManagerEvent event);

    /// Handler must 'give': manager, wallet, event.*
    typedef void (*BRCryptoCWMListenerWalletEvent) (BRCryptoCWMListenerContext context,
                                                    BRCryptoWalletManager manager,
                                                    BRCryptoWallet wallet,
                                                    BRCryptoWalletEvent event);

    /// Handler must 'give': manager, wallet, transfer
    typedef void (*BRCryptoCWMListenerTransferEvent) (BRCryptoCWMListenerContext context,
                                                      BRCryptoWalletManager manager,
                                                      BRCryptoWallet wallet,
                                                      BRCryptoTransfer transfer,
                                                      BRCryptoTransferEvent event);

    typedef struct {
        BRCryptoCWMListenerContext context;
        BRCryptoCWMListenerWalletManagerEvent walletManagerEventCallback;
        BRCryptoCWMListenerWalletEvent walletEventCallback;
        BRCryptoCWMListenerTransferEvent transferEventCallback;
    } BRCryptoCWMListener;

    /// MARK: Wallet Manager

    /// Can return NULL
    extern BRCryptoWalletManager
    cryptoWalletManagerCreate (BRCryptoCWMListener listener,
                               BRCryptoCWMClient client,
                               BRCryptoAccount account,
                               BRCryptoNetwork network,
                               BRCryptoSyncMode mode,
                               BRCryptoAddressScheme scheme,
                               const char *path);

    extern BRCryptoNetwork
    cryptoWalletManagerGetNetwork (BRCryptoWalletManager cwm);

    extern BRCryptoAccount
    cryptoWalletManagerGetAccount (BRCryptoWalletManager cwm);

    extern BRCryptoSyncMode
    cryptoWalletManagerGetMode (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerSetMode (BRCryptoWalletManager cwm, BRCryptoSyncMode mode);

    extern BRCryptoWalletManagerState
    cryptoWalletManagerGetState (BRCryptoWalletManager cwm);

    extern BRCryptoAddressScheme
    cryptoWalletManagerGetAddressScheme (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerSetAddressScheme (BRCryptoWalletManager cwm,
                                         BRCryptoAddressScheme scheme);

    extern const char *
    cryptoWalletManagerGetPath (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerSetNetworkReachable (BRCryptoWalletManager cwm,
                                            BRCryptoBoolean isNetworkReachable);

    extern BRCryptoBoolean
    cryptoWalletManagerHasWallet (BRCryptoWalletManager cwm,
                                  BRCryptoWallet wallet);

    extern BRCryptoWallet
    cryptoWalletManagerGetWallet (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerAddWallet (BRCryptoWalletManager cwm,
                                  BRCryptoWallet wallet);

    extern void
    cryptoWalletManagerRemWallet (BRCryptoWalletManager cwm,
                                  BRCryptoWallet wallet);

    /**
     * Returns a newly allocated array of the managers's wallets.
     *
     * The caller is responsible for deallocating the returned array using
     * free().
     *
     * @param cwm the wallet manager
     * @param count the number of wallets returned
     *
     * @return An array of wallets w/ an incremented reference count (aka 'taken')
     *         or NULL if there are no wallters in the manager.
     */
    extern BRCryptoWallet *
    cryptoWalletManagerGetWallets (BRCryptoWalletManager cwm,
                                   size_t *count);

    extern BRCryptoWallet
    cryptoWalletManagerGetWalletForCurrency (BRCryptoWalletManager cwm,
                                             BRCryptoCurrency currency);

    extern BRCryptoWallet
    cryptoWalletManagerRegisterWallet (BRCryptoWalletManager cwm,
                                       BRCryptoCurrency currency);

    extern void
    cryptoWalletManagerStop (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerConnect (BRCryptoWalletManager cwm,
                                BRCryptoPeer peer);

    extern void
    cryptoWalletManagerDisconnect (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerSync (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerSyncToDepth (BRCryptoWalletManager cwm,
                                    BRCryptoSyncDepth depth);

    // TODO: Workaround to create a TransferEvent.created for GEN (required CWM)
    extern BRCryptoTransfer
    cryptoWalletManagerCreateTransfer (BRCryptoWalletManager cwm,
                                       BRCryptoWallet wallet,
                                       BRCryptoAddress target,
                                       BRCryptoAmount amount,
                                       BRCryptoFeeBasis estimatedFeeBasis,
                                       size_t attributesCount,
                                       OwnershipKept BRCryptoTransferAttribute *attributes);

    extern BRCryptoBoolean
    cryptoWalletManagerSign (BRCryptoWalletManager cwm,
                             BRCryptoWallet wallet,
                             BRCryptoTransfer transfer,
                             const char *paperKey);

    extern void
    cryptoWalletManagerSubmit (BRCryptoWalletManager cwm,
                               BRCryptoWallet wid,
                               BRCryptoTransfer tid,
                               const char *paperKey);

    extern void
    cryptoWalletManagerSubmitForKey (BRCryptoWalletManager cwm,
                                     BRCryptoWallet wallet,
                                     BRCryptoTransfer transfer,
                                     BRCryptoKey key);

    extern void
    cryptoWalletManagerSubmitSigned (BRCryptoWalletManager cwm,
                                     BRCryptoWallet wallet,
                                     BRCryptoTransfer transfer);

    /**
     * Estimate the wallet's maximum or minimun transfer amount.
     */
    extern BRCryptoAmount
    cryptoWalletManagerEstimateLimit (BRCryptoWalletManager manager,
                                      BRCryptoWallet  wallet,
                                      BRCryptoBoolean asMaximum,
                                      BRCryptoAddress target,
                                      BRCryptoNetworkFee fee,
                                      BRCryptoBoolean *needEstimate,
                                      BRCryptoBoolean *isZeroIfInsuffientFunds);

    /**
     * Estimate the fee to transfer `amount` from `wallet` using the `feeBasis`.  Return an amount
     * represented in the wallet's fee currency.
     *
     * @param manager the manager
     * @param wallet the wallet
     * @param amount the amount to transfer
     * @param feeBasis the fee basis for the transfer
     *
     * @return the fee
     */

    extern void
    cryptoWalletManagerEstimateFeeBasis (BRCryptoWalletManager manager,
                                         BRCryptoWallet  wallet,
                                         BRCryptoCookie cookie,
                                         BRCryptoAddress target,
                                         BRCryptoAmount  amount,
                                         BRCryptoNetworkFee fee);

    extern void
    cryptoWalletManagerEstimateFeeBasisForWalletSweep (BRCryptoWalletManager manager,
                                                       BRCryptoWallet wallet,
                                                       BRCryptoCookie cookie,
                                                       BRCryptoWalletSweeper sweeper,
                                                       BRCryptoNetworkFee fee);

    extern void
    cryptoWalletManagerEstimateFeeBasisForPaymentProtocolRequest (BRCryptoWalletManager manager,
                                                                  BRCryptoWallet wallet,
                                                                  BRCryptoCookie cookie,
                                                                  BRCryptoPaymentProtocolRequest request,
                                                                  BRCryptoNetworkFee fee);

    extern void
    cryptoWalletManagerWipe (BRCryptoNetwork network,
                             const char *path);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoWalletManager, cryptoWalletManager);

    /// MARK: Wallet Migrator

    typedef struct BRCryptoWalletMigratorRecord *BRCryptoWalletMigrator;

    typedef enum {
        CRYPTO_WALLET_MIGRATOR_SUCCESS,
        CRYPTO_WALLET_MIGRATOR_ERROR_TRANSACTION,
        CRYPTO_WALLET_MIGRATOR_ERROR_BLOCK,
        CRYPTO_WALLET_MIGRATOR_ERROR_PEER
    } BRCryptoWalletMigratorStatusType;

    typedef struct {
        BRCryptoWalletMigratorStatusType type;
        // union {} u;
    } BRCryptoWalletMigratorStatus;

    extern BRCryptoWalletMigrator // NULL on error
    cryptoWalletMigratorCreate (BRCryptoNetwork network,
                                const char *storagePath);

    extern void
    cryptoWalletMigratorRelease (BRCryptoWalletMigrator migrator);

    extern BRCryptoWalletMigratorStatus
    cryptoWalletMigratorHandleTransactionAsBTC (BRCryptoWalletMigrator migrator,
                                                const uint8_t *bytes,
                                                size_t bytesCount,
                                                uint32_t blockHeight,
                                                uint32_t timestamp);

    extern BRCryptoWalletMigratorStatus
    cryptoWalletMigratorHandleBlockAsBTC (BRCryptoWalletMigrator migrator,
                                          BRCryptoData32 hash,
                                          uint32_t height,
                                          uint32_t nonce,
                                          uint32_t target,
                                          uint32_t txCount,
                                          uint32_t version,
                                          uint32_t timestamp,
                                          uint8_t *flags,  size_t flagsLen,
                                          BRCryptoData32 *hashes, size_t hashesCount,
                                          BRCryptoData32 merkleRoot,
                                          BRCryptoData32 prevBlock);

    extern BRCryptoWalletMigratorStatus
    cryptoWalletMigratorHandleBlockBytesAsBTC (BRCryptoWalletMigrator migrator,
                                               const uint8_t *bytes,
                                               size_t bytesCount,
                                               uint32_t height);

    extern BRCryptoWalletMigratorStatus
    cryptoWalletMigratorHandlePeerAsBTC (BRCryptoWalletMigrator migrator,
                                         uint32_t address,
                                         uint16_t port,
                                         uint64_t services,
                                         uint32_t timestamp);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoWalletManager_h */
