//
//  BRBlockChainDBTest.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 4/15/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import XCTest
@testable import BRCrypto

class BRBlockChainDBTest: XCTestCase {
    var db: BlockChainDB! = nil
    var expectation: XCTestExpectation!

    override func setUp() {
        db = BlockChainDB ()
    }

    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }

    func testBlockchains() {
        expectation = XCTestExpectation (description: "blockchain")

        let blockchainId = "bitcoin-mainnet"
        db.getBlockchain (blockchainId: blockchainId) { (res: Result<BlockChainDB.Model.Blockchain, BlockChainDB.QueryError>) in
            guard case let .success (blockchain) = res
                else { XCTAssert(false); return }

            XCTAssertEqual (blockchainId, blockchain.id)

            self.expectation.fulfill()
        }

        wait (for: [expectation], timeout: 60)


        expectation = XCTestExpectation (description: "blockchains")

        db.getBlockchains (mainnet: false) { (res: Result<[BlockChainDB.Model.Blockchain], BlockChainDB.QueryError>) in
            guard case let .success (blockchains) = res
                else { XCTAssert(false); return }

            XCTAssertFalse (blockchains.isEmpty)

            self.expectation.fulfill()
        }

        wait (for: [expectation], timeout: 60)
    }

    func testCurrencies() {
        expectation = XCTestExpectation (description: "currency")

        let currencyId = "btc"
        db.getCurrency (currencyId: currencyId) { (res: Result<BlockChainDB.Model.Currency, BlockChainDB.QueryError>) in
            guard case let .success (currency) = res
                else { XCTAssert(false); return }

            XCTAssertEqual (currencyId, currency.code)
            self.expectation.fulfill()
        }
        wait (for: [expectation], timeout: 60)


        expectation = XCTestExpectation (description: "currencies")

        db.getCurrencies { (res: Result<[BlockChainDB.Model.Currency], BlockChainDB.QueryError>) in
            guard case let .success (currencies) = res
                else { XCTAssert(false); return }

            XCTAssertFalse (currencies.isEmpty)

            self.expectation.fulfill()
        }

        wait (for: [expectation], timeout: 60)
   }

    func testTransfers() {
        expectation = XCTestExpectation (description: "transfer")

        let transferId = "bitcoin-testnet:ea4ed7efc701b5fdcfc38d4901f75f90c1a5de3e13fa38590289b2244f8887cb:0"
        db.getTransfer (transferId: transferId) { (res: Result<BlockChainDB.Model.Transfer, BlockChainDB.QueryError>) in
            guard case let .success (transfer) = res
                else { XCTAssert(false); return }

            XCTAssertEqual (transferId, transfer.id)
            self.expectation.fulfill()
        }
        wait (for: [expectation], timeout: 60)

        //
        //
        //

//        expectation = XCTestExpectation (description: "transfers")
//
//        let blockchainId = "bitcoin-mainnet"
//        db.getTransfers (blockchainId: blockchainId, addresses: []) { (res: Result<[BlockChainDB.Model.Transfer], BlockChainDB.QueryError>) in
//            guard case let .success (transfers) = res
//                else { XCTAssert(false); return }
//
//            XCTAssertFalse (transfers.isEmpty)
//            self.expectation.fulfill()
//        }
//
//        wait (for: [expectation], timeout: 60)
    }

    func testTransactions () {
        expectation = XCTestExpectation (description: "transactions")

        let transactionId = "bitcoin-testnet:ea4ed7efc701b5fdcfc38d4901f75f90c1a5de3e13fa38590289b2244f8887cb"
        db.getTransaction (transactionId: transactionId,
                           includeRaw: true) {
                            (res: Result<BlockChainDB.Model.Transaction, BlockChainDB.QueryError>) in
                            guard case let .success (transaction) = res
                                else { XCTAssert(false); return }

                            XCTAssertEqual (transactionId, transaction.id)
                            self.expectation.fulfill()
        }
        wait (for: [expectation], timeout: 60)

        ///
        ///
        ///
        expectation = XCTestExpectation (description: "transactions")

        let blockchainId = "bitcoin-testnet"
        db.getTransactions (blockchainId: blockchainId,
                            addresses: [],
                            includeRaw: true) {
                                (res: Result<[BlockChainDB.Model.Transaction], BlockChainDB.QueryError>) in
                                guard case let .success (transactions) = res
                                    else { XCTAssert(false); return }

                                XCTAssertTrue (transactions.isEmpty)
                                self.expectation.fulfill()
        }

        wait (for: [expectation], timeout: 60)

        ///
        ///
        ///
        expectation = XCTestExpectation (description: "transactions /w addresses nonsense")

        db.getTransactions (blockchainId: blockchainId,
                            addresses: ["abc", "def"],
                            includeRaw: true) {
                                (res: Result<[BlockChainDB.Model.Transaction], BlockChainDB.QueryError>) in
                                guard case let .success (transactions) = res
                                    else { XCTAssert(false); return }

                                XCTAssertTrue (transactions.isEmpty)
                                self.expectation.fulfill()
        }

        wait (for: [expectation], timeout: 60)

        ///
        ///
        ///
        expectation = XCTestExpectation (description: "transactions /w addresses")

        db.getTransactions (blockchainId: blockchainId,
                            addresses: ["2NEpHgLvBJqGFVwQPUA3AQPjpE5gNWhETfT"],
                            begBlockNumber: 1446080,
                            endBlockNumber: 1446090) {
                                (res: Result<[BlockChainDB.Model.Transaction], BlockChainDB.QueryError>) in
                                guard case let .success (transactions) = res
                                    else { XCTAssert(false); return }

                                XCTAssertEqual (2, transactions.count)
                                self.expectation.fulfill()
        }

        wait (for: [expectation], timeout: 60)

        ///
        ///
        ///
        expectation = XCTestExpectation (description: "transactions w/ [0,11000) w/ no address")

        db.getTransactions (blockchainId: blockchainId,
                            addresses: [],
                            begBlockNumber: 1446080,
                            endBlockNumber: 1446090,
                            includeRaw: true) {
                                (res: Result<[BlockChainDB.Model.Transaction], BlockChainDB.QueryError>) in
                                // A 'status' 400
                                guard case .failure = res
                                    else { XCTAssert(false); return }

                                self.expectation.fulfill()
        }

        wait (for: [expectation], timeout: 60)

    }

    func testBlocks () {
        expectation = XCTestExpectation (description: "block")

//        let blockId = "bitcoin-mainnet:0000000000000000001ed7770597decf0f98fe4f099111c6a0073ceabbd1e812"
        let blockId = "bitcoin-testnet:000000000000004deedbdb977277330aa156385bbc60ddc3e49938556b436330"
        db.getBlock (blockId: blockId, includeRaw: true) { (res: Result<BlockChainDB.Model.Block, BlockChainDB.QueryError>) in
            guard case let .success (block) = res
                else { XCTAssert(false); return }

            XCTAssertEqual (blockId, block.id)
            self.expectation.fulfill()
        }
        wait (for: [expectation], timeout: 60)

        ///
        ///
        ///
        expectation = XCTestExpectation (description: "blocks")

        let blockchainId = "bitcoin-testnet"
        db.getBlocks (blockchainId: blockchainId,
                      begBlockNumber: 1446080,
                      endBlockNumber: 1446090,
                      includeRaw: true) { (res: Result<[BlockChainDB.Model.Block], BlockChainDB.QueryError>) in
            guard case let .success (blocks) = res
                else { XCTAssert(false); return }

            XCTAssertEqual ((1446090 - 1446080),  blocks.count)
            self.expectation.fulfill()
        }

        wait (for: [expectation], timeout: 60)
    }

    func dontTestWallet () {
        let walletId = UUID (uuidString: "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96")?.uuidString ?? "empty-wallet-id"

        expectation = XCTestExpectation (description: "wallet create")

        let wallet = (id: walletId, currencies: BlockChainDB.minimalCurrencies)
        db.createWallet (wallet) {
            (res: Result<BlockChainDB.Model.Wallet, BlockChainDB.QueryError>) in
            guard case let .success (wallet) = res
                else { XCTAssert(false); return }

            XCTAssertEqual (walletId, wallet.id )
            self.expectation.fulfill()
        }

        wait(for: [expectation], timeout: 60)

        expectation = XCTestExpectation (description: "wallet delete")

        db.deleteWallet(id: walletId) {
            (res: Result<BlockChainDB.Model.Wallet, BlockChainDB.QueryError>) in
            guard case let .success (wallet) = res
                else { XCTAssert(false); return }

            XCTAssertEqual (walletId, wallet.id )
            self.expectation.fulfill()
        }

        wait(for: [expectation], timeout: 60)
    }

    func testSubscription () {

    }

//    func testPerformanceExample() {
//        self.measure {
//        }
//    }
}
