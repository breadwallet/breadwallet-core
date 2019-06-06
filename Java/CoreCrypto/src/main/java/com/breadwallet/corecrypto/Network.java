/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.bitcoin.BRChainParams;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

import static com.google.common.base.Preconditions.checkNotNull;

/* package */
final class Network implements com.breadwallet.crypto.Network {

    /* package */
    static Network create(String uids, String name, boolean isMainnet, Currency currency, UnsignedLong height,
                          Map<Currency, NetworkAssociation> associations) {
        String code = currency.getCode();
        if (code .equals(com.breadwallet.crypto.Currency.CODE_AS_BTC)) {
            return new Network(uids, name, isMainnet, currency, height, associations, new BitcoinNetwork(isMainnet));
        } else if (code .equals(com.breadwallet.crypto.Currency.CODE_AS_BCH)) {
            return new Network(uids, name, isMainnet, currency, height, associations, new BitcashNetwork(isMainnet));
        } else if (code .equals(com.breadwallet.crypto.Currency.CODE_AS_ETH)) {
            return new Network(uids, name, isMainnet, currency, height, associations, new EthereumNetwork(uids));
        } else {
            return new Network(uids, name, isMainnet, currency, height, associations, new Generic());
        }
    }

    /* package */
    static Network from(com.breadwallet.crypto.Network network) {
        if (network instanceof Network) {
            return (Network) network;
        }
        throw new IllegalArgumentException("Unsupported network instance");
    }

    private final String uids;
    private final String name;
    private final boolean isMainnet;
    private final Currency currency;
    private final Set<Currency> currencies;
    private final Map<Currency, NetworkAssociation> associations;
    private final CurrencyNetwork impl;

    private UnsignedLong height;

    private Network(String uids, String name, boolean isMainnet, Currency currency, UnsignedLong height, Map<Currency,
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
    public Optional<Unit> baseUnitFor(com.breadwallet.crypto.Currency currency) {
        NetworkAssociation association = associations.get(currency);
        return association == null ? Optional.absent() : Optional.of(association.getBaseUnit());
    }

    @Override
    public Optional<Unit> defaultUnitFor(com.breadwallet.crypto.Currency currency) {
        NetworkAssociation association = associations.get(currency);
        return association == null ? Optional.absent() : Optional.of(association.getDefaultUnit());
    }

    @Override
    public Optional<Set<Unit>> unitsFor(com.breadwallet.crypto.Currency currency) {
        NetworkAssociation association = associations.get(currency);
        return association == null ? Optional.absent() : Optional.of(association.getUnits());
    }

    @Override
    public Optional<com.breadwallet.crypto.Address> addressFor(String address) {
        return impl.addressFor(address);
    }

    @Override
    public boolean hasUnitFor(com.breadwallet.crypto.Currency currency, Unit unit) {
        return unitsFor(currency).transform(input -> input.contains(unit)).or(false);
    }

    @Override
    public boolean hasCurrency(com.breadwallet.crypto.Currency currency) {
        return currencies.contains(currency);
    }

    @Override
    public com.breadwallet.crypto.Currency getCurrency() {
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
    public UnsignedLong getHeight() {
        return height;
    }

    @Override
    public String toString() {
        return name;
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof Network)) {
            return false;
        }

        // height not included in the equality and hashcode calculation
        Network network = (Network) object;
        return isMainnet == network.isMainnet &&
                uids.equals(network.uids) &&
                name.equals(network.name) &&
                currency.equals(network.currency) &&
                currencies.equals(network.currencies) &&
                associations.equals(network.associations) &&
                impl.equals(network.impl);
    }

    @Override
    public int hashCode() {
        // height not included in the equality and hashcode calculation
        return Objects.hash(uids, name, isMainnet, currency, currencies, associations, impl);
    }

    /* package */
    BRChainParams asBtc() {
        return impl.asBtc();
    }

    /* package */
    void setHeight(UnsignedLong height) {
        this.height = height;
    }

    // TODO(discuss): Should asBtc() and asEth() be returning Optional instead of throwing an exception?
    private interface CurrencyNetwork {
        default BRChainParams asBtc() {
            throw new IllegalStateException("Invalid network type");
        }

        List<WalletManagerMode> getSupportedModes();

        Optional<com.breadwallet.crypto.Address> addressFor(String address);
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
        public Optional<com.breadwallet.crypto.Address> addressFor(String address) {
            return Address.createAsBtc(address).transform(a -> a);
        }

        @Override
        public boolean equals(Object object) {
            if (this == object) {
                return true;
            }

            if (!(object instanceof BitcoinNetwork)) {
                return false;
            }

            BitcoinNetwork that = (BitcoinNetwork) object;
            return chainParams.equals(that.chainParams);
        }

        @Override
        public int hashCode() {
            return Objects.hash(chainParams);
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
        public Optional<com.breadwallet.crypto.Address> addressFor(String address) {
            return Address.createAsBtc(address).transform(a -> a);
        }

        @Override
        public boolean equals(Object object) {
            if (this == object) {
                return true;
            }

            if (!(object instanceof BitcashNetwork)) {
                return false;
            }

            BitcashNetwork that = (BitcashNetwork) object;
            return chainParams.equals(that.chainParams);
        }

        @Override
        public int hashCode() {
            return Objects.hash(chainParams);
        }
    }

    // TODO(fix): Implement this (including equals and hashCode)
    private static class EthereumNetwork implements CurrencyNetwork {

        EthereumNetwork(String uids) {
        }

        @Override
        public List<WalletManagerMode> getSupportedModes() {
            return Arrays.asList(WalletManagerMode.API_ONLY, WalletManagerMode.API_WITH_P2P_SUBMIT);
        }

        @Override
        public Optional<com.breadwallet.crypto.Address> addressFor(String address) {
            return Address.createAsEth(address).transform(a -> a);
        }
    }

    // TODO(fix): Implement this (including equals and hashCode)
    private static class Generic implements CurrencyNetwork {

        @Override
        public List<WalletManagerMode> getSupportedModes() {
            return Collections.singletonList(WalletManagerMode.API_ONLY);
        }

        @Override
        public Optional<com.breadwallet.crypto.Address> addressFor(String address) {
            return Optional.absent();
        }
    }
}
