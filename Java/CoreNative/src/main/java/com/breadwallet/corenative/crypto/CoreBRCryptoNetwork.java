/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;

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

    void setHeight(UnsignedLong height);

    CoreBRCryptoCurrency getCurrency();

    void setCurrency(CoreBRCryptoCurrency currency);

    boolean hasCurrency(CoreBRCryptoCurrency currency);

    UnsignedLong getCurrencyCount();

    CoreBRCryptoCurrency getCurrency(UnsignedLong index);

    String getUids();

    boolean isMainnet();

    UnsignedLong getHeight();

    String getName();

    int getType();

    void addCurrency(CoreBRCryptoCurrency currency, CoreBRCryptoUnit baseUnit, CoreBRCryptoUnit defaultUnit);

    void addCurrencyUnit(CoreBRCryptoCurrency currency, CoreBRCryptoUnit unit);

    Optional<CoreBRCryptoUnit> getUnitAsBase(CoreBRCryptoCurrency currency);

    Optional<CoreBRCryptoUnit> getUnitAsDefault(CoreBRCryptoCurrency currency);

    UnsignedLong getUnitCount(CoreBRCryptoCurrency currency);

    Optional<CoreBRCryptoUnit> getUnitAt(CoreBRCryptoCurrency currency, UnsignedLong index);

    BRCryptoNetwork asBRCryptoNetwork();
}
