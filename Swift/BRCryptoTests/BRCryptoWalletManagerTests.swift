//
//  BRCryptoWalletManagerTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 1/11/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
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

    // Bitcoin

    // Ethereum

}
