/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoWalletManager extends PointerType implements CoreBRCryptoWalletManager {

    public BRCryptoWalletManager(Pointer address) {
        super(address);
    }

    public BRCryptoWalletManager() {
        super();
    }

    @Override
    public CoreBRCryptoWallet getWallet() {
        BRCryptoWallet wallet = CryptoLibrary.INSTANCE.cryptoWalletManagerGetWallet(this);
        wallet = CryptoLibrary.INSTANCE.cryptoWalletTake(wallet);
        return new OwnedBRCryptoWallet(wallet);
    }

    @Override
    public UnsignedLong getWalletsCount() {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.cryptoWalletManagerGetWalletsCount(this).longValue());
    }

    @Override
    public CoreBRCryptoWallet getWallet(UnsignedLong index) {
        BRCryptoWallet wallet = CryptoLibrary.INSTANCE.cryptoWalletManagerGetWalletAtIndex(this, new SizeT(index.longValue()));
        wallet = CryptoLibrary.INSTANCE.cryptoWalletTake(wallet);
        return new OwnedBRCryptoWallet(wallet);
    }

    @Override
    public int getMode() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetMode(this);
    }

    @Override
    public String getPath() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetPath(this).getString(0, "UTF-8");
    }

    @Override
    public int getState() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetState(this);
    }

    @Override
    public void connect() {
        CryptoLibrary.INSTANCE.cryptoWalletManagerConnect(this);
    }

    @Override
    public void disconnect() {
        CryptoLibrary.INSTANCE.cryptoWalletManagerDisconnect(this);
    }

    @Override
    public void sync() {
        CryptoLibrary.INSTANCE.cryptoWalletManagerSync(this);
    }

    @Override
    public void submit(CoreBRCryptoWallet wallet, CoreBRCryptoTransfer transfer, String paperKey) {
        CryptoLibrary.INSTANCE.cryptoWalletManagerSubmit(this, wallet.asBRCryptoWallet(), transfer.asBRCryptoTransfer(), paperKey);
    }
}
