package com.breadwallet.cryptodemo;

import android.support.annotation.Nullable;
import android.util.Log;

import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.events.network.NetworkEvent;
import com.breadwallet.crypto.events.system.DefaultSystemEventVisitor;
import com.breadwallet.crypto.events.system.SystemEvent;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.system.SystemManagerAddedEvent;
import com.breadwallet.crypto.events.system.SystemNetworkAddedEvent;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.transfer.TransferListener;
import com.breadwallet.crypto.events.wallet.DefaultWalletEventVisitor;
import com.breadwallet.crypto.events.wallet.WalletCreatedEvent;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.wallet.WalletListener;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerListener;

import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.WeakHashMap;

public class CoreSystemListener implements SystemListener {

    private static final String TAG = CoreSystemListener.class.getName();


    private final Set<WalletManagerListener> walletManagerListeners = Collections.newSetFromMap(new WeakHashMap<>());
    private final Set<WalletListener> walletListeners = Collections.newSetFromMap(new WeakHashMap<>());
    private final Set<TransferListener> transferListeners = Collections.newSetFromMap(new WeakHashMap<>());

    private final WalletManagerMode mode;

    public CoreSystemListener(WalletManagerMode mode) {
        this.mode = mode;
    }

    public void addListener(WalletManagerListener listener) {
        synchronized (walletManagerListeners) {
            walletManagerListeners.add(listener);
        }
    }

    public void removeListener(WalletManagerListener  listener) {
        synchronized (walletManagerListeners) {
            walletManagerListeners.remove(listener);
        }
    }

    public void addListener(WalletListener listener) {
        synchronized (walletListeners) {
            walletListeners.add(listener);
        }
    }

    public void removeListener(WalletListener listener) {
        synchronized (walletListeners) {
            walletListeners.remove(listener);
        }
    }

    public void addListener(TransferListener listener) {
        synchronized (transferListeners) {
            transferListeners.add(listener);
        }
    }

    public void removeListener(TransferListener listener) {
        synchronized (transferListeners) {
            transferListeners.remove(listener);
        }
    }

    private Set<WalletListener> copyWalletListeners() {
        synchronized (walletListeners) {
            return new HashSet<>(walletListeners);
        }
    }

    private Set<WalletManagerListener> copyWalletManagerListeners() {
        synchronized (walletManagerListeners) {
            return new HashSet<>(walletManagerListeners);
        }
    }

    private Set<TransferListener> copyTransferListeners() {
        synchronized (transferListeners) {
            return new HashSet<>(transferListeners);
        }
    }

    @Override
    public void handleSystemEvent(System system, SystemEvent event) {
        Log.d(TAG, String.format("System: %s", event));
        event.accept(new DefaultSystemEventVisitor<Void>() {
            @Nullable
            @Override
            public Void visit(SystemManagerAddedEvent event) {
                WalletManager manager = event.getWalletManager();
                manager.connect();
                return null;
            }

            @Nullable
            @Override
            public Void visit(SystemNetworkAddedEvent event) {
                List<WalletManagerMode> supportedModes = event.getNetwork().getSupportedModes();
                system.createWalletManager(event.getNetwork(), supportedModes.contains(mode) ? mode : supportedModes.get(0));
                return null;
            }
        });
    }

    @Override
    public void handleNetworkEvent(System system, Network network, NetworkEvent event) {
        Log.d(TAG, String.format("Network: %s", event));
    }

    @Override
    public void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
        Log.d(TAG, String.format("Manager (%s): %s", manager.getName(), event));
        for(WalletManagerListener listener: copyWalletManagerListeners()) {
            listener.handleManagerEvent(system, manager, event);
        }
    }

    @Override
    public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        Log.d(TAG, String.format("Wallet (%s:%s): %s", manager.getName(), wallet.getName(), event));
        for(WalletListener listener: copyWalletListeners()) {
            listener.handleWalletEvent(system, manager, wallet, event);
        }

        event.accept(new DefaultWalletEventVisitor<Void>() {
            @Nullable
            @Override
            public Void visit(WalletCreatedEvent event) {
                Wallet wallet = manager.getPrimaryWallet();
                Log.d(TAG, String.format("Wallet addresses: %s <--> %s", wallet.getSource(), wallet.getTarget()));
                return null;
            }
        });
    }

    @Override
    public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
        Log.d(TAG, String.format("Transfer (%s:%s): %s", manager.getName(), wallet.getName(), event));
        for(TransferListener listener: copyTransferListeners()) {
            listener.handleTransferEvent(system, manager, wallet, transfer, event);
        }
    }
}
