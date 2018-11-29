//
//  BRCoreXTests.swift
//  BRCoreXTests
//
//  Created by Ed Gamble on 10/30/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import XCTest
@testable import BRCrypto

class BRCoreXTests: XCTestCase {

    override func setUp() {
    }

    override func tearDown() {
    }

    func testCurrency() {
        let btc = Bitcoin.currency
        let eth = Ethereum.currency

        XCTAssert ("BTC" == btc.code)
        XCTAssert (btc == btc)
        XCTAssert (btc != eth)
    }

    func testUnit () {
        XCTAssertFalse(Ethereum.Units.ETHER.isCompatible(Bitcoin.Units.SATOSHI))
    }

    func testAmount () {
        let ETHER = Ethereum.Units.ETHER
        let WEI   = Ethereum.Units.WEI
        let GWEI  = Ethereum.Units.GWEI

        XCTAssert (nil != Amount (exactly: 1.5, unit: ETHER))
        XCTAssert (nil != Amount (exactly: 1.0, unit: ETHER))
        XCTAssert (nil == Amount (exactly: 1.5, unit: WEI))

        XCTAssert (1.5 == Amount (exactly: 1.5, unit: ETHER)?.double)
        XCTAssert (1.5 * 1e9 == Amount (exactly: 1.5, unit: ETHER)?.coerce(unit: GWEI).double)

        XCTAssert (+10 == Amount(value: +10, unit: ETHER).double)
        XCTAssert (-10 == Amount(value: -10, unit: ETHER).double)

        XCTAssert (-10 == Amount(value: -10.0, unit: ETHER).double)

        XCTAssertEqual(25.0, Amount(value: 10.0, unit: ETHER).scale(by: 2.5)?.double ?? 0.0, accuracy: 1e-6)
        XCTAssertEqual( 2.0, Amount(value: 10.0, unit: ETHER).scale(by: 1/5)?.double ?? 0.0, accuracy: 1e-6)

        XCTAssert(Amount (value: 1, unit: ETHER) >  Amount (value: 1, unit: GWEI))
        XCTAssert(Amount (value: 1, unit: ETHER) == Amount (value: 1, unit: ETHER))
        XCTAssert(Amount (value: 1, unit: ETHER) != Amount (value: 2, unit: ETHER))
        XCTAssert(Amount (value: 1, unit: ETHER) == Amount (value: 1e9, unit: GWEI))
        XCTAssert(Amount (value: 1e-3, unit: ETHER) == Amount (value: 1e6, unit: GWEI))

        XCTAssert(Amount (value: 2.5, unit: ETHER) == Amount (value: 1.5, unit: ETHER) + Amount (value: 1.0, unit: ETHER))
        XCTAssert(Amount (value: 0.5, unit: ETHER) == Amount (value: 1.5, unit: ETHER) - Amount (value: 1.0, unit: ETHER))

        let a1 = Amount (value: 1, unit: ETHER)
        let a2 = Amount (value: 1, unit: Bitcoin.Units.SATOSHI)
        XCTAssertTrue  (a1.isCompatible(a1))
        XCTAssertFalse (a1.isCompatible(a2))

        XCTAssertEqual(a1.description, "1.0 \(Ethereum.currency.symbol)")

        XCTAssertEqual    ("9.123", Amount (value: 9.12345, unit: ETHER).describe(decimals: 3, withSymbol: false))
        XCTAssertNotEqual ("9.123", Amount (value: 9.12345, unit: ETHER).describe(decimals: 4, withSymbol: false))
        XCTAssertEqual    ("9.123 \(ETHER.symbol)", Amount (value: 9.12345, unit: ETHER).describe(decimals: 3, withSymbol: true))
    }

    func testCurrencyPair () {
        let BTC = Bitcoin.currency.defaultUnit!
        let USD = Fiat.US.defaultUnit!

        let pair = CurrencyPair (baseCurrency: BTC, quoteCurrency: USD, exchangeRate: 6000)

        // BTC -> USD
        let inUSD = pair.exchange(asBase: Amount (value: 1.0, unit: BTC))
        XCTAssertEqual(6000, inUSD?.double ?? 0, accuracy: 1e-6)

        // USD -> BTC
        let inBTC = pair.exchange(asQuote: Amount (value: 6000.0, unit: USD))
        XCTAssertEqual(1.0, inBTC?.double ?? 0, accuracy: 1e-6)

        XCTAssertEqual("\(BTC.name)/\(USD.name)=\(6000.0)", pair.description)
    }

    
//    func testPerformanceExample() {
//        self.measure {
//        }
//    }

}
