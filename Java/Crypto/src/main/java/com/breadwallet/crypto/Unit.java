/*
 * Unit
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.primitives.UnsignedInteger;

public interface Unit {

    Currency getCurrency();

    String getName();

    String getSymbol();

    Unit getBase();

    UnsignedInteger getDecimals();

    boolean isCompatible(Unit other);

    boolean hasCurrency(Currency currency);

    boolean equals(Object o);

    int hashCode();
}
