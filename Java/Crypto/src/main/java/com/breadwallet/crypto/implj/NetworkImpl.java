/*
 * Network
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.libcrypto.CryptoLibrary;
import com.breadwallet.crypto.libcrypto.bitcoin.BRChainParams;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import static com.google.common.base.Preconditions.checkNotNull;

public final class NetworkImpl implements Network {

    /* package */
    static NetworkImpl create(String uids, String name, boolean isMainnet, CurrencyImpl currency, long height,
                              Map<CurrencyImpl, NetworkAssociation> associations) {
        String code = currency.getCode();
        if (code .equals(Currency.CODE_AS_BTC)) {
            return new NetworkImpl(uids, name, isMainnet, currency, height, associations, new BitcoinNetwork(isMainnet));
        } else if (code .equals(Currency.CODE_AS_BCH)) {
            return new NetworkImpl(uids, name, isMainnet, currency, height, associations, new BitcashNetwork(isMainnet));
        } else if (code .equals(Currency.CODE_AS_ETH)) {
            return new NetworkImpl(uids, name, isMainnet, currency, height, associations, new EthereumNetwork(uids));
        } else {
            return new NetworkImpl(uids, name, isMainnet, currency, height, associations, new Generic());
        }
    }

    private final String uids;
    private final String name;
    private final boolean isMainnet;
    private final CurrencyImpl currency;
    private final Set<CurrencyImpl> currencies;
    private final Map<CurrencyImpl, NetworkAssociation> associations;

    private long height;

    private final CurrencyNetwork impl;

    private NetworkImpl(String uids, String name, boolean isMainnet, CurrencyImpl currency, long height, Map<CurrencyImpl,
            NetworkAssociation> associations, CurrencyNetwork impl) {
        this.uids = uids;
        this.name = name;
        this.isMainnet = isMainnet;
        this.currency = currency;
        this.height = height;
        this.associations = associations;
        this.currencies = new HashSet<>(associations.keySet());
        this.impl = impl;
    }

    @Override
    public Optional<Unit> baseUnitFor(Currency currency) {
        NetworkAssociation association = associations.get(currency);
        return association == null ? Optional.absent() : Optional.of(association.getBaseUnit());
    }

    @Override
    public Optional<Unit> defaultUnitFor(Currency currency) {
        NetworkAssociation association = associations.get(currency);
        return association == null ? Optional.absent() : Optional.of(association.getDefaultUnit());
    }

    @Override
    public Optional<Set<Unit>> unitsFor(Currency currency) {
        NetworkAssociation association = associations.get(currency);
        return association == null ? Optional.absent() : Optional.of(association.getUnits());
    }

    @Override
    public Optional<Address> addressFor(String address) {
        return impl.addressFor(address);
    }

    @Override
    public boolean hasUnitFor(Currency currency, Unit unit) {
        return unitsFor(currency).transform(input -> input.contains(unit)).or(false);
    }

    @Override
    public boolean hasCurrency(Currency currency) {
        return currencies.contains(currency);
    }

    @Override
    public Currency getCurrency() {
        return currency;
    }

    @Override
    public List<WalletManagerMode> getSupportedModes() {
        return impl.getSupportedModes();
    }

    @Override
    public String getUids() {
        return uids;
    }

    @Override
    public boolean isMainnet() {
        return isMainnet;
    }

    @Override
    public long getHeight() {
        return height;
    }

    @Override
    public String toString() {
        return name;
    }

    @Override
    public void setHeight(long height) {
        this.height = height;
    }

    @Override
    public BRChainParams asBtc() {
        return impl.asBtc();
    }

    // TODO: SHould asBtc() and asEth() be returning Optional instead of throwing an exception?
    private interface CurrencyNetwork {
        default BRChainParams asBtc() {
            throw new IllegalStateException("Invalid network type");
        }

        List<WalletManagerMode> getSupportedModes();

        Optional<Address> addressFor(String address);
    }

    private static class BitcoinNetwork implements CurrencyNetwork {

        private final BRChainParams chainParams;

        BitcoinNetwork(boolean isMainnet) {
            Pointer ptr = CryptoLibrary.LIBRARY.getGlobalVariableAddress(isMainnet ? "BRMainNetParams" : "BRTestNetParams");
            checkNotNull(ptr);
            this.chainParams = new BRChainParams(ptr.getPointer(0));
        }

        @Override
        public BRChainParams asBtc() {
            return chainParams;
        }

        @Override
        public List<WalletManagerMode> getSupportedModes() {
            return Collections.singletonList(WalletManagerMode.P2P_ONLY);
        }

        @Override
        public Optional<Address> addressFor(String address) {
            return AddressImpl.createAsBtc(address).transform(a -> a);
        }
    }

    private static class BitcashNetwork implements CurrencyNetwork {

        private final BRChainParams chainParams;

        BitcashNetwork(boolean isMainnet) {
            Pointer ptr = CryptoLibrary.LIBRARY.getGlobalVariableAddress(isMainnet ? "BRBCashParams" : "BRBCashTestNetParams");
            checkNotNull(ptr);
            this.chainParams = new BRChainParams(ptr.getPointer(0));
        }

        @Override
        public List<WalletManagerMode> getSupportedModes() {
            return Collections.singletonList(WalletManagerMode.P2P_ONLY);
        }

        @Override
        public Optional<Address> addressFor(String address) {
            return AddressImpl.createAsBtc(address).transform(a -> a);
        }
    }

    private static class EthereumNetwork implements CurrencyNetwork {

        EthereumNetwork(String uids) {
            // TODO: Implement this
        }

        @Override
        public List<WalletManagerMode> getSupportedModes() {
            return Arrays.asList(WalletManagerMode.API_ONLY, WalletManagerMode.API_WITH_P2P_SUBMIT);
        }

        @Override
        public Optional<Address> addressFor(String address) {
            return AddressImpl.createAsEth(address).transform(a -> a);
        }
    }

    private static class Generic implements CurrencyNetwork {

        @Override
        public List<WalletManagerMode> getSupportedModes() {
            return Collections.singletonList(WalletManagerMode.API_ONLY);
        }

        @Override
        public Optional<Address> addressFor(String address) {
            // TODO: How is this going to be handled?
            return Optional.absent();
        }
    }
}
