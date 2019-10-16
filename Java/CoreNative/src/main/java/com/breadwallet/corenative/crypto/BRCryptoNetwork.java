/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedBytes;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedInts;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.ptr.PointerByReference;

import java.util.ArrayList;
import java.util.List;

public class BRCryptoNetwork extends PointerType {

    public static BRCryptoNetwork createAsBtc(String uids, String name, boolean isMainnet) {
        Pointer globalPtr = CryptoLibrary.LIBRARY.getGlobalVariableAddress(isMainnet ? "BRMainNetParams" : "BRTestNetParams");
        return CryptoLibrary.INSTANCE.cryptoNetworkCreateAsBTC(uids, name, (byte) (isMainnet ? 0x00 : 0x40), globalPtr.getPointer(0));
    }

    public static BRCryptoNetwork createAsBch(String uids, String name, boolean isMainnet) {
        Pointer globalPtr = CryptoLibrary.LIBRARY.getGlobalVariableAddress(isMainnet ? "BRBCashParams" : "BRBCashTestNetParams");
        return CryptoLibrary.INSTANCE.cryptoNetworkCreateAsBTC(uids, name, (byte) (isMainnet ? 0x00 : 0x40), globalPtr.getPointer(0));
    }

    public static Optional<BRCryptoNetwork> createAsEth(String uids, String name, boolean isMainnet) {
        if (uids.contains("mainnet")) {
            Pointer globalPtr = CryptoLibrary.LIBRARY.getGlobalVariableAddress("ethereumMainnet");
            return Optional.of(CryptoLibrary.INSTANCE.cryptoNetworkCreateAsETH(uids, name, 1, globalPtr.getPointer(0)));

        } else if (uids.contains("testnet") || uids.contains("ropsten")) {
            Pointer globalPtr = CryptoLibrary.LIBRARY.getGlobalVariableAddress("ethereumTestnet");
            return Optional.of(CryptoLibrary.INSTANCE.cryptoNetworkCreateAsETH(uids, name, 3, globalPtr.getPointer(0)));

        } else if (uids.contains ("rinkeby")) {
            Pointer globalPtr = CryptoLibrary.LIBRARY.getGlobalVariableAddress("ethereumRinkeby");
            return Optional.of(CryptoLibrary.INSTANCE.cryptoNetworkCreateAsETH(uids, name, 4, globalPtr.getPointer(0)));

        } else {
            return Optional.absent();
        }
    }

    public static BRCryptoNetwork createAsGen(String uids, String name, boolean isMainnet) {
        return CryptoLibrary.INSTANCE.cryptoNetworkCreateAsGEN(uids, name, isMainnet ? (byte) 1 : 0);
    }

    public BRCryptoNetwork(Pointer address) {
        super(address);
    }

    public BRCryptoNetwork() {
        super();
    }

    public BRCryptoCurrency getCurrency() {
        return CryptoLibrary.INSTANCE.cryptoNetworkGetCurrency(this);
    }

    public void setCurrency(BRCryptoCurrency currency) {
        CryptoLibrary.INSTANCE.cryptoNetworkSetCurrency(this, currency);
    }

    public boolean hasCurrency(BRCryptoCurrency currency) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoNetworkHasCurrency(this, currency);
    }

    public UnsignedLong getCurrencyCount() {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.cryptoNetworkGetCurrencyCount(this).longValue());
    }

    public BRCryptoCurrency getCurrency(UnsignedLong index) {
        return CryptoLibrary.INSTANCE.cryptoNetworkGetCurrencyAt(this,
                new SizeT(index.longValue()));
    }

    public List<BRCryptoNetworkFee> getFees() {
        List<BRCryptoNetworkFee> fees = new ArrayList<>();
        SizeTByReference count = new SizeTByReference();
        Pointer feesPtr = CryptoLibrary.INSTANCE.cryptoNetworkGetNetworkFees(this, count);
        if (null != feesPtr) {
            try {
                int feesSize = UnsignedInts.checkedCast(count.getValue().longValue());
                for (Pointer feePtr: feesPtr.getPointerArray(0, feesSize)) {
                    fees.add(new BRCryptoNetworkFee(feePtr));
                }

            } finally {
                Native.free(Pointer.nativeValue(feesPtr));
            }
        }
        return fees;
    }

    public void setFees(List<BRCryptoNetworkFee> fees) {
        BRCryptoNetworkFee[] cryptoFees = new BRCryptoNetworkFee[fees.size()];
        for (int i = 0; i < fees.size(); i++) cryptoFees[i] = fees.get(i);

        CryptoLibrary.INSTANCE.cryptoNetworkSetNetworkFees(this, cryptoFees, new SizeT(cryptoFees.length));
    }

    public String getUids() {
        return CryptoLibrary.INSTANCE.cryptoNetworkGetUids(this).getString(0, "UTF-8");
    }

    public boolean isMainnet() {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoNetworkIsMainnet(this);
    }

    public UnsignedLong getHeight() {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.cryptoNetworkGetHeight(this));
    }

    public void setHeight(UnsignedLong height) {
        CryptoLibrary.INSTANCE.cryptoNetworkSetHeight(this, height.longValue());
    }

    public UnsignedInteger getConfirmationsUntilFinal() {
        return UnsignedInteger.fromIntBits(CryptoLibrary.INSTANCE.cryptoNetworkGetConfirmationsUntilFinal(this));
    }

    public void setConfirmationsUntilFinal(UnsignedInteger confirmationsUntilFinal) {
        CryptoLibrary.INSTANCE.cryptoNetworkSetConfirmationsUntilFinal(this, confirmationsUntilFinal.intValue());
    }

    public String getName() {
        return CryptoLibrary.INSTANCE.cryptoNetworkGetName(this).getString(0, "UTF-8");
    }

    public void addFee(BRCryptoNetworkFee fee) {
        CryptoLibrary.INSTANCE.cryptoNetworkAddNetworkFee(this, fee);
    }

    public void addCurrency(BRCryptoCurrency currency, BRCryptoUnit baseUnit, BRCryptoUnit defaultUnit) {
        CryptoLibrary.INSTANCE.cryptoNetworkAddCurrency(this, currency, baseUnit, defaultUnit);
    }

    public void addCurrencyUnit(BRCryptoCurrency currency, BRCryptoUnit unit) {
        CryptoLibrary.INSTANCE.cryptoNetworkAddCurrencyUnit(this, currency, unit);
    }

    public Optional<BRCryptoUnit> getUnitAsBase(BRCryptoCurrency currency) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoNetworkGetUnitAsBase(this, currency));
    }

    public Optional<BRCryptoUnit> getUnitAsDefault(BRCryptoCurrency currency) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoNetworkGetUnitAsDefault(this, currency));
    }

    public UnsignedLong getUnitCount(BRCryptoCurrency currency) {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.cryptoNetworkGetUnitCount(this, currency).longValue());
    }

    public Optional<BRCryptoUnit> getUnitAt(BRCryptoCurrency currency, UnsignedLong index) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoNetworkGetUnitAt(this, currency, new SizeT(index.longValue())));
    }

    public Optional<BRCryptoAddress> addressFor(String address) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoNetworkCreateAddressFromString(this, address));
    }

    public BRCryptoNetwork take() {
        return CryptoLibrary.INSTANCE.cryptoNetworkTake(this);
    }

    public void give() {
        CryptoLibrary.INSTANCE.cryptoNetworkGive(this);
    }
}
