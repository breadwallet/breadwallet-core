package com.breadwallet.crypto.jni.bitcoin;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoBoolean;
import com.breadwallet.crypto.jni.support.BRAddress;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

import java.util.ArrayList;
import java.util.List;

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
    public List<String> getUnusedAddrsLegacy(int limit) {
        BRAddress address = CryptoLibrary.INSTANCE.BRWalletManagerGetUnusedAddrsLegacy(this, limit);
        try {
            List<String> addresses = new ArrayList<>();
            for (BRAddress addr : (BRAddress[]) address.toArray(limit)) {
                addresses.add(addr.getAddressAsString());
            }
            return new ArrayList<>(addresses);
        } finally {
            Native.free(Pointer.nativeValue(address.getPointer()));
        }
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

    @Override
    public void announceBlockNumber(int rid, long blockNumber) {
        CryptoLibrary.INSTANCE.bwmAnnounceBlockNumber(this, rid, blockNumber);
    }

    @Override
    public void announceSubmit(int rid, CoreBRTransaction transaction, int error) {
        CryptoLibrary.INSTANCE.bwmAnnounceSubmit(this, rid, transaction.asBRTransaction(), error);
    }

    @Override
    public void announceTransaction(int rid, CoreBRTransaction transaction) {
        // TODO: We copy here so that we don't have our memory free'd from underneath us; is this OK?
        BRTransaction tx = transaction.asBRTransactionDeepCopy();
        CryptoLibrary.INSTANCE.bwmAnnounceTransaction(this, rid, tx);
    }

    @Override
    public void announceTransactionComplete(int rid, boolean success) {
        CryptoLibrary.INSTANCE.bwmAnnounceTransactionComplete(this, rid, success ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE);
    }
}
