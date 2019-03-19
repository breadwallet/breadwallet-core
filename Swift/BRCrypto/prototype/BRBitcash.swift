//
//  BRBitcash.swift
//  BRCrypto
//
//  Created by Ed Gamble on 11/26/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import BRCore

public struct Bitcash {
    public static let currency = Currency (code: "BCH", symbol:  "₿", name: "Bitcash", decimals: 8,
                                           baseUnit: (name: "SAT", symbol: "sat"))
    public struct Units {
        public static let SATOSHI = Bitcoin.currency.baseUnit!
        public static let BITCASH = Bitcoin.currency.defaultUnit!
    }

    public struct Networks {
        public static let mainnet = Network.bitcash (name: "BCH Mainnet", forkId: 0x00, chainParams: bitcashParams (1))
        public static let testnet = Network.bitcash (name: "BCH Testnet", forkId: 0x40, chainParams: bitcashParams (0))
    }

    public struct AddressSchemes {
        public static let segwit = BitcoinSegwitAddressScheme()
        public static let legacy = BitcoinLegacyAddressScheme()

        public static let `default` = legacy
    }
}

