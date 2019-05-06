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
    // XCTestExpectation ::  expectation = XCTestExpectation (description: "")

    func handleSystemEvent(system: System, event: SystemEvent) {
        switch event {
        default:
            break
        }
    }
    
    func handleNetworkEvent(system: System, network: Network, event: NetworkEvent) {
        switch event {
        case .created:
            // A network was created; create the corresponding wallet manager.  Note: an actual
            // App might not be interested in having a wallet manager for every network -
            // specifically, test networks are announced and having a wallet manager for a
            // testnet won't happen in a deployed App.

            let _ = system.createWalletManager (network: network,
                                                mode: WalletManagerMode.p2p_only)
        }
    }

    func handleManagerEvent (system: System, manager: WalletManager, event: WalletManagerEvent) {
        switch event {
        case .created:
            manager.connect()
            // A WalletManager was created; create a wallet for each currency.
            break
        default:
            break
        }
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

    func testSystem() {
        let listener = TestListener ()

        SystemBase.resetForTest()
        let sys = SystemBase.create (listener: listener,
                                     account: account,
                                     path: coreDataDir,
                                     query: BlockChainDB())

        sys.start (networksNeeded: ["bitcoin-mainnet", "ethereum-mainnet"]);
        sleep(10)
    }
}
