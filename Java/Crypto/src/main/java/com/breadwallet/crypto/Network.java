/*
 * Network
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.bitcoin.BRChainParams;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import static com.google.common.base.Preconditions.checkNotNull;

public final class Network {

    /* package */
    static Network create(String uids, String name, boolean isMainnet, Currency currency, long height,
                          Map<Currency, NetworkAssociation> associations) {
        String code = currency.getCode();
        if (code .equals(Currency.CODE_AS_BTC)) {
            return new Network(uids, name, isMainnet, currency, height, associations, new Bitcoin(isMainnet));
        } else if (code .equals(Currency.CODE_AS_BCH)) {
            return new Network(uids, name, isMainnet, currency, height, associations, new Bitcash(isMainnet));
        } else if (code .equals(Currency.CODE_AS_ETH)) {
            return new Network(uids, name, isMainnet, currency, height, associations, new Ethereum(uids));
        } else {
            return new Network(uids, name, isMainnet, currency, height, associations, new Generic());
        }
    }

    private final String uids;
    private final String name;
    private final boolean isMainnet;
    private final Currency currency;
    private final Set<Currency> currencies;
    private final Map<Currency, NetworkAssociation> associations;

    private long height;

    private final Impl impl;

    private Network(String uids, String name, boolean isMainnet, Currency currency, long height, Map<Currency,
            NetworkAssociation> associations, Impl impl) {
        this.uids = uids;
        this.name = name;
        this.isMainnet = isMainnet;
        this.currency = currency;
        this.height = height;
        this.associations = associations;
        this.currencies = new HashSet<>(associations.keySet());
        this.impl = impl;
    }

    public Optional<Unit> baseUnitFor(Currency currency) {
        NetworkAssociation association = associations.get(currency);
        return association == null ? Optional.absent() : Optional.of(association.getBaseUnit());
    }

    public Optional<Unit> defaultUnitFor(Currency currency) {
        NetworkAssociation association = associations.get(currency);
        return association == null ? Optional.absent() : Optional.of(association.getDefaultUnit());
    }

    public Optional<Set<Unit>> unitsFor(Currency currency) {
        NetworkAssociation association = associations.get(currency);
        return association == null ? Optional.absent() : Optional.of(association.getUnits());
    }

    public Optional<Address> addressFor(String address) {
        return impl.addressFor(address);
    }

    public boolean hasUnitFor(Currency currency, Unit unit) {
        return unitsFor(currency).transform(input -> input.contains(unit)).or(false);
    }

    public boolean hasCurrency(Currency currency) {
        return currencies.contains(currency);
    }

    public Currency getCurrency() {
        return currency;
    }

    public List<WalletManagerMode> getSupportedModes() {
        return impl.getSupportedModes();
    }

    public String getUids() {
        return uids;
    }

    public boolean isMainnet() {
        return isMainnet;
    }

    public long getHeight() {
        return height;
    }

    /* package */
    void setHeight(long height) {
        this.height = height;
    }

    /* package */
    BRChainParams asBtc() {
        return impl.asBtc();
    }

    @Override
    public String toString() {
        return name;
    }

    private interface Impl {
        default BRChainParams asBtc() {
            throw new IllegalStateException("Invalid network type");
        }

        List<WalletManagerMode> getSupportedModes();

        Optional<Address> addressFor(String address);
    }

    private static class Bitcoin implements Impl {

        private final BRChainParams chainParams;

        Bitcoin(boolean isMainnet) {
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
            return Address.createAsBtc(address);
        }
    }

    private static class Bitcash implements Impl {

        private final BRChainParams chainParams;

        Bitcash(boolean isMainnet) {
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
            return Address.createAsBtc(address);
        }
    }

    private static class Ethereum implements Impl {

        Ethereum(String uids) {
            // TODO: Implement this
        }

        @Override
        public List<WalletManagerMode> getSupportedModes() {
            return Arrays.asList(WalletManagerMode.API_ONLY, WalletManagerMode.API_WITH_P2P_SUBMIT);
        }

        @Override
        public Optional<Address> addressFor(String address) {
            return Address.createAsEth(address);
        }
    }

    private static class Generic implements Impl {

        @Override
        public List<WalletManagerMode> getSupportedModes() {
            return Collections.singletonList(WalletManagerMode.API_ONLY);
        }

        @Override
        public Optional<Address> addressFor(String address) {
            return Optional.absent();
        }
    }
}
