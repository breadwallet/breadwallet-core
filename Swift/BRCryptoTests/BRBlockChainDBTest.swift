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

        db.getBlockchains { (res: Result<[BlockChainDB.Model.Blockchain], BlockChainDB.QueryError>) in
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

        let transferId = "bitcoin-mainnet:78a05731f82519762e8462f709e095908e77edd5eb740ba1202451212a884b9e:2"
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

        let transactionId = "bitcoin-mainnet:cfadf40e698a8f90452c2c5b96304638376c7aa98b4448c90e7a2192db0c8a02"
        db.getTransaction(transactionId: transactionId, includeRaw: true) { (res: Result<BlockChainDB.Model.Transaction, BlockChainDB.QueryError>) in
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

        let blockchainId = "bitcoin-mainnet"
        db.getTransactions (blockchainId: blockchainId, addresses: [], includeRaw: true) { (res: Result<[BlockChainDB.Model.Transaction], BlockChainDB.QueryError>) in
            guard case let .success (transactions) = res
                else { XCTAssert(false); return }

            XCTAssertTrue (transactions.isEmpty)
            self.expectation.fulfill()
        }

        wait (for: [expectation], timeout: 60)

        expectation = XCTestExpectation (description: "transactions /w addresses")

        db.getTransactions (blockchainId: blockchainId, addresses: ["abc", "def"], includeRaw: true) { (res: Result<[BlockChainDB.Model.Transaction], BlockChainDB.QueryError>) in
            guard case let .success (transactions) = res
                else { XCTAssert(false); return }

            XCTAssertTrue (transactions.isEmpty)
            self.expectation.fulfill()
        }

        wait (for: [expectation], timeout: 60)

        ///
        ///
        expectation = XCTestExpectation (description: "transactions w/ [0,11000)")

        db.getTransactions (blockchainId: blockchainId,
                            addresses: [],
                            begBlockNumber: 0,
                            endBlockNumber: 11000,
                           includeRaw: true) { (res: Result<[BlockChainDB.Model.Transaction], BlockChainDB.QueryError>) in
            guard case let .success (transactions) = res
                else { XCTAssert(false); return }

            XCTAssertFalse (transactions.isEmpty)
            self.expectation.fulfill()
        }

        wait (for: [expectation], timeout: 60)

    }

    func testBlocks () {
        expectation = XCTestExpectation (description: "block")

        let blockId = "bitcoin-mainnet:0000000000000000001ed7770597decf0f98fe4f099111c6a0073ceabbd1e812"
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

        let blockchainId = "bitcoin-mainnet"
        db.getBlocks (blockchainId: blockchainId, includeRaw: true) { (res: Result<[BlockChainDB.Model.Block], BlockChainDB.QueryError>) in
            guard case let .success (blocks) = res
                else { XCTAssert(false); return }

            XCTAssertFalse (blocks.isEmpty)
            self.expectation.fulfill()
        }

        wait (for: [expectation], timeout: 60)
    }

    func testWallet () {
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
