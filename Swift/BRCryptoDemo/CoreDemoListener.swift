//
//  CoreXDemoBitcoinClient.swift
//  CoreXDemo
//
//  Created by Ed Gamble on 11/8/18.
//  Copyright © 2018-2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import UIKit
import Foundation
import BRCrypto

class CoreDemoListener: SystemListener {
    
    static let eventQueue: DispatchQueue = DispatchQueue.global()
    
    private var managerListeners: [WalletManagerListener] = []
    private var walletListeners: [WalletListener] = []
    private var transferListeners: [TransferListener] = []

    private let networkCurrencyCodesToMode: [String:WalletManagerMode]
    private let registerCurrencyCodes: [String]

    internal var isMainnet: Bool

    public init (networkCurrencyCodesToMode: [String:WalletManagerMode],
                 registerCurrencyCodes: [String],
                 isMainnet: Bool) {
        self.networkCurrencyCodesToMode = networkCurrencyCodesToMode
        self.registerCurrencyCodes = registerCurrencyCodes;
        self.isMainnet = isMainnet
    }

    func add(managerListener: WalletManagerListener) {
        CoreDemoListener.eventQueue.async {
            if !self.managerListeners.contains (where: { $0 === managerListener }) {
                self.managerListeners.append (managerListener)
            }
        }
    }
    
    func remove(managerListener: WalletManagerListener) {
        CoreDemoListener.eventQueue.async {
            if let i = self.managerListeners.firstIndex (where: { $0 === managerListener }) {
                self.managerListeners.remove (at: i)
            }
        }
    }
    
    func add(walletListener: WalletListener) {
        CoreDemoListener.eventQueue.async {
            if !self.walletListeners.contains (where: { $0 === walletListener }) {
                self.walletListeners.append (walletListener)
            }
        }
    }
    
    func remove(walletListener: WalletListener) {
        CoreDemoListener.eventQueue.async {
            if let i = self.walletListeners.firstIndex (where: { $0 === walletListener }) {
                self.walletListeners.remove (at: i)
            }
        }
    }
    
    func add(transferListener: TransferListener) {
        CoreDemoListener.eventQueue.async {
            if !self.transferListeners.contains (where: { $0 === transferListener }) {
                self.transferListeners.append (transferListener)
            }
        }
    }

    func remove(transferListener: TransferListener) {
        CoreDemoListener.eventQueue.async {
            if let i = self.transferListeners.firstIndex (where: { $0 === transferListener }) {
                self.transferListeners.remove (at: i)
            }
        }
    }
    
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
                if !success {
                    system.wipe (network: network)
                    let successRetry = system.createWalletManager (network: network,
                                                                   mode: mode,
                                                                   addressScheme: scheme,
                                                                   currencies: currencies)
                    if !successRetry { UIApplication.doError(network: network) }
                }
            }

        case .managerAdded (let manager):
            //TODO: Don't connect here. connect on touch...
            DispatchQueue.main.async {
                manager.connect (using: UIApplication.peer (network: manager.network))
            }

        case .discoveredNetworks (let networks):
            let allCurrencies = networks.flatMap { $0.currencies }
            print ("APP: System: Currencies (Added): ")
            allCurrencies.forEach { print ("APP: System:    \($0.code)") }
        }
    }

    func handleManagerEvent(system: System, manager: WalletManager, event: WalletManagerEvent) {
        CoreDemoListener.eventQueue.async {
            print ("APP: Manager (\(manager.name)): \(event)")
            self.managerListeners.forEach {
                $0.handleManagerEvent(system: system,
                                      manager: manager,
                                      event: event)
            }
        }
    }

    func handleWalletEvent(system: System, manager: WalletManager, wallet: Wallet, event: WalletEvent) {
        CoreDemoListener.eventQueue.async {
            print ("APP: Wallet (\(manager.name):\(wallet.name)): \(event)")
            self.walletListeners.forEach {
                $0.handleWalletEvent (system: system,
                                      manager: manager,
                                      wallet: wallet,
                                      event: event)
            }
        }
    }

    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
        CoreDemoListener.eventQueue.async {
            print ("APP: Transfer (\(manager.name):\(wallet.name)): \(event)")
            self.transferListeners.forEach {
                $0.handleTransferEvent (system: system,
                                        manager: manager,
                                        wallet: wallet,
                                        transfer: transfer,
                                        event: event)
            }
        }
    }

    func handleNetworkEvent(system: System, network: Network, event: NetworkEvent) {
        print ("APP: Network: \(event)")
    }
}

