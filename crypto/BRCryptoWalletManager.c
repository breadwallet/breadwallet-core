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

/// MARK: - Forward Declarations

static void
cwmWalletManagerEventCallbackAsBTC (BRWalletManagerClientContext context,
                                    BRWalletManager manager,
                                    BRWalletManagerEvent event);

static void
cwmWalletEventCallbackAsBTC (BRWalletManagerClientContext context,
                             BRWalletManager manager,
                             BRWallet *wallet,
                             BRWalletEvent event);

static void
cwmTransactionEventCallbackAsBTC (BRWalletManagerClientContext context,
                                  BRWalletManager manager,
                                  BRWallet *wallet,
                                  BRTransaction *transaction,
                                  BRTransactionEvent event);

static void
cwmWalletManagerEventCallbackAsETH (BREthereumClientContext context,
                                    BREthereumEWM ewm,
                                    // BREthereumWallet wid,
                                    // BREthereumTransaction tid,
                                    BREthereumEWMEvent event,
                                    BREthereumStatus status,
                                    const char *errorDescription);

static void
cwmWalletEventCallbackAsETH (BREthereumClientContext context,
                             BREthereumEWM ewm,
                             BREthereumWallet wid,
                             BREthereumWalletEvent event,
                             BREthereumStatus status,
                             const char *errorDescription);

static void
cwmTransactionEventCallbackAsETH (BREthereumClientContext context,
                                  BREthereumEWM ewm,
                                  BREthereumWallet wid,
                                  BREthereumTransfer tid,
                                  BREthereumTransferEvent event,
                                  BREthereumStatus status,
                                  const char *errorDescription);

/// MARK: - Wallet Manager

struct BRCryptoWalletManagerRecord {
    BRCryptoBlockChainType type;
    union {
        BRWalletManager btc;
        BREthereumEWM eth;
    } u;

    BRCryptoCWMListener listener;
    BRCryptoNetwork network;
    BRCryptoAccount account;
    BRCryptoSyncMode mode;

    /// The primary wallet
    BRCryptoWallet wallet;

    /// All wallets
    BRArrayOf(BRCryptoWallet) wallets;
    char *path;
};

static BRCryptoWalletManager
cryptoWalletManagerCreateInternal (BRCryptoCWMListener listener,
                                   BRCryptoAccount account,
                                   BRCryptoBlockChainType type,
                                   BRCryptoNetwork network,
                                   BRCryptoSyncMode mode,
                                   char *path) {
    BRCryptoWalletManager cwm = malloc (sizeof (struct BRCryptoWalletManagerRecord));

    cwm->type = type;
    cwm->listener = listener;
    cwm->network = network;
    cwm->account = account;
    cwm->mode = mode;
    cwm->path = path;

    cwm->wallet = NULL;
    array_new (cwm->wallets, 1);

    return cwm;
}

extern BRCryptoWalletManager
cryptoWalletManagerCreate (BRCryptoCWMListener listener,
                           BRCryptoAccount account,
                           BRCryptoNetwork network,
                           BRCryptoSyncMode mode,
                           const char *path) {
    // ?? extend path... with network-type : network-name
    // ?? done by ewmCreate()?
    
    char *cwmPath = strdup (path);

    BRCryptoCurrency currency   = cryptoNetworkGetCurrency (network);
    BRCryptoUnit     unit       = cryptoNetworkGetUnitAsDefault (network, currency);

    BRCryptoWalletManager  cwm  = cryptoWalletManagerCreateInternal (listener,
                                                                     account,
                                                                     cryptoNetworkGetBlockChainType (network),
                                                                     network,
                                                                     mode,
                                                                     cwmPath);

    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWalletManagerClient client = {
                cwm,
                cwmTransactionEventCallbackAsBTC,
                cwmWalletEventCallbackAsBTC,
                cwmWalletManagerEventCallbackAsBTC
            };

            cwm->u.btc = BRWalletManagerNew (client,
                                             cryptoAccountAsBTC (account),
                                             cryptoNetworkAsBTC (network),
                                             (uint32_t) cryptoAccountGetTimestamp(account),
                                             cwmPath);

            cwm->wallet = cryptoWalletCreateAsBTC (unit, unit, BRWalletManagerGetWallet (cwm->u.btc));
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumClient client = {
                cwm,
                NULL, // BREthereumClientHandlerGetBalance funcGetBalance;
                NULL, // BREthereumClientHandlerGetGasPrice funcGetGasPrice;
                NULL, // BREthereumClientHandlerEstimateGas funcEstimateGas;
                NULL, // BREthereumClientHandlerSubmitTransaction funcSubmitTransaction;
                NULL, // BREthereumClientHandlerGetTransactions funcGetTransactions; // announce one-by-one
                NULL, // BREthereumClientHandlerGetLogs funcGetLogs; // announce one-by-one
                NULL, // BREthereumClientHandlerGetBlocks funcGetBlocks;
                NULL, // BREthereumClientHandlerGetTokens funcGetTokens; // announce one-by-one
                NULL, // BREthereumClientHandlerGetBlockNumber funcGetBlockNumber;
                NULL, //BREthereumClientHandlerGetNonce funcGetNonce;

                // Events - Announce changes to entities that normally impact the UI.
                cwmWalletManagerEventCallbackAsETH,
                NULL, // BREthereumClientHandlerPeerEvent funcPeerEvent;
                cwmWalletEventCallbackAsETH,
                NULL, // BREthereumClientHandlerTokenEvent funcTokenEvent;
                //       BREthereumClientHandlerBlockEvent funcBlockEvent;
                cwmTransactionEventCallbackAsETH

            };

            cwm->u.eth = ewmCreate (cryptoNetworkAsETH(network),
                                    cryptoAccountAsETH(account),
                                    (BREthereumTimestamp) cryptoAccountGetTimestamp(account),
                                    (BREthereumMode) mode,
                                    client,
                                    cwmPath);

            cwm->wallet = cryptoWalletCreateAsETH (unit, unit, ewmGetWallet (cwm->u.eth));
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            break;
        }
    }

    array_add (cwm->wallets, cwm->wallet);

    listener.walletManagerEventCallback (listener.context, cwm);  // created
    listener.walletEventCallback (listener.context, cwm, cwm->wallet);

    return cwm;
}

extern BRCryptoNetwork
cryptoWalletManagerGetNetwork (BRCryptoWalletManager cwm) {
    return cwm->network;
}

extern BRCryptoAccount
cryptoWalletManagerGetAccount (BRCryptoWalletManager cwm) {
    return cwm->account;
}

extern BRCryptoSyncMode
cryptoWalletManagerGetMode (BRCryptoWalletManager cwm) {
    return cwm->mode;
}

extern const char *
cryptoWalletManagerGetPath (BRCryptoWalletManager cwm) {
    return cwm->path;
}

extern BRCryptoWallet
cryptoWalletManagerGetWallet (BRCryptoWalletManager cwm) {
    return cwm->wallet;
}

extern size_t
cryptoWalletManagerGetWalletsCount (BRCryptoWalletManager cwm) {
    return array_count (cwm->wallets);
}

extern BRCryptoWallet
cryptoWalletManagerGetWalletAtIndex (BRCryptoWalletManager cwm,
                                        size_t index) {
    return cwm->wallets[index];
}

//static ssize_t
//cryptoWalletManagerGetWalletsIndex (BRCryptoWalletManager cwm,
//                                    BRCryptoWallet wallet) {
//    for (ssize_t index = 0; index < array_count(cwm->wallets); index++)
//        if (wallet == cwm->wallets[index])
//            return index;
//    return -1;
//}

extern BRCryptoWallet
cryptoWalletManagerGetWalletForCurrency (BRCryptoWalletManager cwm,
                                         BRCryptoCurrency currency) {
    for (size_t index = 0; index < array_count(cwm->wallets); index++)
        if (currency == cryptoWalletGetCurrency (cwm->wallets[index]))
            return cwm->wallets[index];

    // There was no wallet.  Create one
//    BRCryptoNetwork  network = cryptoWalletManagerGetNetwork(cwm);
//    BRCryptoUnit     unit    = cryptoNetworkGetUnitAsDefault (network, currency);
    BRCryptoWallet   wallet  = NULL;

    switch (cwm->type) {

        case BLOCK_CHAIN_TYPE_BTC:
//            wallet = cryptoWalletCreateAsBTC (unit, <#BRWallet *btc#>)
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            // Lookup Ethereum Token
            // Lookup Ethereum Wallet - ewmGetWalletHoldingToken()
            // Find Currency + Unit

//            wallet = cryptoWalletCreateAsETH (unit, <#BREthereumWallet eth#>)
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            break;
    }


    return wallet;
}

/// MARK: - Connect/Disconnect/Sync

extern void
cryptoWalletManagerConnect (BRCryptoWalletManager cwm) {
    // Mode
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
    // Mode
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

/// MARK: - Callbacks BTC / ETH

static void
cwmWalletManagerEventCallbackAsBTC (BRWalletManagerClientContext context,
                                    BRWalletManager manager,
                                    BRWalletManagerEvent event) {
    BRCryptoWalletManager cwm = context;

    switch (event.type) {
        case BITCOIN_WALLET_MANAGER_CONNECTED:
            break;
        case BITCOIN_WALLET_MANAGER_DISCONNECTED:
            break;
        case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
            break;
        case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
            break;
    }

    cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                              cwm
                                              // event
                                              );
}

static void
cwmWalletEventCallbackAsBTC (BRWalletManagerClientContext context,
                             BRWalletManager btcManager,
                             BRWallet *btcWallet,
                             BRWalletEvent event) {
    BRCryptoWalletManager cwm = context;
    assert (BLOCK_CHAIN_TYPE_BTC == cwm->type);

    BRCryptoWallet wallet = NULL;

    // Find an existing wallet, if one exists.
    for (size_t index = 0; index < cryptoWalletManagerGetWalletsCount(cwm); index++)
        if (btcWallet == cryptoWalletAsBTC (cwm->wallets[index])) {
            wallet = cwm->wallets[index];
            break;
        }

    int needEvent = 1;

    switch (event.type) {
        case BITCOIN_WALLET_CREATED:
            needEvent = 0;
            break;
        case BITCOIN_WALLET_BALANCE_UPDATED:
            break;
        case BITCOIN_WALLET_DELETED:
            break;
    }

    if (needEvent) {
        assert (NULL != wallet);

        cwm->listener.walletEventCallback (cwm->listener.context,
                                           cwm,
                                           wallet
                                           // event
                                           );
    }
}

static void
cwmTransactionEventCallbackAsBTC (BRWalletManagerClientContext context,
                                  BRWalletManager manager,
                                  BRWallet *wallet,
                                  BRTransaction *transaction,
                                  BRTransactionEvent event) {
    BRCryptoWalletManager cwm = context;

    switch (event.type) {
        case BITCOIN_TRANSACTION_ADDED:
            break;
        case BITCOIN_TRANSACTION_UPDATED:
            break;
        case BITCOIN_TRANSACTION_DELETED:
            break;
    }

    cwm->listener.transferEventCallback (cwm->listener.context,
                                         cwm,
                                         NULL, // crypto wallet)
                                         NULL // crypto transfer
                                         // event
                                         );
}

static void
cwmWalletManagerEventCallbackAsETH (BREthereumClientContext context,
                                    BREthereumEWM ewm,
                                    // BREthereumWallet wid,
                                    // BREthereumTransaction tid,
                                    BREthereumEWMEvent event,
                                    BREthereumStatus status,
                                    const char *errorDescription) {
    BRCryptoWalletManager cwm = context;

    int needEvent = 1;

    switch (event) {
        case EWM_EVENT_CREATED:
            needEvent = 0;
            break;
        case EWM_EVENT_SYNC_STARTED:
            break;
        case EWM_EVENT_SYNC_CONTINUES:
            break;
        case EWM_EVENT_SYNC_STOPPED:
            break;
        case EWM_EVENT_NETWORK_UNAVAILABLE:
            break;
        case EWM_EVENT_DELETED:
            break;
    }

    if (needEvent) {
        cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                  cwm
                                                  // event
                                                  );
    }
}

static void
cwmWalletEventCallbackAsETH (BREthereumClientContext context,
                             BREthereumEWM ewm,
                             BREthereumWallet wid,
                             BREthereumWalletEvent event,
                             BREthereumStatus status,
                             const char *errorDescription) {
    BRCryptoWalletManager cwm = context;

    BRCryptoWallet wallet = NULL;

    // Find an existing wallet, if one exists.
    for (size_t index = 0; index < cryptoWalletManagerGetWalletsCount(cwm); index++)
        if (wid == cryptoWalletAsETH (cwm->wallets[index])) {
            wallet = cwm->wallets[index];
            break;
        }

    int needEvent = 1;

    switch (event) {
        case WALLET_EVENT_CREATED:
            needEvent = 0;
            if (wallet != cwm->wallet) {
                BREthereumToken token = ewmWalletGetToken (ewm, wid);
                assert (NULL != token);
                // Find the currency for token
                BRCryptoCurrency currency = cryptoNetworkGetCurrency (cwm->network);
                BRCryptoUnit     unit     = cryptoNetworkGetUnitAsDefault (cwm->network, currency);

                wallet = cryptoWalletCreateAsETH (unit, unit, wid);

                cwm->listener.walletEventCallback (cwm->listener.context,
                                                   cwm,
                                                   wallet
                                                   // created
                                                   );

                cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                          cwm
                                                          // wallet added
                                                          );
            }
            break;
        case WALLET_EVENT_BALANCE_UPDATED:
            break;
        case WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED:
            break;
        case WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED:
            break;
        case WALLET_EVENT_DELETED:
            break;
    }

    if (needEvent) {
        cwm->listener.walletEventCallback (cwm->listener.context,
                                           cwm,
                                           wallet // crypto wallet
                                           // event
                                           );
    }
}

static void
cwmTransactionEventCallbackAsETH (BREthereumClientContext context,
                                  BREthereumEWM ewm,
                                  BREthereumWallet wid,
                                  BREthereumTransfer tid,
                                  BREthereumTransferEvent event,
                                  BREthereumStatus status,
                                  const char *errorDescription) {
    BRCryptoWalletManager cwm = context;

    switch (event) {
        case TRANSFER_EVENT_CREATED:
            break;
        case TRANSFER_EVENT_SIGNED:
            break;
        case TRANSFER_EVENT_SUBMITTED:
            break;
        case TRANSFER_EVENT_INCLUDED:
            break;
        case TRANSFER_EVENT_ERRORED:
            break;
        case TRANSFER_EVENT_GAS_ESTIMATE_UPDATED:
            break;
        case TRANSFER_EVENT_BLOCK_CONFIRMATIONS_UPDATED:
            break;
        case TRANSFER_EVENT_DELETED:
            break;
    }
    
    cwm->listener.transferEventCallback (cwm->listener.context,
                                         cwm,
                                         NULL, // crypto wallet)
                                         NULL // crypto transfer
                                         // event
                                         );
}
