/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.events.network.NetworkEvent;
import com.breadwallet.crypto.events.system.SystemEvent;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;
import com.google.common.base.Optional;

import java.lang.ref.WeakReference;
import java.util.concurrent.ExecutorService;

/* package */
final class SystemAnnouncer {

    private final ExecutorService executor;
    private final System system;
    private final WeakReference<SystemListener> listener;

    /* package */
    SystemAnnouncer(System system, ExecutorService executor, SystemListener listener) {
        this.system = system;
        this.executor = executor;
        this.listener = new WeakReference<>(listener);
    }

    /* package */
    void announceSystemEvent(SystemEvent event) {
        SystemListener listener = this.listener.get();
        if (null != listener) {
            executor.submit(() -> listener.handleSystemEvent(system, event));
        }
    }

    /* package */
    void announceNetworkEvent(Network network, NetworkEvent event) {
        SystemListener listener = this.listener.get();
        if (null != listener) {
            executor.submit(() -> listener.handleNetworkEvent(system, network, event));
        }
    }

    /* package */
    void announceWalletManagerEvent(WalletManager walletManager, WalletManagerEvent event) {
        SystemListener listener = this.listener.get();
        if (null != listener) {
            executor.submit(() -> listener.handleManagerEvent(system, walletManager, event));
        }
    }

    /* package */
    void announceWalletEvent(WalletManager walletManager, Wallet wallet, WalletEvent event) {
        SystemListener listener = this.listener.get();
        if (null != listener) {
            executor.submit(() -> listener.handleWalletEvent(system, walletManager, wallet, event));
        }
    }

    /* package */
    void announceTransferEvent(WalletManager walletManager, Wallet wallet, Transfer transfer, TranferEvent event) {
        SystemListener listener = this.listener.get();
        if (null != listener) {
            executor.submit(() -> listener.handleTransferEvent(system, walletManager, wallet, transfer, event));
        }
    }

    /* package */
    Optional<SystemListener> getListener() {
        return Optional.fromNullable(listener.get());
    }
}
