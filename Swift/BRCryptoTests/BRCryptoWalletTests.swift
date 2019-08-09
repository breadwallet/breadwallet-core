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

class BRCryptoWalletTests: BRCryptoSystemBaseTests {

    override func setUp() {
        super.setUp()
    }

    override func tearDown() {
    }

    func testWalletBTC() {
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

        XCTAssertTrue  (system === wallet.system)
        XCTAssertEqual (manager, wallet.manager)
        XCTAssertEqual (network.currency, wallet.currency)
        XCTAssertEqual (wallet.unit, manager.unit)
        XCTAssertEqual (wallet.unit,       network.defaultUnitFor(currency: network.currency))
        XCTAssertEqual (wallet.unitForFee, network.defaultUnitFor(currency: network.currency))
        XCTAssertEqual (wallet.balance, Amount.create(integer: 0, unit: wallet.unit))
        XCTAssertEqual (wallet.state, WalletState.created)
        XCTAssertEqual (wallet.target, wallet.targetForScheme(manager.addressScheme))
        XCTAssertEqual (wallet, wallet)

        let feeBasisPricePerCostFactor = Amount.create (integer: 5000, unit: network.baseUnitFor(currency: network.currency)!)
        let feeBasisCostFactor = 1.0
        let feeBasis: TransferFeeBasis! =
            wallet.createTransferFeeBasis (pricePerCostFactor: feeBasisPricePerCostFactor,
                                           costFactor: feeBasisCostFactor)
        XCTAssertNotNil(feeBasis)
        XCTAssertEqual (feeBasis.currency, wallet.unitForFee.currency)
        XCTAssertEqual (feeBasis.pricePerCostFactor, feeBasisPricePerCostFactor)
        XCTAssertEqual (feeBasis.costFactor, feeBasisCostFactor)

        // No existing transfers
        XCTAssertTrue (wallet.transfers.isEmpty)

        // With no existing transfers, cannot create a transfer (no BTC UTXOs)
        let transferBaseUnit = network.baseUnitFor(currency: network.currency)!
        let transferTargetAddress = wallet.target
        let transferAmount = Amount.create (integer: 40000, unit: transferBaseUnit)
        var transfer: Transfer! = wallet.createTransfer (target: transferTargetAddress,
                                                         amount: transferAmount,
                                                         estimatedFeeBasis: feeBasis)
        XCTAssertNil(transfer)

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

        // Try again
        transfer = wallet.createTransfer (target: transferTargetAddress,
                                          amount: transferAmount,
                                          estimatedFeeBasis: feeBasis)
        XCTAssertNotNil(transfer)
        XCTAssertEqual (transfer.wallet,  wallet)
        XCTAssertEqual (transfer.manager, manager)
        XCTAssertNotNil(transfer.estimatedFeeBasis)
        // the transfer's estimatedFeeBasis is the original feeBasis but w/ a correct cost factor
        XCTAssertNotEqual (transfer.estimatedFeeBasis!.fee, feeBasis.fee)
        XCTAssertNotEqual (transfer.fee, feeBasis.fee)
        XCTAssertEqual (transfer.target, transferTargetAddress)
        XCTAssertEqual (transfer.unit, wallet.unit)
        XCTAssertEqual (transfer.amount, transferAmount)
        XCTAssertEqual (transfer.amountDirected, transferAmount)

        XCTAssertNil   (transfer.confirmedFeeBasis)
        XCTAssertNil   (transfer.confirmation)
        XCTAssertNil   (transfer.confirmations)
        XCTAssertNil   (transfer.confirmationsAt(blockHeight: 10))

        XCTAssertNil   (transfer.hash)
        if case .created = transfer.state {} else { XCTAssertTrue (false ) }
        XCTAssertEqual (transfer.direction, TransferDirection.recovered)

        // Estiamte the fee
        let feeEstimateExpectation = XCTestExpectation (description: "FeeEstimate")
        var feeEstimateResult: Result<TransferFeeBasis, Wallet.FeeEstimationError>!
        wallet.estimateFee (target: transferTargetAddress,
                            amount: transferAmount,
                            fee: network.minimumFee) { (res: Result<TransferFeeBasis, Wallet.FeeEstimationError>) in
                                feeEstimateResult = res
                                feeEstimateExpectation.fulfill()
        }
        wait (for: [feeEstimateExpectation], timeout: 10)
        XCTAssertNotNil (feeEstimateResult)
        if case .success = feeEstimateResult! {} else { XCTAssertTrue(false) }

        manager.disconnect()
    }

    func testWalletETH() {
        isMainnet = false
        currencyCodesNeeded = ["eth"]
        prepareAccount (AccountSpecification (dict: [
            "identifier": "ginger",
            "paperKey":   "ginger settle marine tissue robot crane night number ramp coast roast critic",
            "timestamp":  "2018-01-01",
            "network":    (isMainnet ? "mainnet" : "testnet")
            ]))
        let listener = CryptoTestSystemListener (currencyCodesNeeded: currencyCodesNeeded, isMainnet: isMainnet)

        // Connect and wait for a number of transfers
        var walletCount: Int = 2
        let walletExpectation = XCTestExpectation (description: "Wallet")
        listener.managerHandlers += [
            { (system: System, manager: WalletManager, event: WalletManagerEvent) -> Void in
                switch event {
                case .walletAdded:
                    walletCount -= 1
                    if 0 == walletCount {
                        walletExpectation.fulfill()
                    }
                default: break
                }
            }]

        prepareSystem(listener: listener)

        let network: Network! = system.networks.first { "eth" == $0.currency.code && isMainnet == $0.isMainnet }
        XCTAssertNotNil (network)

        let manager: WalletManager! = system.managers.first { $0.network == network }
        XCTAssertNotNil (manager)

        wait (for: [walletExpectation ], timeout: 10)
        XCTAssertFalse (manager.wallets.isEmpty)
        XCTAssertTrue  (manager.wallets.count >= 2)
        let w0 = manager.wallets[0]
        let w1 = manager.wallets[1]

        XCTAssertEqual (w0, w0)
        XCTAssertNotEqual (w0, w1)
    }
}
