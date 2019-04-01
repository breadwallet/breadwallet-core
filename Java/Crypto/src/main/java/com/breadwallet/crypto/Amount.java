/*
 * Amount
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import java.math.BigInteger;

public class Amount {
    public final BigInteger value;  // UInt256 *core
    public final Unit unit;
    public final boolean isNegative;

    private Amount (BigInteger value, Unit unit) {
        this.value = value;
        this.unit  = unit;
        this.isNegative = -1 == value.signum();
    }

    public Amount (long value, Unit unit) {
        this (BigInteger.valueOf(value), unit);
    }
}
