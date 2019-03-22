//
//  BRCryptoWalletManagerTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 1/11/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//

import XCTest
@testable import BRCrypto

#if false
class Listener : BitcoinListener, EthereumListener {
    func handleTokenEvent(manager: WalletManager, token: EthereumToken, event: EthereumTokenEvent) {

    }

    func handleManagerEvent(manager: WalletManager, event: WalletManagerEvent) {
    }

    func handleWalletEvent(manager: WalletManager, wallet: Wallet, event: WalletEvent) {
    }

    func handleTransferEvent(manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
    }
}
#endif

class BRCryptoWalletManagerTests: XCTestCase {

    #if false
    var listener: Listener!
    #endif

    override func setUp() {
    }

    override func tearDown() {
    }

    /// MARK: Bitcoin



    /// MARK: Ethereum

}
