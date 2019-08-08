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

public class BlockChainDB {

    /// Base URL (String) for the BRD BlockChain DB
    let bdbBaseURL: String

    /// Base URL (String) for BRD API Services
    let apiBaseURL: String

    // The seesion to use for DataTaskFunc as in `session.dataTask (with: request, ...)`
    let session: URLSession

    /// A DispatchQueue Used for certain queries that can't be accomplished in the session's data
    /// task.  Such as when multiple request are needed in getTransactions().
    let queue = DispatchQueue.init(label: "BlockChainDB")

    /// A function type that decorates a `request`, handles 'challenges', performs decrypting and/or
    /// uncompression of response data, redirects if requires and creates a `URLSessionDataTask`
    /// for the provided `session`.
    public typealias DataTaskFunc = (URLSession, URLRequest, @escaping (Data?, URLResponse?, Error?) -> Void) -> URLSessionDataTask

    /// A DataTaskFunc for submission to the BRD API
    internal let apiDataTaskFunc: DataTaskFunc

    /// A DataTaskFunc for submission to the BRD BlockChain DB
    internal let bdbDataTaskFunc: DataTaskFunc

    /// A default DataTaskFunc that simply invokes `session.dataTask (with: request, ...)`
    static let defaultDataTaskFunc: DataTaskFunc = {
        (_ session: URLSession,
        _ request: URLRequest,
        _ completionHandler: @escaping (Data?, URLResponse?, Error?) -> Void) -> URLSessionDataTask in
        session.dataTask (with: request, completionHandler: completionHandler)
    }

    ///
    /// A Subscription allows for BlockchainDB 'Asynchronous Notifications'.
    ///
    public struct Subscription {
        
        /// A unique identifier for the subscription
        public let subscriptionId: String

        /// A unique identifier for the device.
        public let deviceId: String

        ///
        /// An endpoint definition allowing the BlockchainDB to 'target' this App.  Allows
        /// for APNS, FCM and other notification systems.  This is an optional value; when set to
        /// .none, any existing notification will be disabled
        ///
        /// environment : { unknown, production, development }
        /// kind        : { unknown, apns, fcm, ... }
        /// value       : For apns/fcm this will be the registration token, apns should be hex-encoded
        public let endpoint: (environment: String, kind: String, value: String)?

        public init (id: String, deviceId: String? = nil, endpoint: (environment: String, kind: String, value: String)?) {
            self.subscriptionId = id
            self.deviceId = deviceId ?? id
            self.endpoint = endpoint
        }
    }

    /// A User-specific identifier - a string representation of a UUIDv4
    internal var walletId: String? = nil

    /// A Subscription specificiation
    internal var subscription: Subscription? = nil

    private var modelSubscription: Model.Subscription? {
        guard let walletId = walletId,  let subscription = subscription, let endpoint = subscription.endpoint
            else { return nil }

        return (id: subscription.subscriptionId,
                wallet: walletId,
                device: subscription.deviceId,
                endpoint: (environment: endpoint.environment, kind: endpoint.kind, value: endpoint.value))
    }

    // A BlockChainDB wallet requires at least one 'currency : [<addr>+]' entry.  Create a minimal,
    // default one using a compromised ETH address.  This will get replaced as soon as we have
    // a real ETH account.
    internal static let minimalCurrencies = ["eth" : ["A9De3DBd7D561e67527BC1ECB025C59D53B9F7EF"]]

    internal func subscribe (walletId: String, subscription: Subscription) {
        self.walletId = walletId
        self.subscription = subscription

        // Subscribing requires a wallet on the BlockChainDD, so start by create the BlockChainDB
        // Model.Wallet and then get or create one on the DB.

        let wallet = (id: walletId, currencies: BlockChainDB.minimalCurrencies)

        getOrCreateWallet (wallet) { (walletRes: Result<Model.Wallet, QueryError>) in
            guard case .success = walletRes
                else { print ("SYS: BDB:Wallet: Missed"); return }

            if let model = self.modelSubscription {
                // If the subscription included an endpoint, then put the subscription on the
                // blockchainDB - via POST or PUT.
                self.getSubscription (id: model.id) { (subRes: Result<Model.Subscription, QueryError>) in
                    switch subRes {
                    case let .success (subscription):
                        self.updateSubscription (subscription) {
                            (resSub: Result<BlockChainDB.Model.Subscription, BlockChainDB.QueryError>) in
                        }

                    case .failure:
                        self.createSubscription (model) {
                            (resSub: Result<BlockChainDB.Model.Subscription, BlockChainDB.QueryError>) in
                        }
                    }
                }
            }

            else {
                // Otherwise delete it.
                self.deleteSubscription(id: subscription.subscriptionId) {
                    (subRes: Result<BlockChainDB.Model.Subscription, BlockChainDB.QueryError>) in
                }
            }
        }
    }

    ///
    /// Initialize a BlockChainDB
    ///
    /// - Parameters:
    ///   - session: the URLSession to use.  Defaults to `URLSession (configuration: .default)`
    ///   - bdbBaseURL: the baseURL for the BRD BlockChain DB.  Defaults to "http://blockchain-db.us-east-1.elasticbeanstalk.com"
    ///   - bdbDataTaskFunc: an optional DataTaskFunc for BRD BlockChain DB.  This defaults to
    ///       `session.dataTask (with: request, ...)`
    ///   - apiBaseURL: the baseRUL for the BRD API Server.  Defaults to "https://api.breadwallet.com".
    ///       if this is a DEBUG build then "https://stage2.breadwallet.com" will be used instead.
    ///   - apiDataTaskFunc: an optional DataTaskFunc for BRD API services.  For a non-DEBUG build,
    ///       this function would need to properly authenticate with BRD.  This means 'decorating
    ///       the request' header, perhaps responding to a 'challenge', perhaps decripting and/or
    ///       uncompressing response data.  This defaults to `session.dataTask (with: request, ...)`
    ///       which suffices for DEBUG builds.
    ///
    public init (session: URLSession = URLSession (configuration: .default),
                 bdbBaseURL: String = "https://test-blockchaindb-api.brd.tools", // "http://blockchain-db.us-east-1.elasticbeanstalk.com",
                 bdbDataTaskFunc: DataTaskFunc? = nil,
                 apiBaseURL: String = "https://api.breadwallet.com",
                 apiDataTaskFunc: DataTaskFunc? = nil) {

        self.session = session

        self.bdbBaseURL = bdbBaseURL
        self.bdbDataTaskFunc = bdbDataTaskFunc ?? BlockChainDB.defaultDataTaskFunc

        #if DEBUG
        self.apiBaseURL = "https://stage2.breadwallet.com"
        #else
        self.apiBaseURL = apiBaseURL
        #endif
        self.apiDataTaskFunc = apiDataTaskFunc ?? BlockChainDB.defaultDataTaskFunc
    }

    ///
    /// A QueryError subtype of Error
    ///
    /// - url:
    /// - submission:
    /// - noData:
    /// - jsonParse:
    /// - model:
    /// - noEntity:
    ///
    public enum QueryError: Error {
        // HTTP URL build failed
        case url (String)

        // HTTP submission error
        case submission (Error)

        // HTTP response unexpected (typically not 200/OK)
        case response (Int) // includes the status code

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
    /// The BlockChainDB Model (aka Schema-ish)
    ///
    public struct Model {

        /// Blockchain

        public typealias BlockchainFee = (amount: String, tier: String, confirmations: String) // currency?
        public typealias Blockchain = (
            id: String,
            name: String,
            network: String,
            isMainnet: Bool,
            currency: String,
            blockHeight: UInt64,
            feeEstimates: [BlockchainFee])

        static internal func asBlockchainFee (json: JSON) -> Model.BlockchainFee? {
            guard let amount = json.asDict(name: "fee")?["amount"] as? String,
                let tier = json.asString(name: "tier")
                else { return nil }

            // We've seen 'null' - assign "1" if so.
            let confirmations = json.asString(name: "estimated_confirmation_in") ?? "1"

            return (amount: amount, tier: tier, confirmations: confirmations)
        }

        static internal func asBlockchain (json: JSON) -> Model.Blockchain? {
            guard let id = json.asString (name: "id"),
                let name = json.asString (name: "name"),
                let network = json.asString (name: "network"),
                let isMainnet = json.asBool (name: "is_mainnet"),
                let currency = json.asString (name: "native_currency_id"),
                let blockHeight = json.asUInt64 (name: "block_height")
                else { return nil }

            guard let feeEstimates = json.asArray(name: "fee_estimates")?
                .map ({ JSON (dict: $0) })
                .map ({ asBlockchainFee (json: $0) }) as? [Model.BlockchainFee]
            else { return nil }

            return (id: id, name: name, network: network, isMainnet: isMainnet, currency: currency,
                    blockHeight: blockHeight,
                    feeEstimates: feeEstimates)
        }

        /// We define default blockchains but these are wholly insufficient given that the
        /// specfication includes `blockHeight` (which can never be correct).
        static public let defaultBlockchains: [Blockchain] = [
            // Mainnet
            (id: "bitcoin-mainnet",       name: "Bitcoin",       network: "mainnet", isMainnet: true,  currency: "btc", blockHeight:  654321,
             feeEstimates: [(amount: "30", tier: "10m", confirmations: "")]),
            (id: "bitcoin-cash-mainnet",  name: "Bitcoin Cash",  network: "mainnet", isMainnet: true,  currency: "bch", blockHeight: 1000000,
             feeEstimates: [(amount: "30", tier: "10m", confirmations: "")]),
            (id: "ethereum-mainnet",      name: "Ethereum",      network: "mainnet", isMainnet: true,  currency: "eth", blockHeight: 8000000,
             feeEstimates: [(amount: "2000000000", tier: "1m", confirmations: "")]),
            (id: "ripple-mainnet",        name: "Ripple",        network: "mainnet", isMainnet: true,  currency: "xrp", blockHeight: 5000000,
            feeEstimates: [(amount: "20", tier: "1m", confirmations: "")]),

            // Testnet
            (id: "bitcoin-testnet",       name: "Bitcoin Test",      network: "testnet", isMainnet: false, currency: "btc", blockHeight:  900000,
             feeEstimates: [(amount: "30", tier: "10m", confirmations: "")]),
            (id: "bitcoin-cash-testnet",  name: "Bitcoin Cash Test", network: "testnet", isMainnet: false, currency: "bch", blockHeight: 1200000,
             feeEstimates: [(amount: "30", tier: "10m", confirmations: "")]),
            (id: "ethereum-testnet",      name: "Ethereum Testnet",  network: "testnet", isMainnet: false, currency: "eth", blockHeight: 1000000,
             feeEstimates: [(amount: "2000000000", tier: "1m", confirmations: "")]),
            (id: "ethereum-rinkeby",      name: "Ethereum Rinkeby",  network: "rinkeby", isMainnet: false, currency: "eth", blockHeight: 2000000,
             feeEstimates: [(amount: "2000000000", tier: "1m", confirmations: "")]),
            (id: "ripple-testnet",        name: "Ripple Testnet",    network: "testnet", isMainnet: false, currency: "xrp", blockHeight: 25000,
             feeEstimates: [(amount: "20", tier: "1m", confirmations: "")]),
        ]

        /// Currency & CurrencyDenomination

        public typealias CurrencyDenomination = (name: String, code: String, decimals: UInt8, symbol: String /* extra */)
        public typealias Currency = (
            id: String,
            name: String,
            code: String,
            type: String,
            blockchainID: String,
            address: String?,
            demoninations: [CurrencyDenomination])

       static internal func asCurrencyDenomination (json: JSON) -> Model.CurrencyDenomination? {
            guard let name = json.asString (name: "name"),
                let code = json.asString (name: "short_name"),
                let decimals = json.asUInt8 (name: "decimals")
                // let symbol = json.asString (name: "symbol")
                else { return nil }

            let symbol = lookupSymbol (code)

            return (name: name, code: code, decimals: decimals, symbol: symbol)
        }

        static internal let currencySymbols = ["btc":"₿", "eth":"Ξ"]
        static internal func lookupSymbol (_ code: String) -> String {
            return currencySymbols[code] ?? code
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

            // Mainnet
            (id: "Bitcoin", name: "Bitcoin", code: "btc", type: "native", blockchainID: "bitcoin-mainnet", address: nil,
             demoninations: [(name: "satoshi", code: "sat", decimals: 0, symbol: lookupSymbol ("sat")),
                             (name: "bitcoin", code: "btc", decimals: 8, symbol: lookupSymbol ("btc"))]),

            (id: "Bitcoin-Cash", name: "Bitcoin Cash", code: "bch", type: "native", blockchainID: "bitcoin-cash-mainnet", address: nil,
             demoninations: [(name: "satoshi",      code: "sat", decimals: 0, symbol: lookupSymbol ("sat")),
                             (name: "bitcoin cash", code: "bch", decimals: 8, symbol: lookupSymbol ("bch"))]),

            (id: "Ethereum", name: "Ethereum", code: "eth", type: "native", blockchainID: "ethereum-mainnet", address: nil,
             demoninations: [(name: "wei",   code: "wei",  decimals:  0, symbol: lookupSymbol ("wei")),
                             (name: "gwei",  code: "gwei", decimals:  9, symbol: lookupSymbol ("gwei")),
                             (name: "ether", code: "eth",  decimals: 18, symbol: lookupSymbol ("eth"))]),

            (id: "BRD Token", name: "BRD Token", code: "brd", type: "erc20", blockchainID: "ethereum-mainnet", address: addressBRDMainnet,
             demoninations: [(name: "BRD_INTEGER",   code: "BRDI",  decimals:  0, symbol: "brdi"),
                             (name: "BRD",           code: "BRD",   decimals: 18, symbol: "brd")]),

            (id: "EOS Token", name: "EOS Token", code: "eos", type: "erc20", blockchainID: "ethereum-mainnet", address: "0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0",
             demoninations: [(name: "EOS_INTEGER",   code: "EOSI",  decimals:  0, symbol: "eosi"),
                             (name: "EOS",           code: "EOS",   decimals: 18, symbol: "eos")]),

            (id: "Ripple", name: "Ripple", code: "xrp", type: "native", blockchainID: "ripple-mainnet", address: nil,
             demoninations: [(name: "drop", code: "drop", decimals: 0, symbol: "drop"),
                             (name: "xrp",  code: "xrp",  decimals: 6, symbol: "xrp")]),

            // Testnet
            (id: "Bitcoin-Testnet", name: "Bitcoin", code: "btc", type: "native", blockchainID: "bitcoin-testnet", address: nil,
             demoninations: [(name: "satoshi", code: "sat", decimals: 0, symbol: lookupSymbol ("sat")),
                             (name: "bitcoin", code: "btc", decimals: 8, symbol: lookupSymbol ("btc"))]),

            (id: "Bitcoin-Cash-Testnet", name: "Bitcoin Cash Test", code: "bch", type: "native", blockchainID: "bitcoin-cash-testnet", address: nil,
             demoninations: [(name: "satoshi",           code: "sat", decimals: 0, symbol: lookupSymbol ("sat")),
                             (name: "bitcoin cash test", code: "bch", decimals: 8, symbol: lookupSymbol ("bch"))]),

            (id: "Ethereum-Testnet", name: "Ethereum", code: "eth", type: "native", blockchainID: "ethereum-testnet", address: nil,
             demoninations: [(name: "wei",   code: "wei",  decimals:  0, symbol: lookupSymbol ("wei")),
                             (name: "gwei",  code: "gwei", decimals:  9, symbol: lookupSymbol ("gwei")),
                             (name: "ether", code: "eth",  decimals: 18, symbol: lookupSymbol ("eth"))]),

            (id: "BRD Token Testnet", name: "BRD Token", code: "brd", type: "erc20", blockchainID: "ethereum-testnet", address: addressBRDTestnet,
             demoninations: [(name: "BRD_INTEGER",   code: "BRDI",  decimals:  0, symbol: "brdi"),
                             (name: "BRD",           code: "BRD",   decimals: 18, symbol: "brd")]),

            (id: "Ripple", name: "Ripple", code: "xrp", type: "native", blockchainID: "ripple-testnet", address: nil,
             demoninations: [(name: "drop", code: "drop", decimals: 0, symbol: "drop"),
                             (name: "xrp",  code: "xrp",  decimals: 6, symbol: "xrp")]),

       ]

        static internal let addressBRDTestnet = "0x7108ca7c4718efa810457f228305c9c71390931a" // testnet
        static internal let addressBRDMainnet = "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6" // mainnet

        /// Transfer

        public typealias Transfer = (
            id: String,
            source: String?,
            target: String?,
            amountValue: String,
            amountCurrency: String,
            acknowledgements: UInt64,
            index: UInt64,
            transactionId: String?,
            blockchainId: String)

        static internal func asTransfer (json: JSON) -> Model.Transfer? {
            guard let id   = json.asString (name: "transfer_id"),
                let bid    = json.asString (name: "blockchain_id"),
                let index  = json.asUInt64 (name: "index"),
                let amount = json.asDict (name: "amount").map ({ JSON (dict: $0) }),
                let amountValue    = amount.asString (name: "amount"),
                let amountCurrency = amount.asString (name: "currency_id")
                else { return nil }

            // TODO: Resolve if optional or not
            let acks   = json.asUInt64 (name: "acknowledgements") ?? 0
            let source = json.asString (name: "from_address")
            let target = json.asString (name: "to_address")
            let tid    = json.asString (name: "transaction_id")

            return (id: id, source: source, target: target,
                    amountValue: amountValue, amountCurrency: amountCurrency,
                    acknowledgements: acks, index: index,
                    transactionId: tid, blockchainId: bid)
        }

        /// Transaction

        public typealias Transaction = (
            id: String,
            blockchainId: String,
            hash: String,
            identifier: String,
            blockHash: String?,
            blockHeight: UInt64?,
            index: UInt64?,
            confirmations: UInt64?,
            status: String,
            size: UInt64,
            timestamp: Date?,
            firstSeen: Date?,
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
                let size       = json.asUInt64 (name: "size")
                else { return nil }

            // TODO: Resolve if optional or not
            let acks       = json.asUInt64 (name: "acknowledgements") ?? 0
            // TODO: Resolve if optional or not
            let firstSeen     = json.asDate   (name: "first_seen")
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

        public typealias Block = (
            id: String,
            blockchainId: String,
            hash: String,
            height: UInt64,
            header: String?,
            raw: Data?,
            mined: Date,
            size: UInt64,
            prevHash: String?,
            nextHash: String?, // fees
            transactions: [Transaction]?,
            acknowledgements: UInt64
        )

        static internal func asBlock (json: JSON) -> Model.Block? {
            guard let id = json.asString(name: "block_id"),
                let bid      = json.asString(name: "blockchain_id"),
                let hash     = json.asString (name: "hash"),
                let height   = json.asUInt64 (name: "height"),
                let mined    = json.asDate   (name: "mined"),
                let size     = json.asUInt64 (name: "size")
                else { return nil }

            let acks       = json.asUInt64 (name: "acknowledgements") ?? 0
            let header   = json.asString (name: "header")
            let raw      = json.asData   (name: "raw")
            let prevHash = json.asString (name: "prev_hash")
            let nextHash = json.asString (name: "next_hash")

            let transactions = json.asArray (name: "transactions")?
                .map ({ JSON (dict: $0 )})
                .map ({ asTransaction (json: $0)}) as? [Model.Transaction]  // not quite

            return (id: id, blockchainId: bid,
                    hash: hash, height: height, header: header, raw: raw, mined: mined, size: size,
                    prevHash: prevHash, nextHash: nextHash,
                    transactions: transactions,
                    acknowledgements: acks)
        }

        /// Wallet

        public typealias Wallet = (
            id: String,
            currencies: [String:[String]]  // "currency_id": [<address>, ...]
        )

//        static internal func asWalletCurrency (json: JSON) -> Model.WalletCurrency? {
//            guard let currency = json.asString(name: "currency_id")
//            else { return nil }
//
//            let addresses = json.asStringArray(name: "addresses") ?? []
//
//            return (currency: currency, addresses: addresses)
//        }
//
//        static internal func asJSON (walletCurrency: Model.WalletCurrency) -> JSON.Dict {
//            return [
//                "currency_id" : walletCurrency.currency,
//                "addresses"   : walletCurrency.addresses
//            ]
//        }

        static internal func asWallet (json: JSON) -> Model.Wallet? {
            guard let id = json.asString (name: "id")
                else { return nil }

            if let currencies = json.asDict(name: "currencies") as? [String:[String]] {
                return (id: id, currencies: currencies)
            }
            else {
                print ("SYS: BDB:Missed Wallet Currencies")
                return (id: id, currencies: [String:[String]]())
            }
        }

        static internal func asJSON (wallet: Wallet) -> JSON.Dict {
            return [
                "id"            : wallet.id,
                "created"       : "2019-05-06T01:08:49.495+0000",
                "currencies"    : wallet.currencies
            ]
        }

        /// Subscription

        public typealias SubscriptionEndpoint = (environment: String, kind: String, value: String)
        public typealias Subscription = (
            id: String,
            wallet: String,
            device: String,
            endpoint: SubscriptionEndpoint
        )

        static internal func asSubscriptionEndpoint (json: JSON) -> SubscriptionEndpoint? {
            guard let environment = json.asString (name: "environment"),
            let kind = json.asString(name: "kind"),
            let value = json.asString(name: "value")
                else { return nil }

            return (environment: environment, kind: kind, value: value)
        }

        static internal func asJSON (subscriptionEndpoint: SubscriptionEndpoint) -> JSON.Dict {
            return [
                "environment"   : subscriptionEndpoint.environment,
                "kind"          : subscriptionEndpoint.kind,
                "value"         : subscriptionEndpoint.value
            ]
        }

        static internal func asSubscription (json: JSON) -> Subscription? {
            guard let id = json.asString (name: "subscription_id"),
                let wallet = json.asString (name: "wallet_id"),
                let device = json.asString (name: "device_id"),
                let endpoint = json.asDict(name: "endpoint")
                    .flatMap ({ asSubscriptionEndpoint (json: JSON (dict: $0)) })
                else { return nil }

            return (id: id, wallet: wallet, device: device, endpoint: endpoint)
        }

        static internal func asJSON (subscription: Subscription) -> JSON.Dict {
            return [
                "subscription_id"   : subscription.id,
                "wallet_id"         : subscription.wallet,
                "device_id"         : subscription.device,
                "endpoint"          : asJSON (subscriptionEndpoint: subscription.endpoint)
            ]
        }

    } // End of Model

    public func getBlockchains (mainnet: Bool? = nil, completion: @escaping (Result<[Model.Blockchain],QueryError>) -> Void) {
        bdbMakeRequest (path: "blockchains", query: mainnet.map { zip (["testnet"], [($0 ? "false" : "true")]) }) {
            (more: Bool, res: Result<[JSON], QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getManyExpected(data: $0, transform: Model.asBlockchain)
            })
        }
    }

    public func getBlockchain (blockchainId: String, completion: @escaping (Result<Model.Blockchain,QueryError>) -> Void) {
        bdbMakeRequest(path: "blockchains/\(blockchainId)", query: nil, embedded: false) {
            (more: Bool, res: Result<[JSON], QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getOneExpected (id: blockchainId, data: $0, transform: Model.asBlockchain)
            })
        }
    }

    public func getCurrencies (blockchainId: String? = nil, completion: @escaping (Result<[Model.Currency],QueryError>) -> Void) {
        bdbMakeRequest (path: "currencies", query: blockchainId.map { zip(["blockchain_id"], [$0]) }) {
            (more: Bool, res: Result<[JSON], QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getManyExpected(data: $0, transform: Model.asCurrency)
            })
        }
    }

    public func getCurrency (currencyId: String, completion: @escaping (Result<Model.Currency,QueryError>) -> Void) {
        bdbMakeRequest (path: "currencies/\(currencyId)", query: nil, embedded: false) {
            (more: Bool, res: Result<[JSON], QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getOneExpected(id: currencyId, data: $0, transform: Model.asCurrency)
            })
        }
    }

    /// Subscription

    internal func makeSubscriptionRequest (_ subscription: Model.Subscription, path: String, httpMethod: String,
                                           completion: @escaping (Result<Model.Subscription, QueryError>) -> Void) {
        makeRequest (bdbDataTaskFunc, bdbBaseURL,
                     path: path,
                     query: nil,
                     data: Model.asJSON(subscription: subscription),
                     httpMethod: httpMethod) {
                        (res: Result<JSON.Dict, QueryError>) in
                        completion (res.flatMap {
                            Model.asSubscription(json: JSON(dict: $0))
                                .map { Result.success ($0) }
                                ?? Result.failure(QueryError.model("Missed Subscription"))
                        })
        }
    }

    public func getSubscription (id: String, completion: @escaping (Result<Model.Subscription, QueryError>) -> Void) {
        bdbMakeRequest (path: "subscriptions/\(id)", query: nil, embedded: false) {
            (more: Bool, res: Result<[JSON], QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getOneExpected (id: id, data: $0, transform: Model.asSubscription)
            })
        }
    }

    public func createSubscription (_ subscription: Model.Subscription,
                                    completion: @escaping (Result<Model.Subscription, QueryError>) -> Void) {
        makeSubscriptionRequest (subscription,
                                 path: "subscriptions",
                                 httpMethod: "POST",
                                 completion: completion)
     }

    public func getOrCreateSubscription (_ subscription: Model.Subscription,
                                         completion: @escaping (Result<Model.Subscription, QueryError>) -> Void) {
        getSubscription(id: subscription.id) { (res: Result<BlockChainDB.Model.Subscription, BlockChainDB.QueryError>) in
            if case .success = res { completion (res) }
            else {
                self.createSubscription (subscription, completion: completion)
            }
        }
    }

    public func updateSubscription (_ subscription: Model.Subscription, completion: @escaping (Result<Model.Subscription, QueryError>) -> Void) {
        let path = "subscriptions/\(subscription.id )"
        makeSubscriptionRequest (subscription,
                                 path: path,
                                 httpMethod: "PUT",
                                 completion: completion)
    }

    public func deleteSubscription (id: String, completion: @escaping (Result<Model.Subscription, QueryError>) -> Void) {
        let path = "subscriptions/\(id)"
        makeRequest (bdbDataTaskFunc, bdbBaseURL,
                     path: path,
                     query: nil,
                     data: nil,
                     httpMethod: "DELETE") {
                        (res: Result<JSON.Dict, QueryError>) in
                        // Not likely
                        completion (res.flatMap {
                            Model.asSubscription(json: JSON(dict: $0))
                                .map { Result.success ($0) }
                                ?? Result.failure(QueryError.model("Missed Delete Subscription"))
                        })
        }
    }

    /// Wallet

    internal func makeWalletRequest (_ wallet: Model.Wallet, path: String, httpMethod: String,
                                           completion: @escaping (Result<Model.Wallet, QueryError>) -> Void) {
        makeRequest (bdbDataTaskFunc, bdbBaseURL,
                     path: path,
                     query: nil,
                     data: Model.asJSON (wallet: wallet),
                     httpMethod: httpMethod) {
                        (res: Result<JSON.Dict, QueryError>) in
                        completion (res.flatMap {
                            Model.asWallet(json: JSON(dict: $0))
                                .map { Result.success ($0) }
                                ?? Result.failure(QueryError.model("Missed Wallet"))
                        })
        }
    }

    public func getWallet (walletId: String, completion: @escaping (Result<Model.Wallet, QueryError>) -> Void) {
        bdbMakeRequest(path: "wallets/\(walletId)", query: nil, embedded: false) { (more: Bool, res: Result<[JSON], QueryError>) in
            precondition(!more)
            completion (res.flatMap {
                BlockChainDB.getOneExpected(id: walletId, data: $0, transform: Model.asWallet)
            })
        }
    }

    public func createWallet (_ wallet: Model.Wallet, completion: @escaping (Result<Model.Wallet, QueryError>) -> Void) {
        makeWalletRequest (wallet,
                           path: "wallets",
                           httpMethod: "POST",
                           completion: completion)
    }

    public func getOrCreateWallet (_ wallet: Model.Wallet, completion: @escaping (Result<Model.Wallet, QueryError>) -> Void) {
        getWallet (walletId: wallet.id) { (res: Result<Model.Wallet, QueryError>) in
            if case .success = res { completion (res) }
            else {
                self.createWallet (wallet, completion: completion)
            }
        }
    }

    public func updateWallet (_ wallet: Model.Wallet, completion: @escaping (Result<Model.Wallet, QueryError>) -> Void) {
        let path = "wallets/\(wallet.id)"
        makeWalletRequest (wallet,
                           path: path,
                           httpMethod: "PUT",
                           completion: completion)
    }

    public func deleteWallet (id: String, completion: @escaping (Result<Model.Wallet, QueryError>) -> Void) {
        let path = "wallets/\(id)"
        makeRequest (bdbDataTaskFunc, bdbBaseURL,
                     path: path,
                     query: nil,
                     data: nil,
                     httpMethod: "DELETE") {
                        (res: Result<JSON.Dict, QueryError>) in
                        // Not likely
                        completion (res.flatMap {
                            Model.asWallet(json: JSON(dict: $0))
                                .map { Result.success ($0) }
                                ?? Result.failure(QueryError.model("Missed Delete Wallet"))
                        })
        }
    }

    // Transfers

    public func getTransfers (blockchainId: String, addresses: [String], completion: @escaping (Result<[Model.Transfer], QueryError>) -> Void) {
        let queryKeys = ["blockchain_id"] + Array (repeating: "address", count: addresses.count)
        let queryVals = [blockchainId]    + addresses

        bdbMakeRequest (path: "transfers", query: zip (queryKeys, queryVals)) {
            (more: Bool, res: Result<[JSON], QueryError>) in
            completion (res.flatMap {
                BlockChainDB.getManyExpected (data: $0, transform: Model.asTransfer)
            })
        }
    }

    public func getTransfer (transferId: String, completion: @escaping (Result<Model.Transfer, QueryError>) -> Void) {
        bdbMakeRequest (path: "transfers/\(transferId)", query: nil, embedded: false) {
            (more: Bool, res: Result<[JSON], QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getOneExpected (id: transferId, data: $0, transform: Model.asTransfer)
            })
        }
    }

    // Transactions

    static let ADDRESS_COUNT = 50

    public func getTransactions (blockchainId: String,
                                 addresses: [String],
                                 begBlockNumber: UInt64 = 0,
                                 endBlockNumber: UInt64 = 0,
                                 includeRaw: Bool = false,
                                 includeProof: Bool = false,
                                 completion: @escaping (Result<[Model.Transaction], QueryError>) -> Void) {
        // This query could overrun the endpoint's page size (typically 5,000).  If so, we'll need
        // to repeat the request for the next batch.
        self.queue.async {
            var error: QueryError? = nil
            var results = [Model.Transaction]()

            for addresses in addresses.chunked(into: BlockChainDB.ADDRESS_COUNT) {
                if nil != error { break }
                let queryKeys = ["blockchain_id", "start_height", "end_height", "include_proof", "include_raw"]
                    + Array (repeating: "address", count: addresses.count)

                var queryVals = [blockchainId, "0", "0", includeProof.description, includeRaw.description]
                    + addresses

                let semaphore = DispatchSemaphore (value: 0)

                //            var moreResults = false
                var begBlockNumber = begBlockNumber

                for begHeight in stride (from: begBlockNumber, to: endBlockNumber, by: 5000) {
                    if nil != error { break }
                    queryVals[1] = begHeight.description
                    queryVals[2] = min (begHeight + 5000, endBlockNumber).description

                    //                moreResults = false

                    self.bdbMakeRequest (path: "transactions", query: zip (queryKeys, queryVals)) {
                        (more: Bool, res: Result<[JSON], QueryError>) in
                        // Flag if `more`
                        //                    moreResults = more

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
                }
            }

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

        bdbMakeRequest (path: "transactions/\(transactionId)", query: zip (queryKeys, queryVals), embedded: false) {
            (more: Bool, res: Result<[JSON], QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getOneExpected (id: transactionId, data: $0, transform: Model.asTransaction)
            })
        }
    }

    public func createTransaction (blockchainId: String,
                                   hashAsHex: String,
                                   transaction: Data,
                                   completion: @escaping (Result<Void, QueryError>) -> Void) {
        let json: JSON.Dict = [
            "blockchain_id": blockchainId,
            "transaction_id": hashAsHex,
            "data" : transaction.base64EncodedString()
        ]

        makeRequest (bdbDataTaskFunc, bdbBaseURL,
                     path: "/transactions",
                     data: json,
                     httpMethod: "POST") {
                        (res: Result<JSON.Dict, BlockChainDB.QueryError>) in
                        switch res {
                        case .success: completion (Result.success(()))
                        case .failure(let error):
                            // Consider 301 or 404 errors as success - owing to a 'quirk' in
                            // transaction submission.
                            if case let QueryError.response (status) = error,
                                (302 == status || 404 == status) {
                                completion (Result.success(()))
                            }
                            else {
                                completion (Result.failure(error))
                            }
                        }}
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

                self.bdbMakeRequest (path: "blocks", query: zip (queryKeys, queryVals)) {
                    (more: Bool, res: Result<[JSON], QueryError>) in

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

        bdbMakeRequest (path: "blocks/\(blockId)", query: zip (queryKeys, queryVals), embedded: false) {
            (more: Bool, res: Result<[JSON], QueryError>) in
            precondition (!more)
            completion (res.flatMap {
                BlockChainDB.getOneExpected (id: blockId, data: $0, transform: Model.asBlock)
            })
        }
    }

    /// BTC - nothing

    /// ETH

    /// The ETH JSON_RPC request identifier.
    var rid: UInt32 = 0

    /// Return the current request identifier and then increment it.
    var ridIncr: UInt32 {
        let rid = self.rid
        self.rid += 1
        return rid
    }

    public struct ETH {
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
            isError: String)

        static internal func asTransaction (json: JSON) -> ETH.Transaction? {
            guard let hash = json.asString(name: "hash"),
                let sourceAddr   = json.asString(name: "from"),
                let targetAddr   = json.asString(name: "to"),
                let contractAddr = json.asString(name: "contractAddress"),
                let amount       = json.asString(name: "value"),
                let gasLimit     = json.asString(name: "gas"),
                let gasPrice     = json.asString(name: "gasPrice"),
                let data         = json.asString(name: "input"),
                let nonce        = json.asString(name: "nonce"),
                let gasUsed      = json.asString(name: "gasUsed"),
                let blockNumber  = json.asString(name: "blockNumber"),
                let blockHash    = json.asString(name: "blockHash"),
                let blockConfirmations    = json.asString(name: "confirmations"),
                let blockTransactionIndex = json.asString(name: "transactionIndex"),
                let blockTimestamp        = json.asString(name: "timeStamp"),
                let isError      = json.asString(name: "isError")
                else { return nil }

            return (hash: hash,
                    sourceAddr: sourceAddr, targetAddr: targetAddr, contractAddr: contractAddr,
                    amount: amount, gasLimit: gasLimit, gasPrice: gasPrice,
                    data: data, nonce: nonce, gasUsed: gasUsed,
                    blockNumber: blockNumber, blockHash: blockHash,
                    blockConfirmations: blockConfirmations, blockTransactionIndex: blockTransactionIndex, blockTimestamp: blockTimestamp,
                    isError: isError)
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
            blockTimestamp: String)

        // BRD API servcies *always* appends `topics` with ""; we need to axe that.
        static internal func dropLastIfEmpty (_ strings: [String]?) -> [String]? {
            return (nil != strings && !strings!.isEmpty && "" == strings!.last!
                ? strings!.dropLast()
                : strings)
        }

        static internal func asLog (json: JSON) -> ETH.Log? {
            guard let hash = json.asString(name: "transactionHash"),
                let contract    = json.asString(name: "address"),
                let topics      = dropLastIfEmpty (json.asStringArray (name: "topics")),
                let data        = json.asString(name: "data"),
                let gasPrice    = json.asString(name: "gasPrice"),
                let gasUsed     = json.asString(name: "gasUsed"),
                let logIndex    = json.asString(name: "logIndex"),
                let blockNumber = json.asString(name: "blockNumber"),
                let blockTransactionIndex = json.asString(name: "transactionIndex"),
                let blockTimestamp        = json.asString(name: "timeStamp")
                else { return nil }

            return (hash: hash, contract: contract, topics: topics, data: data,
                    gasPrice: gasPrice, gasUsed: gasUsed,
                    logIndex: logIndex,
                    blockNumber: blockNumber, blockTransactionIndex: blockTransactionIndex, blockTimestamp: blockTimestamp)
        }

        public typealias Token = (
            address: String,
            symbol: String,
            name: String,
            description: String,
            decimals: UInt32,
            defaultGasLimit: String?,
            defaultGasPrice: String?)

        static internal func asToken (json: JSON) -> ETH.Token? {
            guard let name   = json.asString(name: "name"),
                let symbol   = json.asString(name: "code"),
                let address  = json.asString(name: "contract_address"),
                let decimals = json.asUInt8(name: "scale")
                else { return nil }

            let description = "Token for '\(symbol)'"

            return (address: address, symbol: symbol, name: name, description: description,
                    decimals: UInt32(decimals),
                    defaultGasLimit: nil,
                    defaultGasPrice: nil)
        }
    }

    public func getBalanceAsETH (network: String,
                                 address: String,
                                 completion: @escaping (Result<String,QueryError>) -> Void) {
        let json: JSON.Dict = [
            "jsonrpc" : "2.0",
            "method"  : "eth_getBalance",
            "params"  : [address, "latest"],
            "id"      : ridIncr
        ]

        apiMakeRequestJSON (network: network, data: json,
                            completion: BlockChainDB.getOneResultString(completion))
    }

    public func getBalanceAsTOK (network: String,
                                 address: String,
                                 contract: String,
                                 completion: @escaping (Result<String,QueryError>) -> Void) {
        let json: JSON.Dict = [ "id" : ridIncr ]

        var queryDict = [
            "module"    : "account",
            "action"    : "tokenbalance",
            "address"   : address,
            "contractaddress" : contract
        ]

        apiMakeRequestQUERY (network: network,
                             query: zip (Array(queryDict.keys), Array(queryDict.values)),
                             data: json,
                             completion: BlockChainDB.getOneResultString (completion))
    }

    public func getGasPriceAsETH (network: String,
                                  completion: @escaping (Result<String,QueryError>) -> Void) {
        let json: JSON.Dict = [
            "method" : "eth_gasPrice",
            "params" : [],
            "id" : ridIncr
        ]

        apiMakeRequestJSON (network: network, data: json,
                            completion: BlockChainDB.getOneResultString(completion))
    }

    public func getGasEstimateAsETH (network: String,
                                     from: String,
                                     to: String,
                                     amount: String,
                                     data: String,
                                     completion: @escaping (Result<String,QueryError>) -> Void) {
        var param = ["from":from, "to":to]
        if amount != "0x" { param["value"] = amount }
        if data   != "0x" { param["data"]  = data }

        let json: JSON.Dict = [
            "jsonrpc" : "2.0",
            "method"  : "eth_estimateGas",
            "params"  : [param],
            "id"      : ridIncr
        ]

        apiMakeRequestJSON (network: network, data: json,
                            completion: BlockChainDB.getOneResultString(completion))
    }

    public func submitTransactionAsETH (network: String,
                                        transaction: String,
                                        completion: @escaping (Result<String,QueryError>) -> Void) {
        let json: JSON.Dict = [
            "jsonrpc" : "2.0",
            "method"  : "eth_sendRawTransaction",
            "params"  : [transaction],
            "id"      : ridIncr
        ]

        apiMakeRequestJSON (network: network, data: json,
                            completion: BlockChainDB.getOneResultString(completion))
    }

    public func getTransactionsAsETH (network: String,
                                      address: String,
                                      begBlockNumber: UInt64,
                                      endBlockNumber: UInt64,
                                      completion: @escaping (Result<[ETH.Transaction],QueryError>) -> Void) {
        let json: JSON.Dict = [
            "account" : address,
            "id"      : ridIncr ]

        var queryDict = [
            "module"    : "account",
            "action"    : "txlist",
            "address"   : address,
            "startBlock": begBlockNumber.description,
            "endBlock"  : endBlockNumber.description
        ]

        apiMakeRequestQUERY (network: network, query: zip (Array(queryDict.keys), Array(queryDict.values)), data: json) {
            (res: Result<JSON, QueryError>) in

            completion (res
                .flatMap { (json: JSON) -> Result<[ETH.Transaction], QueryError> in
                    guard let _ = json.asString (name: "status"),
                        let   _ = json.asString (name: "message"),
                        let   result  = json.asArray (name:  "result")
                        else { return Result.failure(QueryError.model("Missed {status, message, result")) }

                    let transactions = result.map { ETH.asTransaction (json: JSON (dict: $0)) }

                    return transactions.contains(where: { nil == $0 })
                        ? Result.failure (QueryError.model ("ETH.Transaction parse error"))
                        : Result.success (transactions as! [ETH.Transaction])
            })
        }
    }

    public func getLogsAsETH (network: String,
                              contract: String?,
                              address: String,
                              event: String,
                              begBlockNumber: UInt64,
                              endBlockNumber: UInt64,
                              completion: @escaping (Result<[ETH.Log],QueryError>) -> Void) {
        let json: JSON.Dict = [ "id" : ridIncr ]

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

        apiMakeRequestQUERY (network: network, query: zip (Array(queryDict.keys), Array(queryDict.values)), data: json) {
            (res: Result<JSON, QueryError>) in

            completion (res
                .flatMap { (json: JSON) -> Result<[ETH.Log], QueryError> in
                    guard let _ = json.asString (name: "status"),
                        let   _ = json.asString (name: "message"),
                        let   result  = json.asArray (name:  "result")
                        else { return Result.failure(QueryError.model("Missed {status, message, result")) }

                    let logs = result.map { ETH.asLog (json: JSON (dict: $0)) }

                    return logs.contains(where: { nil == $0 })
                        ? Result.failure (QueryError.model ("ETH.Log parse error"))
                        : Result.success (logs as! [ETH.Log])
                })
        }
    }

    public func getTokensAsETH (completion: @escaping (Result<[ETH.Token],QueryError>) -> Void) {

        // Everything returned by BRD must/absolutely-must be in BlockChainDB currencies.  Thus,
        // when stubbed, so too must these.
        apiMakeRequestTOKEN () { (res: Result<[JSON.Dict], QueryError>) in
            completion (res
                .flatMap { (jsonArray: [JSON.Dict]) -> Result<[ETH.Token], QueryError> in
                    let tokens = jsonArray.map { ETH.asToken (json: JSON (dict: $0)) }

                    return tokens.contains(where: { nil == $0 })
                        ? Result.failure (QueryError.model ("ETH.Tokens parse error"))
                        : Result.success (tokens as! [ETH.Token])
                })
        }
    }

    public func getBlocksAsETH (network: String,
                                address: String,
                                interests: UInt32,
                                blockStart: UInt64,
                                blockStop: UInt64,
                                completion: @escaping (Result<[UInt64],QueryError>) -> Void) {
        func parseBlockNumber (_ s: String) -> UInt64? {
            return s.starts(with: "0x")
                ? UInt64 (s.dropFirst(2), radix: 16)
                : UInt64 (s)
        }

        queue.async {
            let semaphore = DispatchSemaphore (value: 0)

            var transactions: [ETH.Transaction] = []
            var transactionsSuccess: Bool = false

            var logs: [ETH.Log] = []
            var logsSuccess: Bool = false

            self.getTransactionsAsETH (network: network,
                                       address: address,
                                       begBlockNumber: blockStart,
                                       endBlockNumber: blockStop) {
                                        (res: Result<[ETH.Transaction],QueryError>) in
                                        res.resolve (
                                            success: {
                                                transactions.append (contentsOf: $0)
                                                transactionsSuccess = true },
                                            failure: { (_) in transactionsSuccess = false })
                                        semaphore.signal() }

            self.getLogsAsETH (network: network,
                               contract: nil,
                               address: address,
                               event: "0xa9059cbb",  // ERC20 Transfer
                               begBlockNumber: blockStart,
                               endBlockNumber: blockStop) {
                                (res: Result<[ETH.Log],QueryError>) in
                                res.resolve (
                                    success: {
                                        logs.append (contentsOf: $0)
                                        logsSuccess = true },
                                    failure: { (_) in logsSuccess = false })
                                semaphore.signal() }

            semaphore.wait()
            semaphore.wait()

            var numbers: [UInt64] = []
            if transactionsSuccess && logsSuccess {
                numbers += transactions
                    .filter {
                        return (
                            /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET */
                            (0 != (interests & UInt32 (1 << 0)) && address == $0.sourceAddr) ||

                                /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET */
                                (0 != (interests & UInt32 (1 << 1)) && address == $0.targetAddr))
                    }
                    .map { parseBlockNumber ($0.blockNumber) ?? 0 }

                numbers += logs
                    .filter {
                        if $0.topics.count != 3 { return false }
                        return (
                            /* CLIENT_GET_BLOCKS_LOGS_AS_SOURCE */
                            (0 != (interests & UInt32 (1 << 2)) && address == $0.topics[1]) ||
                                /* CLIENT_GET_BLOCKS_LOGS_AS_TARGET */
                                (0 != (interests & UInt32 (1 << 3)) && address == $0.topics[2]))
                    }
                    .map { parseBlockNumber($0.blockNumber) ?? 0}

                completion (Result.success (numbers))
            }
            else {
                completion (Result.failure(QueryError.noData))
            }
        }
    }

    public func getBlockNumberAsETH (network: String,
                                     completion: @escaping (Result<String,QueryError>) -> Void) {
        let json: JSON.Dict = [
            "method" : "eth_blockNumber",
            "params" : [],
            "id" : ridIncr
        ]

        apiMakeRequestJSON (network: network, data: json,
                            completion: BlockChainDB.getOneResultString (completion))
    }
    
    public func getNonceAsETH (network: String,
                               address: String,
                               completion: @escaping (Result<String,QueryError>) -> Void) {
        let json: JSON.Dict = [
            "method" : "eth_getTransactionCount",
            "params" : [address, "latest"],
            "id" : ridIncr
        ]

        apiMakeRequestJSON (network: network, data: json,
                            completion: BlockChainDB.getOneResultString (completion))
    }

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

    private func sendRequest<T> (_ request: URLRequest, _ dataTaskFunc: DataTaskFunc, completion: @escaping (Result<T, QueryError>) -> Void) {
        dataTaskFunc (session, request) { (data, res, error) in
            guard nil == error else {
                completion (Result.failure(QueryError.submission (error!))) // NSURLErrorDomain
                return
            }

            guard let res = res as? HTTPURLResponse else {
                completion (Result.failure (QueryError.url ("No Response")))
                return
            }

            guard 200 == res.statusCode else {
                completion (Result.failure (QueryError.response(res.statusCode)))
                return
            }

            guard let data = data else {
                completion (Result.failure (QueryError.noData))
                return
            }

            do {
                guard let json = try JSONSerialization.jsonObject(with: data, options: []) as? T
                    else {
                        print ("SYS: BDB:API: ERROR: JSON.Dict: '\(data.map { String(format: "%c", $0) }.joined())'")
                        completion (Result.failure(QueryError.jsonParse(nil)));
                        return }

                completion (Result.success (json))
            }
            catch let jsonError as NSError {
                print ("SYS: BDB:API: ERROR: JSON.Error: '\(data.map { String(format: "%c", $0) }.joined())'")
                completion (Result.failure (QueryError.jsonParse (jsonError)))
                return
            }
            }.resume()
    }

    internal func makeRequest<T> (_ dataTaskFunc: DataTaskFunc,
                                  _ baseURL: String,
                                  path: String,
                                  query: Zip2Sequence<[String],[String]>? = nil,
                                  data: JSON.Dict? = nil,
                                  httpMethod: String = "POST",
                                  completion: @escaping (Result<T, QueryError>) -> Void) {
        guard var urlBuilder = URLComponents (string: baseURL)
            else { completion (Result.failure(QueryError.url("URLComponents"))); return }

        urlBuilder.path = path.starts(with: "/") ? path : "/\(path)"
        if let query = query {
            urlBuilder.queryItems = query.map { URLQueryItem (name: $0, value: $1) }
        }

        guard let url = urlBuilder.url
            else { completion (Result.failure (QueryError.url("URLComponents.url"))); return }

        print ("SYS: BDB:Request: \(url.absoluteString): Method: \(httpMethod): Data: \(data?.description ?? "[]")")

        var request = URLRequest (url: url)
        request.addValue ("application/json", forHTTPHeaderField: "accept")
        request.addValue ("application/json", forHTTPHeaderField: "Content-Type")
        request.httpMethod = httpMethod

        // If we have data as a JSON.Dict, then add it as the httpBody to the request.
        if let data = data {
            do { request.httpBody = try JSONSerialization.data (withJSONObject: data, options: []) }
            catch let jsonError as NSError {
                completion (Result.failure (QueryError.jsonParse(jsonError)))
            }
        }

        sendRequest (request, dataTaskFunc, completion: completion)
    }

    internal func bdbMakeRequest (path: String,
                                  query: Zip2Sequence<[String],[String]>?,
                                  embedded: Bool = true,
                                  completion: @escaping (Bool, Result<[JSON], QueryError>) -> Void) {
        makeRequest (bdbDataTaskFunc, bdbBaseURL,
                     path: path,
                     query: query,
                     data: nil,
                     httpMethod: "GET") { (res: Result<JSON.Dict, QueryError>) in
                        let res = res.map { JSON (dict: $0) }

                        // See if there is a 'page' Dict in the JSON
                        let page: JSON.Dict? = try! res
                            .map { $0.asDict(name: "page") }
                            .recover { (ignore) in return nil }
                            .get()

                        // The page is full if 'total_pages' is more than 1
                        let full: Bool = page.map { (($0["total_pages"] as? Int) ?? 0) > 1 } ?? false

                        // If called not embedded then never be full
                        // precondition (...)

                        // Callback with `more` and the result (maybe error)
                        completion (false && (embedded && full),
                            res.flatMap { (json: JSON) -> Result<[JSON], QueryError> in
                                let json = (embedded
                                    ? (json.asDict(name: "_embedded")?[path] ?? [])
                                    : [json.dict])

                                guard let data = json as? [JSON.Dict]
                                    else { return Result.failure(QueryError.model ("[JSON.Dict] expected")) }

                                return Result.success (data.map { JSON (dict: $0) })
                        })
        }
    }

    private func apiGetNetworkName (_ name: String) -> String {
        let name = name.lowercased()
        return name == "testnet" ? "ropsten" : name
    }
    
    internal func apiMakeRequestJSON (network: String,
                                      data: JSON.Dict,
                                      completion: @escaping (Result<JSON, QueryError>) -> Void) {
        let path = "/ethq/\(apiGetNetworkName(network))/proxy"
        makeRequest (apiDataTaskFunc, apiBaseURL,
                     path: path,
                     query: nil,
                     data: data,
                     httpMethod: "POST") { (res: Result<JSON.Dict, QueryError>) in
                        completion (res.map { JSON (dict: $0) })
        }
    }

    internal func apiMakeRequestQUERY (network: String,
                                       query: Zip2Sequence<[String],[String]>?,
                                       data: JSON.Dict,
                                       completion: @escaping (Result<JSON, QueryError>) -> Void) {
        let path = "/ethq/\(apiGetNetworkName(network))/query"
        makeRequest (apiDataTaskFunc, apiBaseURL,
                     path: path,
                     query: query,
                     data: data,
                     httpMethod: "POST") { (res: Result<JSON.Dict, QueryError>) in
                        completion (res.map { JSON (dict: $0) })
        }
    }

    internal func apiMakeRequestTOKEN (completion: @escaping (Result<[JSON.Dict], QueryError>) -> Void) {
        let path = "/currencies"
        makeRequest (apiDataTaskFunc, apiBaseURL,
                     path: path,
                     query: zip(["type"], ["erc20"]),
                     data: nil,
                     httpMethod: "GET",
                     completion: completion)
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

    ///
    /// Given JSON extract a value and then apply a completion
    ///
    /// - Parameters:
    ///   - extract: A function that extracts the "result" field from JSON to return T?
    ///   - completion: A function to process a Result on T
    ///
    /// - Returns: A function to process a Result on JSON
    ///
    private static func getOneResult<T> (_ extract: @escaping (JSON) -> (String) ->T?,
                                         _ completion: @escaping (Result<T,QueryError>) -> Void) -> ((Result<JSON,QueryError>) -> Void) {
        return { (res: Result<JSON,QueryError>) in
            completion (res.flatMap {
                extract ($0)("result").map { Result.success ($0) } // extract()() returns an optional
                    ?? Result<T,QueryError>.failure(QueryError.noData) })
        }
    }


    /// Given JSON extract a value with JSON.asString (returning String?) and then apply a completion
    ///
    /// - Parameter completion: A function to process a Result on String
    ///
    /// - Returns: A function to process a Result on JSON
    ///
    private static func getOneResultString (_ completion: @escaping (Result<String,QueryError>) -> Void) -> ((Result<JSON,QueryError>) -> Void) {
        return getOneResult (JSON.asString, completion)
    }
}
