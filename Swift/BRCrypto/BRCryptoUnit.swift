//
//  BRCryptoUnit.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import BRCryptoC

///
/// A unit of measure for a currency.  There can be multiple units for a given currency (analogous
/// to 'System International' units of (meters, kilometers, miles, ...) for a dimension of
/// 'length').  For example, Ethereum has units of: WEI, GWEI, ETHER, METHER, ... and Bitcoin of:
/// BTC, SATOSHI, ...
///
/// Each Currency has a 'baseUnit' - which is defined as the 'integer-ish' unit - such as SATOSHI
/// ane WEI for Bitcoin and Ethereum, respectively.  There can be multiple 'derivedUnits' - which
/// are derived by scaling off of a baseUnit.  For example, BTC and ETHER respectively.
///
public final class Unit: Hashable {
    internal let core: BRCryptoUnit

    public var currency: Currency {
        return Currency (core: cryptoUnitGetCurrency(core), take: false)
    }

    internal var uids: String {
        return asUTF8String (cryptoUnitGetUids (core))
    }

    public var name: String {
        return asUTF8String (cryptoUnitGetName (core))
    }

    public var symbol: String {
        return asUTF8String (cryptoUnitGetSymbol (core))
    }

    public var base: Unit {
        return Unit (core: cryptoUnitGetBaseUnit (core), take: false)
    }

    public var decimals: UInt8 {
        return cryptoUnitGetBaseDecimalOffset (core)
    }

    public func isCompatible (with that: Unit) -> Bool {
        return CRYPTO_TRUE == cryptoUnitIsCompatible (self.core, that.core)
    }

    public func hasCurrency (_ currency: Currency) -> Bool {
        return CRYPTO_TRUE == cryptoUnitHasCurrency (core, currency.core);
    }

    internal init (core: BRCryptoUnit,
                   take: Bool) {
        self.core = take ? cryptoUnitTake(core) : core
    }

    internal convenience init (core: BRCryptoUnit) {
        self.init (core: core, take: true)
    }

    internal convenience init (currency: Currency,
                               code: String,
                               name: String,
                               symbol: String) {
        self.init (core: cryptoUnitCreateAsBase (currency.core, code, name, symbol),
                   take: false)
    }

    internal convenience init (currency: Currency,
                               code: String,
                               name: String,
                               symbol: String,
                               base: Unit,
                               decimals: UInt8) {
        self.init (core: cryptoUnitCreate (currency.core, code, name, symbol, base.core, decimals),
                   take: false)
    }

    deinit {
        cryptoUnitGive (core)
    }

    public static func == (lhs: Unit, rhs: Unit) -> Bool {
        return lhs === rhs || CRYPTO_TRUE == cryptoUnitIsIdentical (lhs.core, rhs.core)
    }

    public func hash (into hasher: inout Hasher) {
        hasher.combine (uids)
    }
}
