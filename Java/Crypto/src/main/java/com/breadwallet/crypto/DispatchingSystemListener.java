/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.events.network.NetworkEvent;
import com.breadwallet.crypto.events.system.DefaultSystemListener;
import com.breadwallet.crypto.events.system.SystemEvent;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;

import java.util.Objects;
import java.util.concurrent.CopyOnWriteArraySet;

public final class DispatchingSystemListener implements SystemListener {

    private final CopyOnWriteArraySet<SystemListener> listeners;

    public DispatchingSystemListener() {
        this.listeners = new CopyOnWriteArraySet<>();
    }

    @Override
    public void handleSystemEvent(System system, SystemEvent event) {
        for (SystemListener listener: listeners) {
            listener.handleSystemEvent(system, event);
        };
    }

    @Override
    public void handleNetworkEvent(System system, Network network, NetworkEvent event) {
        for (SystemListener listener: listeners) {
            listener.handleNetworkEvent(system, network, event);
        }
    }

    @Override
    public void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
        for (SystemListener listener: listeners) {
            listener.handleManagerEvent(system, manager, event);
        }
    }

    @Override
    public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        for (SystemListener listener: listeners) {
            listener.handleWalletEvent(system, manager, wallet, event);
        }
    }

    @Override
    public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
        for (SystemListener listener: listeners) {
            listener.handleTransferEvent(system, manager, wallet, transfer, event);
        }
    }

    // SystemListener registration

    /**
     * Add a listener for all events.
     */
    public void addSystemListener(SystemListener listener) {
        listeners.add(listener);
    }

    /**
     * Remove a listener for all events.
     */
    public void removeSystemListener(SystemListener listener) {
        listeners.remove(listener);
    }

    // WalletManagerListener registration

    /**
     * Add a listener for events scoped to a {@link WalletManager}.
     *
     * This includes {@link WalletManagerEvent}, {@link WalletEvent} and {@link TranferEvent} events.
     */
    public void addWalletManagerListener(WalletManager manager, SystemListener listener) {
        listeners.add(new ScopedWalletManagerListener(manager, listener));
    }

    /**
     * Remove a listener for events scoped to a {@link WalletManager}.
     */
    public void removeWalletManagerListener(WalletManager manager, SystemListener listener) {
        listeners.remove(new ScopedWalletManagerListener(manager, listener));
    }

    // WalletListener registration

    /**
     * Add a listener for events scoped to a {@link Wallet}.
     *
     * This includes {@link WalletEvent} and {@link TranferEvent} events.
     */
    public void addWalletListener(Wallet wallet, SystemListener listener) {
        listeners.add(new ScopedWalletListener(wallet, listener));
    }

    /**
     * Remove a listener for events scoped to a {@link Wallet}.
     */
    public void removeWalletListener(Wallet wallet, SystemListener listener) {
        listeners.remove(new ScopedWalletListener(wallet, listener));
    }

    // TransferListener registration

    /**
     * Add a listener for events scoped to a {@link Transfer}.
     *
     * This includes {@link TranferEvent} events.
     */
    public void addTransferListener(Transfer transfer, SystemListener listener) {
        listeners.add(new ScopedTransferListener(transfer, listener));
    }

    /**
     * Remove a listener for events scoped to a {@link Transfer}.
     */
    public void removeTransferListener(Transfer transfer, SystemListener listener) {
        listeners.remove(new ScopedTransferListener(transfer, listener));
    }

    private static class ScopedWalletManagerListener implements DefaultSystemListener {

        private final WalletManager manager;
        private final SystemListener listener;

        ScopedWalletManagerListener(WalletManager manager, SystemListener listener) {
            this.manager = manager;
            this.listener = listener;
        }

        @Override
        public void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
            if (this.manager.equals(manager)) {
                listener.handleManagerEvent(system, manager, event);
            }
        }

        @Override
        public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
            if (this.manager.equals(manager)) {
                listener.handleWalletEvent(system, manager, wallet, event);
            }
        }

        @Override
        public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
            if (this.manager.equals(manager)) {
                listener.handleTransferEvent(system, manager, wallet, transfer, event);
            }
        }

        @Override
        public boolean equals(Object object) {
            if (this == object) {
                return true;
            }

            if (!(object instanceof ScopedWalletManagerListener)) {
                return false;
            }

            ScopedWalletManagerListener that = (ScopedWalletManagerListener) object;
            return manager.equals(that.manager) &&
                    listener.equals(that.listener);
        }

        @Override
        public int hashCode() {
            return Objects.hash(manager, listener);
        }
    }

    private static class ScopedWalletListener implements DefaultSystemListener {

        private final Wallet wallet;
        private final SystemListener listener;

        ScopedWalletListener(Wallet wallet, SystemListener listener) {
            this.wallet = wallet;
            this.listener = listener;
        }

        @Override
        public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
            if (this.wallet.equals(wallet)) {
                listener.handleWalletEvent(system, manager, wallet, event);
            }
        }

        @Override
        public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
            if (this.wallet.equals(wallet)) {
                listener.handleTransferEvent(system, manager, wallet, transfer, event);
            }
        }

        @Override
        public boolean equals(Object object) {
            if (this == object) {
                return true;
            }

            if (!(object instanceof ScopedWalletListener)) {
                return false;
            }

            ScopedWalletListener that = (ScopedWalletListener) object;
            return wallet.equals(that.wallet) &&
                    listener.equals(that.listener);
        }

        @Override
        public int hashCode() {
            return Objects.hash(wallet, listener);
        }
    }

    private static class ScopedTransferListener implements DefaultSystemListener {

        private final Transfer transfer;
        private final SystemListener listener;

        ScopedTransferListener(Transfer transfer, SystemListener listener) {
            this.transfer = transfer;
            this.listener = listener;
        }

        @Override
        public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
            if (this.transfer.equals(transfer)) {
                listener.handleTransferEvent(system, manager, wallet, transfer, event);
            }
        }

        @Override
        public boolean equals(Object object) {
            if (this == object) {
                return true;
            }

            if (!(object instanceof ScopedTransferListener)) {
                return false;
            }

            ScopedTransferListener that = (ScopedTransferListener) object;
            return transfer.equals(that.transfer) &&
                    listener.equals(that.listener);
        }

        @Override
        public int hashCode() {
            return Objects.hash(transfer, listener);
        }
    }
}
