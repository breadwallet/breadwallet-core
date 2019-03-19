//
//  BRBitcash.swift
//  BRCrypto
//
//  Created by Ed Gamble on 11/26/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import BRCore

public struct Bitcash {
    public static let currency = Currency(code: "BCH",
                                          name: "Bitcoin Cash",
                                          baseUnit: Units.satoshi,
                                          defaultUnit: Units.bitcash,
                                          supportedUnits: Units.allCases)

    public enum Units: UInt8, CurrencyUnit, CaseIterable {
        case satoshi = 0
        case bit = 2
        case millibitcash = 5
        case bitcash = 8 // 1 Satoshi = 1e-8 BTC
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

