package com.breadwallet.crypto.wallet;

import com.breadwallet.crypto.system.System;
import com.breadwallet.crypto.walletmanager.WalletManager;
import com.breadwallet.crypto.wallet.events.WalletEvent;

public interface WalletListener {

    void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event);
}
