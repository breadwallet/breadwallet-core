//
//  BRCryptoNetworkTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 3/21/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//

import XCTest

class BRCryptoNetworkTests: XCTestCase {

    override func setUp() {
    }

    override func tearDown() {
    }

    func testNetwork () {
        #if false
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
        #endif
    }

    func testPerformanceExample() {
        self.measure {
        }
    }

}
