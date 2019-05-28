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

import com.breadwallet.crypto.jni.bitcoin.BRChainParams;
import com.google.common.base.Optional;

import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

// TODO: Add impl related code
public class Network {

    /* package */
    static Network create(String uids, String name, boolean isMainnet, Currency currency, long height,
                          Map<Currency, NetworkAssociation> associations) {
        String code = currency.getCode();
        if (code .equals(Currency.CODE_AS_BTC)) {
            return new Network(uids, name, isMainnet, currency, height, associations, new NetworkBtcImpl(isMainnet));
        } else if (code .equals(Currency.CODE_AS_BCH)) {
            return new Network(uids, name, isMainnet, currency, height, associations, null);
        } else if (code .equals(Currency.CODE_AS_ETH)) {
            return new Network(uids, name, isMainnet, currency, height, associations, null);
        } else {
            // TODO: Handle this
            throw new IllegalStateException(String.format("Invalid %s currency", code ));
        }
    }

    private final String uids;
    private final String name;
    private final boolean isMainnet;
    private final Currency currency;
    private final Set<Currency> currencies;
    private final Map<Currency, NetworkAssociation> associations;

    private long height;

    @Nullable
    private final NetworkBtcImpl btc;

    private Network(String uids, String name, boolean isMainnet, Currency currency, long height, Map<Currency,
            NetworkAssociation> associations, NetworkBtcImpl btc) {
        this.uids = uids;
        this.name = name;
        this.isMainnet = isMainnet;
        this.currency = currency;
        this.height = height;
        this.associations = associations;
        this.currencies = associations.keySet();  // TODO: This set is tied to the associations data structure; are we OK with this?
        this.btc = btc;
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

    @Override
    public String toString() {
        return name;
    }

    /* package */
    Optional<BRChainParams> asBtc() {
        return Optional.fromNullable(btc.core);
    }

    /* package */
    void setHeight(long height) {
        this.height = height;
    }
}
