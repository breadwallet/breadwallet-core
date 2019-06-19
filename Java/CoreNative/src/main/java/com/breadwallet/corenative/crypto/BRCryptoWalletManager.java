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
        return new OwnedBRCryptoWallet(CryptoLibrary.INSTANCE.cryptoWalletManagerGetWallet(this));
    }

    @Override
    public UnsignedLong getWalletsCount() {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.cryptoWalletManagerGetWalletsCount(this).longValue());
    }

    @Override
    public CoreBRCryptoWallet getWallet(UnsignedLong index) {
        return new OwnedBRCryptoWallet(CryptoLibrary.INSTANCE.cryptoWalletManagerGetWalletAtIndex(this, new SizeT(index.longValue())));
    }

    @Override
    public boolean containsWallet(CoreBRCryptoWallet wallet) {
        return  BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoWalletManagerHasWallet(this, wallet.asBRCryptoWallet());
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

    @Override
    public void announceBlockNumber(BRCryptoCallbackHandle handle, UnsignedLong blockchainHeight, boolean success) {
        CryptoLibrary.INSTANCE.cwmAnnounceBlockNumber(this, handle, blockchainHeight.longValue(), success ?
                BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE);
    }

    @Override
    public void announceTransaction(BRCryptoCallbackHandle handle, byte[] transaction, UnsignedLong timestamp,
                                    UnsignedLong blockHeight) {
        CryptoLibrary.INSTANCE.cwmAnnounceTransaction(this, handle, transaction, new SizeT(transaction.length),
                timestamp.longValue(), blockHeight.longValue());
    }

    @Override
    public void announceTransactionComplete(BRCryptoCallbackHandle handle, boolean success) {
        CryptoLibrary.INSTANCE.cwmAnnounceTransactionComplete(this, handle, success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    @Override
    public void announceSubmit(BRCryptoCallbackHandle handle, boolean success) {
        CryptoLibrary.INSTANCE.cwmAnnounceSubmit(this, handle, success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }
}
