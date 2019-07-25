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

import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

import java.util.List;
import java.util.Set;

public interface Network {

    Optional<? extends Unit> baseUnitFor(Currency currency);

    Optional<? extends Unit> defaultUnitFor(Currency currency);

    Optional<Set<? extends Unit>> unitsFor(Currency currency);

    Optional<? extends Address> addressFor(String address);

    Optional<Boolean> hasUnitFor(Currency currency, Unit unit);

    boolean hasCurrency(Currency currency);

    Currency getCurrency();

    Set<? extends Currency> getCurrencies();

    List<? extends NetworkFee> getFees();

    NetworkFee getMinimumFee();

    Optional<? extends Currency> getCurrencyByCode(String code);

    List<WalletManagerMode> getSupportedModes();

    String getUids();

    String getName();

    boolean isMainnet();

    UnsignedLong getHeight();

    boolean equals(Object o);

    int hashCode();
}
