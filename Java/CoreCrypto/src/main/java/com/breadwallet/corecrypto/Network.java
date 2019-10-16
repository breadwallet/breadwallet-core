/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoCurrency;
import com.breadwallet.corenative.crypto.BRCryptoNetwork;
import com.breadwallet.corenative.crypto.BRCryptoNetworkFee;
import com.google.common.base.Optional;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

import static com.google.common.base.Preconditions.checkState;

/* package */
final class Network implements com.breadwallet.crypto.Network {

    /* package */
    static Network create(String uids, String name, boolean isMainnet, Currency currency, UnsignedLong height,
                          Map<Currency, NetworkAssociation> associations,
                          List<NetworkFee> fees, UnsignedInteger confirmationsUntilFinal) {
        BRCryptoNetwork core;

        String code = currency.getCode();
        switch (code) {
            case com.breadwallet.crypto.Currency.CODE_AS_BTC:
                core = BRCryptoNetwork.createAsBtc(uids, name, isMainnet);

                break;
            case com.breadwallet.crypto.Currency.CODE_AS_BCH:
                core = BRCryptoNetwork.createAsBch(uids, name, isMainnet);

                break;
            case com.breadwallet.crypto.Currency.CODE_AS_ETH:
                Optional<BRCryptoNetwork> optional = BRCryptoNetwork.createAsEth(uids, name, isMainnet);
                if (optional.isPresent()) {
                    core = optional.get();
                } else {
                    throw new IllegalArgumentException("Unsupported ETH network");
                }

                break;
            default:
                core = BRCryptoNetwork.createAsGen(uids, name, isMainnet);
                break;
        }

        core.setHeight(height);
        core.setCurrency(currency.getCoreBRCryptoCurrency());

        for (Map.Entry<Currency, NetworkAssociation> entry: associations.entrySet()) {
            Currency associationCurrency = entry.getKey();
            NetworkAssociation association = entry.getValue();

            core.addCurrency(associationCurrency.getCoreBRCryptoCurrency(),
                    association.getBaseUnit().getCoreBRCryptoUnit(),
                    association.getDefaultUnit().getCoreBRCryptoUnit());

            for (Unit unit: association.getUnits()) {
                core.addCurrencyUnit(associationCurrency.getCoreBRCryptoCurrency(), unit.getCoreBRCryptoUnit());
            }
        }

        checkState(!fees.isEmpty());
        for (NetworkFee fee: fees) {
            core.addFee(fee.getCoreBRCryptoNetworkFee());
        }

        core.setConfirmationsUntilFinal(confirmationsUntilFinal);

        return Network.create(core);
    }

    /* package */
    static Network create(BRCryptoNetwork core) {
        Network network = new Network(core);
        ReferenceCleaner.register(network, core::give);
        return network;
    }

    /* package */
    static Network from(com.breadwallet.crypto.Network network) {
        if (network == null) {
            return null;
        }

        if (network instanceof Network) {
            return (Network) network;
        }

        throw new IllegalArgumentException("Unsupported network instance");
    }

    private final BRCryptoNetwork core;

    private final Supplier<String> uidsSupplier;
    private final Supplier<String> nameSupplier;
    private final Supplier<Boolean> isMainnetSupplier;
    private final Supplier<Currency> currencySupplier;
    private final Supplier<Set<Currency>> currenciesSupplier;

    private Network(BRCryptoNetwork core) {
        this.core = core;

        uidsSupplier = Suppliers.memoize(core::getUids);
        nameSupplier = Suppliers.memoize(core::getName);
        isMainnetSupplier = Suppliers.memoize(core::isMainnet);
        currencySupplier = Suppliers.memoize(() -> Currency.create(core.getCurrency()));

        currenciesSupplier = Suppliers.memoize(() -> {
            Set<Currency> currencies = new HashSet<>();
            UnsignedLong count = core.getCurrencyCount();
            for (UnsignedLong i = UnsignedLong.ZERO; i.compareTo(count) < 0; i = i.plus(UnsignedLong.ONE)) {
                currencies.add(Currency.create(core.getCurrency(i)));
            }
            return currencies;
        });
    }

    @Override
    public String getUids() {
        return uidsSupplier.get();
    }

    @Override
    public String getName() {
        return nameSupplier.get();
    }

    @Override
    public boolean isMainnet() {
        return isMainnetSupplier.get();
    }

    @Override
    public UnsignedLong getHeight() {
        return core.getHeight();
    }

    @Override
    public UnsignedInteger getConfirmationsUntilFinal() {
        return core.getConfirmationsUntilFinal();
    }

    @Override
    public Optional<NetworkPeer> createPeer(String address, UnsignedInteger port, @Nullable String publicKey) {
        return NetworkPeer.create(this, address, port, publicKey);
    }

    @Override
    public Currency getCurrency() {
        return currencySupplier.get();
    }

    @Override
    public Set<Currency> getCurrencies() {
        return new HashSet<>(currenciesSupplier.get());
    }

    @Override
    public Optional<Currency> getCurrencyByCode(String code) {
        for (Currency currency: getCurrencies()) {
            if (code.equals(currency.getCode())) {
                return Optional.of(currency);
            }
        }
        return Optional.absent();
    }

    @Override
    public Optional<Currency> getCurrencyByIssuer(String issuer) {
        String issuerLowercased = issuer.toLowerCase(Locale.ROOT);
        for (Currency currency: getCurrencies()) {
            if (issuerLowercased.equals(currency.getIssuer().orNull())) {
                return Optional.of(currency);
            }
        }
        return Optional.absent();
    }

    @Override
    public List<? extends NetworkFee> getFees() {
        List<NetworkFee> fees = new ArrayList<>();
        for (BRCryptoNetworkFee fee: core.getFees()) {
            fees.add(NetworkFee.create(fee));
        }
        return fees;
    }

    @Override
    public NetworkFee getMinimumFee() {
        NetworkFee minimumFee = null;
        for (NetworkFee fee: getFees()) {
            if (minimumFee == null || fee.getConfirmationTimeInMilliseconds().compareTo(minimumFee.getConfirmationTimeInMilliseconds()) > 0) {
                minimumFee = fee;
            }
        }
        return minimumFee;
    }

    @Override
    public boolean hasCurrency(com.breadwallet.crypto.Currency currency) {
        return core.hasCurrency(Currency.from(currency).getCoreBRCryptoCurrency());
    }

    @Override
    public Optional<Unit> baseUnitFor(com.breadwallet.crypto.Currency currency) {
        if (!hasCurrency(currency)) {
            return Optional.absent();
        }
        return core.getUnitAsBase(Currency.from(currency).getCoreBRCryptoCurrency()).transform(Unit::create);
    }

    @Override
    public Optional<Unit> defaultUnitFor(com.breadwallet.crypto.Currency currency) {
        if (!hasCurrency(currency)) {
            return Optional.absent();
        }
        return core.getUnitAsDefault(Currency.from(currency).getCoreBRCryptoCurrency()).transform(Unit::create);
    }

    @Override
    public Optional<Set<? extends com.breadwallet.crypto.Unit>> unitsFor(com.breadwallet.crypto.Currency currency) {
        if (!hasCurrency(currency)) {
            return Optional.absent();
        }

        Set<Unit> units = new HashSet<>();

        BRCryptoCurrency currencyCore = Currency.from(currency).getCoreBRCryptoCurrency();
        UnsignedLong count = core.getUnitCount(currencyCore);

        for (UnsignedLong i = UnsignedLong.ZERO; i.compareTo(count) < 0; i = i.plus(UnsignedLong.ONE)) {
            Optional<Unit> unit = core.getUnitAt(currencyCore, i).transform(Unit::create);
            if (!unit.isPresent()) {
                return Optional.absent();
            }

            units.add(unit.get());
        }

        return Optional.of(units);
    }

    @Override
    public Optional<Boolean> hasUnitFor(com.breadwallet.crypto.Currency currency, com.breadwallet.crypto.Unit unit) {
        return unitsFor(currency).transform(input -> input.contains(unit));
    }

    @Override
    public Optional<Address> addressFor(String address) {
        return core.addressFor(address).transform(Address::create);
    }

    @Override
    public String toString() {
        return getName();
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof Network)) {
            return false;
        }

        Network network = (Network) object;
        return getUids().equals(network.getUids());
    }

    @Override
    public int hashCode() {
        return Objects.hash(getUids());
    }

    /* package */
    void setHeight(UnsignedLong height) {
        core.setHeight(height);
    }

    /* package */
    void setFees(List<NetworkFee> fees) {
        checkState(!fees.isEmpty());
        List<BRCryptoNetworkFee> cryptoFees = new ArrayList<>(fees.size());
        for (NetworkFee fee: fees) {
            cryptoFees.add(fee.getCoreBRCryptoNetworkFee());
        }
        core.setFees(cryptoFees);
    }

    /* package */
    BRCryptoNetwork getCoreBRCryptoNetwork() {
        return core;
    }
}
