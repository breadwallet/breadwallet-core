package com.breadwallet.crypto;

import com.breadwallet.crypto.events.system.SystemListener;
import com.google.common.base.Optional;

import java.util.ArrayList;
import java.util.List;

// TODO: System is an unfortunate name in Java. How about CryptoManager?
public interface System {

    void start(List<String> networksNeeded);

    void stop();

    void createWalletManager(Network network, WalletManagerMode mode);

    Optional<SystemListener> getSystemListener();

    Account getAccount();

    String getPath();

    // TODO: Add blockchain-db

    List<Network> getNetworks();

    List<WalletManager> getWalletManagers();

    default List<Wallet> getWallets() {
        List<Wallet> wallets = new ArrayList<>();
        for (WalletManager manager: getWalletManagers()) {
            wallets.addAll(manager.getWallets());
        }
        return null;
    }
}
