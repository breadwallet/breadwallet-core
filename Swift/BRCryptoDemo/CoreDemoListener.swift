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
    public var managerListeners: [WalletManagerListener] = []
    public var walletListeners: [WalletListener] = []
    public var transferListeners: [TransferListener] = []

    let currencyCodeToModeMap: [String : WalletManagerMode] = [
        Currency.codeAsBTC : WalletManagerMode.p2p_only,
        Currency.codeAsBCH : WalletManagerMode.p2p_only,
        Currency.codeAsETH : WalletManagerMode.api_only
        ]

    func handleSystemEvent(system: System, event: SystemEvent) {
        print ("APP: System: \(event)")
        switch event {
        case .created:
            break

        case .networkAdded(let network):

            // A network was created; create the corresponding wallet manager.  Note: an actual
            // App might not be interested in having a wallet manager for every network -
            // specifically, test networks are announced and having a wallet manager for a
            // testnet won't happen in a deployed App.

            let code = network.currency.code.lowercased()

            let _ = system.createWalletManager (network: network,
                                                mode: currencyCodeToModeMap[code] ?? WalletManagerMode.api_only)

        case .managerAdded (let manager):
            manager.connect()

        }
    }

    func handleManagerEvent(system: System, manager: WalletManager, event: WalletManagerEvent) {
        print ("APP: Manager (\(manager.name)): \(event)")
        managerListeners.forEach {
            $0.handleManagerEvent(system: system,
                                  manager: manager,
                                  event: event)
        }
    }

    func handleWalletEvent(system: System, manager: WalletManager, wallet: Wallet, event: WalletEvent) {
        print ("APP: Wallet (\(manager.name):\(wallet.name)): \(event)")
        walletListeners.forEach {
            $0.handleWalletEvent (system: system,
                                  manager: manager,
                                  wallet: wallet,
                                  event: event)
        }
    }

    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
        print ("APP: Transfer (\(manager.name):\(wallet.name)): \(event)")
        transferListeners.forEach {
            $0.handleTransferEvent (system: system,
                                    manager: manager,
                                    wallet: wallet,
                                    transfer: transfer,
                                    event: event)
        }
    }

    func handleNetworkEvent(system: System, network: Network, event: NetworkEvent) {
        print ("APP: Network: \(event)")
    }
}

