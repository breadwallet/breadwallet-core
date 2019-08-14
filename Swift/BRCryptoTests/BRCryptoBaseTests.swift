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
/// Event Matching
///
protocol MatchableEvent {
    func match (_ that: Self, strict: Bool) -> Bool
}

extension TransferEvent:  MatchableEvent {
    public func match (_ that: TransferEvent, strict: Bool) -> Bool {
        switch (self, that) {
        case (.created, .created):
            return true
        case (let .changed (oldState1, newState1), let .changed (oldState2, newState2)):
            return !strict || (oldState1 == oldState2 && newState1 == newState2)
        case (.deleted, .deleted):
            return true
        default: return false
        }
    }
}

extension WalletEvent: MatchableEvent {
    public func match (_ that: WalletEvent, strict: Bool) -> Bool {
        switch (self, that) {
        case (.created, .created):
            return true
        case (let .changed (oldState1, newState1), let .changed (oldState2, newState2)):
            return !strict || (oldState1 == oldState2 && newState1 == newState2)
        case (.deleted, .deleted):
            return true

        case (let .transferAdded (transfer1), let .transferAdded(transfer2)):
            return !strict || (transfer1 == transfer2)
        case (let .transferChanged (transfer1), let .transferChanged(transfer2)):
            return !strict || (transfer1 == transfer2)
        case (let .transferDeleted (transfer1), let .transferDeleted(transfer2)):
            return !strict || (transfer1 == transfer2)
        case (let .transferSubmitted (transfer1, success1), let .transferSubmitted(transfer2, success2)):
            return !strict || (transfer1 == transfer2 && success1 == success2)

        case (let .balanceUpdated (amount1), let .balanceUpdated (amount2)):
            return !strict || (amount1 == amount2)
        case (let .feeBasisUpdated (feeBasis1), let .feeBasisUpdated (feeBasis2)):
            return !strict || (feeBasis1 == feeBasis2)
        case (let .feeBasisEstimated (feeBasis1), let .feeBasisEstimated (feeBasis2)):
            return !strict || (feeBasis1 == feeBasis2)

        default: return false
        }
    }
}

extension WalletManagerEvent: MatchableEvent {
    public func match (_ that: WalletManagerEvent, strict: Bool) -> Bool {
        switch (self, that) {
        case (.created, .created): return true
        case (let .changed (oldState1, newState1), let .changed (oldState2, newState2)):
            return !strict || (oldState1 == oldState2 && newState1 == newState2)
        case (let .walletAdded (wallet1), let .walletAdded(wallet2)):
            return !strict || (wallet1 == wallet2)
        case (let .walletChanged (wallet1), let .walletChanged(wallet2)):
            return !strict || (wallet1 == wallet2)
        case (let .walletDeleted (wallet1), let .walletDeleted(wallet2)):
            return !strict || (wallet1 == wallet2)
        case (.syncStarted, .syncStarted): return true
        case (let .syncProgress (progress1), let .syncProgress (progress2)):
            return !strict || (progress1 == progress2)
        case (let .syncEnded (error1), let .syncEnded (error2)):
            return !strict || (error1 == error2)
        case (let .blockUpdated (height1), let .blockUpdated (height2)):
            return !strict || (height1 == height2)
        default: return false
        }
    }
}

extension NetworkEvent:       MatchableEvent {
    func match (_ that: NetworkEvent, strict: Bool) -> Bool {
        switch (self, that) {
        case (.created, .created):
            return true
        }
    }
}

extension SystemEvent: MatchableEvent {
    public func match (_ that: SystemEvent, strict: Bool) -> Bool {
        switch (self, that) {
        case (.created, .created):
            return true
        case (let .networkAdded (network1), let .networkAdded (network2)):
            return !strict || (network1 == network2)
        case (let .managerAdded (manager1), let .managerAdded (manager2)):
            return !strict || (manager1 == manager2)
        default:
            return false
        }
    }
}


struct EventMatcher<ME: MatchableEvent> {
    let event: ME

    // When `strict` expect an exact match
    let strict: Bool

    // Given an array of events, if `scan` is true, then this EventMatcher can match any event
    // in the array (scan over events until a match is found)
    let scan: Bool

    // Return `true` is this `EventMatcher` matches `other` using `strict`
    func match (_ other: ME) -> Bool {
        return self.event.match (other, strict: strict)
    }

    init (event: ME, strict: Bool = true, scan: Bool = false) {
        self.event = event
        self.strict = strict
        self.scan = scan
    }

    // if `self` matches in `others` from `index` return the index at the match.  This will
    // use `scan`.
    func match (_ others: [ME], from index: Int) -> Int? {
        if index >= others.count { return nil }         // No event -> no match
        else if match (others[index]) { return index }  // On match -> index
        else if !scan { return nil }                    // No scan -> no match
        else { return match (others, from: 1 + index) } // keep looking
    }

    // Return the first index in `others` where `self` matches
    func match (_ others: [ME]) -> Int? {
        return match(others, from: 0)
    }
}

extension Array where Element:MatchableEvent {
    private func match (selfIndex: Int, _ matchers: [EventMatcher<Element>], matchIndex: Int) -> Bool {
        if matchIndex >= matchers.count { return true  } // No matchers left -> success
        else if selfIndex >= self.count { return false } // Matchers but no elements -> failure
        else if let index = matchers[matchIndex].match (self, from: selfIndex) {
            // Got a match -> next matcher, next index
            return match (selfIndex: 1 + index, matchers, matchIndex: 1 + matchIndex)
        }
        else { return false}
    }

    // Return true if every `matcher` has a match in self
    func match (_ matchers: [EventMatcher<Element>]) -> Bool {
        return match (selfIndex: 0, matchers, matchIndex: 0)
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


    public init (currencyCodesNeeded: [String], isMainnet: Bool) {
        self.currencyCodesNeeded = currencyCodesNeeded
        self.isMainnet = isMainnet
    }

    var systemHandlers: [SystemEventHandler] = []
    var systemEvents: [SystemEvent] = []

    func handleSystemEvent(system: System, event: SystemEvent) {
        print ("TST: System Event: \(event)")
        systemEvents.append (event)
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
        print ("TST: Wallet Event: \(event)")
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

    var transferCount: Int = 0;
    var transferHandlers: [TransferEventHandler] = []
    var transferEvents: [TransferEvent] = []
    var transferExpectation = XCTestExpectation (description: "TransferExpectation")

    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
        print ("TST: Transfer Event: \(event)")
        transferEvents.append (event)
        switch event {
        case .created:
            if 1 == transferCount { transferExpectation.fulfill()}
            if 1 <= transferCount { transferCount -= 1 }
        default: break
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
