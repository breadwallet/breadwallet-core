//
//  BRCore2Tests.swift
//  BRCore2Tests
//
//  Created by Ed Gamble on 5/18/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import XCTest
import BRCore

class BRCoreTests: XCTestCase {
    
    override func setUp() {
        super.setUp()
        // Put setup code here. This method is called before the invocation of each test method in the class.
    }
    
    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
        super.tearDown()
    }
    
    func testExample() {
//        let unit = EthereumAmountUnit.ether(WEI)
//        let value = EthereumAmount.ether(createUInt256(10), WEI)
        // This is an example of a functional test case.
        // Use XCTAssert and related functions to verify your tests produce the correct results.
    }

    func testEthereum () {
        let client = TestLightClient (network: EthereumNetwork.mainnet,
                                      paperKey: "ginger settle marine tissue robot crane night number ramp coast roast critic")

        client.node.connect()

        sleep(1 * 60)
        client.node.disconnect()
    }

    func testPerformanceExample() {
        // This is an example of a performance test case.
        self.measure {
            // Put the code you want to measure the time of here.
        }
    }
    
}

// MARK: - Test Client
//
//
//
class TestLightClient : EthereumClient {
    var network : EthereumNetwork
    var node : EthereumWalletManager!

    //
    // Constructors
    //
    init(network: EthereumNetwork, paperKey: String) {
        self.network = network
        self.node = EthereumWalletManager (client: self,
                                           network: network,
                                           type: EthereumType.brd,
                                           paperKey: paperKey)
    }

    //    required init(network: EthereumNetwork, publicKey: Data) {
    //        self.network = network
    //        self.node = EthereumEWM.JSON_RPC (client: self,
    //                                                listener: self,
    //                                                network: network,
    //                                                publicKey: publicKey)
    //    }

    //
    // Client Protocol - JSON_RPC
    //
    func getBalance(ewm: EthereumWalletManager, wid: EthereumWalletId, address: String, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceBalance()
        ewm.announceBalance (wid: wid, balance: "0xffc0", rid: rid)
    }

    func getGasPrice(ewm: EthereumWalletManager, wid: EthereumWalletId, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceGasPrice()
        ewm.announceGasPrice (wid: wid, gasPrice: "0x77", rid: rid)
    }

    func getGasEstimate(ewm: EthereumWalletManager, wid: EthereumWalletId, tid: EthereumTransferId, to: String, amount: String, data: String, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceGasEstimate()
        ewm.announceGasEstimate (wid: wid, tid: tid, gasEstimate: "0x21000", rid: rid)
    }

    func submitTransaction(ewm: EthereumWalletManager, wid: EthereumWalletId, tid: EthereumTransferId, rawTransaction: String, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceSubmitTransaction()
        ewm.announceSubmitTransaction (wid: wid, tid: tid, hash: "0xffaabb", rid: rid)
        return
    }

    func getTransactions(ewm: EthereumWalletManager, address: String, rid: Int32) {
        // JSON_RPC -> [JSON] -> forEach {Result -> announceSubmitTransaction()}
        ewm.announceTransaction(rid: rid,
                                       hash: "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
                                       sourceAddr: ewm.address,
                                       targetAddr: "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
                                       contractAddr: "",
                                       amount: "11113000000000",
                                       gasLimit: "21000",
                                       gasPrice: "21000000000",
                                       data: "0x",
                                       nonce: "118",
                                       gasUsed: "21000",
                                       blockNumber: "1627184",
                                       blockHash: "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                                       blockConfirmations: "339050",
                                       blockTransactionIndex: "3",
                                       blockTimestamp: "1516477482",
                                       isError: "0")
        return
    }

    func getLogs(ewm: EthereumWalletManager, address: String, event: String, rid: Int32) {
        ewm.announceLog(rid: rid,
                               hash: "0xa37bd8bd8b1fa2838ef65aec9f401f56a6279f99bb1cfb81fa84e923b1b60f2b",
                               contract: "0x722dd3f80bac40c951b51bdd28dd19d435762180",
                               topics: ["0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
                                        "0x0000000000000000000000000000000000000000000000000000000000000000",
                                        "0x000000000000000000000000bdfdad139440d2db9ba2aa3b7081c2de39291508"],
                               data: "0x0000000000000000000000000000000000000000000000000000000000002328",
                               gasPrice: "0xba43b7400",
                               gasUsed: "0xc64e",
                               logIndex: "0x",
                               blockNumber: "0x1e487e",
                               blockTransactionIndex: "0x",
                               blockTimestamp: "0x59fa1ac9")
        return
    }

    func getTokens(ewm: EthereumWalletManager, rid: Int32) {
        ewm.announceToken (rid: rid,
                           address: "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",
                           symbol: "BRD",
                           name: "BRD Token",
                           description: "The BRD Token",
                           decimals: 18)

        ewm.announceToken (rid: rid,
                           address: (ewm.network == EthereumNetwork.mainnet
                                ? "0x9e3359f862b6c7f5c660cfd6d1aa6909b1d9504d"
                                : "0x6e67ccd648244b3b8e2f56149b40ba8de9d79b09"),
                           symbol: "CCC",
                           name: "Container Crypto Coin",
                           description: "",
                           decimals: 18)
    }


    func getBlockNumber(ewm: EthereumWalletManager, rid: Int32) {
        ewm.announceBlockNumber(blockNumber: "5900000", rid: rid)
    }

    func getNonce(ewm: EthereumWalletManager, address: String, rid: Int32) {
        ewm.announceNonce(address: address, nonce: "17", rid: rid)
        }

        func savePeers(ewm: EthereumWalletManager) {
            print ("TST: savePeers\n")
        }

        func saveBlocks(ewm: EthereumWalletManager) {
            print ("TST: saveBlocks\n")
        }

        func changeTransaction(ewm: EthereumWalletManager, change: EthereumClientChangeType) {
            print ("TST: changeTransaction\n")
        }

        func changeLog(ewm: EthereumWalletManager, change: EthereumClientChangeType) {
            print ("TST: changeLog\n")
        }


    //
    // Listener Protocol
    //
    func handleEWMEvent(ewm: EthereumWalletManager, event: EthereumEWMEvent) {
        print ("TST: EWMEvent: \(event)")
    }

    func handlePeerEvent(ewm: EthereumWalletManager, event: EthereumPeerEvent) {
        print ("TST: PeerEvent: \(event)\n")
    }

    func handleWalletEvent(ewm: EthereumWalletManager,
                           wallet: EthereumWallet,
                           event: EthereumWalletEvent) {
        // Get the wallet... then
        print ("TST: WalletEvent: \(event)\n")
        switch (event) {
        default: // Never here
            break;
        }
    }

    func handleBlockEvent(ewm: EthereumWalletManager,
                          block: EthereumBlock,
                          event: EthereumBlockEvent) {
        // Get the block(?)... then
        switch (event) {
        default: // Never here
            break;
        }
    }

    func handleTransferEvent(ewm: EthereumWalletManager,
                                wallet: EthereumWallet,
                                transfer: EthereumTransfer,
                                event: EthereumTransferEvent) {
        // Get the transaction... then
        print ("TST: TransactionEvent: \(event)\n")
        switch (event) {
        default: // Never here
            break;
        }
    }

    func saveNodes(ewm: EthereumWalletManager) {
        print ("TST: SaveNodes")

    }


}

