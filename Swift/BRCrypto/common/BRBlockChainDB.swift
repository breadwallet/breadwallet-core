//
//  BRBlockChainDB.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import Foundation // DispatchQueue

import BRCore
import BRCore.Ethereum


public class BlockChainDB {

    // TODO: proper error cases
    public enum QueryError: Error {
        case NetworkUnavailable
        case RequestFormat  // sent a mistake
        case Model          // reply w/ an error (missing label, can't parse)
    }

    // MARK: DB Models

    public struct Model {

        public typealias BlockchainID = Identifier<Blockchain>
        public typealias CurrencyCode = Identifier<Currency>
        public typealias CurrencyID = Identifier<Currency>
        public typealias TransactionID = Identifier<Transaction>
        public typealias TransferID = Identifier<Transfer>
        public typealias WalletID = Identifier<Wallet>

        public struct Blockchain: Codable, Identifiable {
            public let id: BlockchainID
            let name: String
            let currencyId: CurrencyID
            let networkName: String
            let isMainnet: Bool
            let blockHeight: UInt64
            let feeEstimates: [FeeEstimate]

            enum CodingKeys: String, CodingKey {
                case id
                case name
                case currencyId = "native_currency_id"
                case networkName = "network"
                case isMainnet = "is_mainnet"
                case blockHeight = "block_height"
                case feeEstimates = "fee_estimates"
            }
        }

        public struct FeeEstimate: Codable {
            let estimatedConfirmationBlocks: UInt64
            let fee: Fee

            enum CodingKeys: String, CodingKey {
                case estimatedConfirmationBlocks = "estimated_confirmation_in"
                case fee
            }
        }

        public struct Fee: Codable {
            let amount: UInt64 // TODO: UInt256 Codable -- assume base unit always used here?
            let currencyCode: CurrencyCode

            enum CodingKeys: String, CodingKey {
                case currencyCode = "currency_id"
                case amount
            }
        }

        public struct Currency: Codable, Identifiable {
            enum TokenType: String, Codable {
                case unknown
                case native
                case erc20
                case omni
            }

            public let id: CurrencyID
            let blockchainId: BlockchainID
            let code: String
            let name: String
            let type: TokenType
            let denominations: [CurrencyDenomination]
            let address: String?
            // initial_supply -- not needed?
            // total_supply -- not needed?

            enum CodingKeys: String, CodingKey {
                case id = "currency_id"
                case blockchainId = "blockchain_id"
                case code
                case name
                case type
                case denominations
                case address
            }
        }

        public struct CurrencyDenomination: Codable {
            let decimals: UInt8
            let name: String
            let symbol: String

            enum CodingKeys: String, CodingKey {
                case decimals
                case name
                case symbol = "short_name"
            }
        }

        public struct Block: Codable {
            // TODO
        }

        public struct Subscription: Codable {
            // TODO
        }

        public struct Transaction: Codable, Identifiable {
            enum Status: String, Codable {
                case unknown
                case submitted
                case feeTooLow = "fee_too_low"
                case failed
                case confirmed
            }

            public let id: TransactionID // The partition identifier for a transaction uses a compound key
            let transactionId: String // Hex-encoded transaction ID (can differ from the hash)
            let blockchainId: BlockchainID
            let blockHash: String
            let blockHeight: UInt64
            let confirmations: UInt64
            let fee: Fee
            let firstSeen: Date
            let hash: String
            let size: UInt64 // Size of transaction in bytes
            let index: UInt64? // Index of this transaction within its block
            let raw: String
            let status: Status
            let timestamp: Date
            let transfers: [Transfer]
            let acknowledgements: UInt64

            enum CodingKeys: String, CodingKey {
                case id = "identifier"
                case transactionId = "transaction_id"
                case blockchainId = "blockchain_id"
                case blockHash = "block_hash"
                case blockHeight = "block_height"
                case confirmations
                case fee
                case firstSeen = "first_seen"
                case hash
                case size
                case index
                case raw
                case status
                case timestamp
                case transfers
                case acknowledgements
            }
        }

        public struct Transfer: Codable, Identifiable {
            public let id: TransferID
            let transactionId: TransactionID
            let blockchainId: BlockchainID
            let amount: Amount
            let index: UInt64?
            let fromAddress: String
            let toAddress: String
            let acknowledgements: UInt64

            enum CodingKeys: String, CodingKey {
                case id = "transfer_id"
                case transactionId = "transaction_id"
                case blockchainId = "blockchain_id"
                case amount
                case index
                case fromAddress = "from_address"
                case toAddress = "to_address"
                case acknowledgements
            }
        }

        public struct Wallet: Codable, Identifiable {
            public let id: WalletID
            let currencies: [WalletCurrency]
            let created: Date
            let updated: Date

            enum CodingKeys: String, CodingKey {
                case id = "wallet_id"
                case currencies
                case created
                case updated
            }
        }

        public struct WalletCurrency: Codable {
            let currencyId: CurrencyID
            let addresses: [String]

            enum CodingKeys: String, CodingKey {
                case currencyId = "currency_id"
                case addresses
            }
        }

        public struct Amount: Codable {
            let amount: UInt256
            let currencyId: CurrencyID

            enum CodingKeys: String, CodingKey {
                case amount
                case currencyId = "currency_id"
            }
        }
    }

    // MARK: Requests

    public struct Request {

        // MARK: Blockchains

        public struct GetBlockchains: APIRequest {
            public typealias Response = [Model.Blockchain]
            public var path: String { return "/blockchains" }

            private(set) var testnet: Bool? = nil
        }

        public struct GetBlockchain: APIRequest {
            public typealias Response = Model.Blockchain
            public var path: String { return "/blockchain/\(id)" }
            public var queryItems: [URLQueryItem]? { return nil }

            let id: Model.BlockchainID
        }

        // MARK: Blocks

        struct GetBlocks: APIRequest {
            // TODO: paged requets/responses
            typealias Response = Model.Block
            var path: String { return "/blocks" }
            //let queryItems: [URLQueryItem]?

            let blockchainId: Model.BlockchainID?
            let startHeight: Int64?
            let endHeight: Int64?

            enum CodingKeys: String, CodingKey {
                case blockchainId = "blockchain_id"
                case startHeight = "start_height"
                case endHeight = "end_height"
            }
        }

        struct GetBlock: APIRequest {
            typealias Response = Model.Block
            var path: String { return "/block/\(blockId)" }
            var queryItems: [URLQueryItem]? { return nil }
            let blockId: String // TODO: typed identifiers
        }

        // MARK: Currencies

        public struct GetCurrencies: APIRequest {
            public typealias Response = [Model.Currency]
            public var path: String { return "/currencies" }

            let blockchainId: Model.BlockchainID?
            let match: String?

            enum CodingKeys: String, CodingKey {
                case blockchainId = "blockchain_id"
                case match
            }
        }

        public struct GetCurrency: APIRequest {
            public typealias Response = Model.Currency
            public var path: String { return "/currencies/\(id)" }
            public var queryItems: [URLQueryItem]? { return nil }

            let id: Model.CurrencyID
        }

        // MARK: Subscriptions
        // TODO

        // MARK: Transactions

        public struct GetTransactions: APIRequest {
            // TODO: paged requets/responses
            public typealias Response = [Model.Transaction]
            public var path: String { return "/transactions" }

            let address: String?
            let blockchainId: Model.BlockchainID?
            let startHeight: Int64?
            let endHeight: Int64?
            let includeProof: Bool?
            let includeRaw: Bool?

            enum CodingKeys: String, CodingKey {
                case address
                case blockchainId = "blockchain_id"
                case startHeight = "start_height"
                case endHeight = "end_height"
                case includeProof = "include_proof"
                case includeRaw = "include_raw"
            }
        }

        public struct GetTransaction: APIRequest {
            public typealias Response = Model.Transaction
            public var path: String { return "/transactions/\(id)" }
            public var queryItems: [URLQueryItem]? { return nil }

            let id: Model.TransactionID
        }

        // MARK: Transfers

        public struct GetTransfers: APIRequest {
            public typealias Response = [Model.Transfer]
            public var path: String { return "/transfers" }

            let addresses: [String]
            let blockchainId: Model.BlockchainID?
            let walletId: Model.WalletID?

            enum CodingKeys: String, CodingKey {
                case addresses = "address"
                case blockchainId = "blockchain_id"
                case walletId = "wallet_id"
            }
        }

        public struct GetTransfer: APIRequest {
            public typealias Response = Model.Transfer
            public var path: String { return "/transfers/\(id)" }
            public var queryItems: [URLQueryItem]? { return nil }

            let id: Model.TransferID
        }

        // MARK: Wallets

        public struct CreateWallet: APIRequest {
            public typealias Response = String
            public var method: HTTPMethod { return .post }
            public var path: String { return "/wallets" }
            public var queryItems: [URLQueryItem]? { return nil }
            public var body: Data? {
                return try? JSONEncoder().encode(self)
            }

            let wallet: Model.Wallet
        }

        public struct GetWallet: APIRequest {
            public typealias Response = Model.Wallet
            public var path: String { return "/wallets/\(id)" }
            public var queryItems: [URLQueryItem]? { return nil }

            let id: Model.WalletID
        }
    }

    // MARK: - Properties

    let queue = DispatchQueue.init(label: "BlockChainDB")
    let dispatcher: RequestDispatcher

    // MARK -

//    public func getBlockchains (mainnet: Bool = true, completion: @escaping ResultCallback<Request.GetBlockchains.Response>) {
//        let req = Request.GetBlockchains(testnet: !mainnet) // TODO: cleanup
//        queue.async {
//            self.dispatcher.dispatch(req, completion: completion)
//        }
//    }
//
//    public func getCurrencies (blockchainID: Model.BlockchainID? = nil, completion: @escaping ResultCallback<Request.GetCurrencies.Response>) {
//        let req = Request.GetCurrencies(blockchainId: blockchainID, match: nil)
//        queue.async {
//            self.dispatcher.dispatch(req, completion: completion)
//        }
//    }

    public func dispatch<Request: APIRequest>(_ request: Request, completion: @escaping ResultCallback<Request.Response>) {
        queue.async {
            self.dispatcher.dispatch(request, completion: completion)
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

    public init (dispatcher: RequestDispatcher) {
        self.dispatcher = dispatcher
    }

    internal let currencySymbols = ["btc":"₿", "eth":"Ξ"]
    internal func lookupSymbol (_ code: String) -> String {
        return currencySymbols[code] ?? code
    }

    static internal let addressBRDTestnet = "0x7108ca7c4718efa810457f228305c9c71390931a" // testnet
    static internal let addressBRDMainnet = "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6" // mainnet
}

// MARK: - Mocks

extension BlockChainDB {
    struct MockDispatcher: RequestDispatcher {
        var baseURL: URL { return URL(string: "https://blockchain-db.us-east-1.elasticbeanstalk.com")! }

        func dispatch<Request: APIRequest>(_ request: Request, completion: ResultCallback<Request.Response>) {
            switch request {
            case let request as BlockChainDB.Request.GetBlockchains:
                var response: [BlockChainDB.Model.Blockchain] = []

                if request.testnet ?? false { // testnet or nil
                    response.append(BlockChainDB.Model.Blockchain(id: "bitcoin-testnet",
                                                                  name: "Bitcoin",
                                                                  currencyId: "btc",
                                                                  networkName: "testnet",
                                                                  isMainnet: true,
                                                                  blockHeight: 900000,
                                                                  feeEstimates: []))

                    response.append(BlockChainDB.Model.Blockchain(id: "bitcash-testnet",
                                                                  name: "Bitcoin Cash",
                                                                  currencyId: "bch",
                                                                  networkName: "testnet",
                                                                  isMainnet: true,
                                                                  blockHeight: 1200000,
                                                                  feeEstimates: []))

                    response.append(BlockChainDB.Model.Blockchain(id: "ethereum-testnet",
                                                                  name: "Ethereum",
                                                                  currencyId: "eth",
                                                                  networkName: "ropsten",
                                                                  isMainnet: true,
                                                                  blockHeight: 1000000,
                                                                  feeEstimates: []))

                    response.append(BlockChainDB.Model.Blockchain(id: "ethereum-rinkeby",
                                                                  name: "Ethereum",
                                                                  currencyId: "eth",
                                                                  networkName: "rinkeby",
                                                                  isMainnet: true,
                                                                  blockHeight: 2000000,
                                                                  feeEstimates: []))
                } else  { // mainnet
                    response.append(BlockChainDB.Model.Blockchain(id: "bitcoin-mainnet",
                                                                  name: "Bitcoin",
                                                                  currencyId: "btc",
                                                                  networkName: "mainnet",
                                                                  isMainnet: true,
                                                                  blockHeight: 600000,
                                                                  feeEstimates: []))

                    response.append(BlockChainDB.Model.Blockchain(id: "bitcash-mainnet",
                                                                  name: "Bitcoin Cash",
                                                                  currencyId: "bch",
                                                                  networkName: "mainnet",
                                                                  isMainnet: true,
                                                                  blockHeight: 1000000,
                                                                  feeEstimates: []))

                    response.append(BlockChainDB.Model.Blockchain(id: "ethereum-mainnet",
                                                                  name: "Ethereum",
                                                                  currencyId: "eth",
                                                                  networkName: "mainnet",
                                                                  isMainnet: true,
                                                                  blockHeight: 8000000,
                                                                  feeEstimates: []))
                }

                guard let value = response as? Request.Response else { return completion(.failure(BlockChainDB.QueryError.Model)) }
                completion(.success(value))

            case let request as BlockChainDB.Request.GetCurrencies:
                var response: [Model.Currency] = []

                if request.blockchainId?.description.starts(with: "bitcoin") ?? true {
                    // BTC
                    response.append(Model.Currency(id: "btc",
                                                   blockchainId: "bitcoin-mainnet",
                                                   code: "btc",
                                                   name: "Bitcoin",
                                                   type: .native,
                                                   denominations: [
                                                    Model.CurrencyDenomination(decimals: 0, name: "satoshi", symbol: "sat"),
                                                    Model.CurrencyDenomination(decimals: 8, name: "bitcoin", symbol: "btc")],
                                                   address: nil))
                    // testnet?
                }

                if request.blockchainId?.description.starts(with: "bitcash") ?? true {
                    // BCH
                    response.append(Model.Currency(id: "bch",
                                                   blockchainId: "bitcoin-mainnet",
                                                   code: "bch",
                                                   name: "Bitcoin Cash",
                                                   type: .native,
                                                   denominations: [
                                                    Model.CurrencyDenomination(decimals: 0, name: "satoshi", symbol: "sat"),
                                                    Model.CurrencyDenomination(decimals: 8, name: "bitcash", symbol: "bch")],
                                                   address: nil))
                    // testnet??
                }

                if request.blockchainId?.description.starts(with: "ethereum") ?? true {
                    // ETH
                    response.append(Model.Currency(id: "eth",
                                                   blockchainId: "ethereum-mainnet",
                                                   code: "eth",
                                                   name: "Ethereum",
                                                   type: .native,
                                                   denominations: [
                                                    Model.CurrencyDenomination(decimals: 0, name: "wei", symbol: "wei"),
                                                    Model.CurrencyDenomination(decimals: 9, name: "gwei", symbol: "gwei"),
                                                    Model.CurrencyDenomination(decimals: 18, name: "ether", symbol: "eth")],
                                                   address: nil))

                    response.append(Model.Currency(id: "brd",
                                                   blockchainId: "ethereum-mainnet",
                                                   code: "BRD",
                                                   name: "BRD Token",
                                                   type: .erc20,
                                                   denominations: [
                                                    Model.CurrencyDenomination(decimals: 0, name: "BRDI", symbol: "BRDI"),
                                                    Model.CurrencyDenomination(decimals: 18, name: "BRD", symbol: "BRD")],
                                                   address: nil))
                    // ropsten, rinkeby?
                }

                guard let value = response as? Request.Response else { return completion(.failure(BlockChainDB.QueryError.Model)) }
                completion(.success(value))

            default:
                completion(.failure(BlockChainDB.QueryError.RequestFormat))
            }
        }
    }
}
