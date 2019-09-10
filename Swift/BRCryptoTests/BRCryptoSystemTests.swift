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
        modeMap = ["btc":WalletManagerMode.api_only]
        prepareAccount()
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

        XCTAssertTrue (listener.checkSystemEvents(
            [EventMatcher (event: SystemEvent.managerAdded(manager: manager), strict: true, scan: true)
            ]))

        XCTAssertTrue (listener.checkManagerEvents(
            [WalletManagerEvent.created,
             WalletManagerEvent.walletAdded(wallet: wallet)], strict: true))

        XCTAssertTrue (listener.checkWalletEvents(
            [WalletEvent.created], strict: true))
    }

    func testSystemAppCurrencies() {
        isMainnet = false
        currencyCodesNeeded = ["eth"]
        modeMap = ["eth":WalletManagerMode.api_only]

        currencyModels = [System.asBlockChainDBModelCurrency (uids: "ethereum-ropsten:0xffff",
                                                              name: "FOO Token",
                                                              code: "FOO",
                                                              type: "ERC20",
                                                              decimals: 10)!]

        prepareAccount()
        // Create a query that fails (no authentication)

        prepareSystem (query: BlockChainDB())

        XCTAssertTrue (system.networks.count >= 1)
        let network: Network! = system.networks.first { "eth" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        XCTAssertNotNil (network.currencyBy(code: "eth"))
        XCTAssertNotNil (network.currencyBy(code: "FOO"))

        let fooCurrency = network.currencyBy(code: "FOO")!
        XCTAssertEqual("ERC20",  fooCurrency.type)
        
        guard let fooDef = network.defaultUnitFor(currency: fooCurrency)
            else { XCTAssertTrue (false); return }
        XCTAssertEqual(10, fooDef.decimals)
        XCTAssertEqual("FOO", fooDef.symbol)

        guard let fooBase = network.baseUnitFor(currency: fooCurrency)
            else { XCTAssertTrue (false); return }
        XCTAssertEqual (0, fooBase.decimals)
        XCTAssertEqual ("FOOI", fooBase.symbol)
    }
}
