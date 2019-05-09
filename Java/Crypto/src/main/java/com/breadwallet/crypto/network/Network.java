/*
 * Network
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.network;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.walletmanager.WalletManagerMode;
import com.google.common.base.Function;
import com.google.common.base.Optional;

import org.checkerframework.checker.nullness.compatqual.NullableDecl;

import java.util.List;
import java.util.Map;
import java.util.Set;

// TODO: Add impl related code
public final class Network {

    private static final class Association {
        private final Unit baseUnit;
        private final Unit defaultUnit;
        private final Set<Unit> units;

        public Association(Unit baseUnit, Unit defaultUnit, Set<Unit> units) {
            this.baseUnit = baseUnit;
            this.defaultUnit = defaultUnit;
            this.units = units;
        }
    }

    private final String uids;
    private final String name;
    private final boolean isMainnet;
    private final Currency currency;
    private final Set<Currency> currencies;
    private final long height;
    private final Map<Currency, Association> associations;

    private Network(String uids, String name, boolean isMainnet, Currency currency, long height, Map<Currency, Association> associations) {
        this.uids = uids;
        this.name = name;
        this.isMainnet = isMainnet;
        this.currency = currency;
        this.height = height;
        this.associations = associations;
        this.currencies = associations.keySet();  // TODO: This set is tied to the associations data structure; are we OK with this?
    }

    public Optional<Unit> baseUnitFor(Currency currency) {
        Association association = associations.get(currency);
        return association == null ? Optional.absent() : Optional.of(association.baseUnit);
    }

    public Optional<Unit> defaultUnitFor(Currency currency) {
        Association association = associations.get(currency);
        return association == null ? Optional.absent() : Optional.of(association.defaultUnit);
    }

    public Optional<Set<Unit>> unitsFor(Currency currency) {
        Association association = associations.get(currency);
        return association == null ? Optional.absent() : Optional.of(association.units);
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

    public long getHeight() {
        return height;
    }

    public List<WalletManagerMode> getSupportedModes() {
        // TODO: Fill in once impl code is in place
        return null;
    }

    public Optional<Address> addressFor(String address) {
        // TODO: Fill in once impl code is in place
        return Optional.absent();
    }
}
