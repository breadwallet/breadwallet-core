//
//  BRCryptoBaseTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 3/28/19.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import XCTest
@testable import BRCrypto

class BRCryptoBaseTests: XCTestCase {
    var accountSpecifications: [AccountSpecification] = []
    var accountSpecification: AccountSpecification! {
        return accountSpecifications.count > 0
            ? accountSpecifications[0]
            : nil
    }

    var isMainnet = true

    let configPath = Bundle(for: BRCryptoBaseTests.self).path(forResource: "CoreTestsConfig", ofType: "json")!

    var coreDataDir: String!

    func coreDirClear () {
        do {
            if FileManager.default.fileExists(atPath: coreDataDir) {
                try FileManager.default.removeItem(atPath: coreDataDir)
            }
        }
        catch {
            print ("Error: \(error)")
            XCTAssert(false)
        }
    }

    func coreDirCreate () {
        do {
            try FileManager.default.createDirectory (atPath: coreDataDir,
                                                     withIntermediateDirectories: true,
                                                     attributes: nil)
        }
        catch {
            XCTAssert(false)
        }
    }

    var account: Account!

    func prepareAccount (_ spec: AccountSpecification? = nil) {
        let defaultSpecification = AccountSpecification (dict: [
            "identifier": "ginger",
            "paperKey":   "ginger settle marine tissue robot crane night number ramp coast roast critic",
            "timestamp":  "2018-01-01",
            "network":    (isMainnet ? "mainnet" : "testnet")
            ])

        self.accountSpecifications = (spec != nil
            ? [spec!]
            : AccountSpecification.loadFrom(configPath: configPath, defaultSpecification: defaultSpecification))
            .filter { $0.network == (isMainnet ? "mainnet" : "testnet") }

        let specifiction = accountSpecification!

        /// Create the account
        let walletId = UUID (uuidString: "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96")?.uuidString ?? "empty-wallet-id"
        account = Account.createFrom (phrase: specifiction.paperKey,
                                      timestamp: specifiction.timestamp,
                                      uids: walletId)
    }
    
    override func setUp() {
        super.setUp()

        /// Create the 'storagePath'
        coreDataDir = FileManager.default
            .urls(for: .documentDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("Core").path

        coreDirCreate()
        coreDirClear()
        XCTAssert (nil != coreDataDir)

        print ("TST: StoragePath: \(coreDataDir ?? "<none>")");
   }

    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }
}

///
/// Listeners
///

class TestWalletManagerListener: WalletManagerListener {
    let handler: WalletManagerEventHandler

    init (_ handler: @escaping WalletManagerEventHandler) {
        self.handler = handler
    }

    func handleManagerEvent(system: System, manager: WalletManager, event: WalletManagerEvent) {
        handler (system, manager, event)
    }
}

class TestWalletListener: WalletListener {
    let handler: WalletEventHandler

    init (_ handler: @escaping WalletEventHandler) {
        self.handler = handler
    }

    func handleWalletEvent(system: System, manager: WalletManager, wallet: Wallet, event: WalletEvent) {
        handler (system, manager, wallet, event)
    }
}

class TestTransferListener: TransferListener {
    let handler: TransferEventHandler

    init (_ handler: @escaping TransferEventHandler) {
        self.handler = handler
    }

    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
        handler (system, manager, wallet, transfer, event)
    }
}

class TestNetworkListener: NetworkListener {
    let handler: NetworkEventHandler

    init (_ handler: @escaping NetworkEventHandler) {
        self.handler = handler
    }
    func handleNetworkEvent(system: System, network: Network, event: NetworkEvent) {
        handler (system, network, event)
    }
}

class CryptoTestSystemListener: SystemListener {

    private let currencyCodesNeeded: [String]
    private let isMainnet: Bool

    var networkExpectation = XCTestExpectation (description: "NetworkExpectation")
    var managerExpectation = XCTestExpectation (description: "ManagerExpectation")
    var walletExpectation  = XCTestExpectation (description: "ManagerExpectation")

    var systemHandlers: [SystemEventHandler] = []

    public init (currencyCodesNeeded: [String], isMainnet: Bool) {
        self.currencyCodesNeeded = currencyCodesNeeded
        self.isMainnet = isMainnet
    }

    func handleSystemEvent(system: System, event: SystemEvent) {
        print ("TST: System Event: \(event)")
        switch event {
        case .networkAdded (let network):
            if isMainnet == network.isMainnet &&
                currencyCodesNeeded.contains (where: { nil != network.currencyBy (code: $0) }) {
                let mode = system.supportsMode (network: network, WalletManagerMode.api_only)
                    ? WalletManagerMode.api_only
                    : system.defaultMode(network: network)
                let scheme = system.defaultAddressScheme(network: network)

                let _ = system.createWalletManager (network: network,
                                                    mode: mode,
                                                    addressScheme: scheme)
            }
            networkExpectation.fulfill()

        case .managerAdded:
            managerExpectation.fulfill()

        default: break
        }
        systemHandlers.forEach { $0 (system, event) }
    }

    var managerHandlers: [WalletManagerEventHandler] = []

    func handleManagerEvent(system: System, manager: WalletManager, event: WalletManagerEvent) {
        print ("TST: Manager Event: \(event)")
        if case .walletAdded = event { walletExpectation.fulfill() }
        managerHandlers.forEach { $0 (system, manager, event) }
    }

    var walletHandlers: [WalletEventHandler] = []

    func handleWalletEvent(system: System, manager: WalletManager, wallet: Wallet, event: WalletEvent) {
        print ("TST: Wallet Event: \(event)")
        walletHandlers.forEach { $0 (system, manager, wallet, event) }
    }

    var transferHandlers: [TransferEventHandler] = []

    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
        print ("TST: Transfer Event: \(event)")
        transferHandlers.forEach { $0 (system, manager, wallet, transfer, event) }
    }

    var networkHandlers: [NetworkEventHandler] = []

    func handleNetworkEvent(system: System, network: Network, event: NetworkEvent) {
        print ("TST: Network Event: \(event)")
        networkHandlers.forEach { $0 (system, network, event) }
    }
}

class BRCryptoSystemBaseTests: BRCryptoBaseTests {

    var listener: CryptoTestSystemListener!
    var query: BlockChainDB!
    var system: System!

    var currencyCodesNeeded = ["btc"]

    func createDefaultListener() -> CryptoTestSystemListener {
        return CryptoTestSystemListener (currencyCodesNeeded: currencyCodesNeeded, isMainnet: isMainnet)
    }

    func createDefaultQuery () -> BlockChainDB {
        return BlockChainDB()
    }

    func prepareSystem (listener: CryptoTestSystemListener? = nil, query: BlockChainDB? = nil) {

        self.listener = listener ?? createDefaultListener()
        self.query    = query    ?? createDefaultQuery()

        system = System (listener:  self.listener,
                         account:   self.account,
                         onMainnet: self.isMainnet,
                         path:      self.coreDataDir,
                         query:     self.query)

        XCTAssertEqual (coreDataDir, system.path)
        XCTAssertTrue  (self.query === system.query)
        XCTAssertEqual (account.uids, system.account.uids)

        system.configure() // Don't connect
        wait (for: [self.listener.networkExpectation], timeout: 5)
        wait (for: [self.listener.managerExpectation], timeout: 5)
        wait (for: [self.listener.walletExpectation ], timeout: 5)
    }

    override func setUp() {
        super.setUp()
    }
}
