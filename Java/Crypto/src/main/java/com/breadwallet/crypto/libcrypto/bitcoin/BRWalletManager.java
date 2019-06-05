/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.libcrypto.bitcoin;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;
import com.breadwallet.crypto.libcrypto.utility.SizeT;
import com.breadwallet.crypto.libcrypto.utility.SizeTByReference;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoBoolean;
import com.breadwallet.crypto.libcrypto.support.BRAddress;
import com.google.common.primitives.UnsignedInts;
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
    public void generateUnusedAddrs(int limit) {
        CryptoLibrary.INSTANCE.BRWalletManagerGenerateUnusedAddrs(this, limit);
    }

    @Override
    public List<String> getAllAddrs() {
        SizeTByReference addressesCountReference = new SizeTByReference();
        BRAddress address = CryptoLibrary.INSTANCE.BRWalletManagerGetAllAddrs(this, addressesCountReference);
        try {
            List<String> addresses = new ArrayList<>();
            int addressesCount = UnsignedInts.checkedCast(addressesCountReference.getValue());
            for (BRAddress addr: (BRAddress[]) address.toArray(addressesCount)) {
                addresses.add(addr.getAddressAsString());
            }
            return new ArrayList<>(addresses);
        } finally {
            Native.free(Pointer.nativeValue(address.getPointer()));
        }
    }

    @Override
    public List<String> getAllAddrsLegacy() {
        SizeTByReference addressesCountReference = new SizeTByReference();
        BRAddress address = CryptoLibrary.INSTANCE.BRWalletManagerGetAllAddrsLegacy(this, addressesCountReference);
        try {
            List<String> addresses = new ArrayList<>();
            int addressesCount = UnsignedInts.checkedCast(addressesCountReference.getValue());
            for (BRAddress addr: (BRAddress[]) address.toArray(addressesCount)) {
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
    public void submitTransaction(CoreBRTransaction transaction, byte[] seed) {
        // TODO(discuss): We copy here so that we don't have our memory free'd from underneath us; is this OK?
        BRTransaction tx = transaction.asBRTransactionDeepCopy();
        CryptoLibrary.INSTANCE.BRWalletManagerSubmitTransaction(this, tx, seed, new SizeT(seed.length));
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
        // TODO(discuss): We copy here so that we don't have our memory free'd from underneath us; is this OK?
        BRTransaction tx = transaction.asBRTransactionDeepCopy();
        CryptoLibrary.INSTANCE.bwmAnnounceTransaction(this, rid, tx);
    }

    @Override
    public void announceTransactionComplete(int rid, boolean success) {
        CryptoLibrary.INSTANCE.bwmAnnounceTransactionComplete(this, rid, success ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE);
    }
}
