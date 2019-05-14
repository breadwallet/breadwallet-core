package com.breadwallet.crypto.events.transfer;

import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.Transfer;

public interface TransferListener {

    void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event);
}
