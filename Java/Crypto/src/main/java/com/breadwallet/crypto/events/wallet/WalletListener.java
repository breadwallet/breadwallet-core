package com.breadwallet.crypto.events.wallet;

import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.WalletManager;

public interface WalletListener {

    void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event);
}
