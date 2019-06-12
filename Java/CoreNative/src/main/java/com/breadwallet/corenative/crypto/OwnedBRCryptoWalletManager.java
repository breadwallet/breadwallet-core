/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.google.common.primitives.UnsignedLong;

import java.util.Objects;

// TODO(fix): Add a finalize method with a call to cryptoWalletManagerGive()

/* package */
class OwnedBRCryptoWalletManager implements CoreBRCryptoWalletManager {

    private final BRCryptoWalletManager core;

    /* package */
    OwnedBRCryptoWalletManager(BRCryptoWalletManager core) {
        this.core = core;
    }

    @Override
    public CoreBRCryptoWallet getWallet() {
        return core.getWallet();
    }

    @Override
    public UnsignedLong getWalletsCount() {
        return core.getWalletsCount();
    }

    @Override
    public CoreBRCryptoWallet getWallet(UnsignedLong index) {
        return core.getWallet(index);
    }

    @Override
    public int getMode() {
        return core.getMode();
    }

    @Override
    public String getPath() {
        return core.getPath();
    }

    @Override
    public int getState() {
        return core.getState();
    }

    @Override
    public void connect() {
        core.connect();
    }

    @Override
    public void disconnect() {
        core.disconnect();
    }

    @Override
    public void sync() {
        core.sync();
    }

    @Override
    public void submit(CoreBRCryptoWallet wallet, CoreBRCryptoTransfer transfer, String paperKey) {
        core.submit(wallet, transfer, paperKey);
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof OwnedBRCryptoWalletManager) {
            OwnedBRCryptoWalletManager that = (OwnedBRCryptoWalletManager) object;
            return core.equals(that.core);
        }

        if (object instanceof BRCryptoWalletManager) {
            BRCryptoWalletManager that = (BRCryptoWalletManager) object;
            return core.equals(that);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
