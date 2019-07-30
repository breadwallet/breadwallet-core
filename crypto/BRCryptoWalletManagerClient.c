//
//  BRCryptoWalletManagerClient.c
//  BRCrypto
//
//  Created by Michael Carrara on 6/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
#include "BRCryptoBase.h"
#include "BRCryptoPrivate.h"
#include "BRCryptoWalletManager.h"
#include "BRCryptoWalletManagerClient.h"
#include "BRCryptoWalletManagerPrivate.h"

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
    CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION,

} BRCryptoCWMCallbackType;

struct BRCryptoCWMClientCallbackStateRecord {
    BRCryptoCWMCallbackType type;
    union {
        struct {
            BRTransaction *transaction;
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
            BRGenericTransfer tid;
        } genWithTransaction;
    } u;
    int rid;
};

/// MARK: - BTC Callbacks

static void
cwmGetBlockNumberAsBTC (BRWalletManagerClientContext context,
                        BRWalletManager manager,
                        int rid) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
                         const char **addresses,
                         size_t addressCount,
                         uint64_t begBlockNumber,
                         uint64_t endBlockNumber,
                         int rid) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
                           OwnershipGiven BRTransaction *transaction,
                           int rid) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_BTC_SUBMIT_TRANSACTION;
    callbackState->u.btcSubmit.transaction = transaction;
    callbackState->rid = rid;

    UInt256 txHash = UInt256Reverse (transaction->txHash);
    char *hashAsHex = encodeHexCreate (NULL, txHash.u8, sizeof(txHash.u8));

    size_t txLength = BRTransactionSerialize (transaction, NULL, 0);
    uint8_t *tx = calloc (txLength, sizeof(uint8_t));
    BRTransactionSerialize(transaction, tx, txLength);

    cwm->client.btc.funcSubmitTransaction (cwm->client.context,
                                           cryptoWalletManagerTake (cwm),
                                           callbackState,
                                           tx,
                                           txLength,
                                           hashAsHex);

    free (tx);
    free (hashAsHex);
    cryptoWalletManagerGive (cwm);
}

static void
cwmWalletManagerEventAsBTC (BRWalletManagerClientContext context,
                            OwnershipKept BRWalletManager btcManager,
                            BRWalletManagerEvent event) {
    // Extract CWM and avoid a race condition by ensuring cwm->u.btc
    BRCryptoWalletManager cwm = context;
    if (NULL == cwm->u.btc) cwm->u.btc = btcManager;

    assert (BLOCK_CHAIN_TYPE_BTC == cwm->type);

    int needEvent = 1;
    BRCryptoWalletManagerEvent cwmEvent = { CRYPTO_WALLET_MANAGER_EVENT_CREATED };

    switch (event.type) {
        case BITCOIN_WALLET_MANAGER_CREATED:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CREATED
            };
            break;

        case BITCOIN_WALLET_MANAGER_CONNECTED:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = { cwm->state, CRYPTO_WALLET_MANAGER_STATE_CONNECTED }}
            };
            cryptoWalletManagerSetState (cwm, CRYPTO_WALLET_MANAGER_STATE_CONNECTED);
            break;

        case BITCOIN_WALLET_MANAGER_DISCONNECTED:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = { cwm->state, CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED }}
            };
            cryptoWalletManagerSetState (cwm, CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED);
            break;

        case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED
            };
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      cwmEvent);

            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = { cwm->state, CRYPTO_WALLET_MANAGER_STATE_SYNCING }}
            };
            cryptoWalletManagerSetState (cwm, CRYPTO_WALLET_MANAGER_STATE_SYNCING);
           break;

        case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED
            };
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      cwmEvent);

            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = { cwm->state, CRYPTO_WALLET_MANAGER_STATE_CONNECTED }}
            };
            cryptoWalletManagerSetState (cwm, CRYPTO_WALLET_MANAGER_STATE_CONNECTED);
            break;

        case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED,
                { .blockHeight = { event.u.blockHeightUpdated.value }}
            };
            BRCryptoNetwork network = cryptoWalletManagerGetNetwork (cwm);
            cryptoNetworkSetHeight (network, event.u.blockHeightUpdated.value);
            cryptoNetworkGive (network);
            break;
    }

    if (needEvent)
        cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                  cryptoWalletManagerTake (cwm),
                                                  cwmEvent);
}

static void
cwmWalletEventAsBTC (BRWalletManagerClientContext context,
                     OwnershipKept BRWalletManager btcManager,
                     OwnershipKept BRWallet *btcWallet,
                     BRWalletEvent event) {
    // Extract CWM and avoid a race condition by ensuring cwm->u.btc
    BRCryptoWalletManager cwm = context;
    if (NULL == cwm->u.btc) cwm->u.btc = btcManager;

    assert (BLOCK_CHAIN_TYPE_BTC == cwm->type);

    switch (event.type) {
        case BITCOIN_WALLET_CREATED: {
            // Get `currency` (it is 'taken')
            BRCryptoCurrency currency = cryptoNetworkGetCurrency (cwm->network);

            // We'll create a wallet using the currency's default unit.
            BRCryptoUnit unit = cryptoNetworkGetUnitAsDefault (cwm->network, currency);

            // Demand no 'wallet'
            assert (NULL == cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet));
            BRCryptoWallet wallet = cryptoWalletCreateAsBTC (unit, unit, cwm->u.btc, btcWallet);

            // Avoid a race condition on the CWM's 'primaryWallet'
            if (NULL == cwm->wallet) cwm->wallet = cryptoWalletTake (wallet);

            // Update CWM with the new wallet.
            cryptoWalletManagerAddWallet (cwm, wallet);

            // Generate a CRYPTO wallet event for CREATED...
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (wallet),
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_CREATED
                                               });

            // ... and then a CRYPTO wallet manager event for WALLET_ADDED
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                                                          CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                          { .wallet = { cryptoWalletTake (wallet) }}
                                                      });

            cryptoWalletGive (wallet);
            cryptoUnitGive (unit);
            cryptoCurrencyGive (currency);
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

        case BITCOIN_WALLET_TRANSACTION_SUBMITTED: {
            // Demand 'wallet'
            BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet);
            assert (NULL != wallet);

            // Find the wallet's transfer for 'btc'. (it is 'taken'); t must exist already in wallet (otherwise how could it have been submitted?)
            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, event.u.submitted.transaction);
            assert (NULL != transfer);

            BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
            assert (CRYPTO_TRANSFER_STATE_SUBMITTED != oldState.type);

            if (event.u.submitted.error == 0) {
                // Set the transfer state to submitted
                BRCryptoTransferState newState = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_SUBMITTED };

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
                // Set the transfer state to errorred
                BRCryptoTransferState newState = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_ERRORRED };
                snprintf(newState.u.errorred.message, 128, "Error: %d", event.u.submitted.error);

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

            cryptoTransferGive (transfer);
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
                                                   { .feeBasisEstimated = { event.u.feeEstimated.cookie, cryptoFeeBasisTake(feeBasis) }}
                                               });

            cryptoFeeBasisGive (feeBasis);
            cryptoUnitGive (feeUnit);
            cryptoWalletGive (wallet);
            break;
        }
    }
}

static void
cwmTransactionEventAsBTC (BRWalletManagerClientContext context,
                          OwnershipKept BRWalletManager btcManager,
                          OwnershipKept BRWallet *btcWallet,
                          OwnershipKept BRTransaction *btcTransaction,
                          BRTransactionEvent event) {
    // Extract CWM and avoid a race condition by ensuring cwm->u.btc
    BRCryptoWalletManager cwm = context;
    if (NULL == cwm->u.btc) cwm->u.btc = btcManager;

    assert (BLOCK_CHAIN_TYPE_BTC == cwm->type);

    // Find 'wallet' based on BTC... (it is taken)
    BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet);

    // ... and demand 'wallet'
    assert (NULL != wallet && btcWallet == cryptoWalletAsBTC (wallet));

    switch (event.type) {

        case BITCOIN_TRANSACTION_CREATED: {
            BRCryptoUnit unit         = cryptoWalletGetUnit (wallet);
            BRCryptoUnit unitForFee = cryptoWalletGetUnitForFee(wallet);

            // This event occurs for a user created transaction. We create the
            // cryptoTransfer using the btcTransaction, rather than a copy. That is
            // because at this point, the transaction is not signed. As a result, we
            // can't do an equality check on a copy, as the txHash is all zeroes. So,
            // create a cryptoTransfer using the original btcTransaction with the
            // understanding that the cryptoWalletCreateTransfer() that led to this
            // callback being triggered (on the same thread), will not call
            // cryptoTransferCreateAsBTC but rather use cryptoWalletFindTransferAsBTC.

            assert (!BRTransactionIsSigned (btcTransaction));

            // The transfer finally - based on the wallet's currency (BTC)
            BRCryptoTransfer transfer = cryptoTransferCreateAsBTC (unit,
                                                                   unitForFee,
                                                                   cryptoWalletAsBTC (wallet),
                                                                   btcTransaction);

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
            // This event occurs for a user created transaction. In that case, we must be wrapping
            // the exact transfer that was signed (i.e. not a copy).

            assert (BRTransactionIsSigned (btcTransaction));

            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, btcTransaction); // taken
            assert (NULL != transfer);
            assert (btcTransaction == cryptoTransferAsBTC (transfer));

            BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
            assert (CRYPTO_TRANSFER_STATE_SIGNED != oldState.type);

            BRCryptoTransferState newState = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_SIGNED };
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
            // or if the transaction arrived during a sync. If it came from a sync, transfer will
            // be NULL, so we must create a wrapping cryptoTransfer using a copy of the underlying
            // BTC transaction that is owned by the BRWalletManager.If this is a user-generated transfer,
            // we are already have a copy of the underlying btcTransaction because we made a copy on
            // submission.

            assert (BRTransactionIsSigned (btcTransaction));

            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, btcTransaction); // taken
            if (NULL == transfer) {
                BRCryptoUnit unit         = cryptoWalletGetUnit (wallet);
                BRCryptoUnit unitForFee = cryptoWalletGetUnitForFee(wallet);

                // The transfer finally - based on the wallet's currency (BTC)
                transfer = cryptoTransferCreateAsBTC (unit,
                                                      unitForFee,
                                                      cryptoWalletAsBTC (wallet),
                                                      BRTransactionCopy (btcTransaction));

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

            // ... update state to reflect included if the timestamp and block height are already set
            if (0 != btcTransaction->timestamp && TX_UNCONFIRMED != btcTransaction->blockHeight) {
                BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
                assert (CRYPTO_TRANSFER_STATE_INCLUDED != oldState.type);

                BRCryptoTransferState newState = {
                    CRYPTO_TRANSFER_STATE_INCLUDED,
                    { .included = {
                        btcTransaction->blockHeight,
                        0,
                        btcTransaction->timestamp,
                        NULL
                    }}
                };

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

            cryptoTransferGive (transfer);
            break;
        }

        case BITCOIN_TRANSACTION_UPDATED: {
            // This event occurs when the timestamp and/or blockHeight have been changed
            // due to the transaction being confirmed or unconfirmed (in the case of a blockchain
            // reorg). Regardless of if this was a user created transaction or if it came from a
            // sync, we are holding a copy of the transaction that BRWalletManager has.
            // As a result, we need to update our copy of the transaction.

            assert (BRTransactionIsSigned (btcTransaction));

            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, btcTransaction); // taken
            assert (NULL != transfer);
            assert (btcTransaction != cryptoTransferAsBTC (transfer));

            BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
            BRCryptoTransferState newState = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_CREATED };

            BRTransaction *ourTransaction = cryptoTransferAsBTC (transfer);
            uint32_t timestamp = ourTransaction->timestamp;
            uint32_t blockHeight = ourTransaction->blockHeight;

            uint32_t newTimestamp = btcTransaction->timestamp;
            uint32_t newBlockHeight = btcTransaction->blockHeight;

            ourTransaction->timestamp = newTimestamp;
            ourTransaction->blockHeight = newBlockHeight;

            int changed = (timestamp == newTimestamp && blockHeight == newBlockHeight) ? 0 : 1;
            if (!changed) {
                // nothing to do, state hasn't changed
            } else if (0 == newTimestamp || TX_UNCONFIRMED == newBlockHeight) {
                // TODO(discuss): Should there be a pending state?
                // newState already initialized to CRYPTO_TRANSFER_STATE_CREATED
            } else {
                newState = (BRCryptoTransferState) {
                    CRYPTO_TRANSFER_STATE_INCLUDED,
                    { .included = {
                        newBlockHeight,
                        0,
                        newTimestamp,
                        NULL
                    }}
                };
            }

            if (changed) {
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

            cryptoTransferGive (transfer);
            break;
        }

        case BITCOIN_TRANSACTION_DELETED: {
            // This event occurs when a transaction has been deleted from a wallet.
            // Regardless of if this was a user created transaction or if it came from a
            // sync, we are holding a copy of the transaction that BRWalletManager has. Find it
            // and remove it from the wallet.

            assert (BRTransactionIsSigned (btcTransaction));

            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, btcTransaction); // taken
            assert (NULL != transfer);
            assert (btcTransaction != cryptoTransferAsBTC (transfer));

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
}

/// MARK: ETH Callbacks

static void
cwmWalletManagerEventAsETH (BREthereumClientContext context,
                            BREthereumEWM ewm,
                            BREthereumEWMEvent event,
                            BREthereumStatus status,
                            const char *errorDescription) {
    BRCryptoWalletManager cwm = context;
    if (NULL == cwm->u.eth) cwm->u.eth = ewm;

    int needEvent = 1;
    BRCryptoWalletManagerEvent cwmEvent;

    switch (event) {
        case EWM_EVENT_CREATED:
            needEvent = 0;
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CREATED
            };
            break;

        case EWM_EVENT_SYNC_STARTED:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED
            };
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      cwmEvent);

            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = { cwm->state, CRYPTO_WALLET_MANAGER_STATE_SYNCING }}
            };
            cryptoWalletManagerSetState (cwm, CRYPTO_WALLET_MANAGER_STATE_SYNCING);
            break;

        case EWM_EVENT_SYNC_CONTINUES:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES,
                { .sync = { 50 }}
            };
            break;

        case EWM_EVENT_SYNC_STOPPED:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED
            };
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      cwmEvent);

            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = { cwm->state, CRYPTO_WALLET_MANAGER_STATE_CONNECTED }}
            };
            cryptoWalletManagerSetState (cwm, CRYPTO_WALLET_MANAGER_STATE_CONNECTED);
            break;

        case EWM_EVENT_NETWORK_UNAVAILABLE:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                { .state = { cwm->state, CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED }}
            };
            cryptoWalletManagerSetState (cwm, CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED);
            break;

        case EWM_EVENT_BLOCK_HEIGHT_UPDATED: {
            uint64_t blockHeight = ewmGetBlockHeight(ewm);
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED,
                { .blockHeight = { blockHeight }}
            };
            BRCryptoNetwork network = cryptoWalletManagerGetNetwork (cwm);
            cryptoNetworkSetHeight (network, blockHeight);
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
}

static void
cwmPeerEventAsETH (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   //BREthereumWallet wid,
                   //BREthereumTransactionId tid,
                   BREthereumPeerEvent event,
                   BREthereumStatus status,
                   const char *errorDescription) {
    BRCryptoWalletManager cwm = context;
    (void) cwm;
}

static void
cwmWalletEventAsETH (BREthereumClientContext context,
                     BREthereumEWM ewm,
                     BREthereumWallet wid,
                     BREthereumWalletEvent event,
                     BREthereumStatus status,
                     const char *errorDescription) {
    BRCryptoWalletManager cwm = context;
    if (NULL == cwm->u.eth) cwm->u.eth = ewm;

    BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsETH (cwm, wid); // taken

    // TODO: crypto{Wallet,Transfer}Give()

    switch (event.type) {
        case WALLET_EVENT_CREATED:
            if (NULL == wallet) {
                BREthereumToken token = ewmWalletGetToken (ewm, wid);

                // Find the wallet's currency.
                BRCryptoCurrency currency = (NULL == token
                                             ? cryptoNetworkGetCurrency (cwm->network)
                                             : cryptoNetworkGetCurrencyforTokenETH (cwm->network, token));

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
                BRCryptoUnit     unit     = cryptoNetworkGetUnitAsDefault (cwm->network, currency);
                assert (NULL != unit);

                // Create the appropriate wallet based on currency
                wallet = cryptoWalletCreateAsETH (unit, unit, cwm->u.eth, wid); // taken

                // Avoid a race on cwm->wallet - but be sure to assign the ETH wallet (not a TOK one).
                if (NULL == cwm->wallet && NULL == token) cwm->wallet = cryptoWalletTake (wallet);

                cryptoWalletManagerAddWallet (cwm, wallet);

                cryptoUnitGive (unit);
                cryptoCurrencyGive (currency);

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

        case WALLET_EVENT_BALANCE_UPDATED: {
            if (NULL != wallet) {
                BRCryptoCurrency currency = cryptoNetworkGetCurrency(cwm->network);
                BRCryptoUnit unit = cryptoNetworkGetUnitAsBase(cwm->network, currency);

                // Get the wallet's amount...
                BREthereumAmount amount = ewmWalletGetBalance(cwm->u.eth, wid);

                // ... and then the 'raw integer' (UInt256) value
                UInt256 value = (AMOUNT_ETHER == amountGetType(amount)
                                 ? amountGetEther(amount).valueInWEI
                                 : amountGetTokenQuantity(amount).valueAsInteger);

                // Use currency to create a cyrptoAmount in the base unit (implicitly).
                BRCryptoAmount cryptoAmount = cryptoAmountCreate(currency, CRYPTO_FALSE, value);

                cryptoUnitGive(unit);
                cryptoCurrencyGive(currency);

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
        case WALLET_EVENT_FEE_ESTIMATED:
            if (NULL != wallet) {
                if (SUCCESS == status) {
                    BRCryptoUnit feeUnit = cryptoWalletGetUnitForFee(wallet);

                    BRCryptoFeeBasis feeBasis = cryptoFeeBasisCreateAsETH (feeUnit, event.u.feeEstimate.gasEstimate, event.u.feeEstimate.gasPrice);

                    cwm->listener.walletEventCallback(cwm->listener.context,
                                                      cryptoWalletManagerTake(cwm),
                                                      wallet,
                                                      (BRCryptoWalletEvent) {
                                                       CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED,
                                                       { .feeBasisEstimated = { event.u.feeEstimate.cookie, feeBasis }}
                                                   });

                    cryptoUnitGive (feeUnit);
                } else {
                    // TODO(fix): How do we want to handle failure
                }
            }
    }
}

static void
cwmEventTokenAsETH (BREthereumClientContext context,
                    BREthereumEWM ewm,
                    BREthereumToken token,
                    BREthereumTokenEvent event) {

    BRCryptoWalletManager cwm = context;

    switch (event) {
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
}


static void
cwmTransactionEventAsETH (BREthereumClientContext context,
                          BREthereumEWM ewm,
                          BREthereumWallet wid,
                          BREthereumTransfer tid,
                          BREthereumTransferEvent event,
                          BREthereumStatus status,
                          const char *errorDescription) {
    BRCryptoWalletManager cwm = context;
    BRCryptoWallet wallet     = cryptoWalletManagerFindWalletAsETH (cwm, wid); // taken

    // TODO: Wallet may be NULL for a sync-discovered transfer w/o a currency.
    if (NULL == wallet) return;

    BRCryptoTransfer transfer = cryptoWalletFindTransferAsETH (wallet, tid);   // taken

    assert (NULL != transfer || TRANSFER_EVENT_CREATED == event);

    // We'll transition from `oldState` to `newState`; get some placeholder values in place.
    BRCryptoTransferState oldState = { CRYPTO_TRANSFER_STATE_CREATED };
    BRCryptoTransferState newState = { CRYPTO_TRANSFER_STATE_CREATED };
    if (NULL != transfer) oldState = cryptoTransferGetState (transfer);

    switch (event) {
        case TRANSFER_EVENT_CREATED: {
            assert (NULL == transfer);

            BRCryptoUnit unit         = cryptoWalletGetUnit (wallet);
            BRCryptoUnit unitForFee = cryptoWalletGetUnitForFee(wallet);

            // if (NULL != transfer) cryptoTransferGive (transfer);

            transfer = cryptoTransferCreateAsETH (unit,
                                                  unitForFee,
                                                  cwm->u.eth,
                                                  tid); // taken

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
                                               wallet,
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_TRANSFER_ADDED,
                                                   { .transfer = { transfer }}
                                               });

            cryptoUnitGive (unitForFee);
            cryptoUnitGive (unit);
            break;
        }

        case TRANSFER_EVENT_SIGNED:
            if (NULL != transfer) {
                newState = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_SIGNED };

                cryptoTransferSetState (transfer, newState);
                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     wallet,
                                                     transfer,
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CHANGED,
                                                         { .state = { oldState, newState }}
                                                     });
            }
            break;

        case TRANSFER_EVENT_SUBMITTED:
            if (NULL != transfer) {
                newState = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_SUBMITTED };
                cryptoTransferSetState (transfer, newState);
                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     wallet,
                                                     transfer,
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CHANGED,
                                                         { .state = { oldState, newState }}
                                                     });
            }
            break;

        case TRANSFER_EVENT_INCLUDED:
            if (NULL != transfer ){
                uint64_t blockNumber, blockTransactionIndex, blockTimestamp;
                BREthereumGas gasUsed;

                ewmTransferExtractStatusIncluded(ewm, tid, NULL, &blockNumber, &blockTransactionIndex, &blockTimestamp, &gasUsed);

                newState = (BRCryptoTransferState) {
                    CRYPTO_TRANSFER_STATE_INCLUDED,
                    { .included = {
                        blockNumber,
                        blockTransactionIndex,
                        blockTimestamp,
                        NULL   // fee from gasUsed?  What is gasPrice???
                    }}
                };

                cryptoTransferSetState (transfer, newState);
                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     wallet,
                                                     transfer,
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CHANGED,
                                                         { .state = { oldState, newState }}
                                                     });
            }
            break;

        case TRANSFER_EVENT_ERRORED:
            if (NULL != transfer) {
                newState = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_ERRORRED };
                strncpy (newState.u.errorred.message, "Some Error", 128);

                cryptoTransferSetState (transfer, newState);
                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     wallet,
                                                     transfer,
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CHANGED,
                                                         { .state = { oldState, newState }}
                                                     });
            }
            break;

        case TRANSFER_EVENT_GAS_ESTIMATE_UPDATED:
            if (NULL != transfer) {
            }
            break;

        case TRANSFER_EVENT_DELETED:
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
                newState = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_DELETED };

                cryptoTransferSetState (transfer, newState);
                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     cryptoWalletTake (wallet),
                                                     cryptoTransferTake (transfer),
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_CHANGED,
                                                         { .state = { oldState, newState }}
                                                     });

                cryptoTransferSetState (transfer, newState);
                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     wallet,
                                                     transfer,
                                                     (BRCryptoTransferEvent) {
                                                         CRYPTO_TRANSFER_EVENT_DELETED
                                                     });
            }
            break;
    }

    // TODO: crypto{Wallet,Transfer}Give()
}

static void
cwmGetBalanceAsETH (BREthereumClientContext context,
                    BREthereumEWM ewm,
                    BREthereumWallet wid,
                    const char *address,
                    int rid) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
                        BRGenericWalletManager manager,
                        int rid) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
                         BRGenericWalletManager manager,
                         const char *address,
                         uint64_t begBlockNumber,
                         uint64_t endBlockNumber,
                         int rid) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

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
cwmSubmitTransactionAsGEN (BRGenericClientContext context,
                           BRGenericWalletManager manager,
                           BRGenericWallet wallet,
                           BRGenericTransfer transfer,
                           int rid) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION;
    callbackState->u.genWithTransaction.wid = wallet;
    callbackState->u.genWithTransaction.tid = transfer;
    callbackState->rid = rid;

    BRGenericHash hash = gwmTransferGetHash (cwm->u.gen, transfer);
    char *hashAsHex = genericHashAsString (hash);
//    UInt256 txHash = UInt256Reverse (transaction->txHash);
//    char *hashAsHex = encodeHexCreate (NULL, txHash.u8, sizeof(txHash.u8));


    size_t txLength = 0; // BRTransactionSerialize (transaction, NULL, 0);
    uint8_t *tx = calloc (txLength, sizeof(uint8_t));
    // BRTransactionSerialize(transaction, tx, txLength);

    cwm->client.gen.funcSubmitTransaction (cwm->client.context,
                                           cryptoWalletManagerTake (cwm),
                                           callbackState,
                                           tx, txLength,
                                           hashAsHex);

    free (hashAsHex);

    cryptoWalletManagerGive (cwm);
}

// MARK: - Client Creation Functions

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

        case CWM_CALLBACK_TYPE_GEN_GET_BLOCK_NUMBER:
            gwmAnnounceBlockNumber (cwm->u.gen,
                                    callbackState->rid,
                                    blockNumber);
            break;

        default:
            break;
    }
    cryptoWalletManagerGive (cwm);
    free (callbackState);
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
    free (callbackState);
}

extern void
cwmAnnounceGetBlockNumberFailure (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState);
    assert (CWM_CALLBACK_TYPE_BTC_GET_BLOCK_NUMBER == callbackState->type ||
            CWM_CALLBACK_TYPE_ETH_GET_BLOCK_NUMBER == callbackState->type ||
            CWM_CALLBACK_TYPE_GEN_GET_BLOCK_NUMBER == callbackState->type);
    free (callbackState);
}

extern void
cwmAnnounceGetTransactionsItemBTC (OwnershipKept BRCryptoWalletManager cwm,
                                   OwnershipGiven BRCryptoCWMClientCallbackState callbackState,
                                   OwnershipKept uint8_t *transaction,
                                   size_t transactionLength,
                                   uint64_t timestamp,
                                   uint64_t blockHeight) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_BTC_GET_TRANSACTIONS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    // TODO(fix): How do we want to handle failure? Should the caller of this function be notified?
    BRTransaction *tx = BRTransactionParse (transaction, transactionLength);
    if (NULL != tx) {
        assert (timestamp <= UINT32_MAX); assert (blockHeight <= UINT32_MAX);
        tx->timestamp = (uint32_t) timestamp;
        tx->blockHeight = (uint32_t) blockHeight;
        bwmAnnounceTransaction (cwm->u.btc, callbackState->rid, tx);
    }

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

extern void
cwmAnnounceGetTransactionsItemGEN (BRCryptoWalletManager cwm,
                                   BRCryptoCWMClientCallbackState callbackState,
                                   uint8_t *transaction,
                                   size_t transactionLength,
                                   uint64_t timestamp,
                                   uint64_t blockHeight) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_BTC_GET_TRANSACTIONS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    // Fundamentally, the `transfer` must allow for determining the `wallet`
    BRGenericTransfer transfer = gwmRecoverTransfer (cwm->u.gen, transaction, transactionLength);

    // Announce to GWM.  Note: the equivalent BTC+ETH announce transaction is going to
    // create BTC+ETH wallet manager + wallet + transfer events that we'll handle by incorporating
    // the BTC+ETH transfer into 'crypto'.  However, GEN does not generate similar events.
    //
    // gwmAnnounceTransfer (cwm->u.gen, callbackState->rid, transfer);

    cryptoWalletManagerHandleTransferGEN (cwm, transfer);

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
        gwmAnnounceTransferComplete (cwm->u.gen,
                                     callbackState->rid,
                                     CRYPTO_TRUE == success);

    } else {
        assert (0);
    }

    cryptoWalletManagerGive (cwm);
    free (callbackState);
}

extern void
cwmAnnounceSubmitTransferSuccess (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState);
    assert (CWM_CALLBACK_TYPE_BTC_SUBMIT_TRANSACTION == callbackState->type ||
            CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    // TODO(fix): What is the memory management story for this transaction?

    if (CWM_CALLBACK_TYPE_BTC_SUBMIT_TRANSACTION == callbackState->type && BLOCK_CHAIN_TYPE_BTC == cwm->type) {
    bwmAnnounceSubmit (cwm->u.btc,
                       callbackState->rid,
                       callbackState->u.btcSubmit.transaction,
                       0);
    }

    else if (CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION == callbackState->type && BLOCK_CHAIN_TYPE_GEN == cwm->type) {
        gwmAnnounceSubmit (cwm->u.gen,
                           callbackState->rid,
                           callbackState->u.genWithTransaction.tid,
                           0);
    }

    cryptoWalletManagerGive (cwm);
    free (callbackState);
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
    free (callbackState);
}

extern void
cwmAnnounceSubmitTransferFailure (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState);
    assert (CWM_CALLBACK_TYPE_BTC_SUBMIT_TRANSACTION == callbackState->type ||
            CWM_CALLBACK_TYPE_ETH_SUBMIT_TRANSACTION == callbackState->type ||
            CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    if (CWM_CALLBACK_TYPE_BTC_SUBMIT_TRANSACTION == callbackState->type && BLOCK_CHAIN_TYPE_BTC == cwm->type) {
        bwmAnnounceSubmit (cwm->u.btc,
                           callbackState->rid,
                           callbackState->u.btcSubmit.transaction,
                           1);

    } else if (CWM_CALLBACK_TYPE_ETH_SUBMIT_TRANSACTION == callbackState->type && BLOCK_CHAIN_TYPE_ETH == cwm->type) {
        // TODO(fix): Do we want to receive the error code and message from Java/Swift?
        ewmAnnounceSubmitTransfer (cwm->u.eth,
                                   callbackState->u.ethWithTransaction.wid,
                                   callbackState->u.ethWithTransaction.tid,
                                   NULL,
                                   0,
                                   "unknown failure",
                                   callbackState->rid);

    } else if (CWM_CALLBACK_TYPE_GEN_SUBMIT_TRANSACTION == callbackState->type && BLOCK_CHAIN_TYPE_GEN == cwm->type) {
        gwmAnnounceSubmit (cwm->u.gen,
                           callbackState->rid,
                           callbackState->u.genWithTransaction.tid,
                           1);

    } else {
        assert (0);
    }

    cryptoWalletManagerGive (cwm);
    free (callbackState);
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
    free (callbackState);
}

extern void
cwmAnnounceGetBalanceFailure (OwnershipKept BRCryptoWalletManager cwm,
                              OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_BALANCE == callbackState->type);
    free (callbackState);
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
    free (callbackState);
}

extern void
cwmAnnounceGetGasPriceFailure (OwnershipKept BRCryptoWalletManager cwm,
                               OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_GAS_PRICE == callbackState->type);
    free (callbackState);
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
    free (callbackState);
}

extern void
cwmAnnounceGetGasEstimateFailure (OwnershipKept BRCryptoWalletManager cwm,
                                  OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_ESTIMATE_GAS == callbackState->type);

    ewmAnnounceGasEstimateFailure (cwm->u.eth,
                                   callbackState->u.ethWithWalletAndCookie.wid,
                                   callbackState->u.ethWithWalletAndCookie.cookie,
                                   callbackState->rid);
    free (callbackState);
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
    free (callbackState);
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
    free (callbackState);
}

extern void
cwmAnnounceGetBlocksFailure (OwnershipKept BRCryptoWalletManager cwm,
                             OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_BLOCKS == callbackState->type);
    free (callbackState);
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
    free (callbackState);
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
    free (callbackState);
}

extern void
cwmAnnounceGetNonceFailure (OwnershipKept BRCryptoWalletManager cwm,
                            OwnershipGiven BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_NONCE == callbackState->type);
    free (callbackState);
}
