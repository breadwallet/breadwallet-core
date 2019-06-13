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
import com.breadwallet.crypto.WalletManagerMode;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

/* package */
final class Network implements com.breadwallet.crypto.Network {

    /* package */
    static Network create(String uids, String name, boolean isMainnet, Currency currency, UnsignedLong height,
                          Map<Currency, NetworkAssociation> associations) {
        CoreBRCryptoNetwork core = null;

        String code = currency.getCode();
        if (code .equals(com.breadwallet.crypto.Currency.CODE_AS_BTC)) {
            core = CoreBRCryptoNetwork.createAsBtc(uids, name, isMainnet);

        } else if (code .equals(com.breadwallet.crypto.Currency.CODE_AS_BCH)) {
            core = CoreBRCryptoNetwork.createAsBch(uids, name, isMainnet);

        } else if (code .equals(com.breadwallet.crypto.Currency.CODE_AS_ETH)) {
            Optional<CoreBRCryptoNetwork> optional = CoreBRCryptoNetwork.createAsEth(uids, name, isMainnet);
            if (optional.isPresent()) {
                core = optional.get();
            } else {
                throw new IllegalArgumentException("Unsupported ETH network");
            }

        } else {
            throw new IllegalArgumentException("Unsupported network");
        }

        core.setHeight(height);
        core.setCurrency(currency.getCoreBRCryptoCurrency());
        return new Network(core, associations);
    }

    /* package */
    static Network from(com.breadwallet.crypto.Network network) {
        if (network instanceof Network) {
            return (Network) network;
        }
        throw new IllegalArgumentException("Unsupported network instance");
    }

    private final CoreBRCryptoNetwork core;

    private Network(CoreBRCryptoNetwork core, Map<Currency, NetworkAssociation> associations) {
        this.core = core;

        for (Map.Entry<Currency, NetworkAssociation> entry: associations.entrySet()) {
            Currency currency = entry.getKey();
            NetworkAssociation association = entry.getValue();

            core.addCurrency(currency.getCoreBRCryptoCurrency(),
                    association.getBaseUnit().getCoreBRCryptoUnit(),
                    association.getDefaultUnit().getCoreBRCryptoUnit());

            for (Unit unit: association.getUnits()) {
                core.addCurrencyUnit(currency.getCoreBRCryptoCurrency(), unit.getCoreBRCryptoUnit());
            }
        }
    }

    @Override
    public String getUids() {
        return core.getUids();
    }

    @Override
    public boolean isMainnet() {
        return core.isMainnet();
    }

    @Override
    public UnsignedLong getHeight() {
        return core.getHeight();
    }

    @Override
    public Currency getCurrency() {
        return Currency.create(core.getCurrency());
    }

    @Override
    public Set<Currency> getCurrencies() {
        Set<Currency> transfers = new HashSet<>();

        UnsignedLong count = core.getCurrencyCount();
        for (UnsignedLong i = UnsignedLong.ZERO; i.compareTo(count) < 0; i = i.plus(UnsignedLong.ONE)) {
            transfers.add(Currency.create(core.getCurrency(i)));
        }

        return transfers;
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
    public boolean hasCurrency(com.breadwallet.crypto.Currency currency) {
        Currency currencyImpl = Currency.from(currency);
        return core.hasCurrency(currencyImpl.getCoreBRCryptoCurrency());
    }

    @Override
    public Optional<Unit> baseUnitFor(com.breadwallet.crypto.Currency currency) {
        Currency currencyImpl = Currency.from(currency);
        return core.getUnitAsBase(currencyImpl.getCoreBRCryptoCurrency()).transform(Unit::create);
    }

    @Override
    public Optional<Unit> defaultUnitFor(com.breadwallet.crypto.Currency currency) {
        Currency currencyImpl = Currency.from(currency);
        return core.getUnitAsDefault(currencyImpl.getCoreBRCryptoCurrency()).transform(Unit::create);
    }

    @Override
    public Optional<Set<? extends com.breadwallet.crypto.Unit>> unitsFor(com.breadwallet.crypto.Currency currency) {
        Currency currencyImpl = Currency.from(currency);
        CoreBRCryptoCurrency currencyCore = currencyImpl.getCoreBRCryptoCurrency();

        Set<Unit> units = new HashSet<>();
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
    public boolean hasUnitFor(com.breadwallet.crypto.Currency currency, com.breadwallet.crypto.Unit unit) {
        return unitsFor(currency).transform(input -> input.contains(unit)).or(false);
    }

    @Override
    public List<WalletManagerMode> getSupportedModes() {
        switch (core.getType()) {
            case BRCryptoBlockChainType.BLOCK_CHAIN_TYPE_BTC: {
                return Collections.singletonList(WalletManagerMode.P2P_ONLY);
            }
            case BRCryptoBlockChainType.BLOCK_CHAIN_TYPE_ETH: {
                return Arrays.asList(WalletManagerMode.API_ONLY, WalletManagerMode.API_WITH_P2P_SUBMIT);
            }
            case BRCryptoBlockChainType.BLOCK_CHAIN_TYPE_GEN: {
                return Collections.singletonList(WalletManagerMode.API_ONLY);
            }
            default:
                throw new IllegalStateException("Invalid network type");
        }
    }

    @Override
    public Optional<Address> addressFor(String address) {
        switch (core.getType()) {
            case BRCryptoBlockChainType.BLOCK_CHAIN_TYPE_BTC: {
                return Address.createAsBtc(address);
            }
            case BRCryptoBlockChainType.BLOCK_CHAIN_TYPE_ETH: {
                return Address.createAsEth(address);
            }
            case BRCryptoBlockChainType.BLOCK_CHAIN_TYPE_GEN: {
                // TODO(fix): Implement this
                return Optional.absent();
            }
            default:
                throw new IllegalStateException("Invalid network type");
        }
    }

    @Override
    public String toString() {
        return core.getName();
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
