/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.libcrypto.bitcoin;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoBoolean;
import com.breadwallet.crypto.libcrypto.utility.SizeT;
import com.breadwallet.crypto.libcrypto.support.BRAddress;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRWallet extends PointerType {

    public static final UnsignedLong DEFAULT_FEE_PER_KB = UnsignedLong.fromLongBits(1000L * 10);

    public BRWallet(Pointer address) {
        super(address);
    }

    public BRWallet() {
        super();
    }

    public UnsignedLong getBalance() {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.BRWalletBalance(this));
    }

    public UnsignedLong getFeeForTx(CoreBRTransaction tx) {
        BRTransaction coreTransfer = tx.asBRTransaction();
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.BRWalletFeeForTx(this, coreTransfer));
    }

    public UnsignedLong getFeeForTxAmount(UnsignedLong amount) {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.BRWalletFeeForTxAmount(this, amount.longValue()));
    }

    public UnsignedLong getFeePerKb() {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.BRWalletFeePerKb(this));
    }

    public void setFeePerKb(UnsignedLong feePerKb) {
        CryptoLibrary.INSTANCE.BRWalletSetFeePerKb(this, feePerKb.longValue());
    }

    public BRAddress.ByValue legacyAddress() {
        return CryptoLibrary.INSTANCE.BRWalletLegacyAddress(this);
    }

    public UnsignedLong getAmountReceivedFromTx(CoreBRTransaction tx) {
        BRTransaction coreTransfer = tx.asBRTransaction();
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.BRWalletAmountReceivedFromTx(this, coreTransfer));
    }

    public UnsignedLong getAmountSentByTx(CoreBRTransaction tx) {
        BRTransaction coreTransfer = tx.asBRTransaction();
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.BRWalletAmountSentByTx(this, coreTransfer));
    }

    public boolean containsAddress(String address) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.BRWalletContainsAddress(this, address);
    }

    public boolean matches(BRWallet o) {
        return this.equals(o);
    }
}
