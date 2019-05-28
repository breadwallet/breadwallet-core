package com.breadwallet.crypto;

import com.breadwallet.crypto.events.system.SystemListener;
import com.google.common.base.Optional;

import java.util.ArrayList;
import java.util.List;

// TODO: System is an unfortunate name in Java. How about CryptoManager or AccountManager?
public interface System {

    // TODO: Implement subscription
    void subscribe(String subscriptionToken);

    void initialize(List<String> networksNeeded);

    default void start() {
        for (WalletManager manager: getWalletManagers()) {
            manager.connect();
        }
    }

    default void stop() {
        for (WalletManager manager: getWalletManagers()) {
            manager.disconnect();
        }
    }

    default void sync() {
        for (WalletManager manager: getWalletManagers()) {
            manager.sync();
        }
    }

    void createWalletManager(Network network, WalletManagerMode mode);

    Optional<SystemListener> getSystemListener();

    Account getAccount();

    String getPath();

    List<Network> getNetworks();

    List<WalletManager> getWalletManagers();

    default List<Wallet> getWallets() {
        List<Wallet> wallets = new ArrayList<>();
        for (WalletManager manager: getWalletManagers()) {
            wallets.addAll(manager.getWallets());
        }
        return wallets;
    }
}
