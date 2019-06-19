/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.primitives.UnsignedLong;

import java.util.Objects;

/* package */
class OwnedBRCryptoWalletManager implements CoreBRCryptoWalletManager {

    private final BRCryptoWalletManager core;

    /* package */
    OwnedBRCryptoWalletManager(BRCryptoWalletManager core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoWalletManagerGive(core);
        }
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
    public boolean containsWallet(CoreBRCryptoWallet wallet) {
        return core.containsWallet(wallet);
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
    public void announceBlockNumber(BRCryptoCallbackHandle handle, UnsignedLong blockchainHeight, boolean success) {
        core.announceBlockNumber(handle, blockchainHeight, success);
    }

    @Override
    public void announceTransaction(BRCryptoCallbackHandle handle, byte[] transaction, UnsignedLong timestamp,
                                    UnsignedLong blockHeight) {
        core.announceTransaction(handle, transaction, timestamp, blockHeight);
    }

    @Override
    public void announceTransactionComplete(BRCryptoCallbackHandle handle, boolean success) {
        core.announceTransactionComplete(handle, success);
    }

    @Override
    public void announceSubmit(BRCryptoCallbackHandle handle, boolean success) {
        core.announceSubmit(handle, success);
    }

    @Override
    public void announceBalance(BRCryptoCallbackHandle handle, UnsignedLong balance, boolean success) {
        core.announceBalance(handle, balance, success);
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
