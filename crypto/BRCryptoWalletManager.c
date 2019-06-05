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

    /// The primary wallet
    BRCryptoWallet wallet;

    /// All wallets
    BRArrayOf(BRCryptoWallet) wallets;
    char *path;
};


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
                                 BRWalletManager manager,
                                 uint64_t begBlockNumber,
                                 uint64_t endBlockNumber,
                                 int rid) {
    BRCryptoWalletManager cwm = context;
    cwm->client.btc.funcGetTransactions (cwm->client.context, manager, begBlockNumber, endBlockNumber, rid);
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
                                    BRWalletManager manager,
                                    BRWalletManagerEvent event) {
    BRCryptoWalletManager cwm = context;

    switch (event.type) {
        case BITCOIN_WALLET_MANAGER_CREATED:
            break;
        case BITCOIN_WALLET_MANAGER_CONNECTED:
            break;
        case BITCOIN_WALLET_MANAGER_DISCONNECTED:
            break;
        case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
            break;
        case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
            break;
        case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
            break;
    }

    (void) cwm;
    //    cwm->listener.walletManagerEventCallback (cwm->listener.context,
    //                                              cwm
    //                                              // event
    //                                              );
}

static void
cwmWalletEventAsBTC (BRWalletManagerClientContext context,
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
        case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
            break;
        case BITCOIN_WALLET_DELETED:
            break;
    }

    if (needEvent) {
        assert (NULL != wallet);

        //        cwm->listener.walletEventCallback (cwm->listener.context,
        //                                           cwm,
        //                                           wallet
        //                                           // event
        //                                           );
    }
}

static void
cwmTransactionEventAsBTC (BRWalletManagerClientContext context,
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

    (void) cwm;

    //    cwm->listener.transferEventCallback (cwm->listener.context,
    //                                         cwm,
    //                                         NULL, // crypto wallet)
    //                                         NULL // crypto transfer
    //                                         // event
    //                                         );
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
        case EWM_EVENT_BLOCK_HEIGHT_UPDATED:
            break;
        case EWM_EVENT_DELETED:
            break;
    }

    if (needEvent) {
        (void) cwm;

        //        cwm->listener.walletManagerEventCallback (cwm->listener.context,
        //                                                  cwm
        //                                                  // event
        //                                                  );
    }
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

                wallet = cryptoWalletCreateAsETH (unit, unit, cwm->u.eth, wid);

                //                cwm->listener.walletEventCallback (cwm->listener.context,
                //                                                   cwm,
                //                                                   wallet
                //                                                   // created
                //                                                   );

                //                cwm->listener.walletManagerEventCallback (cwm->listener.context,
                //                                                          cwm
                //                                                          // wallet added
                //                                                          );
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
        //        cwm->listener.walletEventCallback (cwm->listener.context,
        //                                           cwm,
        //                                           wallet // crypto wallet
        //                                           // event
        //                                           );
    }
}

static void
cwmEventTokenAsETH (BREthereumClientContext context,
                  BREthereumEWM ewm,
                  BREthereumToken token,
                  BREthereumTokenEvent event) {

    BRCryptoWalletManager cwm = context;
    (void) cwm;
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
        case TRANSFER_EVENT_DELETED:
            break;
    }

    (void) cwm;

    //    cwm->listener.transferEventCallback (cwm->listener.context,
    //                                         cwm,
    //                                         NULL, // crypto wallet)
    //                                         NULL // crypto transfer
    //                                         // event
    //                                         );
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
static BRCryptoWalletManager
cryptoWalletManagerCreateInternal (BRCryptoCWMListener listener,
                                   BRCryptoCWMClient client,
                                   BRCryptoAccount account,
                                   BRCryptoBlockChainType type,
                                   BRCryptoNetwork network,
                                   BRSyncMode mode,
                                   char *path) {
    BRCryptoWalletManager cwm = malloc (sizeof (struct BRCryptoWalletManagerRecord));

    cwm->type = type;
    cwm->listener = listener;
    cwm->client  = client;
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
                           BRCryptoCWMClient client,
                           BRCryptoAccount account,
                           BRCryptoNetwork network,
                           BRSyncMode mode,
                           const char *path) {
    // ?? extend path... with network-type : network-name
    // ?? done by ewmCreate()?
    
    char *cwmPath = strdup (path);

    BRCryptoCurrency currency   = cryptoNetworkGetCurrency (network);
    BRCryptoUnit     unit       = cryptoNetworkGetUnitAsDefault (network, currency);

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

            cwm->u.btc = BRWalletManagerNew (client,
                                             cryptoAccountAsBTC (account),
                                             cryptoNetworkAsBTC (network),
                                             (uint32_t) cryptoAccountGetTimestamp(account),
                                             mode,
                                             cwmPath);

            cwm->wallet = cryptoWalletCreateAsBTC (unit, unit, cwm->u.btc, BRWalletManagerGetWallet (cwm->u.btc));
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

            cwm->u.eth = ewmCreate (cryptoNetworkAsETH(network),
                                    cryptoAccountAsETH(account),
                                    (BREthereumTimestamp) cryptoAccountGetTimestamp(account),
                                    (BREthereumMode) mode,
                                    client,
                                    cwmPath);

            cwm->wallet = cryptoWalletCreateAsETH (unit, unit, cwm->u.eth, ewmGetWallet (cwm->u.eth));
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            break;
        }
    }

    array_add (cwm->wallets, cwm->wallet);

    //    listener.walletManagerEventCallback (listener.context, cwm);  // created
    //    listener.walletEventCallback (listener.context, cwm, cwm->wallet);

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

extern BRSyncMode
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
