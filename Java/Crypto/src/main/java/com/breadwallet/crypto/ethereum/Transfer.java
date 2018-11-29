package com.breadwallet.crypto.ethereum;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.Wallet;

class Transfer extends com.breadwallet.crypto.Transfer {
    long core;

    private Transfer(long core, Wallet wallet, Address source, Address target, Amount amount, Amount fee) {
        super(wallet, source, target, amount, fee);
        this.core = core;
    }

    protected Transfer (Wallet wallet, long core) {
        this (core, wallet, null, null, null, null);
    }

}
