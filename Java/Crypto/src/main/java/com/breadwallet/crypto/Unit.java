package com.breadwallet.crypto;

public class Unit {
    public final Currency currency;
    public final String name;
    public final String symbol;
    public final long scale;

    public final Unit base;

    public boolean isCompatible (Unit that) {
        return this == that ||
                this.currency == that.currency;
    }

    public Unit (Currency currency, String name, String symbol) {
        this.currency = currency;
        this.name = name;
        this.symbol = symbol;
        this.scale = 1;
        this.base = null;
    }

    public Unit(String name, String symbol, long scale, Unit base) {
        this.currency = base.currency;
        this.name = name;
        this.symbol = symbol;
        this.scale = scale;
        this.base = base;
    }

}
