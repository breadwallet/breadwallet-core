/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public interface BRCryptoWalletManagerEventType {

    int CRYPTO_WALLET_MANAGER_EVENT_CREATED = 0;
    int CRYPTO_WALLET_MANAGER_EVENT_CHANGED = 1;
    int CRYPTO_WALLET_MANAGER_EVENT_DELETED = 2;
    int CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED = 3;
    int CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED = 4;
    int CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED = 5;
    int CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED = 6;
    int CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES = 7;
    int CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED = 8;
    int CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED = 9;
}
