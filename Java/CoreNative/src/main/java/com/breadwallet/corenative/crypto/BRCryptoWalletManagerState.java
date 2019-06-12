/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public interface BRCryptoWalletManagerState {

    int CRYPTO_WALLET_MANAGER_STATE_CREATED = 0;
    int CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED = 1;
    int CRYPTO_WALLET_MANAGER_STATE_CONNECTED = 2;
    int CRYPTO_WALLET_MANAGER_STATE_SYNCING = 3;
    int CRYPTO_WALLET_MANAGER_STATE_DELETED = 4;
}
