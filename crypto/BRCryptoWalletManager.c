//
//  BRCryptoWalletManager.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <assert.h>
#include <pthread.h>
#include <arpa/inet.h>      // struct in_addr

#include "BRCryptoBase.h"
#include "BRCryptoKey.h"
#include "BRCryptoPrivate.h"
#include "BRCryptoWalletManager.h"
#include "BRCryptoWalletManagerClient.h"
#include "BRCryptoWalletManagerPrivate.h"

#include "bitcoin/BRWalletManager.h"
#include "ethereum/BREthereum.h"

static void
cryptoWalletManagerRelease (BRCryptoWalletManager cwm);

static void
cryptoWalletManagerInstallETHTokensForCurrencies (BRCryptoWalletManager cwm);

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoWalletManager, cryptoWalletManager)

/// =============================================================================================
///
/// MARK: - Wallet Manager
///
///

private_extern BRCryptoWalletManagerState
cryptoWalletManagerStateInit(BRCryptoWalletManagerStateType type) {
    switch (type) {
        case CRYPTO_WALLET_MANAGER_STATE_CREATED:
        case CRYPTO_WALLET_MANAGER_STATE_CONNECTED:
        case CRYPTO_WALLET_MANAGER_STATE_SYNCING:
        case CRYPTO_WALLET_MANAGER_STATE_DELETED:
            return (BRCryptoWalletManagerState) { type };
        case CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED:
            assert (0); // if you are hitting this, use cryptoWalletManagerStateDisconnectedInit!
            return (BRCryptoWalletManagerState) {
                CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED,
                { .disconnected = { BRDisconnectReasonUnknown() } }
            };
    }
}

private_extern BRCryptoWalletManagerState
cryptoWalletManagerStateDisconnectedInit(BRDisconnectReason reason) {
    return (BRCryptoWalletManagerState) {
        CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED,
        { .disconnected = { reason } }
    };
}

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
                                   BRCryptoAddressScheme scheme,
                                   char *path) {
    BRCryptoWalletManager cwm = calloc (1, sizeof (struct BRCryptoWalletManagerRecord));

    cwm->type = type;
    cwm->listener = listener;
    cwm->client  = client;
    cwm->network = cryptoNetworkTake (network);
    cwm->account = cryptoAccountTake (account);
    cwm->state   = cryptoWalletManagerStateInit (CRYPTO_WALLET_MANAGER_STATE_CREATED);
    cwm->addressScheme = scheme;
    cwm->path = strdup (path);

    cwm->wallet = NULL;
    array_new (cwm->wallets, 1);

    cwm->ref = CRYPTO_REF_ASSIGN (cryptoWalletManagerRelease);

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&cwm->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    return cwm;
}

extern BRCryptoWalletManager
cryptoWalletManagerCreate (BRCryptoCWMListener listener,
                           BRCryptoCWMClient client,
                           BRCryptoAccount account,
                           BRCryptoNetwork network,
                           BRSyncMode mode,
                           BRCryptoAddressScheme scheme,
                           const char *path) {

    // In rare cases a Wallet Manager cannot be created.  If not, we'll perform a 'goto' and, on
    // `1 == error`, perform some cleanup actions.
    int error = 0;

    // TODO: extend path... with network-type : network-name - or is that done by ewmCreate(), ...
    char *cwmPath = strdup (path);

    BRCryptoWalletManager  cwm  = cryptoWalletManagerCreateInternal (listener,
                                                                     client,
                                                                     account,
                                                                     cryptoNetworkGetBlockChainType (network),
                                                                     network,
                                                                     scheme,
                                                                     cwmPath);

    // Primary wallet currency and unit.
    BRCryptoCurrency currency = cryptoNetworkGetCurrency (cwm->network);
    BRCryptoUnit     unit     = cryptoNetworkGetUnitAsDefault (cwm->network, currency);

    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWalletManagerClient client = cryptoWalletManagerClientCreateBTCClient (cwm);

            // Create BWM - will also create the BWM primary wallet....
            cwm->u.btc = BRWalletManagerNew (client,
                                             cryptoAccountAsBTC (account),
                                             cryptoNetworkAsBTC (network),
                                             (uint32_t) cryptoAccountGetTimestamp(account),
                                             mode,
                                             cwmPath,
                                             cryptoNetworkGetHeight(network),
                                             cryptoNetworkGetConfirmationsUntilFinal (network));
            if (NULL == cwm->u.btc) { error = 1; break ; }

            // ... get the CWM primary wallet in place...
            cwm->wallet = cryptoWalletCreateAsBTC (unit, unit, cwm->u.btc, BRWalletManagerGetWallet (cwm->u.btc));

            // ... add the CWM primary wallet to CWM
            cryptoWalletManagerAddWallet (cwm, cwm->wallet);

            // ... and finally start the BWM event handling (with CWM fully in place).
            BRWalletManagerStart (cwm->u.btc);

            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumClient client = cryptoWalletManagerClientCreateETHClient (cwm);

            // Create EWM - will also create the EWM primary wallet....
            cwm->u.eth = ewmCreate (cryptoNetworkAsETH(network),
                                    cryptoAccountAsETH(account),
                                    (BREthereumTimestamp) cryptoAccountGetTimestamp(account),
                                    mode,
                                    client,
                                    cwmPath,
                                    cryptoNetworkGetHeight(network),
                                    cryptoNetworkGetConfirmationsUntilFinal (network));
            if (NULL == cwm->u.eth) { error = 1; break; }

            // ... get the CWM primary wallet in place...
            cwm->wallet = cryptoWalletCreateAsETH (unit, unit, cwm->u.eth, ewmGetWallet(cwm->u.eth));

            // ... add the CWM primary wallet to CWM
            cryptoWalletManagerAddWallet (cwm, cwm->wallet);

            // ... and finally start the EWM event handling (with CWM fully in place).
            ewmStart (cwm->u.eth);

            // This will install ERC20 Tokens for the CWM Currencies.  Corresponding Wallets are
            // not created for these currencies.
            cryptoWalletManagerInstallETHTokensForCurrencies(cwm);

            // We finish here with possibly EWM events in the EWM handler queue and/or with
            // CWM events in the CWM handler queue.

            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
#define GEN_DISPATCHER_PERIOD       (10)        // related to block proccessing

            pthread_mutex_lock (&cwm->lock);
            BRGenericClient client = cryptoWalletManagerClientCreateGENClient (cwm);

            // Create CWM as 'GEN' based on the network's base currency.
            const char *type = cryptoNetworkGetCurrencyCode (network);

            cwm->u.gen = genManagerCreate (client,
                                           type,
                                           cryptoNetworkAsGEN (network),
                                           cryptoAccountAsGEN (account, type),
                                           cryptoAccountGetTimestamp(account),
                                           cwmPath,
                                           GEN_DISPATCHER_PERIOD,
                                           cryptoNetworkGetHeight(network));
            if (NULL == cwm->u.gen) {
                pthread_mutex_unlock (&cwm->lock);
                error = 1;
                break; }

            // ... and create the primary wallet
            cwm->wallet = cryptoWalletCreateAsGEN (unit, unit, cwm->u.gen, genManagerCreatePrimaryWallet (cwm->u.gen));

            // ... and add the primary wallet to the wallet manager...
            cryptoWalletManagerAddWallet (cwm, cwm->wallet);

            pthread_mutex_unlock (&cwm->lock);

            // Announce the new wallet manager;
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                                                          CRYPTO_WALLET_MANAGER_EVENT_CREATED
                                                      });

            // ... and announce the created wallet.
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (cwm->wallet),
                                               (BRCryptoWalletEvent) {
                                                   CRYPTO_WALLET_EVENT_CREATED
                                               });

            // ... and announce the manager's new wallet.
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                                                          CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED,
                                                          { .wallet = { cryptoWalletTake (cwm->wallet) }}
                                                      });
            pthread_mutex_lock (&cwm->lock);

            // Load transfers from persistent storage
            BRArrayOf(BRGenericTransfer) transfers = genManagerLoadTransfers (cwm->u.gen);
            for (size_t index = 0; index < array_count (transfers); index++) {
                // TODO: A BRGenericTransfer must allow us to determine the Wallet (via a Currency).
                cryptoWalletManagerHandleTransferGEN (cwm, transfers[index]);
            }

            // Having added the transfers, get the wallet balance...
            BRCryptoAmount balance = cryptoWalletGetBalance (cwm->wallet);
            pthread_mutex_unlock (&cwm->lock);

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

    if (error) {
        cryptoWalletManagerGive (cwm);
        cwm = NULL;
    }

    cryptoUnitGive(unit);
    cryptoCurrencyGive(currency);

    free (cwmPath);

    return cwm;
}

static void
cryptoWalletManagerRelease (BRCryptoWalletManager cwm) {
    // Ensure CWM is stopped...
    cryptoWalletManagerStop (cwm);

    // ... then release memory.
    cryptoAccountGive (cwm->account);
    cryptoNetworkGive (cwm->network);
    if (NULL != cwm->wallet) cryptoWalletGive (cwm->wallet);

    for (size_t index = 0; index < array_count(cwm->wallets); index++)
        cryptoWalletGive (cwm->wallets[index]);
    array_free (cwm->wallets);

    // Release the specific cwm type, if it exists.
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            if (NULL != cwm->u.btc)
                BRWalletManagerFree (cwm->u.btc);
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            if (NULL != cwm->u.eth)
                ewmDestroy (cwm->u.eth);
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            if (NULL != cwm->u.gen)
                genManagerRelease (cwm->u.gen);
            break;
    }

    free (cwm->path);

    pthread_mutex_destroy (&cwm->lock);

    memset (cwm, 0, sizeof(*cwm));
    free (cwm);
}

private_extern void
cryptoWalletManagerStop (BRCryptoWalletManager cwm) {
    // Stop the specific cwm type, if it exists.
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            if (NULL != cwm->u.btc)
                BRWalletManagerStop (cwm->u.btc);
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            if (NULL != cwm->u.eth)
                ewmStop (cwm->u.eth);
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            if (NULL != cwm->u.gen)
                genManagerStop (cwm->u.gen);
            break;
    }
}

extern BRCryptoNetwork
cryptoWalletManagerGetNetwork (BRCryptoWalletManager cwm) {
    return cryptoNetworkTake (cwm->network);
}

extern BRCryptoAccount
cryptoWalletManagerGetAccount (BRCryptoWalletManager cwm) {
    return cryptoAccountTake (cwm->account);
}

extern void
cryptoWalletManagerSetMode (BRCryptoWalletManager cwm, BRSyncMode mode) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            BRWalletManagerSetMode (cwm->u.btc, mode);
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            ewmUpdateMode (cwm->u.eth, mode);
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            assert (SYNC_MODE_BRD_ONLY == mode);
            break;
        default:
            assert (0);
            break;
    }
}

extern BRSyncMode
cryptoWalletManagerGetMode (BRCryptoWalletManager cwm) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
           return BRWalletManagerGetMode (cwm->u.btc);
        case BLOCK_CHAIN_TYPE_ETH:
            return ewmGetMode (cwm->u.eth);
        case BLOCK_CHAIN_TYPE_GEN:
            return SYNC_MODE_BRD_ONLY;
        default:
            assert (0);
            return SYNC_MODE_BRD_ONLY;

    }
}

extern BRCryptoWalletManagerState
cryptoWalletManagerGetState (BRCryptoWalletManager cwm) {
    pthread_mutex_lock (&cwm->lock);
    BRCryptoWalletManagerState state = cwm->state;
    pthread_mutex_unlock (&cwm->lock);
    return state;
}

private_extern void
cryptoWalletManagerSetState (BRCryptoWalletManager cwm,
                             BRCryptoWalletManagerState state) {
    pthread_mutex_lock (&cwm->lock);
    cwm->state = state;
    pthread_mutex_unlock (&cwm->lock);
}

extern BRCryptoAddressScheme
cryptoWalletManagerGetAddressScheme (BRCryptoWalletManager cwm) {
    pthread_mutex_lock (&cwm->lock);
    BRCryptoAddressScheme scheme = cwm->addressScheme;
    pthread_mutex_unlock (&cwm->lock);
    return scheme;
}

extern void
cryptoWalletManagerSetAddressScheme (BRCryptoWalletManager cwm,
                                     BRCryptoAddressScheme scheme) {
    pthread_mutex_lock (&cwm->lock);
    cwm->addressScheme = scheme;
    pthread_mutex_unlock (&cwm->lock);
}

extern const char *
cryptoWalletManagerGetPath (BRCryptoWalletManager cwm) {
    return cwm->path;
}

extern void
cryptoWalletManagerSetNetworkReachable (BRCryptoWalletManager cwm,
                                        BRCryptoBoolean isNetworkReachable) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            BRWalletManagerSetNetworkReachable (cwm->u.btc, isNetworkReachable);
            break;
        default:
            break;
    }
}

//extern BRCryptoPeer
//cryptoWalletManagerGetPeer (BRCryptoWalletManager cwm) {
//    return (NULL == cwm->peer ? NULL : cryptoPeerTake (cwm->peer));
//}
//
//extern void
//cryptoWalletManagerSetPeer (BRCryptoWalletManager cwm,
//                            BRCryptoPeer peer) {
//    BRCryptoPeer oldPeer = cwm->peer;
//    cwm->peer = (NULL == peer ? NULL : cryptoPeerTake(peer));
//    if (NULL != oldPeer) cryptoPeerGive (oldPeer);
//}

extern BRCryptoWallet
cryptoWalletManagerGetWallet (BRCryptoWalletManager cwm) {
    return cryptoWalletTake (cwm->wallet);
}

extern BRCryptoWallet *
cryptoWalletManagerGetWallets (BRCryptoWalletManager cwm, size_t *count) {
    pthread_mutex_lock (&cwm->lock);
    *count = array_count (cwm->wallets);
    BRCryptoWallet *wallets = NULL;
    if (0 != *count) {
        wallets = calloc (*count, sizeof(BRCryptoWallet));
        for (size_t index = 0; index < *count; index++) {
            wallets[index] = cryptoWalletTake(cwm->wallets[index]);
        }
    }
    pthread_mutex_unlock (&cwm->lock);
    return wallets;
}


extern BRCryptoWallet
cryptoWalletManagerGetWalletForCurrency (BRCryptoWalletManager cwm,
                                         BRCryptoCurrency currency) {
    BRCryptoWallet wallet = NULL;
    pthread_mutex_lock (&cwm->lock);
    for (size_t index = 0; index < array_count(cwm->wallets); index++) {
        if (currency == cryptoWalletGetCurrency (cwm->wallets[index])) {
            wallet = cryptoWalletTake (cwm->wallets[index]);
            break;
        }
    }
    pthread_mutex_unlock (&cwm->lock);
    return wallet;
}

extern BRCryptoWallet
cryptoWalletManagerRegisterWallet (BRCryptoWalletManager cwm,
                                   BRCryptoCurrency currency) {
    BRCryptoWallet wallet = cryptoWalletManagerGetWalletForCurrency (cwm, currency);
    if (NULL == wallet) {
        switch (cwm->type) {
            case BLOCK_CHAIN_TYPE_BTC:
                assert (0); // Only BTC currency; has `primaryWallet
                break;

            case BLOCK_CHAIN_TYPE_ETH: {
                const char *issuer = cryptoCurrencyGetIssuer (currency);
                BREthereumAddress ethAddress = addressCreate (issuer);
                BREthereumToken ethToken = ewmLookupToken (cwm->u.eth, ethAddress);
                assert (NULL != ethToken);
                ewmGetWalletHoldingToken (cwm->u.eth, ethToken);
                break;
            }
            case BLOCK_CHAIN_TYPE_GEN:
                assert (0);
                break;
        }
    }
    return wallet;
}

extern BRCryptoBoolean
cryptoWalletManagerHasWallet (BRCryptoWalletManager cwm,
                              BRCryptoWallet wallet) {
    BRCryptoBoolean r = CRYPTO_FALSE;
    pthread_mutex_lock (&cwm->lock);
    for (size_t index = 0; index < array_count (cwm->wallets) && CRYPTO_FALSE == r; index++) {
        r = cryptoWalletEqual(cwm->wallets[index], wallet);
    }
    pthread_mutex_unlock (&cwm->lock);
    return r;
}

private_extern void
cryptoWalletManagerAddWallet (BRCryptoWalletManager cwm,
                              BRCryptoWallet wallet) {
    pthread_mutex_lock (&cwm->lock);
    if (CRYPTO_FALSE == cryptoWalletManagerHasWallet (cwm, wallet)) {
        array_add (cwm->wallets, cryptoWalletTake (wallet));
    }
    pthread_mutex_unlock (&cwm->lock);
}

private_extern void
cryptoWalletManagerRemWallet (BRCryptoWalletManager cwm,
                              BRCryptoWallet wallet) {

    BRCryptoWallet managerWallet = NULL;
    pthread_mutex_lock (&cwm->lock);
    for (size_t index = 0; index < array_count (cwm->wallets); index++) {
        if (CRYPTO_TRUE == cryptoWalletEqual(cwm->wallets[index], wallet)) {
            managerWallet = cwm->wallets[index];
            array_rm (cwm->wallets, index);
            break;
        }
    }
    pthread_mutex_unlock (&cwm->lock);

    // drop reference outside of lock to avoid potential case where release function runs
    if (NULL != managerWallet) cryptoWalletGive (managerWallet);
}

static void
cryptoWalletManagerInstallETHTokensForCurrencies (BRCryptoWalletManager cwm) {
    BRCryptoNetwork  network    = cryptoNetworkTake (cwm->network);
    BRCryptoCurrency currency   = cryptoNetworkGetCurrency(network);
    BRCryptoUnit     unitForFee = cryptoNetworkGetUnitAsBase (network, currency);

    size_t currencyCount = cryptoNetworkGetCurrencyCount (network);
    for (size_t index = 0; index < currencyCount; index++) {
        BRCryptoCurrency c = cryptoNetworkGetCurrencyAt (network, index);
        if (c != currency) {
            BRCryptoUnit unitDefault = cryptoNetworkGetUnitAsDefault (network, c);

            switch (cwm->type) {
                case BLOCK_CHAIN_TYPE_BTC:
                    break;
                case BLOCK_CHAIN_TYPE_ETH: {
                    const char *address = cryptoCurrencyGetIssuer(c);
                    if (NULL != address) {
                        BREthereumGas      ethGasLimit = gasCreate(TOKEN_BRD_DEFAULT_GAS_LIMIT);
                        BREthereumGasPrice ethGasPrice = gasPriceCreate(etherCreate(createUInt256(TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64)));

                        // This has the perhaps surprising side-effect of updating the properties
                        // of an existing token.  That is, `address` is used to locate a token and
                        // if found it is updated.  Either created or updated the token will be
                        // persistently saved.
                        //
                        // Argubably EWM should create a wallet for the token.  But, it doesn't.
                        // So we'll call `ewmGetWalletHoldingToken()` to get a wallet.

                        BREthereumToken token = ewmCreateToken (cwm->u.eth,
                                                                address,
                                                                cryptoCurrencyGetCode (c),
                                                                cryptoCurrencyGetName(c),
                                                                cryptoCurrencyGetUids(c), // description
                                                                cryptoUnitGetBaseDecimalOffset(unitDefault),
                                                                ethGasLimit,
                                                                ethGasPrice);
                        assert (NULL != token); (void) &token;
                    }
                    break;
                }
                case BLOCK_CHAIN_TYPE_GEN:
                    break;
            }
            cryptoUnitGive(unitDefault);
        }
        cryptoCurrencyGive(c);
    }
    cryptoUnitGive(unitForFee);
    cryptoCurrencyGive(currency);
    cryptoNetworkGive(network);
}

/// MARK: - Connect/Disconnect/Sync

extern void
cryptoWalletManagerConnect (BRCryptoWalletManager cwm,
                            BRCryptoPeer peer) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            // Assume `peer` is NULL; UINT128_ZERO will restore BRPeerManager peer discovery
            UInt128  address = UINT128_ZERO;
            uint16_t port    = 0;

            if (NULL != peer) {
                address = cryptoPeerGetAddrAsInt(peer);
                port = cryptoPeerGetPort (peer);
            }

            // Calling `SetFixedPeer` will 100% disconnect.  We could avoid calling SetFixedPeer
            // if we kept a reference to `peer` and checked if it differs.
            BRWalletManagerSetFixedPeer (cwm->u.btc, address, port);
            BRWalletManagerConnect (cwm->u.btc);
            break;
        }
        case BLOCK_CHAIN_TYPE_ETH:
            ewmConnect (cwm->u.eth);
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            genManagerConnect(cwm->u.gen);
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
            genManagerDisconnect (cwm->u.gen);
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
            ewmSync (cwm->u.eth, ETHEREUM_BOOLEAN_FALSE);
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            genManagerSync(cwm->u.gen);
            break;
    }
}

extern void
cryptoWalletManagerSyncToDepth (BRCryptoWalletManager cwm,
                                BRSyncDepth depth) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            BRWalletManagerScanToDepth (cwm->u.btc, depth);
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            ewmSyncToDepth (cwm->u.eth, ETHEREUM_BOOLEAN_FALSE, depth);
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            // TODO(fix): Implement this
            assert (0);
            break;
    }
}

static BRCryptoTransferState
cryptoTransferStateCreateGEN (BRGenericTransferState generic,
                              BRCryptoUnit feeUnit) { // feeUnit already taken
    switch (generic.type) {
        case GENERIC_TRANSFER_STATE_CREATED:
            return cryptoTransferStateInit(CRYPTO_TRANSFER_STATE_CREATED);
        case GENERIC_TRANSFER_STATE_SIGNED:
            return cryptoTransferStateInit(CRYPTO_TRANSFER_STATE_SIGNED);
        case GENERIC_TRANSFER_STATE_SUBMITTED:
            return cryptoTransferStateInit(CRYPTO_TRANSFER_STATE_SUBMITTED);
        case GENERIC_TRANSFER_STATE_INCLUDED:
            return cryptoTransferStateIncludedInit (generic.u.included.blockNumber,
                                                    generic.u.included.transactionIndex,
                                                    generic.u.included.timestamp,
                                                    cryptoAmountCreateInternal (feeUnit,
                                                                                CRYPTO_FALSE,
                                                                                generic.u.included.fee,
                                                                                0));
        case GENERIC_TRANSFER_STATE_ERRORED:
            return cryptoTransferStateErroredInit (BRTransferSubmitErrorUnknown());
        case GENERIC_TRANSFER_STATE_DELETED:
            return cryptoTransferStateInit(CRYPTO_TRANSFER_STATE_SIGNED);
    }
}

private_extern void
cryptoWalletManagerSetTransferStateGEN (BRCryptoWalletManager cwm,
                                        BRCryptoWallet wallet,
                                        BRCryptoTransfer transfer,
                                        BRGenericTransferState newGenericState) {
    pthread_mutex_lock (&cwm->lock);

    BRGenericTransfer      genericTransfer = cryptoTransferAsGEN (transfer);
    BRGenericTransferState oldGenericState = genTransferGetState (genericTransfer);

    if (!genTransferStateEqual (oldGenericState, newGenericState)) {
        BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
        BRCryptoTransferState newState = cryptoTransferStateCreateGEN (newGenericState,
                                                                       cryptoTransferGetUnitForFee(transfer));

        pthread_mutex_unlock (&cwm->lock);
        cwm->listener.transferEventCallback (cwm->listener.context,
                                             cryptoWalletManagerTake (cwm),
                                             cryptoWalletTake (wallet),
                                             cryptoTransferTake(transfer),
                                             (BRCryptoTransferEvent) {
            CRYPTO_TRANSFER_EVENT_CHANGED,
            { .state = {
                cryptoTransferStateCopy (&oldState),
                cryptoTransferStateCopy (&newState) }}
        });
        pthread_mutex_lock (&cwm->lock);

        genTransferSetState (genericTransfer, newGenericState);
        cryptoTransferSetState (transfer, newState);

        cryptoTransferStateRelease (&oldState);
        cryptoTransferStateRelease (&newState);
    }
    pthread_mutex_unlock (&cwm->lock);
}

extern BRCryptoBoolean
cryptoWalletManagerSign (BRCryptoWalletManager cwm,
                         BRCryptoWallet wallet,
                         BRCryptoTransfer transfer,
                         const char *paperKey) {
    BRCryptoBoolean success = CRYPTO_FALSE;

    // Derived the seed used for signing.
    UInt512 seed = cryptoAccountDeriveSeed(paperKey);

    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            success = AS_CRYPTO_BOOLEAN (BRWalletManagerSignTransaction (cwm->u.btc,
                                                                         cryptoWalletAsBTC (wallet),
                                                                         cryptoTransferAsBTC(transfer),
                                                                         &seed,
                                                                         sizeof (seed)));
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH:
            // TODO(fix): ewmWalletSignTransferWithPaperKey() doesn't return a status
            break;

        case BLOCK_CHAIN_TYPE_GEN:
            success = AS_CRYPTO_BOOLEAN (genManagerSignTransfer (cwm->u.gen,
                                                                 cryptoWalletAsGEN (wallet),
                                                                 cryptoTransferAsGEN (transfer),
                                                                 seed));
            if (CRYPTO_TRUE == success)
                cryptoWalletManagerSetTransferStateGEN (cwm, wallet, transfer,
                                                        genTransferStateCreateOther (GENERIC_TRANSFER_STATE_SIGNED));
            break;
    }

    // Zero-out the seed.
    seed = UINT512_ZERO;

    return success;
}

extern void
cryptoWalletManagerSubmit (BRCryptoWalletManager cwm,
                           BRCryptoWallet wallet,
                           BRCryptoTransfer transfer,
                           const char *paperKey) {

    // Derive the seed used for signing
    UInt512 seed = cryptoAccountDeriveSeed(paperKey);

    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC: {

            if (BRWalletManagerSignTransaction (cwm->u.btc,
                                                cryptoWalletAsBTC (wallet),
                                                cryptoTransferAsBTC(transfer),
                                                &seed,
                                                sizeof (seed))) {
                BRWalletManagerSubmitTransaction (cwm->u.btc,
                                                  cryptoWalletAsBTC (wallet),
                                                  cryptoTransferAsBTC(transfer));
            }
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            ewmWalletSignTransferWithPaperKey (cwm->u.eth,
                                               cryptoWalletAsETH (wallet),
                                               cryptoTransferAsETH (transfer),
                                               paperKey);

            ewmWalletSubmitTransfer (cwm->u.eth,
                                     cryptoWalletAsETH (wallet),
                                     cryptoTransferAsETH (transfer));
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWallet genWallet = cryptoWalletAsGEN (wallet);
            BRGenericTransfer genTransfer = cryptoTransferAsGEN (transfer);

            // Sign the transfer
            if (genManagerSignTransfer (cwm->u.gen, genWallet, genTransfer, seed)) {
                cryptoWalletManagerSetTransferStateGEN (cwm, wallet, transfer,
                                                        genTransferStateCreateOther (GENERIC_TRANSFER_STATE_SIGNED));
                // Submit the transfer
                genManagerSubmitTransfer (cwm->u.gen, genWallet, genTransfer);
            }
            break;
        }
    }

    // Zero-out the seed.
    seed = UINT512_ZERO;

    return;
}

extern void
cryptoWalletManagerSubmitForKey (BRCryptoWalletManager cwm,
                                 BRCryptoWallet wallet,
                                 BRCryptoTransfer transfer,
                                 BRCryptoKey key) {
    // Signing requires `key` to have a secret (that is, be a private key).
    if (!cryptoKeyHasSecret(key)) return;

    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            if (BRWalletManagerSignTransactionForKey (cwm->u.btc,
                                                      cryptoWalletAsBTC (wallet),
                                                      cryptoTransferAsBTC(transfer),
                                                      cryptoKeyGetCore (key))) {
                BRWalletManagerSubmitTransaction (cwm->u.btc,
                                                  cryptoWalletAsBTC (wallet),
                                                  cryptoTransferAsBTC(transfer));
            }
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            ewmWalletSignTransfer (cwm->u.eth,
                                   cryptoWalletAsETH (wallet),
                                   cryptoTransferAsETH (transfer),
                                   *cryptoKeyGetCore (key));

            ewmWalletSubmitTransfer (cwm->u.eth,
                                     cryptoWalletAsETH (wallet),
                                     cryptoTransferAsETH (transfer));
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWallet genWallet = cryptoWalletAsGEN (wallet);
            BRGenericTransfer genTransfer = cryptoTransferAsGEN (transfer);

            if (genManagerSignTransferWithKey (cwm->u.gen, genWallet, genTransfer, cryptoKeyGetCore (key))) {
                cryptoWalletManagerSetTransferStateGEN (cwm, wallet, transfer,
                                                        genTransferStateCreateOther (GENERIC_TRANSFER_STATE_SIGNED));
                genManagerSubmitTransfer (cwm->u.gen, genWallet, genTransfer);
            }
            break;
        }
    }
}

extern void
cryptoWalletManagerSubmitSigned (BRCryptoWalletManager cwm,
                                 BRCryptoWallet wallet,
                                 BRCryptoTransfer transfer) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWalletManagerSubmitTransaction (cwm->u.btc,
                                              cryptoWalletAsBTC (wallet),
                                              cryptoTransferAsBTC(transfer));
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            ewmWalletSubmitTransfer (cwm->u.eth,
                                     cryptoWalletAsETH (wallet),
                                     cryptoTransferAsETH (transfer));
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            genManagerSubmitTransfer (cwm->u.gen,
                                      cryptoWalletAsGEN (wallet),
                                      cryptoTransferAsGEN (transfer));
            break;
        }
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
                           BRGenericManager gwm) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_GEN == manager->type && gwm == manager->u.gen);
}

private_extern BRCryptoWallet
cryptoWalletManagerFindWalletAsBTC (BRCryptoWalletManager cwm,
                                    BRWallet *btc) {
    BRCryptoWallet wallet = NULL;
    pthread_mutex_lock (&cwm->lock);
    for (size_t index = 0; index < array_count (cwm->wallets); index++) {
        if (btc == cryptoWalletAsBTC (cwm->wallets[index])) {
            wallet = cryptoWalletTake (cwm->wallets[index]);
            break;
        }
    }
    pthread_mutex_unlock (&cwm->lock);
    return wallet;
}

private_extern BRCryptoWallet
cryptoWalletManagerFindWalletAsETH (BRCryptoWalletManager cwm,
                                    BREthereumWallet eth) {
    BRCryptoWallet wallet = NULL;
    pthread_mutex_lock (&cwm->lock);
    for (size_t index = 0; index < array_count (cwm->wallets); index++) {
        if (eth == cryptoWalletAsETH (cwm->wallets[index])) {
            wallet = cryptoWalletTake (cwm->wallets[index]);
            break;
        }
    }
    pthread_mutex_unlock (&cwm->lock);
    return wallet;
}

private_extern BRCryptoWallet
cryptoWalletManagerFindWalletAsGEN (BRCryptoWalletManager cwm,
                                    BRGenericWallet gen) {
    BRCryptoWallet wallet = NULL;
    pthread_mutex_lock (&cwm->lock);
    for (size_t index = 0; index < array_count (cwm->wallets); index++) {
        if (gen == cryptoWalletAsGEN (cwm->wallets[index])) {
            wallet = cryptoWalletTake (cwm->wallets[index]);
            break;
        }
    }
    pthread_mutex_unlock (&cwm->lock);
    return wallet;
}

extern void
cryptoWalletManagerHandleTransferGEN (BRCryptoWalletManager cwm,
                                      BRGenericTransfer transferGeneric) {
    // TODO: I don't think any locks are needed here...
    
    // TODO: Determine the currency from `transferGeneric`
    BRCryptoCurrency currency = cryptoNetworkGetCurrency (cwm->network);
    BRCryptoWallet   wallet   = cryptoWalletManagerGetWalletForCurrency (cwm, currency);

    // Look for a known transfer
    BRCryptoTransfer transfer = cryptoWalletFindTransferAsGEN (wallet, transferGeneric);

    // If we don't know about `transferGeneric`, create a crypto transfer
    if (NULL == transfer) {
        BRCryptoUnit unit = cryptoNetworkGetUnitAsBase (cwm->network, currency);
        BRCryptoUnit unitForFee = cryptoNetworkGetUnitAsBase (cwm->network, currency);

        // Create the generic transfer...
        transfer = cryptoTransferCreateAsGEN (unit, unitForFee, transferGeneric);

        // Set the state
        BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
        BRCryptoTransferState newState = cryptoTransferStateCreateGEN (genTransferGetState(transferGeneric),
                                                                       cryptoTransferGetUnitForFee(transfer));
        cryptoTransferSetState (transfer, newState);

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
            { .transfer = { cryptoTransferTake (transfer) }}
        });

        // If the state is not created, announce a transfer state change.
        if (CRYPTO_TRANSFER_STATE_CREATED != newState.type) {
            cwm->listener.transferEventCallback (cwm->listener.context,
                                                 cryptoWalletManagerTake (cwm),
                                                 cryptoWalletTake (wallet),
                                                 cryptoTransferTake(transfer),
                                                 (BRCryptoTransferEvent) {
                CRYPTO_TRANSFER_EVENT_CHANGED,
                { .state = {
                    cryptoTransferStateCopy (&oldState),
                    cryptoTransferStateCopy (&newState) }}
            });
        }

        cryptoTransferStateRelease (&oldState);
        cryptoTransferStateRelease (&newState);
        cryptoUnitGive(unitForFee);
        cryptoUnitGive(unit);
    }

    cryptoTransferGive(transfer);
    cryptoWalletGive (wallet);
    cryptoCurrencyGive(currency);
}

extern const char *
BRCryptoWalletManagerEventTypeString (BRCryptoWalletManagerEventType t) {
    switch (t) {
        case CRYPTO_WALLET_MANAGER_EVENT_CREATED:
        return "CRYPTO_WALLET_MANAGER_EVENT_CREATED";

        case CRYPTO_WALLET_MANAGER_EVENT_CHANGED:
        return "CRYPTO_WALLET_MANAGER_EVENT_CHANGED";

        case CRYPTO_WALLET_MANAGER_EVENT_DELETED:
        return "CRYPTO_WALLET_MANAGER_EVENT_DELETED";

        case CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED:
        return "CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED";

        case CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED:
        return "CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED";

        case CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED:
        return "CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED";

        case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED:
        return "CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED";

        case CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES:
        return "CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES";

        case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED:
        return "CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED";

        case CRYPTO_WALLET_MANAGER_EVENT_SYNC_RECOMMENDED:
        return "CRYPTO_WALLET_MANAGER_EVENT_SYNC_RECOMMENDED";

        case CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED:
        return "CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED";
    }
    return "<CRYPTO_WALLET_MANAGER_EVENT_TYPE_UNKNOWN>";
}

/// MARK: Wallet Migrator

struct BRCryptoWalletMigratorRecord {
    BRFileService fileService;
    const char *fileServiceTransactionType;
    const char *fileServiceBlockType;
    const char *fileServicePeerType;

    int theErrorHackHappened;
    BRFileServiceError theErrorHack;
};

static void theErrorHackReset (BRCryptoWalletMigrator migrator) {
    migrator->theErrorHackHappened = 0;
}

static void
cryptoWalletMigratorErrorHandler (BRFileServiceContext context,
                                  BRFileService fs,
                                  BRFileServiceError error) {
    // TODO: Racy on 'cryptoWalletMigratorRelease'?
    BRCryptoWalletMigrator migrator = (BRCryptoWalletMigrator) context;

    migrator->theErrorHackHappened = 1;
    migrator->theErrorHack = error;
}

extern BRCryptoWalletMigrator
cryptoWalletMigratorCreate (BRCryptoNetwork network,
                            const char *storagePath) {
    BRCryptoWalletMigrator migrator = calloc (1, sizeof (struct BRCryptoWalletMigratorRecord));

    migrator->fileService = BRWalletManagerCreateFileService (cryptoNetworkAsBTC(network),
                                                              storagePath,
                                                              migrator,
                                                              cryptoWalletMigratorErrorHandler);
    if (NULL == migrator->fileService) {
        cryptoWalletMigratorRelease(migrator);
        return NULL;
    }

    BRWalletManagerExtractFileServiceTypes (migrator->fileService,
                                            &migrator->fileServiceTransactionType,
                                            &migrator->fileServiceBlockType,
                                            &migrator->fileServicePeerType);

    return migrator;
}

extern void
cryptoWalletMigratorRelease (BRCryptoWalletMigrator migrator) {
    if (NULL != migrator->fileService) fileServiceRelease(migrator->fileService);
    free (migrator);
}

extern BRCryptoWalletMigratorStatus
cryptoWalletMigratorHandleTransactionAsBTC (BRCryptoWalletMigrator migrator,
                                            const uint8_t *bytes,
                                            size_t bytesCount,
                                            uint32_t blockHeight,
                                            uint32_t timestamp) {
    BRTransaction *tx = BRTransactionParse(bytes, bytesCount);
    if (NULL == tx)
        return (BRCryptoWalletMigratorStatus) {
            CRYPTO_WALLET_MIGRATOR_ERROR_TRANSACTION
        };

    tx->blockHeight = blockHeight;
    tx->timestamp   = timestamp;

    // Calls cryptoWalletMigratorErrorHandler on error.
    theErrorHackReset(migrator);
    fileServiceSave (migrator->fileService, migrator->fileServiceTransactionType, tx);
    BRTransactionFree(tx);

    if (migrator->theErrorHackHappened)
        return (BRCryptoWalletMigratorStatus) {
            CRYPTO_WALLET_MIGRATOR_ERROR_TRANSACTION
        };
    else
        return (BRCryptoWalletMigratorStatus) {
            CRYPTO_WALLET_MIGRATOR_SUCCESS
        };
}

extern BRCryptoWalletMigratorStatus
cryptoWalletMigratorHandleBlockAsBTC (BRCryptoWalletMigrator migrator,
                                      UInt256 hash,
                                      uint32_t height,
                                      uint32_t nonce,
                                      uint32_t target,
                                      uint32_t txCount,
                                      uint32_t version,
                                      uint32_t timestamp,
                                      uint8_t *flags,  size_t flagsLen,
                                      UInt256 *hashes, size_t hashesCount,
                                      UInt256 merkleRoot,
                                      UInt256 prevBlock) {
    BRMerkleBlock *block = BRMerkleBlockNew();
    block->blockHash = hash;
    block->height = height;
    block->nonce  = nonce;
    block->target = target;
    block->totalTx = txCount;
    block->version = version;
    if (0 != timestamp) block->timestamp = timestamp;

    BRMerkleBlockSetTxHashes (block, hashes, hashesCount, flags, flagsLen);

    block->merkleRoot = merkleRoot;
    block->prevBlock  = prevBlock;

    // ...
    theErrorHackReset(migrator);
    fileServiceSave (migrator->fileService, migrator->fileServiceBlockType, block);
    BRMerkleBlockFree (block);

    if (migrator->theErrorHackHappened)
        return (BRCryptoWalletMigratorStatus) {
            CRYPTO_WALLET_MIGRATOR_ERROR_BLOCK
        };
    else
        return (BRCryptoWalletMigratorStatus) {
            CRYPTO_WALLET_MIGRATOR_SUCCESS
        };
}

extern BRCryptoWalletMigratorStatus
cryptoWalletMigratorHandleBlockBytesAsBTC (BRCryptoWalletMigrator migrator,
                                           const uint8_t *bytes,
                                           size_t bytesCount,
                                           uint32_t height) {
    BRMerkleBlock *block = BRMerkleBlockParse (bytes, bytesCount);
    if (NULL == block)
        return (BRCryptoWalletMigratorStatus) {
            CRYPTO_WALLET_MIGRATOR_ERROR_BLOCK
        };

    block->height = height;

    // ...
    theErrorHackReset(migrator);
    fileServiceSave (migrator->fileService, migrator->fileServiceBlockType, block);
    BRMerkleBlockFree (block);

    if (migrator->theErrorHackHappened)
        return (BRCryptoWalletMigratorStatus) {
            CRYPTO_WALLET_MIGRATOR_ERROR_BLOCK
        };
    else
        return (BRCryptoWalletMigratorStatus) {
            CRYPTO_WALLET_MIGRATOR_SUCCESS
        };
}

extern BRCryptoWalletMigratorStatus
cryptoWalletMigratorHandlePeerAsBTC (BRCryptoWalletMigrator migrator,
                                     uint32_t address,
                                     uint16_t port,
                                     uint64_t services,
                                     uint32_t timestamp) {
    BRPeer peer;

    peer.address = (UInt128) { .u32 = { 0, 0, 0xffff, address }};
    peer.port = port;
    peer.services = services;
    peer.timestamp = timestamp;
    peer.flags = 0;

    theErrorHackReset(migrator);
    fileServiceSave (migrator->fileService, migrator->fileServicePeerType, &peer);

    if (migrator->theErrorHackHappened)
        return (BRCryptoWalletMigratorStatus) {
            CRYPTO_WALLET_MIGRATOR_ERROR_PEER
        };
    else
        return (BRCryptoWalletMigratorStatus) {
            CRYPTO_WALLET_MIGRATOR_SUCCESS
        };
}
