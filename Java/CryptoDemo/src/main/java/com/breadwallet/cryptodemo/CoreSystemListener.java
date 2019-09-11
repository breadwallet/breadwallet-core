package com.breadwallet.cryptodemo;

import android.support.annotation.Nullable;
import android.util.Log;

import com.breadwallet.crypto.AddressScheme;
import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.events.network.NetworkEvent;
import com.breadwallet.crypto.events.system.DefaultSystemEventVisitor;
import com.breadwallet.crypto.events.system.SystemDiscoveredNetworksEvent;
import com.breadwallet.crypto.events.system.SystemEvent;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.system.SystemManagerAddedEvent;
import com.breadwallet.crypto.events.system.SystemNetworkAddedEvent;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.wallet.DefaultWalletEventVisitor;
import com.breadwallet.crypto.events.wallet.WalletCreatedEvent;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;
import com.google.common.base.Optional;

import java.util.ArrayList;
import java.util.List;

public class CoreSystemListener implements SystemListener {

    private static final String TAG = CoreSystemListener.class.getName();

    private final WalletManagerMode mode;
    private final boolean isMainnet;
    private final List<String> currencyCodesNeeded;

    /* package */
    CoreSystemListener(WalletManagerMode mode, boolean isMainnet, List<String> currencyCodesNeeded) {
        this.mode = mode;
        this.isMainnet = isMainnet;
        this.currencyCodesNeeded = new ArrayList<>(currencyCodesNeeded);
    }

    @Override
    public void handleSystemEvent(System system, SystemEvent event) {
        Log.d(TAG, String.format("System: %s", event));
        event.accept(new DefaultSystemEventVisitor<Void>() {
            @Nullable
            @Override
            public Void visit(SystemManagerAddedEvent event) {
                WalletManager manager = event.getWalletManager();
                manager.setNetworkReachable(Utilities.isNetworkReachable(CoreCryptoApplication.getContext()));
                manager.connect();
                return null;
            }

            @Nullable
            @Override
            public Void visit(SystemNetworkAddedEvent event) {
                Network network = event.getNetwork();

                boolean isNetworkNeeded = false;
                for (String currencyCode: currencyCodesNeeded) {
                    Optional<? extends Currency> currency = network.getCurrencyByCode(currencyCode);
                    if (currency.isPresent()) {
                        isNetworkNeeded = true;
                        break;
                    }
                }

                if (isMainnet == network.isMainnet() && isNetworkNeeded) {
                    WalletManagerMode wmMode = system.supportsWalletManagerModes(network, mode) ?
                            mode : system.getDefaultWalletManagerMode(network);

                    AddressScheme addressScheme = system.getDefaultAddressScheme(network);
                    system.createWalletManager(event.getNetwork(), wmMode, addressScheme);
                }
                return null;
            }

            @Nullable
            @Override
            public Void visit(SystemDiscoveredNetworksEvent event) {
                Log.d(TAG, String.format("Currencies (Discovered): %s", event));
                for (Network network: event.getNetworks()) {
                    for (Currency currency: network.getCurrencies()) {
                        Log.d(TAG, String.format("    Currency: %s for %s", currency.getCode(), network.getName()));
                    }
                }
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
    }

    @Override
    public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        Log.d(TAG, String.format("Wallet (%s:%s): %s", manager.getName(), wallet.getName(), event));

        event.accept(new DefaultWalletEventVisitor<Void>() {
            @Nullable
            @Override
            public Void visit(WalletCreatedEvent event) {
                Log.d(TAG, String.format("Wallet addresses: %s <--> %s", wallet.getSource(), wallet.getTarget()));
                return null;
            }
        });
    }

    @Override
    public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
        Log.d(TAG, String.format("Transfer (%s:%s): %s", manager.getName(), wallet.getName(), event));
    }
}
