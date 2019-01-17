//
//  BRFiat.swift
//  BRCrypto
//
//  Created by Ed Gamble on 11/24/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//
//  Copyright © 2018 breadwallet. All rights reserved.
//

import Core

public struct Fiat {
    /// TODO: Dicionary<String,Currency> w/ ISO<abcd> currencies

    public static let US = Currency (code: "USD", symbol: "$", name: "dollar", decimals: 2,
                                     baseUnit: (name: "cents", symbol: "c"))
    // TODO: Fill unit - better create default.
    public struct USD {
        public static let Cent   = US.baseUnit
        public static let Dollar = US.defaultUnit
    }

    public static let JP = Currency (code: "JPY", symbol: "JPY", name: "yen", decimals: 0,
                                     baseUnit: (name: "Yen", symbol: "Y"))
    public struct JPY {
        public static let Yen = JP.baseUnit
    }

}
