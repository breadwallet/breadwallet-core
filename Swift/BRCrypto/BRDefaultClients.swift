//
//  BRDefaultClients.swift
//  BRCrypto
//
//  Created by Ed Gamble on 11/28/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import Foundation
import BRCore
import BRCore.Ethereum

///
/// Bitcoin Stubbed Clients
///


public class DefaultBitcoinBackendClient: BitcoinBackendClient {
    public init () {}
}

public class DefaultBitcoinPersistenceClient: BitcoinPersistenceClient {
    public func savePeers (manager: WalletManager,
                           data: Dictionary<String, String>) {
        print ("TST: BTC: savePeers")
    }

    public func saveBlocks(manager: WalletManager,
                           data: Dictionary<String, String>) {
        print ("TST: BTC: saveBlocks")
    }

    public func changeTransaction (manager: WalletManager,
                                   change: WalletManagerPersistenceChangeType,
                                   hash: String,
                                   data: String) {
        print ("TST: BTC: changeTransaction")
    }

    public init () {}
}

///
/// Ethereum Stubbed Clients
///

///
/// A default implementatio for EthereumBackendClient
///
public class DefaultEthereumBackendClient: EthereumBackendClient {
    public func getBalance(ewm: EthereumWalletManager, wid: BREthereumWallet, address: String, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceBalance()
        let balance = nil == ewmWalletGetToken(ewm.core, wid)
            ?  "200000000000000000" // 0.2 ETH
            : "3000000000000000000" // 3.0 TOK
        ewm.announceBalance (wid: wid, balance: balance, rid: rid)  // 0.2 ETH
    }

    public func getGasPrice(ewm: EthereumWalletManager, wid: BREthereumWallet, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceGasPrice()
        ewm.announceGasPrice (wid: wid, gasPrice: "5000000000", rid: rid) // 5 GWEI
    }

    public func getGasEstimate(ewm: EthereumWalletManager, wid: BREthereumWallet, tid: BREthereumTransfer, to: String, amount: String, data: String, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceGasEstimate()
        let gasEstimate = nil == ewmWalletGetToken(ewm.core, wid)
            ? "21000" // 0.2 ETH
            : "92000"
        ewm.announceGasEstimate (wid: wid, tid: tid, gasEstimate: gasEstimate, rid: rid)
    }

    public func submitTransaction(ewm: EthereumWalletManager, wid: BREthereumWallet, tid: BREthereumTransfer, rawTransaction: String, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceSubmitTransaction()
        ewm.announceSubmitTransaction (wid: wid, tid: tid, hash: "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220", errorCode: -1, errorMessage: nil, rid: rid)
        return
    }

    public func getTransactions(ewm: EthereumWalletManager, address: String, begBlockNumber: UInt64, endBlockNumber: UInt64, rid: Int32) {
        // JSON_RPC -> [JSON] -> forEach {Result -> announceSubmitTransaction()}
        if (begBlockNumber <= 1627184 && 1627184 <= endBlockNumber) {
            ewm.announceTransaction(rid: rid,
                                    hash: "0xa37bd8bd8b1fa2838ef65aec9f401f56a6279f99bb1cfb81fa84e923b1b60f2b",
                                    sourceAddr: ewm.primaryWallet.target.description,
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
        }
        ewm.announceTransactionComplete(rid: rid, success: true)
        return
    }

    public func getLogs(ewm: EthereumWalletManager, address: String, event: String, begBlockNumber: UInt64, endBlockNumber: UInt64, rid: Int32) {
        if (begBlockNumber <= 0x1e487e) {
            ewm.announceLog(rid: rid,
                            hash: "0xa37bd8bd8b1fa2838ef65aec9f401f56a6279f99bb1cfb81fa84e923b1b60f2b",
                            contract: (ewm.network == Ethereum.Networks.mainnet
                                ? "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"
                                : "0x7108ca7c4718efa810457f228305c9c71390931a"),
                            topics: ["0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
                                     "0x00000000000000000000000000000000000000000000000000000000000000aa",
                                     "0x000000000000000000000000bdfdad139440d2db9ba2aa3b7081c2de39291508"],
                            data: "2500000000000000000", // 2.5e18 // "0x0000000000000000000000000000000000000000000000002328000000000000",
                gasPrice: "0xba43b7400",
                gasUsed: "0xc64e",
                logIndex: "0x",
                blockNumber: "0x1e487e",
                blockTransactionIndex: "0x",
                blockTimestamp: "0x59fa1ac9")
        }
        ewm.announceLogComplete(rid: rid, success: true)
        return
    }

    public func getBlocks(ewm: EthereumWalletManager, address: String, interests: UInt32, blockStart: UInt64, blockStop: UInt64, rid: Int32) {
        var blockNumbers : [UInt64] = []
        switch address.lowercased() {
        case "0xb302B06FDB1348915599D21BD54A06832637E5E8".lowercased():
            if 0 != interests & UInt32 (1 << 3) /* CLIENT_GET_BLOCKS_LOGS_AS_TARGET */ {
                blockNumbers += [4847049,
                                 4847152,
                                 4894677,
                                 4965538,
                                 4999850,
                                 5029844]
            }

            if 0 != interests & UInt32 (1 << 2) /* CLIENT_GET_BLOCKS_LOGS_AS_SOURCE */ {
                blockNumbers += [5705175]
            }

            if 0 != interests & UInt32 (1 << 1) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET */ {
                blockNumbers += [4894027,
                                 4908682,
                                 4991227]
            }

            if 0 != interests & UInt32 (1 << 0) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE */ {
                blockNumbers += [4894330,
                                 4894641,
                                 4894677,
                                 4903993,
                                 4906377,
                                 4997449,
                                 4999850,
                                 4999875,
                                 5000000,
                                 5705175]
            }

        case "0xa9de3dbD7d561e67527bC1Ecb025c59D53b9F7Ef".lowercased():
            if 0 != interests & UInt32 (1 << 3) /* CLIENT_GET_BLOCKS_LOGS_AS_TARGET */ {
                blockNumbers += [5506607,
                                 5877545]
            }

            if 0 != interests & UInt32 (1 << 2) /* CLIENT_GET_BLOCKS_LOGS_AS_SOURCE */ {
                blockNumbers += [5506764,
                                 5509990,
                                 5511681]
            }

            if 0 != interests & UInt32 (1 << 1) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET */ {
                blockNumbers += [5506602]
            }

            if 0 != interests & UInt32 (1 << 0) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE */ {
                blockNumbers += [5506764,
                                 5509990,
                                 5511681,
                                 5539808]
            }

        default:
            blockNumbers.append(contentsOf: [blockStart,
                                             (blockStart + blockStop) / 2,
                                             blockStop])
        }

        blockNumbers = blockNumbers.filter { blockStart < $0 && $0 < blockStop }
        ewm.announceBlocks(rid: rid, blockNumbers: blockNumbers)
    }

    public func getTokens(ewm: EthereumWalletManager, rid: Int32) {
        ewm.announceToken (rid: rid,
                           address: (ewm.network == Ethereum.Networks.mainnet
                            ? "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"
                            : "0x7108ca7c4718efa810457f228305c9c71390931a"),
                           symbol: "BRD",
                           name: "BRD Token",
                           description: "The BRD Token",
                           decimals: 18)

        ewm.announceToken (rid: rid,
                           address: (ewm.network == Ethereum.Networks.mainnet
                            ? "0x9e3359f862b6c7f5c660cfd6d1aa6909b1d9504d"
                            : "0x6e67ccd648244b3b8e2f56149b40ba8de9d79b09"),
                           symbol: "CCC",
                           name: "Container Crypto Coin",
                           description: "",
                           decimals: 18)

        ewm.announceToken (rid: rid,
                           address: "0xdd974d5c2e2928dea5f71b9825b8b646686bd200",
                           symbol: "KNC",
                           name: "KNC Token",
                           description: "",
                           decimals: 18)

    }

    public func getBlockNumber(ewm: EthereumWalletManager, rid: Int32) {
        ewm.announceBlockNumber(blockNumber: "5900000", rid: rid)
    }

    public func getNonce(ewm: EthereumWalletManager, address: String, rid: Int32) {
        ewm.announceNonce(address: address, nonce: "48", rid: rid)
    }

    public init () {}
}

///
/// A default implementation for EthereumPersistenceClient.  Note: never called.
///
public class DefaultEthereumPersistenceClient: EthereumPersistenceClient {
    public func savePeers(manager: WalletManager,
                          data: Dictionary<String, String>) {
        print ("TST: ETH: savePeers")
    }

    public func saveBlocks(manager: WalletManager,
                           data: Dictionary<String, String>) {
        print ("TST: ETH: saveBlocks")
    }

    public func changeTransaction (manager: WalletManager,
                                   change: WalletManagerPersistenceChangeType,
                                   hash: String,
                                   data: String) {
        print ("TST: ETH: changeTransaction")
    }

    public func changeLog (manager: WalletManager,
                           change: WalletManagerPersistenceChangeType,
                           hash: String,
                           data: String) {
        print ("TST: ETH: changeLog")
    }

    public init () {}
}

