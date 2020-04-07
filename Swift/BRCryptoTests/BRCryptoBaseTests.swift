//
//  BRCryptoBaseTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 3/28/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
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

    func prepareAccount (_ spec: AccountSpecification? = nil, identifier: String? = nil) {
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

        // If there is an identifier, filter
        if let id = identifier {
            self.accountSpecifications.removeAll { $0.identifier != id }
        }
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

    private let isMainnet: Bool
    private let networkCurrencyCodesToMode: [String:WalletManagerMode]
    private let registerCurrencyCodes: [String]

    public init (networkCurrencyCodesToMode: [String:WalletManagerMode],
                 registerCurrencyCodes: [String],
                 isMainnet: Bool) {
        self.networkCurrencyCodesToMode = networkCurrencyCodesToMode
        self.registerCurrencyCodes = registerCurrencyCodes;
        self.isMainnet = isMainnet
    }

    var systemHandlers: [SystemEventHandler] = []
    var systemEvents: [SystemEvent] = []

    func handleSystemEvent(system: System, event: SystemEvent) {
        print ("TST: System Event: \(event)")
        systemEvents.append (event)
        switch event {
        case .networkAdded (let network):
            if isMainnet == network.isMainnet,
                 network.currencies.contains(where: { nil != networkCurrencyCodesToMode[$0.code] }),
                 let currencyMode = self.networkCurrencyCodesToMode [network.currency.code] {
                 // Get a valid mode, ideally from `currencyMode`

                 let mode = network.supportsMode (currencyMode)
                     ? currencyMode
                     : network.defaultMode

                 let scheme = network.defaultAddressScheme

                 let currencies = network.currencies
                     .filter { (c) in registerCurrencyCodes.contains { c.code == $0 } }

                 let success = system.createWalletManager (network: network,
                                                           mode: mode,
                                                           addressScheme: scheme,
                                                           currencies: currencies)
                XCTAssertTrue(success)

//            if isMainnet == network.isMainnet &&
//                currencyCodesNeeded.contains (where: { nil != network.currencyBy (code: $0) }) {
//                let mode = modeMap[network.currency.code] ?? system.defaultMode(network: network)
//                XCTAssertTrue (system.supportsMode(network: network, mode))
//
//                let scheme = system.defaultAddressScheme(network: network)
//                let _ = system.createWalletManager (network: network,
//                                                    mode: mode,
//                                                    addressScheme: scheme,
//                                                    currencies: Set<Currency>())
            }
            networkExpectation.fulfill()

        case .managerAdded:
            managerExpectation.fulfill()

        default: break
        }

        systemHandlers.forEach { $0 (system, event) }
    }

    func checkSystemEvents (_ expected: [SystemEvent], strict: Bool = false) -> Bool {
        return checkSystemEvents (expected.map { EventMatcher (event: $0, strict: strict, scan: false) })
    }

    func checkSystemEvents (_ matchers: [EventMatcher<SystemEvent>]) -> Bool {
        return systemEvents.match(matchers)
    }

    // MARK: - Wallet Manager Handler

    var managerHandlers: [WalletManagerEventHandler] = []
    var managerEvents: [WalletManagerEvent] = []
    var managerExpectation = XCTestExpectation (description: "ManagerExpectation")


    func handleManagerEvent(system: System, manager: WalletManager, event: WalletManagerEvent) {
        print ("TST: Manager Event: \(event)")
        managerEvents.append(event)
        if case .walletAdded = event { walletExpectation.fulfill() }
        managerHandlers.forEach { $0 (system, manager, event) }
    }

    func checkManagerEvents (_ expected: [WalletManagerEvent], strict: Bool = false) -> Bool {
        return checkManagerEvents (expected.map { EventMatcher (event: $0, strict: strict, scan: false) })
    }

    func checkManagerEvents (_ matchers: [EventMatcher<WalletManagerEvent>]) -> Bool {
        return managerEvents.match(matchers)
    }

    // MARK: - Wallet Handler

    var walletHandlers: [WalletEventHandler] = []
    var walletEvents: [WalletEvent] = []
    var walletExpectation  = XCTestExpectation (description: "ManagerExpectation")

    func handleWalletEvent(system: System, manager: WalletManager, wallet: Wallet, event: WalletEvent) {
        print ("TST: Wallet (\(wallet.name)) Event: \(event)")
        walletEvents.append(event)
        walletHandlers.forEach { $0 (system, manager, wallet, event) }
    }

    func checkWalletEvents (_ expected: [WalletEvent], strict: Bool = false) -> Bool {
        return checkWalletEvents (expected.map { EventMatcher (event: $0, strict: strict, scan: false) })
    }

    func checkWalletEvents (_ matchers: [EventMatcher<WalletEvent>]) -> Bool {
        return walletEvents.match(matchers)
    }

    // MARK: - Transfer Handler

    var transferIncluded: Bool = false
    var transferCount: Int = 0;
    var transferHandlers: [TransferEventHandler] = []
    var transferEvents: [TransferEvent] = []
    var transferExpectation = XCTestExpectation (description: "TransferExpectation")

    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
        print ("TST: Transfer Event: \(event)")
        transferEvents.append (event)
        if transferIncluded, case .included = transfer.state {
            if 1 == transferCount { transferExpectation.fulfill()}
            if 1 <= transferCount { transferCount -= 1 }
        }
        else if case .created = transfer.state {
            if 1 == transferCount { transferExpectation.fulfill()}
            if 1 <= transferCount { transferCount -= 1 }
        }
        transferHandlers.forEach { $0 (system, manager, wallet, transfer, event) }
    }

   func checkTransferEvents (_ expected: [TransferEvent], strict: Bool = false) -> Bool {
        return checkTransferEvents (expected.map { EventMatcher (event: $0, strict: strict, scan: false) })
    }

    func checkTransferEvents (_ matchers: [EventMatcher<TransferEvent>]) -> Bool {
        return transferEvents.match(matchers)
    }

    // MARK: - Network Handler

    var networkHandlers: [NetworkEventHandler] = []
    var networkEvents: [NetworkEvent] = []
    var networkExpectation = XCTestExpectation (description: "NetworkExpectation")

    func handleNetworkEvent(system: System, network: Network, event: NetworkEvent) {
        print ("TST: Network Event: \(event)")
        networkEvents.append (event)
        networkHandlers.forEach { $0 (system, network, event) }
    }

    func checkNetworkEvents (_ expected: [NetworkEvent], strict: Bool = false) -> Bool {
        return checkNetworkEvents (expected.map { EventMatcher (event: $0, strict: strict, scan: false) })
    }

    func checkNetworkEvents (_ matchers: [EventMatcher<NetworkEvent>]) -> Bool {
        return networkEvents.match(matchers)
    }

    //
    // Common Sequences
    //

    func checkSystemEventsCommonlyWith (network: Network, manager: WalletManager) -> Bool {
        return checkSystemEvents(
            [EventMatcher (event: SystemEvent.created),
             EventMatcher (event: SystemEvent.networkAdded(network: network), strict: true, scan: true),
             EventMatcher (event: SystemEvent.managerAdded(manager: manager), strict: true, scan: true)
        ])
    }

    func checkManagerEventsCommonlyWith (mode: WalletManagerMode,
                                         wallet: Wallet,
                                         lenientDisconnect: Bool = false) -> Bool {
        let disconnectReason = (mode == WalletManagerMode.api_only
            ? WalletManagerDisconnectReason.requested
            : (mode == WalletManagerMode.p2p_only
                ? WalletManagerDisconnectReason.posix (errno: 54, message: "Connection reset by peer")
                : WalletManagerDisconnectReason.unknown))
        
        let disconnectEventMatcher =
            EventMatcher (event: WalletManagerEvent.changed (oldState: WalletManagerState.connected,
                                                             newState: WalletManagerState.disconnected (reason: disconnectReason)),
                          strict: !lenientDisconnect,
                          scan: true)       // block height updates might intervene.

        return checkManagerEvents(
            [EventMatcher (event: WalletManagerEvent.created),
             EventMatcher (event: WalletManagerEvent.walletAdded (wallet: wallet)),
             EventMatcher (event: WalletManagerEvent.changed (oldState: WalletManagerState.created,
                                                              newState: WalletManagerState.connected)),
             EventMatcher (event: WalletManagerEvent.syncStarted),
             EventMatcher (event: WalletManagerEvent.changed (oldState: WalletManagerState.connected,
                                                              newState: WalletManagerState.syncing)),

             // On API_ONLY here is no .syncProgress: timestamp: nil, percentComplete: 0
                EventMatcher (event: WalletManagerEvent.walletChanged(wallet: wallet), strict: true, scan: true),

                EventMatcher (event: WalletManagerEvent.syncEnded (reason: WalletManagerSyncStoppedReason.complete), strict: false, scan: true),
                EventMatcher (event: WalletManagerEvent.changed (oldState: WalletManagerState.syncing,
                                                                 newState: WalletManagerState.connected)),
                disconnectEventMatcher
        ])
    }
    
    func checkWalletEventsCommonlyWith (mode: WalletManagerMode, balance: Amount, transfer: Transfer) -> Bool {
        switch mode {
        case .p2p_only:
            return checkWalletEvents(
                [EventMatcher (event: WalletEvent.created),
                 EventMatcher (event: WalletEvent.transferAdded(transfer: transfer), strict: true, scan: true),
                 EventMatcher (event: WalletEvent.balanceUpdated(amount: balance), strict: true, scan: true),
            ])
        case .api_only:
            // Balance before transfer... doesn't seem right. But worse, all balance events arrive
            // before all transfer events.
            return checkWalletEvents(
                [EventMatcher (event: WalletEvent.created),
                 EventMatcher (event: WalletEvent.balanceUpdated(amount: balance), strict: true, scan: true),
                 EventMatcher (event: WalletEvent.transferAdded(transfer: transfer), strict: true, scan: true),
            ])
        default:
            return false
        }
    }
}

class BRCryptoSystemBaseTests: BRCryptoBaseTests {

    var listener: CryptoTestSystemListener!
    var query: BlockChainDB!
    var system: System!

    var registerCurrencyCodes = [String]()
    var currencyCodesToMode = ["btc":WalletManagerMode.api_only]

    var currencyModels: [BlockChainDB.Model.Currency] = []

    func createDefaultListener() -> CryptoTestSystemListener {
        return CryptoTestSystemListener (networkCurrencyCodesToMode: currencyCodesToMode,
                                         registerCurrencyCodes: registerCurrencyCodes,
                                         isMainnet: isMainnet)
    }

    func createDefaultQuery () -> BlockChainDB {
        return BlockChainDB.createForTest()
    }

    func prepareSystem (listener: CryptoTestSystemListener? = nil, query: BlockChainDB? = nil) {

        self.listener = listener ?? createDefaultListener()
        self.query    = query    ?? createDefaultQuery()

        system = System (listener:  self.listener,
                         account:   self.account,
                         onMainnet: self.isMainnet,
                         path:      self.coreDataDir,
                         query:     self.query)

        XCTAssertEqual (coreDataDir + "/" + self.account.fileSystemIdentifier, system.path)
        XCTAssertTrue  (self.query === system.query)
        XCTAssertEqual (account.uids, system.account.uids)

        system.configure(withCurrencyModels: currencyModels) // Don't connect
        wait (for: [self.listener.networkExpectation], timeout: 5)
        wait (for: [self.listener.managerExpectation], timeout: 5)
        wait (for: [self.listener.walletExpectation ], timeout: 5)
    }

    override func setUp() {
        super.setUp()
    }
}
