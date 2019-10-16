/*
 * WalletManagerMode
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

public enum WalletManagerMode {
    API_ONLY,
    API_WITH_P2P_SUBMIT,
    P2P_ONLY,
    P2P_WITH_API_SYNC;

    public int toSerialization () {
        switch (this) {
            case API_ONLY:            return 0xf0;
            case API_WITH_P2P_SUBMIT: return 0xf1;
            case P2P_WITH_API_SYNC:   return 0xf2;
            case P2P_ONLY:            return 0xf3;
            default: return 0; // error
        }
    }

    public static WalletManagerMode fromSerialization (int serialization) {
        switch (serialization) {
            case 0xf0: return WalletManagerMode.API_ONLY;
            case 0xf1: return WalletManagerMode.API_WITH_P2P_SUBMIT;
            case 0xf2: return WalletManagerMode.P2P_WITH_API_SYNC;
            case 0xf3: return WalletManagerMode.P2P_ONLY;
            default: return null;
        }
    }
}
