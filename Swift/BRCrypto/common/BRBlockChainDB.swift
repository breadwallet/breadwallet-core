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

    let dbBaseURL: String
    let ethBaseURL: String?

    // For `session.dataTask()`
    let session = URLSession (configuration: .default)

    // Temporary - adding asynchronicity to ETh requests.
    let queue = DispatchQueue.init(label: "BlockChainDB")

    public enum QueryError: Error {
        // HTTP URL build failed
        case url (String)

        // HTTP submission error
        case submission (Error)

        // HTTP submission didn't error but returned no data
        case noData

        // JSON parse failed, generically
        case jsonParse (Error?)

        // Could not convert JSON -> T
        case model (String)

        // JSON entity expected but not provided - e.g. requested a 'transferId' that doesn't exist.
        case noEntity (id: String?)
    }

    ///
    ///
    ///
    
    public struct Model {

        /// Blockchain

        public typealias Blockchain = (id: String, name: String, network: String, isMainnet: Bool, currency: String, blockHeight: UInt64 /* fee Estimate */)

        static internal func asBlockchain (json: JSON) -> Model.Blockchain? {
            guard let id = json.asString (name: "id"),
                let name = json.asString (name: "name"),
                let network = json.asString (name: "network"),
                let isMainnet = json.asBool (name: "is_mainnet"),
                let currency = json.asString (name: "native_currency_id"),
                let blockHeight = json.asUInt64 (name: "block_height")
                else { return nil }

            return (id: id, name: name, network: network, isMainnet: isMainnet, currency: currency, blockHeight: blockHeight)
        }

        static public let defaultBlockchains: [Blockchain] = [
            // Mainnet
 //           (id: "bitcoin-mainnet",  name: "Bitcoin",  network: "mainnet", isMainnet: true,  currency: "btc", blockHeight:  600000),
            (id: "bitcash-mainnet",  name: "Bitcash",  network: "mainnet", isMainnet: true,  currency: "bch", blockHeight: 1000000),
            (id: "ethereum-mainnet", name: "Ethereum", network: "mainnet", isMainnet: true,  currency: "eth", blockHeight: 8000000),

            (id: "bitcoin-testnet",  name: "Bitcoin",  network: "testnet", isMainnet: false, currency: "btc", blockHeight:  900000),
            (id: "bitcash-testnet",  name: "Bitcash",  network: "testnet", isMainnet: false, currency: "bch", blockHeight: 1200000),
            (id: "ethereum-testnet", name: "Ethereum", network: "testnet", isMainnet: false, currency: "eth", blockHeight: 1000000),
            (id: "ethereum-rinkeby", name: "Ethereum", network: "rinkeby", isMainnet: false, currency: "eth", blockHeight: 2000000)
        ]

        /// Currency & CurrencyDenomination

        public typealias CurrencyDenomination = (name: String, code: String, decimals: UInt8, symbol: String /* extra */)
        public typealias Currency = (id: String, name: String, code: String, type: String, blockchainID: String, address: String?, demoninations: [CurrencyDenomination])

        static internal func asCurrencyDenomination (json: JSON) -> Model.CurrencyDenomination? {
            guard let name = json.asString (name: "name"),
                let code = json.asString (name: "short_name"),
                let decimals = json.asUInt8 (name: "decimals")
                // let symbol = json.asString (name: "symbol")
                else { return nil }

            let symbol = lookupSymbol (code)

            return (name: name, code: code, decimals: decimals, symbol: symbol)
        }

        static internal func asCurrency (json: JSON) -> Model.Currency? {
            guard // let id = json.asString (name: "id"),
                let name = json.asString (name: "name"),
                let code = json.asString (name: "code"),
                let type = json.asString (name: "type"),
                let bid  = json.asString (name: "blockchain_id")
                else { return nil }

            let id = name

            // Address is optional
            let address = json.asString(name: "address")

            // All denomincations must parse
            guard let demoninations = json.asArray (name: "denominations")?
                .map ({ JSON (dict: $0 )})
                .map ({ asCurrencyDenomination(json: $0)})
                else { return nil }
            
            return demoninations.contains (where: { nil == $0 })
                ? nil
                : (id: id, name: name, code: code, type: type, blockchainID: bid, address: address, demoninations: (demoninations as! [CurrencyDenomination]))
        }

        static public let defaultCurrencies: [Currency] = [
            (id: "Bitcoin", name: "Bitcoin", code: "btc", type: "native", blockchainID: "bitcoin-mainnet", address: nil,
             demoninations: [(name: "satoshi", code: "sat", decimals: 0, symbol: lookupSymbol ("sat")),
                             (name: "bitcoin", code: "btc", decimals: 8, symbol: lookupSymbol ("btc"))]),

            (id: "Bitcash", name: "Bitcash", code: "bch", type: "native", blockchainID: "bitcash-mainnet", address: nil,
             demoninations: [(name: "satoshi", code: "sat", decimals: 0, symbol: lookupSymbol ("sat")),
                             (name: "bitcoin", code: "bch", decimals: 8, symbol: lookupSymbol ("bch"))]),


            (id: "Ethereum", name: "Ethereum", code: "eth", type: "native", blockchainID: "ethereum-mainnet", address: nil,
             demoninations: [(name: "wei",   code: "wei",  decimals:  0, symbol: lookupSymbol ("wei")),
                             (name: "gwei",  code: "gwei", decimals:  9, symbol: lookupSymbol ("gwei")),
                             (name: "ether", code: "eth",  decimals: 18, symbol: lookupSymbol ("eth"))]),


            (id: "BRD Token", name: "BRD Token", code: "BRD", type: "erc20", blockchainID: "ethereum-mainnet", address: BlockChainDB.addressBRDMainnet,
             demoninations: [(name: "BRD_INTEGER",   code: "BRDI",  decimals:  0, symbol: "brdi"),
                             (name: "BRD",           code: "BRD",   decimals: 18, symbol: "brd")]),


            (id: "EOS Token", name: "EOS Token", code: "EOS", type: "erc20", blockchainID: "ethereum-mainnet", address: "0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0",
             demoninations: [(name: "EOS_INTEGER",   code: "EOSI",  decimals:  0, symbol: "eosi"),
                             (name: "EOS",           code: "EOS",   decimals: 18, symbol: "eos")])
        ]

        /// Transfer

        public typealias Transfer = (id: String, source: String?, target: String?, amountValue: String, amountCurrency: String, acknowledgements: UInt64, index: UInt64, transactionId: String?, blockchainId: String)

        static internal func asTransfer (json: JSON) -> Model.Transfer? {
            guard let id = json.asString(name: "transfer_id"),
                let bid = json.asString(name: "blockchain_id"),
                let acknowledgements = json.asUInt64(name: "acknowledgements"),
                let index = json.asUInt64(name: "index"),
                let amount = json.asDict(name: "amount").map ({ JSON (dict: $0) }),
                let amountValue = amount.asString (name: "amount"),
                let amountCurrency = amount.asString (name: "currency_id")
                else { return nil }

            let source = json.asString(name: "from_address")
            let target = json.asString(name: "to_address")
            let tid = json.asString(name: "transaction_id")

            return (id: id, source: source, target: target,
                    amountValue: amountValue, amountCurrency: amountCurrency,
                    acknowledgements: acknowledgements, index: index,
                    transactionId: tid, blockchainId: bid)
        }

        /// Transaction

        public typealias Transaction = (id: String, blockchainId: String,
            hash: String, identifier: String,
            blockHash: String?, blockHeight: UInt64?, index: UInt64?, confirmations: UInt64?, status: String,
            size: UInt64, timestamp: Date?, firstSeen: Date,
            raw: Data?,
            transfers: [Transfer],
            acknowledgements: UInt64
        )

        static internal func asTransaction (json: JSON) -> Model.Transaction? {
            guard let id = json.asString(name: "transaction_id"),
                let bid        = json.asString (name: "blockchain_id"),
                let hash       = json.asString (name: "hash"),
                let identifier = json.asString (name: "identifier"),
                let status     = json.asString (name: "status"),
                let size       = json.asUInt64 (name: "size"),
                let firstSeen  = json.asDate   (name: "first_seen"),
                let acks       = json.asUInt64 (name: "acknowledgements")
                else { return nil }

            let blockHash     = json.asString (name: "block_hash")
            let blockHeight   = json.asUInt64 (name: "block_height")
            let index         = json.asUInt64 (name: "index")
            let confirmations = json.asUInt64 (name: "confirmations")
            let timestamp     = json.asDate   (name: "timestamp")

            let raw = json.asData (name: "raw")

            guard let transfers = json.asArray (name: "transfers")?
                .map ({ JSON (dict: $0 )})
                .map ({ asTransfer (json: $0)})
                else { return nil }

            return (id: id, blockchainId: bid,
                     hash: hash, identifier: identifier,
                     blockHash: blockHash, blockHeight: blockHeight, index: index, confirmations: confirmations, status: status,
                     size: size, timestamp: timestamp, firstSeen: firstSeen,
                     raw: raw,
                     transfers: (transfers as! [Transfer]),
                     acknowledgements: acks)
        }

        /// Block

        public typealias Block = (id: String, blockchainId: String,
            hash: String, height: UInt64, header: String?, raw: Data?, mined: Date, size: UInt64,
            prevHash: String?, nextHash: String?, // fees
            transactions: [Transaction]?,
            acknowledgements: UInt64
        )

        static internal func asBlock (json: JSON) -> Model.Block? {
            guard let id = json.asString(name: "block_id"),
                let bid      = json.asString(name: "blockchain_id"),
                let hash     = json.asString (name: "hash"),
                let height   = json.asUInt64 (name: "height"),
                let mined    = json.asDate   (name: "mined"),
                let size     = json.asUInt64 (name: "size"),
                let acks       = json.asUInt64 (name: "acknowledgements")
                else { return nil }

            let header   = json.asString (name: "header")
            let raw      = json.asData   (name: "raw")
            let prevHash = json.asString (name: "prev_hash")
            let nextHash = json.asString (name: "next_hash")

            let transactions = json.asArray (name: "transactions")?
                .map ({ JSON (dict: $0 )})
                .map ({ asTransaction (json: $0)}) as? [Model.Transaction]  // no quite - i

            return (id: id, blockchainId: bid,
                    hash: hash, height: height, header: header, raw: raw, mined: mined, size: size,
                    prevHash: prevHash, nextHash: nextHash,
                    transactions: transactions,
                    acknowledgements: acks)
        }
    }

    public func getBlockchains (mainnet: Bool? = nil, completion: @escaping (Result<[Model.Blockchain],QueryError>) -> Void) {
        dbMakeRequest (path: "blockchains", query: mainnet.map { zip (["testnet"], [($0 ? "false" : "true")]) }) {
            (more: Bool, res: Result<[JSON], BlockChainDB.QueryError>) in
            precondition (!more)
            // How to handle an error?
            //    a) ignore it and return `defaultBlockchain`?
            //    b) pass it along, let caller use `defaultBlockchains`
            // We'll choose 'b'

            // Map a successful result and convert JSON -> Model.Blockchain.
            completion (res.flatMap {
                BlockChainDB.getManyExpected(data: $0, transform: Model.asBlockchain)
            })
        }
    }

    public func getBlockchain (blockchainId: String, completion: @escaping (Result<Model.Blockchain,QueryError>) -> Void) {
        dbMakeRequest(path: "blockchains/\(blockchainId)", query: nil, embedded: false) {
            (more: Bool, res: Result<[BlockChainDB.JSON], BlockChainDB.QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getOneExpected (id: blockchainId, data: $0, transform: Model.asBlockchain)
            })
        }
    }

    public func getCurrencies (blockchainId: String? = nil, completion: @escaping (Result<[Model.Currency],QueryError>) -> Void) {
        dbMakeRequest (path: "currencies", query: blockchainId.map { zip(["blockchain_id"], [$0]) }) {
            (more: Bool, res: Result<[BlockChainDB.JSON], BlockChainDB.QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getManyExpected(data: $0, transform: Model.asCurrency)
            })
        }
    }

    public func getCurrency (currencyId: String, completion: @escaping (Result<Model.Currency,QueryError>) -> Void) {
        dbMakeRequest (path: "currencies/\(currencyId)", query: nil, embedded: false) {
            (more: Bool, res: Result<[BlockChainDB.JSON], BlockChainDB.QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getOneExpected(id: currencyId, data: $0, transform: Model.asCurrency)
            })
        }
    }

    public func getTransfers (blockchainId: String, addresses: [String], completion: @escaping (Result<[Model.Transfer], QueryError>) -> Void) {
        let queryKeys = ["blockchain_id"] + Array (repeating: "address", count: addresses.count)
        let queryVals = [blockchainId]    + addresses

        dbMakeRequest (path: "transfers", query: zip (queryKeys, queryVals)) {
            (more: Bool, res: Result<[BlockChainDB.JSON], BlockChainDB.QueryError>) in
            completion (res.flatMap {
                BlockChainDB.getManyExpected (data: $0, transform: Model.asTransfer)
            })
        }
    }

    public func getTransfer (transferId: String, completion: @escaping (Result<Model.Transfer, QueryError>) -> Void) {
        dbMakeRequest (path: "transfers/\(transferId)", query: nil, embedded: false) {
            (more: Bool, res: Result<[BlockChainDB.JSON], BlockChainDB.QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getOneExpected (id: transferId, data: $0, transform: Model.asTransfer)
            })
        }
    }

    // Transactions

    public func getTransactions (blockchainId: String,
                                 addresses: [String],
                                 begBlockNumber: UInt64 = 0,
                                 endBlockNumber: UInt64 = 0,
                                 includeRaw: Bool = false,
                                 includeProof: Bool = false,
                                 completion: @escaping (Result<[Model.Transaction], QueryError>) -> Void) {
        //
        // This query could overrun the endpoint's page size (typically 5,000).  If so, we'll need
        // to repeat the request for the next batch.
        //
        self.queue.async {
            let semaphore = DispatchSemaphore (value: 0)

            var moreResults = false
            var begBlockNumber = begBlockNumber

            var error: QueryError? = nil
            var results = [Model.Transaction]()

            repeat {
                let queryKeys = ["blockchain_id", "start_height", "end_height",
                                 "include_proof", "include_raw"]
                    + Array (repeating: "address", count: addresses.count)

                let queryVals = [blockchainId, begBlockNumber.description, endBlockNumber.description,
                                 includeProof.description, includeRaw.description]
                    + addresses

                moreResults = false

                self.dbMakeRequest (path: "transactions", query: zip (queryKeys, queryVals)) {
                    (more: Bool, res: Result<[BlockChainDB.JSON], BlockChainDB.QueryError>) in
                    // Flag if `more`
                    moreResults = more

                    // Append `transactions` with the resulting transactions.
                    results += try! res
                        .flatMap { BlockChainDB.getManyExpected(data: $0, transform: Model.asTransaction) }
                        .recover { error = $0; return [] }.get()

                    if more && nil == error {
                        begBlockNumber = results.reduce(0) {
                            max ($0, ($1.blockHeight ?? 0))
                        }
                    }

                    semaphore.signal()
                }

                semaphore.wait()
            } while moreResults && nil == error

            completion (nil == error
                ? Result.success (results)
                : Result.failure (error!))
        }
    }

    public func getTransaction (transactionId: String,
                                includeRaw: Bool = false,
                                includeProof: Bool = false,
                                completion: @escaping (Result<Model.Transaction, QueryError>) -> Void) {
        let queryKeys = ["include_proof", "include_raw"]
        let queryVals = [includeProof.description, includeRaw.description]

        dbMakeRequest (path: "transactions/\(transactionId)", query: zip (queryKeys, queryVals), embedded: false) {
            (more: Bool, res: Result<[BlockChainDB.JSON], BlockChainDB.QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getOneExpected (id: transactionId, data: $0, transform: Model.asTransaction)
            })
        }
    }

    // Blocks

    public func getBlocks (blockchainId: String,
                           begBlockNumber: UInt64 = 0,
                           endBlockNumber: UInt64 = 0,
                           includeRaw: Bool = false,
                           includeTx: Bool = false,
                           includeTxRaw: Bool = false,
                           includeTxProof: Bool = false,
                           completion: @escaping (Result<[Model.Block], QueryError>) -> Void) {
        self.queue.async {
            let semaphore = DispatchSemaphore (value: 0)

            var moreResults = false
            var begBlockNumber = begBlockNumber

            var error: QueryError? = nil
            var results = [Model.Block]()

            repeat {
                let queryKeys = ["blockchain_id", "start_height", "end_height",  "include_raw",
                                 "include_tx", "include_tx_raw", "include_tx_proof"]

                let queryVals = [blockchainId, begBlockNumber.description, endBlockNumber.description, includeRaw.description,
                                 includeTx.description, includeTxRaw.description, includeTxProof.description]

                self.dbMakeRequest (path: "blocks", query: zip (queryKeys, queryVals)) {
                    (more: Bool, res: Result<[BlockChainDB.JSON], BlockChainDB.QueryError>) in

                    // Flag if `more`
                    moreResults = more

                    // Append `transactions` with the resulting transactions.  Be sure
                    results += try! res
                        .flatMap { BlockChainDB.getManyExpected(data: $0, transform: Model.asBlock) }
                        .recover { error = $0; return [] }.get()

                    if more && nil == error {
                        begBlockNumber = results.reduce(0) {
                            max ($0, $1.height)
                        }
                    }

                    semaphore.signal()
                }

                semaphore.wait()
            } while moreResults && nil == error

            completion (nil == error
                ? Result.success (results)
                : Result.failure (error!))
        }

    }

    public func getBlock (blockId: String,
                          includeRaw: Bool = false,
                          includeTx: Bool = false,
                          includeTxRaw: Bool = false,
                          includeTxProof: Bool = false,
                          completion: @escaping (Result<Model.Block, QueryError>) -> Void) {
        let queryKeys = ["include_raw", "include_tx", "include_tx_raw", "include_tx_proof"]

        let queryVals = [includeRaw.description, includeTx.description, includeTxRaw.description, includeTxProof.description]

        dbMakeRequest (path: "blocks/\(blockId)", query: zip (queryKeys, queryVals), embedded: false) {
            (more: Bool, res: Result<[BlockChainDB.JSON], BlockChainDB.QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getOneExpected (id: blockId, data: $0, transform: Model.asBlock)
            })
        }
    }

    /// ...

    /// BTC
    public struct BTC {
        typealias Transaction = (btc: BRCoreTransaction, rid: Int32)

    }

    internal func getBlockNumberAsBTC (bwm: BRWalletManager,
                                       blockchainId: String,
                                       rid: Int32,
                                       done: @escaping (UInt64, Int32) -> Void) {
        getBlockchain (blockchainId: blockchainId) { (res: Result<BlockChainDB.Model.Blockchain, BlockChainDB.QueryError>) in
            switch res {
            case let .success (blockchain):
                done (blockchain.blockHeight, rid)
            case .failure(_):
                done (0, rid)  // No
            }
        }
    }

    internal func getTransactionsAsBTC (bwm: BRWalletManager,
                                        blockchainId: String,
                                        addresses: [String],
                                        begBlockNumber: UInt64,
                                        endBlockNumber: UInt64,
                                        rid: Int32,
                                        done: @escaping (Bool, Int32) -> Void,
                                        each: @escaping (BTC.Transaction) -> Void) {
        getTransactions (blockchainId: blockchainId,
                         addresses: addresses,
                         begBlockNumber: begBlockNumber,
                         endBlockNumber: endBlockNumber,
                         includeRaw: true) { (res: Result<[BlockChainDB.Model.Transaction], BlockChainDB.QueryError>) in
                            let btcRes = res
                                .flatMap { (dbTransactions: [BlockChainDB.Model.Transaction]) -> Result<[BRCoreTransaction], BlockChainDB.QueryError> in
                                    let transactions:[BRCoreTransaction?] = dbTransactions
                                        .map {
                                            guard let raw = $0.raw
                                                else { return nil }

                                            let bytes = [UInt8] (raw)
                                            guard let btcTransaction = BRTransactionParse (bytes, bytes.count)
                                                else { return nil }

                                            btcTransaction.pointee.timestamp =
                                                $0.timestamp.map { UInt32 ($0.timeIntervalSince1970) } ?? UInt32 (0)
                                            btcTransaction.pointee.blockHeight =
                                                $0.blockHeight.map { UInt32 ($0) } ?? 0

                                            return  btcTransaction
                                    }

                                    return transactions.contains(where: { nil == $0 })
                                        ? Result.failure (QueryError.model ("BRCoreTransaction parse error"))
                                        : Result.success (transactions as! [BRCoreTransaction])
                                }

                            switch btcRes {
                            case .failure: done (false, rid)
                            case let .success (btc):
                                btc.forEach { each ((btc: $0, rid: rid)) }
                                done (true, rid)
                            }
        }
    }

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

        static internal func asTransaction (json: JSON, rid: Int32) -> ETH.Transaction? {
            guard let hash = json.asString(name: "hash"),
                let sourceAddr = json.asString(name: "from"),
                let targetAddr = json.asString(name: "to"),
                let contractAddr = json.asString(name: "contractAddress"),
                let amount     = json.asString(name: "value"),
                let gasLimit   = json.asString(name: "gas"),
                let gasPrice   = json.asString(name: "gasPrice"),
                let data       = json.asString(name: "input"),
                let nonce      = json.asString(name: "nonce"),
                let gasUsed    = json.asString(name: "gasUsed"),
                let blockNumber    = json.asString(name: "blockNumber"),
                let blockHash      = json.asString(name: "blockHash"),
                let blockConfirmations    = json.asString(name: "confirmations"),
                let blockTransactionIndex = json.asString(name: "transactionIndex"),
                let blockTimestamp        = json.asString(name: "timeStamp"),
                let isError    = json.asString(name: "isError")
                else { return nil }

            return (hash: hash,
                    sourceAddr: sourceAddr, targetAddr: targetAddr, contractAddr: contractAddr,
                    amount: amount, gasLimit: gasLimit, gasPrice: gasPrice,
                    data: data, nonce: nonce, gasUsed: gasUsed,
                    blockNumber: blockNumber, blockHash: blockHash,
                    blockConfirmations: blockConfirmations, blockTransactionIndex: blockTransactionIndex, blockTimestamp: blockTimestamp,
                    isError: isError,
                    rid: rid)
        }

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

        static internal func dropLastIfEmpty (_ strings: [String]?) -> [String]? {
            return (nil != strings && !strings!.isEmpty && "" == strings!.last!
                ? strings!.dropLast()
                : strings)
        }

        static internal func asLog (json: JSON, rid: Int32) -> ETH.Log? {
            guard let hash = json.asString(name: "transactionHash"),
                let contract = json.asString(name: "address"),
                let topics   = dropLastIfEmpty (json.asStringArray (name: "topics")),
                let data     = json.asString(name: "data"),
                let gasPrice = json.asString(name: "gasPrice"),
                let gasUsed  = json.asString(name: "gasUsed"),
                let logIndex = json.asString(name: "logIndex"),
                let blockNumber = json.asString(name: "blockNumber"),
                let blockTransactionIndex = json.asString(name: "transactionIndex"),
                let blockTimestamp = json.asString(name: "timeStamp")
                else { return nil }

            return (hash: hash, contract: contract, topics: topics, data: data,
                    gasPrice: gasPrice, gasUsed: gasUsed,
                    logIndex: logIndex,
                    blockNumber: blockNumber, blockTransactionIndex: blockTransactionIndex, blockTimestamp: blockTimestamp,
                    rid: rid)
        }

        public typealias Token = (
            address: String,
            symbol: String,
            name: String,
            description: String,
            decimals: UInt32,
            defaultGasLimit: String?,
            defaultGasPrice: String?,
            rid: Int32)

        static internal func asToken (json: JSON, rid: Int32) -> ETH.Token? {
            // {"code":"1ST","name":"FirstBlood","type":"erc20","scale":18,"is_supported":true,
            //  "contract_address":"0xaf30D2a7e90d7dC361C8c4585E9bB7d2F6f15bc7",
            //  "sale_address":"","aliases":[],"colors":["#f15a22","#f15a22"]}
            guard let name   = json.asString(name: "name"),
                let symbol   = json.asString(name: "code"),
                let address  = json.asString(name: "contract_address"),
                let decimals = json.asUInt8(name: "scale")
                else { return nil }

            let description = "Token for '\(symbol)'"

            return (address: address, symbol: symbol, name: name, description: description,
                    decimals: UInt32(decimals),
                    defaultGasLimit: nil,
                    defaultGasPrice: nil,
                    rid: rid)
        }

        public typealias Block = (numbers: [UInt64], rid: Int32)
        public typealias BlockNumber = (number: String, rid: Int32)
        public typealias Nonce = (address: String, nonce: String, rid: Int32)
    }

    public func getBalanceAsETH (ewm: BREthereumEWM,
                                 wid: BREthereumWallet,
                                 address: String,
                                 rid: Int32,
                                 completion: @escaping (ETH.Balance) -> Void) {
        let json: JSON.Dict = [
            "jsonrpc" : "2.0",
            "method"  : "eth_getBalance",
            "params"  : [address, "latest"],
            "id"      : rid
        ]

        ethMakeRequestJSON(ewm: ewm, data: json) { (res: Result<BlockChainDB.JSON, BlockChainDB.QueryError>) in
            let balance = try! res
                .map { $0.asString (name: "result")! }
                .recover { (ignore) in "200000000000000000" }
                .get()

            completion ((wid: wid, balance: balance, rid: rid))
        }
    }

    public func getBalanceAsTOK (ewm: BREthereumEWM,
                                 wid: BREthereumWallet,
                                 address: String,
                                 rid: Int32,
                                 completion: @escaping (ETH.Balance) -> Void) {
        let json: JSON.Dict = [ "id" : rid ]

        let contract = asUTF8String(tokenGetAddress(ewmWalletGetToken(ewm, wid)))

        var queryDict = [
            "module"    : "account",
            "action"    : "tokenbalance",
            "address"   : address,
            "contractaddress" : contract
        ]

        ethMakeRequestQUERY (ewm: ewm,
                             query: zip (Array(queryDict.keys), Array(queryDict.values)),
                             data: json) { (res: Result<BlockChainDB.JSON, BlockChainDB.QueryError>) in
                                let balance = try! res
                                    .map { $0.asString (name: "result")! }
                                    .recover { (ignore) in "0x1" }
                                    .get()

                                completion ((wid: wid, balance: balance, rid: rid))
        }
    }

    public func getGasPriceAsETH (ewm: BREthereumEWM,
                                  wid: BREthereumWallet,
                                  rid: Int32,
                                  completion: @escaping (ETH.GasPrice) -> Void) {
        let json: JSON.Dict = [
            "method" : "eth_gasPrice",
            "params" : [],
            "id" : rid
        ]

        ethMakeRequestJSON(ewm: ewm, data: json) { (res: Result<BlockChainDB.JSON, BlockChainDB.QueryError>) in
            let gasPrice = try! res
                .map { $0.asString (name: "result")! }
                .recover { (ignore) in "0xffc0" }
                .get()

            completion ((wid: wid, gasPrice: gasPrice, rid: rid))
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
        let json: JSON.Dict = [
            "jsonrpc" : "2.0",
            "method"  : "eth_getBalance",
            "params"  : [["from":from, "to":to, "value":amount, "data":data]],
            "id"      : rid
        ]

        ethMakeRequestJSON(ewm: ewm, data: json) { (res: Result<BlockChainDB.JSON, BlockChainDB.QueryError>) in
            let gasEstimate = try! res
                .map { $0.asString (name: "result")! }
                .recover { (ignore) in "92000" }
                .get()

            completion ((wid: wid, tid: tid, gasEstimate: gasEstimate, rid: rid))
        }
    }

    public func submitTransactionAsETH (ewm: BREthereumEWM,
                                        wid: BREthereumWallet,
                                        tid: BREthereumTransfer,
                                        transaction: String,
                                        rid: Int32,
                                        completion: @escaping (ETH.Submit) -> Void) {
        let json: JSON.Dict = [
            "jsonrpc" : "2.0",
            "method"  : "eth_sendRawTransaction",
            "params"  : [transaction],
            "id"      : rid
        ]

        ethMakeRequestJSON(ewm: ewm, data: json) { (res: Result<BlockChainDB.JSON, BlockChainDB.QueryError>) in
            let hash = try! res
                .map { $0.asString (name: "result")! }
                .recover { (ignore) in "0x123abc456def" }
                .get()

            completion ((wid: wid, tid: tid, hash: hash, errorCode: Int32(-1), errorMessage: nil, rid: rid))
        }
    }

    public func getTransactionsAsETH (ewm: BREthereumEWM,
                                      address: String,
                                      begBlockNumber: UInt64,
                                      endBlockNumber: UInt64,
                                      rid: Int32,
                                      done: @escaping (Bool, Int32) -> Void,
                                      each: @escaping (ETH.Transaction) -> Void) {
        let json: JSON.Dict = [
            "account" : address,
            "id"      : rid ]

        var queryDict = [
            "module"    : "account",
            "action"    : "txlist",
            "address"   : address,
            "startBlock": begBlockNumber.description,
            "endBlock"  : endBlockNumber.description
        ]

        ethMakeRequestQUERY (ewm: ewm, query: zip (Array(queryDict.keys), Array(queryDict.values)), data: json) {
            (res: Result<JSON, QueryError>) in

            let resLogs = res
                .flatMap({ (json: BlockChainDB.JSON) -> Result<[ETH.Transaction], QueryError> in
                    guard let _ = json.asString (name: "status"),
                        let   _ = json.asString (name: "message"),
                        let   result  = json.asArray (name:  "result")
                        else { return Result.failure(QueryError.model("Missed {status, message, result")) }

                    let transactions = result.map { ETH.asTransaction (json: JSON (dict: $0), rid: rid) }

                    return transactions.contains(where: { nil == $0 })
                        ? Result.failure (QueryError.model ("ETH.Transaction parse error"))
                        : Result.success (transactions as! [ETH.Transaction])
                })

            switch resLogs {
            case .failure: done (false, rid)
            case let .success(a):
                a.forEach { each ($0) }
                done (true, rid)
            }
        }
    }

    public func getLogsAsETH (ewm: BREthereumEWM,
                              contract: String?,
                              address: String,
                              event: String,
                              begBlockNumber: UInt64,
                              endBlockNumber: UInt64,
                              rid: Int32,
                              done: @escaping (Bool, Int32) -> Void,
                              each: @escaping (ETH.Log) -> Void) {
        let json: JSON.Dict = [ "id" : rid ]

        var queryDict = [
            "module"    : "logs",
            "action"    : "getLogs",
            "fromBlock" : begBlockNumber.description,
            "toBlock"   : endBlockNumber.description,
            "topic0"    : event,
            "topic1"    : address,
            "topic_1_2_opr" : "or",
            "topic2"    : address
        ]
        if nil != contract { queryDict["address"] = contract! }

        ethMakeRequestQUERY (ewm: ewm, query: zip (Array(queryDict.keys), Array(queryDict.values)), data: json) {
            (res: Result<JSON, QueryError>) in

            let resLogs = res
                .flatMap({ (json: JSON) -> Result<[ETH.Log], QueryError> in
                    guard let _ = json.asString (name: "status"),
                        let   _ = json.asString (name: "message"),
                        let   result  = json.asArray (name:  "result")
                        else { return Result.failure(QueryError.model("Missed {status, message, result")) }

                    let logs = result.map { ETH.asLog (json: JSON (dict: $0), rid: rid) }

                    return logs.contains(where: { nil == $0 })
                        ? Result.failure (QueryError.model ("ETH.Log parse error"))
                        : Result.success (logs as! [ETH.Log])
                })

            switch resLogs {
            case .failure: done (false, rid)
            case let .success(a):
                a.forEach { each ($0) }
                done (true, rid)
            }
        }
    }

    public func getTokensAsETH (ewm: BREthereumEWM,
                                rid: Int32,
                                done: @escaping (Bool, Int32) -> Void,
                                each: @escaping (ETH.Token) -> Void) {
        // Everything returned by BRD must/absolutely-must be in BlockChainDB currencies.  Thus,
        // when stubbed, so too must these.
        ethMakeRequestTOKEN (ewm: ewm) { (res: Result<[JSON.Dict], QueryError>) in
            let resTokens = res
                .flatMap({ (jsonArray: [JSON.Dict]) -> Result<[ETH.Token], QueryError> in
                    let tokens = jsonArray.map { ETH.asToken (json: JSON (dict: $0), rid: rid) }

                    return tokens.contains(where: { nil == $0 })
                        ? Result.failure (QueryError.model ("ETH.Tokens parse error"))
                        : Result.success (tokens as! [ETH.Token])

                })

            switch resTokens {
            case .failure: done (false, rid)
            case let .success(a):
                a.forEach { each ($0) }
                done (true, rid)
            }
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
        let json: JSON.Dict = [
            "method" : "eth_blockNumber",
            "params" : [],
            "id" : rid
        ]

        ethMakeRequestJSON(ewm: ewm, data: json) { (res: Result<BlockChainDB.JSON, BlockChainDB.QueryError>) in
            let result = try! res
                .map { $0.asString (name: "result")! }
                .recover { (ignore) in "0xffc0" }
                .get()

            completion ((number: result, rid: rid ))
        }
    }
    
    public func getNonceAsETH (ewm: BREthereumEWM,
                               address: String,
                               rid: Int32,
                               completion: @escaping (ETH.Nonce) -> Void) {
        let json: JSON.Dict = [
            "method" : "eth_getTransactionCount",
            "params" : [address, "latest"],
            "id" : rid
        ]

        ethMakeRequestJSON(ewm: ewm, data: json) { (res: Result<BlockChainDB.JSON, BlockChainDB.QueryError>) in
            let result = try! res
                .map { $0.asString (name: "result")! }
                .recover { (ignore) in "118" }
                .get()
            completion ((address: address, nonce: result, rid: rid ))
        }
    }

    public init (dbBaseURL: String = "http://blockchain-db.us-east-1.elasticbeanstalk.com",
                 ethBaseURL: String? = nil) {
        self.dbBaseURL = dbBaseURL

        if nil == ethBaseURL {
            #if DEBUG
            self.ethBaseURL = "https://stage2.breadwallet.com"
            #else
            self.ethBaseURL = "https://api.breadwallet.com"
            #endif
        }
        else {
            self.ethBaseURL = ethBaseURL
        }
    }

    static internal let currencySymbols = ["btc":"₿", "eth":"Ξ"]
    static internal func lookupSymbol (_ code: String) -> String {
        return currencySymbols[code] ?? code
    }

    static internal let addressBRDTestnet = "0x7108ca7c4718efa810457f228305c9c71390931a" // testnet
    static internal let addressBRDMainnet = "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6" // mainnet

    static internal let dateFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSZ"
        return formatter
    }()

    internal struct JSON {
        typealias Dict = [String:Any]

        let dict: Dict

        init (dict: Dict) {
            self.dict = dict
        }

        internal func asString (name: String) -> String? {
            return dict[name] as? String
        }

        internal func asBool (name: String) -> Bool? {
            return dict[name] as? Bool
        }

        internal func asUInt64 (name: String) -> UInt64? {
            return (dict[name] as? NSNumber)
                .flatMap { UInt64 (exactly: $0)}
        }

        internal func asUInt8 (name: String) -> UInt8? {
            return (dict[name] as? NSNumber)
                .flatMap { UInt8 (exactly: $0)}
        }

        internal func asDate (name: String) -> Date? {
            return (dict[name] as? String)
                .flatMap { dateFormatter.date (from: $0) }
        }

        internal func asData (name: String) -> Data? {
            return (dict[name] as? String)
                .flatMap { Data (base64Encoded: $0)! }
        }

        internal func asArray (name: String) -> [Dict]? {
            return dict[name] as? [Dict]
        }

        internal func asDict (name: String) -> Dict? {
            return dict[name] as? Dict
        }

        internal func asStringArray (name: String) -> [String]? {
            return dict[name] as? [String]
        }
    }

    private func sendRequest (_ request: URLRequest, completion: @escaping (Result<JSON, QueryError>) -> Void) {
        session.dataTask (with: request) { (data, res, error) in
            guard nil == error else {
                completion (Result.failure(QueryError.submission (error!))) // NSURLErrorDomain
                return
            }

            guard let res = res as? HTTPURLResponse else {
                completion (Result.failure (QueryError.url ("No Response")))
                return
            }

            guard 200 == res.statusCode else {
                completion (Result.failure (QueryError.url ("Status: \(res.statusCode) ")))
                return
            }

            guard let data = data else {
                completion (Result.failure (QueryError.noData))
                return
            }

            do {
                guard let json = try JSONSerialization.jsonObject(with: data, options: []) as? JSON.Dict
                    else {
                        print ("Data JSON.Dict: \(data.map { String(format: "%c", $0) }.joined())")
                        completion (Result.failure(QueryError.jsonParse(nil)));
                        return }

                completion (Result.success (JSON (dict: json)))
            }
            catch let jsonError as NSError {
                print ("Data JSON.Error: \(data.map { String(format: "%c", $0) }.joined())")
                completion (Result.failure (QueryError.jsonParse (jsonError)))
                return
            }
        }.resume()
    }

    internal func dbMakeRequest (path: String,
                                 query: Zip2Sequence<[String],[String]>?,
                                 embedded: Bool = true,
                                 completion: @escaping (Bool, Result<[JSON], QueryError>) -> Void) {
        guard var urlBuilder = URLComponents (string: dbBaseURL)
            else { completion (false, Result.failure(QueryError.url("URLComponents"))); return }

        urlBuilder.path = "/\(path)"
        if let query = query {
            urlBuilder.queryItems = query.map { URLQueryItem (name: $0, value: $1) }
        }

        guard let url = urlBuilder.url else {
            completion (false, Result.failure (QueryError.url("URLComponents.url")))
            return
        }

        var request = URLRequest (url: url)
        request.addValue("application/json", forHTTPHeaderField: "accept")

        sendRequest (request) { (res: Result<BlockChainDB.JSON, BlockChainDB.QueryError>) in
            // See if there is a 'page' Dict in the JSON
            let page: JSON.Dict? = try! res.map { $0.asDict(name: "page") }.recover { (ignore) in return nil }.get()

            // The page is full if 'total_pages' is more than 1
            let full: Bool = page.map { (($0["total_pages"] as? Int) ?? 0) > 1 } ?? false

            // If called not embedded then never be full
            // precondition (...)

            // Callback with `more` and the result (maybe error)
            completion (false, // embedded && full,
                res.flatMap { (json: BlockChainDB.JSON) -> Result<[JSON], BlockChainDB.QueryError> in
                    let json = (embedded
                        ? json.asDict(name: "_embedded")?[path]
                        : [json.dict])

                    guard let data = json as? [JSON.Dict]
                        else { return Result.failure(QueryError.model ("[JSON.Dict] expected")) }

                    return Result.success (data.map { JSON (dict: $0) })
            })
        }
    }

    internal func ethMakeRequestJSON (ewm: BREthereumEWM, data: JSON.Dict, completion: @escaping (Result<JSON, QueryError>) -> Void) {
        let networkName = asUTF8String (networkGetName (ewmGetNetwork(ewm))).lowercased()

        guard var urlBuilder = URLComponents (string: ethBaseURL!)
            else { completion (Result.failure(QueryError.url("URLComponents"))); return }

        urlBuilder.path = "/ethq/\(networkName)/proxy"

        guard let url = urlBuilder.url
            else { completion (Result.failure (QueryError.url("URLComponents.url"))); return }

        print ("SYS: BDB: Query: \(url.absoluteString): Dict: \(data)")
        var request = URLRequest (url: url)
        request.addValue ("application/json", forHTTPHeaderField: "accept")
        request.addValue ("application/json", forHTTPHeaderField: "Content-Type")
        request.httpMethod = "POST"

        do { request.httpBody = try JSONSerialization.data (withJSONObject: data, options: []) }
        catch let jsonError as NSError {
            completion (Result.failure (QueryError.jsonParse(jsonError)))
        }

        sendRequest(request) { (res: Result<BlockChainDB.JSON, BlockChainDB.QueryError>) in
            completion (res)
        }
    }

    internal func ethMakeRequestQUERY (ewm: BREthereumEWM,
                                       query: Zip2Sequence<[String],[String]>?,
                                       data: JSON.Dict,
                                       completion: @escaping (Result<JSON, QueryError>) -> Void) {
        //        let json: JSON.Dict = [ "id" : rid ]

        let networkName = asUTF8String (networkGetName (ewmGetNetwork(ewm))).lowercased()

        guard var urlBuilder = URLComponents (string: ethBaseURL!)
            else { completion (Result.failure(QueryError.url("URLComponents"))); return }

        urlBuilder.path = "/ethq/\(networkName)/query"
        if let query = query {
            urlBuilder.queryItems = query.map { URLQueryItem (name: $0, value: $1) }
        }

        guard let url = urlBuilder.url
            else { completion (Result.failure (QueryError.url("URLComponents.url"))); return }

        print ("SYS: BDB: Query: \(url.absoluteString)")
        var request = URLRequest (url: url)
        request.addValue ("application/json", forHTTPHeaderField: "accept")
        request.addValue ("application/json", forHTTPHeaderField: "Content-Type")
        request.httpMethod = "POST"

        do { request.httpBody = try JSONSerialization.data (withJSONObject: data, options: []) }
        catch let jsonError as NSError {
            completion (Result.failure (QueryError.jsonParse(jsonError)))
        }

        sendRequest(request) { (res: Result<BlockChainDB.JSON, BlockChainDB.QueryError>) in
            // {"status":1, "message":"OK", "result":[...]}
            completion (res)
        }
    }

    internal func ethMakeRequestTOKEN (ewm: BREthereumEWM,
                                       completion: @escaping (Result<[JSON.Dict], QueryError>) -> Void) {

        guard var urlBuilder = URLComponents (string: ethBaseURL!)
            else { completion (Result.failure(QueryError.url("URLComponents"))); return }

        urlBuilder.path = "/currencies"
        urlBuilder.queryItems = [URLQueryItem (name: "type", value: "erc20")]

        guard let url = urlBuilder.url
            else { completion (Result.failure (QueryError.url("URLComponents.url"))); return }

        print ("SYS: BDB: Query: \(url.absoluteString)")
        var request = URLRequest (url: url)
        request.addValue ("application/json", forHTTPHeaderField: "accept")
        request.addValue ("application/json", forHTTPHeaderField: "Content-Type")
        request.httpMethod = "GET"

        session.dataTask (with: request) { (data, res, error) in
            guard nil == error else {
                completion (Result.failure(QueryError.submission (error!))) // NSURLErrorDomain
                return
            }

            guard let res = res as? HTTPURLResponse else {
                completion (Result.failure (QueryError.url ("No Response")))
                return
            }

            guard 200 == res.statusCode else {
                completion (Result.failure (QueryError.url ("Status: \(res.statusCode) ")))
                return
            }

            guard let data = data else {
                completion (Result.failure (QueryError.noData))
                return
            }

            do {
                guard let json = try JSONSerialization.jsonObject(with: data, options: []) as? [JSON.Dict]
                    else {
                        print ("Data JSON.Dict: \(data.map { String(format: "%c", $0) }.joined())")
                        completion (Result.failure(QueryError.jsonParse(nil)));
                        return }

                completion (Result.success (json))
            }
            catch let jsonError as NSError {
                print ("Data JSON.Error: \(data.map { String(format: "%c", $0) }.joined())")
                completion (Result.failure (QueryError.jsonParse (jsonError)))
                return
            }
        }.resume()
    }

    ///
    /// Convert an array of JSON into a single value using a specified transform
    ///
    /// - Parameters:
    ///   - id: If not value exists, report QueryError.NoEntity (id: id)
    ///   - data: The array of JSON
    ///   - transform: Function to tranfrom JSON -> T?
    ///
    /// - Returns: A `Result` with success of `T`
    ///
    private static func getOneExpected<T> (id: String, data: [JSON], transform: (JSON) -> T?) -> Result<T, QueryError> {
        switch data.count {
        case  0:
            return Result.failure (QueryError.noEntity(id: id))
        case  1:
            guard let transfer = transform (data[0])
                else { return Result.failure (QueryError.model ("(JSON) -> T transform error (one)"))}
            return Result.success (transfer)
        default:
            return Result.failure (QueryError.model ("(JSON) -> T expected one only"))
        }
    }

    ///
    /// Convert an array of JSON into an array of `T` using a specified transform.  If any
    /// individual JSON cannot be converted, then a QueryError is return for `Result`
    ///
    /// - Parameters:
    ///   - data: Array of JSON
    ///   - transform: Function to transform JSON -> T?
    ///
    /// - Returns: A `Result` with success of `[T]`
    ///
    private static func getManyExpected<T> (data: [JSON], transform: (JSON) -> T?) -> Result<[T], QueryError> {
        let results = data.map (transform)
        return results.contains(where: { $0 == nil })
            ? Result.failure(QueryError.model ("(JSON) -> T transform error (many)"))
            : Result.success(results as! [T])
    }
}
