//
//  BRCryptoWalletManager.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
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

#include "BRCryptoWalletManager.h"
#include "BRCryptoBase.h"
#include "BRCryptoPrivate.h"

#include "bitcoin/BRWalletManager.h"
#include "ethereum/BREthereum.h"

static void
cryptoWalletManagerRelease (BRCryptoWalletManager cwm);

struct BRCryptoWalletManagerRecord {
    BRCryptoBlockChainType type;
    union {
        BRWalletManager btc;
        BREthereumEWM eth;
    } u;

    BRCryptoCWMListener listener;
    BRCryptoCWMClient client;
    BRCryptoNetwork network;
    BRCryptoAccount account;
    BRSyncMode mode;

    BRCryptoWalletManagerState state;

    /// The primary wallet
    BRCryptoWallet wallet;

    /// All wallets
    BRArrayOf(BRCryptoWallet) wallets;
    char *path;

    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoWalletManager, cryptoWalletManager)

/// MARK: - BTC Callbacks

static void
cwmGetBlockNumberAsBTC (BRWalletManagerClientContext context,
                        BRWalletManager manager,
                        int rid) {
    BRCryptoWalletManager cwm = context;
    cwm->client.btc.funcGetBlockNumber (cwm->client.context, manager, rid);
}

static void
cwmGetTransactionsAsBTC (BRWalletManagerClientContext context,
                         BRWalletManager btcManager,
                         uint64_t begBlockNumber,
                         uint64_t endBlockNumber,
                         int rid) {
    BRCryptoWalletManager cwm = context;
    cwm->client.btc.funcGetTransactions (cwm->client.context, btcManager, begBlockNumber, endBlockNumber, rid);
}

static void
cwmSubmitTransactionAsBTC (BRWalletManagerClientContext context,
                           BRWalletManager manager,
                           BRWallet *wallet,
                           BRTransaction *transaction,
                           int rid) {
    BRCryptoWalletManager cwm = context;
    cwm->client.btc.funcSubmitTransaction (cwm->client.context, manager, wallet, transaction, rid);
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
                                               wallet,
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_TRANSFER_ADDED,
                                                   { .transfer = { transfer }}
                                               });
            break;
        }

        case BITCOIN_TRANSACTION_UPDATED: {
            assert (NULL != transfer);

            BRCryptoTransferState oldState = cryptoTransferGetState (transfer);

            // TODO: The newState is always 'included'?
            BRCryptoTransferState newState = {
                CRYPTO_TRANSFER_STATE_INCLUDED,
                { .included = {
                    event.u.updated.blockHeight,
                    0,
                    event.u.updated.timestamp,
                    NULL
                }}
            };

            // Only announce changes.
            if (CRYPTO_TRANSFER_STATE_INCLUDED != oldState.type) {
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
                                                 wallet,
                                                 transfer,
                                                 (BRCryptoTransferEvent) {
                                                     CRYPTO_TRANSFER_EVENT_DELETED
                                                 });

            break;
        }
    }
}

static void
cwmPublishTransactionAsBTC (void *context,
                            int error) {
    BRCryptoWalletManager cwm = context;
    (void) cwm;
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

    int needEvent = 1;
    BRCryptoWalletEvent cwmEvent = { CRYPTO_WALLET_EVENT_CREATED };

    switch (event) {
        case WALLET_EVENT_CREATED:
            needEvent = 0;

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
                break;
            }

        case WALLET_EVENT_BALANCE_UPDATED: {
            BRCryptoCurrency currency = cryptoNetworkGetCurrency (cwm->network);
            BRCryptoUnit     unit     = cryptoNetworkGetUnitAsBase (cwm->network, currency);

            // Get the wallet's amount...
            BREthereumAmount amount = ewmWalletGetBalance (cwm->u.eth, wid);

            // ... and then the 'raw integer' (UInt256) value
            UInt256 value = (AMOUNT_ETHER == amountGetType (amount)
                             ? amountGetEther(amount).valueInWEI
                             : amountGetTokenQuantity(amount).valueAsInteger);

            // Use currency to create a cyrptoAmount in the base unit (implicitly).
            BRCryptoAmount cryptoAmount = cryptoAmountCreate (currency, CRYPTO_FALSE, value);

            cryptoUnitGive (unit);
            cryptoCurrencyGive (currency);

            cwmEvent = (BRCryptoWalletEvent) {
                CRYPTO_WALLET_EVENT_BALANCE_UPDATED,
                { .balanceUpdated = { cryptoAmount }}
            };
            break;
        }

        case WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED:
        case WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED:
            cwmEvent = (BRCryptoWalletEvent) {
                CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED,
                { .feeBasisUpdated = { cryptoFeeBasisCreateAsETH
                    (ewmWalletGetDefaultGasLimit (cwm->u.eth, wid),
                     ewmWalletGetDefaultGasPrice (cwm->u.eth, wid)) }}
            };
            break;

        case WALLET_EVENT_DELETED:
            cwmEvent = (BRCryptoWalletEvent) {
                CRYPTO_WALLET_EVENT_DELETED
            };

            break;
    }

    if (needEvent) {
        cwm->listener.walletEventCallback (cwm->listener.context,
                                           cryptoWalletManagerTake (cwm),
                                           wallet,
                                           cwmEvent);

        // TODO: crypto{Wallet,Transfer}Give()
    }
}

static void
cwmEventTokenAsETH (BREthereumClientContext context,
                    BREthereumEWM ewm,
                    BREthereumToken token,
                    BREthereumTokenEvent event) {

    BRCryptoWalletManager cwm = context; (void) cwm;

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

    int needEvent = 1;
    BRCryptoTransferEvent cwmEvent = { CRYPTO_TRANSFER_EVENT_CREATED };

    BRCryptoTransferState oldState = { CRYPTO_TRANSFER_STATE_CREATED };
    BRCryptoTransferState newState = { CRYPTO_TRANSFER_STATE_CREATED };
    if (NULL != transfer) oldState = cryptoTransferGetState (transfer);

    switch (event) {
        case TRANSFER_EVENT_CREATED:
            if (NULL != transfer) cryptoTransferGive (transfer);

            needEvent = 0;
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
            newState = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_SIGNED };
            cwmEvent = (BRCryptoTransferEvent) {
                CRYPTO_TRANSFER_EVENT_CHANGED,
                { .state = { oldState, newState }}
            };
            cryptoTransferSetState (transfer, newState);
            break;

        case TRANSFER_EVENT_SUBMITTED:
            newState = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_SUBMITTED };
            cwmEvent = (BRCryptoTransferEvent) {
                CRYPTO_TRANSFER_EVENT_CHANGED,
                { .state = { oldState, newState }}
            };
            cryptoTransferSetState (transfer, newState);
            break;

        case TRANSFER_EVENT_INCLUDED: {
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
            cwmEvent = (BRCryptoTransferEvent) {
                CRYPTO_TRANSFER_EVENT_CHANGED,
                { .state = { oldState, newState }}
            };
            cryptoTransferSetState (transfer, newState);
            break;
        }
        case TRANSFER_EVENT_ERRORED:
            newState = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_ERRORRED };
            strncpy (newState.u.errorred.message, "Some Error", 128);

            cwmEvent = (BRCryptoTransferEvent) {
                CRYPTO_TRANSFER_EVENT_CHANGED,
                { .state = { oldState, newState }}
            };
            cryptoTransferSetState (transfer, newState);
            break;

        case TRANSFER_EVENT_GAS_ESTIMATE_UPDATED:
            needEvent = 0;
            break;

        case TRANSFER_EVENT_DELETED:
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
            cwmEvent = (BRCryptoTransferEvent) {
                CRYPTO_TRANSFER_EVENT_CHANGED,
                { .state = { oldState, newState }}
            };
            cryptoTransferSetState (transfer, newState);

            cwm->listener.transferEventCallback (cwm->listener.context,
                                                 cryptoWalletManagerTake (cwm),
                                                 cryptoWalletTake (wallet),
                                                 cryptoTransferTake (transfer),
                                                 cwmEvent);

            cwmEvent = (BRCryptoTransferEvent) {CRYPTO_TRANSFER_EVENT_DELETED };
            break;
    }

    if (needEvent) {
        cwm->listener.transferEventCallback (cwm->listener.context,
                                             cryptoWalletManagerTake (cwm),
                                             wallet,
                                             transfer,
                                             cwmEvent);

        // TODO: crypto{Wallet,Transfer}Give()
    }
}

static void
cwmGetBalanceAsETH (BREthereumClientContext context,
                    BREthereumEWM ewm,
                    BREthereumWallet wid,
                    const char *address,
                    int rid) {
    BRCryptoWalletManager cwm = context;
    cwm->client.eth.funcGetBalance (cwm->client.context, ewm, wid, address, rid);
}

static void
cwmGetGasPriceAsETH (BREthereumClientContext context,
                     BREthereumEWM ewm,
                     BREthereumWallet wid,
                     int rid) {
    BRCryptoWalletManager cwm = context;
    cwm->client.eth.funcGetGasPrice (cwm->client.context, ewm, wid, rid);
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
    BRCryptoWalletManager cwm = context;
    cwm->client.eth.funcEstimateGas (cwm->client.context, ewm, wid, tid, from, to, amount, data, rid);
}

static void
cwmSubmitTransactionAsETH (BREthereumClientContext context,
                           BREthereumEWM ewm,
                           BREthereumWallet wid,
                           BREthereumTransfer tid,
                           const char *transaction,
                           int rid) {
    BRCryptoWalletManager cwm = context;
    cwm->client.eth.funcSubmitTransaction (cwm->client.context, ewm, wid, tid, transaction, rid);
}

static void
cwmGetTransactionsAsETH (BREthereumClientContext context,
                         BREthereumEWM ewm,
                         const char *account,
                         uint64_t begBlockNumber,
                         uint64_t endBlockNumber,
                         int rid) {
    BRCryptoWalletManager cwm = context;
    cwm->client.eth.funcGetTransactions (cwm->client.context, ewm, account, begBlockNumber, endBlockNumber, rid);
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
    BRCryptoWalletManager cwm = context;
    cwm->client.eth.funcGetLogs (cwm->client.context, ewm, contract, addressIgnore, event, begBlockNumber, endBlockNumber, rid);
}

static void
cwmGetBlocksAsETH (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   const char *address,
                   BREthereumSyncInterestSet interests,
                   uint64_t blockNumberStart,
                   uint64_t blockNumberStop,
                   int rid) {
    BRCryptoWalletManager cwm = context;
    cwm->client.eth.funcGetBlocks (cwm->client.context, ewm, address, interests, blockNumberStart, blockNumberStop, rid);
}

static void
cwmGetTokensAsETH (BREthereumClientContext context,
                   BREthereumEWM ewm,
                   int rid) {
    BRCryptoWalletManager cwm = context;
    cwm->client.eth.funcGetTokens (cwm->client.context, ewm, rid);
}

static void
cwmGetBlockNumberAsETH (BREthereumClientContext context,
                        BREthereumEWM ewm,
                        int rid) {
    BRCryptoWalletManager cwm = context;
    cwm->client.eth.funcGetBlockNumber (cwm->client.context, ewm, rid);
}

static void
cwmGetNonceAsETH (BREthereumClientContext context,
                  BREthereumEWM ewm,
                  const char *address,
                  int rid) {
    BRCryptoWalletManager cwm = context;
    cwm->client.eth.funcGetNonce (cwm->client.context, ewm, address, rid);
}


/// =============================================================================================
///
/// MARK: - Wallet Manager
///
///
#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-function"
static BRArrayOf(BRCryptoCurrency)
cryptoWalletManagerGetCurrenciesOfIntereest (BRCryptoWalletManager cwm) {
    BRArrayOf(BRCryptoCurrency) currencies;

    array_new (currencies, 3);
    return currencies;
}

static void
cryptoWalletManagerReleaseCurrenciesOfIntereest (BRCryptoWalletManager cwm,
                                                 BRArrayOf(BRCryptoCurrency) currencies) {
    for (size_t index = 0; index < array_count(currencies); index++)
        cryptoCurrencyGive (currencies[index]);
    array_free (currencies);
}
#pragma clang diagnostic pop
#pragma GCC diagnostic pop

static BRCryptoWalletManager
cryptoWalletManagerCreateInternal (BRCryptoCWMListener listener,
                                   BRCryptoCWMClient client,
                                   BRCryptoAccount account,
                                   BRCryptoBlockChainType type,
                                   BRCryptoNetwork network,
                                   BRSyncMode mode,
                                   char *path) {
    BRCryptoWalletManager cwm = calloc (1, sizeof (struct BRCryptoWalletManagerRecord));

    cwm->type = type;
    cwm->listener = listener;
    cwm->client  = client;
    cwm->network = cryptoNetworkTake (network);
    cwm->account = cryptoAccountTake (account);
    cwm->state   = CRYPTO_WALLET_MANAGER_STATE_CREATED;
    cwm->mode = mode;
    cwm->path = strdup (path);

    cwm->wallet = NULL;
    array_new (cwm->wallets, 1);

    cwm->ref = CRYPTO_REF_ASSIGN (cryptoWalletManagerRelease);

    return cwm;
}

extern BRCryptoWalletManager
cryptoWalletManagerCreate (BRCryptoCWMListener listener,
                           BRCryptoCWMClient client,
                           BRCryptoAccount account,
                           BRCryptoNetwork network,
                           BRSyncMode mode,
                           const char *path) {

    // TODO: extend path... with network-type : network-name - or is that done by ewmCreate(), ...
    char *cwmPath = strdup (path);

    BRCryptoWalletManager  cwm  = cryptoWalletManagerCreateInternal (listener,
                                                                     client,
                                                                     account,
                                                                     cryptoNetworkGetBlockChainType (network),
                                                                     network,
                                                                     mode,
                                                                     cwmPath);

    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWalletManagerClient client = {
                cwm,
                cwmGetBlockNumberAsBTC,
                cwmGetTransactionsAsBTC,
                cwmSubmitTransactionAsBTC,
                cwmTransactionEventAsBTC,
                cwmWalletEventAsBTC,
                cwmWalletManagerEventAsBTC
            };

            // Race Here - WalletEvent before cwm->u.btc is assigned.
            cwm->u.btc = BRWalletManagerNew (client,
                                             cryptoAccountAsBTC (account),
                                             cryptoNetworkAsBTC (network),
                                             (uint32_t) cryptoAccountGetTimestamp(account),
                                             mode,
                                             cwmPath);

            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumClient client = {
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

            // Race Here - WalletEvent before cwm->u.eth is assigned.
            cwm->u.eth = ewmCreate (cryptoNetworkAsETH(network),
                                    cryptoAccountAsETH(account),
                                    (BREthereumTimestamp) cryptoAccountGetTimestamp(account),
                                    (BREthereumMode) mode,
                                    client,
                                    cwmPath);

            // During the creation of both the BTC and ETH wallet managers, the primary wallet will
            // be created and will have wallet events generated.  There will be a race on `cwm->wallet` but
            // that race is resolved in the BTC and ETH event handlers, respectively.
            //
            // There are others wallets to create.  Specifically, for the Ethereum network we'll want to
            // create wallets for each and every ERC20 token of interest.
            //
            // TODO: How to decide on tokens-of-interest and when to decide (CORE-291).
            //
            // We should pass in 'tokens-of-interest' as List<Currency-Code> and then add the tokens
            // one-by-one - specifically 'add them' not 'announce them'.  If we 'announce them' then the
            // install event gets queued until the wallet manager connects.  Or, we could query them,
            // as we do below, and have the BRD endpoint provide them asynchronously and handled w/
            // 'announce..
            //
            // When a token is announced, we'll create a CRYPTO wallet if-and-only-if the token has
            // a knonw currency.  EVERY TOKEN SHOULD, eventually - key word being 'eventually'.
            //
            // TODO: Only finds MAINNET tokens
            ewmUpdateTokens(cwm->u.eth);

            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            break;
        }
    }

    // NOTE: Race on cwm->u.{btc,et} is resolved in the event handlers


    free (cwmPath);

    //    listener.walletManagerEventCallback (listener.context, cwm);  // created
    //    listener.walletEventCallback (listener.context, cwm, cwm->wallet);

    return cwm;
}

static void
cryptoWalletManagerRelease (BRCryptoWalletManager cwm) {
    printf ("Wallet Manager: Release\n");
    cryptoAccountGive (cwm->account);
    cryptoNetworkGive (cwm->network);
    if (NULL != cwm->wallet) cryptoWalletGive (cwm->wallet);

    for (size_t index = 0; index < array_count(cwm->wallets); index++)
        cryptoWalletGive (cwm->wallets[index]);
    array_free (cwm->wallets);

    free (cwm->path);
    free (cwm);
}

extern BRCryptoNetwork
cryptoWalletManagerGetNetwork (BRCryptoWalletManager cwm) {
    return cryptoNetworkTake (cwm->network);
}

extern BRCryptoAccount
cryptoWalletManagerGetAccount (BRCryptoWalletManager cwm) {
    return cryptoAccountTake (cwm->account);
}

extern BRSyncMode
cryptoWalletManagerGetMode (BRCryptoWalletManager cwm) {
    return cwm->mode;
}

extern BRCryptoWalletManagerState
cryptoWalletManagerGetState (BRCryptoWalletManager cwm) {
    return cwm->state;
}

private_extern void
cryptoWalletManagerSetState (BRCryptoWalletManager cwm,
                             BRCryptoWalletManagerState state) {
    cwm->state = state;
}

extern const char *
cryptoWalletManagerGetPath (BRCryptoWalletManager cwm) {
    return cwm->path;
}

extern BRCryptoWallet
cryptoWalletManagerGetWallet (BRCryptoWalletManager cwm) {
    return cryptoWalletTake (cwm->wallet);
}

extern size_t
cryptoWalletManagerGetWalletsCount (BRCryptoWalletManager cwm) {
    return array_count (cwm->wallets);
}

extern BRCryptoWallet
cryptoWalletManagerGetWalletAtIndex (BRCryptoWalletManager cwm,
                                     size_t index) {
    return cryptoWalletTake (cwm->wallets[index]);
}

extern BRCryptoWallet
cryptoWalletManagerGetWalletForCurrency (BRCryptoWalletManager cwm,
                                         BRCryptoCurrency currency) {
    for (size_t index = 0; index < array_count(cwm->wallets); index++)
        if (currency == cryptoWalletGetCurrency (cwm->wallets[index]))
            return cryptoWalletTake (cwm->wallets[index]);
    return NULL;
}

extern BRCryptoBoolean
cryptoWalletManagerHasWallet (BRCryptoWalletManager cwm,
                              BRCryptoWallet wallet) {
    for (size_t index = 0; index < array_count (cwm->wallets); index++)
        if (CRYPTO_TRUE == cryptoWalletEqual(cwm->wallets[index], wallet))
            return CRYPTO_TRUE;
    return CRYPTO_FALSE;
}

private_extern void
cryptoWalletManagerAddWallet (BRCryptoWalletManager cwm,
                              BRCryptoWallet wallet) {
    if (CRYPTO_FALSE == cryptoWalletManagerHasWallet (cwm, wallet))
        array_add (cwm->wallets, cryptoWalletTake (wallet));
}

private_extern void
cryptoWalletManagerRemWallet (BRCryptoWalletManager cwm,
                              BRCryptoWallet wallet) {
    for (size_t index = 0; index < array_count (cwm->wallets); index++)
        if (CRYPTO_TRUE == cryptoWalletEqual(cwm->wallets[index], wallet)) {
            array_rm (cwm->wallets, index);
            cryptoWalletGive (wallet);
            return;
        }
}

/// MARK: - Connect/Disconnect/Sync

extern void
cryptoWalletManagerConnect (BRCryptoWalletManager cwm) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            BRWalletManagerConnect (cwm->u.btc);
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            ewmConnect (cwm->u.eth);
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            break;
    }
}

extern void
cryptoWalletManagerDisconnect (BRCryptoWalletManager cwm) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            BRWalletManagerDisconnect (cwm->u.btc);
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            ewmDisconnect (cwm->u.eth);
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            break;
    }
}

extern void
cryptoWalletManagerSync (BRCryptoWalletManager cwm) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            BRWalletManagerScan (cwm->u.btc);
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            ewmSync (cwm->u.eth);
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            break;
    }
}

extern void
cryptoWalletManagerSubmit (BRCryptoWalletManager cwm,
                           BRCryptoWallet wallet,
                           BRCryptoTransfer transfer,
                           const char *paperKey) {
    UInt512 seed = cryptoAccountDeriveSeed(paperKey);

    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            BRWalletSignTransaction (cryptoWalletAsBTC(wallet),
                                     cryptoTransferAsBTC(transfer),
                                     &seed,
                                     sizeof (seed));

            BRPeerManagerPublishTx (BRWalletManagerGetPeerManager(cwm->u.btc),
                                    cryptoTransferAsBTC(transfer),
                                    cwm,
                                    cwmPublishTransactionAsBTC);
            break;

        case BLOCK_CHAIN_TYPE_ETH:
            ewmWalletSignTransferWithPaperKey (cwm->u.eth,
                                               cryptoWalletAsETH (wallet),
                                               cryptoTransferAsETH (transfer),
                                               paperKey);

            ewmWalletSubmitTransfer (cwm->u.eth,
                                     cryptoWalletAsETH (wallet),
                                     cryptoTransferAsETH (transfer));
            break;

        case BLOCK_CHAIN_TYPE_GEN:
            break;

    }
}

private_extern BRCryptoBoolean
cryptoWalletManagerHasETH (BRCryptoWalletManager manager,
                           BREthereumEWM ewm) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_ETH == manager->type && ewm == manager->u.eth);
}

private_extern BRCryptoBoolean
cryptoWalletManagerHasBTC (BRCryptoWalletManager manager,
                           BRWalletManager bwm) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_BTC == manager->type && bwm == manager->u.btc);
}

private_extern BRCryptoWallet
cryptoWalletManagerFindWalletAsBTC (BRCryptoWalletManager cwm,
                                    BRWallet *btc) {
    for (size_t index = 0; index < array_count (cwm->wallets); index++)
        if (btc == cryptoWalletAsBTC (cwm->wallets[index]))
            return cryptoWalletTake (cwm->wallets[index]);
    return NULL;
}

private_extern BRCryptoWallet
cryptoWalletManagerFindWalletAsETH (BRCryptoWalletManager cwm,
                                    BREthereumWallet eth) {
    for (size_t index = 0; index < array_count (cwm->wallets); index++)
        if (eth == cryptoWalletAsETH (cwm->wallets[index]))
            return cryptoWalletTake (cwm->wallets[index]);
    return NULL;
}

