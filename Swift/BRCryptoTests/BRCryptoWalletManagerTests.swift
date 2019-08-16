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
        modeMap = ["btc":WalletManagerMode.api_only]
        prepareAccount()
        prepareSystem()

        let walletManagerDisconnectExpectation = XCTestExpectation (description: "Wallet Manager Disconnect")
        listener.managerHandlers += [
            { (system: System, manager:WalletManager, event: WalletManagerEvent) in
                if case let .changed(_, newState) = event, case .disconnected = newState {
                    walletManagerDisconnectExpectation.fulfill()
                }
            }]

        let network: Network! = system.networks.first { "btc" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        let manager: WalletManager! = system.managers.first { $0.network == network }
        let wallet = manager.primaryWallet

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

        // Events
        
        XCTAssertTrue (listener.checkSystemEvents(
            [EventMatcher (event: SystemEvent.created),
             EventMatcher (event: SystemEvent.networkAdded(network: network), strict: true, scan: true),
             EventMatcher (event: SystemEvent.managerAdded(manager: manager), strict: true, scan: true)
            ]))

        XCTAssertTrue (listener.checkManagerEvents(
            [WalletManagerEvent.created,
             WalletManagerEvent.walletAdded(wallet: wallet)],
            strict: true))

        XCTAssertTrue (listener.checkWalletEvents(
            [WalletEvent.created],
            strict: true))

        XCTAssertTrue (listener.checkTransferEvents(
            [],
            strict: true))

        // Connect
        listener.transferCount = 5
        manager.connect()
        wait (for: [self.listener.transferExpectation], timeout: 5)

        manager.disconnect()
        wait (for: [walletManagerDisconnectExpectation], timeout: 5)

        XCTAssertTrue (listener.checkManagerEvents (
            [EventMatcher (event: WalletManagerEvent.created),
             EventMatcher (event: WalletManagerEvent.walletAdded(wallet: wallet)),
             EventMatcher (event: WalletManagerEvent.changed(oldState: WalletManagerState.created,   newState: WalletManagerState.connected)),
             EventMatcher (event: WalletManagerEvent.syncStarted),
             EventMatcher (event: WalletManagerEvent.changed(oldState: WalletManagerState.connected, newState: WalletManagerState.syncing)),
             EventMatcher (event: WalletManagerEvent.syncProgress(percentComplete: 0), strict: false),

             EventMatcher (event: WalletManagerEvent.syncEnded(error: nil), strict: false, scan: true),
             EventMatcher (event: WalletManagerEvent.changed(oldState: WalletManagerState.syncing, newState: WalletManagerState.connected)),
             EventMatcher (event: WalletManagerEvent.changed(oldState: WalletManagerState.connected, newState: WalletManagerState.disconnected)),
             ]))
    }

    func testWalletManagerETH () {
        isMainnet = false
        currencyCodesNeeded = ["eth", "brd"]
        modeMap = ["eth":WalletManagerMode.api_only]
        prepareAccount()

        let listener = CryptoTestSystemListener (currencyCodesNeeded: currencyCodesNeeded,
                                                 isMainnet: isMainnet,
                                                 modeMap: modeMap)

        // Listen for a non-primary wallet - specifically the BRD wallet
        var walletBRD: Wallet! = nil
        let walletBRDExpectation = XCTestExpectation (description: "BRD Wallet")
        listener.managerHandlers += [
            { (system: System, manager:WalletManager, event: WalletManagerEvent) in
                if case let .walletAdded(wallet) = event, "brd" == wallet.name {
                    walletBRD = wallet
                    walletBRDExpectation.fulfill()
                }
            }]

        let walletManagerDisconnectExpectation = XCTestExpectation (description: "Wallet Manager Disconnect")
        listener.managerHandlers += [
            { (system: System, manager:WalletManager, event: WalletManagerEvent) in
                if case let .changed(_, newState) = event, case .disconnected = newState {
                    walletManagerDisconnectExpectation.fulfill()
                }
            }]

        prepareSystem (listener: listener)

        let network: Network! = system.networks.first { "eth" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        let manager: WalletManager! = system.managers.first { $0.network == network }
        XCTAssertNotNil (manager)

        let walletETH = manager.primaryWallet
        wait (for: [walletBRDExpectation], timeout: 10)

        // Events

        XCTAssertTrue (listener.checkSystemEvents(
            [EventMatcher (event: SystemEvent.created),
             EventMatcher (event: SystemEvent.networkAdded(network: network), strict: true, scan: true),
             EventMatcher (event: SystemEvent.managerAdded(manager: manager), strict: true, scan: true)
            ]))

        XCTAssertTrue (listener.checkManagerEvents(
            [WalletManagerEvent.created,
             WalletManagerEvent.walletAdded(wallet: walletETH),
             WalletManagerEvent.walletAdded(wallet: walletBRD)],
            strict: true))

        XCTAssertTrue (listener.checkWalletEvents(
            [WalletEvent.created,
             WalletEvent.created],
            strict: true))

        XCTAssertTrue (listener.checkTransferEvents(
            [],
            strict: true))

        // Connect
        listener.transferCount = 2
        manager.connect()
        wait (for: [self.listener.transferExpectation], timeout: 60)

        sleep (30) // allow some 'ongoing' syncs to occur; don't want to see events for these.
        manager.disconnect()
        wait (for: [walletManagerDisconnectExpectation], timeout: 5)

        // Same as BTC
        XCTAssertTrue (listener.checkManagerEvents (
            [EventMatcher (event: WalletManagerEvent.created),
             EventMatcher (event: WalletManagerEvent.walletAdded(wallet: walletETH)),
             EventMatcher (event: WalletManagerEvent.walletAdded(wallet: walletBRD)),
             EventMatcher (event: WalletManagerEvent.changed(oldState: WalletManagerState.created,   newState: WalletManagerState.connected)),
             // wallet changed?
             EventMatcher (event: WalletManagerEvent.syncStarted, strict: true, scan:true),
             EventMatcher (event: WalletManagerEvent.changed(oldState: WalletManagerState.connected, newState: WalletManagerState.syncing)),
             EventMatcher (event: WalletManagerEvent.syncProgress(percentComplete: 0), strict: false),

             EventMatcher (event: WalletManagerEvent.syncEnded(error: nil), strict: false, scan: true),
             EventMatcher (event: WalletManagerEvent.changed(oldState: WalletManagerState.syncing, newState: WalletManagerState.connected)),
             EventMatcher (event: WalletManagerEvent.changed(oldState: WalletManagerState.connected, newState: WalletManagerState.disconnected)),
            ]))
    }
}
