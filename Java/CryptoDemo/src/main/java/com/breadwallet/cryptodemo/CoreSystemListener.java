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
import com.breadwallet.crypto.events.system.SystemCreatedEvent;
import com.breadwallet.crypto.events.system.SystemEvent;
import com.breadwallet.crypto.events.system.SystemEventVisitor;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.system.SystemManagerAddedEvent;
import com.breadwallet.crypto.events.system.SystemNetworkAddedEvent;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.transfer.TransferListener;
import com.breadwallet.crypto.events.wallet.DefaultWalletEventVisitor;
import com.breadwallet.crypto.events.wallet.WalletBalanceUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletCreatedEvent;
import com.breadwallet.crypto.events.wallet.WalletDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.wallet.WalletEventVisitor;
import com.breadwallet.crypto.events.wallet.WalletFeeBasisUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletListener;
import com.breadwallet.crypto.events.wallet.WalletTransferAddedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferSubmittedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;

import java.util.ArrayList;
import java.util.List;

public class CoreSystemListener implements SystemListener {

    private static final String TAG = CoreSystemListener.class.getName();

    private final List<WalletListener> walletListeners = new ArrayList<>();
    private final List<TransferListener> transferListeners = new ArrayList<>();

    private final WalletManagerMode mode;

    public CoreSystemListener(WalletManagerMode mode) {
        this.mode = mode;
    }

    public void addListener(WalletListener listener) {
        walletListeners.add(listener);
    }

    public void removeListener(WalletListener listener) {
        walletListeners.remove(listener);
    }

    public void addListener(TransferListener listener) {
        transferListeners.add(listener);
    }

    public void removeListener(TransferListener listener) {
        transferListeners.remove(listener);
    }

    @Override
    public void handleSystemEvent(System system, SystemEvent event) {
        Log.d(TAG, event.toString());
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
                system.createWalletManager(event.getNetwork(), mode);
                return null;
            }
        });
    }

    @Override
    public void handleNetworkEvent(System system, Network network, NetworkEvent event) {
        Log.d(TAG, event.toString());
    }

    @Override
    public void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
        Log.d(TAG, event.toString());
    }

    @Override
    public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        Log.d(TAG, event.toString());
        for(WalletListener listener: walletListeners) {
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
        Log.d(TAG, event.toString());
        for(TransferListener listener: transferListeners) {
            listener.handleTransferEvent(system, manager, wallet, transfer, event);
        }
    }
}
