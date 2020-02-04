//
//  BRCryptoNetworkTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 3/21/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
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
        let btc = Currency (uids: "bitcoin-mainnet:__native__",  name: "Bitcoin",  code: "btc", type: "native", issuer: nil)

        let BTC_SATOSHI = BRCrypto.Unit (currency: btc, code: "sat",  name: "Satoshi", symbol: "SAT")
        let BTC_BTC     = BRCrypto.Unit (currency: btc, code: "btc",  name: "Bitcoin", symbol: "B", base: BTC_SATOSHI, decimals: 8)

        let fee = NetworkFee (timeIntervalInMilliseconds: 30 * 1000,
                              pricePerCostFactor: Amount.create(integer: 1000, unit: BTC_SATOSHI))

        let _ = NetworkFee (core: fee.core, take: true)

        let network = Network.findBuiltin(uids: "bitcoin-mainnet")!

        XCTAssertEqual (network.uids, "bitcoin-mainnet")
        XCTAssertEqual (network.name, "Bitcoin")
        XCTAssertTrue  (network.isMainnet)

        let height = network.height;
        network.height *= 2
        XCTAssertEqual (network.height, 2 * height)

        XCTAssertEqual (network.currency, btc)
        XCTAssertTrue  (network.hasCurrency(btc))
        XCTAssertTrue  (network.currencyBy(code: "btc").map { $0 == btc } ?? false)
        XCTAssertTrue  (network.currencyBy(issuer: "foo") == nil)

        XCTAssertTrue  (network.baseUnitFor    (currency: btc).map { $0 == BTC_SATOSHI} ?? false)
        XCTAssertTrue  (network.defaultUnitFor (currency: btc).map { $0 == BTC_BTC    } ?? false)

        XCTAssertTrue (network.unitsFor(currency: btc).map { $0.subtracting([BTC_SATOSHI, BTC_BTC]).isEmpty } ?? false)
        XCTAssertTrue (network.hasUnitFor(currency: btc, unit: BTC_BTC)     ?? false)
        XCTAssertTrue (network.hasUnitFor(currency: btc, unit: BTC_SATOSHI) ?? false)

        let eth = Currency (uids: "ethereum-mainnet:__native__", name: "Ethereum", code: "ETH", type: "native", issuer: nil)
        let ETH_WEI  = BRCrypto.Unit (currency: eth, code: "ETH-WEI", name: "WEI",   symbol: "wei")

        XCTAssertFalse (network.hasCurrency(eth))
        XCTAssertNil   (network.baseUnitFor(currency: eth))
        XCTAssertNil   (network.unitsFor(currency: eth))

        XCTAssertFalse (network.hasUnitFor(currency: eth, unit: ETH_WEI) ?? false)
        XCTAssertFalse (network.hasUnitFor(currency: eth, unit: BTC_BTC) ?? false)
        XCTAssertFalse (network.hasUnitFor(currency: btc, unit: ETH_WEI) ?? false)

        XCTAssertEqual(1, network.fees.count)

        // Hashable
        let networksTable: [Network: Int] = [network : 1]
        XCTAssertEqual(1, networksTable[network])
    }

    func testNetworkETH () {
        let eth = Currency (uids: "ethereum-mainnet:__native__", name: "Ethereum", code: "eth", type: "native", issuer: nil)
        let ETH_WEI   = BRCrypto.Unit (currency: eth, code: "wei", name: "WEI",   symbol: "wei")
        let ETH_GWEI  = BRCrypto.Unit (currency: eth, code: "gwei", name: "GWEI",  symbol: "gwei", base: ETH_WEI, decimals: 9)
        let ETH_ETHER = BRCrypto.Unit (currency: eth, code: "eth", name: "ETHER", symbol: "E",    base: ETH_WEI, decimals: 18)

        let brd = Currency (uids: "ethereum-mainnet:0x558ec3152e2eb2174905cd19aea4e34a23de9ad6", name: "BRD Token", code: "brd", type: "erc20", issuer: "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6")
        // let brd_brdi = BRCrypto.Unit (currency: brd, uids: "BRDI", name: "BRD Token INT", symbol: "BRDI")
        // let brd_brd  = BRCrypto.Unit (currency: brd, uids: "BRD",  name: "BRD Token",     symbol: "BRD", base: brd_brdi, decimals: 18)

        let btc = Currency (uids: "bitcoin-mainnet:__native__",  name: "Bitcoin",  code: "BTC", type: "native", issuer: nil)

        let network = Network.findBuiltin(uids: "ethereum-mainnet")!

        let fee1 = NetworkFee (timeIntervalInMilliseconds: 1 * 60 * 1000,
                               pricePerCostFactor: Amount.create(integer: 25000000000, unit: network.baseUnitFor(currency: network.currency)!))

        XCTAssertEqual ("Ethereum", network.description)
        XCTAssertTrue  (network.hasCurrency(eth))
        XCTAssertTrue  (network.hasCurrency(brd))
        XCTAssertFalse (network.hasCurrency(btc))

        XCTAssertNotNil (network.currencyBy (code: "eth"))
        XCTAssertNotNil (network.currencyBy (code: "brd"))

        XCTAssertNotNil (network.currencyBy (issuer: "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"))
        XCTAssertNotNil (network.currencyBy (issuer: "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6".uppercased()))
        XCTAssertNil    (network.currencyBy (issuer: "foo"))

        XCTAssertTrue (network.hasUnitFor(currency: eth, unit: ETH_WEI)   ?? false)
        XCTAssertTrue (network.hasUnitFor(currency: eth, unit: ETH_GWEI)  ?? false)
        XCTAssertTrue (network.hasUnitFor(currency: eth, unit: ETH_ETHER) ?? false)

        XCTAssertEqual(fee1, network.minimumFee)

        XCTAssertNil (network.defaultUnitFor(currency: btc))
        XCTAssertNil (network.baseUnitFor(currency: btc))

        let _ = Network (core: network.core, take: true)
    }
}
