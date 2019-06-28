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
#include "BRCryptoBase.h"
#include "BRCryptoPrivate.h"
#include "BRCryptoWalletManager.h"
#include "BRCryptoWalletManagerClient.h"
#include "BRCryptoWalletManagerPrivate.h"

#include "bitcoin/BRWalletManager.h"
#include "ethereum/BREthereum.h"

static void
cryptoWalletManagerRelease (BRCryptoWalletManager cwm);

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoWalletManager, cryptoWalletManager)

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
            BRWalletManagerClient client = cryptoWalletManagerClientCreateBTCClient (cwm);

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
            BREthereumClient client = cryptoWalletManagerClientCreateETHClient (cwm);

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
#define GEN_DISPATCHER_PERIOD       (10)        // related to block proccessing

            BRGenericClient client = cryptoWalletManagerClientCreateGENClient (cwm);

            // Create CWM as 'GEN' based on the network's base currency.
            const char *type = cryptoNetworkGetCurrencyCode (network);


            cwm->u.gen = gwmCreate (client,
                                    type,
                                    cryptoAccountAsGEN (account, type),
                                    cryptoAccountGetTimestamp(account),
                                    cwmPath,
                                    GEN_DISPATCHER_PERIOD,
                                    cryptoNetworkGetHeight(network));

            // Announce the new wallet manager;
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                                                          CRYPTO_WALLET_MANAGER_EVENT_CREATED
                                                      });

            // Create the primary wallet...
            BRCryptoCurrency currency = cryptoNetworkGetCurrency (cwm->network);
            BRCryptoUnit     unit     = cryptoNetworkGetUnitAsDefault (cwm->network, currency);

            cwm->wallet = cryptoWalletCreateAsGEN (unit, unit, cwm->u.gen, gwmCreatePrimaryWallet (cwm->u.gen));

            cryptoUnitGive(unit);
            cryptoCurrencyGive(currency);

            // ... and announce the created wallet.
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (cwm->wallet),
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_CREATED
                                               });

            // Add the primary wallet to the wallet manager...
            cryptoWalletManagerAddWallet (cwm, cwm->wallet);

            // ... and announce the manager's new wallet.
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                                                          CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                          { .wallet = { cryptoWalletTake (cwm->wallet) }}
                                                      });

            // Load transfers from persistent storage
            BRArrayOf(BRGenericTransfer) transfers = gwmLoadTransfers (cwm->u.gen);
            for (size_t index = 0; index < array_count (transfers); index++) {
                cryptoWalletManagerHandleTransferGEN (cwm, transfers[index]);
            }

            // Having added the transfers, get the wallet balance...
            BRCryptoAmount balance = cryptoWalletGetBalance (cwm->wallet);

            // ... and announce the balance
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (cwm->wallet),
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_BALANCE_UPDATED,
                                                   { .balanceUpdated = { balance }}
                                               });
            
            break;
        }
    }

    // NOTE: Race on cwm->u.{btc,eth} is resolved in the event handlers


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

    // TODO: btc, eth, gen
    
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
            gwmConnect(cwm->u.gen);
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
            gwmDisconnect (cwm->u.gen);
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
            gwmSync(cwm->u.gen);
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

            // TODO(fix): What should be used as the callback here?
            BRPeerManagerPublishTx (BRWalletManagerGetPeerManager(cwm->u.btc),
                                    cryptoTransferAsBTC(transfer),
                                    NULL,
                                    NULL);
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
            gwmWalletSubmitTransfer (cwm->u.gen,
                                     cryptoWalletAsGEN (wallet),
                                     cryptoTransferAsGEN (transfer),
                                     seed);
            break;

    }
}

private_extern BRWalletManager
cryptoWalletManagerAsBTC (BRCryptoWalletManager manager) {
    assert (BLOCK_CHAIN_TYPE_BTC == manager->type);
    return manager->u.btc;
}

private_extern BREthereumEWM
cryptoWalletManagerAsETH (BRCryptoWalletManager manager) {
    assert (BLOCK_CHAIN_TYPE_ETH == manager->type);
    return manager->u.eth;
}

private_extern BRCryptoBoolean
cryptoWalletManagerHasBTC (BRCryptoWalletManager manager,
                           BRWalletManager bwm) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_BTC == manager->type && bwm == manager->u.btc);
}

private_extern BRCryptoBoolean
cryptoWalletManagerHasETH (BRCryptoWalletManager manager,
                           BREthereumEWM ewm) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_ETH == manager->type && ewm == manager->u.eth);
}

private_extern BRCryptoBoolean
cryptoWalletManagerHasGEN (BRCryptoWalletManager manager,
                           BRGenericWalletManager gwm) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_GEN == manager->type && gwm == manager->u.gen);
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

private_extern BRCryptoWallet
cryptoWalletManagerFindWalletAsGEN (BRCryptoWalletManager cwm,
                                    BRGenericWallet gen) {
    for (size_t index = 0; index < array_count (cwm->wallets); index++)
        if (gen == cryptoWalletAsGEN (cwm->wallets[index]))
            return cryptoWalletTake (cwm->wallets[index]);
    return NULL;
}

extern void
cryptoWalletManagerHandleTransferGEN (BRCryptoWalletManager cwm,
                                      BRGenericTransfer transferGeneric) {

    // TODO: Determine the currency from `transferGeneric`
    BRCryptoCurrency currency = cryptoNetworkGetCurrency (cwm->network);

    // Create the generic transfers...
    BRCryptoTransfer transfer = cryptoTransferCreateAsGEN (currency, cwm->u.gen, transferGeneric);

    // TODO: Determine the wallet from transfer/transferGeneric
    BRCryptoWallet   wallet   = cwm->wallet;

    // .. and announce the newly created transfer.
    cwm->listener.transferEventCallback (cwm->listener.context,
                                         cryptoWalletManagerTake (cwm),
                                         cryptoWalletTake (wallet),
                                         cryptoTransferTake(transfer),
                                         (BRCryptoTransferEvent) {
                                             CRYPTO_TRANSFER_EVENT_CREATED
                                         });

    // Add the restored transfer to its wallet...
    cryptoWalletAddTransfer (cwm->wallet, transfer);

    // ... and announce the wallet's newly added transfer
    cwm->listener.walletEventCallback (cwm->listener.context,
                                       cryptoWalletManagerTake (cwm),
                                       cryptoWalletTake (wallet),
                                       (BRCryptoWalletEvent) {
                                           CRYPTO_WALLET_EVENT_TRANSFER_ADDED,
                                           { .transfer = { transfer }}
                                       });

}

