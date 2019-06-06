/*
 * WalletManagerMode
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

public enum WalletManagerMode {
    API_ONLY,
    API_WITH_P2P_SUBMIT,
    P2P_ONLY,
    P2P_WITH_API_SYNC
}
