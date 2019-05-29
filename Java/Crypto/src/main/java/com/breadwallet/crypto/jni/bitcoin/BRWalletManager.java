package com.breadwallet.crypto.jni.bitcoin;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRWalletManager extends PointerType implements CoreBRWalletManager {

    public BRWalletManager(Pointer address) {
        super(address);
    }

    public BRWalletManager() {
        super();
    }

    @Override
    public BRWallet getWallet() {
        return CryptoLibrary.INSTANCE.BRWalletManagerGetWallet(this);
    }

    @Override
    public BRPeerManager getPeerManager() {
        return CryptoLibrary.INSTANCE.BRWalletManagerGetPeerManager(this);
    }

    @Override
    public void connect() {
        CryptoLibrary.INSTANCE.BRWalletManagerConnect(this);
    }

    @Override
    public void disconnect() {
        CryptoLibrary.INSTANCE.BRWalletManagerDisconnect(this);
    }

    @Override
    public void scan() {
        CryptoLibrary.INSTANCE.BRWalletManagerScan(this);
    }

    @Override
    public boolean matches(BRWalletManager o) {
        return this.equals(o);
    }
}
