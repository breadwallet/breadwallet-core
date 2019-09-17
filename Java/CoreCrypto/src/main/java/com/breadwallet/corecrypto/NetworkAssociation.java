/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import java.util.Set;

/* package */
final class NetworkAssociation {

    private final Unit baseUnit;
    private final Unit defaultUnit;
    private final Set<Unit> units;

    /* package */
    NetworkAssociation(Unit baseUnit, Unit defaultUnit, Set<Unit> units) {
        this.baseUnit = baseUnit;
        this.defaultUnit = defaultUnit;
        this.units = units;
    }

    /* package */
    Unit getBaseUnit() {
        return baseUnit;
    }

    /* package */
    Unit getDefaultUnit() {
        return defaultUnit;
    }

    /* package */
    Set<Unit> getUnits() {
        return units;
    }
}
