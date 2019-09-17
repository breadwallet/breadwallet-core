/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.support;

public interface BRSyncMode {

    int SYNC_MODE_BRD_ONLY = 0;
    int SYNC_MODE_BRD_WITH_P2P_SEND = 1;
    int SYNC_MODE_P2P_WITH_BRD_SYNC = 2;
    int SYNC_MODE_P2P_ONLY = 3;
}
