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

    func testWalletManagerMigrateBTC () {
        isMainnet = false
        currencyCodesNeeded = ["btc"]
        modeMap = ["btc":WalletManagerMode.api_only]
        prepareAccount (AccountSpecification (dict: [
            "identifier": "ginger",
            "paperKey":   "ginger settle marine tissue robot crane night number ramp coast roast critic",
            "timestamp":  "2018-01-01",
            "network":    (isMainnet ? "mainnet" : "testnet")
            ]))
        prepareSystem ()

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
        XCTAssertNotNil (manager)

        let wallet = manager.primaryWallet
        XCTAssertNotNil(wallet)

        // Connect
        listener.transferCount = 25
        manager.connect()
        wait (for: [self.listener.transferExpectation], timeout: 120)

        sleep (10) // allow some 'ongoing' syncs to occur; don't want to see events for these.
        manager.disconnect()
        wait (for: [walletManagerDisconnectExpectation], timeout: 5)

        let transfers = wallet.transfers
        XCTAssertTrue (transfers.count >= 25)
        XCTAssertTrue (transfers.allSatisfy { nil != $0.hash })

        // Get the blobs (for testing)
        let transferBlobs = transfers.map { system.asBlob(transfer: $0)! }

        // Create a new system with MigrateSystemListener (see below).  This listener will
        // create a BTC wallet manager with transfers migrated from `TransferBlobs`
        let migrateListener = MigrateSystemListener (transactionBlobs: transferBlobs)
        let migrateQuery    = system.query
        let migratePath     = system.path + "Migrate"

        let migrateSystem = System (listener: migrateListener,
                                    account: system.account,
                                    onMainnet: system.onMainnet,
                                    path: migratePath,
                                    query: migrateQuery)

        // transfers annonced on `configure`
        migrateListener.transferCount = transferBlobs.count
        migrateSystem.configure()
        wait (for: [migrateListener.migratedManagerExpectation], timeout: 30)
        wait (for: [migrateListener.transferExpectation], timeout: 30)
        XCTAssertFalse (migrateListener.migratedFailed)

        // Get the transfers from the migratedManager's primary wallet.
        let migratedTransfers = migrateListener.migratedManager.primaryWallet.transfers

        // Compare the count; then compare the hash sets as equal.
        XCTAssertEqual (transfers.count, migratedTransfers.count)
        XCTAssertEqual (Set (transfers.map { $0.hash! }), Set (migratedTransfers.map { $0.hash! }))

        //
        // Produce an invalid transferBlobs and  check for a failure
        //
        let muckedTransferBlobs = [(bytes: [UInt8](arrayLiteral: 0, 1, 2), blockHeight: UInt32(0), timestamp: UInt32(0))]
        let muckedListener = MigrateSystemListener (transactionBlobs: muckedTransferBlobs)
        let muckedQuery    = system.query
        let muckedPath     = system.path + "mucked"

        let muckedSystem = System (listener: muckedListener,
                                    account: system.account,
                                    onMainnet: system.onMainnet,
                                    path: muckedPath,
                                    query: muckedQuery)

        // transfers annonced on `configure`
        muckedSystem.configure()
        wait (for: [muckedListener.migratedManagerExpectation], timeout: 30)
        XCTAssertTrue (muckedListener.migratedFailed)
    }
}

class MigrateSystemListener: SystemListener {
    let transactionBlobs: [System.TransactionBlob]

    var migratedNetwork: Network! = nil
    var migratedManager: WalletManager! = nil

    var migratedManagerExpectation = XCTestExpectation (description: "Migrated Manager")
    init (transactionBlobs: [System.TransactionBlob]) {
        self.transactionBlobs = transactionBlobs
    }

    var migratedFailed = false

    func handleSystemEvent(system: System, event: SystemEvent) {
        switch event {
        case .created: break
        case .networkAdded(let network):
            // Network of interest
            if system.onMainnet == network.isMainnet
                && network.currency.code == Currency.codeAsBTC
                && nil == migratedNetwork {

                migratedNetwork = network

                // Migrate
                if (system.migrateRequired(network: network)) {
                    do {
                        try system.migrateStorage (network: network,
                                                   transactionBlobs: transactionBlobs,
                                                   blockBlobs: [],
                                                   peerBlobs: [])
                    }
                    catch { migratedFailed = true }
                }

                // Wallet Manager
                let _ = system.createWalletManager (network: network,
                                                    mode: system.defaultMode(network: network),
                                                    addressScheme: system.defaultAddressScheme(network: network))
            }

        case .managerAdded(let manager):
            if nil == migratedManager && manager.network == migratedNetwork {
                migratedManager = manager
                migratedManagerExpectation.fulfill()
            }
        }
    }

    func handleManagerEvent(system: System, manager: WalletManager, event: WalletManagerEvent) {
    }

    func handleWalletEvent(system: System, manager: WalletManager, wallet: Wallet, event: WalletEvent) {
    }

    var transferIncluded: Bool = false
    var transferCount: Int = 0;
//    var transferHandlers: [TransferEventHandler] = []
//    var transferEvents: [TransferEvent] = []
    var transferExpectation = XCTestExpectation (description: "TransferExpectation")

    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
        if transferIncluded, case .included = transfer.state {
            if 1 == transferCount { transferExpectation.fulfill()}
            if 1 <= transferCount { transferCount -= 1 }
        }
        else if case .created = transfer.state {
            if 1 == transferCount { transferExpectation.fulfill()}
            if 1 <= transferCount { transferCount -= 1 }
        }
    }

    func handleNetworkEvent(system: System, network: Network, event: NetworkEvent) {
    }
}
