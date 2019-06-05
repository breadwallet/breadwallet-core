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

import java.util.List;
import java.util.Set;

public interface Network {

    Optional<Unit> baseUnitFor(Currency currency);

    Optional<Unit> defaultUnitFor(Currency currency);

    Optional<Set<Unit>> unitsFor(Currency currency);

    Optional<Address> addressFor(String address);

    boolean hasUnitFor(Currency currency, Unit unit);

    boolean hasCurrency(Currency currency);

    Currency getCurrency();

    List<WalletManagerMode> getSupportedModes();

    String getUids();

    boolean isMainnet();

    long getHeight();

    boolean equals(Object o);

    int hashCode();
}
