//
//  CoreXDemoBitcoinClient.swift
//  CoreXDemo
//
//  Created by Ed Gamble on 11/8/18.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import Foundation
import BRCrypto

class CoreDemoListener: SystemListener {
    public var walletListeners: [WalletListener] = []
    public var transferListeners: [TransferListener] = []

    func handleSystemEvent(system: System, event: SystemEvent) {
        NSLog ("SystemEvent: \(event)")
        switch event {
        case .created:
            break

        case .networkAdded(let network):
            // A network was created; create the corresponding wallet manager.  Note: an actual
            // App might not be interested in having a wallet manager for every network -
            // specifically, test networks are announced and having a wallet manager for a
            // testnet won't happen in a deployed App.

            let _ = system.createWalletManager (network: network,
                                                mode: WalletManagerMode.api_only)

        case .managerAdded (let manager):
            manager.connect()

        }
    }

    func handleManagerEvent(system: System, manager: WalletManager, event: WalletManagerEvent) {
        NSLog ("ManagerEvent: \(event)")
        switch event {
        case .created:
            break
        case .changed: // (let oldState, let newState):
            break
        case .deleted:
            break
        case .walletAdded: // (let wallet):
            break
        case .walletChanged: // (let wallet):
            break
        case .walletDeleted: // (let wallet):
            break
        case .syncStarted:
            break
        case .syncProgress: // (let percentComplete):
            break
        case .syncEnded: // (let error):
            break
        }
    }

    func handleWalletEvent(system: System, manager: WalletManager, wallet: Wallet, event: WalletEvent) {
        NSLog ("WalletEvent: \(event)")
        walletListeners.forEach {
            $0.handleWalletEvent (system: system,
                                  manager: manager,
                                  wallet: wallet,
                                  event: event)
        }
    }

    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
        NSLog ("TransferEvent: \(event)")
        transferListeners.forEach {
            $0.handleTransferEvent (system: system,
                                    manager: manager,
                                    wallet: wallet,
                                    transfer: transfer,
                                    event: event)
        }
    }

    func handleNetworkEvent(system: System, network: Network, event: NetworkEvent) {
        NSLog ("NetworkEvent: \(event)")
    }
}

