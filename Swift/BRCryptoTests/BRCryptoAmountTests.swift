//
//  BRCryptoAmpuntTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 10/30/18.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
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

        let test = BTC_SATOSHI.core == BTC_SATOSHI.base.core
        XCTAssertTrue  (test)

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
        XCTAssertFalse (btc1.isNegative)

        let btc1n = btc1.negate
        XCTAssert (-100000000 == btc1n.double (as: BTC_SATOSHI))
        XCTAssert (-1         == btc1n.double (as: BTC_BTC))
        XCTAssertTrue(btc1n.isNegative)
        XCTAssertFalse(btc1n.negate.isNegative)

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

        let formatter = NumberFormatter()
        formatter.minimumFractionDigits = 2
        formatter.generatesDecimalNumbers = true
        XCTAssert ("-1.50" == btc4.string (as: BTC_BTC, formatter: formatter))

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

        //
        // String
        //
        let btc1s = Amount.create (string: "100000000", unit: BTC_SATOSHI)!
        XCTAssert (100000000 == btc1s.double (as: BTC_SATOSHI))
        XCTAssert (1         == btc1s.double (as: BTC_BTC))

        let btc2s = Amount.create (string: "1",   unit: BTC_BTC)!
        XCTAssert (1         == btc2s.double (as: BTC_BTC))
        XCTAssert (100000000 == btc2s.double (as: BTC_SATOSHI))

        XCTAssert (btc1s == btc2s)

        let btc3s = Amount.create (string: "0x5f5e100", unit: BTC_SATOSHI)!
        XCTAssert (100000000 == btc3s.double (as: BTC_SATOSHI))
        XCTAssert (1         == btc3s.double (as: BTC_BTC))

        XCTAssert ("SAT100,000,000" == btc3s.string (as: BTC_SATOSHI))
        XCTAssert ("B1.00"          == btc3s.string (as: BTC_BTC))

        let btc4s = Amount.create (string: "0x5f5e100", negative: true, unit: BTC_SATOSHI)!
        XCTAssert (-100000000 == btc4s.double (as: BTC_SATOSHI))
        XCTAssert (-1         == btc4s.double (as: BTC_BTC))
        XCTAssert ("-SAT100,000,000" == btc4s.string (as: BTC_SATOSHI))
        XCTAssert ("-B1.00"          == btc4s.string (as: BTC_BTC))

        XCTAssertNil (Amount.create (string: "w0x5f5e100", unit: BTC_SATOSHI))
        XCTAssertNil (Amount.create (string: "0x5f5e100w", unit: BTC_SATOSHI))
        XCTAssertNil (Amount.create (string: "1000000000000000000000000000000000000000000000000000000000000000000000000000000000000", unit: BTC_SATOSHI))

        // Negative/Positive
        XCTAssertNil (Amount.create (string: "-1", unit: BTC_SATOSHI))
        XCTAssertNil (Amount.create (string: "+1", unit: BTC_SATOSHI))
        XCTAssertNil (Amount.create (string: "0.1", unit: BTC_SATOSHI))
        XCTAssertNil (Amount.create (string: "1.1", unit: BTC_SATOSHI))
        XCTAssertNotNil (Amount.create (string: "1.0", unit: BTC_SATOSHI))
        XCTAssertNotNil (Amount.create (string: "1.",  unit: BTC_SATOSHI))

        XCTAssertNotNil (Amount.create (string: "0.1", unit: BTC_BTC))
        XCTAssertNotNil (Amount.create (string: "1.1", unit: BTC_BTC))
        XCTAssertNotNil (Amount.create (string: "1.0", unit: BTC_BTC))
        XCTAssertNotNil (Amount.create (string: "1.",  unit: BTC_BTC))

        XCTAssert ( 10000000 == Amount.create (string: "0.1", unit: BTC_BTC)?.double(as: BTC_SATOSHI))
        XCTAssert (110000000 == Amount.create (string: "1.1", unit: BTC_BTC)?.double(as: BTC_SATOSHI))
        XCTAssert (100000000 == Amount.create (string: "1.0", unit: BTC_BTC)?.double(as: BTC_SATOSHI))
        XCTAssert (100000000 == Amount.create (string: "1.", unit: BTC_BTC)?.double(as: BTC_SATOSHI))

        XCTAssertNotNil (Amount.create (string: "0.12345678",  unit: BTC_BTC))
        XCTAssertNil    (Amount.create (string: "0.123456789", unit: BTC_BTC))

        }

    func testAmountETH () {
        let eth = Currency (uids: "Ethereum", name: "Ethereum", code: "ETH", type: "native")

        let ETH_WEI  = BRCrypto.Unit (currency: eth, uids: "ETH-WEI", name: "WEI",   symbol: "wei")
        let ETH_GWEI = BRCrypto.Unit (currency: eth, uids: "ETH-GWEI", name: "GWEI",  symbol: "gwei", base: ETH_WEI, decimals: 9)
        let ETH_ETHER = BRCrypto.Unit (currency: eth, uids: "ETH-ETH", name: "ETHER", symbol: "E",    base: ETH_WEI, decimals: 18)

        let a1 = Amount.create(string:  "12.12345678", negative: false, unit: ETH_ETHER)
        XCTAssertNotNil(a1)
        XCTAssertNotNil(a1!.double (as: ETH_ETHER))
        XCTAssertEqual(12.12345678, a1!.double (as: ETH_ETHER)!)

        let a2 = Amount.create(string: "123.12345678", negative: false, unit: ETH_ETHER)
        XCTAssertNotNil(a2)
        XCTAssertNotNil(a2!.double (as: ETH_ETHER))
        XCTAssertEqual(123.12345678, a2!.double (as: ETH_ETHER)!)

        let a3 = Amount.create(string:  "12.12345678", negative: false, unit: ETH_GWEI)
        XCTAssertNotNil(a3)
        XCTAssertNotNil(a3!.double (as: ETH_GWEI))
        XCTAssertEqual(12.12345678, a3!.double (as: ETH_GWEI)!)

        let a4 = Amount.create(string:  "123.12345678", negative: false, unit: ETH_GWEI)
        XCTAssertNotNil(a4)
        XCTAssertNotNil(a4!.double (as: ETH_GWEI))
        XCTAssertEqual(123.12345678, a4!.double (as: ETH_GWEI)!)


        let a5 = Amount.create(string: "1.234567891234567891", negative: false, unit: ETH_ETHER)
        XCTAssertNotNil(a5)
        XCTAssertNotNil (a5?.double(as: ETH_WEI))
        XCTAssertEqual(1234567891234567891, a5?.double(as: ETH_WEI)!)
        // Lost precision - last 5 digits
        XCTAssertEqual("wei1,234,567,891,234,570,000", a5?.string(as: ETH_WEI)!)

        XCTAssertEqual("1234567891234567891", a5?.string (base: 10, preface: ""))
        XCTAssertEqual("1000000000000000000", Amount.create(string: "1", negative: false, unit: ETH_ETHER)!.string (base: 10, preface: ""))
        // String (1000000000000000000, radix:16, uppercase: true) -> DE0B6B3A7640000
        XCTAssertEqual("0xDE0B6B3A7640000".lowercased(), Amount.create(string: "1", negative: false, unit: ETH_ETHER)!.string (base: 16, preface: "0x"))
    }

    func testAmountBTC () {
        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native")

        let BTC_SATOSHI = BRCrypto.Unit (currency: btc, uids: "BTC-SAT",  name: "Satoshi", symbol: "SAT")
        let BTC_BTC = BRCrypto.Unit (currency: btc, uids: "BTC-BTC",  name: "Bitcoin", symbol: "B", base: BTC_SATOSHI, decimals: 8)


        let btc1 = Amount.createAsBTC(100000000, BTC_BTC)
        XCTAssert (100000000 == btc1.double (as: BTC_SATOSHI))
        XCTAssert (1         == btc1.double (as: BTC_BTC))
        XCTAssertFalse (btc1.isNegative)


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
