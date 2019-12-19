//
//  BRCryptoSystemTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 3/25/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
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
        currencyCodesToMode = ["btc":WalletManagerMode.api_only]
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
        currencyCodesToMode = ["eth":WalletManagerMode.api_only]

        // We need the UIDS to contain a valid ETH address BUT not be a default.  Since we are
        // using `isMainnet = false` use a mainnet address.
        currencyModels = [System.asBlockChainDBModelCurrency (uids: "ethereum-ropsten" + ":" + BlockChainDB.Model.addressBRDMainnet,
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
        XCTAssertNotNil (network.currencyBy(code: "foo"))
        XCTAssertNil    (network.currencyBy(code: "FOO"))

        let fooCurrency = network.currencyBy(code: "foo")!
        XCTAssertEqual("erc20",  fooCurrency.type)

        guard let fooDef = network.defaultUnitFor(currency: fooCurrency)
            else { XCTAssertTrue (false); return }
        XCTAssertEqual(10, fooDef.decimals)
        XCTAssertEqual("foo", fooDef.symbol)

        guard let fooBase = network.baseUnitFor(currency: fooCurrency)
            else { XCTAssertTrue (false); return }
        XCTAssertEqual (0, fooBase.decimals)
        XCTAssertEqual ("fooi", fooBase.symbol)
    }

    func testSystemModes () {
        isMainnet = false
        currencyCodesToMode = ["btc":WalletManagerMode.api_only]
        prepareAccount()
        prepareSystem()

        XCTAssertTrue (system.networks.count >= 1)
        let network: Network! = system.networks.first { "btc" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)
        XCTAssertTrue (system.supportsMode(network: network, system.defaultMode(network: network)))

        system.networks
            .forEach { (network) in
                XCTAssertTrue (system.supportsMode(network: network, system.defaultMode(network: network)))
        }

        System.supportedModesMap
            .forEach { (argument) in
                let (bid, modes) = argument
                XCTAssertTrue (modes.contains (System.defaultModesMap[bid]!))
        }
    }

    func testSystemAddressSchemes () {
        isMainnet = false
        currencyCodesToMode = ["btc":WalletManagerMode.api_only]
        prepareAccount()
        prepareSystem()

        XCTAssertTrue (system.networks.count >= 1)
        let network: Network! = system.networks.first { "btc" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)
        XCTAssertTrue (system.supportsAddressScheme (network: network, system.defaultAddressScheme(network: network)))

        system.networks
            .forEach { (network) in
                XCTAssertTrue (system.supportsAddressScheme (network: network, system.defaultAddressScheme(network: network)))
        }

        System.supportedAddressSchemesMap
            .forEach { (argument) in
                let (bid, schemes) = argument
                XCTAssertTrue (schemes.contains (System.defaultAddressSchemeMap[bid]!))
        }
    }
}
