/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.events.network.NetworkEvent;
import com.breadwallet.crypto.events.system.SystemEvent;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.transfer.TransferListener;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.wallet.WalletListener;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerListener;

import java.lang.ref.WeakReference;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArraySet;
import java.util.concurrent.ExecutorService;

/* package */
final class SystemEventCoordinator {

    private final ExecutorService executor;
    private final System system;

    private final CopyOnWriteArraySet<SystemListener> listeners;

    /* package */
    SystemEventCoordinator(ExecutorService executor, System system, SystemListener baseListener) {
        this.executor = executor;
        this.system = system;
        this.listeners = new CopyOnWriteArraySet<>();
        this.listeners.add(new WeakSystemListener(baseListener));
    }

    // Event callbacks

    /* package */
    void announceSystemEvent(SystemEvent event) {
        Set<SystemListener> listeners = getListeners();
        executor.submit(() -> {
            for (SystemListener listener: listeners) {
                listener.handleSystemEvent(system, event);
            }
        });
    }

    /* package */
    void announceNetworkEvent(Network network, NetworkEvent event) {
        Set<SystemListener> listeners = getListeners();
        executor.submit(() -> {
            for (SystemListener listener: listeners) {
                listener.handleNetworkEvent(system, network, event);
            }
        });
    }

    /* package */
    void announceWalletManagerEvent(WalletManager walletManager, WalletManagerEvent event) {
        Set<SystemListener> listeners = getListeners();
        executor.submit(() -> {
            for (SystemListener listener: listeners) {
                listener.handleManagerEvent(system, walletManager, event);
            }
        });
    }

    /* package */
    void announceWalletEvent(WalletManager walletManager, Wallet wallet, WalletEvent event) {
        Set<SystemListener> listeners = getListeners();
        executor.submit(() -> {
            for (SystemListener listener: listeners) {
                listener.handleWalletEvent(system, walletManager, wallet, event);
            }
        });
    }

    /* package */
    void announceTransferEvent(WalletManager walletManager, Wallet wallet, Transfer transfer, TranferEvent event) {
        Set<SystemListener> listeners = getListeners();
        executor.submit(() -> {
            for (SystemListener listener: listeners) {
                listener.handleTransferEvent(system, walletManager, wallet, transfer, event);
            }
        });
    }

    // Filtered Listeners

    private Set<SystemListener> getListeners() {
        return listeners;
    }

    /* package */
    void addSystemListener(SystemListener listener) {
        listeners.add(new WeakSystemListener(listener));
    }

    /* package */
    void removeSystemListener(SystemListener listener) {
        listeners.remove(new WeakSystemListener(listener));
    }


    /* package */
    void addWalletManagerListener(WalletManager manager, WalletManagerListener listener) {
        listeners.add(new ScopedWalletManagerListener(manager, listener));
    }

    /* package */
    void removeWalletManagerListener(WalletManager manager, WalletManagerListener listener) {
        listeners.remove(new ScopedWalletManagerListener(manager, listener));
    }

    /* package */
    void addWalletListener(Wallet wallet, WalletListener listener) {
        listeners.add(new ScopedWalletListener(wallet, listener));
    }

    /* package */
    void removeWalletListener(Wallet wallet, WalletListener listener) {
        listeners.remove(new ScopedWalletListener(wallet, listener));
    }


    /* package */
    void addTransferListener(Transfer transfer, TransferListener listener) {
        listeners.add(new ScopedTransferListener(transfer, listener));
    }

    /* package */
    void removeTransferListener(Transfer transfer, TransferListener listener) {
        listeners.remove(new ScopedTransferListener(transfer, listener));
    }


    private interface DefaultSystemListener extends SystemListener {
        default void handleSystemEvent(System system, SystemEvent event) {
        }

        default void handleNetworkEvent(System system, Network network, NetworkEvent event) {
        }

        default void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
        }

        default void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        }

        default void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
        }
    }

    private static class WeakSystemListener implements SystemListener {
        private final WeakReference<SystemListener> listener;

        WeakSystemListener(SystemListener listener) {
            this.listener = new WeakReference<>(listener);
        }

        @Override
        public void handleSystemEvent(System system, SystemEvent event) {
            SystemListener listener = this.listener.get();
            if (null != listener) {
                listener.handleSystemEvent(system, event);
            }
        }

        @Override
        public void handleNetworkEvent(System system, Network network, NetworkEvent event) {
            SystemListener listener = this.listener.get();
            if (null != listener) {
                listener.handleNetworkEvent(system, network, event);
            }
        }

        @Override
        public void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
            SystemListener listener = this.listener.get();
            if (null != listener) {
                listener.handleManagerEvent(system, manager, event);
            }
        }

        @Override
        public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
            SystemListener listener = this.listener.get();
            if (null != listener) {
                listener.handleWalletEvent(system, manager, wallet, event);
            }
        }

        @Override
        public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
            SystemListener listener = this.listener.get();
            if (null != listener) {
                listener.handleTransferEvent(system, manager, wallet, transfer, event);
            }
        }
    }

    private static class ScopedWalletManagerListener implements DefaultSystemListener {

        private final int filterHash;
        private final int identityHash;
        private final WeakReference<WalletManagerListener> listener;

        ScopedWalletManagerListener(WalletManager manager, WalletManagerListener listener) {
            this.filterHash = Objects.hash(manager);
            this.identityHash = Objects.hash(manager, listener);
            this.listener = new WeakReference<>(listener);
        }

        @Override
        public void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
            if (Objects.hash(manager) == filterHash) {
                WalletManagerListener listener = this.listener.get();
                if (null != listener) {
                    listener.handleManagerEvent(system, manager, event);
                }
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
            return identityHash == that.identityHash;
        }

        @Override
        public int hashCode() {
            return identityHash;
        }
    }

    private static class ScopedWalletListener implements DefaultSystemListener {

        private final int filterHash;
        private final int identityHash;
        private final WeakReference<WalletListener> listener;

        ScopedWalletListener(Wallet wallet, WalletListener listener) {
            this.filterHash = Objects.hash(wallet);
            this.identityHash = Objects.hash(wallet, listener);
            this.listener = new WeakReference<>(listener);
        }

        @Override
        public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
            if (Objects.hash(wallet) == filterHash) {
                WalletListener listener = this.listener.get();
                if (null != listener) {
                    listener.handleWalletEvent(system, manager, wallet, event);
                }
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
            return identityHash == that.identityHash;
        }

        @Override
        public int hashCode() {
            return identityHash;
        }
    }

    private static class ScopedTransferListener implements DefaultSystemListener {

        private final int filterHash;
        private final int identityHash;
        private final WeakReference<TransferListener> listener;

        ScopedTransferListener(Transfer transfer, TransferListener listener) {
            this.filterHash = Objects.hash(transfer);
            this.identityHash = Objects.hash(transfer, listener);
            this.listener = new WeakReference<>(listener);
        }

        @Override
        public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
            if (Objects.hash(transfer) == filterHash) {
                TransferListener listener = this.listener.get();
                if (null != listener) {
                    listener.handleTransferEvent(system, manager, wallet, transfer, event);
                }
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
            return identityHash == that.identityHash;
        }

        @Override
        public int hashCode() {
            return identityHash;
        }
    }
}
