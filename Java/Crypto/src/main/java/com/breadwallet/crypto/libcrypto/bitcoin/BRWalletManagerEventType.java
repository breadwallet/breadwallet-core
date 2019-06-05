/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.libcrypto.bitcoin;

public interface BRWalletManagerEventType {

    int BITCOIN_WALLET_MANAGER_CREATED = 0;
    int BITCOIN_WALLET_MANAGER_CONNECTED = 1;
    int BITCOIN_WALLET_MANAGER_DISCONNECTED = 2;
    int BITCOIN_WALLET_MANAGER_SYNC_STARTED = 3;
    int BITCOIN_WALLET_MANAGER_SYNC_STOPPED = 4;
    int BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED = 5;
}
