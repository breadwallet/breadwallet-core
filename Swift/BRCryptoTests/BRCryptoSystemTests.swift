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

class TestListener: SystemListener {
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
//        case .changed(let oldState, let newState):
//            break
//        case .deleted:
//            break
//        case .walletAdded(let wallet):
//            break
//        case .walletChanged(let wallet):
//            break
//        case .walletDeleted(let wallet):
//            break
//        case .syncStarted:
//            break
//        case .syncProgress(let percentComplete):
//            break
//        case .syncEnded(let error):
//            break
        }
    }

    func handleWalletEvent(system: System, manager: WalletManager, wallet: Wallet, event: WalletEvent) {
    }

    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
    }


}

class BRCryptoSystemTests: BRCryptoBaseTests {
//    let url  = URL (string: "http:/brd.com/")!

    var account: Account!
    var storagePath: String!

    override func setUp() {
        super.setUp()

        account = Account.createFrom (phrase: paperKey)!

        storagePath = FileManager.default
            .urls(for: .documentDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("Core").path

        do {
            if FileManager.default.fileExists(atPath: storagePath) {
                try FileManager.default.removeItem(atPath: storagePath)
            }

            try FileManager.default.createDirectory (atPath: storagePath,
                                                     withIntermediateDirectories: true,
                                                     attributes: nil)
        }
        catch let error as NSError {
            print("Error: \(error.localizedDescription)")
        }

        NSLog ("StoragePath: \(storagePath ?? "<none>")");
    }

    override func tearDown() {
    }

    func testSystem() {
        let listener = TestListener ()

        let sys = SystemBase (listener: listener,
                              account: account,
                              path: storagePath,
                              query: BlockChainDB())

        sys.start (networksNeeded: ["bitcoin-mainnet", "ethereum-mainnet"]);
        sleep(10)
    }

}
