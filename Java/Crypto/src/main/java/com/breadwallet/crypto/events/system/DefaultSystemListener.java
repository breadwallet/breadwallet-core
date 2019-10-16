/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.system;

import com.breadwallet.crypto.System;
import com.breadwallet.crypto.events.network.DefaultNetworkListener;
import com.breadwallet.crypto.events.transfer.DefaultTransferListener;
import com.breadwallet.crypto.events.wallet.DefaultWalletListener;
import com.breadwallet.crypto.events.walletmanager.DefaultWalletManagerListener;

public interface DefaultSystemListener extends SystemListener, DefaultWalletManagerListener, DefaultWalletListener, DefaultTransferListener, DefaultNetworkListener {

    default void handleSystemEvent(System system, SystemEvent event) {

    }
}
