//
//  BRCryptoAmpuntTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 10/30/18.
//  Copyright © 2018-2019 Breadwallet AG. All rights reserved.
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
        let btc = Currency (uids: "Bitcoin", name: "Bitcoin", code: "BTC", type: "native", issuer: nil)

        XCTAssert (btc.name == "Bitcoin")
        XCTAssert (btc.code == "BTC")
        XCTAssert (btc.type == "native")

        let eth = Currency (uids: "Ethereum", name: "Ethereum", code: "ETH", type: "native", issuer: nil)
        XCTAssert (eth.name == "Ethereum")
        XCTAssert (eth.code == "ETH")
        XCTAssert (eth.type == "native")

        XCTAssertFalse (btc.name == eth.name)
        XCTAssertFalse (btc.code == eth.code)
        XCTAssertTrue  (btc.type == eth.type)

        // Not normally how a Currency is created; but used internally
        let eth_too = Currency (core: eth.core)
        XCTAssert (eth_too.name == "Ethereum")
        XCTAssert (eth_too.code == "ETH")
        XCTAssert (eth_too.type == "native")
     }

    func testUnit () {
        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native", issuer: nil)
        let eth = Currency (uids: "Ethereum", name: "Ethereum", code: "ETH", type: "native", issuer: nil)

        let BTC_SATOSHI = BRCrypto.Unit (currency: btc, code: "BTC-SAT",  name: "Satoshi", symbol: "SAT")
        let BTC_BTC     = BRCrypto.Unit (currency: btc, code: "BTC-BTC", name: "Bitcoin", symbol: "B", base: BTC_SATOSHI, decimals: 8)

        XCTAssert (BTC_SATOSHI.currency.code == btc.code)
        XCTAssert (BTC_SATOSHI.name == "Satoshi")
        XCTAssert (BTC_SATOSHI.symbol == "SAT");
        XCTAssertTrue  (BTC_SATOSHI.hasCurrency (btc))
        XCTAssertFalse (BTC_SATOSHI.hasCurrency (eth))
        XCTAssertTrue  (BTC_SATOSHI.isCompatible (with: BTC_SATOSHI))

        let test = BTC_SATOSHI.core == BTC_SATOSHI.base.core
        XCTAssertTrue  (test)

        XCTAssert (BTC_BTC.currency.code == btc.code)
        XCTAssertTrue  (BTC_BTC.isCompatible (with: BTC_SATOSHI))
        XCTAssertTrue  (BTC_SATOSHI.isCompatible (with: BTC_BTC))

        let ETH_WEI = BRCrypto.Unit (currency: eth, code: "ETH-WEI", name: "WEI", symbol: "wei")
        XCTAssertFalse (ETH_WEI.isCompatible (with: BTC_BTC))
        XCTAssertFalse (BTC_BTC.isCompatible (with: ETH_WEI))

        // Not normally how a Unit is created; but used internally
        let BTC_SATOSHI_TOO = Unit (core: BTC_SATOSHI.core)
        XCTAssert (BTC_SATOSHI_TOO.currency.code == btc.code)
        XCTAssert (BTC_SATOSHI_TOO.name == "Satoshi")
        XCTAssert (BTC_SATOSHI_TOO.symbol == "SAT");
        XCTAssertTrue  (BTC_SATOSHI_TOO.hasCurrency (btc))
        XCTAssertFalse (BTC_SATOSHI_TOO.hasCurrency (eth))
        XCTAssertTrue  (BTC_SATOSHI_TOO.isCompatible (with: BTC_SATOSHI))

    }

    func testAmount () {
        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native", issuer: nil)
        let eth = Currency (uids: "Ethereum", name: "Ethereum", code: "ETH", type: "native", issuer: nil)

        let BTC_SATOSHI = BRCrypto.Unit (currency: btc, code: "BTC-SAT",  name: "Satoshi", symbol: "SAT")
        let BTC_BTC     = BRCrypto.Unit (currency: btc, code: "BTC-BTC",  name: "Bitcoin", symbol: "B", base: BTC_SATOSHI, decimals: 8)

        let ETH_WEI   = BRCrypto.Unit (currency: eth, code: "ETH-WEI", name: "WEI",   symbol: "wei")
        let ETH_GWEI  = BRCrypto.Unit (currency: eth, code: "ETH-GWEI", name: "GWEI",  symbol: "gwei", base: ETH_WEI, decimals: 9)
        let ETH_ETHER = BRCrypto.Unit (currency: eth, code: "ETH-ETH", name: "ETHER", symbol: "E",    base: ETH_WEI, decimals: 18)

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

        if #available(iOS 13, *) {
            XCTAssertEqual ("-B 1.50", btc4.string (as: BTC_BTC))
            XCTAssertEqual ("-SAT 150,000,000", btc4.string (as: BTC_SATOSHI)!)
        }
        else {
            XCTAssertEqual ("-B1.50", btc4.string (as: BTC_BTC))
            XCTAssertEqual ("-SAT150,000,000", btc4.string (as: BTC_SATOSHI)!)
        }

        XCTAssertEqual (btc1.double(as: BTC_BTC),     btc1.convert(to: BTC_SATOSHI)?.double(as: BTC_BTC))
        XCTAssertEqual (btc1.double(as: BTC_SATOSHI), btc1.convert(to: BTC_BTC)?.double(as: BTC_SATOSHI))

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

        let a1 = Amount.create (double: +1.0, unit: ETH_WEI)
        let a2 = Amount.create (double: -1.0, unit: ETH_WEI)
        XCTAssert (+0.0 == (a1 + a2)!.double (as: ETH_WEI));
        XCTAssert (+0.0 == (a2 + a1)!.double (as: ETH_WEI));
        XCTAssert (-2.0 == (a2 + a2)!.double (as: ETH_WEI));
        XCTAssert (+2.0 == (a1 + a1)!.double (as: ETH_WEI));

        XCTAssert (+2.0 == (a1 - a2)!.double (as: ETH_WEI))
        XCTAssert (-2.0 == (a2 - a1)!.double (as: ETH_WEI))
        XCTAssert (+0.0 == (a1 - a1)!.double (as: ETH_WEI))
        XCTAssert (+0.0 == (a2 - a2)!.double (as: ETH_WEI))

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

        if #available(iOS 13, *) {
            XCTAssertEqual ("SAT 100,000,000", btc3s.string (as: BTC_SATOSHI)!)
            XCTAssertEqual ("B 1.00",          btc3s.string (as: BTC_BTC)!)
        }
        else {
            XCTAssertEqual ("SAT100,000,000", btc3s.string (as: BTC_SATOSHI)!)
            XCTAssertEqual ("B1.00",          btc3s.string (as: BTC_BTC)!)
        }

        let btc4s = Amount.create (string: "0x5f5e100", negative: true, unit: BTC_SATOSHI)!
        XCTAssert (-100000000 == btc4s.double (as: BTC_SATOSHI))
        XCTAssert (-1         == btc4s.double (as: BTC_BTC))

        if #available(iOS 13, *) {
            XCTAssertEqual ("-SAT 100,000,000", btc4s.string (as: BTC_SATOSHI)!)
            XCTAssertEqual ("-B 1.00",          btc4s.string (as: BTC_BTC)!)
        }
        else {
            XCTAssertEqual ("-SAT100,000,000", btc4s.string (as: BTC_SATOSHI)!)
            XCTAssertEqual ("-B1.00",          btc4s.string (as: BTC_BTC)!)
        }

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
        var result: String! = nil
        let eth = Currency (uids: "Ethereum", name: "Ethereum", code: "ETH", type: "native", issuer: nil)

        let ETH_WEI   = BRCrypto.Unit (currency: eth, code: "ETH-WEI", name: "WEI",   symbol: "wei")
        let ETH_GWEI  = BRCrypto.Unit (currency: eth, code: "ETH-GWEI", name: "GWEI",  symbol: "gwei", base: ETH_WEI, decimals: 9)
        let ETH_ETHER = BRCrypto.Unit (currency: eth, code: "ETH-ETH", name: "ETHER", symbol: "E",    base: ETH_WEI, decimals: 18)

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

        // Avoid a 'exact double' representation error:
        //    was 1.234567891234567891, now: 1.234567891234567936
        let a5 = Amount.create(string: "1.234567891234567936", negative: false, unit: ETH_ETHER)
        XCTAssertNotNil(a5)
        XCTAssertNotNil (a5?.double(as: ETH_WEI))
        XCTAssertEqual(1234567891234567936, a5?.double(as: ETH_WEI)!)
        XCTAssertEqual("1234567891234567936", a5?.string (base: 10, preface: ""))
        // Lost precision - last 5 digits
        if #available(iOS 13, *) { result = "wei 1,234,567,891,234,568,000" }
        else { result = "wei1,234,567,891,234,570,000" }
        XCTAssertEqual(result, a5?.string(as: ETH_WEI)!)

        XCTAssertEqual("1000000000000000000", Amount.create(string: "1", negative: false, unit: ETH_ETHER)!.string (base: 10, preface: ""))
        // String (1000000000000000000, radix:16, uppercase: true) -> DE0B6B3A7640000
        XCTAssertEqual("0xDE0B6B3A7640000".lowercased(), Amount.create(string: "1", negative: false, unit: ETH_ETHER)!.string (base: 16, preface: "0x"))

        let a6 = Amount.create(string: "123000000000000000000.0", negative: false, unit: ETH_WEI)
        XCTAssertNotNil(a6)
        if #available(iOS 13, *) { result = "wei 123,000,000,000,000,000,000" }
        else { result = "wei123,000,000,000,000,000,000" }
        XCTAssertEqual(result, a6?.string(as: ETH_WEI)!)

        let a6Double = a6?.double (as: ETH_WEI)
        XCTAssertEqual(a6Double, 1.23e20)

        let a7 = Amount.create(string: "123456789012345678.0", negative: false, unit: ETH_WEI)
        XCTAssertNotNil(a7)
        XCTAssertEqual   ("123456789012345678",         a7?.string(base: 10, preface: ""))
        // Note: a DIFFERENT VALUE between iOS 13
        if #available(iOS 13, *) { result = "wei 123,456,789,012,345,680" }
        else { result = "wei123,456,789,012,346,000" }
        XCTAssertEqual   (result, a7?.string(as: ETH_WEI)!)
//        XCTAssertNotEqual("wei123,456,789,012,345,678", a7?.string(as: ETH_WEI)!)

        let a7Double = a7?.double(as: ETH_WEI)
        XCTAssertEqual(a7Double, 1.2345678901234568e17)


    }

    func testAmountBTC () {
        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native", issuer: nil)

        let BTC_SATOSHI = BRCrypto.Unit (currency: btc, code: "BTC-SAT",  name: "Satoshi", symbol: "SAT")
        let BTC_BTC     = BRCrypto.Unit (currency: btc, code: "BTC-BTC",  name: "Bitcoin", symbol: "B", base: BTC_SATOSHI, decimals: 8)

        XCTAssertEqual (btc, BTC_SATOSHI.currency)

        let btc1 = Amount.create (integer: 100000000, unit: BTC_SATOSHI)
        XCTAssert (100000000 == btc1.double (as: BTC_SATOSHI))
        XCTAssert (1         == btc1.double (as: BTC_BTC))
        XCTAssertFalse (btc1.isNegative)
    }

    func testAmountExtended () {
        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native", issuer: nil)

        let BTC_SATOSHI = BRCrypto.Unit (currency: btc, code: "BTC-SAT",   name: "Satoshi",  symbol: "SAT")
        let BTC_MONGO   = BRCrypto.Unit (currency: btc, code: "BTC-MONGO", name: "BitMongo", symbol: "BM", base: BTC_SATOSHI, decimals: 70)

        let btc1 = Amount.create (integer: 100000000, unit: BTC_SATOSHI)
        let btc2 = Amount.create (integer: 100000001, unit: BTC_SATOSHI)
        XCTAssertFalse(btc1 > btc2)
        XCTAssertFalse(btc1 > btc1)
        XCTAssertTrue (btc2 > btc1)
        XCTAssertTrue (btc1 <= btc2)
        XCTAssertTrue (btc1 <= btc1)
        XCTAssertTrue (btc2 >= btc1)
        XCTAssertTrue (btc2 >= btc2)

        XCTAssertEqual (btc1.currency, btc)
        XCTAssertTrue  (btc1.hasCurrency(btc))

        let btc3 = Amount.create(double: 1e20, unit: BTC_SATOSHI)
        XCTAssertNotNil (btc3.double(as: BTC_MONGO))

        let btc4 = Amount (core: btc1.core, take: true)
        XCTAssertTrue (btc4.core == btc1.core)
    }

    func testCurrencyPair () {
        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native", issuer: nil)

        let BTC_SATOSHI = BRCrypto.Unit (currency: btc, code: "BTC-SAT",  name: "Satoshi", symbol: "SAT")
        let BTC_BTC     = BRCrypto.Unit (currency: btc, code: "BTC-BTC",  name: "Bitcoin", symbol: "B", base: BTC_SATOSHI, decimals: 8)

        let usd = Currency (uids: "USDollar", name: "USDollar", code: "USD", type: "fiat", issuer: nil)

        let usd_cents  = BRCrypto.Unit (currency: usd, code: "USD-Cents", name: "Cents", symbol: "c")
        let usd_dollar = BRCrypto.Unit (currency: usd, code: "USD-Dollar", name: "Dollars", symbol: "$", base: usd_cents, decimals: 2)

        let pair = CurrencyPair (baseUnit: BTC_BTC, quoteUnit: usd_dollar, exchangeRate: 10000)
        XCTAssertEqual("Bitcoin/Dollars=10000.0", pair.description)

        // BTC -> USD
        let oneBTCinUSD = pair.exchange(asBase: Amount.create(double: 1.0, unit: BTC_BTC))
        XCTAssertNotNil(oneBTCinUSD)
        XCTAssertEqual (10000.0, oneBTCinUSD!.double(as: usd_dollar)!, accuracy: 1e-6)

        // USD -> BTC
        let oneUSDinBTC = pair.exchange(asQuote: Amount.create(double: 1.0, unit: usd_dollar))
        XCTAssertNotNil (oneUSDinBTC)
        XCTAssertEqual (1/10000.0, oneUSDinBTC!.double(as: BTC_BTC)!, accuracy: 1e-6)

        let oneBTC = Amount.create(double: 1.0, unit: BTC_BTC)
        XCTAssertEqual("$10,000.00", oneBTC.string(pair: pair))
    }
}
