//
//  BRCoreXTests.swift
//  BRCoreXTests
//
//  Created by Ed Gamble on 10/30/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import XCTest
@testable import BRCoreX

class BRCoreXTests: XCTestCase {

    override func setUp() {
    }

    override func tearDown() {
    }

    func testCurrency() {
        let btc = Currency.bitcoin
        let eth = Currency.ethereum

        XCTAssert ("BTC" == btc.code)
        XCTAssert (btc == btc)
        XCTAssert (btc != eth)
    }

    func testUnit () {

    }

    func testAmount () {

    }

    func testCurrencyPair () {
    }

    
    func testPerformanceExample() {
        self.measure {
        }
    }

}
