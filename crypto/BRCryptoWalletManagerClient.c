//
//  BRCryptoWalletManagerClient.c
//  BRCrypto
//
//  Created by Michael Carrara on 6/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <errno.h>
#include <math.h>  // round()

#include "BRCryptoBase.h"
#include "BRCryptoStatusP.h"
#include "BRCryptoNetworkP.h"
#include "BRCryptoAmountP.h"
#include "BRCryptoFeeBasisP.h"
#include "BRCryptoTransferP.h"
#include "BRCryptoWalletP.h"

#include "BRCryptoWalletManager.h"
#include "BRCryptoWalletManagerClient.h"
#include "BRCryptoWalletManagerP.h"

#include "bitcoin/BRWalletManager.h"
#include "ethereum/BREthereum.h"
#include "support/BRBase.h"

typedef enum  {
    CWM_CALLBACK_TYPE_BTC_GET_BLOCK_NUMBER,
    CWM_CALLBACK_TYPE_BTC_GET_TRANSACTIONS,
    CWM_CALLBACK_TYPE_BTC_SUBMIT_TRANSACTION,

    CWM_CALLBACK_TYPE_ETH_GET_BALANCE,
    CWM_CALLBACK_TYPE_ETH_GET_GAS_PRICE,
    CWM_CALLBACK_TYPE_ETH_ESTIMATE_GAS,
    CWM_CALLBACK_TYPE_ETH_SUBMIT_TRANSACTION,
    CWM_CALLBACK_TYPE_ETH_GET_TRANSACTIONS,
    CWM_CALLBACK_TYPE_ETH_GET_LOGS,
    CWM_CALLBACK_TYPE_ETH_GET_BLOCKS,
    CWM_CALLBACK_TYPE_ETH_GET_TOKENS,
    CWM_CALLBACK_TYPE_ETH_GET_BLOCK_NUMBER,
    CWM_CALLBACK_TYPE_ETH_GET_NONCE,

    CWM_CALLBACK_TYPE_GEN_GET_BLOCK_NUMBER,
    CWM_CALLBACK_TYPE_GEN_GET_TRANSACTIONS,
    CWM_CALLBACK_TYPE_GEN_GET_TRANSFERS,
    CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION,

} BRCryptoCWMCallbackType;

struct BRCryptoCWMClientCallbackStateRecord {
    BRCryptoCWMCallbackType type;
    union {
        struct {
            UInt256 txHash;
        } btcSubmit;
        struct {
            BREthereumWallet wid;
        } ethWithWallet;
        struct {
            BREthereumWallet wid;
            BREthereumCookie cookie;
        } ethWithWalletAndCookie;
        struct {
            BREthereumWallet wid;
            BREthereumTransfer tid;
        } ethWithTransaction;

        struct {
            BRGenericWallet wid;
        } genWithWallet;
        struct {
            BRGenericWallet wid;
            BRGenericTransfer tid; // A copy; must be released.
        } genWithTransaction;
    } u;
    int rid;
};

static void
cwmClientCallbackStateRelease (BRCryptoCWMClientCallbackState state) {
    switch (state->type) {
        case CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION:
            genTransferRelease (state->u.genWithTransaction.tid);
            break;
        default:
            break;
    }
    free (state);
}

/// MARK: - BTC Callbacks

static void
cwmGetBlockNumberAsBTC (BRWalletManagerClientContext context,
                        BRWalletManager manager,
                        int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_BTC_GET_BLOCK_NUMBER;
    callbackState->rid = rid;

    cwm->client.btc.funcGetBlockNumber (cwm->client.context,
                                        cryptoWalletManagerTake (cwm),
                                        callbackState);

    cryptoWalletManagerGive (cwm);
}

static void
cwmGetTransactionsAsBTC (BRWalletManagerClientContext context,
                         BRWalletManager manager,
                         OwnershipKept const char **addresses,
                         size_t addressCount,
                         uint64_t begBlockNumber,
                         uint64_t endBlockNumber,
                         int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_BTC_GET_TRANSACTIONS;
    callbackState->rid = rid;

    cwm->client.btc.funcGetTransactions (cwm->client.context,
                                         cryptoWalletManagerTake (cwm),
                                         callbackState,
                                         addresses,
                                         addressCount,
                                         begBlockNumber,
                                         endBlockNumber);

    cryptoWalletManagerGive (cwm);
}

static void
cwmSubmitTransactionAsBTC (BRWalletManagerClientContext context,
                           BRWalletManager manager,
                           BRWallet *wallet,
                           OwnershipKept uint8_t *transaction,
                           size_t transactionLength,
                           UInt256 transactionHash,
                           int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_BTC_SUBMIT_TRANSACTION;
    callbackState->u.btcSubmit.txHash = transactionHash;
    callbackState->rid = rid;

    UInt256 txHash = UInt256Reverse (transactionHash);
    char *hashAsHex = encodeHexCreate (NULL, txHash.u8, sizeof(txHash.u8));

    cwm->client.btc.funcSubmitTransaction (cwm->client.context,
                                           cryptoWalletManagerTake (cwm),
                                           callbackState,
                                           transaction,
                                           transactionLength,
                                           hashAsHex);

    free (hashAsHex);
    cryptoWalletManagerGive (cwm);
}

static void
cwmWalletManagerEventAsBTC (BRWalletManagerClientContext context,
                            OwnershipKept BRWalletManager btcManager,
                            BRWalletManagerEvent event) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    // Avoid a race condition by ensuring cwm->u.btc
    if (NULL == cwm->u.btc) cwm->u.btc = btcManager;

    assert (BLOCK_CHAIN_TYPE_BTC == cwm->type);

    int needEvent = 1;
    BRCryptoWalletManagerEvent cwmEvent = { CRYPTO_WALLET_MANAGER_EVENT_CREATED };

    switch (event.type) {
        case BITCOIN_WALLET_MANAGER_CREATED: {
            // Demand a 'wallet'
            BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsBTC (cwm, BRWalletManagerGetWallet (btcManager));
            assert (NULL != wallet);
            cryptoWalletGive (wallet);

            // Clear need for event as we propagate them here
            needEvent = 0;

            // Generate a CRYPTO wallet manager event for CREATED...
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                                                          CRYPTO_WALLET_MANAGER_EVENT_CREATED
                                                      });

            // Generate a CRYPTO wallet event for CREATED...
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (cwm->wallet),
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_CREATED
                                               });

            // ... and then a CRYPTO wallet manager event for WALLET_ADDED
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                                                          CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                          { .wallet = { cryptoWalletTake (cwm->wallet) }}
                                                      });
            break;
        }

        case BITCOIN_WALLET_MANAGER_CONNECTED: {
            BRCryptoWalletManagerState state = cryptoWalletManagerStateInit (CRYPTO_WALLET_MANAGER_STATE_CONNECTED);
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = { cwm->state, state }}
            };
            cryptoWalletManagerSetState (cwm, state);
            break;
        }
        case BITCOIN_WALLET_MANAGER_DISCONNECTED: {
            BRCryptoWalletManagerState state = cryptoWalletManagerStateDisconnectedInit (event.u.disconnected.reason);
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = { cwm->state, state }}
            };
            cryptoWalletManagerSetState (cwm, state);
            break;
        }
        case BITCOIN_WALLET_MANAGER_SYNC_STARTED: {
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED
            };
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      cwmEvent);

            BRCryptoWalletManagerState state = cryptoWalletManagerStateInit (CRYPTO_WALLET_MANAGER_STATE_SYNCING);
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = { cwm->state, state }}
            };
            cryptoWalletManagerSetState (cwm, state);
            break;
        }
        case BITCOIN_WALLET_MANAGER_SYNC_PROGRESS: {
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES,
                { .syncContinues = {
                    event.u.syncProgress.timestamp,
                    event.u.syncProgress.percentComplete
                }}
            };
            break;
        }
        case BITCOIN_WALLET_MANAGER_SYNC_STOPPED: {
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED,
                { .syncStopped = {
                    event.u.syncStopped.reason,
                }}
            };
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      cwmEvent);

            BRCryptoWalletManagerState state = cryptoWalletManagerStateInit (CRYPTO_WALLET_MANAGER_STATE_CONNECTED);
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = { cwm->state, state }}
            };
            cryptoWalletManagerSetState (cwm, state);
            break;
        }
        case BITCOIN_WALLET_MANAGER_SYNC_RECOMMENDED: {
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_SYNC_RECOMMENDED,
                { .syncRecommended = {
                    event.u.syncRecommended.depth,
                }}
            };
            break;
        }
        case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED: {
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED,
                { .blockHeight = { event.u.blockHeightUpdated.value }}
            };
            BRCryptoNetwork network = cryptoWalletManagerGetNetwork (cwm);
            cryptoNetworkSetHeight (network, event.u.blockHeightUpdated.value);
            cryptoNetworkGive (network);
            break;
        }
    }

    if (needEvent)
        cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                  cryptoWalletManagerTake (cwm),
                                                  cwmEvent);

    cryptoWalletManagerGive (cwm);
}

static void
cwmWalletEventAsBTC (BRWalletManagerClientContext context,
                     OwnershipKept BRWalletManager btcManager,
                     OwnershipKept BRWallet *btcWallet,
                     BRWalletEvent event) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    // Avoid a race condition by ensuring cwm->u.btc
    if (NULL == cwm->u.btc) cwm->u.btc = btcManager;

    assert (BLOCK_CHAIN_TYPE_BTC == cwm->type);

    switch (event.type) {
        case BITCOIN_WALLET_CREATED: {
            // Demand 'wallet'
            BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet);
            assert (NULL != wallet);

            cryptoWalletGive (wallet);
            break;
        }

        case BITCOIN_WALLET_BALANCE_UPDATED: {
            // Get `currency` (it is 'taken')
            BRCryptoCurrency currency = cryptoNetworkGetCurrency (cwm->network);

            // The balance value will be 'SATOSHI', so use the currency's base unit.
            BRCryptoUnit unit = cryptoNetworkGetUnitAsBase (cwm->network, currency);

            // Demand 'wallet'
            BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet);
            assert (NULL != wallet);

            // Get the amount (it is 'taken')
            BRCryptoAmount amount = cryptoAmountCreateInteger (event.u.balance.satoshi, unit); // taken

            // Generate BALANCE_UPDATED with 'amount' (taken)
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (wallet),
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_BALANCE_UPDATED,
                                                   { .balanceUpdated = { cryptoAmountTake (amount) }}
                                               });

            // ... and then a CRYPTO wallet manager event for WALLET_CHANGED
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                                                          CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED,
                                                          { .wallet = { cryptoWalletTake (wallet) }}
                                                      });

            cryptoAmountGive (amount);
            cryptoWalletGive (wallet);
            cryptoUnitGive (unit);
            cryptoCurrencyGive (currency);
            break;
        }

        case BITCOIN_WALLET_FEE_PER_KB_UPDATED: {
            // Demand 'wallet'
            BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet);
            assert (NULL != wallet);

            // Use the wallet's fee unit
            BRCryptoUnit feeUnit = cryptoWalletGetUnitForFee(wallet);

            // Create the fee basis using a default transaction size, in bytes, and the new fee per KB
            BRCryptoFeeBasis feeBasis = cryptoFeeBasisCreateAsBTC (feeUnit,
                                                                   (uint32_t) event.u.feePerKb.value,
                                                                   1000);

            // Generate FEE_BASIS_UPDATED for default fee basis change
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (wallet),
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED,
                                                   { .feeBasisUpdated = { cryptoFeeBasisTake (feeBasis) }}
                                               });

            // ... and then a CRYPTO wallet manager event for WALLET_CHANGED
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                                                          CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED,
                                                          { .wallet = { cryptoWalletTake (wallet) }}
                                                      });

            cryptoFeeBasisGive (feeBasis);
            cryptoUnitGive (feeUnit);
            cryptoWalletGive (wallet);
            break;
        }

        case BITCOIN_WALLET_TRANSACTION_SUBMIT_SUCCEEDED: {
            // Demand 'wallet'
            BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet);
            assert (NULL != wallet);

            // Find the wallet's transfer for 'btc'. (it is 'taken'); it must exist already in wallet (otherwise how
            // could it have been submitted?)
            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, event.u.submitSucceeded.transaction);
            assert (NULL != transfer);

            BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
            assert (CRYPTO_TRANSFER_STATE_SUBMITTED != oldState.type);

            BRCryptoTransferState newState = cryptoTransferStateInit (CRYPTO_TRANSFER_STATE_SUBMITTED);
            cryptoTransferSetState (transfer, newState);

            cwm->listener.transferEventCallback (cwm->listener.context,
                                                 cryptoWalletManagerTake (cwm),
                                                 cryptoWalletTake (wallet),
                                                 cryptoTransferTake (transfer),
                                                 (BRCryptoTransferEvent) {
                                                     CRYPTO_TRANSFER_EVENT_CHANGED,
                                                     { .state = { oldState, newState }}
                                                 });

            cryptoTransferGive (transfer);
            cryptoWalletGive (wallet);
            break;
        }

        case BITCOIN_WALLET_TRANSACTION_SUBMIT_FAILED: {
            // Demand 'wallet'
            BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet);
            assert (NULL != wallet);

            // Find the wallet's transfer for 'btc'. (it is 'taken'); it must exist already in wallet (otherwise how
            // could it have been submitted?)
            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, event.u.submitFailed.transaction);
            assert (NULL != transfer);

            BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
            // allow changes to different error states? don't assert (CRYPTO_TRANSFER_STATE_ERRORED != oldState.type);

            BRCryptoTransferState newState = cryptoTransferStateErroredInit (event.u.submitFailed.error);
            cryptoTransferSetState (transfer, newState);

            cwm->listener.transferEventCallback (cwm->listener.context,
                                                 cryptoWalletManagerTake (cwm),
                                                 cryptoWalletTake (wallet),
                                                 cryptoTransferTake (transfer),
                                                 (BRCryptoTransferEvent) {
                                                     CRYPTO_TRANSFER_EVENT_CHANGED,
                                                     { .state = { oldState, newState }}
                                                 });

            cryptoTransferGive (transfer);
            cryptoWalletGive (wallet);
            break;
        }

        case BITCOIN_WALLET_FEE_ESTIMATED: {
            // Demand 'wallet'
            BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet);
            assert (NULL != wallet);

            // Use the wallet's fee unit
            BRCryptoUnit feeUnit = cryptoWalletGetUnitForFee (wallet);

            // Create the fee basis using the transaction size, in bytes, and the fee per KB
            BRCryptoFeeBasis feeBasis = cryptoFeeBasisCreateAsBTC (feeUnit,
                                                                   (uint32_t) event.u.feeEstimated.feePerKb,
                                                                   event.u.feeEstimated.sizeInByte);

            // Generate FEE_BASIS_ESTIMATED
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (wallet),
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED,
                                                   { .feeBasisEstimated = {
                                                       CRYPTO_SUCCESS,
                                                       event.u.feeEstimated.cookie,
                                                       cryptoFeeBasisTake(feeBasis)
                                                   }}
                                               });

            cryptoFeeBasisGive (feeBasis);
            cryptoUnitGive (feeUnit);
            cryptoWalletGive (wallet);
            break;
        }

        case BITCOIN_WALLET_DELETED: {
            // Demand 'wallet' ...
            BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet);
            assert (NULL != wallet);

            // ...and CWM holding 'wallet'
            assert (CRYPTO_TRUE == cryptoWalletManagerHasWallet (cwm, wallet));

            // Update cwm to remove 'wallet'
            cryptoWalletManagerRemWallet (cwm, wallet);

            // Generate a CRYPTO wallet manager event for WALLET_DELETED...
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                                                          CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED,
                                                          { .wallet = { cryptoWalletTake (wallet) }}
                                                      });

            // ... and then a CRYPTO wallet event for DELETED.
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (wallet),
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_DELETED
                                               });

            cryptoWalletGive (wallet);
            break;
        }
    }

    cryptoWalletManagerGive (cwm);
}

static void
cwmTransactionEventAsBTC (BRWalletManagerClientContext context,
                          OwnershipKept BRWalletManager btcManager,
                          OwnershipKept BRWallet *btcWallet,
                          OwnershipKept BRTransaction *btcTransaction,
                          BRTransactionEvent event) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    // Avoid a race condition by ensuring cwm->u.btc
    if (NULL == cwm->u.btc) cwm->u.btc = btcManager;

    assert (BLOCK_CHAIN_TYPE_BTC == cwm->type);

    // Find 'wallet' based on BTC... (it is taken)
    BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet);

    // ... and demand 'wallet'
    assert (NULL != wallet && btcWallet == cryptoWalletAsBTC (wallet));

    switch (event.type) {

        case BITCOIN_TRANSACTION_CREATED: {
            // See the comments on the BRTransactionEventType type definition for details
            // on when this occurs.

            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, btcTransaction); // taken
            assert (NULL == transfer);

            BRCryptoUnit unit         = cryptoWalletGetUnit (wallet);
            BRCryptoUnit unitForFee   = cryptoWalletGetUnitForFee(wallet);
            BRCryptoBoolean isBTC     = AS_CRYPTO_BOOLEAN (BRWalletManagerHandlesBTC(btcManager));

            // The transfer finally - based on the wallet's currency (BTC)
            transfer = cryptoTransferCreateAsBTC (unit,
                                                  unitForFee,
                                                  cryptoWalletAsBTC (wallet),
                                                  btcTransaction,
                                                  isBTC);

            // Generate a CRYPTO transfer event for CREATED'...
            cwm->listener.transferEventCallback (cwm->listener.context,
                                                 cryptoWalletManagerTake (cwm),
                                                 cryptoWalletTake (wallet),
                                                 cryptoTransferTake (transfer),
                                                 (BRCryptoTransferEvent) {
                                                     CRYPTO_TRANSFER_EVENT_CREATED
                                                 });

            // ... add 'transfer' to 'wallet' (argubaly late... but to prove a point)...
            cryptoWalletAddTransfer (wallet, transfer);

            // ... and then generate a CRYPTO wallet event for 'TRANSFER_ADDED'
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (wallet),
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_TRANSFER_ADDED,
                                                   { .transfer = { cryptoTransferTake (transfer) }}
                                               });

            cryptoTransferGive (transfer);
            cryptoUnitGive (unitForFee);
            cryptoUnitGive (unit);
            break;
        }

        case BITCOIN_TRANSACTION_SIGNED: {
            // See the comments on the BRTransactionEventType type definition for details
            // on when this occurs.

            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, btcTransaction); // taken
            assert (NULL != transfer);

            BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
            assert (CRYPTO_TRANSFER_STATE_SIGNED != oldState.type);

            BRCryptoTransferState newState = cryptoTransferStateInit (CRYPTO_TRANSFER_STATE_SIGNED);
            cryptoTransferSetState (transfer, newState);

            cwm->listener.transferEventCallback (cwm->listener.context,
                                                 cryptoWalletManagerTake (cwm),
                                                 cryptoWalletTake (wallet),
                                                 cryptoTransferTake (transfer ),
                                                 (BRCryptoTransferEvent) {
                                                     CRYPTO_TRANSFER_EVENT_CHANGED,
                                                     { .state = { oldState, newState }}
                                                 });

            cryptoTransferGive (transfer);
            break;
        }

        case BITCOIN_TRANSACTION_ADDED: {
            // This event occurs when either a user created transaction has been submitted
            // or if the transaction arrived during a sync. If it came from a sync, this is
            // the first we will have seen it. If this is a user-generated transfer, we already
            // have a crypto transfer for it.
            //
            // See the comments on the BRTransactionEventType type definition for more details
            // on when this occurs.

            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, btcTransaction); // taken
            if (NULL == transfer) {
                BRCryptoUnit unit         = cryptoWalletGetUnit (wallet);
                BRCryptoUnit unitForFee   = cryptoWalletGetUnitForFee(wallet);
                BRCryptoBoolean isBTC     = AS_CRYPTO_BOOLEAN (BRWalletManagerHandlesBTC(btcManager));

                // The transfer finally - based on the wallet's currency (BTC)
                transfer = cryptoTransferCreateAsBTC (unit,
                                                      unitForFee,
                                                      cryptoWalletAsBTC (wallet),
                                                      btcTransaction,
                                                      isBTC);

                // Generate a CRYPTO transfer event for CREATED'...
                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     cryptoWalletTake (wallet),
                                                     cryptoTransferTake (transfer),
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CREATED
                                                     });

                // ... add 'transfer' to 'wallet' (argubaly late... but to prove a point)...
                cryptoWalletAddTransfer (wallet, transfer);

                // ... and then generate a CRYPTO wallet event for 'TRANSFER_ADDED'
                cwm->listener.walletEventCallback (cwm->listener.context,
                                                   cryptoWalletManagerTake (cwm),
                                                   cryptoWalletTake (wallet),
                                                   (BRCryptoWalletEvent) {
                                                       CRYPTO_WALLET_EVENT_TRANSFER_ADDED,
                                                       { .transfer = { cryptoTransferTake (transfer) }}
                                                   });

                cryptoUnitGive (unitForFee);
                cryptoUnitGive (unit);
            }

            // We do NOT announce a state change here because the BTC logic will send a
            // BITCOIN_TRANSACTION_UPDATED event to announce the transaction's height and
            // timestamp

            cryptoTransferGive (transfer);
            break;
        }

        case BITCOIN_TRANSACTION_UPDATED: {
            // This event occurs when the timestamp and/or blockHeight have been changed
            // due to the transaction being confirmed or unconfirmed (in the case of a blockchain
            // reorg).
            //
            // See the comments on the BRTransactionEventType type definition for more details
            // on when this occurs.

            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, btcTransaction); // taken
            assert (NULL != transfer);

            BRCryptoTransferState oldState = cryptoTransferGetState (transfer);

            // We will update the state in two cases:
            //     - If we are NOT in the SUBMITTED state and receive an event indicating that the
            //       transaction is UNCONFIRMED; then set the state to SUBMITTED
            //     - If we are NOT in the INCLDUED state and receive an event indicated that the
            //       transaction is CONFIRMED; then set the state to INCLUDED
            //     - Otherwise, ignore
            if (CRYPTO_TRANSFER_STATE_SUBMITTED != oldState.type &&
                (0 == event.u.updated.timestamp || TX_UNCONFIRMED == event.u.updated.blockHeight)) {
                BRCryptoTransferState newState = cryptoTransferStateInit (CRYPTO_TRANSFER_STATE_SUBMITTED);

                cryptoTransferSetState (transfer, newState);

                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     cryptoWalletTake (wallet),
                                                     cryptoTransferTake (transfer),
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CHANGED,
                                                         { .state = { oldState, newState }}
                                                     });

            } else if (CRYPTO_TRANSFER_STATE_INCLUDED != oldState.type &&
                       0 != event.u.updated.timestamp && TX_UNCONFIRMED != event.u.updated.blockHeight) {
                // The transfer is included and thus we now have a feeBasisConfirmed.  For BTC
                // the feeBasisConfirmed is identical to feeBasisEstimated
                BRCryptoFeeBasis feeBasisConfirmed = cryptoTransferGetEstimatedFeeBasis (transfer);

                BRCryptoTransferState newState = cryptoTransferStateIncludedInit (event.u.updated.blockHeight,
                                                                                  0,
                                                                                  event.u.updated.timestamp,
                                                                                  feeBasisConfirmed,
                                                                                  CRYPTO_TRUE,
                                                                                  NULL);

                cryptoFeeBasisGive(feeBasisConfirmed);

                cryptoTransferSetState (transfer, newState);

                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     cryptoWalletTake (wallet),
                                                     cryptoTransferTake (transfer),
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CHANGED,
                                                         { .state = { oldState, newState }}
                                                     });
            } else {
                // no change; just release the old state and carry on
                cryptoTransferStateRelease (&oldState);
            }

            cryptoTransferGive (transfer);
            break;
        }

        case BITCOIN_TRANSACTION_DELETED: {
            // This event occurs when a transaction has been deleted from a wallet.
            //
            // See the comments on the BRTransactionEventType type definition for more details
            // on when this occurs.

            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, btcTransaction); // taken
            assert (NULL != transfer);

            // Generate a CRYPTO wallet event for 'TRANSFER_DELETED'...
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (wallet),
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_TRANSFER_DELETED,
                                                   { .transfer = { cryptoTransferTake (transfer) }}
                                               });

            // ... Remove 'transfer' from 'wallet'
            cryptoWalletRemTransfer (wallet, transfer);

            // ... and then follow up with a CRYPTO transfer event for 'DELETED'
            cwm->listener.transferEventCallback (cwm->listener.context,
                                                 cryptoWalletManagerTake (cwm),
                                                 cryptoWalletTake (wallet),
                                                 cryptoTransferTake (transfer),
                                                 (BRCryptoTransferEvent) {
                                                     CRYPTO_TRANSFER_EVENT_DELETED
                                                 });

            cryptoTransferGive (transfer);
            break;
        }
    }

    cryptoWalletGive (wallet);
    cryptoWalletManagerGive (cwm);
}

/// MARK: ETH Callbacks

static BRCryptoWalletManagerState
cwmStateFromETH (BREthereumEWMState state) {
    switch (state) {
        case EWM_STATE_CREATED:      return cryptoWalletManagerStateInit (CRYPTO_WALLET_MANAGER_STATE_CREATED);
        case EWM_STATE_CONNECTED:    return cryptoWalletManagerStateInit (CRYPTO_WALLET_MANAGER_STATE_CONNECTED);
        case EWM_STATE_SYNCING:      return cryptoWalletManagerStateInit (CRYPTO_WALLET_MANAGER_STATE_SYNCING);
        case EWM_STATE_DISCONNECTED: return cryptoWalletManagerStateDisconnectedInit (cryptoWalletManagerDisconnectReasonUnknown());
        case EWM_STATE_DELETED:      return cryptoWalletManagerStateInit (CRYPTO_WALLET_MANAGER_STATE_DELETED);
    }
}

static void
cwmWalletManagerEventAsETH (BREthereumClientContext context,
                            BREthereumEWM ewm,
                            BREthereumEWMEvent event) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    // Avoid a race condition by ensuring cwm->u.eth
    if (NULL == cwm->u.eth) cwm->u.eth = ewm;

    int needEvent = 1;
    BRCryptoWalletManagerEvent cwmEvent = { CRYPTO_WALLET_MANAGER_EVENT_CREATED };  // avoid warning

    switch (event.type) {
        case EWM_EVENT_CREATED: {
            // Demand a 'wallet'
            BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsETH (cwm, ewmGetWallet (ewm));
            assert (NULL != wallet);
            cryptoWalletGive (wallet);

            // Clear need for event as we propagate them here
            needEvent = 0;

            // Generate a CRYPTO wallet manager event for CREATED...
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                                                          CRYPTO_WALLET_MANAGER_EVENT_CREATED
                                                      });

            // Generate a CRYPTO wallet event for CREATED...
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (cwm->wallet),
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_CREATED
                                               });

            // ... and then a CRYPTO wallet manager event for WALLET_ADDED
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                                                          CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                          { .wallet = { cryptoWalletTake (cwm->wallet) }}
                                                      });

            break;
        }

        case EWM_EVENT_CHANGED:
            // If the newState is `syncing` we want a syncStarted event
            if (EWM_STATE_SYNCING == event.u.changed.newState) {
                assert (EWM_STATE_CONNECTED == event.u.changed.oldState);
                cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                          cryptoWalletManagerTake(cwm),
                                                          (BRCryptoWalletManagerEvent) {
                                                              CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED
                                                          });
            }

            // If the oldState is `syncing` we want a syncEnded event
            if (EWM_STATE_SYNCING == event.u.changed.oldState) {
                assert (EWM_STATE_CONNECTED == event.u.changed.newState);
                cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                          cryptoWalletManagerTake(cwm),
                                                          (BRCryptoWalletManagerEvent) {
                                                              CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED,
                                                              { .syncStopped = { cryptoSyncStoppedReasonComplete() } }
                                                          });
            }

            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = {
                    cwmStateFromETH (event.u.changed.oldState),
                    cwmStateFromETH (event.u.changed.newState)
                }}};

            break;

        case EWM_EVENT_SYNC_PROGRESS:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES,
                { .syncContinues = {
                    event.u.syncProgress.timestamp,
                    event.u.syncProgress.percentComplete }}
            };
            break;

        case EWM_EVENT_NETWORK_UNAVAILABLE:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = { cwm->state, CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED }}
            };
            cryptoWalletManagerSetState (cwm, cryptoWalletManagerStateDisconnectedInit ( cryptoWalletManagerDisconnectReasonUnknown() ));
            break;

        case EWM_EVENT_BLOCK_HEIGHT_UPDATED: {
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED,
                { .blockHeight = { event.u.blockHeight.value }}
            };
            BRCryptoNetwork network = cryptoWalletManagerGetNetwork (cwm);
            cryptoNetworkSetHeight (network, event.u.blockHeight.value);
            cryptoNetworkGive (network);
            break;
        }
        case EWM_EVENT_DELETED:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_DELETED
            };
            break;
    }

    if (needEvent)
        cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                  cryptoWalletManagerTake (cwm),
                                                  cwmEvent);

    cryptoWalletManagerGive (cwm);
}

static void
cwmPeerEventAsETH (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   BREthereumPeerEvent event) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    // Avoid a race condition by ensuring cwm->u.eth
    if (NULL == cwm->u.eth) cwm->u.eth = ewm;

    cryptoWalletManagerGive (cwm);
}

static void
cwmWalletEventAsETH (BREthereumClientContext context,
                     BREthereumEWM ewm,
                     BREthereumWallet wid,
                     BREthereumWalletEvent event) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    // Avoid a race condition by ensuring cwm->u.eth
    if (NULL == cwm->u.eth) cwm->u.eth = ewm;

    BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsETH (cwm, wid); // taken

    switch (event.type) {
        case WALLET_EVENT_CREATED: {
            // The primary wallet was created/added in the EWM_EVENT_CREATED handler
            if (NULL != wallet) {
                cryptoWalletGive (wallet);

            // We only need to handle newly observed token wallets here
            } else  {
                BREthereumToken token = ewmWalletGetToken (ewm, wid);
                assert (NULL != token);

                // Find the wallet's currency.
                BRCryptoCurrency currency = cryptoNetworkGetCurrencyforTokenETH (cwm->network, token);

                // The currency might not exist.  We installed all tokens announced by
                // `ewmGetTokens()` but, at least during debugging, not all of those tokens will
                // have a corresponding currency.
                //
                // If a currency does exit, then when we get the EWM TOKEN_CREATED event we'll
                // 'ping' the EWM wallet which will create the EWM wallet and bring us here where
                // we'll create the CRYPTO wallet (based on having the token+currency).  However,
                // if we installed token X, don't have Currency X BUT FOUND A LOG during sync, then
                // the EWM wallet gets created automaticaly and we end up here w/o a Currency.
                //
                // Thus:
                if (NULL == currency) return;

                // Find the default unit; it too must exist.
                BRCryptoUnit    unit = cryptoNetworkGetUnitAsDefault (cwm->network, currency);
                assert (NULL != unit);

                // Find the fee Unit
                BRCryptoCurrency feeCurrency = cryptoNetworkGetCurrency (cwm->network);
                assert (NULL != feeCurrency);

                BRCryptoUnit     feeUnit     = cryptoNetworkGetUnitAsDefault (cwm->network, feeCurrency);
                assert (NULL != feeUnit);

                // Create the appropriate wallet based on currency
                wallet = cryptoWalletCreateAsETH (unit, feeUnit, cwm->u.eth, wid); // taken

                cryptoWalletManagerAddWallet (cwm, wallet);

                cryptoUnitGive (feeUnit);
                cryptoCurrencyGive (feeCurrency);

                cryptoUnitGive (unit);
                cryptoCurrencyGive (currency);

                // This is invoked directly on an EWM thread. (as is all this function's code).
                cwm->listener.walletEventCallback (cwm->listener.context,
                                                   cryptoWalletManagerTake (cwm),
                                                   cryptoWalletTake (wallet),
                                                   (BRCryptoWalletEvent) {
                                                       CRYPTO_WALLET_EVENT_CREATED
                                                   });

                cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                          cryptoWalletManagerTake (cwm),
                                                          (BRCryptoWalletManagerEvent) {
                                                              CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                              { .wallet = { wallet }}
                                                          });
            }
            break;
        }

        case WALLET_EVENT_BALANCE_UPDATED: {
            if (NULL != wallet) {
                BRCryptoUnit unit = cryptoWalletGetUnit(wallet);

                // Get the wallet's amount...
                BREthereumAmount amount = ewmWalletGetBalance(cwm->u.eth, wid);

                // ... and then the 'raw integer' (UInt256) value
                UInt256 value = (AMOUNT_ETHER == amountGetType(amount)
                                 ? amountGetEther(amount).valueInWEI
                                 : amountGetTokenQuantity(amount).valueAsInteger);

                // Create a cryptoAmount in the wallet's unit.
                BRCryptoAmount cryptoAmount = cryptoAmountCreate (unit, CRYPTO_FALSE, value);

                cryptoUnitGive(unit);

                // Generate a BALANCE_UPDATED for the wallet
                cwm->listener.walletEventCallback(cwm->listener.context,
                                                  cryptoWalletManagerTake(cwm),
                                                  cryptoWalletTake (wallet),
                                                  (BRCryptoWalletEvent) {
                                                      CRYPTO_WALLET_EVENT_BALANCE_UPDATED,
                                                      {.balanceUpdated = {cryptoAmount}}
                                                  });

                // ... and then a CRYPTO wallet manager event for WALLET_CHANGED
                cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                          cryptoWalletManagerTake (cwm),
                                                          (BRCryptoWalletManagerEvent) {
                                                              CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED,
                                                              { .wallet = { wallet }}
                                                          });
            }
            break;
        }

        case WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED:
        case WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED:
            if (NULL != wallet) {
                BRCryptoUnit feeUnit = cryptoWalletGetUnitForFee(wallet);

                BRCryptoFeeBasis feeBasis = cryptoFeeBasisCreateAsETH (feeUnit,
                                                                       ewmWalletGetDefaultGasLimit(cwm->u.eth, wid),
                                                                       ewmWalletGetDefaultGasPrice(cwm->u.eth,wid));
                // Generate a FEE_BASIS_UPDATED for the wallet
                cwm->listener.walletEventCallback(cwm->listener.context,
                                                  cryptoWalletManagerTake(cwm),
                                                  cryptoWalletTake (wallet),
                                                  (BRCryptoWalletEvent) {
                                                      CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED,
                                                      {.feeBasisUpdated = {feeBasis}}
                                                  });

                // ... and then a CRYPTO wallet manager event for WALLET_CHANGED
                cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                          cryptoWalletManagerTake (cwm),
                                                          (BRCryptoWalletManagerEvent) {
                                                              CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED,
                                                              { .wallet = { wallet }}
                                                          });
                cryptoUnitGive (feeUnit);
            }
            break;

        case WALLET_EVENT_FEE_ESTIMATED:
            if (NULL != wallet) {
                if (SUCCESS == event.status) {
                    BRCryptoUnit feeUnit = cryptoWalletGetUnitForFee(wallet);

                    BRCryptoFeeBasis feeBasis = cryptoFeeBasisCreateAsETH (feeUnit, event.u.feeEstimate.gasEstimate, event.u.feeEstimate.gasPrice);

                    cwm->listener.walletEventCallback(cwm->listener.context,
                                                      cryptoWalletManagerTake(cwm),
                                                      wallet,
                                                      (BRCryptoWalletEvent) {
                                                           CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED,
                                                           { .feeBasisEstimated = {
                                                               CRYPTO_SUCCESS,
                                                               event.u.feeEstimate.cookie,
                                                               feeBasis
                                                           }}
                                                       });

                    cryptoUnitGive (feeUnit);
                } else {
                    cwm->listener.walletEventCallback(cwm->listener.context,
                                                      cryptoWalletManagerTake(cwm),
                                                      wallet,
                                                      (BRCryptoWalletEvent) {
                                                           CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED,
                                                           { .feeBasisEstimated = {
                                                               cryptoStatusFromETH (event.status),
                                                               event.u.feeEstimate.cookie,
                                                           }}
                                                       });
                }
            }
            break;

        case WALLET_EVENT_DELETED:
            if (NULL != wallet) {
                // Generate a CRYPTO wallet manager event for WALLET_DELETED...
                cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                          cryptoWalletManagerTake (cwm),
                                                          (BRCryptoWalletManagerEvent) {
                                                              CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED,
                                                              { .wallet = { cryptoWalletTake (wallet) }}
                                                          });

                // ... and then a CRYPTO wallet event for DELETED.
                cwm->listener.walletEventCallback (cwm->listener.context,
                                                   cryptoWalletManagerTake (cwm),
                                                   wallet,
                                                   (BRCryptoWalletEvent) {
                                                       CRYPTO_WALLET_EVENT_DELETED
                                                   });
            }
            break;
    }

    cryptoWalletManagerGive (cwm);
}

static void
cwmEventTokenAsETH (BREthereumClientContext context,
                    BREthereumEWM ewm,
                    BREthereumToken token,
                    BREthereumTokenEvent event) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    // Avoid a race condition by ensuring cwm->u.eth
    if (NULL == cwm->u.eth) cwm->u.eth = ewm;

    switch (event.type) {
        case TOKEN_EVENT_CREATED: {
            BRCryptoNetwork network = cryptoWalletManagerGetNetwork (cwm);

            // A token was created.  We want a corresponding EWM wallet to be created as well; it
            // will be created automatically by simply 'pinging' the wallet for token.  However,
            // only create the token's wallet if we know about the currency.

            BRCryptoCurrency currency = cryptoNetworkGetCurrencyforTokenETH (network, token);

            if (NULL != currency) {
                ewmGetWalletHoldingToken (ewm, token);
                cryptoCurrencyGive (currency);
            }

            cryptoNetworkGive(network);

            // This will cascade into a WALLET_EVENT_CREATED which will in turn create a
            // BRCryptoWallet too

            // Nothing more
            break;
        }
        case TOKEN_EVENT_DELETED:
            // Nothing more (for now)
            break;
    }

    cryptoWalletManagerGive (cwm);
}


static void
cwmTransactionEventAsETH (BREthereumClientContext context,
                          BREthereumEWM ewm,
                          BREthereumWallet wid,
                          BREthereumTransfer tid,
                          BREthereumTransferEvent event) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    // Avoid a race condition by ensuring cwm->u.eth
    if (NULL == cwm->u.eth) cwm->u.eth = ewm;

    BRCryptoWallet wallet     = cryptoWalletManagerFindWalletAsETH (cwm, wid); // taken
    // TODO: Wallet may be NULL for a sync-discovered transfer w/o a currency.
    if (NULL == wallet) return;

    BRCryptoTransfer transfer = cryptoWalletFindTransferAsETH (wallet, tid);   // taken

    switch (event.type) {
        case TRANSFER_EVENT_CREATED: {
            assert (NULL == transfer);
            if (NULL == transfer) {
                BRCryptoUnit unit       = cryptoWalletGetUnit (wallet);
                BRCryptoUnit unitForFee = cryptoWalletGetUnitForFee(wallet);

                transfer = cryptoTransferCreateAsETH (unit,
                                                      unitForFee,
                                                      cwm->u.eth,
                                                      tid,
                                                      NULL); // taken

                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     cryptoWalletTake (wallet),
                                                     cryptoTransferTake (transfer),
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CREATED
                                                     });

                cryptoWalletAddTransfer (wallet, transfer);

                cwm->listener.walletEventCallback (cwm->listener.context,
                                                   cryptoWalletManagerTake (cwm),
                                                   cryptoWalletTake (wallet),
                                                   (BRCryptoWalletEvent) {
                                                       CRYPTO_WALLET_EVENT_TRANSFER_ADDED,
                                                       { .transfer = { cryptoTransferTake (transfer) }}
                                                   });

                cryptoUnitGive (unitForFee);
                cryptoUnitGive (unit);
            }
            break;
        }

        case TRANSFER_EVENT_SIGNED: {
            assert (NULL != transfer);
            if (NULL != transfer) {
                BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
                BRCryptoTransferState newState = cryptoTransferStateInit (CRYPTO_TRANSFER_STATE_SIGNED);

                cryptoTransferSetState (transfer, newState);

                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     cryptoWalletTake (wallet),
                                                     cryptoTransferTake (transfer),
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CHANGED,
                                                         { .state = { oldState, newState }}
                                                     });
            }
            break;
        }

        case TRANSFER_EVENT_SUBMITTED: {
            assert (NULL != transfer);
            if (NULL != transfer) {
                BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
                BRCryptoTransferState newState = cryptoTransferStateInit (CRYPTO_TRANSFER_STATE_SUBMITTED);

                cryptoTransferSetState (transfer, newState);

                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     cryptoWalletTake (wallet),
                                                     cryptoTransferTake (transfer),
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CHANGED,
                                                         { .state = { oldState, newState }}
                                                     });
            }
            break;
        }

        case TRANSFER_EVENT_INCLUDED: {
            assert (NULL != transfer);
            if (NULL != transfer ){
                uint64_t blockNumber, blockTransactionIndex, blockTimestamp;
                BREthereumGas gasUsed;

                BRCryptoTransferState oldState = cryptoTransferGetState (transfer);

                BREthereumFeeBasis ethFeeBasis = ewmTransferGetFeeBasis (ewm, tid);

                BRCryptoUnit unit = cryptoTransferGetUnitForFee(transfer);
                BRCryptoFeeBasis feeBasisConfirmed = cryptoFeeBasisCreateAsETH (unit,
                                                                                feeBasisGetGasLimit(ethFeeBasis),
                                                                                feeBasisGetGasPrice(ethFeeBasis));

                ewmTransferExtractStatusIncluded(ewm, tid, NULL, &blockNumber, &blockTransactionIndex, &blockTimestamp, &gasUsed);
                BRCryptoTransferState newState = cryptoTransferStateIncludedInit (blockNumber,
                                                                                  blockTransactionIndex,
                                                                                  blockTimestamp,
                                                                                  feeBasisConfirmed,
                                                                                  CRYPTO_TRUE,
                                                                                  NULL);

                cryptoFeeBasisGive (feeBasisConfirmed);
                cryptoUnitGive (unit);

                cryptoTransferSetState (transfer, newState);

                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     cryptoWalletTake (wallet),
                                                     cryptoTransferTake (transfer),
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CHANGED,
                                                         { .state = { oldState, newState }}
                                                     });
            }
            break;
        }

        case TRANSFER_EVENT_ERRORED: {
            assert (NULL != transfer);
            if (NULL != transfer) {
                BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
                BRCryptoTransferState newState = cryptoTransferStateErroredInit (cryptoTransferSubmitErrorUnknown ());

                cryptoTransferSetState (transfer, newState);

                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     cryptoWalletTake (wallet),
                                                     cryptoTransferTake (transfer),
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CHANGED,
                                                         { .state = { oldState, newState }}
                                                     });
            }
            break;
        }

        case TRANSFER_EVENT_GAS_ESTIMATE_UPDATED: {
            assert (NULL != transfer);
            break;
        }

        case TRANSFER_EVENT_DELETED: {
            assert (NULL != transfer);
            if (NULL != transfer) {
                cryptoWalletRemTransfer (wallet, transfer);

                // Deleted from wallet
                cwm->listener.walletEventCallback (cwm->listener.context,
                                                   cryptoWalletManagerTake (cwm),
                                                   cryptoWalletTake (wallet),
                                                   (BRCryptoWalletEvent) {
                                                       CRYPTO_WALLET_EVENT_TRANSFER_DELETED,
                                                       { .transfer = { cryptoTransferTake (transfer) }}
                                                   });

                // State changed
                BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
                BRCryptoTransferState newState = cryptoTransferStateInit (CRYPTO_TRANSFER_STATE_DELETED);

                cryptoTransferSetState (transfer, newState);

                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     cryptoWalletTake (wallet),
                                                     cryptoTransferTake (transfer),
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CHANGED,
                                                         { .state = { oldState, newState }}
                                                     });

                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     cryptoWalletTake (wallet),
                                                     cryptoTransferTake (transfer),
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_DELETED
                                                     });
            }
            break;
        }
    }

    if (NULL != transfer) {
        cryptoTransferGive (transfer);
    }

    if (NULL != wallet) {
        cryptoWalletGive (wallet);
    }

    cryptoWalletManagerGive (cwm);
}

static void
cwmGetBalanceAsETH (BREthereumClientContext context,
                    BREthereumEWM ewm,
                    BREthereumWallet wid,
                    const char *address,
                    int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_ETH_GET_BALANCE;
    callbackState->u.ethWithWallet.wid = wid;
    callbackState->rid = rid;

    BREthereumNetwork network = ewmGetNetwork (ewm);
    char *networkName = networkCopyNameAsLowercase (network);

    BREthereumToken token = ewmWalletGetToken (ewm, wid);
    if (NULL == token) {
        cwm->client.eth.funcGetEtherBalance (cwm->client.context,
                                             cryptoWalletManagerTake (cwm),
                                             callbackState,
                                             networkName,
                                             address);
    } else {
        cwm->client.eth.funcGetTokenBalance (cwm->client.context,
                                             cryptoWalletManagerTake (cwm),
                                             callbackState,
                                             networkName,
                                             address,
                                             tokenGetAddress (token));
    }

    free (networkName);
    cryptoWalletManagerGive (cwm);
}

static void
cwmGetGasPriceAsETH (BREthereumClientContext context,
                     BREthereumEWM ewm,
                     BREthereumWallet wid,
                     int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_ETH_GET_GAS_PRICE;
    callbackState->u.ethWithWallet.wid = wid;
    callbackState->rid = rid;

    BREthereumNetwork network = ewmGetNetwork (ewm);
    char *networkName = networkCopyNameAsLowercase (network);

    cwm->client.eth.funcGetGasPrice (cwm->client.context,
                                     cryptoWalletManagerTake (cwm),
                                     callbackState,
                                     networkName);

    free (networkName);
    cryptoWalletManagerGive (cwm);
}

static void
cwmGetGasEstimateAsETH (BREthereumClientContext context,
                        BREthereumEWM ewm,
                        BREthereumWallet wid,
                        BREthereumCookie cookie,
                        const char *from,
                        const char *to,
                        const char *amount,
                        const char *price,
                        const char *data,
                        int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_ETH_ESTIMATE_GAS;
    callbackState->u.ethWithWalletAndCookie.wid = wid;
    callbackState->u.ethWithWalletAndCookie.cookie = cookie;
    callbackState->rid = rid;

    BREthereumNetwork network = ewmGetNetwork (ewm);
    char *networkName = networkCopyNameAsLowercase (network);

    cwm->client.eth.funcEstimateGas (cwm->client.context,
                                     cryptoWalletManagerTake (cwm),
                                     callbackState,
                                     networkName,
                                     from,
                                     to,
                                     amount,
                                     price,
                                     data);

    free (networkName);
    cryptoWalletManagerGive (cwm);
}

static void
cwmSubmitTransactionAsETH (BREthereumClientContext context,
                           BREthereumEWM ewm,
                           BREthereumWallet wid,
                           BREthereumTransfer tid,
                           const char *transaction,
                           int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_ETH_SUBMIT_TRANSACTION;
    callbackState->u.ethWithTransaction.wid = wid;
    callbackState->u.ethWithTransaction.tid = tid;
    callbackState->rid = rid;

    BREthereumNetwork network = ewmGetNetwork (ewm);
    char *networkName = networkCopyNameAsLowercase (network);

    cwm->client.eth.funcSubmitTransaction (cwm->client.context,
                                           cryptoWalletManagerTake (cwm),
                                           callbackState,
                                           networkName,
                                           transaction);

    free (networkName);
    cryptoWalletManagerGive (cwm);
}

static void
cwmGetTransactionsAsETH (BREthereumClientContext context,
                         BREthereumEWM ewm,
                         const char *account,
                         uint64_t begBlockNumber,
                         uint64_t endBlockNumber,
                         int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_ETH_GET_TRANSACTIONS;
    callbackState->rid = rid;

    BREthereumNetwork network = ewmGetNetwork (ewm);
    char *networkName = networkCopyNameAsLowercase (network);

    cwm->client.eth.funcGetTransactions (cwm->client.context,
                                         cryptoWalletManagerTake (cwm),
                                         callbackState,
                                         networkName,
                                         account,
                                         begBlockNumber,
                                         endBlockNumber);

    free (networkName);
    cryptoWalletManagerGive (cwm);
}

static void
cwmGetLogsAsETH (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 const char *contract,
                 const char *addressIgnore,
                 const char *event,
                 uint64_t begBlockNumber,
                 uint64_t endBlockNumber,
                 int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_ETH_GET_LOGS;
    callbackState->rid = rid;

    BREthereumNetwork network = ewmGetNetwork (ewm);
    char *networkName = networkCopyNameAsLowercase (network);

    cwm->client.eth.funcGetLogs (cwm->client.context,
                                 cryptoWalletManagerTake (cwm),
                                 callbackState,
                                 networkName,
                                 contract,
                                 addressIgnore,
                                 event,
                                 begBlockNumber,
                                 endBlockNumber);

    free (networkName);
    cryptoWalletManagerGive (cwm);
}

static void
cwmGetBlocksAsETH (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   const char *address,
                   BREthereumSyncInterestSet interests,
                   uint64_t blockNumberStart,
                   uint64_t blockNumberStop,
                   int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_ETH_GET_BLOCKS;
    callbackState->rid = rid;

    BREthereumNetwork network = ewmGetNetwork (ewm);
    char *networkName = networkCopyNameAsLowercase (network);

    cwm->client.eth.funcGetBlocks (cwm->client.context,
                                   cryptoWalletManagerTake (cwm),
                                   callbackState,
                                   networkName,
                                   address,
                                   interests,
                                   blockNumberStart,
                                   blockNumberStop);

    free (networkName);
    cryptoWalletManagerGive (cwm);
}

static void
cwmGetTokensAsETH (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_ETH_GET_TOKENS;
    callbackState->rid = rid;

    cwm->client.eth.funcGetTokens (cwm->client.context,
                                   cryptoWalletManagerTake (cwm),
                                   callbackState);

    cryptoWalletManagerGive (cwm);
}

static void
cwmGetBlockNumberAsETH (BREthereumClientContext context,
                        BREthereumEWM ewm,
                        int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_ETH_GET_BLOCK_NUMBER;
    callbackState->rid = rid;

    BREthereumNetwork network = ewmGetNetwork (ewm);
    char *networkName = networkCopyNameAsLowercase (network);

    cwm->client.eth.funcGetBlockNumber (cwm->client.context,
                                        cryptoWalletManagerTake (cwm),
                                        callbackState,
                                        networkName);

    free (networkName);
    cryptoWalletManagerGive (cwm);
}

static void
cwmGetNonceAsETH (BREthereumClientContext context,
                  BREthereumEWM ewm,
                  const char *address,
                  int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_ETH_GET_NONCE;
    callbackState->rid = rid;

    BREthereumNetwork network = ewmGetNetwork (ewm);
    char *networkName = networkCopyNameAsLowercase (network);

    cwm->client.eth.funcGetNonce (cwm->client.context,
                                  cryptoWalletManagerTake (cwm),
                                  callbackState,
                                  networkName,
                                  address);

    free (networkName);
    cryptoWalletManagerGive (cwm);
}

// MARK: - GEN Callbacks

static void
cwmGetBlockNumberAsGEN (BRGenericClientContext context,
                        BRGenericManager manager,
                        int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_GEN_GET_BLOCK_NUMBER;
    callbackState->rid = rid;

    cwm->client.gen.funcGetBlockNumber (cwm->client.context,
                                        cryptoWalletManagerTake (cwm),
                                        callbackState);

    cryptoWalletManagerGive (cwm);
}

static void
cwmGetTransactionsAsGEN (BRGenericClientContext context,
                         BRGenericManager manager,
                         const char *address,
                         uint64_t begBlockNumber,
                         uint64_t endBlockNumber,
                         int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_GEN_GET_TRANSACTIONS;
    callbackState->rid = rid;

    cwm->client.gen.funcGetTransactions (cwm->client.context,
                                         cryptoWalletManagerTake (cwm),
                                         callbackState,
                                         address,
                                         begBlockNumber, endBlockNumber);

    cryptoWalletManagerGive (cwm);
}

static void
cwmGetTransfersAsGEN (BRGenericClientContext context,
                      BRGenericManager manager,
                      const char *address,
                      uint64_t begBlockNumber,
                      uint64_t endBlockNumber,
                      int rid) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_GEN_GET_TRANSFERS;
    callbackState->rid = rid;

    cwm->client.gen.funcGetTransfers (cwm->client.context,
                                      cryptoWalletManagerTake (cwm),
                                      callbackState,
                                      address,
                                      begBlockNumber, endBlockNumber);

    cryptoWalletManagerGive (cwm);
}

static void
cwmSubmitTransactionAsGEN (BRGenericClientContext context,
                           BRGenericManager manager,
                           BRGenericWallet wallet,
                           BRGenericTransfer transfer,
                           OwnershipKept uint8_t *tx,
                           size_t txLength,
                           BRGenericHash hash,
                           int rid) {
    // Extract CWM, checking to make sure it still lives
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak(context);
    if (NULL == cwm) return;

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION;
    callbackState->u.genWithTransaction.wid = wallet;
    callbackState->u.genWithTransaction.tid = genTransferCopy (transfer);
    callbackState->rid = rid;

    char *hashAsHex = genericHashAsString (hash);

    cwm->client.gen.funcSubmitTransaction (cwm->client.context,
                                           cryptoWalletManagerTake (cwm),
                                           callbackState,
                                           tx, txLength,
                                           hashAsHex);

    free (hashAsHex);

    cryptoWalletManagerGive (cwm);
}

// MARK: - Client Creation Functions

// The below client functions pass a BRCryptoWalletManager reference to the underlying
// currency-specific wallet managers WITHOUT incrementing the reference count. This
// is because if we incremented the count, the CWM's reference count would have no
// way (currently) of being set back to zero as this particular reference would never
// be given.
//
// So, now that we've given a reference without incrementing the count, we have a
// situation where one of these callbacks can occur whilst `cryptoWalletManagerRelease`
// is executing. To handle that issue, each callback uses `cryptoWalletManagerTakeWeak`
// to check if the release is currently happening (i.e. reference count of 0). If so,
// they have an early exit and the release can proceed as usual. If it is not releasing,
// the reference count is incremented for the duration of the call.
//
// The natural question is, can these callbacks occur *after* `cryptoWalletManagerRelease`?
// The answer, thankfully, is NO. The callbacks are called as by A) a thread
// owned by the currency-specific wallet manager, which will be cleaned up gracefully as
// part of `cryptoWalletManagerRelease`; or B) an app thread, which necessitates the CWM
// reference count not being 0. In either case, the BRCryptoWalletManager's memory
// has not yet been freed.
//
// TLDR; use `cryptoWalletManagerTakeWeak` in *ALL* BRWalletManagerClient,
//       BREthereumClient and cryptoWalletManagerClientCreateGENClient callbacks.

extern BRWalletManagerClient
cryptoWalletManagerClientCreateBTCClient (OwnershipKept BRCryptoWalletManager cwm) {
    return (BRWalletManagerClient) {
        cwm,
        cwmGetBlockNumberAsBTC,
        cwmGetTransactionsAsBTC,
        cwmSubmitTransactionAsBTC,
        cwmTransactionEventAsBTC,
        cwmWalletEventAsBTC,
        cwmWalletManagerEventAsBTC
    };
}

extern BREthereumClient
cryptoWalletManagerClientCreateETHClient (OwnershipKept BRCryptoWalletManager cwm) {
    // All these client callbacks are invoked directly on an ETH thread.
    return (BREthereumClient) {
        cwm,
        cwmGetBalanceAsETH,
        cwmGetGasPriceAsETH,
        cwmGetGasEstimateAsETH,
        cwmSubmitTransactionAsETH,
        cwmGetTransactionsAsETH,
        cwmGetLogsAsETH,
        cwmGetBlocksAsETH,
        cwmGetTokensAsETH,
        cwmGetBlockNumberAsETH,
        cwmGetNonceAsETH,

        // Events - Announce changes to entities that normally impact the UI.
        cwmWalletManagerEventAsETH,
        cwmPeerEventAsETH,
        cwmWalletEventAsETH,
        cwmEventTokenAsETH,
        //       BREthereumClientHandlerBlockEvent funcBlockEvent;
        cwmTransactionEventAsETH
    };
}

extern BRGenericClient
cryptoWalletManagerClientCreateGENClient (BRCryptoWalletManager cwm) {
    return (BRGenericClient) {
        cwm,
        cwmGetBlockNumberAsGEN,
        cwmGetTransactionsAsGEN,
        cwmGetTransfersAsGEN,
        cwmSubmitTransactionAsGEN
    };
}

/// MARK: - Announce Functions

extern void
cwmAnnounceGetBlockNumberSuccessAsInteger (OwnershipKept BRCryptoWalletManager cwm,
                                           OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                           uint64_t blockNumber) {
    assert (cwm); assert (callbackState);
    assert (CWM_CALLBACK_TYPE_BTC_GET_BLOCK_NUMBER == callbackState->type ||
            CWM_CALLBACK_TYPE_GEN_GET_BLOCK_NUMBER == callbackState->type);

    cwm = cryptoWalletManagerTake (cwm);

    switch (callbackState->type) {
        case CWM_CALLBACK_TYPE_BTC_GET_BLOCK_NUMBER:
            bwmAnnounceBlockNumber (cwm->u.btc,
                                    callbackState->rid,
                                    blockNumber);
            break;

        case CWM_CALLBACK_TYPE_GEN_GET_BLOCK_NUMBER: {
            genManagerAnnounceBlockNumber (cwm->u.gen,
                                           callbackState->rid,
                                           blockNumber);

            // GEN does not signal events; so we must do it ourselves.
            BRCryptoNetwork network = cryptoWalletManagerGetNetwork(cwm);
            cryptoNetworkSetHeight (network, blockNumber);

            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake(cwm),
                                                      ((BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED,
                { .blockHeight = { blockNumber } }
            }));

            cryptoNetworkGive(network);
            break;
        }

        default:
            break;
    }
    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetBlockNumberSuccessAsString (OwnershipKept BRCryptoWalletManager cwm,
                                          OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                          OwnershipKept const char *blockNumber) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_BLOCK_NUMBER == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceBlockNumber (cwm->u.eth,
                            blockNumber,
                            callbackState->rid);

    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetBlockNumberFailure (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState);
    assert (CWM_CALLBACK_TYPE_BTC_GET_BLOCK_NUMBER == callbackState->type ||
            CWM_CALLBACK_TYPE_ETH_GET_BLOCK_NUMBER == callbackState->type ||
            CWM_CALLBACK_TYPE_GEN_GET_BLOCK_NUMBER == callbackState->type);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetTransactionsItemBTC (OwnershipKept BRCryptoWalletManager cwm,
                                   OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                   BRCryptoTransferStateType status,
                                   OwnershipKept uint8_t *transaction,
                                   size_t transactionLength,
                                   uint64_t timestamp,
                                   uint64_t blockHeight) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_BTC_GET_TRANSACTIONS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    bwmAnnounceTransaction (cwm->u.btc,
                            callbackState->rid,
                            transaction,
                            transactionLength,
                            timestamp,
                            blockHeight);

    cryptoWalletManagerGive (cwm);
    // DON'T free (callbackState);
}

extern void
cwmAnnounceGetTransactionsItemETH (OwnershipKept BRCryptoWalletManager cwm,
                                   OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                   OwnershipKept const char *hash,
                                   OwnershipKept const char *from,
                                   OwnershipKept const char *to,
                                   OwnershipKept const char *contract,
                                   OwnershipKept const char *amount, // value
                                   OwnershipKept const char *gasLimit,
                                   OwnershipKept const char *gasPrice,
                                   OwnershipKept const char *data,
                                   OwnershipKept const char *nonce,
                                   OwnershipKept const char *gasUsed,
                                   OwnershipKept const char *blockNumber,
                                   OwnershipKept const char *blockHash,
                                   OwnershipKept const char *blockConfirmations,
                                   OwnershipKept const char *blockTransactionIndex,
                                   OwnershipKept const char *blockTimestamp,
                                   // cumulative gas used,
                                   // confirmations
                                   // txreceipt_status
                                   OwnershipKept const char *isError) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_TRANSACTIONS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceTransaction (cwm->u.eth,
                            callbackState->rid,
                            hash,
                            from,
                            to,
                            contract,
                            amount,
                            gasLimit,
                            gasPrice,
                            data,
                            nonce,
                            gasUsed,
                            blockNumber,
                            blockHash,
                            blockConfirmations,
                            blockTransactionIndex,
                            blockTimestamp,
                            isError);

    cryptoWalletManagerGive (cwm);
    // DON'T free (callbackState);
}

static BRGenericTransferState
cwmAnnounceGetTransferStateGEN (BRGenericTransfer transfer,
                                BRCryptoTransferStateType status,
                                uint64_t timestamp,
                                uint64_t blockHeight) {
    switch (status) {
        case CRYPTO_TRANSFER_STATE_CREATED:
            return genTransferStateCreateOther (GENERIC_TRANSFER_STATE_CREATED);
        case CRYPTO_TRANSFER_STATE_SIGNED:
            return genTransferStateCreateOther (GENERIC_TRANSFER_STATE_SIGNED);
        case CRYPTO_TRANSFER_STATE_SUBMITTED:
            return genTransferStateCreateOther (GENERIC_TRANSFER_STATE_SUBMITTED);
        case CRYPTO_TRANSFER_STATE_INCLUDED:
            return genTransferStateCreateIncluded (blockHeight,
                                                   GENERIC_TRANSFER_TRANSACTION_INDEX_UNKNOWN,
                                                   timestamp,
                                                   genTransferGetFeeBasis (transfer),
                                                   CRYPTO_TRUE,
                                                   NULL);
        case CRYPTO_TRANSFER_STATE_ERRORED:
            if (BLOCK_HEIGHT_UNBOUND == blockHeight)
                return genTransferStateCreateErrored (GENERIC_TRANSFER_SUBMIT_ERROR_ONE);
            else
                return genTransferStateCreateIncluded (blockHeight,
                                                       GENERIC_TRANSFER_TRANSACTION_INDEX_UNKNOWN,
                                                       timestamp,
                                                       genTransferGetFeeBasis (transfer),
                                                       CRYPTO_FALSE,
                                                       NULL);
        case CRYPTO_TRANSFER_STATE_DELETED:
            return genTransferStateCreateOther (GENERIC_TRANSFER_STATE_DELETED);
    }
}

extern void
cwmAnnounceGetTransactionsItemGEN (BRCryptoWalletManager cwm,
                                   BRCryptoCWMClientCallbackState callbackState,
                                   BRCryptoTransferStateType status,
                                   uint8_t *transaction,
                                   size_t transactionLength,
                                   uint64_t timestamp,
                                   uint64_t blockHeight) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_GEN_GET_TRANSACTIONS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    BRArrayOf(BRGenericTransfer) transfers = genManagerRecoverTransfersFromRawTransaction (cwm->u.gen,
                                                                                           transaction, transactionLength,
                                                                                           timestamp,
                                                                                           blockHeight,
                                                                                           0); // no error, handle below
    // Announce to GWM.  Note: the equivalent BTC+ETH announce transaction is going to
    // create BTC+ETH wallet manager + wallet + transfer events that we'll handle by incorporating
    // the BTC+ETH transfer into 'crypto'.  However, GEN does not generate similar events.

    if (transfers != NULL) {
        pthread_mutex_lock (&cwm->lock);
        for (size_t index = 0; index < array_count (transfers); index++) {
            BRGenericTransfer genTransfer = transfers[index];
            // TODO: A BRGenericTransfer must allow us to determine the Wallet (via a Currency).

            // Update the GEN state based on the `status`
            genTransferSetState (genTransfer, cwmAnnounceGetTransferStateGEN (genTransfer, status, timestamp, blockHeight));

            // Generate required events.
            cryptoWalletManagerHandleTransferGEN (cwm, genTransfer);
        }
        pthread_mutex_unlock (&cwm->lock);

        // The wallet manager takes ownership of the actual transfers - so just
        // delete the array of pointers
        array_free(transfers);
    }

    cryptoWalletManagerGive (cwm);
    // DON'T free (callbackState);
}

extern void
cwmAnnounceGetTransactionsComplete (OwnershipKept BRCryptoWalletManager cwm,
                                    OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                    BRCryptoBoolean success) {
    assert (cwm); assert (callbackState);
    assert (CWM_CALLBACK_TYPE_BTC_GET_TRANSACTIONS == callbackState->type ||
            CWM_CALLBACK_TYPE_ETH_GET_TRANSACTIONS == callbackState->type ||
            CWM_CALLBACK_TYPE_GEN_GET_TRANSACTIONS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    if (CWM_CALLBACK_TYPE_BTC_GET_TRANSACTIONS == callbackState->type && BLOCK_CHAIN_TYPE_BTC == cwm->type) {
        bwmAnnounceTransactionComplete (cwm->u.btc,
                                        callbackState->rid,
                                        success);

    } else if (CWM_CALLBACK_TYPE_ETH_GET_TRANSACTIONS == callbackState->type && BLOCK_CHAIN_TYPE_ETH == cwm->type) {
        ewmAnnounceTransactionComplete (cwm->u.eth,
                                        callbackState->rid,
                                        AS_ETHEREUM_BOOLEAN (success));

    } else if (CWM_CALLBACK_TYPE_GEN_GET_TRANSACTIONS == callbackState->type && BLOCK_CHAIN_TYPE_GEN== cwm->type) {
        genManagerAnnounceTransferComplete (cwm->u.gen,
                                            callbackState->rid,
                                            CRYPTO_TRUE == success);
    } else {
        assert (0);
    }

    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetTransferItemGEN (BRCryptoWalletManager cwm,
                               BRCryptoCWMClientCallbackState callbackState,
                               BRCryptoTransferStateType status,
                               OwnershipKept const char *hash,
                               OwnershipKept const char *uids,
                               OwnershipKept const char *from,
                               OwnershipKept const char *to,
                               OwnershipKept const char *amount,
                               OwnershipKept const char *currency,
                               OwnershipKept const char *fee,
                               uint64_t timestamp,
                               uint64_t blockHeight,
                               size_t attributesCount,
                               OwnershipKept const char **attributeKeys,
                               OwnershipKept const char **attributeVals) {
    assert (cwm); assert (callbackState);
    assert (CWM_CALLBACK_TYPE_GEN_GET_TRANSFERS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    // Lookup the network's currency
    BRCryptoNetwork network = cryptoWalletManagerGetNetwork (cwm);
    BRCryptoCurrency walletCurrency = cryptoNetworkGetCurrencyForUids (network, currency);

    // Find the corresponding wallet.
    BRCryptoWallet wallet = (NULL == walletCurrency
                             ? NULL
                             : cryptoWalletManagerGetWalletForCurrency (cwm, walletCurrency));

    // If we have a wallet, then proceed
    if (NULL != wallet) {
        // Create a 'GEN' transfer
        BRGenericWallet   genWallet   = cryptoWalletAsGEN(wallet);
        BRGenericTransfer genTransfer = genManagerRecoverTransfer (cwm->u.gen, genWallet, hash, uids,
                                                                   from, to,
                                                                   amount, currency, fee,
                                                                   timestamp, blockHeight,
                                                                   CRYPTO_TRANSFER_STATE_ERRORED == status);

        genTransferSetState (genTransfer, cwmAnnounceGetTransferStateGEN (genTransfer, status, timestamp, blockHeight));

        // If we are passed in attribues, they will replace any attribute already held
        // in `genTransfer`.  Specifically, for example, if we created an XRP transfer, then
        // we might have a 'DestinationTag'.  If the attributes provided do not include
        // 'DestinatinTag' then that attribute will be lost.  Losing such an attribute would
        // indicate a BlockSet error in processing transfers.
        if (attributesCount > 0) {
            BRGenericAddress genTarget = genTransferGetTargetAddress (genTransfer);

            // Build the transfer attributes
            BRArrayOf(BRGenericTransferAttribute) genAttributes;
            array_new(genAttributes, attributesCount);
            for (size_t index = 0; index < attributesCount; index++) {
                const char *keyFound;
                BRCryptoBoolean isRequiredAttribute;
                BRCryptoBoolean isAttribute = genWalletHasTransferAttributeForKey (genWallet,
                                                                                   genTarget,
                                                                                   attributeKeys[index],
                                                                                   &keyFound,
                                                                                   &isRequiredAttribute);
                if (CRYPTO_TRUE == isAttribute)
                    array_add (genAttributes,
                               genTransferAttributeCreate (keyFound,
                                                           attributeVals[index],
                                                           CRYPTO_TRUE == isRequiredAttribute));
            }
            genTransferSetAttributes(genTransfer, genAttributes);
            genTransferAttributeReleaseAll(genAttributes);
            genAddressRelease(genTarget);
        }

        // Announce to GWM.  Note: the equivalent BTC+ETH announce transaction is going to
        // create BTC+ETH wallet manager + wallet + transfer events that we'll handle by
        // incorporating the BTC+ETH transfer into 'crypto'.  However, GEN does not generate
        // similar events.
        //
        // genManagerAnnounceTransfer (cwm->u.gen, callbackState->rid, transfer);
        cryptoWalletManagerHandleTransferGEN (cwm, genTransfer);
    }

    if (NULL != wallet) cryptoWalletGive (wallet);
    if (NULL != walletCurrency) cryptoCurrencyGive (walletCurrency);

    cryptoNetworkGive (network);
    cryptoWalletManagerGive (cwm);
    // DON'T free (callbackState);
}

extern void
cwmAnnounceGetTransfersComplete (OwnershipKept BRCryptoWalletManager cwm,
                                 OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                 BRCryptoBoolean success) {
    assert (cwm); assert (callbackState);
    assert (CWM_CALLBACK_TYPE_GEN_GET_TRANSFERS == callbackState->type);

    cwm = cryptoWalletManagerTake (cwm);

    if (CWM_CALLBACK_TYPE_GEN_GET_TRANSFERS == callbackState->type && BLOCK_CHAIN_TYPE_GEN== cwm->type) {
        genManagerAnnounceTransferComplete (cwm->u.gen, callbackState->rid, CRYPTO_TRUE == success);
    } else {
        assert (0);
    }

    // TODO: This even occurs even when the balance doesn't change (no new transfers).

    // Synchronizing of transfers is complete - calculate the new balance
    BRCryptoAmount balance = cryptoWalletGetBalance(cwm->wallet);
    // ... and announce the balance
    cwm->listener.walletEventCallback (cwm->listener.context,
                                       cryptoWalletManagerTake (cwm),
                                       cryptoWalletTake (cwm->wallet),
                                       (BRCryptoWalletEvent) {
                                           CRYPTO_WALLET_EVENT_BALANCE_UPDATED,
                                           { .balanceUpdated = { cryptoAmountTake(balance) }}
                                       });

    cryptoAmountGive(balance);
    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

static void
cwmAnnounceSubmitTransferResultGEN (OwnershipKept BRCryptoWalletManager cwm,
                                    OwnershipKept BRCryptoCWMClientCallbackState callbackState,
                                    int error) {
    assert (cwm); assert(callbackState);
    assert (CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION == callbackState->type);
    // Assume cwm taken already;

    genManagerAnnounceSubmit (cwm->u.gen,
                              callbackState->rid,
                              callbackState->u.genWithTransaction.tid,
                              error);

    BRCryptoWallet   wallet   = cryptoWalletManagerFindWalletAsGEN (cwm, callbackState->u.genWithTransaction.wid);
    BRCryptoTransfer transfer = (NULL == wallet ? NULL : cryptoWalletFindTransferAsGEN (wallet, callbackState->u.genWithTransaction.tid));

    // TODO: Assert on these?
    if (NULL != wallet && NULL != transfer) {
        cryptoWalletManagerSetTransferStateGEN (cwm,
                                                wallet,
                                                transfer,
                                                (error
                                                 ? genTransferStateCreateErrored (GENERIC_TRANSFER_SUBMIT_ERROR_ONE)
                                                 : genTransferStateCreateOther (GENERIC_TRANSFER_STATE_SUBMITTED)));
    }

    if (NULL != wallet)   cryptoWalletGive(wallet);
    if (NULL != transfer) cryptoTransferGive(transfer);
    // callbackState->u.genWithTransaction.tid is untouched; still owned by `callbackState`
}

extern void
cwmAnnounceSubmitTransferSuccess (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState);
    assert (CWM_CALLBACK_TYPE_BTC_SUBMIT_TRANSACTION == callbackState->type ||
            CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    if (CWM_CALLBACK_TYPE_BTC_SUBMIT_TRANSACTION == callbackState->type && BLOCK_CHAIN_TYPE_BTC == cwm->type) {
        bwmAnnounceSubmit (cwm->u.btc,
                           callbackState->rid,
                           callbackState->u.btcSubmit.txHash,
                           0);
    }

    else if (CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION == callbackState->type && BLOCK_CHAIN_TYPE_GEN == cwm->type) {
        cwmAnnounceSubmitTransferResultGEN (cwm, callbackState, 0);
    }

    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceSubmitTransferSuccessForHash (OwnershipKept BRCryptoWalletManager cwm,
                                         OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                         OwnershipKept const char *hash) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_SUBMIT_TRANSACTION == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceSubmitTransfer (cwm->u.eth,
                               callbackState->u.ethWithTransaction.wid,
                               callbackState->u.ethWithTransaction.tid,
                               hash,
                               -1,
                               NULL,
                               callbackState->rid);

    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceSubmitTransferFailure (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState);
    assert (CWM_CALLBACK_TYPE_BTC_SUBMIT_TRANSACTION == callbackState->type ||
            CWM_CALLBACK_TYPE_ETH_SUBMIT_TRANSACTION == callbackState->type ||
            CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    // TODO(fix): For BTC/GEN, we pass EIO as the posix error. For ETH, 0 and a made up message.
    //            We should receive error information (ideally not posix codes) from the platform layer

    if (CWM_CALLBACK_TYPE_BTC_SUBMIT_TRANSACTION == callbackState->type && BLOCK_CHAIN_TYPE_BTC == cwm->type) {
        bwmAnnounceSubmit (cwm->u.btc,
                           callbackState->rid,
                           callbackState->u.btcSubmit.txHash,
                           EIO);

    } else if (CWM_CALLBACK_TYPE_ETH_SUBMIT_TRANSACTION == callbackState->type && BLOCK_CHAIN_TYPE_ETH == cwm->type) {
        ewmAnnounceSubmitTransfer (cwm->u.eth,
                                   callbackState->u.ethWithTransaction.wid,
                                   callbackState->u.ethWithTransaction.tid,
                                   NULL,
                                   0,
                                   "unknown failure",
                                   callbackState->rid);

    } else if (CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION == callbackState->type && BLOCK_CHAIN_TYPE_GEN == cwm->type) {
        cwmAnnounceSubmitTransferResultGEN (cwm, callbackState, EIO);
    } else {
        assert (0);
    }

    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetBalanceSuccess (OwnershipKept BRCryptoWalletManager cwm,
                              OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                              OwnershipKept const char *balance) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_BALANCE == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceWalletBalance (cwm->u.eth,
                              callbackState->u.ethWithWallet.wid,
                              balance,
                              callbackState->rid);

    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetBalanceFailure (OwnershipKept BRCryptoWalletManager cwm,
                              OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_BALANCE == callbackState->type);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetGasPriceSuccess (OwnershipKept BRCryptoWalletManager cwm,
                               OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                               OwnershipKept const char *gasPrice) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_GAS_PRICE == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceGasPrice (cwm->u.eth,
                         callbackState->u.ethWithWallet.wid,
                         gasPrice,
                         callbackState->rid);

    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetGasPriceFailure (OwnershipKept BRCryptoWalletManager cwm,
                               OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_GAS_PRICE == callbackState->type);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetGasEstimateSuccess (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                  OwnershipKept const char *gasEstimate,
                                  OwnershipKept const char *gasPrice) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_ESTIMATE_GAS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceGasEstimateSuccess (cwm->u.eth,
                                   callbackState->u.ethWithWalletAndCookie.wid,
                                   callbackState->u.ethWithWalletAndCookie.cookie,
                                   gasEstimate,
                                   gasPrice,
                                   callbackState->rid);

    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetGasEstimateFailure (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                  BRCryptoStatus status) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_ESTIMATE_GAS == callbackState->type);

    ewmAnnounceGasEstimateFailure (cwm->u.eth,
                                   callbackState->u.ethWithWalletAndCookie.wid,
                                   callbackState->u.ethWithWalletAndCookie.cookie,
                                   cryptoStatusAsETH (status),
                                   callbackState->rid);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetLogsItem(OwnershipKept BRCryptoWalletManager cwm,
                       OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                       OwnershipKept const char *strHash,
                       OwnershipKept const char *strContract,
                       int topicCount,
                       OwnershipKept const char **arrayTopics,
                       OwnershipKept const char *strData,
                       OwnershipKept const char *strGasPrice,
                       OwnershipKept const char *strGasUsed,
                       OwnershipKept const char *strLogIndex,
                       OwnershipKept const char *strBlockNumber,
                       OwnershipKept const char *strBlockTransactionIndex,
                       OwnershipKept const char *strBlockTimestamp) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_LOGS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceLog (cwm->u.eth,
                    callbackState->rid,
                    strHash,
                    strContract,
                    topicCount,
                    arrayTopics,
                    strData,
                    strGasPrice,
                    strGasUsed,
                    strLogIndex,
                    strBlockNumber,
                    strBlockTransactionIndex,
                    strBlockTimestamp);

    cryptoWalletManagerGive (cwm);
    // don't free (callbackState);
}

extern void
cwmAnnounceGetLogsComplete(OwnershipKept BRCryptoWalletManager cwm,
                           OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                           BRCryptoBoolean success) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_LOGS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceLogComplete (cwm->u.eth,
                            callbackState->rid,
                            AS_ETHEREUM_BOOLEAN (success));

    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetBlocksSuccess (OwnershipKept BRCryptoWalletManager cwm,
                             OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                             int blockNumbersCount,
                             OwnershipKept uint64_t *blockNumbers) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_BLOCKS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceBlocks (cwm->u.eth,
                       callbackState->rid,
                       blockNumbersCount,
                       blockNumbers);

    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetBlocksFailure (OwnershipKept BRCryptoWalletManager cwm,
                             OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_BLOCKS == callbackState->type);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetTokensItem(OwnershipKept BRCryptoWalletManager cwm,
                         OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                         OwnershipKept const char *address,
                         OwnershipKept const char *symbol,
                         OwnershipKept const char *name,
                         OwnershipKept const char *description,
                         unsigned int decimals,
                         OwnershipKept const char *strDefaultGasLimit,
                         OwnershipKept const char *strDefaultGasPrice) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_TOKENS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceToken (cwm->u.eth,
                      callbackState->rid,
                      address,
                      symbol,
                      name,
                      description,
                      decimals,
                      strDefaultGasLimit,
                      strDefaultGasPrice);

    cryptoWalletManagerGive (cwm);
    // don't free (callbackState);
}

extern void
cwmAnnounceGetTokensComplete(OwnershipKept BRCryptoWalletManager cwm,
                             OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                             BRCryptoBoolean success) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_TOKENS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceTokenComplete (cwm->u.eth,
                              callbackState->rid,
                              AS_ETHEREUM_BOOLEAN (success));

    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetNonceSuccess (OwnershipKept BRCryptoWalletManager cwm,
                            OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                            OwnershipKept const char *address,
                            OwnershipKept const char *nonce) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_NONCE == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceNonce (cwm->u.eth,
                      address,
                      nonce,
                      callbackState->rid);

    cryptoWalletManagerGive (cwm);
    cwmClientCallbackStateRelease (callbackState);
}

extern void
cwmAnnounceGetNonceFailure (OwnershipKept BRCryptoWalletManager cwm,
                            OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_NONCE == callbackState->type);
    cwmClientCallbackStateRelease (callbackState);
}
