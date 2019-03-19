//
//  BRCoreXTests.swift
//  BRCoreXTests
//
//  Created by Ed Gamble on 10/30/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import XCTest
@testable import BRCrypto

class BRCryptoBaseTests: XCTestCase {

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
        XCTAssertFalse(Ethereum.currency.isCompatible(withUnit: Bitcoin.Units.satoshi))
        XCTAssertFalse(Bitcoin.currency.isCompatible(withUnit: Ethereum.Units.ether))
        XCTAssertFalse(Bitcoin.currency.isCompatible(withUnit: Bitcash.Units.bitcash))
    }

    func testAmount () {
        let ETHER = Ethereum.Units.ether
        let WEI   = Ethereum.Units.wei
        let GWEI  = Ethereum.Units.gwei
        let eth = Ethereum.currency

        XCTAssert (nil != Amount (currency: eth, exactly: 1.5, unit: ETHER))
        XCTAssert (nil != Amount (currency: eth, exactly: 1.0, unit: ETHER))
        XCTAssert (nil == Amount (currency: eth, exactly: 1.5, unit: WEI))

        XCTAssert (1.5 == Amount (currency: eth, exactly: 1.5, unit: ETHER)?.double)
        XCTAssert (1.5 * 1e9 == Amount (currency: eth, exactly: 1.5, unit: ETHER)?.double(as: GWEI))

        XCTAssert (+10 == Amount(currency: eth, value: +10, unit: ETHER).double)
        XCTAssert (-10 == Amount(currency: eth, value: -10, unit: ETHER).double)

        XCTAssert (-10 == Amount(currency: eth, value: -10.0, unit: ETHER).double)

        XCTAssertEqual(25.0, Amount(currency: eth, value: 10.0, unit: ETHER).scale(by: 2.5)?.double ?? 0.0, accuracy: 1e-6)
        XCTAssertEqual( 2.0, Amount(currency: eth, value: 10.0, unit: ETHER).scale(by: 1/5)?.double ?? 0.0, accuracy: 1e-6)

        XCTAssert(Amount (currency: eth, value: 1, unit: ETHER) >  Amount (currency: eth, value: 1, unit: GWEI))
        XCTAssert(Amount (currency: eth, value: 1, unit: ETHER) == Amount (currency: eth, value: 1, unit: ETHER))
        XCTAssert(Amount (currency: eth, value: 1, unit: ETHER) != Amount (currency: eth, value: 2, unit: ETHER))
        XCTAssert(Amount (currency: eth, value: 1, unit: ETHER) == Amount (currency: eth, value: 1e9, unit: GWEI))
        XCTAssert(Amount (currency: eth, value: 1e-3, unit: ETHER) == Amount (currency: eth, value: 1e6, unit: GWEI))

        XCTAssert(Amount (currency: eth, value: 2.5, unit: ETHER) == Amount (currency: eth, value: 1.5, unit: ETHER) + Amount (currency: eth, value: 1.0, unit: ETHER))
        XCTAssert(Amount (currency: eth, value: 0.5, unit: ETHER) == Amount (currency: eth, value: 1.5, unit: ETHER) - Amount (currency: eth, value: 1.0, unit: ETHER))

        let a1 = Amount (currency: eth, value: 1, unit: ETHER)
        let a2 = Amount (currency: Bitcoin.currency, value: 1, unit: Bitcoin.Units.satoshi)
        XCTAssertTrue  (a1.isCompatible(with: a1))
        XCTAssertFalse (a1.isCompatible(with: a2))

        XCTAssertEqual(a1.description, "1.0 \(Ethereum.currency.symbol)")

        XCTAssertEqual    ("9.123", Amount (currency: eth, value: 9.12345, unit: ETHER).describe(as: ETHER, decimals: 3, withSymbol: false))
        XCTAssertNotEqual ("9.123", Amount (currency: eth, value: 9.12345, unit: ETHER).describe(as: ETHER, decimals: 4, withSymbol: false))
        XCTAssertEqual    ("9.123 \(ETHER.symbol)", Amount (currency: eth, value: 9.12345, unit: ETHER).describe(as: ETHER, decimals: 3, withSymbol: true))
    }

    func testCurrencyPair () {
        let BTC = Bitcoin.currency.defaultUnit
        let USD = Fiat.US.defaultUnit

        let pair = CurrencyPair (baseCurrency: Bitcoin.currency, quoteCurrency: Fiat.US, exchangeRate: 6000)

        // BTC -> USD
        let inUSD = pair.exchange(asBase: Amount (currency: Bitcoin.currency, value: 1.0, unit: BTC))
        XCTAssertEqual(6000, inUSD?.double ?? 0, accuracy: 1e-6)

        // USD -> BTC
        let inBTC = pair.exchange(asQuote: Amount (currency: Fiat.US, value: 6000.0, unit: USD))
        XCTAssertEqual(1.0, inBTC?.double ?? 0, accuracy: 1e-6)

        XCTAssertEqual("\(BTC.name)/\(USD.name)=\(6000.0)", pair.description)
    }

    func testAccount () {
        let _ = Account (phrase: "ginger settle marine tissue robot crane night number ramp coast roast critic")
        let s1 = Account.deriveSeed(phrase: "ginger settle marine tissue robot crane night number ramp coast roast critic")
        let _ = Account (seed: s1)
    }

    func testNetwork () {
        // ==
        XCTAssertEqual (Bitcoin.Networks.mainnet, Bitcoin.Networks.mainnet)
        XCTAssertEqual (Ethereum.Networks.rinkeby, Ethereum.Networks.rinkeby)
        XCTAssertNotEqual(Bitcoin.Networks.mainnet, Bitcoin.Networks.testnet)
        XCTAssertNotEqual(Bitcoin.Networks.mainnet, Ethereum.Networks.mainnet)
        XCTAssertNotEqual(Bitcoin.Networks.mainnet, Ethereum.Networks.ropsten)
        XCTAssertNotEqual (Ethereum.Networks.rinkeby, Ethereum.Networks.ropsten)

        // name
        XCTAssertEqual("BTC Mainnet", Bitcoin.Networks.mainnet.name)
        XCTAssertEqual("BCH Mainnet", Bitcash.Networks.mainnet.name)
        XCTAssertEqual("ETH Mainnet", Ethereum.Networks.mainnet.name)

        // description
        XCTAssertEqual(Bitcoin.Networks.mainnet.description, Bitcoin.Networks.mainnet.name)

        // currency
        XCTAssertEqual(Bitcoin.currency, Bitcoin.Networks.mainnet.currency)
        XCTAssertEqual(Bitcoin.currency, Bitcoin.Networks.testnet.currency)
        XCTAssertEqual(Bitcash.currency, Bitcash.Networks.mainnet.currency)
        XCTAssertEqual(Ethereum.currency, Ethereum.Networks.mainnet.currency)
        XCTAssertEqual(Ethereum.currency, Ethereum.Networks.foundation.currency)


        // hashable
        let networks = Set (arrayLiteral: Bitcoin.Networks.mainnet,
                                            Bitcash.Networks.mainnet,
                                           Ethereum.Networks.mainnet)
        XCTAssertTrue(networks.contains(Bitcash.Networks.mainnet))
        XCTAssertFalse(networks.contains(Bitcoin.Networks.testnet))
    }

    func testAddress () {
        let r1 = Address (raw: "foo")
        let r2 = Address (raw: "bar")
        let r3 = Address (raw: "foo")

        XCTAssertEqual (r1, r3)
        XCTAssertNotEqual (r1, r2)

        let e1 = Address (ethereum: "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62")
        let e2 = Address (ethereum: "0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f")

        XCTAssertEqual(e1, e1)
        XCTAssertNotEqual (e1, e2)
        XCTAssertNotEqual (r1, e1)

        // hashable
        XCTAssertEqual (r1.hashValue, r1.hashValue);
        XCTAssertEqual (e1.hashValue, e1.hashValue);

        XCTAssertNotEqual (e1.hashValue, e2.hashValue);

        XCTAssertEqual("foo", r1.description)
        XCTAssertEqual("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", e1.description)
    }
//    func testPerformanceExample() {
//        self.measure {
//        }
//    }

}
