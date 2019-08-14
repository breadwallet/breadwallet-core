//
//  BRCryptoTransferTests.swift
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
import BRCryptoC

class BRCryptoTransferTests: BRCryptoSystemBaseTests {
    override func setUp() {
        super.setUp()
    }

    override func tearDown() {
    }

    func testTransferBTC() {
        isMainnet = false
        currencyCodesNeeded = ["btc"]
        prepareAccount (AccountSpecification (dict: [
            "identifier": "ginger",
            "paperKey":   "ginger settle marine tissue robot crane night number ramp coast roast critic",
            "timestamp":  "2018-01-01",
            "network":    (isMainnet ? "mainnet" : "testnet")
            ]))
        let listener = CryptoTestSystemListener (currencyCodesNeeded: currencyCodesNeeded, isMainnet: isMainnet)
        prepareSystem(listener: listener)

        let network: Network! = system.networks.first { "btc" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        let manager: WalletManager! = system.managers.first { $0.network == network }
        XCTAssertNotNil (manager)

        let wallet = manager.primaryWallet
        XCTAssertNotNil(wallet)

        // Connect and wait for a number of transfers
        var transferCount: Int = 10
        let transferExpectation = XCTestExpectation (description: "Transfer")
        listener.walletHandlers += [
            { (system: System, manager: WalletManager, wallet: Wallet, event: WalletEvent) -> Void in
                switch event {
                case .transferAdded:
                    transferCount -= 1
                    if 0 == transferCount {
                        transferExpectation.fulfill()
                    }
                default: break
                }
            }]
        manager.connect()
        wait (for: [transferExpectation ], timeout: 70)

        XCTAssertFalse (wallet.transfers.isEmpty)
        XCTAssertTrue  (wallet.transfers.count >= 10)
        let t0 = wallet.transfers[0]
        let t1  = wallet.transfers[1]

        XCTAssertEqual (t0.wallet,  wallet)
        XCTAssertEqual (t0.manager, manager)
        XCTAssertNotNil (t0.confirmedFeeBasis)

        XCTAssertNotNil (t0.confirmation)
        // confirmations
        // confirmationsAs
        let t0c = t0.confirmation!

        // Fails until TransferState(core: ...) fills in Fee - requires unit...CORE-421
        XCTAssertNotNil (t0c.fee)

        XCTAssertNotNil (t0.hash)
        XCTAssertNotNil (wallet.transferBy(hash: t0.hash!))
        XCTAssertNotNil (wallet.transferBy(hash: t1.hash!))
        XCTAssertNotNil (wallet.transferBy(core: t0.core))
        XCTAssertNotNil (wallet.transferBy(core: t1.core))

        if case .included = t0.state {} else { XCTAssertTrue (false)}
        // direction

        manager.disconnect()
    }

    func testTransferETH () {
        isMainnet = false
        currencyCodesNeeded = ["eth"]
        prepareAccount (AccountSpecification (dict: [
            "identifier": "ginger",
            "paperKey":   "ginger settle marine tissue robot crane night number ramp coast roast critic",
            "timestamp":  "2018-01-01",
            "network":    (isMainnet ? "mainnet" : "testnet")
            ]))
        let listener = CryptoTestSystemListener (currencyCodesNeeded: currencyCodesNeeded, isMainnet: isMainnet)
        prepareSystem(listener: listener)

        let network: Network! = system.networks.first { "eth" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        let manager: WalletManager! = system.managers.first { $0.network == network }
        XCTAssertNotNil (manager)

        let wallet = manager.primaryWallet
        XCTAssertNotNil(wallet)

        // Connect and wait for a number of transfers
        var transferCount: Int = 3
        let transferExpectation = XCTestExpectation (description: "Transfer")
        listener.walletHandlers += [
            { (system: System, manager: WalletManager, wallet: Wallet, event: WalletEvent) -> Void in
                switch event {
                case .transferAdded:
                    transferCount -= 1
                    if 0 == transferCount {
                        transferExpectation.fulfill()
                    }
                default: break
                }
            }]
        manager.connect()
        wait (for: [transferExpectation ], timeout: 70)

        XCTAssertFalse (wallet.transfers.isEmpty)
        XCTAssertTrue  (wallet.transfers.count >= 3)
        let t0 = wallet.transfers[0]
        let t1  = wallet.transfers[1]

        XCTAssertTrue  (t0.identical(that: t0))
        XCTAssertFalse (t0.identical(that: t1))
        XCTAssertEqual (t0, t0)
        XCTAssertNotEqual (t0, t1)

        XCTAssertTrue  (t0.system === system)
        XCTAssertEqual (t0.wallet,  wallet)
        XCTAssertEqual (t0.manager, manager)
        XCTAssertNotNil(t0.source)
        XCTAssertNotNil(t0.target)
        XCTAssertNotNil (t0.confirmedFeeBasis)

        XCTAssertNotNil (t0.confirmation)
        // confirmations
        // confirmationsAs
        let t0c = t0.confirmation!

        // Fails until TransferState(core: ...) fills in Fee - requires unit...CORE-421
        XCTAssertNotNil (t0c.fee)

        XCTAssertNotNil (t0.hash)
        XCTAssertNotNil (t1.hash)
        XCTAssertEqual (t0.hash, t0.hash)
        XCTAssertNotEqual(t0.hash, t1.hash)
        let _: [TransferHash:Int] = [t0.hash!:0, t1.hash!:1]

        if case .included = t0.state {} else { XCTAssertTrue (false)}
        // direction

        manager.disconnect()
    }

    
    func testTransferConfirmation () {
        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native", issuer: nil)
        //let BTC_SATOSHI = BRCrypto.Unit (currency: btc, uids: "BTC-SAT",  name: "Satoshi", symbol: "SAT")

        let confirmation = TransferConfirmation (blockNumber: 1,
                                                 transactionIndex: 2,
                                                 timestamp: 3,
                                                 fee: nil)
        XCTAssertEqual(1, confirmation.blockNumber)
        XCTAssertEqual(2, confirmation.transactionIndex)
        XCTAssertEqual(3, confirmation.timestamp)
        XCTAssertNil(confirmation.fee)
    }

    func testTransferDirection () {
        XCTAssertEqual(TransferDirection.sent,      TransferDirection (core: BRCryptoTransferDirection (rawValue: 0)))
        XCTAssertEqual(TransferDirection.received,  TransferDirection (core: BRCryptoTransferDirection (rawValue: 1)))
        XCTAssertEqual(TransferDirection.recovered, TransferDirection (core: BRCryptoTransferDirection (rawValue: 2)))
    }

    func testTransferHash () {
    }

    func testTransferFeeBasis () {
    }

    func testTransferState () {
        // XCTAssertEqual (TransferState.created, TransferState(core: CRYPTO_TRANSFER_STATE_CREATED))
        // ...
    }

}
