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

struct TransferResult {
    let target: Bool
    let address: String
    let confirmation: TransferConfirmation
    let hash: String
    let amount: UInt64

    func validate (transfer: Transfer) -> Bool {
        guard address == (target ? transfer.target : transfer.source)?.description
            else { return false }

        guard let transferHash = transfer.hash?.description, self.hash == transferHash
            else { return false }

        guard let transferInteger = transfer.amount.integerRawSmall, amount == transferInteger
            else { return false }

       guard let transferConfirmation = transfer.confirmation // , self.confirmation == transferConfirmation
            else { return false }

         // If we have a result confirmation fee, compare to transfer fee
        guard nil == confirmation.fee || confirmation.fee == transfer.fee
            else { return false }

        // If the transfer's confirmations's transactionIndex is non-zero, compare it
        guard 0 == transferConfirmation.transactionIndex ||
            confirmation.transactionIndex == transferConfirmation.transactionIndex
            else { return false }

        guard transferConfirmation.blockNumber == confirmation.blockNumber,
            transferConfirmation.timestamp == confirmation.timestamp
            else { return false }

        return true
    }
}

class BRCryptoTransferTests: BRCryptoSystemBaseTests {
    let syncTimeoutInSeconds = 120.0

    /// Recv: 0.00010000
    ///
    /// Address: https://live.blockcypher.com/btc-testnet/address/mzjmRwzABk67iPSrLys1ACDdGkuLcS6WQ4/
    /// Transactions:
    ///    0: https://live.blockcypher.com/btc-testnet/tx/f6d9bca3d4346ce75c151d1d8f061d56ff25e41a89553544b80d316f7d9ccedc/
    ///    1:
    let knownAccountSpecification = AccountSpecification (dict: [
        "identifier": "general",
        "paperKey":   "general shaft mirror pave page talk basket crumble thrive gaze bamboo maid",
        "timestamp":  "2019-08-15",
        "network":    "testnet"
    ])

    func knownTransferResultsByModeStrangely (mode: WalletManagerMode) -> [TransferResult] {
        return [
            //
            // This transfer result has a different `timestamp` depending on `mode`.  When the
            // `mode` is .p2p_only, the P2P code sets the timestamp as the average of the previous
            // block's timestamp and the current block's timestamp.  In .api_only, the transaction
            // timestamp is set as the block's timestamp.  Note: in .p2p_only the transaction's
            // timestamp will be again different if it identified in the process of being included
            // in the blockchain.
            //
            TransferResult (target: true,
                            address: "mzjmRwzABk67iPSrLys1ACDdGkuLcS6WQ4",
                            confirmation: TransferConfirmation (blockNumber: 1574853,
                                                                transactionIndex: 26,
                                                                timestamp: (mode == .p2p_only // ?? 2019-08-16T16:53:30Z ??
                                                                    ? 1565974068
                                                                    : 1565974410),
                                                                fee: nil,
                                                                success: true,
                                                                error: nil),
                            hash: "0xf6d9bca3d4346ce75c151d1d8f061d56ff25e41a89553544b80d316f7d9ccedc",
                            amount: UInt64(1000000))
        ]
    }

    override func setUp() {
        super.setUp()
    }

    override func tearDown() {
    }

    /// MARK: - BTC

    func runTransferBTCTest (mode: WalletManagerMode) {
        isMainnet = false
        currencyCodesToMode = ["btc":mode]
        prepareAccount (knownAccountSpecification)
        prepareSystem()

        let knownTransferResults = knownTransferResultsByModeStrangely(mode: mode)

        let walletManagerDisconnectExpectation = XCTestExpectation (description: "Wallet Manager Disconnect")
        let walletManagerSyncDoneExpectation   = XCTestExpectation (description: "Wallet Manager Sync Done")
        listener.managerHandlers += [
            { (system: System, manager:WalletManager, event: WalletManagerEvent) in
                if case let .changed(_, newState) = event, case .disconnected = newState {
                    walletManagerDisconnectExpectation.fulfill()
                }
            },
            { (system: System, manager:WalletManager, event: WalletManagerEvent) in
                if case .syncEnded = event {
                    walletManagerSyncDoneExpectation.fulfill()
                }
            }
        ]

        let network: Network! = system.networks.first { "btc" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        let manager: WalletManager! = system.managers.first { $0.network == network }
        XCTAssertNotNil (manager)
        manager.addressScheme = AddressScheme.btcLegacy

        let wallet = manager.primaryWallet
        XCTAssertNotNil(wallet)

        // Connect and wait for a number of transfers
        listener.transferIncluded = true
        listener.transferCount = knownTransferResults.count
        manager.connect()
        wait (for: [listener.transferExpectation], timeout: syncTimeoutInSeconds)

        // If a P2P mode, wait for syncDone
        if (WalletManagerMode.p2p_only == mode) {
            wait (for: [walletManagerSyncDoneExpectation], timeout: syncTimeoutInSeconds)
        }

        manager.disconnect()
        wait (for: [walletManagerDisconnectExpectation], timeout: 5)

        let transfers = wallet.transfers
        XCTAssertEqual  (transfers.count, knownTransferResults.count)

        XCTAssertTrue (zip (knownTransferResults, transfers).reduce(true) {
            (result:Bool, pair:(TransferResult, Transfer)) -> Bool in
            return result && pair.0.validate(transfer: pair.1)
        })

        let transfer = transfers[0]
        XCTAssertNotNil (transfer.confirmation)
        XCTAssertNotNil (transfer.hash)

        XCTAssertNotNil (wallet.transferBy(hash: transfer.hash!))
        XCTAssertNotNil (wallet.transferBy(core: transfer.core))

        transfers.forEach {
            if let address = (transfer.direction == TransferDirection.received
                ? $0.target
                : $0.source) {
                XCTAssertTrue (wallet.hasAddress (address))
            }
            else { XCTAssertTrue(false) }
        }
        // Events

        XCTAssertTrue (listener.checkSystemEventsCommonlyWith (network: network,
                                                               manager: manager))

        // The disconnect reason varies in P2P mode, hence lenient.
        XCTAssertTrue (listener.checkManagerEventsCommonlyWith (mode: mode,
                                                                wallet: wallet,
                                                                lenientDisconnect: (mode == .p2p_only)))

        // The wallet events have a balance evnet with 0.0 in P2P mode. Expect failure.
        XCTAssertTrue (listener.checkWalletEventsCommonlyWith (mode: mode,
                                                               balance: wallet.balance,
                                                               transfer: transfer))
    }

    func testTransferBTC_API() {
        runTransferBTCTest(mode: WalletManagerMode.api_only)
    }

    func testTransferBTC_P2P() {
        runTransferBTCTest(mode: WalletManagerMode.p2p_only)
    }

    /// MARK: - BCH

    /// TODO: This test fails intermittently
    func testTransferBCH_P2P () {
        isMainnet = true
        currencyCodesToMode = ["bch":WalletManagerMode.p2p_only]
        prepareAccount (identifier: "loan(C)")
        prepareSystem()

        let walletManagerDisconnectExpectation = XCTestExpectation (description: "Wallet Manager Disconnect")
        listener.managerHandlers += [
            { (system: System, manager:WalletManager, event: WalletManagerEvent) in
                if case let .changed(_, newState) = event, case .disconnected = newState {
                    walletManagerDisconnectExpectation.fulfill()
                }
            }]

        let network: Network! = system.networks.first { .bch == $0.type && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        let manager: WalletManager! = system.managers.first { $0.network == network }
        XCTAssertNotNil (manager)
        manager.addressScheme = AddressScheme.btcLegacy

        let wallet = manager.primaryWallet
        XCTAssertNotNil(wallet)

        // Connect and wait for a number of transfers
        listener.transferIncluded = true
        listener.transferCount = 1
        manager.connect()
        wait (for: [listener.transferExpectation], timeout: syncTimeoutInSeconds)

        manager.disconnect()
        wait (for: [walletManagerDisconnectExpectation], timeout: 5)

        XCTAssertTrue (wallet.transfers.count > 0)
        if (wallet.transfers.count > 0) {
            let transfer = wallet.transfers[0]
            XCTAssertTrue (nil != transfer.source || nil != transfer.target)
            if let source = transfer.source {
                XCTAssertTrue (source.description.starts (with: (isMainnet ? "bitcoincash" : "bchtest")))
            }
            if let target = transfer.target {
                XCTAssertTrue (target.description.starts (with: (isMainnet ? "bitcoincash" : "bchtest")))
            }
    }
    }
    
    /// MARK: - ETH

    func runTransferETHTest () {
        let network: Network! = system.networks.first { "eth" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        let manager: WalletManager! = system.managers.first { $0.network == network }
        XCTAssertNotNil (manager)

        let wallet = manager.primaryWallet
        XCTAssertNotNil(wallet)

        // Connect and wait for a number of transfers
        listener.transferCount = 3
        manager.connect()
        wait (for: [listener.transferExpectation], timeout: 70)

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

    }

    func testTransferETH_API () {
        isMainnet = false
        currencyCodesToMode = ["eth":WalletManagerMode.api_only]
        prepareAccount (AccountSpecification (dict: [
            "identifier": "ginger",
            "paperKey":   "ginger settle marine tissue robot crane night number ramp coast roast critic",
            "timestamp":  "2018-01-01",
            "network":    (isMainnet ? "mainnet" : "testnet")
            ]))
        prepareSystem()

        runTransferETHTest()
    }

    
    func testTransferConfirmation () {
        let confirmation = TransferConfirmation (blockNumber: 1,
                                                 transactionIndex: 2,
                                                 timestamp: 3,
                                                 fee: nil,
                                                 success: true,
                                                 error: nil)
        XCTAssertEqual(1, confirmation.blockNumber)
        XCTAssertEqual(2, confirmation.transactionIndex)
        XCTAssertEqual(3, confirmation.timestamp)
        XCTAssertNil(confirmation.fee)
    }

    func testTransferDirection () {
        XCTAssertEqual(TransferDirection.sent,      TransferDirection (core: TransferDirection.sent.core))
        XCTAssertEqual(TransferDirection.received,  TransferDirection (core: TransferDirection.received.core))
        XCTAssertEqual(TransferDirection.recovered, TransferDirection (core: TransferDirection.recovered.core))
    }
    #if false
    func testTransferHash () {
    }

    func testTransferFeeBasis () {
    }

    func testTransferState () {
        // XCTAssertEqual (TransferState.created, TransferState(core: CRYPTO_TRANSFER_STATE_CREATED))
        // ...
    }
    #endif
}
