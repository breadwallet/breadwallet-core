/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.BRCryptoBlockChainType;
import com.breadwallet.corenative.crypto.CoreBRCryptoCurrency;
import com.breadwallet.corenative.crypto.CoreBRCryptoNetwork;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
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
        CoreBRCryptoNetwork core;

        String code = currency.getCode();
        switch (code) {
            case com.breadwallet.crypto.Currency.CODE_AS_BTC:
                core = CoreBRCryptoNetwork.createAsBtc(uids, name, isMainnet);

                break;
            case com.breadwallet.crypto.Currency.CODE_AS_BCH:
                core = CoreBRCryptoNetwork.createAsBch(uids, name, isMainnet);

                break;
            case com.breadwallet.crypto.Currency.CODE_AS_ETH:
                Optional<CoreBRCryptoNetwork> optional = CoreBRCryptoNetwork.createAsEth(uids, name, isMainnet);
                if (optional.isPresent()) {
                    core = optional.get();
                } else {
                    throw new IllegalArgumentException("Unsupported ETH network");
                }

                break;
            default:
                core = CoreBRCryptoNetwork.createAsGen(uids, name, isMainnet);
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

        for (NetworkFee fee: fees) {
            core.addFee(fee.getCoreBRCryptoNetworkFee());
        }

        core.setConfirmationsUntilFinal(confirmationsUntilFinal);

        return new Network(core);
    }

    /* package */
    static Network create(CoreBRCryptoNetwork core) {
        return new Network(core);
    }

    /* package */
    static Network from(com.breadwallet.crypto.Network network) {
        if (network instanceof Network) {
            return (Network) network;
        }
        throw new IllegalArgumentException("Unsupported network instance");
    }

    private final CoreBRCryptoNetwork core;

    private final String uids;
    private final String name;
    private final Boolean isMainnet;
    private final Currency currency;
    private final Set<Currency> currencies;
    private final List<NetworkFee> fees;
    private NetworkFee minimumFee;

    private Network(CoreBRCryptoNetwork core) {
        this.core = core;

        uids = core.getUids();
        name = core.getName();
        isMainnet = core.isMainnet();
        currency = Currency.create(core.getCurrency());

        currencies = new HashSet<>();
        UnsignedLong count = core.getCurrencyCount();
        for (UnsignedLong i = UnsignedLong.ZERO; i.compareTo(count) < 0; i = i.plus(UnsignedLong.ONE)) {
            currencies.add(Currency.create(core.getCurrency(i)));
        }

        fees = new ArrayList<>();
        count = core.getFeeCount();
        for (UnsignedLong i = UnsignedLong.ZERO; i.compareTo(count) < 0; i = i.plus(UnsignedLong.ONE)) {
            NetworkFee fee = NetworkFee.create(core.getFee(i));
            if (minimumFee == null || fee.getConfirmationTimeInMilliseconds().compareTo(minimumFee.getConfirmationTimeInMilliseconds()) > 0) {
                minimumFee = fee;
            }
            fees.add(fee);
        }

        checkState(!fees.isEmpty());
        checkState(minimumFee != null);
    }

    @Override
    public String getUids() {
        return uids;
    }

    @Override
    public String getName() {
        return name;
    }

    @Override
    public boolean isMainnet() {
        return isMainnet;
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
    public Currency getCurrency() {
        return currency;
    }

    @Override
    public Set<Currency> getCurrencies() {
        return new HashSet<>(currencies);
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
        String issuerLowercased = issuer.toLowerCase();
        for (Currency currency: getCurrencies()) {
            if (issuerLowercased.equals(currency.getIssuer().orNull())) {
                return Optional.of(currency);
            }
        }
        return Optional.absent();
    }

    @Override
    public List<? extends NetworkFee> getFees() {
        return fees;
    }

    @Override
    public NetworkFee getMinimumFee() {
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

        CoreBRCryptoCurrency currencyCore = Currency.from(currency).getCoreBRCryptoCurrency();
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
        return core.equals(network.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }

    /* package */
    void setHeight(UnsignedLong height) {
        core.setHeight(height);
    }

    /* package */
    CoreBRCryptoNetwork getCoreBRCryptoNetwork() {
        return core;
    }
}
