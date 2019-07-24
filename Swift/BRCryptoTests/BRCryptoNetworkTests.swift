//
//  BRCryptoNetworkTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 3/21/19.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import XCTest
@testable import BRCrypto

class BRCryptoNetworkTests: XCTestCase {

    override func setUp() {
    }

    override func tearDown() {
    }

    func testNetworkBTC () {
        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native", issuer: nil)

        let BTC_SATOSHI = BRCrypto.Unit (currency: btc, uids: "BTC-SAT",  name: "Satoshi", symbol: "SAT")
        let BTC_BTC = BRCrypto.Unit (currency: btc, uids: "BTC-BTC",  name: "Bitcoin", symbol: "B", base: BTC_SATOSHI, decimals: 8)

        let associations = Network.Association (baseUnit: BTC_SATOSHI,
                                                defaultUnit: BTC_BTC,
                                                units: Set (arrayLiteral: BTC_SATOSHI, BTC_BTC))

        let fee = NetworkFee (timeInternalInMilliseconds: 30 * 1000,
                              pricePerCostFactor: Amount.create(integer: 1000, unit: BTC_SATOSHI))

        let network = Network (uids: "bitcoin-mainnet",
                               name: "bitcoin-name",
                               isMainnet: true,
                               currency: btc,
                               height: 100000,
                               associations: [btc:associations],
                               fees: [fee])

        XCTAssertEqual (network.uids, "bitcoin-mainnet")
        XCTAssertEqual (network.name, "bitcoin-name")
        XCTAssertTrue  (network.isMainnet)
        XCTAssertEqual (network.height, 100000)

        network.height *= 2
        XCTAssertEqual (network.height, 2 * 100000)

        XCTAssertEqual (network.currency, btc)
        XCTAssertTrue  (network.hasCurrency(btc))
        XCTAssertTrue  (network.currencyBy(code: "BTC").map { $0 == btc } ?? false)
        XCTAssertTrue  (network.currencyBy(issuer: "foo") == nil)

        XCTAssertTrue  (network.baseUnitFor    (currency: btc).map { $0 == BTC_SATOSHI} ?? false)
        XCTAssertTrue  (network.defaultUnitFor (currency: btc).map { $0 == BTC_BTC    } ?? false)

        XCTAssertTrue (network.unitsFor(currency: btc).map { $0.subtracting([BTC_SATOSHI, BTC_BTC]).isEmpty } ?? false)
        XCTAssertTrue (network.hasUnitFor(currency: btc, unit: BTC_BTC)     ?? false)
        XCTAssertTrue (network.hasUnitFor(currency: btc, unit: BTC_SATOSHI) ?? false)

        let eth = Currency (uids: "Ethereum", name: "Ethereum", code: "ETH", type: "native", issuer: nil)
        let ETH_WEI  = BRCrypto.Unit (currency: eth, uids: "ETH-WEI", name: "WEI",   symbol: "wei")

        XCTAssertFalse (network.hasCurrency(eth))
        XCTAssertNil   (network.baseUnitFor(currency: eth))
        XCTAssertNil   (network.unitsFor(currency: eth))

        XCTAssertFalse (network.hasUnitFor(currency: eth, unit: ETH_WEI) ?? false)
        XCTAssertFalse (network.hasUnitFor(currency: eth, unit: BTC_BTC) ?? false)
        XCTAssertFalse (network.hasUnitFor(currency: btc, unit: ETH_WEI) ?? false)

        XCTAssertEqual(1, network.fees.count)
    }

    func testNetworkETH () {
        let eth = Currency (uids: "Ethereum", name: "Ethereum", code: "ETH", type: "native", issuer: nil)
        let ETH_WEI  = BRCrypto.Unit (currency: eth, uids: "ETH-WEI", name: "WEI",   symbol: "wei")
        let ETH_GWEI = BRCrypto.Unit (currency: eth, uids: "ETH-GWEI", name: "GWEI",  symbol: "gwei", base: ETH_WEI, decimals: 9)
        let ETH_ETHER = BRCrypto.Unit (currency: eth, uids: "ETH-ETH", name: "ETHER", symbol: "E",    base: ETH_WEI, decimals: 18)

        let ETH_associations = Network.Association (baseUnit: ETH_WEI,
                                                    defaultUnit: ETH_ETHER,
                                                    units: Set (arrayLiteral: ETH_WEI, ETH_GWEI, ETH_ETHER))

        let brd = Currency (uids: "BRD", name: "BRD Token", code: "brd", type: "erc20", issuer: "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6")

        let brd_brdi = BRCrypto.Unit (currency: brd, uids: "BRD_Integer", name: "BRD Integer", symbol: "BRDI")
        let brd_brd  = BRCrypto.Unit (currency: brd, uids: "BRD_Decimal", name: "BRD_Decimal", symbol: "BRD", base: brd_brdi, decimals: 18)

        let BRD_associations = Network.Association (baseUnit: brd_brdi,
                                                    defaultUnit: brd_brd,
                                                    units: Set (arrayLiteral: brd_brdi, brd_brd))

        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native", issuer: nil)

        let fee = NetworkFee (timeInternalInMilliseconds: 1000,
                                pricePerCostFactor: Amount.create(double: 2.0, unit: ETH_GWEI))

        let network = Network (uids: "ethereum-mainnet",
                               name: "ethereump-name",
                               isMainnet: true,
                               currency: eth,
                               height: 100000,
                               associations: [eth:ETH_associations, brd:BRD_associations],
                               fees: [fee])

        XCTAssertTrue  (network.hasCurrency(eth))
        XCTAssertTrue  (network.hasCurrency(brd))
        XCTAssertFalse (network.hasCurrency(btc))

        XCTAssertNotNil (network.currencyBy (code: "ETH"))
        XCTAssertNotNil (network.currencyBy (code: "brd"))

        XCTAssertNotNil (network.currencyBy (issuer: "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"))
        XCTAssertNotNil (network.currencyBy (issuer: "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6".uppercased()))
        XCTAssertNil    (network.currencyBy (issuer: "foo"))

        XCTAssertTrue (network.hasUnitFor(currency: eth, unit: ETH_WEI)   ?? false)
        XCTAssertTrue (network.hasUnitFor(currency: eth, unit: ETH_GWEI)  ?? false)
        XCTAssertTrue (network.hasUnitFor(currency: eth, unit: ETH_ETHER) ?? false)
    }
}
