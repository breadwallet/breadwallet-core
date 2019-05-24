package com.breadwallet.cryptodemo;

import android.util.Log;

import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.events.network.NetworkEvent;
import com.breadwallet.crypto.events.system.SystemCreatedEvent;
import com.breadwallet.crypto.events.system.SystemEvent;
import com.breadwallet.crypto.events.system.SystemEventVisitor;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.system.SystemManagerAddedEvent;
import com.breadwallet.crypto.events.system.SystemNetworkAddedEvent;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.transfer.TransferListener;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.wallet.WalletListener;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;

import java.util.ArrayList;
import java.util.List;

public class CoreSystemListener implements SystemListener {

    private final List<WalletListener> walletListeners = new ArrayList<>();
    private final List<TransferListener> transferListeners = new ArrayList<>();

    private static final String TAG = CoreSystemListener.class.getName();

    public void addListener(WalletListener listener) {
        walletListeners.add(listener);
    }

    public void addListener(TransferListener listener) {
        transferListeners.add(listener);
    }

    @Override
    public void handleSystemEvent(System system, SystemEvent event) {
        Log.d(TAG, event.toString());

        event.accept(new SystemEventVisitor<Void>() {
            @Override
            public Void visit(SystemCreatedEvent event) {
                return null;
            }

            @Override
            public Void visit(SystemManagerAddedEvent event) {
                WalletManager manager = event.getWalletManager();
                manager.connect();
                return null;
            }

            @Override
            public Void visit(SystemNetworkAddedEvent event) {
                Network network = event.getNetwork();
                WalletManagerMode mode = WalletManagerMode.P2P_ONLY;
                system.createWalletManager(network, mode);
                return null;
            }
        });
    }

    @Override
    public void handleNetworkEvent(System system, Network network, NetworkEvent event) {
        Log.d(TAG, event.toString());
    }

    @Override
    public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
        Log.d(TAG, event.toString());
        for(TransferListener listener: transferListeners) {
            listener.handleTransferEvent(system, manager, wallet, transfer, event);
        }
    }

    @Override
    public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        Log.d(TAG, event.toString());
        for(WalletListener listener: walletListeners) {
            listener.handleWalletEvent(system, manager, wallet, event);
        }
    }

    @Override
    public void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
        Log.d(TAG, event.toString());
    }
}
