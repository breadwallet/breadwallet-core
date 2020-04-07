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

    func testWalletManagerMode () {
        let modes = [WalletManagerMode.api_only,
                     WalletManagerMode.api_with_p2p_submit,
                     WalletManagerMode.p2p_with_api_sync,
                     WalletManagerMode.p2p_only]

        for mode in modes {
            XCTAssertEqual (mode, WalletManagerMode (serialization: mode.serialization))
            XCTAssertEqual (mode, WalletManagerMode (core: mode.core))
        }

        for syncModeInteger in 0..<4 {
            XCTAssertNil (WalletManagerMode (serialization: UInt8 (syncModeInteger)))
        }
    }

    func testWalletManagerBTC() {
        isMainnet = false
        currencyCodesToMode = ["btc":WalletManagerMode.api_only]
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

        XCTAssertTrue  (network.supportedModes.contains(manager.mode))
        XCTAssertEqual (network.defaultAddressScheme, manager.addressScheme)

        let otherAddressScheme = network.supportedAddressSchemes.first { $0 != manager.addressScheme }!
        manager.addressScheme = otherAddressScheme
        XCTAssertEqual (otherAddressScheme, manager.addressScheme)
        manager.addressScheme = network.defaultAddressScheme
        XCTAssertEqual (network.defaultAddressScheme, manager.addressScheme)

        XCTAssertNotNil (manager.baseUnit)
        XCTAssertNotNil (manager.defaultUnit)
        XCTAssertFalse (manager.isActive)
        XCTAssertEqual (manager, manager)
        
        XCTAssertEqual("btc", manager.description)

        XCTAssertFalse (system.wallets.isEmpty)

        // Events

        XCTAssertTrue (listener.checkSystemEventsCommonlyWith (network: network,
                                                               manager: manager))

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

        XCTAssertTrue (listener.checkManagerEventsCommonlyWith (mode: manager.mode,
                                                               wallet: wallet))
    }

    func testWalletManagerETH () {
        isMainnet = false
        registerCurrencyCodes = ["brd"]
        currencyCodesToMode = ["eth":WalletManagerMode.api_only]
        prepareAccount()

        let listener = CryptoTestSystemListener (networkCurrencyCodesToMode: currencyCodesToMode,
                                                 registerCurrencyCodes: registerCurrencyCodes,
                                                 isMainnet: isMainnet)

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
        wait (for: [walletBRDExpectation], timeout: 30)

        // Events

        XCTAssertTrue (listener.checkSystemEventsCommonlyWith (network: network,
                                                               manager: manager))

        XCTAssertTrue (listener.checkManagerEvents(
            [EventMatcher (event: WalletManagerEvent.created),
             EventMatcher (event: WalletManagerEvent.walletAdded(wallet: walletETH), strict: true, scan: false),
             EventMatcher (event: WalletManagerEvent.walletAdded(wallet: walletBRD), strict: true, scan: true)
            ]))

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

        // TODO: We have an 'extra' syncStarted in here; the disconnect reason is 'unknown'?
        XCTAssertTrue (listener.checkManagerEvents (
            [EventMatcher (event: WalletManagerEvent.created),
             EventMatcher (event: WalletManagerEvent.walletAdded(wallet: walletETH)),
             EventMatcher (event: WalletManagerEvent.walletAdded(wallet: walletBRD), strict: true, scan: true),
             EventMatcher (event: WalletManagerEvent.changed(oldState: WalletManagerState.created,   newState: WalletManagerState.connected),
                           strict: true, scan: true),
             // wallet changed?
             EventMatcher (event: WalletManagerEvent.syncStarted, strict: true, scan: true),
             EventMatcher (event: WalletManagerEvent.changed(oldState: WalletManagerState.connected, newState: WalletManagerState.syncing)),
             // We might not see `syncProgress`
             // EventMatcher (event: WalletManagerEvent.syncProgress(timestamp: nil, percentComplete: 0), strict: false),

             EventMatcher (event: WalletManagerEvent.syncEnded(reason: WalletManagerSyncStoppedReason.complete), strict: false, scan: true),
             EventMatcher (event: WalletManagerEvent.changed(oldState: WalletManagerState.syncing, newState: WalletManagerState.connected)),

             // Can have another sync started here... so scan
             EventMatcher (event: WalletManagerEvent.changed(oldState: WalletManagerState.connected,
                                                             newState: WalletManagerState.disconnected (reason: WalletManagerDisconnectReason.unknown)),
                           strict: true, scan: true),
            ]))
    }

    func testWalletManagerXRP() {
        isMainnet = true
        currencyCodesToMode = ["xrp":WalletManagerMode.api_only]
        prepareAccount (identifier: "loan(C)")
        prepareSystem()

        let walletManagerDisconnectExpectation = XCTestExpectation (description: "Wallet Manager Disconnect")
        let walletManagerConnectExpectation    = XCTestExpectation (description: "Wallet Manager Connect")
        let walletManagerSyncDoneExpectation   = XCTestExpectation (description: "Wallet Manager Sync Done")
        listener.managerHandlers += [
            { (system: System, manager:WalletManager, event: WalletManagerEvent) in
                if case let .changed(_, newState) = event, case .disconnected = newState {
                    walletManagerDisconnectExpectation.fulfill()
                }
                else if case let .changed(oldState, newState) = event, case .syncing = oldState, case .connected = newState {
                    walletManagerSyncDoneExpectation.fulfill()
                }
                else if case let .changed(_, newState) = event, case .connected = newState {
                    walletManagerConnectExpectation.fulfill()
                }
            }]

        let network: Network! = system.networks.first { "xrp" == $0.currency.code && isMainnet == $0.isMainnet }
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

        XCTAssertTrue  (network.supportedModes.contains(manager.mode))
        XCTAssertEqual (network.defaultAddressScheme, manager.addressScheme)

        XCTAssertNotNil (manager.baseUnit)
        XCTAssertNotNil (manager.defaultUnit)
        XCTAssertFalse (manager.isActive)
        XCTAssertEqual (manager, manager)

        XCTAssertEqual("xrp", manager.description)

        XCTAssertFalse (system.wallets.isEmpty)

        // Events

        XCTAssertTrue (listener.checkSystemEventsCommonlyWith (network: network,
                                                               manager: manager))

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
        manager.connect()
        wait (for: [walletManagerConnectExpectation], timeout: 15)

        // Wait for sync complete...
        wait (for: [walletManagerSyncDoneExpectation], timeout: 30)

        manager.disconnect()
        wait (for: [walletManagerDisconnectExpectation], timeout: 15)

        XCTAssertTrue (listener.checkManagerEventsCommonlyWith (mode: manager.mode,
                                                               wallet: wallet))
    }

    func testWalletManagerMigrateBTC () {
        isMainnet = false
        currencyCodesToMode = ["btc":WalletManagerMode.api_only]
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

        // transfers announced on `configure`
        migrateListener.transferCount = transferBlobs.count
        migrateSystem.configure(withCurrencyModels: [])
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
        let muckedTransferBlobs = [System.TransactionBlob.btc (bytes: [UInt8](arrayLiteral: 0, 1, 2), blockHeight: UInt32(0), timestamp: UInt32(0))]
        let muckedListener = MigrateSystemListener (transactionBlobs: muckedTransferBlobs)
        let muckedQuery    = system.query
        let muckedPath     = system.path + "mucked"

        let muckedSystem = System (listener: muckedListener,
                                    account: system.account,
                                    onMainnet: system.onMainnet,
                                    path: muckedPath,
                                    query: muckedQuery)

        // transfers annonced on `configure`
        muckedSystem.configure(withCurrencyModels: [])
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
        case .created:
            break

        case .networkAdded(let network):
            // Network of interest
            if system.onMainnet == network.isMainnet
                && network.currency.code == "btc"
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
                                                    mode: network.defaultMode,
                                                    addressScheme: network.defaultAddressScheme,
                                                    currencies: Set<Currency>())
            }

        case .managerAdded(let manager):
            if nil == migratedManager && manager.network == migratedNetwork {
                migratedManager = manager
                migratedManagerExpectation.fulfill()
            }

        case .discoveredNetworks:
            break
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
        // TODO: We sometimes see a TransferEvent.created with TransferState.included - Racy?
        else if case .created = event, case .included = transfer.state {
            if 1 == transferCount { transferExpectation.fulfill()}
            if 1 <= transferCount { transferCount -= 1 }
        }
    }

    func handleNetworkEvent(system: System, network: Network, event: NetworkEvent) {
    }
}
