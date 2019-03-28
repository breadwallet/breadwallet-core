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

    public func getBlockchains (mainnet: Bool = true, completion: (Model.Blockchain) -> Void) {  // Result<DBBlockchain, BlockChainDB.Error>
        if mainnet {
            completion ((id: "bitcoin-mainnet",  name: "Bitcoin",  network: "mainnet", isMainnet: true,  currency: "btc", blockHeight: 600000))
            completion ((id: "ethereum-mainnet", name: "Ethereum", network: "mainnet", isMainnet: true,  currency: "eth", blockHeight: 8000000))
            // eth
        }
        else {
            completion ((id: "bitcoin-testnet",  name: "Bitcoin",  network: "testnet", isMainnet: false, currency: "btc", blockHeight:  900000))
            completion ((id: "ethereum-testnet", name: "Ethereum", network: "testnet", isMainnet: false, currency: "eth", blockHeight: 1000000))
            completion ((id: "ethereum-ropsten", name: "Ethereum", network: "ropsten", isMainnet: false, currency: "eth", blockHeight: 2000000))
       }
    }


    public func getCurrencies (blockchainID: String? = nil, completion: (Model.Currency) -> Void) {
        if blockchainID?.starts(with: "bitcoin") ?? true {
            // BTC
            completion ((id: "Bitcoin", name: "Bitcoin", code: "btc", type: "native", blockchainID: "bitcoin-mainnet", address: nil,
                         demoninations: [(name: "satoshi", code: "sat", decimals: 0, symbol: lookupSymbol ("sat")),
                                         (name: "bitcoin", code: "btc", decimals: 8, symbol: lookupSymbol ("btc"))]))
        }

        if blockchainID?.starts(with: "ethereum") ?? true {
            // ETH
            completion ((id: "Ethereum", name: "Ethereum", code: "eth", type: "native", blockchainID: "ethereum-mainnet", address: nil,
                         demoninations: [(name: "wei",   code: "wei",  decimals:  0, symbol: lookupSymbol ("wei")),
                                         (name: "gwei",  code: "gwei", decimals:  9, symbol: lookupSymbol ("gwei")),
                                         (name: "ether", code: "eth",  decimals: 18, symbol: lookupSymbol ("eth"))]))

            // BRD
        }
    }

    /// ...

    /// ETH
    public struct ETH {
        public typealias Balance = (wid: BREthereumWallet, balance: String, rid: Int32)
    }

    /// Balance As ETH
    public func getBalanceAsETH (ewm: BREthereumEWM,
                                 wid: BREthereumWallet,
                                 address: String,
                                 rid: Int32,
                                 completion: (ETH.Balance) -> Void) {

        let balance = nil == ewmWalletGetToken(ewm, wid)
            ?  "200000000000000000" // 0.2 ETH
            : "3000000000000000000" // 3.0 TOK
        completion ((wid: wid, balance: balance, rid: rid))
    }

    ///

    init () {}

    internal let currencySymbols = ["btc":"b", "eth":"Ξ"]
    internal func lookupSymbol (_ code: String) -> String {
        return currencySymbols[code] ?? code
    }
}
