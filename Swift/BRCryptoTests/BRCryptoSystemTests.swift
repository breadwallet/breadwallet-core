//
//  BRCryptoSystemTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 3/25/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
import XCTest
@testable import BRCrypto

class TestSystem : System {
}

class TestListener: SystemListener {
    func handleNetworkEvent(system: System, network: Network, event: NetworkEvent) {
        guard let system = system as? TestSystem else { assert (false); return }
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

    func handleManagerEvent(system: System, manager: WalletManager, event: WalletManagerEvent) {
        guard let _ /* system */ = system as? TestSystem else { assert (false); return }
        switch event {
        case .created:
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

class BRCryptoSystemTests: XCTestCase {



    let path = "/tmp/"
    let url  = URL (string: "http:/brd.com/")!

    let phrase  = "ginger settle marine tissue robot crane night number ramp coast roast critic"
    var account: Account!

    override func setUp() {
        account = Account.createFrom (phrase: phrase)!

    }

    override func tearDown() {
    }

    func testSystem() {
        let listener = TestListener ()
        
        let sys = TestSystem (account: account,
                                   listener: listener,
                                   persistencePath: path,
                                   blockchainURL: url);

        sys.start();

    }

}
