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
//                result.append ((id: "bitcoin-mainnet",  name: "Bitcoin",  network: "mainnet", isMainnet: true,  currency: "btc", blockHeight: 600000))
                result.append ((id: "ethereum-mainnet", name: "Ethereum", network: "mainnet", isMainnet: true,  currency: "eth", blockHeight: 8000000))
            }

            if !(mainnet ?? false) { // !mainnet or nil
//                result.append  ((id: "bitcoin-testnet",  name: "Bitcoin",  network: "testnet", isMainnet: false, currency: "btc", blockHeight:  900000))
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
//                result.append  ((id: "Bitcoin", name: "Bitcoin", code: "btc", type: "native", blockchainID: "bitcoin-mainnet", address: nil,
//                                 demoninations: [(name: "satoshi", code: "sat", decimals: 0, symbol: self.lookupSymbol ("sat")),
//                                                 (name: "bitcoin", code: "btc", decimals: 8, symbol: self.lookupSymbol ("btc"))]))
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
            }

            completion (result)
        }
    }

    /// ...

    /// ETH
    public struct ETH {
        public typealias Balance = (wid: BREthereumWallet, balance: String, rid: Int32)
        /// ...

        public typealias Block = (numbers: [UInt64], rid: Int32)
    }

    /// Balance As ETH
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

    /// ...


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



    init () {}

    internal let currencySymbols = ["btc":"b", "eth":"Ξ"]
    internal func lookupSymbol (_ code: String) -> String {
        return currencySymbols[code] ?? code
    }

    static internal let addressBRDTestnet = "0x7108ca7c4718efa810457f228305c9c71390931a" // testnet
    static internal let addressBRDMainnet = "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6" // mainnet
}
