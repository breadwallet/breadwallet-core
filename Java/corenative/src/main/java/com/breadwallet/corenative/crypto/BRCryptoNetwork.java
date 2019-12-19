/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryIndirect;
import com.breadwallet.corenative.CryptoLibraryDirect;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedInts;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

import java.util.ArrayList;
import java.util.List;

public class BRCryptoNetwork extends PointerType {

    public static Optional<BRCryptoNetwork> findBuiltin (String uids) {
        Pointer builtin = CryptoLibraryDirect.cryptoNetworkFindBuiltin (uids);
        return (null == builtin
                ? Optional.absent()
                : Optional.of (new BRCryptoNetwork(builtin)));
    }

    public static List<BRCryptoNetwork> installBuiltins () {
        List<BRCryptoNetwork> builtins = new ArrayList<>();
        SizeTByReference count = new SizeTByReference();
        Pointer builtinsPtr = CryptoLibraryDirect.cryptoNetworkInstallBuiltins(count);

        if (null != builtinsPtr) {
            try {
                int builtinsSize = UnsignedInts.checkedCast(count.getValue().longValue());
                for (Pointer builtinPtr : builtinsPtr.getPointerArray(0, builtinsSize)) {
                    builtins.add(new BRCryptoNetwork(builtinPtr));
                }

            } finally {
                Native.free(Pointer.nativeValue(builtinsPtr));
            }
        }
        return builtins;
    }

    public BRCryptoNetwork() {
        super();
    }

    public BRCryptoNetwork(Pointer address) {
        super(address);
    }

    public BRCryptoNetworkCanonicalType getCanonicalType () {
        return BRCryptoNetworkCanonicalType.fromCore(
                CryptoLibraryDirect.cryptoNetworkGetCanonicalType(
                        this.getPointer())
        );
    }

    public BRCryptoCurrency getCurrency() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoCurrency(
                CryptoLibraryDirect.cryptoNetworkGetCurrency(
                        thisPtr
                )
        );
    }

    public void setCurrency(BRCryptoCurrency currency) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoNetworkSetCurrency(
                thisPtr,
                currency.getPointer()
        );
    }

    public boolean hasCurrency(BRCryptoCurrency currency) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoNetworkHasCurrency(thisPtr, currency.getPointer());
    }

    public UnsignedLong getCurrencyCount() {
        Pointer thisPtr = this.getPointer();

        return UnsignedLong.fromLongBits(CryptoLibraryDirect.cryptoNetworkGetCurrencyCount(thisPtr).longValue());
    }

    public BRCryptoCurrency getCurrency(UnsignedLong index) {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoCurrency(
                CryptoLibraryDirect.cryptoNetworkGetCurrencyAt(
                        thisPtr,
                        new SizeT(index.longValue())
                )
        );
    }

    public List<BRCryptoNetworkFee> getFees() {
        Pointer thisPtr = this.getPointer();

        List<BRCryptoNetworkFee> fees = new ArrayList<>();
        SizeTByReference count = new SizeTByReference();
        Pointer feesPtr = CryptoLibraryDirect.cryptoNetworkGetNetworkFees(thisPtr, count);
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
        Pointer thisPtr = this.getPointer();

        BRCryptoNetworkFee[] cryptoFees = new BRCryptoNetworkFee[fees.size()];
        for (int i = 0; i < fees.size(); i++) cryptoFees[i] = fees.get(i);

        CryptoLibraryIndirect.cryptoNetworkSetNetworkFees(thisPtr, cryptoFees, new SizeT(cryptoFees.length));
    }

    public String getUids() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoNetworkGetUids(thisPtr).getString(0, "UTF-8");
    }

    public boolean isMainnet() {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoNetworkIsMainnet(thisPtr);
    }

    public UnsignedLong getHeight() {
        Pointer thisPtr = this.getPointer();

        return UnsignedLong.fromLongBits(CryptoLibraryDirect.cryptoNetworkGetHeight(thisPtr));
    }

    public void setHeight(UnsignedLong height) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoNetworkSetHeight(thisPtr, height.longValue());
    }

    public UnsignedInteger getConfirmationsUntilFinal() {
        Pointer thisPtr = this.getPointer();

        return UnsignedInteger.fromIntBits(CryptoLibraryDirect.cryptoNetworkGetConfirmationsUntilFinal(thisPtr));
    }

    public void setConfirmationsUntilFinal(UnsignedInteger confirmationsUntilFinal) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoNetworkSetConfirmationsUntilFinal(thisPtr, confirmationsUntilFinal.intValue());
    }

    public String getName() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoNetworkGetName(thisPtr).getString(0, "UTF-8");
    }

    public void addFee(BRCryptoNetworkFee networkFee) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoNetworkAddNetworkFee(
                thisPtr,
                networkFee.getPointer()
        );
    }

    public void addCurrency(BRCryptoCurrency currency, BRCryptoUnit baseUnit, BRCryptoUnit defaultUnit) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoNetworkAddCurrency(
                thisPtr,
                currency.getPointer(),
                baseUnit.getPointer(),
                defaultUnit.getPointer()
        );
    }

    public void addCurrencyUnit(BRCryptoCurrency currency, BRCryptoUnit unit) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoNetworkAddCurrencyUnit(
                thisPtr,
                currency.getPointer(),
                unit.getPointer()
        );
    }

    public Optional<BRCryptoUnit> getUnitAsBase(BRCryptoCurrency currency) {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoNetworkGetUnitAsBase(
                        thisPtr,
                        currency.getPointer()
                )
        ).transform(BRCryptoUnit::new);
    }

    public Optional<BRCryptoUnit> getUnitAsDefault(BRCryptoCurrency currency) {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoNetworkGetUnitAsDefault(
                        thisPtr,
                        currency.getPointer()
                )
        ).transform(BRCryptoUnit::new);
    }

    public UnsignedLong getUnitCount(BRCryptoCurrency currency) {
        Pointer thisPtr = this.getPointer();

        return UnsignedLong.fromLongBits(
                CryptoLibraryDirect.cryptoNetworkGetUnitCount(
                        thisPtr,
                        currency.getPointer()
                ).longValue()
        );
    }

    public Optional<BRCryptoUnit> getUnitAt(BRCryptoCurrency currency, UnsignedLong index) {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoNetworkGetUnitAt(
                        thisPtr,
                        currency.getPointer(),
                        new SizeT(index.longValue())
                )
        ).transform(BRCryptoUnit::new);
    }

    public BRCryptoAddressScheme getDefaultAddressScheme() {
        return BRCryptoAddressScheme.fromCore(
                CryptoLibraryDirect.cryptoNetworkGetDefaultAddressScheme(
                        this.getPointer())
        );
    }

    public List<BRCryptoAddressScheme> getSupportedAddressSchemes() {
        Pointer thisPtr = this.getPointer();

        List<BRCryptoAddressScheme> schemes = new ArrayList<>();
        SizeTByReference count = new SizeTByReference();
        Pointer schemesPtr = CryptoLibraryDirect.cryptoNetworkGetSupportedAddressSchemes(thisPtr, count);
        if (null != schemesPtr) {
            try {
                int schemesSize = UnsignedInts.checkedCast(count.getValue().longValue());
                for (int schemeInt: schemesPtr.getIntArray(0, schemesSize)) {
                    schemes.add(BRCryptoAddressScheme.fromCore(schemeInt));
                }
            } finally {
                Native.free(Pointer.nativeValue(schemesPtr));
            }
        }
        return schemes;
    }

    public boolean supportsAddressScheme(BRCryptoAddressScheme addressScheme) {
        return CryptoLibraryDirect.cryptoNetworkSupportsAddressScheme (
                this.getPointer(),
                addressScheme.toCore()
        );
    }

    public BRCryptoSyncMode getDefaultSyncMode() {
        return BRCryptoSyncMode.fromCore(
                CryptoLibraryDirect.cryptoNetworkGetDefaultSyncMode(
                        this.getPointer())
        );
    }

    public List<BRCryptoSyncMode> getSupportedSyncModes() {
       Pointer thisPtr = this.getPointer();

        List<BRCryptoSyncMode> modes = new ArrayList<>();
        SizeTByReference count = new SizeTByReference();
        Pointer modesPtr = CryptoLibraryDirect.cryptoNetworkGetSupportedSyncModes(thisPtr, count);
        if (null != modesPtr) {
            try {
                int modesSize = UnsignedInts.checkedCast(count.getValue().longValue());
                for (int modeInt: modesPtr.getIntArray(0, modesSize)) {
                    modes.add(BRCryptoSyncMode.fromCore(modeInt));
                }
            } finally {
                Native.free(Pointer.nativeValue(modesPtr));
            }
        }
        return modes;
    }

    public boolean supportsSyncMode(BRCryptoSyncMode mode) {
        return CryptoLibraryDirect.cryptoNetworkSupportsSyncMode(
                this.getPointer(),
                mode.toCore()
        );
    }

    public boolean requiresMigration () {
        return CryptoLibraryDirect.cryptoNetworkRequiresMigration(
                this.getPointer()
        );
    }

    public BRCryptoNetwork take() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoNetwork(CryptoLibraryDirect.cryptoNetworkTake(thisPtr));
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoNetworkGive(thisPtr);
    }
}
