/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.system;

import com.breadwallet.crypto.events.network.NetworkListener;
import com.breadwallet.crypto.events.transfer.TransferListener;
import com.breadwallet.crypto.events.wallet.WalletListener;
import com.breadwallet.crypto.events.walletmanager.WalletManagerListener;
import com.breadwallet.crypto.System;

public interface SystemListener extends WalletManagerListener, WalletListener, TransferListener, NetworkListener {

    void handleSystemEvent(System system, SystemEvent event);
}
