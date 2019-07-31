package com.breadwallet.crypto;

import com.breadwallet.crypto.events.network.NetworkEvent;
import com.breadwallet.crypto.events.system.SystemEvent;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.transfer.TransferListener;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.wallet.WalletListener;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerListener;

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

    public void addSystemListener(SystemListener listener) {
        listeners.add(listener);
    }

    public void removeSystemListener(SystemListener listener) {
        listeners.remove(listener);
    }

    // WalletManagerListener registration

    public void addWalletManagerListener(WalletManager manager, WalletManagerListener listener) {
        listeners.add(new ScopedWalletManagerListener(manager, listener));
    }

    public void removeWalletManagerListener(WalletManager manager, WalletManagerListener listener) {
        listeners.remove(new ScopedWalletManagerListener(manager, listener));
    }

    // WalletListener registration

    public void addWalletListener(Wallet wallet, WalletListener listener) {
        listeners.add(new ScopedWalletListener(wallet, listener));
    }

    public void removeWalletListener(Wallet wallet, WalletListener listener) {
        listeners.remove(new ScopedWalletListener(wallet, listener));
    }

    // TransferListener registration

    public void addTransferListener(Transfer transfer, TransferListener listener) {
        listeners.add(new ScopedTransferListener(transfer, listener));
    }

    public void removeTransferListener(Transfer transfer, TransferListener listener) {
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

    private static class ScopedWalletManagerListener implements DefaultSystemListener {

        private final WalletManager manager;
        private final WalletManagerListener listener;

        ScopedWalletManagerListener(WalletManager manager, WalletManagerListener listener) {
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
        private final WalletListener listener;

        ScopedWalletListener(Wallet wallet, WalletListener listener) {
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
        private final TransferListener listener;

        ScopedTransferListener(Transfer transfer, TransferListener listener) {
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
