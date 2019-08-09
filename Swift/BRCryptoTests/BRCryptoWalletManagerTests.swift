//
//  BRCryptoWalletManagerTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 1/11/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import XCTest
@testable import BRCrypto

class BRCryptoWalletManagerTests: BRCryptoSystemBaseTests {
    
    override func setUp() {
        super.setUp()
    }

    override func tearDown() {
    }

    func testWalletManagerBTC() {
        isMainnet = false
        currencyCodesNeeded = ["btc"]
        prepareAccount()
        prepareSystem()

        let network: Network! = system.networks.first { "btc" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        let manager: WalletManager! = system.managers.first { $0.network == network }
        XCTAssertNotNil (manager)
        XCTAssertTrue  (system  === manager.system)
        XCTAssertTrue  (self.query === manager.query)
        XCTAssertEqual (network, manager.network)

        XCTAssertEqual (WalletManagerState.created, manager.state)
        XCTAssertTrue  (manager.height > 0)
        XCTAssertEqual (manager.primaryWallet.manager, manager)
        XCTAssertEqual (1, manager.wallets.count)
        XCTAssertTrue  (manager.wallets.contains(manager.primaryWallet))
        XCTAssertTrue  (network.fees.contains(manager.defaultNetworkFee))

        XCTAssertTrue  (system.supportedModes(network: network).contains(manager.mode))
        XCTAssertEqual (system.defaultAddressScheme(network: network), manager.addressScheme)

        let otherAddressScheme = system.supportedAddressSchemes(network: network).first { $0 != manager.addressScheme }!
        manager.addressScheme = otherAddressScheme
        XCTAssertEqual (otherAddressScheme, manager.addressScheme)
        manager.addressScheme = system.defaultAddressScheme(network: network)
        XCTAssertEqual (system.defaultAddressScheme(network: network), manager.addressScheme)

        XCTAssertNotNil (manager.baseUnit)
        XCTAssertNotNil (manager.defaultUnit)
        XCTAssertFalse (manager.isActive)
        XCTAssertEqual (manager, manager)
        
        XCTAssertEqual("btc", manager.description)

        XCTAssertFalse (system.wallets.isEmpty)
    }

    func testWalletManagerETH () {
        isMainnet = false
        currencyCodesNeeded = ["eth"]
        prepareAccount()
        prepareSystem()

        let network: Network! = system.networks.first { "eth" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        let manager: WalletManager! = system.managers.first { $0.network == network }
        XCTAssertNotNil (manager)
    }
}
