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
        prepareSystem()

        let network: Network! = system.networks.first { "btc" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        let manager: WalletManager! = system.managers.first { $0.network == network }
        XCTAssertNotNil (manager)
    }

    func testTransferETH () {
        isMainnet = false
        currencyCodesNeeded = ["eth"]
        prepareSystem()

        let network: Network! = system.networks.first { "eth" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        let manager: WalletManager! = system.managers.first { $0.network == network }
        XCTAssertNotNil (manager)
    }

    
    func testTransferConfirmation () {
        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native", issuer: nil)
        let BTC_SATOSHI = BRCrypto.Unit (currency: btc, uids: "BTC-SAT",  name: "Satoshi", symbol: "SAT")

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
    func testTransfer () {
/*
        let wid = BRWalletNew ()
        let tid = 
        let coreTransfer = cryptoTransferCreateAsBTC (coreCurrency, wid, tid);

        let transfer = Transfer (core: <#T##BRCryptoTransfer#>,
                                 listener: <#T##TransferListener?#>,
                                 wallet: <#T##Wallet#>,
                                 unit: <#T##Unit#>)
*/
    }
}
