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

fileprivate class TestListener: SystemListener {
    var networkExpectation = XCTestExpectation (description: "NetworkExpectation")
    var managerExpectation = XCTestExpectation (description: "ManagerExpectation")

    // XCTestExpectation ::  expectation = XCTestExpectation (description: "")

    func handleSystemEvent(system: System, event: SystemEvent) {
        
        switch event {
        case .created:
            break

        case .networkAdded(let network):
            networkExpectation.fulfill()

            // A network was created; create the corresponding wallet manager.  Note: an actual
            // App might not be interested in having a wallet manager for every network -
            // specifically, test networks are announced and having a wallet manager for a
            // testnet won't happen in a deployed App.

            let mode = system.supportsMode (network: network, WalletManagerMode.p2p_only)
                ? WalletManagerMode.p2p_only
                : system.defaultMode(network: network)
            let scheme = system.defaultAddressScheme(network: network)


            let _ = system.createWalletManager (network: network,
                                                mode: mode,
                                                addressScheme: scheme)

        case .managerAdded:
            managerExpectation.fulfill()
        }
    }
    
    func handleNetworkEvent(system: System, network: Network, event: NetworkEvent) {
    }

    func handleManagerEvent (system: System, manager: WalletManager, event: WalletManagerEvent) {
    }

    func handleWalletEvent(system: System, manager: WalletManager, wallet: Wallet, event: WalletEvent) {
    }

    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
    }
}

class BRCryptoSystemTests: BRCryptoBaseTests {
//    let url  = URL (string: "http:/brd.com/")!

    var storagePath: String!

    override func setUp() {
        super.setUp()
        print ("TST: StoragePath: \(coreDataDir ?? "<none>")");
    }

    override func tearDown() {
    }

    func testSystemBTC() {
        let listener = TestListener ()
        let query    = BlockChainDB()

        let sys = System (listener: listener,
                          account: account,
                          onMainnet: true,
                          path: coreDataDir,
                          query: query)

        XCTAssertEqual (coreDataDir, sys.path)
        XCTAssertTrue  (query === sys.query)
        XCTAssertEqual (account.uids, sys.account.uids)

        // Create the network and wallet manager - don't connect
        sys.configure()
        wait(for: [listener.networkExpectation], timeout: 5)

        XCTAssertEqual (1, sys.networks.count)
        let network = sys.networks[0]


        wait (for: [listener.managerExpectation], timeout: 5)
        XCTAssertEqual (1, sys.managers.count)
        let manager = sys.managers[0]

        XCTAssertTrue (sys     === manager.system)
//        XCTAssertTrue (account === manager.account)
        XCTAssertTrue (network  == manager.network)
        XCTAssertTrue (query   === manager.query)

        XCTAssertTrue (manager === sys.managerBy(core: manager.core))

        let wallet = manager.primaryWallet
        XCTAssertTrue (sys === wallet.system)
        XCTAssertTrue (manager === wallet.manager)
        XCTAssertTrue (wallet.transfers.isEmpty)

        XCTAssertEqual (network.currency, wallet.currency)
        XCTAssertEqual (manager.currency, wallet.currency)
        XCTAssertTrue  (network.defaultUnitFor(currency: network.currency).map { $0 == wallet.unit } ?? false)
        XCTAssertEqual (wallet.balance, Amount.create(integer: 0, unit: manager.baseUnit))
        XCTAssertEqual (wallet.state, WalletState.created)

        let targetAddress = wallet.target
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
