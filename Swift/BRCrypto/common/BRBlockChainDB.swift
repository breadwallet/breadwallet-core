//
//  BRBlockChainDB.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//

import Foundation

import BRCore
import BRCore.Ethereum

public class BlockChainDB {
    let queue = DispatchQueue.init(label: "BlockChainDB")

    public enum QueryError: Error {
        case NetworkUnavailable
        case RequestFormat  // sent a mistake
        case Model          // reply w/ an error (missing label, can't parse)
    }

    public struct Model {
        /// Blockchain
        public typealias Blockchain = (id: String, name: String, network: String, isMainnet: Bool, currency: String, blockHeight: UInt64 /* fee Estimate */)
        /// Currency
        public typealias CurrencyDenomination = (name: String, code: String, decimals: UInt8, symbol: String /* extra */)
        public typealias Currency = (id: String, name: String, code: String, type: String, blockchainID: String, address: String?, demoninations: [CurrencyDenomination])
        /// ...
    }

    public func getBlockchains (mainnet: Bool? = nil, completion: @escaping ([Model.Blockchain]) -> Void) {  // Result<DBBlockchain, BlockChainDB.Error>
        queue.async {
            var result: [Model.Blockchain] = []

            if mainnet ?? true { // mainnet or nil
                result.append ((id: "bitcoin-mainnet",  name: "Bitcoin",  network: "mainnet", isMainnet: true,  currency: "btc", blockHeight:  600000))
                result.append ((id: "bitcash-mainnet",  name: "Bitcash",  network: "mainnet", isMainnet: true,  currency: "bch", blockHeight: 1000000))
                result.append ((id: "ethereum-mainnet", name: "Ethereum", network: "mainnet", isMainnet: true,  currency: "eth", blockHeight: 8000000))
            }

            if !(mainnet ?? false) { // !mainnet or nil
                result.append  ((id: "bitcoin-testnet",  name: "Bitcoin",  network: "testnet", isMainnet: false, currency: "btc", blockHeight:  900000))
                result.append  ((id: "bitcash-testnet",  name: "Bitcash",  network: "testnet", isMainnet: false, currency: "bch", blockHeight: 1200000))
                result.append  ((id: "ethereum-testnet", name: "Ethereum", network: "testnet", isMainnet: false, currency: "eth", blockHeight: 1000000))
                result.append  ((id: "ethereum-rinkeby", name: "Ethereum", network: "rinkeby", isMainnet: false, currency: "eth", blockHeight: 2000000))
            }

            completion (result)
        }
    }

    public func getCurrencies (blockchainID: String? = nil, completion: @escaping ([Model.Currency]) -> Void) {
        queue.async {
            var result: [Model.Currency] = []

            if blockchainID?.starts(with: "bitcoin") ?? true {
                // BTC
                result.append  ((id: "Bitcoin", name: "Bitcoin", code: "btc", type: "native", blockchainID: "bitcoin-mainnet", address: nil,
                                 demoninations: [(name: "satoshi", code: "sat", decimals: 0, symbol: self.lookupSymbol ("sat")),
                                                 (name: "bitcoin", code: "btc", decimals: 8, symbol: self.lookupSymbol ("btc"))]))
                // testnet?
            }

            if blockchainID?.starts(with: "bitcash") ?? true {
                // BCH
                result.append  ((id: "Bitcash", name: "Bitcash", code: "bch", type: "native", blockchainID: "bitcash-mainnet", address: nil,
                                 demoninations: [(name: "satoshi", code: "sat", decimals: 0, symbol: self.lookupSymbol ("sat")),
                                                 (name: "bitcoin", code: "bch", decimals: 8, symbol: self.lookupSymbol ("bch"))]))
                // testnet??
            }

            if blockchainID?.starts(with: "ethereum") ?? true {
                // ETH
                result.append  ((id: "Ethereum", name: "Ethereum", code: "eth", type: "native", blockchainID: "ethereum-mainnet", address: nil,
                                 demoninations: [(name: "wei",   code: "wei",  decimals:  0, symbol: self.lookupSymbol ("wei")),
                                                 (name: "gwei",  code: "gwei", decimals:  9, symbol: self.lookupSymbol ("gwei")),
                                                 (name: "ether", code: "eth",  decimals: 18, symbol: self.lookupSymbol ("eth"))]))

                result.append  ((id: "BRD Token", name: "BRD Token", code: "BRD", type: "erc20", blockchainID: "ethereum-mainnet", address: BlockChainDB.addressBRDMainnet,
                                 demoninations: [(name: "BRD_INTEGER",   code: "BRDI",  decimals:  0, symbol: "brdi"),
                                                 (name: "BRD",           code: "BRD",   decimals: 18, symbol: "brd")]))
                // ropsten, rinkeby?
            }

            completion (result)
        }
    }

    /// ...

    /// ETH
    public struct ETH {
        public typealias Balance = (wid: BREthereumWallet, balance: String, rid: Int32)
        public typealias GasPrice = (wid: BREthereumWallet, gasPrice: String, rid: Int32)
        public typealias GasEstimate = (wid: BREthereumWallet, tid: BREthereumTransfer, gasEstimate: String, rid: Int32)

        public typealias Submit = (
            wid: BREthereumWallet,
            tid: BREthereumTransfer,
            hash: String,
            errorCode: Int32,
            errorMessage: String?,
            rid: Int32)

        public typealias Transaction = (
            hash: String,
            sourceAddr: String,
            targetAddr: String,
            contractAddr: String,
            amount: String,
            gasLimit: String,
            gasPrice: String,
            data: String,
            nonce: String,
            gasUsed: String,
            blockNumber: String,
            blockHash: String,
            blockConfirmations: String,
            blockTransactionIndex: String,
            blockTimestamp: String,
            isError: String,
            rid: Int32)

        public typealias Log = (
            hash: String,
            contract: String,
            topics: [String],
            data: String,
            gasPrice: String,
            gasUsed: String,
            logIndex: String,
            blockNumber: String,
            blockTransactionIndex: String,
            blockTimestamp: String,
            rid: Int32)

        public typealias Token = (
            address: String,
            symbol: String,
            name: String,
            description: String,
            decimals: UInt32,
            defaultGasLimit: String?,
            defaultGasPrice: String?,
            rid: Int32)

        public typealias Block = (numbers: [UInt64], rid: Int32)
        public typealias BlockNumber = (number: String, rid: Int32)
        public typealias Nonce = (address: String, nonce: String, rid: Int32)

    }

    public func getBalanceAsETH (ewm: BREthereumEWM,
                                 wid: BREthereumWallet,
                                 address: String,
                                 rid: Int32,
                                 completion: @escaping (ETH.Balance) -> Void) {
        queue.async {
            let balance = nil == ewmWalletGetToken(ewm, wid)
                ?  "200000000000000000" // 0.2 ETH
                : "3000000000000000000" // 3.0 TOK
            completion ((wid: wid, balance: balance, rid: rid))
        }
    }

    public func getGasPriceAsETH (ewm: BREthereumEWM,
                                  wid: BREthereumWallet,
                                  rid: Int32,
                                  completion: @escaping (ETH.GasPrice) -> Void) {
        queue.async {
            completion ((wid: wid, gasPrice: "0xffc0", rid: rid ))
        }
    }

    public func getGasEstimateAsETH (ewm: BREthereumEWM,
                                     wid: BREthereumWallet,
                                     tid: BREthereumTransfer,
                                     from: String,
                                     to: String,
                                     amount: String,
                                     data: String,
                                     rid: Int32,
                                     completion: @escaping (ETH.GasEstimate) -> Void) {
        queue.async {
            completion ((wid: wid, tid: tid, gasEstimate: "92000", rid: rid))
        }
    }

    public func submitTransactionAsETH (ewm: BREthereumEWM,
                                        wid: BREthereumWallet,
                                        tid: BREthereumTransfer,
                                        transaction: String,
                                        rid: Int32,
                                        completion: @escaping (ETH.Submit) -> Void) {
        queue.async {
            completion ((wid: wid, tid: tid, hash: "0x123abc456def", errorCode: Int32(-1), errorMessage: nil, rid: rid))
        }
    }

    public func getTransactionsAsETH (ewm: BREthereumEWM,
                                      address: String,
                                      begBlockNumber: UInt64,
                                      endBlockNumber: UInt64,
                                      rid: Int32,
                                      done: @escaping (Bool, Int32) -> Void,
                                      each: @escaping (ETH.Transaction) -> Void) {
        queue.async {
//            if (begBlockNumber <= 1627184 && 1627184 <= endBlockNumber) {
                each ((hash: "0x5f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
                       sourceAddr: "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
                       targetAddr: address,
                       contractAddr: "",
                       amount: "11113000000000",
                       gasLimit: "21000",
                       gasPrice: "21000000000",
                       data: "",
                       nonce:  "118",
                       gasUsed: "21000",
                       blockNumber: "1627184",
                       blockHash: "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                       blockConfirmations: "339050",
                       blockTransactionIndex: "3",
                       blockTimestamp: "1516477482",
                       isError: "0",
                       rid: rid))
 //           }

//            if (begBlockNumber <= 1627184 && 1627184 <= endBlockNumber) {
                each ((hash: "0x5f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
                       sourceAddr: address,
                       targetAddr: "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
                       contractAddr: "",
                       amount: "11113000000000",
                       gasLimit: "21000",
                       gasPrice: "21000000000",
                       data: "",
                       nonce:  "118",
                       gasUsed: "21000",
                       blockNumber: "1627184",
                       blockHash: "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                       blockConfirmations: "339050",
                       blockTransactionIndex: "3",
                       blockTimestamp: "1516477482",
                       isError: "0",
                       rid: rid))
 //           }
            done (true, rid)
        }
    }

    public func getLogsAsETH (ewm: BREthereumEWM,
                              address: String,
                              begBlockNumber: UInt64,
                              endBlockNumber: UInt64,
                              rid: Int32,
                              done: @escaping (Bool, Int32) -> Void,
                              each: @escaping (ETH.Log) -> Void) {
        queue.async {
//            if (begBlockNumber <= 0x1e487e && 0x1e487e <= endBlockNumber) {
                each ((hash: "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
                       contract: "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",
                       topics: ["0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
                                "0x0000000000000000000000000000000000000000000000000000000000000000",
                                "0x000000000000000000000000bdfdad139440d2db9ba2aa3b7081c2de39291508"],
                       data: "0x0000000000000000000000000000000000000000000000000000000000002328",
                       gasPrice: "0xba43b7400",
                       gasUsed: "0xc64e",
                       logIndex: "0x",
                       blockNumber: "0x1e487e",
                       blockTransactionIndex: "0x",
                       blockTimestamp: "0x59fa1ac9",
                       rid: rid))
 //           }

            done (true, rid)
        }
    }

    public func getTokensAsETH (ewm: BREthereumEWM,
                                rid: Int32,
                                done: @escaping (Bool, Int32) -> Void,
                                each: @escaping (ETH.Token) -> Void) {
        queue.async {
            each ((address: "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",
                   symbol: "BRD",
                   name: "BRD Token",
                   description: "BRD Token Description",
                   decimals: 18,
                   defaultGasLimit: nil,
                   defaultGasPrice: nil,
                   rid: rid))

            each ((address: "0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0",
                   symbol: "EOS",
                   name: "EOS",
                   description: "",
                   decimals: 18,
                   defaultGasLimit: nil,
                   defaultGasPrice: nil,
                   rid: rid))

            done (true, rid)
        }
    }

    public func getBlocksAsETH (ewm: BREthereumEWM,
                                address: String,
                                interests: UInt32,
                                blockStart: UInt64,
                                blockStop: UInt64,
                                rid: Int32,
                                completion: @escaping (ETH.Block) -> Void) {
        queue.async {
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

            case "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62".lowercased():
                if 0 != interests & UInt32 (1 << 3) /* CLIENT_GET_BLOCKS_LOGS_AS_TARGET */ {
                    blockNumbers += [5732521]
                }

                if 0 != interests & UInt32 (1 << 2) /* CLIENT_GET_BLOCKS_LOGS_AS_SOURCE */ {
                    // ~8 blocks
                }

                if 0 != interests & UInt32 (1 << 1) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET */ {
                    // ~5 blocks
                }

                if 0 != interests & UInt32 (1 << 0) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE */ {
                    // ~ 45 blocks
                }

            default:
                blockNumbers.append(contentsOf: [blockStart,
                                                 (blockStart + blockStop) / 2,
                                                 blockStop])
            }

            blockNumbers = blockNumbers.filter { blockStart < $0 && $0 < blockStop }
            completion ((numbers: blockNumbers, rid: rid))
        }
    }

    public func getBlockNumberAsETH (ewm: BREthereumEWM,
                                     rid: Int32,
                                     completion: @escaping (ETH.BlockNumber) -> Void) {
        queue.async {
            completion ((number: "0xffc0", rid: rid ))
        }
    }
    
    public func getNonceAsETH (ewm: BREthereumEWM,
                               address: String,
                               rid: Int32,
                               completion: @escaping (ETH.Nonce) -> Void) {
        queue.async {
            completion ((address: address, nonce: "118", rid: rid ))
        }
    }

    public init () {}

    internal let currencySymbols = ["btc":"b", "eth":"Ξ"]
    internal func lookupSymbol (_ code: String) -> String {
        return currencySymbols[code] ?? code
    }

    static internal let addressBRDTestnet = "0x7108ca7c4718efa810457f228305c9c71390931a" // testnet
    static internal let addressBRDMainnet = "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6" // mainnet
}
