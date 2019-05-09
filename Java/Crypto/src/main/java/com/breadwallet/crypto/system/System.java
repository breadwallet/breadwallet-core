package com.breadwallet.crypto.system;

import com.breadwallet.crypto.Account;
import com.breadwallet.crypto.network.Network;
import com.breadwallet.crypto.wallet.Wallet;
import com.breadwallet.crypto.walletmanager.WalletManager;
import com.breadwallet.crypto.walletmanager.WalletManagerMode;
import com.google.common.base.Optional;

import java.util.ArrayList;
import java.util.List;

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
