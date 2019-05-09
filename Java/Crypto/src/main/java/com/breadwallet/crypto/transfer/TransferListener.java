package com.breadwallet.crypto.transfer;

import com.breadwallet.crypto.system.System;
import com.breadwallet.crypto.wallet.Wallet;
import com.breadwallet.crypto.walletmanager.WalletManager;
import com.breadwallet.crypto.transfer.events.TranferEvent;

public interface TransferListener {

    void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event);
}
