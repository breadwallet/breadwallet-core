package com.breadwallet.crypto.libcrypto.crypto;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;
import com.breadwallet.crypto.libcrypto.support.BRMasterPubKey;

/* package */
class OwnedBRCryptoAccount implements CoreBRCryptoAccount {

    private final BRCryptoAccount core;

    /* package */
    OwnedBRCryptoAccount(BRCryptoAccount core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoAccountGive(core);
        }
    }

    @Override
    public long getTimestamp() {
        return core.getTimestamp();
    }

    @Override
    public void setTimestamp(long timestamp) {
        core.setTimestamp(timestamp);
    }

    @Override
    public BRMasterPubKey.ByValue asBtc() {
        return core.asBtc();
    }

    @Override
    public BRCryptoAccount asBRCryptoAccount() {
        return core;
    }
}
