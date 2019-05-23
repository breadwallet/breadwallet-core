package com.breadwallet.crypto;

import java.util.Set;

final class NetworkAssociation {
    private final Unit baseUnit;
    private final Unit defaultUnit;
    private final Set<Unit> units;

    public NetworkAssociation(Unit baseUnit, Unit defaultUnit, Set<Unit> units) {
        this.baseUnit = baseUnit;
        this.defaultUnit = defaultUnit;
        this.units = units;
    }

    public Unit getBaseUnit() {
        return baseUnit;
    }

    public Unit getDefaultUnit() {
        return defaultUnit;
    }

    public Set<Unit> getUnits() {
        return units;
    }
}
