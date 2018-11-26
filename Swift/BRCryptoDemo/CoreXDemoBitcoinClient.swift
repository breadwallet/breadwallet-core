//
//  CoreXDemoBitcoinClient.swift
//  CoreXDemo
//
//  Created by Ed Gamble on 11/8/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import Foundation
import BRCrypto

class CoreXDemoListener : WalletManagerListener, BitcoinListener, EthereumListener {

    public func handleManagerEvent(manager: WalletManager,
                                   event: WalletManagerEvent) {
        print ("TST: UI Handling WalletManager Event")
    }

    public func handleWalletEvent (manager: WalletManager,
                                   wallet: Wallet,
                                   event: WalletEvent) -> Void {
        print ("TST: UI Handling Wallet Event")
    }

    public func handleTransferEvent (manager: WalletManager,
                                     wallet: Wallet,
                                     transfer: Transfer,
                                     event: TransferEvent) -> Void {
        print ("TST: UI Handling Transfer Event")
    }

    func handleTokenEvent(manager: WalletManager, token: EthereumToken, event: EthereumTokenEvent) {
        print ("TST: UI Handling Token Event")
    }
}
