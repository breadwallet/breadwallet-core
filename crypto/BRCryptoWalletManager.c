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
#include <arpa/inet.h>      // struct in_addr

#include "BRCryptoBase.h"

#include "BRCryptoKeyP.h"
#include "BRCryptoAccountP.h"
#include "BRCryptoNetworkP.h"
#include "BRCryptoAddressP.h"
#include "BRCryptoAmountP.h"
#include "BRCryptoFeeBasisP.h"
#include "BRCryptoTransferP.h"
#include "BRCryptoWalletP.h"
#include "BRCryptoPaymentP.h"

#include "BRCryptoWalletManager.h"
#include "BRCryptoWalletManagerClient.h"
#include "BRCryptoWalletManagerP.h"

#include "bitcoin/BRWalletManager.h"
#include "ethereum/BREthereum.h"
#include "support/BRFileService.h"

static void
cryptoWalletManagerInstallETHTokensForCurrencies (BRCryptoWalletManager cwm);

static void
cryptoWalletManagerSyncCallbackGEN (BRGenericManagerSyncContext context,
                                    BRGenericManager manager,
                                    uint64_t begBlockHeight,
                                    uint64_t endBlockHeight,
                                    uint64_t fullSyncIncrement);

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
                { .disconnected = { cryptoWalletManagerDisconnectReasonUnknown() } }
            };
    }
}

private_extern BRCryptoWalletManagerState
cryptoWalletManagerStateDisconnectedInit(BRCryptoWalletManagerDisconnectReason reason) {
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

extern void
cryptoWalletManagerWipe (BRCryptoNetwork network,
                         const char *path) {
    switch (cryptoNetworkGetType(network)) {
        case BLOCK_CHAIN_TYPE_BTC:
            BRWalletManagerWipe (cryptoNetworkAsBTC(network), path);
            break;

        case BLOCK_CHAIN_TYPE_ETH:
            ewmWipe (cryptoNetworkAsETH(network), path);
            break;

        case BLOCK_CHAIN_TYPE_GEN:
            genManagerWipe (cryptoNetworkAsGEN (network), path);
            break;
    }
}

extern BRCryptoWalletManager
cryptoWalletManagerCreate (BRCryptoCWMListener listener,
                           BRCryptoCWMClient client,
                           BRCryptoAccount account,
                           BRCryptoNetwork network,
                           BRCryptoSyncMode mode,
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
                                           cwm,
                                           cryptoWalletManagerSyncCallbackGEN,
                                           cryptoNetworkGetHeight(network));
            if (NULL == cwm->u.gen) {
                pthread_mutex_unlock (&cwm->lock);
                error = 1;
                break; }

            // ... and create the primary wallet
            cwm->wallet = cryptoWalletCreateAsGEN (unit, unit, genManagerGetPrimaryWallet (cwm->u.gen));

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
            array_free (transfers);

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

extern void
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
cryptoWalletManagerSetMode (BRCryptoWalletManager cwm, BRCryptoSyncMode mode) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            BRWalletManagerSetMode (cwm->u.btc, mode);
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            ewmUpdateMode (cwm->u.eth, mode);
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            assert (CRYPTO_SYNC_MODE_API_ONLY == mode);
            break;
        default:
            assert (0);
            break;
    }
}

extern BRCryptoSyncMode
cryptoWalletManagerGetMode (BRCryptoWalletManager cwm) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
           return BRWalletManagerGetMode (cwm->u.btc);
        case BLOCK_CHAIN_TYPE_ETH:
            return ewmGetMode (cwm->u.eth);
        case BLOCK_CHAIN_TYPE_GEN:
            return CRYPTO_SYNC_MODE_API_ONLY;
        default:
            assert (0);
            return CRYPTO_SYNC_MODE_API_ONLY;

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

extern void
cryptoWalletManagerAddWallet (BRCryptoWalletManager cwm,
                              BRCryptoWallet wallet) {
    pthread_mutex_lock (&cwm->lock);
    if (CRYPTO_FALSE == cryptoWalletManagerHasWallet (cwm, wallet)) {
        array_add (cwm->wallets, cryptoWalletTake (wallet));
    }
    pthread_mutex_unlock (&cwm->lock);
}

extern void
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
                BRCryptoData16 addrAsInt = cryptoPeerGetAddrAsInt(peer);
                memcpy (address.u8, addrAsInt.data, sizeof (addrAsInt.data));
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
            pthread_mutex_lock (&cwm->lock);
            if (!genManagerIsConnected (cwm->u.gen)) {
                BRCryptoWalletManagerState oldState = cwm->state;
                BRCryptoWalletManagerState newState = cryptoWalletManagerStateInit (CRYPTO_WALLET_MANAGER_STATE_CONNECTED);

                // assert oldState != CRYPTO_WALLET_MANAGER_STATE_CONNECTED
                genManagerConnect(cwm->u.gen);
                cryptoWalletManagerSetState (cwm, newState);
                pthread_mutex_unlock (&cwm->lock);

                cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                          cryptoWalletManagerTake (cwm),
                                                          (BRCryptoWalletManagerEvent) {
                    CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                    { .state = { oldState, newState }}
                });
            }
            else pthread_mutex_unlock (&cwm->lock);
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
            pthread_mutex_lock (&cwm->lock);
            if (genManagerIsConnected (cwm->u.gen)) {
                BRCryptoWalletManagerState oldState = cwm->state;
                BRCryptoWalletManagerState newState = cryptoWalletManagerStateDisconnectedInit (cryptoWalletManagerDisconnectReasonRequested());

                // assert oldState != CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED
                genManagerDisconnect (cwm->u.gen);
                cryptoWalletManagerSetState (cwm, newState);
                pthread_mutex_unlock (&cwm->lock);

                cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                          cryptoWalletManagerTake (cwm),
                                                          (BRCryptoWalletManagerEvent) {
                    CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
                    { .state = { oldState, newState }}
                });
            }
            else pthread_mutex_unlock (&cwm->lock);
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
                                BRCryptoSyncDepth depth) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            BRWalletManagerScanToDepth (cwm->u.btc, depth);
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            ewmSyncToDepth (cwm->u.eth, ETHEREUM_BOOLEAN_FALSE, depth);
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            genManagerSyncToDepth(cwm->u.gen, depth);
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
        case GENERIC_TRANSFER_STATE_INCLUDED: {
            BRCryptoFeeBasis      basis = cryptoFeeBasisCreateAsGEN (feeUnit, generic.u.included.feeBasis);
            BRCryptoTransferState state = cryptoTransferStateIncludedInit (generic.u.included.blockNumber,
                                                                           generic.u.included.transactionIndex,
                                                                           generic.u.included.timestamp,
                                                                           basis,
                                                                           generic.u.included.success,
                                                                           generic.u.included.error);
            cryptoFeeBasisGive (basis);
            return state;
        }
        case GENERIC_TRANSFER_STATE_ERRORED:
            return cryptoTransferStateErroredInit (cryptoTransferSubmitErrorUnknown());
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

    //
    // If this is an error case, then we must remove the genericTransfer from the
    // genericWallet; otherwise the GEN balance and sequence number will be off.
    //
    // However, we leave the `transfer` in `wallet`.  And trouble is forecasted...
    //
    if (GENERIC_TRANSFER_STATE_ERRORED == newGenericState.type) {
        genWalletRemTransfer(cryptoWalletAsGEN(wallet), genericTransfer);

        BRCryptoAmount balance = cryptoWalletGetBalance(wallet);
        cwm->listener.walletEventCallback (cwm->listener.context,
                                           cryptoWalletManagerTake (cwm),
                                           cryptoWalletTake (cwm->wallet),
                                           (BRCryptoWalletEvent) {
                                               CRYPTO_WALLET_EVENT_BALANCE_UPDATED,
                                               { .balanceUpdated = { balance }}
                                           });

        cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                  cryptoWalletManagerTake (cwm),
                                                  (BRCryptoWalletManagerEvent) {
            CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED,
            { .wallet = cryptoWalletTake (cwm->wallet) }
        });
    }

    pthread_mutex_unlock (&cwm->lock);
}

extern BRCryptoTransfer
cryptoWalletManagerCreateTransfer (BRCryptoWalletManager cwm,
                                   BRCryptoWallet wallet,
                                   BRCryptoAddress target,
                                   BRCryptoAmount amount,
                                   BRCryptoFeeBasis estimatedFeeBasis,
                                   size_t attributesCount,
                                   OwnershipKept BRCryptoTransferAttribute *attributes) {
    BRCryptoTransfer transfer = cryptoWalletCreateTransfer (wallet, target, amount,
                                                            estimatedFeeBasis,
                                                            attributesCount,
                                                            attributes);
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            if (NULL != transfer) {
                cwm->listener.transferEventCallback (cwm->listener.context,
                                                     cryptoWalletManagerTake (cwm),
                                                     cryptoWalletTake (wallet),
                                                     cryptoTransferTake(transfer),
                                                     (BRCryptoTransferEvent) {
                    CRYPTO_TRANSFER_EVENT_CREATED
                });
            }
            break;
    }
    return transfer;
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
                cryptoWalletManagerSubmitSigned (cwm, wallet, transfer);
            }
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            ewmWalletSignTransferWithPaperKey (cwm->u.eth,
                                               cryptoWalletAsETH (wallet),
                                               cryptoTransferAsETH (transfer),
                                               paperKey);

            cryptoWalletManagerSubmitSigned (cwm, wallet, transfer);
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
                cryptoWalletManagerSubmitSigned (cwm, wallet, transfer);
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
                cryptoWalletManagerSubmitSigned (cwm, wallet, transfer);
            }
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            ewmWalletSignTransfer (cwm->u.eth,
                                   cryptoWalletAsETH (wallet),
                                   cryptoTransferAsETH (transfer),
                                   *cryptoKeyGetCore (key));

            cryptoWalletManagerSubmitSigned (cwm, wallet, transfer);
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWallet genWallet = cryptoWalletAsGEN (wallet);
            BRGenericTransfer genTransfer = cryptoTransferAsGEN (transfer);

            if (genManagerSignTransferWithKey (cwm->u.gen, genWallet, genTransfer, cryptoKeyGetCore (key))) {
                cryptoWalletManagerSetTransferStateGEN (cwm, wallet, transfer,
                                                        genTransferStateCreateOther (GENERIC_TRANSFER_STATE_SIGNED));
                cryptoWalletManagerSubmitSigned (cwm, wallet, transfer);
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
            // We don't have GEN Events bubbling up that we can handle by adding transfer
            // to wallet.  So, we'll add transfer here...
            cryptoWalletAddTransfer (wallet, transfer);

            // Add the signed/submitted transfer to the GEN wallet.  If the submission fails
            // we'll remove it then.  For now, include transfer when computing the balance and
            // the sequence-number 
            genWalletAddTransfer (cryptoWalletAsGEN(wallet), cryptoTransferAsGEN(transfer));

            // ... and announce the wallet's newly added transfer
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (wallet),
                                               (BRCryptoWalletEvent) {
                CRYPTO_WALLET_EVENT_TRANSFER_ADDED,
                { .transfer = { cryptoTransferTake (transfer) }}
            });

            // ... perform the actual submit
            genManagerSubmitTransfer (cwm->u.gen,
                                      cryptoWalletAsGEN (wallet),
                                      cryptoTransferAsGEN (transfer));

            // ... and then announce the submission.
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (wallet),
                                               (BRCryptoWalletEvent) {
                CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED,
                { .transfer = { cryptoTransferTake (transfer) }}
            });

            break;
        }
    }
}

extern BRCryptoAmount
cryptoWalletManagerEstimateLimit (BRCryptoWalletManager cwm,
                                  BRCryptoWallet  wallet,
                                  BRCryptoBoolean asMaximum,
                                  BRCryptoAddress target,
                                  BRCryptoNetworkFee fee,
                                  BRCryptoBoolean *needEstimate,
                                  BRCryptoBoolean *isZeroIfInsuffientFunds) {
    assert (NULL != needEstimate && NULL != isZeroIfInsuffientFunds);

    UInt256 amount = UINT256_ZERO;
    BRCryptoUnit unit = cryptoUnitGetBaseUnit (wallet->unit);

    // By default, we don't need an estimate
    *needEstimate = CRYPTO_FALSE;

    // By default, zero does not indicate insufficient funds
    *isZeroIfInsuffientFunds = CRYPTO_FALSE;

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = wallet->u.btc.wid;

            // Amount may be zero if insufficient fees
            *isZeroIfInsuffientFunds = CRYPTO_TRUE;

            // NOTE: We know BTC/BCH has a minimum balance of zero.

            uint64_t balance     = BRWalletBalance (wid);
            uint64_t feePerKB    = 1000 * cryptoNetworkFeeAsBTC (fee);
            uint64_t amountInSAT = (CRYPTO_FALSE == asMaximum
                                    ? BRWalletMinOutputAmountWithFeePerKb (wid, feePerKB)
                                    : BRWalletMaxOutputAmountWithFeePerKb (wid, feePerKB));
            uint64_t fee         = (amountInSAT > 0
                                    ? BRWalletFeeForTxAmountWithFeePerKb (wid, feePerKB, amountInSAT)
                                    : 0);

//            if (CRYPTO_TRUE == asMaximum)
//                assert (balance == amountInSAT + fee);

            if (amountInSAT + fee > balance)
                amountInSAT = 0;

            amount = createUInt256(amountInSAT);
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = wallet->u.eth.ewm;
            BREthereumWallet wid = wallet->u.eth.wid;

            // We always need an estimate as we do not know the fees.
            *needEstimate = CRYPTO_TRUE;

            if (CRYPTO_FALSE == asMaximum)
                amount = createUInt256(0);
            else {
                BREthereumAmount ethAmount = ewmWalletGetBalance (ewm, wid);

                // NOTE: We know ETH has a minimum balance of zero.

                amount = (AMOUNT_ETHER == amountGetType(ethAmount)
                          ? amountGetEther(ethAmount).valueInWEI
                          : amountGetTokenQuantity(ethAmount).valueAsInteger);
            }
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            // TODO: Probably, unfortunately, the need for an estimate is likely currency dependent.
            *needEstimate = CRYPTO_FALSE;

            if (CRYPTO_FALSE == asMaximum)
                amount = createUInt256(0);
            else {
                int negative = 0, overflow = 0;

                // Get the balance
                UInt256 balance = genWalletGetBalance (wallet->u.gen);

                // We are looking for the maximum amount; check if the wallet has a minimum
                // balance.  If so, reduce the above balance.
                BRCryptoBoolean hasMinimum = CRYPTO_FALSE;
                UInt256 balanceMinimum = genWalletGetBalanceLimit (wallet->u.gen, CRYPTO_FALSE, &hasMinimum);

                if (CRYPTO_TRUE == hasMinimum) {
                    balance = subUInt256_Negative(balance, balanceMinimum, &negative);
                    if (negative) balance = UINT256_ZERO;
                }

                // Get the pricePerCostFactor for the (network) fee.
                BRCryptoAmount pricePerCostFactor = cryptoNetworkFeeGetPricePerCostFactor (fee);
                
                // Get a feeBasis using some sketchy defaults
                BRGenericAddress address   = genWalletGetAddress (wallet->u.gen);
                BRGenericFeeBasis feeBasis = genWalletEstimateTransferFee (wallet->u.gen,
                                                                           address,
                                                                           balance,
                                                                           cryptoAmountGetValue(pricePerCostFactor));

                // Finally, compute the fee.
                UInt256 fee = genFeeBasisGetFee (&feeBasis, &overflow);
                assert (!overflow);

                amount = subUInt256_Negative (balance, fee, &negative);
                if (negative) amount = UINT256_ZERO;

                genAddressRelease(address);
                cryptoAmountGive(pricePerCostFactor);

            }
            break;
        }
    }

    return cryptoAmountCreateInternal (unit,
                                       CRYPTO_FALSE,
                                       amount,
                                       0);
}


extern void
cryptoWalletManagerEstimateFeeBasis (BRCryptoWalletManager cwm,
                                     BRCryptoWallet  wallet,
                                     BRCryptoCookie cookie,
                                     BRCryptoAddress target,
                                     BRCryptoAmount  amount,
                                     BRCryptoNetworkFee fee) {
    //    assert (cryptoWalletGetType (wallet) == cryptoFeeBasisGetType(feeBasis));
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWalletManager bwm = cwm->u.btc;
            BRWallet *wid = cryptoWalletAsBTC(wallet);

            BRCryptoBoolean overflow = CRYPTO_FALSE;
            uint64_t feePerKB        = 1000 * cryptoNetworkFeeAsBTC (fee);
            uint64_t btcAmount       = cryptoAmountGetIntegerRaw (amount, &overflow);
            assert(CRYPTO_FALSE == overflow);

            BRWalletManagerEstimateFeeForTransfer (bwm,
                                                   wid,
                                                   cookie,
                                                   btcAmount,
                                                   feePerKB);
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = cwm->u.eth;
            BREthereumWallet wid = cryptoWalletAsETH(wallet);

            BRCryptoAddress source = cryptoWalletGetAddress (wallet, CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT);
            UInt256 ethValue       = cryptoAmountGetValue (amount);

            BREthereumToken  ethToken  = ewmWalletGetToken (ewm, wid);
            BREthereumAmount ethAmount = (NULL != ethToken
                                          ? amountCreateToken (createTokenQuantity (ethToken, ethValue))
                                          : amountCreateEther (etherCreate (ethValue)));

            ewmWalletEstimateTransferFeeForTransfer (ewm,
                                                     wid,
                                                     cookie,
                                                     cryptoAddressAsETH (source),
                                                     cryptoAddressAsETH (target),
                                                     ethAmount,
                                                     cryptoNetworkFeeAsETH (fee),
                                                     ewmWalletGetDefaultGasLimit (ewm, wid));

            cryptoAddressGive (source);
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWallet  genWallet  = cryptoWalletAsGEN (wallet);
            BRGenericAddress genAddress = cryptoAddressAsGEN (target);

            UInt256 genValue = cryptoAmountGetValue(amount);
            UInt256 genPricePerCostFactor = createUInt256 (cryptoNetworkFeeAsGEN(fee));

            BRGenericFeeBasis genFeeBasis = genWalletEstimateTransferFee (genWallet,
                                                                          genAddress,
                                                                          genValue,
                                                                          genPricePerCostFactor);

            BRCryptoUnit unitForFee = cryptoWalletGetUnitForFee (wallet);
            BRCryptoFeeBasis feeBasis = cryptoFeeBasisCreateAsGEN (unitForFee, genFeeBasis);
            
            cwm->listener.walletEventCallback (cwm->listener.context,
                                               cryptoWalletManagerTake (cwm),
                                               cryptoWalletTake (wallet),
                                               (BRCryptoWalletEvent) {
                CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED,
                { .feeBasisEstimated = {
                    CRYPTO_SUCCESS,
                    cookie,
                    cryptoFeeBasisTake (feeBasis)
                }}
            });

            cryptoFeeBasisGive (feeBasis);
            cryptoUnitGive(unitForFee);
        }
    }
}

extern void
cryptoWalletManagerEstimateFeeBasisForWalletSweep (BRCryptoWalletManager cwm,
                                                   BRCryptoWallet wallet,
                                                   BRCryptoCookie cookie,
                                                   BRCryptoWalletSweeper sweeper,
                                                   BRCryptoNetworkFee fee) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWalletManager bwm = cwm->u.btc;
            BRWallet *wid = cryptoWalletAsBTC (wallet);
            uint64_t feePerKB = 1000 * cryptoNetworkFeeAsBTC (fee);

            BRWalletManagerEstimateFeeForSweep (bwm,
                                                wid,
                                                cookie,
                                                cryptoWalletSweeperAsBTC(sweeper),
                                                feePerKB);
            break;
        }
        default:
            assert (0);
            break;
    }
}

extern void
cryptoWalletManagerEstimateFeeBasisForPaymentProtocolRequest (BRCryptoWalletManager cwm,
                                                              BRCryptoWallet wallet,
                                                              BRCryptoCookie cookie,
                                                              BRCryptoPaymentProtocolRequest request,
                                                              BRCryptoNetworkFee fee) {
    switch (cwm->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWalletManager bwm = cwm->u.btc;
            BRWallet *wid = cryptoWalletAsBTC (wallet);
            uint64_t feePerKB = 1000 * cryptoNetworkFeeAsBTC (fee);

            switch (cryptoPaymentProtocolRequestGetType (request)) {
                case CRYPTO_PAYMENT_PROTOCOL_TYPE_BITPAY:
                case CRYPTO_PAYMENT_PROTOCOL_TYPE_BIP70: {
                    BRArrayOf(BRTxOutput) outputs = cryptoPaymentProtocolRequestGetOutputsAsBTC (request);
                    if (NULL != outputs) {
                        BRWalletManagerEstimateFeeForOutputs (bwm, wid, cookie, outputs, array_count (outputs),
                                                              feePerKB);
                        array_free (outputs);
                    }
                    break;
                }
                default: {
                    assert (0);
                    break;
                }
            }
            break;
        }
        default:
            assert (0);
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
                                      OwnershipGiven BRGenericTransfer transferGeneric) {
    int transferWasCreated = 0;

    // TODO: Determine the currency from `transferGeneric`
    BRCryptoCurrency currency   = cryptoNetworkGetCurrency   (cwm->network);
    BRCryptoUnit     unit       = cryptoNetworkGetUnitAsBase (cwm->network, currency);
    BRCryptoUnit     unitForFee = cryptoNetworkGetUnitAsBase (cwm->network, currency);
    BRCryptoWallet   wallet     = cryptoWalletManagerGetWalletForCurrency (cwm, currency);

    // TODO: I don't think any overall locks are needed here...

    // Look for a known transfer
    BRCryptoTransfer transfer = cryptoWalletFindTransferAsGEN (wallet, transferGeneric);

    // If we don't know about `transferGeneric`, create a crypto transfer
    if (NULL == transfer) {
        // Create the generic transfer... `transferGeneric` owned by `transfer`
        transfer = cryptoTransferCreateAsGEN (unit, unitForFee, transferGeneric);

        transferWasCreated = 1;
    }

    // We know 'transfer'; ensure it is up to date.  This is important for the case where
    // we created the transfer and then submitted it.  In that case `transfer` is what we
    // created and `transferGeneric` is what we recovered.  The recovered transfer will have
    // additional information - notably the UIDS.
    else {
        BRGenericTransfer transferGenericOrig = cryptoTransferAsGEN (transfer);

        // Update the UIDS
        if (NULL == genTransferGetUIDS(transferGenericOrig))
            genTransferSetUIDS (transferGenericOrig,
                                genTransferGetUIDS (transferGeneric));
    }

    // Fill in any attributes
    BRArrayOf(BRGenericTransferAttribute) genAttributes = genTransferGetAttributes(transferGeneric);
    BRArrayOf(BRCryptoTransferAttribute)  attributes;
    array_new(attributes, array_count(genAttributes));
    for (size_t index = 0; index < array_count(genAttributes); index++) {
        array_add (attributes,
                   cryptoTransferAttributeCreate (genTransferAttributeGetKey(genAttributes[index]),
                                                  genTransferAttributeGetVal(genAttributes[index]),
                                                  AS_CRYPTO_BOOLEAN (genTransferAttributeIsRequired(genAttributes[index]))));
    }
    cryptoTransferSetAttributes (transfer, attributes);
    array_free_all (attributes, cryptoTransferAttributeGive);

    // Set the state from `transferGeneric`.  This is where we move from 'submitted' to 'included'
    BRCryptoTransferState oldState = cryptoTransferGetState (transfer);
    BRCryptoTransferState newState = cryptoTransferStateCreateGEN (genTransferGetState(transferGeneric), unitForFee);
    cryptoTransferSetState (transfer, newState);

    if (!transferWasCreated)
        genTransferRelease(transferGeneric);

    // Save the transfer as it is now fully updated.
    genManagerSaveTransfer (cwm->u.gen, cryptoTransferAsGEN(transfer));

    // If we created the transfer...
    if (transferWasCreated) {
        // ... announce the newly created transfer.
        cwm->listener.transferEventCallback (cwm->listener.context,
                                             cryptoWalletManagerTake (cwm),
                                             cryptoWalletTake (wallet),
                                             cryptoTransferTake(transfer),
                                             (BRCryptoTransferEvent) {
            CRYPTO_TRANSFER_EVENT_CREATED
        });

        // ... add the transfer to its wallet...
        cryptoWalletAddTransfer (wallet, transfer);

        // ... tell 'generic wallet' about it.
        genWalletAddTransfer (cryptoWalletAsGEN(wallet), cryptoTransferAsGEN(transfer));

        // ... and announce the wallet's newly added transfer
        cwm->listener.walletEventCallback (cwm->listener.context,
                                           cryptoWalletManagerTake (cwm),
                                           cryptoWalletTake (wallet),
                                           (BRCryptoWalletEvent) {
            CRYPTO_WALLET_EVENT_TRANSFER_ADDED,
            { .transfer = { cryptoTransferTake (transfer) }}
        });

        BRCryptoAmount balance = cryptoWalletGetBalance(wallet);
        cwm->listener.walletEventCallback (cwm->listener.context,
                                           cryptoWalletManagerTake (cwm),
                                           cryptoWalletTake (cwm->wallet),
                                           (BRCryptoWalletEvent) {
                                               CRYPTO_WALLET_EVENT_BALANCE_UPDATED,
                                               { .balanceUpdated = { balance }}
                                           });

        cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                  cryptoWalletManagerTake (cwm),
                                                  (BRCryptoWalletManagerEvent) {
            CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED,
            { .wallet = cryptoWalletTake (cwm->wallet) }
        });
    }

    // If the state is not created and changed, announce a transfer state change.
    if (CRYPTO_TRANSFER_STATE_CREATED != newState.type && oldState.type != newState.type) {
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
    cryptoTransferGive(transfer);
    cryptoWalletGive (wallet);
    cryptoCurrencyGive(currency);
}

static void
cryptoWalletManagerSyncCallbackGEN (BRGenericManagerSyncContext context,
                                    BRGenericManager manager,
                                    uint64_t begBlockHeight,
                                    uint64_t endBlockHeight,
                                    uint64_t fullSyncIncrement) {
    BRCryptoWalletManager cwm = cryptoWalletManagerTakeWeak ((BRCryptoWalletManager) context);
    if (NULL == cwm) return;

    // If the sync block range is larger than fullSyncIncrement, then this is a full sync.
    // Otherwise this is an ongoing, periodic sync - which we do not report.  It is as if in
    // P2P mode, a new block is announced.
    int fullSync = (endBlockHeight - begBlockHeight > fullSyncIncrement);

    pthread_mutex_lock (&cwm->lock);

    // If an ongoing sync, we are simply CONNECTED.
    BRCryptoWalletManagerState oldState = cwm->state;
    BRCryptoWalletManagerState newState = cryptoWalletManagerStateInit (fullSync
                                                                        ? CRYPTO_WALLET_MANAGER_STATE_SYNCING
                                                                        : CRYPTO_WALLET_MANAGER_STATE_CONNECTED);

    // Callback a Wallet Manager Event, but only on state changes.  We won't announce incremental
    // progress (with a blockHeight and timestamp.
    if (newState.type != oldState.type) {

        // Update the CWM state before any event callbacks.
        cryptoWalletManagerSetState (cwm, newState);

        pthread_mutex_unlock (&cwm->lock);

        if (fullSync) {
            // Generate a SYNC_STARTED...
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED
            });

            // ... and then a SYNC_CONTINUES at %100
            //            cwm->listener.walletManagerEventCallback (cwm->listener.context,
            //                                                      cryptoWalletManagerTake (cwm),
            //                                                      (BRCryptoWalletManagerEvent) {
            //                CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES,
            //                { .syncContinues = { NO_CRYPTO_SYNC_TIMESTAMP, 0 }}
            //            });
        }
        else {
            // Generate a SYNC_CONTINUES at %100...
            //            cwm->listener.walletManagerEventCallback (cwm->listener.context,
            //                                                      cryptoWalletManagerTake (cwm),
            //                                                      (BRCryptoWalletManagerEvent) {
            //                CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES,
            //                { .syncContinues = { NO_CRYPTO_SYNC_TIMESTAMP, 100 }}
            //            });

            // ... and then a CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED
            cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                      cryptoWalletManagerTake (cwm),
                                                      (BRCryptoWalletManagerEvent) {
                CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED,
                { .syncStopped = { CRYPTO_SYNC_STOPPED_REASON_COMPLETE }}
            });
        }

        cwm->listener.walletManagerEventCallback (cwm->listener.context,
                                                  cryptoWalletManagerTake (cwm),
                                                  (BRCryptoWalletManagerEvent) {
            CRYPTO_WALLET_MANAGER_EVENT_CHANGED,
            { .state = { oldState, newState }}
        });
    }
    else pthread_mutex_unlock (&cwm->lock);

    cryptoWalletManagerGive (cwm);
}

extern const char *
cryptoWalletManagerEventTypeString (BRCryptoWalletManagerEventType t) {
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

    memset (migrator, 0, sizeof(*migrator));
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
                                      BRCryptoData32 prevBlock) {
    BRMerkleBlock *block = BRMerkleBlockNew();

    memcpy (block->blockHash.u8, hash.data, sizeof (hash.data));
    block->height = height;
    block->nonce  = nonce;
    block->target = target;
    block->totalTx = txCount;
    block->version = version;
    if (0 != timestamp) block->timestamp = timestamp;

    BRMerkleBlockSetTxHashes (block, (UInt256*) hashes, hashesCount, flags, flagsLen);

    memcpy (block->merkleRoot.u8, merkleRoot.data, sizeof (merkleRoot.data));
    memcpy (block->prevBlock.u8,  prevBlock.data,  sizeof (prevBlock.data));

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

/// MARK: Disconnect Reason

extern BRCryptoWalletManagerDisconnectReason
cryptoWalletManagerDisconnectReasonRequested(void) {
    return (BRCryptoWalletManagerDisconnectReason) {
        CRYPTO_WALLET_MANAGER_DISCONNECT_REASON_REQUESTED
    };
}

extern BRCryptoWalletManagerDisconnectReason
cryptoWalletManagerDisconnectReasonUnknown(void) {
    return (BRCryptoWalletManagerDisconnectReason) {
        CRYPTO_WALLET_MANAGER_DISCONNECT_REASON_UNKNOWN
    };
}

extern BRCryptoWalletManagerDisconnectReason
cryptoWalletManagerDisconnectReasonPosix(int errnum) {
    return (BRCryptoWalletManagerDisconnectReason) {
        CRYPTO_WALLET_MANAGER_DISCONNECT_REASON_POSIX,
        { .posix = { errnum } }
    };
}

extern char *
cryptoWalletManagerDisconnectReasonGetMessage(BRCryptoWalletManagerDisconnectReason *reason) {
    char *message = NULL;

    switch (reason->type) {
        case CRYPTO_WALLET_MANAGER_DISCONNECT_REASON_POSIX: {
            if (NULL != (message = strerror (reason->u.posix.errnum))) {
                message = strdup (message);
            }
            break;
        }
        default: {
            break;
        }
    }

    return message;
}

/// MARK: Sync Stopped Reason

extern BRCryptoSyncStoppedReason
cryptoSyncStoppedReasonComplete(void) {
    return (BRCryptoSyncStoppedReason) {
        CRYPTO_SYNC_STOPPED_REASON_COMPLETE
    };
}

extern BRCryptoSyncStoppedReason
cryptoSyncStoppedReasonRequested(void) {
    return (BRCryptoSyncStoppedReason) {
        CRYPTO_SYNC_STOPPED_REASON_REQUESTED
    };
}

extern BRCryptoSyncStoppedReason
cryptoSyncStoppedReasonUnknown(void) {
    return (BRCryptoSyncStoppedReason) {
        CRYPTO_SYNC_STOPPED_REASON_UNKNOWN
    };
}

extern BRCryptoSyncStoppedReason
cryptoSyncStoppedReasonPosix(int errnum) {
    return (BRCryptoSyncStoppedReason) {
        CRYPTO_SYNC_STOPPED_REASON_POSIX,
        { .posix = { errnum } }
    };
}

extern char *
cryptoSyncStoppedReasonGetMessage(BRCryptoSyncStoppedReason *reason) {
    char *message = NULL;

    switch (reason->type) {
        case CRYPTO_SYNC_STOPPED_REASON_POSIX: {
            if (NULL != (message = strerror (reason->u.posix.errnum))) {
                message = strdup (message);
            }
            break;
        }
        default: {
            break;
        }
    }

    return message;
}

/// MARK: Sync Mode

extern const char *
cryptoSyncModeString (BRCryptoSyncMode m) {
    switch (m) {
        case CRYPTO_SYNC_MODE_API_ONLY:
        return "CRYPTO_SYNC_MODE_API_ONLY";
        case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND:
        return "CRYPTO_SYNC_MODE_API_WITH_P2P_SEND";
        case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC:
        return "CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC";
        case CRYPTO_SYNC_MODE_P2P_ONLY:
        return "CRYPTO_SYNC_MODE_P2P_ONLY";
    }
}
