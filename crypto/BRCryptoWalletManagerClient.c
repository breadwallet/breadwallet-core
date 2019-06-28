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

    cwm->client.btc.funcGetBlockNumber (cwm->client.context, cwm, callbackState);

    cryptoWalletManagerGive (cwm);
}

static void
cwmGetTransactionsAsBTC (BRWalletManagerClientContext context,
                         BRWalletManager manager,
                         uint64_t begBlockNumber,
                         uint64_t endBlockNumber,
                         int rid) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_BTC_GET_TRANSACTIONS;
    callbackState->rid = rid;

    BRWalletManagerGenerateUnusedAddrs (manager, 25);

    size_t addrCount = 0;
    BRAddress *addrs = BRWalletManagerGetAllAddrs (manager, &addrCount);

    char **addrStrings = calloc (addrCount, sizeof(char *));
    for (size_t index = 0; index < addrCount; index ++)
        addrStrings[index] = (char *) &addrs[index];

    cwm->client.btc.funcGetTransactions (cwm->client.context, cwm, callbackState, addrStrings, addrCount, begBlockNumber, endBlockNumber);

    free (addrStrings);
    free (addrs);
    cryptoWalletManagerGive (cwm);
}

static void
cwmSubmitTransactionAsBTC (BRWalletManagerClientContext context,
                           BRWalletManager manager,
                           BRWallet *wallet,
                           BRTransaction *transaction,
                           int rid) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_BTC_SUBMIT_TRANSACTION;
    callbackState->u.btcSubmit.transaction = transaction;
    callbackState->rid = rid;

    size_t txLength = BRTransactionSerialize (transaction, NULL, 0);
    uint8_t *tx = calloc (txLength, sizeof(uint8_t));
    BRTransactionSerialize(transaction, tx, txLength);

    cwm->client.btc.funcSubmitTransaction (cwm->client.context, cwm, callbackState, tx, txLength);

    free (tx);
    cryptoWalletManagerGive (cwm);
}

static void
cwmWalletManagerEventAsBTC (BRWalletManagerClientContext context,
                            BRWalletManager btcManager,
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
            break;
    }

    if (needEvent)
        cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                  cryptoWalletManagerTake (cwm),
                                                  cwmEvent);
}

static void
cwmWalletEventAsBTC (BRWalletManagerClientContext context,
                     BRWalletManager btcManager,
                     BRWallet *btcWallet,
                     BRWalletEvent event) {
    // Extract CWM and avoid a race condition by ensuring cwm->u.btc
    BRCryptoWalletManager cwm = context;
    if (NULL == cwm->u.btc) cwm->u.btc = btcManager;

    assert (BLOCK_CHAIN_TYPE_BTC == cwm->type);

    // Get `currency` (it is 'taken')
    BRCryptoCurrency currency = cryptoNetworkGetCurrency (cwm->network);

    // Get `wallet`.  For 'CREATED" this *will* be NULL
    BRCryptoWallet wallet   = cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet); // taken

    // Demand 'wallet' otherwise
    assert (NULL != wallet || BITCOIN_WALLET_CREATED == event.type );

    switch (event.type) {
        case BITCOIN_WALLET_CREATED: {
            // Demand no 'wallet'
            assert (NULL == wallet);

            // We'll create a wallet using the currency's default unit.
            BRCryptoUnit unit = cryptoNetworkGetUnitAsDefault (cwm->network, currency);

            // The wallet, finally.
            wallet = cryptoWalletCreateAsBTC (unit, unit, cwm->u.btc, btcWallet);

            // Avoid a race condition on the CWM's 'primaryWallet'
            if (NULL == cwm->wallet) cwm->wallet = cryptoWalletTake (wallet);

            // Update CWM with the new wallet.
            cryptoWalletManagerAddWallet (cwm, wallet);

            // Done with `unit1
            cryptoUnitGive (unit);

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
                                                          { .wallet = { wallet }}
                                                      });

            break;
        }

        case BITCOIN_WALLET_BALANCE_UPDATED: {
            // Demand 'wallet'
            assert (NULL != wallet);

            // The balance value will be 'SATOSHI', so use the currency's base unit.
            BRCryptoUnit     unit     = cryptoNetworkGetUnitAsBase (cwm->network, currency);

            // Get the amount (it is 'taken')
            BRCryptoAmount amount     = cryptoAmountCreateInteger (event.u.balance.satoshi, unit); // taken

            // Done with 'unit'
            cryptoUnitGive (unit);

            // Generate BALANCE_UPDATED with 'amount' (taken)
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               wallet,
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_BALANCE_UPDATED,
                                                   { .balanceUpdated = { amount }}
                                               });

            break;
        }

        case BITCOIN_WALLET_FEE_PER_KB_UPDATED: {
            BRCryptoFeeBasis feeBasis = cryptoFeeBasisCreateAsBTC (event.u.feePerKb.value);

            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               wallet,
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED,
                                                   { .feeBasisUpdated = { feeBasis }}
                                               });
            break;
        }

        case BITCOIN_WALLET_TRANSACTION_SUBMITTED: {
            // Demand 'wallet'
            assert (NULL != wallet);

            // Find the wallet's transfer for 'btc'. (it is 'taken')
            BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, event.u.submitted.transaction);

            // It must exist already in wallet (otherwise how could it have been submitted?)
            assert (NULL != transfer);

            // Create the event with 'transfer' (taken)
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               wallet,
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED,
                                                   { .transfer = { transfer }}
                                               });

            break;
        }

        case BITCOIN_WALLET_DELETED: {
            // Demand 'wallet' ...
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
                                               wallet,
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_DELETED
                                               });

            break;
        }
    }

    cryptoCurrencyGive (currency);
}

static void
cwmTransactionEventAsBTC (BRWalletManagerClientContext context,
                          BRWalletManager btcManager,
                          BRWallet *btcWallet,
                          BRTransaction *btcTransaction,
                          BRTransactionEvent event) {
    // Extract CWM and avoid a race condition by ensuring cwm->u.btc
    BRCryptoWalletManager cwm = context;
    if (NULL == cwm->u.btc) cwm->u.btc = btcManager;

    assert (BLOCK_CHAIN_TYPE_BTC == cwm->type);

    // Find 'wallet' based on BTC... (it is taken)
    BRCryptoWallet wallet = cryptoWalletManagerFindWalletAsBTC (cwm, btcWallet);

    // ... and demand 'wallet'
    assert (NULL != wallet && btcWallet == cryptoWalletAsBTC (wallet));

    // Get `transfer`.  For 'CREATED" this *will* be NULL
    BRCryptoTransfer transfer = cryptoWalletFindTransferAsBTC (wallet, btcTransaction); // taken

    // Demand 'transfer' otherwise
    assert (NULL != transfer || BITCOIN_TRANSACTION_ADDED == event.type );

    switch (event.type) {
        case BITCOIN_TRANSACTION_ADDED: {
            assert (NULL == transfer);

            // The transfer finally - based on the wallet's currency (BTC)
            transfer = cryptoTransferCreateAsBTC (cryptoWalletGetCurrency (wallet),
                                                  cryptoWalletAsBTC (wallet),
                                                  btcTransaction);

            // Generate a CYTPO transfer event for CREATED'...
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

            break;
        }

        case BITCOIN_TRANSACTION_UPDATED: {
            assert (NULL != transfer);

            BRCryptoTransferState oldState = cryptoTransferGetState (transfer);

            // TODO: The newState is always 'included'?

            // Only announce changes.
            if (CRYPTO_TRANSFER_STATE_INCLUDED != oldState.type) {
                BRCryptoTransferState newState = {
                    CRYPTO_TRANSFER_STATE_INCLUDED,
                    { .included = {
                        event.u.updated.blockHeight,
                        0,
                        event.u.updated.timestamp,
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

            break;
        }

        case BITCOIN_TRANSACTION_DELETED: {
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

            break;
        }
    }

    cryptoTransferGive (transfer);
    cryptoWalletGive (wallet);
}

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-function"
static void
cwmPublishTransactionAsBTC (void *context,
                            int error) {
    BRCryptoWalletManager cwm = context;
    (void) cwm;
}
#pragma clang diagnostic pop
#pragma GCC diagnostic pop

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

        case EWM_EVENT_BLOCK_HEIGHT_UPDATED:
            cwmEvent = (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED,
                { .blockHeight = { ewmGetBlockHeight(ewm) }}
            };
            break;
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

    switch (event) {
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

                cwm->listener.walletEventCallback(cwm->listener.context,
                                                  cryptoWalletManagerTake(cwm),
                                                  wallet,
                                                  (BRCryptoWalletEvent) {
                                                      CRYPTO_WALLET_EVENT_BALANCE_UPDATED,
                                                      {.balanceUpdated = {cryptoAmount}}
                                                  });
            }
            break;
        }

        case WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED:
        case WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED:
            if (NULL != wallet) {
                BRCryptoFeeBasis feeBasis = cryptoFeeBasisCreateAsETH(ewmWalletGetDefaultGasLimit(cwm->u.eth, wid),
                                                                      ewmWalletGetDefaultGasPrice(cwm->u.eth,wid));
                cwm->listener.walletEventCallback(cwm->listener.context,
                                                  cryptoWalletManagerTake(cwm),
                                                  wallet,
                                                  (BRCryptoWalletEvent) {
                                                      CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED,
                                                      {.feeBasisUpdated = {feeBasis}}
                                                  });
            }
            break;

        case WALLET_EVENT_DELETED:
            if (NULL != wallet) {
                cwm->listener.walletEventCallback(cwm->listener.context,
                                                  cryptoWalletManagerTake(cwm),
                                                  wallet,
                                                  (BRCryptoWalletEvent) {
                                                      CRYPTO_WALLET_EVENT_DELETED
                                                  });
            }
            break;
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
        case TRANSFER_EVENT_CREATED:
            assert (NULL == transfer);
            // if (NULL != transfer) cryptoTransferGive (transfer);

            transfer = cryptoTransferCreateAsETH (cryptoWalletGetCurrency (wallet),
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
            break;

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
        cwm->client.eth.funcGetEtherBalance (cwm->client.context, cwm, callbackState, networkName, address);
    } else {
        cwm->client.eth.funcGetTokenBalance (cwm->client.context, cwm, callbackState, networkName, address, tokenGetAddress (token));
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

    cwm->client.eth.funcGetGasPrice (cwm->client.context, cwm, callbackState, networkName);

    free (networkName);
    cryptoWalletManagerGive (cwm);
}

static void
cwmGetGasEstimateAsETH (BREthereumClientContext context,
                        BREthereumEWM ewm,
                        BREthereumWallet wid,
                        BREthereumTransfer tid,
                        const char *from,
                        const char *to,
                        const char *amount,
                        const char *data,
                        int rid) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_ETH_ESTIMATE_GAS;
    callbackState->u.ethWithTransaction.wid = wid;
    callbackState->u.ethWithTransaction.tid = tid;
    callbackState->rid = rid;

    BREthereumNetwork network = ewmGetNetwork (ewm);
    char *networkName = networkCopyNameAsLowercase (network);

    cwm->client.eth.funcEstimateGas (cwm->client.context, cwm, callbackState, networkName, from, to, amount, data);

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

    cwm->client.eth.funcSubmitTransaction (cwm->client.context, cwm, callbackState, networkName, transaction);

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

    cwm->client.eth.funcGetTransactions (cwm->client.context, cwm, callbackState, networkName, account, begBlockNumber, endBlockNumber);

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

    cwm->client.eth.funcGetLogs (cwm->client.context, cwm, callbackState, networkName, contract, addressIgnore, event, begBlockNumber, endBlockNumber);

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

    cwm->client.eth.funcGetBlocks (cwm->client.context, cwm, callbackState, networkName, address, interests, blockNumberStart, blockNumberStop);

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

    cwm->client.eth.funcGetTokens (cwm->client.context, cwm, callbackState);

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

    cwm->client.eth.funcGetBlockNumber (cwm->client.context, cwm, callbackState, networkName);

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

    cwm->client.eth.funcGetNonce (cwm->client.context, cwm, callbackState, networkName, address);

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

    cwm->client.gen.funcGetBlockNumber (cwm->client.context, cwm, callbackState);

    cryptoWalletManagerGive (cwm);
}

static void
cwmGetTransactionsAsGEN (BRGenericClientContext context,
                         BRGenericWalletManager manager,
                         uint64_t begBlockNumber,
                         uint64_t endBlockNumber,
                         int rid) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTake (context);

    BRCryptoCWMClientCallbackState callbackState = calloc (1, sizeof(struct BRCryptoCWMClientCallbackStateRecord));
    callbackState->type = CWM_CALLBACK_TYPE_GEN_GET_TRANSACTIONS;
    callbackState->rid = rid;

    cwm->client.gen.funcGetTransactions (cwm->client.context, cwm, callbackState, begBlockNumber, endBlockNumber);

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


    size_t txLength = 0; // BRTransactionSerialize (transaction, NULL, 0);
    uint8_t *tx = calloc (txLength, sizeof(uint8_t));
    // BRTransactionSerialize(transaction, tx, txLength);

    cwm->client.gen.funcSubmitTransaction (cwm->client.context, cwm, callbackState, tx, txLength);

    cryptoWalletManagerGive (cwm);
}

// MARK: - Client Creation Functions

extern BRWalletManagerClient
cryptoWalletManagerClientCreateBTCClient (BRCryptoWalletManager cwm) {
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
cryptoWalletManagerClientCreateETHClient (BRCryptoWalletManager cwm) {
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
cwmAnnounceGetBlockNumberSuccessAsInteger (BRCryptoWalletManager cwm,
                                           BRCryptoCWMClientCallbackState callbackState,
                                           uint64_t blockNumber) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_BTC_GET_BLOCK_NUMBER == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    bwmAnnounceBlockNumber (cwm->u.btc,
                            callbackState->rid,
                            blockNumber);

    cryptoWalletManagerGive (cwm);
    free (callbackState);
}

extern void
cwmAnnounceGetBlockNumberSuccessAsString (BRCryptoWalletManager cwm,
                                          BRCryptoCWMClientCallbackState callbackState,
                                          const char *blockNumber) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_BLOCK_NUMBER == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceBlockNumber (cwm->u.eth,
                            blockNumber,
                            callbackState->rid);

    cryptoWalletManagerGive (cwm);
    free (callbackState);
}

extern void
cwmAnnounceGetBlockNumberFailure (BRCryptoWalletManager cwm,
                                  BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState);
    assert (CWM_CALLBACK_TYPE_BTC_GET_BLOCK_NUMBER == callbackState->type ||
            CWM_CALLBACK_TYPE_ETH_GET_BLOCK_NUMBER == callbackState->type ||
            CWM_CALLBACK_TYPE_GEN_GET_BLOCK_NUMBER == callbackState->type);
    free (callbackState);
}

extern void
cwmAnnounceGetTransactionsItemBTC (BRCryptoWalletManager cwm,
                                   BRCryptoCWMClientCallbackState callbackState,
                                   uint8_t *transaction,
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
cwmAnnounceGetTransactionsItemETH (BRCryptoWalletManager cwm,
                                   BRCryptoCWMClientCallbackState callbackState,
                                   const char *hash,
                                   const char *from,
                                   const char *to,
                                   const char *contract,
                                   const char *amount, // value
                                   const char *gasLimit,
                                   const char *gasPrice,
                                   const char *data,
                                   const char *nonce,
                                   const char *gasUsed,
                                   const char *blockNumber,
                                   const char *blockHash,
                                   const char *blockConfirmations,
                                   const char *blockTransactionIndex,
                                   const char *blockTimestamp,
                                   // cumulative gas used,
                                   // confirmations
                                   // txreceipt_status
                                   const char *isError) {
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
cwmAnnounceGetTransactionsComplete (BRCryptoWalletManager cwm,
                                    BRCryptoCWMClientCallbackState callbackState,
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
cwmAnnounceSubmitTransferSuccess (BRCryptoWalletManager cwm,
                                  BRCryptoCWMClientCallbackState callbackState) {
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
cwmAnnounceSubmitTransferSuccessForHash (BRCryptoWalletManager cwm,
                                         BRCryptoCWMClientCallbackState callbackState,
                                         const char *hash) {
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
cwmAnnounceSubmitTransferFailure (BRCryptoWalletManager cwm,
                                  BRCryptoCWMClientCallbackState callbackState) {
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
cwmAnnounceGetBalanceSuccess (BRCryptoWalletManager cwm,
                              BRCryptoCWMClientCallbackState callbackState,
                              const char *balance) {
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
cwmAnnounceGetBalanceFailure (BRCryptoWalletManager cwm,
                              BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_BALANCE == callbackState->type);
    free (callbackState);
}

extern void
cwmAnnounceGetGasPriceSuccess (BRCryptoWalletManager cwm,
                               BRCryptoCWMClientCallbackState callbackState,
                               const char *gasPrice) {
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
cwmAnnounceGetGasPriceFailure (BRCryptoWalletManager cwm,
                               BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_GAS_PRICE == callbackState->type);
    free (callbackState);
}

extern void
cwmAnnounceGetGasEstimateSuccess (BRCryptoWalletManager cwm,
                                  BRCryptoCWMClientCallbackState callbackState,
                                  const char *gasEstimate) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_ESTIMATE_GAS == callbackState->type);
    cwm = cryptoWalletManagerTake (cwm);

    ewmAnnounceGasEstimate (cwm->u.eth,
                            callbackState->u.ethWithTransaction.wid,
                            callbackState->u.ethWithTransaction.tid,
                            gasEstimate,
                            callbackState->rid);

    cryptoWalletManagerGive (cwm);
    free (callbackState);
}

extern void
cwmAnnounceGetGasEstimateFailure (BRCryptoWalletManager cwm,
                                  BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_ESTIMATE_GAS == callbackState->type);
    free (callbackState);
}

extern void
cwmAnnounceGetLogsItem(BRCryptoWalletManager cwm,
                       BRCryptoCWMClientCallbackState callbackState,
                       const char *strHash,
                       const char *strContract,
                       int topicCount,
                       const char **arrayTopics,
                       const char *strData,
                       const char *strGasPrice,
                       const char *strGasUsed,
                       const char *strLogIndex,
                       const char *strBlockNumber,
                       const char *strBlockTransactionIndex,
                       const char *strBlockTimestamp) {
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
cwmAnnounceGetLogsComplete(BRCryptoWalletManager cwm,
                           BRCryptoCWMClientCallbackState callbackState,
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
cwmAnnounceGetBlocksSuccess (BRCryptoWalletManager cwm,
                             BRCryptoCWMClientCallbackState callbackState,
                             int blockNumbersCount,
                             uint64_t *blockNumbers) {
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
cwmAnnounceGetBlocksFailure (BRCryptoWalletManager cwm,
                             BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_BLOCKS == callbackState->type);
    free (callbackState);
}

extern void
cwmAnnounceGetTokensItem(BRCryptoWalletManager cwm,
                         BRCryptoCWMClientCallbackState callbackState,
                         const char *address,
                         const char *symbol,
                         const char *name,
                         const char *description,
                         unsigned int decimals,
                         const char *strDefaultGasLimit,
                         const char *strDefaultGasPrice) {
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
cwmAnnounceGetTokensComplete(BRCryptoWalletManager cwm,
                             BRCryptoCWMClientCallbackState callbackState,
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
cwmAnnounceGetNonceSuccess (BRCryptoWalletManager cwm,
                            BRCryptoCWMClientCallbackState callbackState,
                            const char *address,
                            const char *nonce) {
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
cwmAnnounceGetNonceFailure (BRCryptoWalletManager cwm,
                            BRCryptoCWMClientCallbackState callbackState) {
    assert (cwm); assert (callbackState); assert (CWM_CALLBACK_TYPE_ETH_GET_NONCE == callbackState->type);
    free (callbackState);
}
