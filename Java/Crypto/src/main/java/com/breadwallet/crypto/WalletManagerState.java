/*
 * WalletManagerState
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

public enum WalletManagerState {
    CREATED,
    DISCONNECTED,
    CONNECTED,
    SYNCING,
    DELETED;

    @Override
    public String toString() {
        switch (this) {
            case DELETED:
                return "Deleted";
            case CREATED:
                return "Created";
            case SYNCING:
                return "Syncing";
            case CONNECTED:
                return "Connected";
            case DISCONNECTED:
                return "Disconnected";
            default:
                return super.toString();
        }
    }
}
