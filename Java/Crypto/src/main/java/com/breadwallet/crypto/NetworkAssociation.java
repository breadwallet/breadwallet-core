package com.breadwallet.crypto;

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
