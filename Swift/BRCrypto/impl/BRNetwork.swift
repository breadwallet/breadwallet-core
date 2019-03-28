//
//  BRNetwork.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//

class NetworkBase: Network {
    public let name: String

    public let isMainnet: Bool

    public let currency: Currency

    public var currencies: Set<Currency>

    internal var associations: Dictionary<Currency, Association> = [:]

    func currencyBy (code: String) -> Currency? {
        return currencies.first { $0.code == code }
    }

    public func hasCurrency(_ that: Currency) -> Bool {
        return currencies.contains(that)
    }

    public func baseUnitFor(currency: Currency) -> Unit? {
        return associations[currency]?.baseUnit
    }

    public func defaultUnitFor(currency: Currency) -> Unit? {
        return associations[currency]?.defaultUnit
    }

    public func unitsFor(currency: Currency) -> Set<Unit>? {
        return associations[currency]?.units
    }

    public func hasUnitFor(currency: Currency, unit: Unit) -> Bool? {
        return unitsFor(currency: currency)?.contains(unit)
    }

    internal struct Association {
        let baseUnit: Unit
        let defaultUnit: Unit
        let units: Set<Unit>
    }

    public init (name: String,
                 isMainnet: Bool,
                 currency: Currency,
                 associations: Dictionary<Currency, Association>) {
        self.name = name
        self.isMainnet = isMainnet
        self.currency = currency
        self.associations = associations

        self.currencies = associations.keys.reduce (into: Set<Currency>()) {
            $0.insert($1)
        }
    }
}
