//
//  BRCryptoWalletManager.h
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

#ifndef BRCryptoWalletManager_h
#define BRCryptoWalletManager_h

#include "BRCryptoBase.h"
#include "BRCryptoNetwork.h"
#include "BRCryptoAccount.h"
#include "BRCryptoTransfer.h"
#include "BRCryptoWallet.h"
#include "BRCryptoWalletManagerClient.h"

#include "ethereum/BREthereum.h"
#include "bitcoin/BRWalletManager.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        CRYPTO_WALLET_MANAGER_STATE_CREATED,
        CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED,
        CRYPTO_WALLET_MANAGER_STATE_CONNECTED,
        CRYPTO_WALLET_MANAGER_STATE_SYNCING,
        CRYPTO_WALLET_MANAGER_STATE_DELETED
    } BRCryptoWalletManagerState;

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

        CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED,
    } BRCryptoWalletManagerEventType;

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
                unsigned int percentComplete;
            } sync;

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

    extern BRCryptoWalletManager
    cryptoWalletManagerCreate (BRCryptoCWMListener listener,
                               BRCryptoCWMClient client,
                               BRCryptoAccount account,
                               BRCryptoNetwork network,
                               BRSyncMode mode,
                               BRCryptoAddressScheme scheme,
                               const char *path);

    extern BRCryptoNetwork
    cryptoWalletManagerGetNetwork (BRCryptoWalletManager cwm);

    extern BRCryptoAccount
    cryptoWalletManagerGetAccount (BRCryptoWalletManager cwm);

    extern BRSyncMode
    cryptoWalletManagerGetMode (BRCryptoWalletManager cwm);

    extern BRCryptoWalletManagerState
    cryptoWalletManagerGetState (BRCryptoWalletManager cwm);

    extern BRCryptoAddressScheme
    cryptoWalletManagerGetAddressScheme (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerSetAddressScheme (BRCryptoWalletManager cwm,
                                         BRCryptoAddressScheme scheme);

    extern const char *
    cryptoWalletManagerGetPath (BRCryptoWalletManager cwm);

    extern BRCryptoBoolean
    cryptoWalletManagerHasWallet (BRCryptoWalletManager cwm,
                                  BRCryptoWallet wallet);

    extern BRCryptoWallet
    cryptoWalletManagerGetWallet (BRCryptoWalletManager cwm);

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

    extern void
    cryptoWalletManagerConnect (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerDisconnect (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerSync (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerSubmit (BRCryptoWalletManager cwm,
                               BRCryptoWallet wid,
                               BRCryptoTransfer tid,
                               const char *paperKey);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoWalletManager, cryptoWalletManager);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoWalletManager_h */
