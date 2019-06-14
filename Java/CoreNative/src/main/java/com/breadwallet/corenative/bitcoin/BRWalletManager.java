/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.bitcoin;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.breadwallet.corenative.crypto.BRCryptoBoolean;
import com.breadwallet.corenative.support.BRAddress;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedInts;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

import java.util.ArrayList;
import java.util.List;

public class BRWalletManager extends PointerType {

    public BRWalletManager(Pointer address) {
        super(address);
    }

    public BRWalletManager() {
        super();
    }

    public void generateUnusedAddrs(UnsignedInteger limit) {
        CryptoLibrary.INSTANCE.BRWalletManagerGenerateUnusedAddrs(this, limit.intValue());
    }

    public List<String> getAllAddrs() {
        SizeTByReference addressesCountReference = new SizeTByReference();
        BRAddress address = CryptoLibrary.INSTANCE.BRWalletManagerGetAllAddrs(this, addressesCountReference);
        try {
            List<String> addresses = new ArrayList<>();
            int addressesCount = UnsignedInts.checkedCast(addressesCountReference.getValue().longValue());
            for (BRAddress addr: (BRAddress[]) address.toArray(addressesCount)) {
                addresses.add(addr.getAddressAsString());
            }
            return new ArrayList<>(addresses);
        } finally {
            Native.free(Pointer.nativeValue(address.getPointer()));
        }
    }

    public List<String> getAllAddrsLegacy() {
        SizeTByReference addressesCountReference = new SizeTByReference();
        BRAddress address = CryptoLibrary.INSTANCE.BRWalletManagerGetAllAddrsLegacy(this, addressesCountReference);
        try {
            List<String> addresses = new ArrayList<>();
            int addressesCount = UnsignedInts.checkedCast(addressesCountReference.getValue().longValue());
            for (BRAddress addr: (BRAddress[]) address.toArray(addressesCount)) {
                addresses.add(addr.getAddressAsString());
            }
            return new ArrayList<>(addresses);
        } finally {
            Native.free(Pointer.nativeValue(address.getPointer()));
        }
    }

    public void announceBlockNumber(int rid, UnsignedLong blockNumber) {
        CryptoLibrary.INSTANCE.bwmAnnounceBlockNumber(this, rid, blockNumber.longValue());
    }

    public void announceSubmit(int rid, CoreBRTransaction transaction, int error) {
        CryptoLibrary.INSTANCE.bwmAnnounceSubmit(this, rid, transaction.asBRTransaction(), error);
    }

    public void announceTransaction(int rid, CoreBRTransaction transaction) {
        // TODO(discuss): We copy here so that we don't have our memory free'd from underneath us; is this OK?
        BRTransaction tx = transaction.asBRTransactionDeepCopy();
        CryptoLibrary.INSTANCE.bwmAnnounceTransaction(this, rid, tx);
    }

    public void announceTransactionComplete(int rid, boolean success) {
        CryptoLibrary.INSTANCE.bwmAnnounceTransactionComplete(this, rid, success ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE);
    }
}
