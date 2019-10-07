/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;

import java.util.List;

public interface CoreBRCryptoNetwork {

    static CoreBRCryptoNetwork createAsBtc(String uids, String name, boolean isMainnet) {
        Pointer globalPtr = CryptoLibrary.LIBRARY.getGlobalVariableAddress(isMainnet ? "BRMainNetParams" : "BRTestNetParams");
        return new OwnedBRCryptoNetwork(CryptoLibrary.INSTANCE.cryptoNetworkCreateAsBTC(uids, name, (byte) (isMainnet ? 0x00 : 0x40), globalPtr.getPointer(0)));
    }

    static CoreBRCryptoNetwork createAsBch(String uids, String name, boolean isMainnet) {
        Pointer globalPtr = CryptoLibrary.LIBRARY.getGlobalVariableAddress(isMainnet ? "BRBCashParams" : "BRBCashTestNetParams");
        return new OwnedBRCryptoNetwork(CryptoLibrary.INSTANCE.cryptoNetworkCreateAsBTC(uids, name, (byte) (isMainnet ? 0x00 : 0x40), globalPtr.getPointer(0)));
    }

    static Optional<CoreBRCryptoNetwork> createAsEth(String uids, String name, boolean isMainnet) {
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

    static CoreBRCryptoNetwork createAsGen(String uids, String name, boolean isMainnet) {
        return new OwnedBRCryptoNetwork(CryptoLibrary.INSTANCE.cryptoNetworkCreateAsGEN(uids, name, isMainnet ? (byte) 1 : 0));
    }

    BRCryptoCurrency getCurrency();

    void setCurrency(BRCryptoCurrency currency);

    boolean hasCurrency(BRCryptoCurrency currency);

    UnsignedLong getCurrencyCount();

    BRCryptoCurrency getCurrency(UnsignedLong index);

    List<BRCryptoNetworkFee> getFees();

    void setFees(List<BRCryptoNetworkFee> fees);

    String getUids();

    boolean isMainnet();

    UnsignedLong getHeight();

    void setHeight(UnsignedLong height);

    UnsignedInteger getConfirmationsUntilFinal();

    void setConfirmationsUntilFinal(UnsignedInteger confirmationsUntilFinal);

    String getName();

    void addFee(BRCryptoNetworkFee fee);

    void addCurrency(BRCryptoCurrency currency, CoreBRCryptoUnit baseUnit, CoreBRCryptoUnit defaultUnit);

    void addCurrencyUnit(BRCryptoCurrency currency, CoreBRCryptoUnit unit);

    Optional<CoreBRCryptoUnit> getUnitAsBase(BRCryptoCurrency currency);

    Optional<CoreBRCryptoUnit> getUnitAsDefault(BRCryptoCurrency currency);

    UnsignedLong getUnitCount(BRCryptoCurrency currency);

    Optional<CoreBRCryptoUnit> getUnitAt(BRCryptoCurrency currency, UnsignedLong index);

    Optional<BRCryptoAddress> addressFor(String address);

    BRCryptoNetwork asBRCryptoNetwork();
}
