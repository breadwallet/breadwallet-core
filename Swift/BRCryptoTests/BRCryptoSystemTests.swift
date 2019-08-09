//
//  BRCryptoSystemTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 3/25/19.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import XCTest
@testable import BRCrypto


class BRCryptoSystemTests: BRCryptoSystemBaseTests {
    
    override func setUp() {
        super.setUp()
    }

    override func tearDown() {
    }

    func testSystemBTC() {
        isMainnet = false
        currencyCodesNeeded = ["btc"]
        prepareSystem()

        XCTAssertTrue (system.networks.count >= 1)
        let network: Network! = system.networks.first { "btc" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        XCTAssertEqual (1, system.managers.count)
        let manager = system.managers[0]

        XCTAssertTrue (system     === manager.system)
//      XCTAssertTrue (account === manager.account)
        XCTAssertTrue (network  == manager.network)
        XCTAssertTrue (query   === manager.query)

        XCTAssertTrue (manager === system.managerBy(core: manager.core))

        let wallet = manager.primaryWallet
        XCTAssertNotNil (wallet)
        XCTAssertTrue (system  === wallet.system)
        XCTAssertTrue (manager === wallet.manager)
        XCTAssertTrue (wallet.transfers.isEmpty)

        XCTAssertEqual (network.currency, wallet.currency)
        XCTAssertEqual (manager.currency, wallet.currency)
        XCTAssertTrue  (network.defaultUnitFor(currency: network.currency).map { $0 == wallet.unit } ?? false)
        XCTAssertEqual (wallet.balance, Amount.create(integer: 0, unit: manager.baseUnit))
        XCTAssertEqual (wallet.state, WalletState.created)

        #if false
        let transfer = wallet.createTransfer (target: targetAddress,
                                              amount: Amount.create (integer: 1, unit: manager.baseUnit))

        XCTAssertNil(transfer) // no balance => no transfer
        #endif

        #if false
        XCTAssertTrue (transfer.wallet === wallet)
        XCTAssertTrue (transfer.manager === manager)
        XCTAssertTrue (transfer.system  === sys)

        XCTAssertEqual (transfer.target, targetAddress)
        XCTAssertEqual (transfer.source, wallet.source)
        #endif
    }
}
