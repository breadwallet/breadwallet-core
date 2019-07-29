package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.crypto.BRCryptoWalletEstimateFeeBasisResult;
import com.sun.jna.Callback;
import com.sun.jna.Pointer;

public interface BRCryptoWalletEstimateFeeBasisCallback extends Callback {
    void apply(Pointer context, BRCryptoWalletEstimateFeeBasisResult.ByValue result);
}
