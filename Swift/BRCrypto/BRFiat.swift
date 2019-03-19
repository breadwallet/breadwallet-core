//
//  BRFiat.swift
//  BRCrypto
//
//  Created by Ed Gamble on 11/24/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//
//  Copyright © 2018 breadwallet. All rights reserved.
//

import BRCore

public struct Fiat {
    /// TODO: Dicionary<String,Currency> w/ ISO<abcd> currencies

    public static let US = Currency(code: "USD",
                                          name: "dollar",
                                          baseUnit: USD.cents,
                                          defaultUnit: USD.dollar)

    public enum USD: UInt8, CurrencyUnit {
        case cents = 0
        case dollar = 2

        public var symbol: String {
            switch self {
            case .cents: return "¢"
            case .dollar: return "$"
            }
        }
    }

    public static let JP = Currency(code: "JPY",
                                    name: "yen",
                                    baseUnit: JPY.yen,
                                    defaultUnit: JPY.yen)

    public enum JPY: UInt8, CurrencyUnit {
        case yen = 0

        public var symbol: String {
            return "¥"
        }
    }

}
