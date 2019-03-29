//
//  BRCryptoAmpuntTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 10/30/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import XCTest
@testable import BRCrypto

class BRCryptoAmountTests: XCTestCase {

    override func setUp() {
    }

    override func tearDown() {
    }

    func testCurrency() {
        let btc = Currency (uids: "Bitcoin", name: "Bitcoin", code: "BTC", type: "native")

        XCTAssert (btc.name == "Bitcoin")
        XCTAssert (btc.code == "BTC")
        XCTAssert (btc.type == "native")

        let eth = Currency (uids: "Ethereum", name: "Ethereum", code: "ETH", type: "native")
        XCTAssert (eth.name == "Ethereum")
        XCTAssert (eth.code == "ETH")
        XCTAssert (eth.type == "native")

        XCTAssertFalse (btc.name == eth.name)
        XCTAssertFalse (btc.code == eth.code)
        XCTAssertTrue  (btc.type == eth.type)
     }

    func testUnit () {
        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native")
        let eth = Currency (uids: "Ethereum", name: "Ethereum", code: "ETH", type: "native")

        let BTC_SATOSHI = BRCrypto.Unit (currency: btc, uids: "BTC-SAT",  name: "Satoshi", symbol: "SAT")
        XCTAssert (BTC_SATOSHI.currency.code == btc.code)
        XCTAssert (BTC_SATOSHI.name == "Satoshi")
        XCTAssert (BTC_SATOSHI.symbol == "SAT");
        XCTAssertTrue  (BTC_SATOSHI.hasCurrency (btc))
        XCTAssertFalse (BTC_SATOSHI.hasCurrency (eth))
        XCTAssertTrue  (BTC_SATOSHI.isCompatible (with: BTC_SATOSHI))
        XCTAssertTrue  (BTC_SATOSHI.core == BTC_SATOSHI.base.core)

        let BTC_BTC = BRCrypto.Unit (currency: btc, uids: "BTC-BTC", name: "Bitcoin", symbol: "B", base: BTC_SATOSHI, decimals: 8)
        XCTAssert (BTC_BTC.currency.code == btc.code)
        XCTAssertTrue  (BTC_BTC.isCompatible (with: BTC_SATOSHI))
        XCTAssertTrue  (BTC_SATOSHI.isCompatible (with: BTC_BTC))

        let ETH_WEI = BRCrypto.Unit (currency: eth, uids: "ETH-WEI", name: "WEI", symbol: "wei")
        XCTAssertFalse (ETH_WEI.isCompatible (with: BTC_BTC))
        XCTAssertFalse (BTC_BTC.isCompatible (with: ETH_WEI))
    }

    func testAmount () {
        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native")
        let eth = Currency (uids: "Ethereum", name: "Ethereum", code: "ETH", type: "native")

        let BTC_SATOSHI = BRCrypto.Unit (currency: btc, uids: "BTC-SAT",  name: "Satoshi", symbol: "SAT")
        let BTC_BTC = BRCrypto.Unit (currency: btc, uids: "BTC-BTC",  name: "Bitcoin", symbol: "B", base: BTC_SATOSHI, decimals: 8)

        let ETH_WEI  = BRCrypto.Unit (currency: eth, uids: "ETH-WEI", name: "WEI",   symbol: "wei")
        let ETH_GWEI = BRCrypto.Unit (currency: eth, uids: "ETH-GWEI", name: "GWEI",  symbol: "gwei", base: ETH_WEI, decimals: 9)
        let ETH_ETHER = BRCrypto.Unit (currency: eth, uids: "ETH-ETH", name: "ETHER", symbol: "E",    base: ETH_WEI, decimals: 18)

        let btc1 = Amount.create (integer: 100000000, unit: BTC_SATOSHI)
        XCTAssert (100000000 == btc1.double (as: BTC_SATOSHI))
        XCTAssert (1         == btc1.double (as: BTC_BTC))

        let btc2 = Amount.create (integer: 1,   unit: BTC_BTC)
        XCTAssert (1         == btc2.double (as: BTC_BTC))
        XCTAssert (100000000 == btc2.double (as: BTC_SATOSHI))

        XCTAssert (btc1 == btc2)

        let btc3 = Amount.create (double: 1.5, unit: BTC_BTC);
        XCTAssert (1.5       == btc3.double (as: BTC_BTC))
        XCTAssert (150000000 == btc3.double (as: BTC_SATOSHI))

        let btc4 = Amount.create (double: -1.5, unit: BTC_BTC);
        XCTAssertTrue (btc4.isNegative)
        XCTAssert (-1.5       == btc4.double (as: BTC_BTC))
        XCTAssert (-150000000 == btc4.double (as: BTC_SATOSHI))

        XCTAssert ("-B1.50"          == btc4.string (as: BTC_BTC))
        XCTAssert ("-SAT150,000,000" == btc4.string (as: BTC_SATOSHI))

        let eth1 = Amount.create (double: 1e9, unit: ETH_GWEI)
        let eth2 = Amount.create (double: 1.0, unit: ETH_ETHER)
        let eth3 = Amount.create (double: 1.1, unit: ETH_ETHER)
        XCTAssert (eth1 == eth2)
        XCTAssert (eth1  < eth3)
        XCTAssert (eth1 != eth3)
        XCTAssert ((eth1 + eth1)! == (eth2 + eth2))
        XCTAssert (0.0 == (eth2 - eth1)!.double (as: ETH_WEI))
        XCTAssert (0.0 == (eth2 - eth1)!.double (as: ETH_ETHER))
        XCTAssert (2.0 == (eth2 + eth1)!.double (as: ETH_ETHER))
        XCTAssertTrue  ((eth2 - eth3)!.isNegative)
        XCTAssertFalse ((eth2 - eth2)!.isNegative)

        #if false
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
        #endif
        }

    func testCurrencyPair () {
        #if false
        let BTC = Bitcoin.currency.defaultUnit!
        let USD = Fiat.US.defaultUnit!

        let pair = CurrencyPair (baseUnit: BTC, quoteUnit: USD, exchangeRate: 6000)

        // BTC -> USD
        let inUSD = pair.exchange(asBase: Amount (value: 1.0, unit: BTC))
        XCTAssertEqual(6000, inUSD?.double ?? 0, accuracy: 1e-6)

        // USD -> BTC
        let inBTC = pair.exchange(asQuote: Amount (value: 6000.0, unit: USD))
        XCTAssertEqual(1.0, inBTC?.double ?? 0, accuracy: 1e-6)

        XCTAssertEqual("\(BTC.name)/\(USD.name)=\(6000.0)", pair.description)
        #endif
    }
}
