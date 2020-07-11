/*
 * Amount
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.base.Optional;

import java.lang.Double;
import java.text.NumberFormat;

public interface Amount extends Comparable<Amount> {

    static Amount create(double value, Unit unit) {
        return CryptoApi.getProvider().amountProvider().create(value, unit);
    }

    static Amount create(long value, Unit unit) {
        return CryptoApi.getProvider().amountProvider().create(value, unit);
    }

    /**
     * Parse `string` into an `Amount`.
     *
     * The string has some limitations:
     *   - it cannot start with '-' or '+' (no sign character)
     *   - if it starts with '0x', it is interpreted as a 'hex string'
     *   - if it has a decimal point, it is interpreted as a 'decimal string'
     *   - otherwise, it is interpreted as an 'integer string'
     *
     * If it is a 'decimal string' and the string includes values after the decimal point, then
     * the number of values must be less than or equal to the unit's decimals.  For example a
     * string of "1.1" in BTC_SATOSHI won't parse as BTC_SATOSHI has 0 decimals (it is a base unit
     * and thus must be an integer).  Whereas, a string of "1.1" in BTC_BTC will parse as BTC_BTC
     * has 8 decimals and the string has but one.  ("0.123456789" won't parse as it has 9 digits
     * after the decimal; both "1." and "1.0" will parse.)
     *
     * Additionally, `string` cannot have any extraneous starting or ending characters.  Said
     * another way, `string` must be fully consumed.  Thus "10w" and "w10" and "1.1w" won't parse.
     *
     * @param value the string to parse
     * @param isNegative true if negative; false otherwise
     * @param unit the string's unit
     *
     * @return The `Amount` if the string can be parsed.
     */
    static Optional<Amount> create(String value, boolean isNegative, Unit unit) {
        return CryptoApi.getProvider().amountProvider().create(value, isNegative, unit);
    }

    Currency getCurrency();

    Unit getUnit();

    boolean hasCurrency(Currency currency);

    boolean isCompatible(Amount withAmount);

    boolean isNegative();

    boolean isZero();

    Optional<? extends Amount> add(Amount o);

    Optional<? extends Amount> sub(Amount o);

    Amount negate();

    Optional<? extends Amount> convert(Unit toUnit);

    Optional<String> toStringAsUnit(Unit asUnit);

    /**
     * Convert `Amount` into `String` using `unit` and `formatter`.
     *
     * - Note: This can introduce inaccuracy.  Use of the `formatter` *requires* that Amount be
     *         converted to a Double and the limit in Double precision can compromise the UInt256 value.
     *         For example, an Amount of 123456789012345678.0 WEI (approx 1.234e20) when converted will
     *         have different string representation:
     *
     * <pre>{@code
     *   Currency eth = Currency.create("Ethereum", "Ethereum", "eth", "native", null);
     *   Unit wei_eth = Unit.create(eth, "ETH-WEI", "WEI", "wei");
     *
     *   Amount a = Amount.create("123456789012345678.0", false, wei_eth).get();
     *   assertEquals("123456789012345678", a.toStringWithBase(10, ""));
     *   assertEquals("wei123,456,789,012,345,680", a.toStringAsUnit(wei_eth).get());
     *   assertNotEquals("wei123,456,789,012,345,678", a.toStringAsUnit(wei_eth).get());
     * }</pre>
     *
     *  In the snippet above, the final 6 digits of '345,678' are rounded to '345,680'.
     */
    Optional<String> toStringAsUnit(Unit asUnit, NumberFormat numberFormatter);

    Optional<String> toStringFromPair(CurrencyPair pair);

    Optional<String> toStringFromPair(CurrencyPair pair, NumberFormat numberFormatter);

    /**
     * Return a 'raw string' (as an integer in self's base unit) using `base` and `preface`.
     *
     * Caution is warranted: this is a string w/o any context (currency in a particular Unit).
     * Don't use this to subvert the other `string(as Unit: Unit, ...)` function.  Should only be
     * used for 'partner' interfaces when their API requires a string value in the base unit.
     *
     * Note: The amount's sign is utterly ignored.
     * Note: Unless there is a 'preface', the result may have leading zeros.
     * Note: For base 16, lowercased hex digits are returned.
     *
     * @param base the numeric base - one of {2, 10, 16}
     * @param preface a string preface
     *
     * @return the amount in the base unit as a 'raw string'
     */
    String toStringWithBase(int base, String preface);

    String toString();

    boolean equals(Object o);

    int hashCode();

    /**
     * Convert amount into {@link Double} using {@link Unit}
     *
     * Note: This can introduce inaccuracy as `Double` precision is less than the possible
     *       number of UInt256 decimals (78).  For example, an Amount of 123456789012345678.0 WEI
     *       (approx 1.234e17) when converted will have a different double representation:
     *
     * <pre>{@code
     *   Currency eth = Currency.create("Ethereum", "Ethereum", "eth", "native", null);
     *   Unit wei_eth = Unit.create(eth, "ETH-WEI", "WEI", "wei");
     *
     *   Amount aAmount = Amount.create("123456789012345678.0", false, wei_eth).get();
     *   double aDouble = aAmount.doubleAmount(wei_eth).get();
     *   assertEquals(1.2345678901234568e17, aDouble, 0.0);
     * }</pre>
     *
     * In the snippet above, the final three digits of '678' are rounded to '680'.
     */
    Optional<Double> doubleAmount(Unit asUnit);
}
