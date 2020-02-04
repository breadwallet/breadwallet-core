/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.cryptodemo;

import android.support.annotation.Nullable;

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
import java.util.Collections;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import static com.google.common.base.Preconditions.checkState;

public class CoreSystemListener implements SystemListener {

    private static final Logger Log = Logger.getLogger(CoreSystemListener.class.getName());

    private final WalletManagerMode preferredMode;
    private final boolean isMainnet;
    private final List<String> currencyCodesNeeded;

    /* package */
    CoreSystemListener(WalletManagerMode preferredMode, boolean isMainnet, List<String> currencyCodesNeeded) {
        this.preferredMode = preferredMode;
        this.isMainnet = isMainnet;
        this.currencyCodesNeeded = new ArrayList<>(currencyCodesNeeded);
    }

    // SystemListener Handlers

    @Override
    public void handleSystemEvent(System system, SystemEvent event) {
        ApplicationExecutors.runOnBlockingExecutor(() -> {
            Log.log(Level.FINE, String.format("System: %s", event));

            event.accept(new DefaultSystemEventVisitor<Void>() {
                @Nullable
                @Override
                public Void visit(SystemNetworkAddedEvent event) {
                    createWalletManager(system, event.getNetwork());
                    return null;
                }

                @Nullable
                @Override
                public Void visit(SystemManagerAddedEvent event) {
                    connectWalletManager(event.getWalletManager());
                    return null;
                }

                @Nullable
                @Override
                public Void visit(SystemDiscoveredNetworksEvent event) {
                    logDiscoveredCurrencies(event.getNetworks());
                    return null;
                }
            });
        });
    }

    @Override
    public void handleNetworkEvent(System system, Network network, NetworkEvent event) {
        ApplicationExecutors.runOnBlockingExecutor(() -> {
            Log.log(Level.FINE, String.format("Network: %s", event));
        });
    }

    @Override
    public void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
        ApplicationExecutors.runOnBlockingExecutor(() -> {
            Log.log(Level.FINE, String.format("Manager (%s): %s", manager.getName(), event));
        });
    }

    @Override
    public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        ApplicationExecutors.runOnBlockingExecutor(() -> {
            Log.log(Level.FINE, String.format("Wallet (%s:%s): %s", manager.getName(), wallet.getName(), event));

            event.accept(new DefaultWalletEventVisitor<Void>() {
                @Nullable
                @Override
                public Void visit(WalletCreatedEvent event) {
                    logWalletAddresses(wallet);
                    return null;
                }
            });
        });
    }

    @Override
    public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
        ApplicationExecutors.runOnBlockingExecutor(() -> {
            Log.log(Level.FINE, String.format("Transfer (%s:%s): %s", manager.getName(), wallet.getName(), event));
        });
    }

    // Misc.

    private void createWalletManager(System system, Network network) {
        boolean isNetworkNeeded = false;
        for (String currencyCode : currencyCodesNeeded) {
            Optional<? extends Currency> currency = network.getCurrencyByCode(currencyCode);
            if (currency.isPresent()) {
                isNetworkNeeded = true;
                break;
            }
        }

        if (isMainnet == network.isMainnet() && isNetworkNeeded) {
            WalletManagerMode mode = network.supportsWalletManagerMode(preferredMode) ?
                    preferredMode : network.getDefaultWalletManagerMode();

            AddressScheme addressScheme = network.getDefaultAddressScheme();
            Log.log(Level.FINE, String.format("Creating %s WalletManager with %s and %s", network, mode, addressScheme));
            boolean success = system.createWalletManager(network, mode, addressScheme, Collections.emptySet());
            if (!success) {
                system.wipe(network);
                checkState(system.createWalletManager(network, mode, addressScheme, Collections.emptySet()));
            }
        }
    }

    private void connectWalletManager(WalletManager walletManager) {
        walletManager.connect(null);
    }

    private void logWalletAddresses(Wallet wallet) {
        Log.log(Level.FINE, String.format("Wallet (target) addresses: %s", wallet.getTarget()));
    }

    private void logDiscoveredCurrencies(List<Network> networks) {
        for (Network network: networks) {
            for (Currency currency: network.getCurrencies()) {
                Log.log(Level.FINE, String.format("Discovered: %s for %s", currency.getCode(), network.getName()));
            }
        };
    }
}
