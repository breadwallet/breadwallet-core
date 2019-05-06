//
//  BRCryptoWalletTests.swift
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
import BRCore

fileprivate class TestListener: SystemListener {
    var walletExpectation = XCTestExpectation (description: "walletExpectation")
    var btcWallet: Wallet! = nil
    var ethWallet: Wallet! = nil
    var brdWallet: Wallet! = nil

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
        if case .created = event {
            print ("Wallet: \(wallet.name)")
            switch wallet.name.lowercased() {
            case Currency.codeAsBTC:
                btcWallet = wallet
            case Currency.codeAsETH:
                ethWallet = wallet
            case "brd":
                brdWallet = wallet
            default:
                return
            }

            if nil != btcWallet && nil != ethWallet && nil != brdWallet {
                walletExpectation.fulfill()
            }
        }
    }

    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
    }
}

///
///
///
class BRCryptoWalletTests: BRCryptoBaseTests {
    fileprivate var listener: TestListener! = nil
    var system: System! = nil

    override func setUp() {
        super.setUp()
        if (nil == listener) {
            // Race condition
            listener = TestListener()
            SystemBase.resetForTest()
            system   = SystemBase.create (listener: listener,
                                          account: account,
                                          path: coreDataDir,
                                          query: BlockChainDB())
            system.start (networksNeeded: ["bitcoin-mainnet", "ethereum-mainnet"])
            wait (for: [listener.walletExpectation], timeout: 10)
        }
    }

    override func tearDown() {
    }


    func genericWalletTest (wallet: Wallet) {

        // guard let defaultUnit = wallet.manager.network.defaultUnitFor(currency: wallet.currency)
        //     else { XCTAssertTrue(false); return }

        guard let baseUnit = wallet.manager.network.baseUnitFor (currency: wallet.currency)
            else { XCTAssertTrue(false); return }

        let feeUnit = wallet.manager.defaultUnit

        XCTAssertEqual(wallet.balance, Amount.create( integer: 0, unit: baseUnit))

        let feeBasis = wallet.defaultFeeBasis
        let fee = wallet.estimateFee (amount: Amount.create(integer: 1, unit: baseUnit), feeBasis: nil)

        switch feeBasis {
        case let .bitcoin(feePerKB):
            // No transactions in wallet... for BTC fee will be zero
            XCTAssertEqual (feePerKB, DEFAULT_FEE_PER_KB)
            XCTAssertEqual (fee, Amount.create(integer: 0, unit: feeUnit))

        case let .ethereum (gasPrice, gasLimit):
            XCTAssertEqual (gasLimit, wallet === listener.ethWallet ? 21000 : 92000)
            XCTAssertEqual (fee.asBTC, gasPrice.asETH * gasLimit)
        }
    }

    func testWallet() {
        genericWalletTest(wallet: listener.btcWallet)
        genericWalletTest(wallet: listener.ethWallet)
        genericWalletTest(wallet: listener.brdWallet)
    }

//    func testPerformanceExample() {
//        self.measure {
//        }
//    }
}
