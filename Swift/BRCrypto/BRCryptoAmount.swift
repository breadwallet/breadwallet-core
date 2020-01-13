//
//  BRCryptoAmount.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import Foundation
import BRCryptoC

///
/// An amount of currency.  This can be negative (as in, 'currency owed' rather then 'currency
/// owned').  Supports basic arithmetic operations (addition, subtraction, comparison); will
/// assert on !isCompatible for mismatched currency.
///
public final class Amount {
    /// The underlying Core memory reference
    internal let core: BRCryptoAmount

    /// The (default) unit.  Without this there is no reasonable implementation of
    /// CustomeStringConvertable.  This property is only used in the `description` function
    /// and to ascertain the Amount's currency typically for consistency in add/sub functions.
    public var unit: Unit {
        return Unit (core: cryptoAmountGetUnit (core), take: false)
    }

    /// The currency
    public var currency: Currency {
        return unit.currency
    }

    public var isNegative: Bool {
        return CRYPTO_TRUE == cryptoAmountIsNegative (core)
    }

    ///
    /// Convert `Amount` into `Double` using `unit`
    ///
    /// - Note: This can introduce inaccuracy as `Double` precision is less than the possible
    /// number of UInt256 decimals (78).  For example, an Amount of 123456789012345678.0 WEI
    /// (approx 1.234e17) when converted will have a different double representation:
    ///
    /// ```
    /// let a7 = Amount.create(string: "123456789012345678.0", negative: false, unit: ETH_WEI)
    /// XCTAssertEqual(1.2345678901234568e17, a7?.double(as: ETH_WEI)
    /// ```
    /// (the final three digits of '678' are rounded to '680')
    ///
    /// - Parameter unit:
    ///
    public func double (as unit: Unit) -> Double? {
        var overflow: BRCryptoBoolean = CRYPTO_FALSE
        let value = cryptoAmountGetDouble(core, unit.core, &overflow)
        return CRYPTO_TRUE == overflow ? nil : value
    }

    /// Convert `Amount` into `String` using `unit` and `formatter`.
    ///
    /// - Note: This can introduce inaccuracy.  Use of the `formatter` *requires* that Amount be
    /// converted to a Double and the limit in Double precision can compromise the UInt256 value.
    /// For example, an Amount of 123456789012345678.0 WEI (approx 1.234e20) when converted will
    /// have different string representation:
    ///
    /// ```
    /// let a7 = Amount.create(string: "123456789012345678.0", negative: false, unit: ETH_WEI)
    /// XCTAssertEqual   ("123456789012345678",         a7?.string(base: 10, preface: ""))
    /// XCTAssertEqual   ("wei123,456,789,012,346,000", a7?.string(as: ETH_WEI)!)
    /// XCTAssertNotEqual("wei123,456,789,012,345,678", a7?.string(as: ETH_WEI)!)
    /// ```
    /// (the final 6 digits of '345,678' are rounded to '346,000')
    ///
    /// - Parameters:
    ///   - unit:
    ///   - formatter: an optional formatter.  If not provided, `formatterWith (unit: unit)` is
    ///        used.
    ///
    public func string (as unit: Unit, formatter: NumberFormatter? = nil) -> String? {
        return double (as: unit)
            // If we are going to use a formatter, then we must have an NSNumber with a Double.
            // A Double will have limited precision compared to the UInt256 held by `Amount`.
            .flatMap { (formatter ?? self.formatterWith (unit: unit))
                .string (from: NSNumber(value: $0)) }
    }

    public func string (pair: CurrencyPair, formatter: NumberFormatter? = nil) -> String? {
        return pair.exchange (asBase: self)?
            .string (as: pair.quoteUnit, formatter: formatter)
    }

    /// INTERNAL: Returns the low uint64_t value optionally.
    internal var integerRawSmall: UInt64? {
        var overflow: BRCryptoBoolean = CRYPTO_FALSE
        let value = cryptoAmountGetIntegerRaw (core, &overflow)
        return CRYPTO_TRUE == overflow ? nil : value
    }

    ///
    /// Return a 'raw string' (as an integer in self's base unit) using `base` and `preface`.
    /// Caution is warranted: this is a string w/o any context (currency in a particular Unit).
    /// Don't use this tp subvert the other `string(as Unit: Unit, ...)` function.  Should only be
    /// used for 'partner' interfaces when their API requires a string value in the base unit.
    ///
    /// - Note: The amount's sign is utterly ignored.
    /// - Note: Unless there is a 'preface', the result may have leading zeros.
    /// - Note: For base 16, lowercased hex digits are returned.
    ///
    /// - Parameters:
    ///   - base: the numeric base - one of {2, 10, 16}.  Defaults to 16
    ///   - preface: a strig preface, defaults to '0x'
    ///
    /// - Returns: the amount in the base unit as a 'raw string'
    ///
    public func string (base: UInt8 = 16, preface: String = "0x") -> String {
        return asUTF8String (cryptoAmountGetStringPrefaced(self.core, Int32(base), preface))
    }

    public func isCompatible (with that: Amount) -> Bool {
        return CRYPTO_TRUE == cryptoAmountIsCompatible (self.core, that.core)
    }

    public func hasCurrency (_ currency: Currency) -> Bool {
        return CRYPTO_TRUE == cryptoAmountHasCurrency (core, currency.core)
    }

    public func add (_ that: Amount) -> Amount? {
        precondition (isCompatible (with: that))
        return cryptoAmountAdd (self.core, that.core)
            .map { Amount (core: $0, take: false) }
    }

    public func sub (_ that: Amount) -> Amount? {
        precondition (isCompatible (with: that))
        return cryptoAmountSub (self.core, that.core)
            .map { Amount (core: $0, take: false) }
    }

    public func convert (to unit: Unit) -> Amount? {
        return cryptoAmountConvertToUnit (self.core, unit.core)
                .map { Amount (core: $0, take: false) }
    }

    public var negate: Amount {
        return Amount (core: cryptoAmountNegate (core), take: false)
    }

    public var isZero: Bool {
        return CRYPTO_TRUE == cryptoAmountIsZero (core)
    }
    
    internal init (core: BRCryptoAmount,
                   take: Bool) {
        self.core = take ? cryptoAmountTake(core) : core
    }

    public static func create (double: Double, unit: Unit) -> Amount {
        return Amount (core: cryptoAmountCreateDouble (double, unit.core),
                       take: false)
    }

    public static func create (integer: Int64, unit: Unit) -> Amount {
        return Amount (core: cryptoAmountCreateInteger (integer, unit.core),
                       take: false)
    }

    ///
    /// Parse `string` into an `Amount`.  The string has some limitations:
    ///  * it cannot start with '-' or '+' (no sign character)
    ///  * if it starts with '0x', it is interpreted as a 'hex string'
    ///  * if it has a decimal point, it is interpreted as a 'decimal string'
    ///  * otherwise, it is interpreted as an 'integer string'
    ///
    /// If it is a 'decimal string' and the string includes values after the decimal point, then
    /// the number of values must be less than or equal to the unit's decimals.  For example a
    /// string of "1.1" in BTC_SATOSHI won't parse as BTC_SATOSHI has 0 decimals (it is a base unit
    /// and thus must be an integer).  Whereas, a string of "1.1" in BTC_BTC will parse as BTC_BTC
    /// has 8 decimals and the string has but one.  ("0.123456789" won't parse as it has 9 digits
    /// after the decimal; both "1." and "1.0" will parse.)
    ///
    /// Additionally, `string` cannot have any extraneous starting or ending characters.  Said
    /// another way, `string` must be fully consumed.  Thus "10w" and "w10" and "1.1w" won't parse.
    ///
    /// - Parameters:
    ///   - string: the string to parse
    ///   - negative: true if negative; false otherwise
    ///   - unit: the string's unit
    ///
    /// - Returns: The `Amount` if the string can be parsed.
    ///
    public static func create (string: String, negative: Bool = false, unit: Unit) -> Amount? {
        return cryptoAmountCreateString (string, (negative ? CRYPTO_TRUE : CRYPTO_FALSE), unit.core)
            .map { Amount (core: $0, take: false) }
    }

    ///
    /// Produce a default NumberFormatter for `unit`.  Uses the User's current locale, a number
    /// style of `.currency`, a currency symbol of `unit.symbol`, and factional digits of
    /// `unit.decimals` (if non-zero).
    ///
    /// - Parameter unit: the unit
    ///
    /// - Returns: the formatter for unit
    ///
    private func formatterWith (unit: Unit) -> NumberFormatter {
        let formatter = NumberFormatter()
        formatter.locale = Locale.current
        formatter.numberStyle = .currency
        formatter.currencySymbol = unit.symbol;
        formatter.generatesDecimalNumbers = 0 != unit.decimals
        formatter.maximumFractionDigits = Int(unit.decimals)
        return formatter
    }

    deinit {
        cryptoAmountGive (core)
    }
}

extension Amount {
    public static func + (lhs: Amount, rhs: Amount) -> Amount? {
        return lhs.add(rhs)
    }

    public static func - (lhs: Amount, rhs: Amount) -> Amount? {
        return lhs.sub(rhs)
    }
}

///
/// Note that incompatible units may return 'false' for all comparisons.  This violates the
/// expectation that `lhs` and `rhs` satisfy one of: ==, >, and <.  Caution.
///
extension Amount: Comparable {
    public static func == (lhs: Amount, rhs: Amount) -> Bool {
        return CRYPTO_COMPARE_EQ == cryptoAmountCompare (lhs.core, rhs.core)
    }

    public static func < (lhs: Amount, rhs: Amount) -> Bool {
        return CRYPTO_COMPARE_LT == cryptoAmountCompare (lhs.core, rhs.core)
    }

    public static func > (lhs: Amount, rhs: Amount) -> Bool {
        return CRYPTO_COMPARE_GT == cryptoAmountCompare (lhs.core, rhs.core)
    }

    public static func != (lhs: Amount, rhs: Amount) -> Bool {
        return CRYPTO_COMPARE_EQ != cryptoAmountCompare (lhs.core, rhs.core)
    }

    public static func <= (lhs: Amount, rhs: Amount) -> Bool {
        return CRYPTO_COMPARE_GT != cryptoAmountCompare (lhs.core, rhs.core)
    }

    public static func >= (lhs: Amount, rhs: Amount) -> Bool {
        return CRYPTO_COMPARE_LT != cryptoAmountCompare (lhs.core, rhs.core)
    }
}

extension Amount: CustomStringConvertible {
    public var description: String {
        return string (as: unit) ?? "<nan>"
    }
}

///
/// "A currency pair is the quotation of the relative value of a currency unit against the unit of
/// another currency in the foreign exchange market. The currency that is used as the reference is
/// called the counter currency, quote currency or currency and the currency that is quoted in
/// relation is called the base currency or transaction currency.
///
/// "The quotation EUR/USD 1.2500 means that one euro is exchanged for 1.2500 US dollars. Here, EUR
/// is the base currency and USD is the quote currency(counter currency)."
///
/// Ref: https://en.wikipedia.org/wiki/Currency_pair
///
/// Thus BTC/USD=1000 means that one BTC is changed for $1,000.  Here, BTC is the base currency
/// and USD is the quote currency.  You would create such an exchange with:
///
///    let BTC_USD_Pair = CurrencyPair (baseUnit:  Bitcoin.Units.BTC,
///                                     quoteUnit: Fiat.USD.Dollar,
///                                     exchangeRate: 1000.0)
///
/// and then use it to find the value of 2 BTC with:
///
///    BTC_USD_Pair.exchange (asBase: Amount (value: 2.0, unit: Bitcoin.Units.BTC))
///
/// which would return: $2,000  (as Amount of 2000.0 in Fiat.USD.Dollar)
///

//public struct CurrencyPair {
//    public func exchange (asBase amount: Amount) -> Amount? {
//        return nil
//    }
//}

public struct CurrencyPair {

    /// In EUR/USD=1.2500, the `baseCurrecny` is EUR.
    public let baseUnit: Unit

    /// In EUR/USD=1.250, the `quoteCurrecny` is USD.
    public let quoteUnit: Unit

    /// In EUR/USD=1.2500, the `exchangeRate` is 1.2500 which represents the number of USD that
    /// one EUR can be exchanged for.
    public let exchangeRate: Double

    ///
    /// Apply `self` CurrencyPair to convert `asBase` (in `baseCurrency`) to `quoteCurrency`.  This
    /// is essentially `asBase * exchangeRate`
    ///
    /// - Parameter amount: the amount of `baseCurrency`
    ///
    /// - Returns: the amount as `quoteCurrency`
    ///
    public func exchange(asBase amount: Amount) -> Amount? {
        return amount.double(as: baseUnit)
            .flatMap { Amount.create (double: $0 * exchangeRate, unit: quoteUnit) }
    }

    ///
    /// Apply `self` CurrencyPair to convert `asQuote` (in `quoteCurrency`) to `baseCurrency`.  This
    /// is essentially `asQuote / exchangeRate`.
    ///
    /// - Parameter amount: the amount of `quoteCurrency`
    ///
    /// - Returns: the amount as `baseCurrency`
    ///
    public func exchange(asQuote amount: Amount) -> Amount? {
        return amount.double (as: quoteUnit)
            .flatMap { Amount.create (double: $0 / exchangeRate, unit: baseUnit) }
    }
}

extension CurrencyPair: CustomStringConvertible {
    public var description: String {
        return "\(baseUnit.name)/\(quoteUnit.name)=\(exchangeRate)"
    }
}
