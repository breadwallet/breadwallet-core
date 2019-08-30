//
//  BRSystem.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import Foundation  // Data, DispatchQueue
import BRCryptoC

///
/// System (a singleton)
///
public final class System {

    /// The listener.  Gets all events for {Network, WalletManger, Wallet, Transfer}
    public private(set) weak var listener: SystemListener?

    /// The account
    public let account: Account

    /// The path for persistent storage
    public let path: String

    /// If on mainnet
    public let onMainnet: Bool

    /// The 'blockchain DB' to use for BRD Server Assisted queries
    public let query: BlockChainDB

    internal let queue = DispatchQueue (label: "Crypto System")

    /// the networks, unsorted.
    public internal(set) var networks: [Network] = []

    internal let callbackCoordinator: SystemCallbackCoordinator

    /// We define default blockchains but these are wholly insufficient given that the
    /// specfication includes `blockHeight` (which can never be correct).

    static let defaultBlockchains: [BlockChainDB.Model.Blockchain] = [
        // Mainnet
        (id: "bitcoin-mainnet",       name: "Bitcoin",       network: "mainnet", isMainnet: true,  currency: "btc", blockHeight: 0,
         feeEstimates: [(amount: "30", tier: "10m", confirmationTimeInMilliseconds: 10 * 60 * 1000)]),
        (id: "bitcoin-cash-mainnet",  name: "Bitcoin Cash",  network: "mainnet", isMainnet: true,  currency: "bch", blockHeight: 0,
         feeEstimates: [(amount: "30", tier: "10m", confirmationTimeInMilliseconds: 10 * 60 * 1000)]),
        (id: "ethereum-mainnet",      name: "Ethereum",      network: "mainnet", isMainnet: true,  currency: "eth", blockHeight: 0,
         feeEstimates: [(amount: "2000000000", tier: "1m", confirmationTimeInMilliseconds: 1 * 60 * 1000)]),
        (id: "ripple-mainnet",        name: "Ripple",        network: "mainnet", isMainnet: true,  currency: "xrp", blockHeight: nil,
         feeEstimates: [(amount: "20", tier: "1m", confirmationTimeInMilliseconds: 1 * 60 * 1000)]),

        // Testnet
        (id: "bitcoin-testnet",       name: "Bitcoin Test",      network: "testnet", isMainnet: false, currency: "btc", blockHeight: 0,
         feeEstimates: [(amount: "30", tier: "10m", confirmationTimeInMilliseconds: 10 * 60 * 1000)]),
        (id: "bitcoin-cash-testnet",  name: "Bitcoin Cash Test", network: "testnet", isMainnet: false, currency: "bch", blockHeight: 0,
         feeEstimates: [(amount: "30", tier: "10m", confirmationTimeInMilliseconds: 10 * 60 * 1000)]),
        (id: "ethereum-ropsten",      name: "Ethereum Testnet",  network: "testnet", isMainnet: false, currency: "eth", blockHeight: 0,
         feeEstimates: [(amount: "2000000000", tier: "1m", confirmationTimeInMilliseconds: 1 * 60 * 1000)]),
        (id: "ripple-testnet",        name: "Ripple Testnet",    network: "testnet", isMainnet: false, currency: "xrp", blockHeight: nil,
         feeEstimates: [(amount: "20", tier: "1m", confirmationTimeInMilliseconds: 1 * 60 * 1000)]),
    ]

    static let defaultCurrencies: [BlockChainDB.Model.Currency] = [
        // Mainnet
        (id: "Bitcoin", name: "Bitcoin", code: "btc", type: "native", blockchainID: "bitcoin-mainnet",
         address: nil, verified: true,
         demoninations: [(name: "satoshi", code: "sat", decimals: 0, symbol: BlockChainDB.Model.lookupSymbol ("sat")),
                         (name: "bitcoin", code: "btc", decimals: 8, symbol: BlockChainDB.Model.lookupSymbol ("btc"))]),

        (id: "Bitcoin-Cash", name: "Bitcoin Cash", code: "bch", type: "native", blockchainID: "bitcoin-cash-mainnet",
         address: nil, verified: true,
         demoninations: [(name: "satoshi",      code: "sat", decimals: 0, symbol: BlockChainDB.Model.lookupSymbol ("sat")),
                         (name: "bitcoin cash", code: "bch", decimals: 8, symbol: BlockChainDB.Model.lookupSymbol ("bch"))]),

        (id: "Ethereum", name: "Ethereum", code: "eth", type: "native", blockchainID: "ethereum-mainnet",
         address: nil, verified: true,
         demoninations: [(name: "wei",   code: "wei",  decimals:  0, symbol: BlockChainDB.Model.lookupSymbol ("wei")),
                         (name: "gwei",  code: "gwei", decimals:  9, symbol: BlockChainDB.Model.lookupSymbol ("gwei")),
                         (name: "ether", code: "eth",  decimals: 18, symbol: BlockChainDB.Model.lookupSymbol ("eth"))]),

        (id: "BRD Token", name: "BRD Token", code: "brd", type: "erc20", blockchainID: "ethereum-mainnet",
         address: BlockChainDB.Model.addressBRDMainnet, verified: true,
         demoninations: [(name: "BRD_INTEGER",   code: "BRDI",  decimals:  0, symbol: "brdi"),
                         (name: "BRD",           code: "BRD",   decimals: 18, symbol: "brd")]),

        (id: "EOS Token", name: "EOS Token", code: "eos", type: "erc20", blockchainID: "ethereum-mainnet",
         address: "0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0", verified: true,
         demoninations: [(name: "EOS_INTEGER",   code: "EOSI",  decimals:  0, symbol: "eosi"),
                         (name: "EOS",           code: "EOS",   decimals: 18, symbol: "eos")]),

        (id: "Ripple", name: "Ripple", code: "xrp", type: "native", blockchainID: "ripple-mainnet",
         address: nil, verified: true,
         demoninations: [(name: "drop", code: "drop", decimals: 0, symbol: "drop"),
                         (name: "xrp",  code: "xrp",  decimals: 6, symbol: "xrp")]),

        // Testnet
        (id: "Bitcoin-Testnet", name: "Bitcoin", code: "btc", type: "native", blockchainID: "bitcoin-testnet",
         address: nil, verified: true,
         demoninations: [(name: "satoshi", code: "sat", decimals: 0, symbol: BlockChainDB.Model.lookupSymbol ("sat")),
                         (name: "bitcoin", code: "btc", decimals: 8, symbol: BlockChainDB.Model.lookupSymbol ("btc"))]),

        (id: "Bitcoin-Cash-Testnet", name: "Bitcoin Cash Test", code: "bch", type: "native", blockchainID: "bitcoin-cash-testnet",
         address: nil, verified: true,
         demoninations: [(name: "satoshi",           code: "sat", decimals: 0, symbol: BlockChainDB.Model.lookupSymbol ("sat")),
                         (name: "bitcoin cash test", code: "bch", decimals: 8, symbol: BlockChainDB.Model.lookupSymbol ("bch"))]),

        (id: "Ethereum-Testnet", name: "Ethereum", code: "eth", type: "native", blockchainID: "ethereum-ropsten",
         address: nil, verified: true,
         demoninations: [(name: "wei",   code: "wei",  decimals:  0, symbol: BlockChainDB.Model.lookupSymbol ("wei")),
                         (name: "gwei",  code: "gwei", decimals:  9, symbol: BlockChainDB.Model.lookupSymbol ("gwei")),
                         (name: "ether", code: "eth",  decimals: 18, symbol: BlockChainDB.Model.lookupSymbol ("eth"))]),

        (id: "BRD Token Testnet", name: "BRD Token", code: "brd", type: "erc20", blockchainID: "ethereum-ropsten",
         address: BlockChainDB.Model.addressBRDTestnet, verified: true,
         demoninations: [(name: "BRD_INTEGER",   code: "BRDI",  decimals:  0, symbol: "brdi"),
                         (name: "BRD",           code: "BRD",   decimals: 18, symbol: "brd")]),

        (id: "Ripple", name: "Ripple", code: "xrp", type: "native", blockchainID: "ripple-testnet",
         address: nil, verified: true,
         demoninations: [(name: "drop", code: "drop", decimals: 0, symbol: "drop"),
                         (name: "xrp",  code: "xrp",  decimals: 6, symbol: "xrp")]),
    ]

    ///
    /// Address Scheme
    ///

    var supportedAddressSchemesMap: [String:[AddressScheme]] = [
        "bitcoin-mainnet":      [.btcSegwit, .btcLegacy],
        "bitcoin-cash-mainnet": [.btcLegacy],
        "ethereum-mainnet":     [.ethDefault],
        "ripple-mainnet":       [.genDefault],
        "bitcoin-testnet":      [.btcSegwit, .btcLegacy],
        "bitcoin-cash-testnet": [.btcLegacy],
        "ethereum-ropsten":     [.ethDefault],
        "ripple-testnet":       [.genDefault]
    ]

    var defaultAddressSchemeMap: [String:AddressScheme] = [
        "bitcoin-mainnet":      .btcSegwit,
        "bitcoin-cash-mainnet": .btcLegacy,
        "ethereum-mainnet":     .ethDefault,
        "ripple-mainnet":       .genDefault,
        "bitcoin-testnet":      .btcSegwit,
        "bitcoin-cash-testnet": .btcLegacy,
        "ethereum-ropsten":     .ethDefault,
        "ripple-testnet":       .genDefault
    ]

    ///
    /// Return the AddressSchemes support for `network`
    ///
    /// - Parameter network: the network
    ///
    /// - Returns: An array of AddressScheme
    ///
    public func supportedAddressSchemes (network: Network) -> [AddressScheme] {
        return supportedAddressSchemesMap[network.uids] ?? [.genDefault]
    }

    ///
    /// Check if `network` supports `scheme`
    ///
    /// - Parameters:
    ///   - network: the network
    ///   - scheme: the scheme
    ///
    /// - Returns: If supported `true`; otherwise `false`.
    ///
    public func supportsAddressScheme (network: Network, _ scheme: AddressScheme) -> Bool {
        return supportedAddressSchemes(network: network).contains (scheme)
    }

    ///
    /// Return the default AddressScheme for `network`
    ///
    /// - Parameter network: the network
    ///
    /// - Returns: The default AddressScheme
    ///
    public func defaultAddressScheme (network: Network) -> AddressScheme {
        return defaultAddressSchemeMap[network.uids] ?? .genDefault
    }

    ///
    /// Wallet Manager Modes
    ///

    var supportedModesMap: [String:[WalletManagerMode]] = [
        "bitcoin-mainnet":      [.p2p_only],
        "bitcoin-cash-mainnet": [.p2p_only],
        "ethereum-mainnet":     [.api_only, .api_with_p2p_submit, .p2p_only],
//        "ripple-mainnet":       [],
        "bitcoin-testnet":      [.p2p_only],
        "bitcoin-cash-testnet": [.p2p_only],
        "ethereum-ropsten":     [.api_only, .api_with_p2p_submit, .p2p_only],
//        "ripple-testnet":       []
    ]

    var defaultModesMap: [String:WalletManagerMode] = [
        "bitcoin-mainnet":      .p2p_only,
        "bitcoin-cash-mainnet": .p2p_only,
        "ethereum-mainnet":     .api_only,
//        "ripple-mainnet":       [],
        "bitcoin-testnet":      .p2p_only,
        "bitcoin-cash-testnet": .p2p_only,
        "ethereum-ropsten":     .api_only,
//        "ripple-testnet":       []
    ]


    /// Return the WalletManagerModes supported by `network`
    ///
    /// - Parameter network: the network
    ///
    /// - Returns: an aray of WalletManagerMode
    ///
    public func supportedModes (network: Network) -> [WalletManagerMode] {
        return supportedModesMap[network.uids] ?? [.api_only]
    }

    ///
    /// Check if `network` supports `mode`
    ///
    /// - Parameters:
    ///   - network: the network
    ///   - mode: the mode
    ///
    /// - Returns: If supported `true`; otherwise `false`
    ///
    public func supportsMode (network: Network, _ mode: WalletManagerMode) -> Bool {
        return supportedModes (network: network).contains (mode)
    }

    ///
    /// Return the default WalletManagerMode for `network`
    ///
    /// - Parameter network: the network
    ///
    /// - Returns: the default mode
    ///
    public func defaultMode (network: Network) -> WalletManagerMode {
        return defaultModesMap[network.uids] ?? .api_only
    }

    ///
    /// Add `network` to `networks`
    ///
    /// - Parameter network: the network to add
    ///
    internal func add (network: Network) {
        networks.append (network)
        announceEvent (SystemEvent.networkAdded(network: network))
    }

    internal func networkBy (uids: String) -> Network? {
        return networks.first { $0.uids == uids }
    }

    /// The system's Wallet Managers, unsorted.  A WalletManager will hold an 'unowned'
    /// reference back to `System`
    public internal(set) var managers: [WalletManager] = [];

    internal func managerBy (core: BRCryptoWalletManager) -> WalletManager? {
        return managers
            .first { $0.core == core }
    }

    ///
    /// Add `manager` to `managers`.  Will signal WalletManagerEvent.created and then
    /// SystemEvent.managerAdded is `manager` is added.
    ///
    /// - Parameter manager: the manager to add.
    ///
    internal func add (manager: WalletManager) {
        if !managers.contains(where: { $0 === manager }) {
            managers.append (manager)
            announceEvent (SystemEvent.managerAdded(manager: manager))
        }
    }

    ///
    /// Create a wallet manager for `network` using `mode.
    ///
    /// - Parameters:
    ///   - network: the wallet manager's network
    ///   - mode: the mode to use
    ///
    public func createWalletManager (network: Network,
                                     mode: WalletManagerMode,
                                     addressScheme: AddressScheme) {
        
        let manager = WalletManager (system: self,
                                     callbackCoordinator: callbackCoordinator,
                                     account: account,
                                     network: network,
                                     mode: mode,
                                     addressScheme: addressScheme,
                                     storagePath: path,
                                     listener: cryptoListener,
                                     client: cryptoClient)

        self.add (manager: manager)
    }

    // Wallets - derived as a 'flatMap' of the managers' wallets.
    public var wallets: [Wallet] {
        return managers.flatMap { $0.wallets }
    }
    
    public init (listener: SystemListener,
                 account: Account,
                 onMainnet: Bool,
                 path: String,
                 query: BlockChainDB) {
        self.listener  = listener
        self.account   = account
        self.onMainnet = onMainnet
        self.path  = path
        self.query = query
        self.callbackCoordinator = SystemCallbackCoordinator()

        let _ = System.systemExtend(with: self)
        listener.handleSystemEvent (system: self, event: SystemEvent.created)
    }

    internal func announceEvent (_ event: SystemEvent) {
        listener?.handleSystemEvent (system: self,
                                     event: event)
    }

    ///
    /// Subscribe (or unsubscribe) to BlockChainDB notifications.  Notifications provide an
    /// asynchronous announcement of DB changes pertinent to the User and are used to avoid
    /// polling the DB for such changes.
    ///
    /// The Subscription includes an `endpoint` which is optional.  If provided, subscriptions
    /// are enabled; if not provided, subscriptions are disabled.  Disabling a sbuscription is
    /// required, even though polling in undesirable, because Notifications are user configured.
    ///
    /// - Parameter subscription: the subscription to enable or to disable notifications
    ///
    public func subscribe (using subscription: BlockChainDB.Subscription) {
        self.query.subscribe (walletId: account.uids,
                              subscription: subscription)
    }

    ///
    /// Announce a BlockChainDB transaction.  This should be called upon the "System User's"
    /// receipt of a BlockchainDB notification.
    ///
    /// - Parameters:
    ///   - transaction: the transaction id which can be used in `getTransfer` to query the
    ///         blockchainDB for details on the transaction
    ///   - data: The transaction JSON data (a dictionary) if available
    ///
    public func announce (transaction id: String, data: [String: Any]) {
        print ("SYS: Announce: \(id)")
    }

    internal func updateSubscribedWallets () {
        let currencyKeyValues = wallets.map { ($0.currency.code, [$0.source.description]) }
        let wallet = (id: account.uids,
                      currencies: Dictionary (uniqueKeysWithValues: currencyKeyValues))
        self.query.updateWallet (wallet) { (res: Result<BlockChainDB.Model.Wallet, BlockChainDB.QueryError>) in
            print ("SYS: SubscribedWallets: \(res)")
        }
    }

    /// MARK: - Network Fees

    ////
    /// A NetworkFeeUpdateError
    ///
    public enum NetworkFeeUpdateError: Error {
        /// The query endpoint for netowrk fees is unresponsive
        case feesUnavailable
    }

    ///
    /// Update the NetworkFees for all known networks.  This will query the `BlockChainDB` to
    /// acquire the fee information and then update each of system's networks with the new fee
    /// structure.  Each updated network will generate a NetworkEvent.feesUpdated event (even if
    /// the actual fees did not change).
    ///
    /// And optional completion handler can be provided.  If provided the completion handler is
    /// invoked with an array of the networks that were updated or with an error.
    ///
    /// It is appropriate to call this function anytime a network's fees are to be used, such as
    /// when a transfer is created and the User can choose among the different fees.
    ///
    /// - Parameter completion: An optional completion handler
    ///
    public func updateNetworkFees (_ completion: ((Result<[Network],NetworkFeeUpdateError>) -> Void)? = nil) {
        self.query.getBlockchains (mainnet: self.onMainnet) {
            (blockChainResult: Result<[BlockChainDB.Model.Blockchain],BlockChainDB.QueryError>) in

            // On an error, just skip out; we'll query again later, presumably
            guard case let .success (blockChainModels) = blockChainResult
                else {
                    completion? (Result.failure (NetworkFeeUpdateError.feesUnavailable))
                    return
            }

            let networks = blockChainModels.compactMap { (blockChainModel: BlockChainDB.Model.Blockchain) -> Network? in
                guard let network = self.networkBy (uids: blockChainModel.id)
                    else { return nil }

                // We always have a feeUnit for network
                let feeUnit = network.baseUnitFor(currency: network.currency)!

                // Get the fees
                let fees = blockChainModel.feeEstimates
                    // Well, quietly ignore a fee if we can't parse the amount.
                    .compactMap { (fee: BlockChainDB.Model.BlockchainFee) -> NetworkFee? in
                        let timeInterval  = 1000 * 60 * Int (fee.tier.dropLast())!
                        return Amount.create (string: fee.amount, unit: feeUnit)
                            .map { NetworkFee (timeIntervalInMilliseconds: UInt64(timeInterval),
                                               pricePerCostFactor: $0) }
                }

                // The fees are unlikely to change; but we'll announce .feesUpdated anyways.
                network.fees = fees
                self.listener?.handleNetworkEvent (system: self, network: network, event: .feesUpdated)

                return network
            }

            completion? (Result.success(networks))
        }
    }

    ///
    /// Stop the system.  All managers are disconnected.  Will inhibit `System` processing.
    ///
    public func stop () {
        managers.forEach { $0.disconnect() }
    }

    public func configureMergeBlockchains (builtin: [BlockChainDB.Model.Blockchain],
                                           remote:  [BlockChainDB.Model.Blockchain]) -> [BlockChainDB.Model.Blockchain] {
        // Both `builtin` and `remote` have a non-null blockHeight -> supported.

        // For existing remotes:
        remote.forEach { (blockchain: BlockChainDB.Model.Blockchain) in
            // 1) api_only is a supported mode
            let modes = supportedModesMap[blockchain.id]
            supportedModesMap[blockchain.id] = (nil == modes
                ? [.api_only]
                : (modes!.contains (.api_only)
                    ? modes!
                    : ([.api_only] + modes!)))

            // 2) api_only is a default mode if a default doesn't exist
            if nil == defaultModesMap[blockchain.id] {
                defaultModesMap[blockchain.id] = .api_only
            }
        }

        // Merge builtin into remote
        return remote.unionOf(builtin) { $0.id }
    }

    ///
    /// Configure the system.  This will query various BRD services, notably the BlockChainDB, to
    /// establish the available networks (aka blockchains) and their currencies.  For each
    /// `Network` there will be `SystemEvent` which can be used by the App to create a
    /// `WalletManager`
    ///
    /// @Note: This should only be called one.
    ///
    public func configure () {
        func currencyDenominationToBaseUnit (currency: Currency, model: BlockChainDB.Model.CurrencyDenomination) -> Unit {
            let uids = "\(currency.name)-\(model.code)"
            return Unit (currency: currency, uids: uids, name: model.name, symbol: model.symbol)
        }

        func currencyToDefaultBaseUnit (currency: Currency) -> Unit {
            let symb = "\(currency.code.uppercased())I"
            let name = "\(currency.code.uppercased())_INTEGER"
            let uids = "\(currency.name)-\(name)"
            return Unit (currency: currency, uids: uids, name: name, symbol: symb)
        }

        func currencyDenominationToUnit (currency: Currency, model: BlockChainDB.Model.CurrencyDenomination, base: Unit) -> Unit {
            let uids = "\(currency.name)-\(model.code)"
            return Unit (currency: currency, uids: uids, name: model.name, symbol: model.symbol, base: base, decimals: model.decimals)
        }

        // query blockchains
        self.query.getBlockchains (mainnet: self.onMainnet) { (blockchainResult: Result<[BlockChainDB.Model.Blockchain],BlockChainDB.QueryError>) in
            // Filter our defaults to be `self.onMainnet` and supported (non-nil blockHeight)
            let blockChainModelsDefaults = System.defaultBlockchains
                .filter { $0.isMainnet == self.onMainnet && nil != $0.blockHeight }

            let blockChainModels = blockchainResult
                // Only supported blockchains, but we'll merge in the defaults to handle
                // blockchains not supported by BDB.
                .map { $0.filter { nil != $0.blockHeight } }
                // On error, return [] - we'll use defaults
                .getWithRecovery { (ignore) in return [] }

            self.configureMergeBlockchains (builtin: blockChainModelsDefaults,
                                            remote: blockChainModels)
                .forEach { (blockchainModel: BlockChainDB.Model.Blockchain) in

                    // query currencies
                    self.query.getCurrencies (blockchainId: blockchainModel.id) { (currencyResult: Result<[BlockChainDB.Model.Currency],BlockChainDB.QueryError>) in
                        // Find applicable defaults by `blockchainID`
                        let defaults = System.defaultCurrencies
                            .filter { $0.blockchainID == blockchainModel.id }

                        let currencyModels = try! currencyResult
                            // On success, always merge `defaultCurrencies`
                            .map { $0.unionOf (defaults) { $0.id }}
                            // On error, use `defaults`
                            .recover { (error: BlockChainDB.QueryError) -> [BlockChainDB.Model.Currency] in
                                return defaults
                            }.get()

                        var associations: [Currency : Network.Association] = [:]

                        // Update associations
                        currencyModels
                            // TODO: Only needed if getCurrencies returns the wrong stuff.
                            .filter { $0.blockchainID == blockchainModel.id }
                            .filter { $0.verified }
                            .forEach { (currencyModel: BlockChainDB.Model.Currency) in
                                // Create the currency
                                let currency = Currency (uids: currencyModel.id,
                                                         name: currencyModel.name,
                                                         code: currencyModel.code,
                                                         type: currencyModel.type,
                                                         issuer: currencyModel.address)

                                // Create the base unit
                                let baseUnit: Unit = currencyModel.demoninations.first { 0 == $0.decimals }
                                    .map { currencyDenominationToBaseUnit(currency: currency, model: $0) }
                                    ?? currencyToDefaultBaseUnit (currency: currency)

                                // Create the other units
                                var units: [Unit] = [baseUnit]
                                units += currencyModel.demoninations.filter { 0 != $0.decimals }
                                    .map { currencyDenominationToUnit (currency: currency, model: $0, base: baseUnit) }

                                // Find the default unit
                                let maximumDecimals = units.reduce (0) { max ($0, $1.decimals) }
                                let defaultUnit = units.first { $0.decimals == maximumDecimals }!

                                // Update associations
                                associations[currency] = Network.Association (baseUnit: baseUnit,
                                                                              defaultUnit: defaultUnit,
                                                                              units: Set<Unit>(units))
                        }

                        // the default currency
                        guard let currency = associations.keys.first (where: { $0.code == blockchainModel.currency.lowercased() }),
                            let feeUnit = associations[currency]?.baseUnit
                            else { print ("SYS: CONFIGURE: Missed Currency (\(blockchainModel.currency)) on '\(blockchainModel.network)': defaultUnit"); return }

                        let fees = blockchainModel.feeEstimates
                            // Well, quietly ignore a fee if we can't parse the amount.
                            .compactMap { (fee: BlockChainDB.Model.BlockchainFee) -> NetworkFee? in
                                let timeInterval  = fee.confirmationTimeInMilliseconds
                                return Amount.create (string: fee.amount, unit: feeUnit)
                                    .map { NetworkFee (timeIntervalInMilliseconds: timeInterval,
                                                       pricePerCostFactor: $0) }
                        }

                        guard !fees.isEmpty
                            else { print ("SYS: CONFIGURE: Missed Fees (\(blockchainModel.name)) on '\(blockchainModel.network)'"); return }

                        // define the network
                        let network = Network (uids: blockchainModel.id,
                                               name: blockchainModel.name,
                                               isMainnet: blockchainModel.isMainnet,
                                               currency: currency,
                                               height: blockchainModel.blockHeight!,
                                               associations: associations,
                                               fees: fees)

                        // save the network
                        self.networks.append (network)

                        // Invoke callbacks.
                        self.listener?.handleNetworkEvent (system: self, network: network, event: NetworkEvent.created)
                        self.listener?.handleSystemEvent  (system: self, event: SystemEvent.networkAdded(network: network))
                    }
            }
        }
    }

    //
    // Static Weak System References
    //
    static var systemIndex: Int32 = 0;
    static var systemMapping: [Int32 : Weak<System>] = [:]

    static func systemLookup (index: Int32) -> System? {
        return systemMapping[index]?.value
    }

    static func systemExtend (with system: System) -> Int32 {
        system.index = OSAtomicIncrement32(&systemIndex)
        systemMapping[system.index] = Weak (value: system)
        return system.index
    }

    static func systemExtract (_ context: BRCryptoCWMListenerContext!,
                               _ cwm: BRCryptoWalletManager!) -> (System, WalletManager)? {
        precondition (nil != context  && nil != cwm)

        let index = 1 + Int32(UnsafeMutableRawPointer(bitPattern: 1)!.distance(to: context))

        return systemLookup(index: index)
            .map { ($0, WalletManager (core: cwm,
                                       system: $0,
                                       callbackCoordinator: $0.callbackCoordinator,
                                       take: true))
        }
    }

    static func systemExtract (_ context: BRCryptoCWMListenerContext!,
                               _ cwm: BRCryptoWalletManager!,
                               _ wid: BRCryptoWallet!) -> (System, WalletManager, Wallet)? {
        precondition (nil != context  && nil != cwm && nil != wid)

        return systemExtract (context, cwm)
            .map { ($0.0,
                    $0.1,
                    $0.1.walletByCoreOrCreate (wid, create: true)!)
        }
    }

    static func systemExtract (_ context: BRCryptoCWMListenerContext!,
                               _ cwm: BRCryptoWalletManager!,
                               _ wid: BRCryptoWallet!,
                               _ tid: BRCryptoTransfer!) -> (System, WalletManager, Wallet, Transfer)? {
        precondition (nil != context  && nil != cwm && nil != wid && nil != tid)

        // create -> CRYPTO_TRANSFER_EVENT_CREATED == event.type
        return systemExtract (context, cwm, wid)
            .map { ($0.0,
                    $0.1,
                    $0.2,
                    $0.2.transferByCoreOrCreate (tid, create: true)!)
        }
    }

    /// The index of this system
    var index: Int32 = 0

    var systemContext: BRCryptoCWMClientContext? {
        let index = Int(self.index)
        return UnsafeMutableRawPointer (bitPattern: index)
    }
}

public enum SystemEvent {
    case created
    case networkAdded (network: Network)
    case managerAdded (manager: WalletManager)
}

///
/// A SystemListener recieves asynchronous events announcing state changes to Networks, to Managers,
/// to Wallets and to Transfers.  This is an application's sole mechanism to learn of asynchronous
/// state changes.
///
/// Note: This must be 'class bound' as System holds a 'weak' reference (for GC reasons).
///
public protocol SystemListener : /* class, */ WalletManagerListener, WalletListener, TransferListener, NetworkListener {
    ///
    /// Handle a System Event
    ///
    /// - Parameters:
    ///   - system: the system
    ///   - event: the event
    ///
    func handleSystemEvent (system: System,
                            event: SystemEvent)
}

/// A Functional Interface for a Handler
public typealias SystemEventHandler = (System, SystemEvent) -> Void

public final class SystemCallbackCoordinator {
    enum Handler {
        case walletFeeEstimate (Wallet.EstimateFeeHandler)
    }

    var index: Int32 = 0;
    var handlers: [Int32: Handler] = [:]

    var cookie: UnsafeMutableRawPointer {
        let index = Int(self.index)
        return UnsafeMutableRawPointer (bitPattern: index)!
    }

    func cookieToIndex (_ cookie: UnsafeMutableRawPointer) -> Int32 {
        return 1 + Int32(UnsafeMutableRawPointer(bitPattern: 1)!.distance(to: cookie))
    }

    public func addWalletFeeEstimateHandler(_ handler: @escaping Wallet.EstimateFeeHandler) -> UnsafeMutableRawPointer {
        index = OSAtomicIncrement32 (&index)
        handlers[index] = Handler.walletFeeEstimate(handler)
        return cookie
    }

    func remWalletFeeEstimateHandler (_ cookie: UnsafeMutableRawPointer) -> Wallet.EstimateFeeHandler? {
        return handlers.removeValue (forKey: cookieToIndex(cookie))
            .flatMap {
                switch $0 {
                case .walletFeeEstimate (let handler): return handler
                }
        }
    }
    func handleWalletFeeEstimateSuccess (_ cookie: UnsafeMutableRawPointer, estimate: TransferFeeBasis) {
        if let handler = remWalletFeeEstimateHandler(cookie) {
            handler (Result.success (estimate))
        }
    }

    func handleWalletFeeEstimateFailure (_ cookie: UnsafeMutableRawPointer, error: Wallet.FeeEstimationError) {
        if let handler = remWalletFeeEstimateHandler(cookie) {
            handler (Result.failure(error))
        }
    }
}


extension System {
    internal var cryptoListener: BRCryptoCWMListener {
        return BRCryptoCWMListener (
            context: systemContext,

            walletManagerEventCallback: { (context, cwm, event) in
                precondition (nil != context  && nil != cwm)
                defer { cryptoWalletManagerGive(cwm) }

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: Event: \(event.type): Missed {cwm}"); return }

                print ("SYS: Event: Manager (\(manager.name)): \(event.type)")

                switch event.type {
                case CRYPTO_WALLET_MANAGER_EVENT_CREATED:
                    system.listener?.handleManagerEvent (system: manager.system,
                                                         manager: manager,
                                                         event: WalletManagerEvent.created)

                case CRYPTO_WALLET_MANAGER_EVENT_CHANGED:
                    print ("SYS: Event: Manager (\(manager.name)): \(event.type): {\(WalletManagerState (core: event.u.state.oldValue)) -> \(WalletManagerState (core: event.u.state.newValue))}")

                    system.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.changed (oldState: WalletManagerState (core: event.u.state.oldValue),
                                                                                             newState: WalletManagerState (core: event.u.state.newValue)))

                case CRYPTO_WALLET_MANAGER_EVENT_DELETED:
                    system.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.deleted)

                case CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED:
                    defer { if let wid = event.u.wallet.value { cryptoWalletGive (wid) }}
                    guard let wallet = manager.walletBy (core: event.u.wallet.value)
                        else { print ("SYS: Event: \(event.type): Missed (wallet)"); return }
                    system.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.walletAdded (wallet: wallet))

                case CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED:
                    defer { if let wid = event.u.wallet.value { cryptoWalletGive (wid) }}
                    guard let wallet = manager.walletBy (core: event.u.wallet.value)
                        else { print ("SYS: Event: \(event.type): Missed (wallet)"); return }
                    system.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.walletChanged(wallet: wallet))

                case CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED:
                    defer { if let wid = event.u.wallet.value { cryptoWalletGive (wid) }}
                    guard let wallet = manager.walletBy (core: event.u.wallet.value)
                        else { print ("SYS: Event: \(event.type): Missed (wallet)"); return }
                    system.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.walletDeleted(wallet: wallet))

                case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED:
                    system.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.syncStarted)

                case CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES:
                    let timestamp: Date? = (0 == event.u.sync.timestamp // CRYPTO_NO_SYNC_TIMESTAMP
                        ? nil
                        : Date (timeIntervalSince1970: TimeInterval(event.u.sync.timestamp)))
                    
                    system.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.syncProgress (
                                                            timestamp: timestamp,
                                                            percentComplete: event.u.sync.percentComplete))

                case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED:
                    system.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.syncEnded(error: nil))

                case CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED:
                    manager.network.height = event.u.blockHeight.value

                default: precondition(false)
                }
        },

            walletEventCallback: { (context, cwm, wid, event) in
                precondition (nil != context  && nil != cwm && nil != wid)
                defer { cryptoWalletManagerGive(cwm); cryptoWalletGive(wid) }

                guard let (system, manager, wallet) = System.systemExtract (context, cwm, wid)
                    else { print ("SYS: Event: \(event.type): Missed {cwm, wid}"); return }

                print ("SYS: Event: Wallet (\(wallet.name)): \(event.type)")

                switch event.type {
                case CRYPTO_WALLET_EVENT_CREATED:
                    system.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event:  WalletEvent.created)

                case CRYPTO_WALLET_EVENT_CHANGED:
                    system.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.changed (oldState: WalletState (core: event.u.state.oldState),
                                                                                    newState: WalletState (core: event.u.state.newState)))
                case CRYPTO_WALLET_EVENT_DELETED:
                    system.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.deleted)

                case CRYPTO_WALLET_EVENT_TRANSFER_ADDED:
                    defer { if let tid = event.u.transfer.value { cryptoTransferGive(tid) }}
                    guard let transfer = wallet.transferBy (core: event.u.transfer.value)
                        else { print ("SYS: Event: \(event.type): Missed (transfer)"); return }
                    system.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.transferAdded (transfer: transfer))

                case CRYPTO_WALLET_EVENT_TRANSFER_CHANGED:
                    defer { if let tid = event.u.transfer.value { cryptoTransferGive(tid) }}
                    guard let transfer = wallet.transferBy (core: event.u.transfer.value)
                        else { print ("SYS: Event: \(event.type): Missed (transfer)"); return }
                    system.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.transferChanged (transfer: transfer))
                case CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED:
                    defer { if let tid = event.u.transfer.value { cryptoTransferGive(tid) }}
                    guard let transfer = wallet.transferBy (core: event.u.transfer.value)
                        else { print ("SYS: Event: \(event.type): Missed (transfer)"); return }
                    system.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.transferSubmitted (transfer: transfer, success: true))

                case CRYPTO_WALLET_EVENT_TRANSFER_DELETED:
                    defer { if let tid = event.u.transfer.value { cryptoTransferGive(tid) }}
                    guard let transfer = wallet.transferBy (core: event.u.transfer.value)
                        else { print ("SYS: Event: \(event.type): Missed (transfer)"); return }
                    system.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.transferDeleted (transfer: transfer))

                case CRYPTO_WALLET_EVENT_BALANCE_UPDATED:
                    let amount = Amount (core: event.u.balanceUpdated.amount,
                                         take: false)
                    system.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.balanceUpdated(amount: amount))

                case CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED:
                    let feeBasis = TransferFeeBasis (core: event.u.feeBasisUpdated.basis, take: false)
                    system.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.feeBasisUpdated(feeBasis: feeBasis))

                case CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED:
                    let cookie = event.u.feeBasisEstimated.cookie!
                    if CRYPTO_SUCCESS == event.u.feeBasisEstimated.status,
                        let feeBasis = event.u.feeBasisEstimated.basis
                            .map ({ TransferFeeBasis (core: $0, take: false) }) {
                        system.callbackCoordinator.handleWalletFeeEstimateSuccess (cookie, estimate: feeBasis)
                    }
                    else {
                        let feeError = Wallet.FeeEstimationError.fromStatus(event.u.feeBasisEstimated.status)
                        system.callbackCoordinator.handleWalletFeeEstimateFailure (cookie, error: feeError)
                    }
                default: precondition (false)
                }
        },

            transferEventCallback: { (context, cwm, wid, tid, event) in
                precondition (nil != context  && nil != cwm && nil != wid && nil != tid)
                defer { cryptoWalletManagerGive(cwm); cryptoWalletGive(wid); cryptoTransferGive(tid) }

                guard let (system, manager, wallet, transfer) = System.systemExtract (context, cwm, wid, tid)
                    else { print ("SYS: Event: \(event.type): Missed {cwm, wid, tid}"); return }

                print ("SYS: Event: Transfer (\(wallet.name) @ \(transfer.hash?.description ?? "pending")): \(event.type)")

                switch (event.type) {
                case CRYPTO_TRANSFER_EVENT_CREATED:
                    system.listener?.handleTransferEvent (system: manager.system,
                                                            manager: manager,
                                                            wallet: wallet,
                                                            transfer: transfer,
                                                            event: TransferEvent.created)

                case CRYPTO_TRANSFER_EVENT_CHANGED:
                    system.listener?.handleTransferEvent (system: manager.system,
                                                            manager: manager,
                                                            wallet: wallet,
                                                            transfer: transfer,
                                                            event: TransferEvent.changed (old: TransferState.init (core: event.u.state.old),
                                                                                          new: TransferState.init (core: event.u.state.new)))

                case CRYPTO_TRANSFER_EVENT_DELETED:
                    system.listener?.handleTransferEvent (system: manager.system,
                                                            manager: manager,
                                                            wallet: wallet,
                                                            transfer: transfer,
                                                            event: TransferEvent.deleted)
                default: precondition(false)
                }
        })
    }
}

extension System {
    internal var clientBTC: BRCryptoCWMClientBTC {
        return BRCryptoCWMClientBTC (
            funcGetBlockNumber: { (context, cwm, sid) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: BTC: GetBlockNumber: Missed {cwm}"); return }
                print ("SYS: BTC: GetBlockNumber")

                manager.query.getBlockchain (blockchainId: manager.network.uids) { (res: Result<BlockChainDB.Model.Blockchain, BlockChainDB.QueryError>) in
                    defer { cryptoWalletManagerGive (cwm!) }
                    res.resolve (
                        success: { cwmAnnounceGetBlockNumberSuccessAsInteger (manager.core, sid, $0.blockHeight!) },
                        failure: { (_) in cwmAnnounceGetBlockNumberFailure (manager.core, sid) })
                }},

            funcGetTransactions: { (context, cwm, sid, addresses, addressesCount, begBlockNumber, endBlockNumber) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: BTC: GetTransactions: Missed {cwm}"); return }
                print ("SYS: BTC: GetTransactions: Blocks: {\(begBlockNumber), \(endBlockNumber)}")

                var cAddresses = addresses!
                var addresses:[String] = Array (repeating: "", count: addressesCount)
                for index in 0..<addressesCount {
                    addresses[index] = asUTF8String (cAddresses.pointee!)
                    cAddresses = cAddresses.advanced(by: 1)
                }

                manager.query.getTransactions (blockchainId: manager.network.uids,
                                               addresses: addresses,
                                               begBlockNumber: begBlockNumber,
                                               endBlockNumber: endBlockNumber,
                                               includeRaw: true) {
                                                (res: Result<[BlockChainDB.Model.Transaction], BlockChainDB.QueryError>) in
                                                defer { cryptoWalletManagerGive (cwm!) }
                                                res.resolve(
                                                    success: {
                                                        $0.forEach { (model: BlockChainDB.Model.Transaction) in
                                                            let timestamp = model.timestamp.map { UInt64 ($0.timeIntervalSince1970) } ?? 0
                                                            let height    = model.blockHeight ?? 0

                                                            if var data = model.raw {
                                                                let bytesCount = data.count
                                                                data.withUnsafeMutableBytes { (bytes: UnsafeMutableRawBufferPointer) -> Void in
                                                                    let bytesAsUInt8 = bytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                                                                    cwmAnnounceGetTransactionsItemBTC (cwm, sid,
                                                                                                       bytesAsUInt8,
                                                                                                       bytesCount,
                                                                                                       timestamp,
                                                                                                       height)
                                                                }
                                                            }
                                                        }
                                                        cwmAnnounceGetTransactionsComplete (cwm, sid, CRYPTO_TRUE) },
                                                    failure: { (_) in cwmAnnounceGetTransactionsComplete (cwm, sid, CRYPTO_FALSE) })

                }},

            funcSubmitTransaction: { (context, cwm, sid, transactionBytes, transactionBytesLength, hashAsHex) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: BTC: SubmitTransaction: Missed {cwm}"); return }
                print ("SYS: BTC: SubmitTransaction")

                let hash = asUTF8String (hashAsHex!)
                let data = Data (bytes: transactionBytes!, count: transactionBytesLength)
                manager.query.createTransaction (blockchainId: manager.network.uids, hashAsHex: hash, transaction: data) {
                    (res: Result<Void, BlockChainDB.QueryError>) in
                    defer { cryptoWalletManagerGive (cwm!) }
                    res.resolve(
                        success: { (_) in cwmAnnounceSubmitTransferSuccess (cwm, sid) },
                        failure: { (_) in cwmAnnounceSubmitTransferFailure (cwm, sid) })
                }
        })
    }
}

extension System {
    internal var clientETH: BRCryptoCWMClientETH {
        return BRCryptoCWMClientETH (
            funcGetEtherBalance: { (context, cwm, sid, network, address) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: ETH: GetEtherBalance: Missed {cwm}"); return }

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network = asUTF8String (networkGetName (ewmGetNetwork (ewm)))
                let address = asUTF8String (address!)

                manager.query.getBalanceAsETH (network: network, address: address) {
                    (res: Result<String, BlockChainDB.QueryError>) in
                    defer { cryptoWalletManagerGive (cwm!) }
                    res.resolve (
                        success: { cwmAnnounceGetBalanceSuccess (cwm, sid, $0) },
                        failure: { (_) in cwmAnnounceGetBalanceFailure (cwm, sid) })
                }},

            funcGetTokenBalance: { (context, cwm, sid, network, address, contract) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: ETH: GetTokenBalance: Missed {cwm}"); return }

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))
                let address  = asUTF8String (address!)
                let contract = asUTF8String (contract!)

                manager.query.getBalanceAsTOK (network: network, address: address, contract: contract) {
                    (res: Result<String, BlockChainDB.QueryError>) in
                    defer { cryptoWalletManagerGive (cwm!) }
                    res.resolve (
                        success: { cwmAnnounceGetBalanceSuccess (cwm, sid, $0) },
                        failure: { (_) in cwmAnnounceGetBalanceFailure (cwm, sid) })
                }},

            funcGetGasPrice: { (context, cwm, sid, network) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: ETH: GetGasPrice: Missed {cwm}"); return }

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.getGasPriceAsETH (network: network) {
                    (res: Result<String, BlockChainDB.QueryError>) in
                    defer { cryptoWalletManagerGive (cwm!) }
                    res.resolve (
                        success: { cwmAnnounceGetGasPriceSuccess (cwm, sid, $0) },
                        failure: { (_) in cwmAnnounceGetGasPriceFailure (cwm, sid) })
                }},

            funcEstimateGas: { (context, cwm, sid, network, from, to, amount, price, data) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: ETH: EstimateGas: Missed {cwm}"); return }

                guard let price = price.map (asUTF8String)
                    else { print ("SYS: ETH: EstimateGas: Missed {price}"); return }

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.getGasEstimateAsETH (network: network,
                                                   from:   asUTF8String(from!),
                                                   to:     asUTF8String(to!),
                                                   amount: asUTF8String(amount!),
                                                   data:   asUTF8String(data!)) {
                                                    (res: Result<String, BlockChainDB.QueryError>) in
                                                    defer { cryptoWalletManagerGive (cwm!) }
                                                    res.resolve (
                                                        success: { cwmAnnounceGetGasEstimateSuccess (cwm, sid, $0, price) },
                                                        failure: { (_) in cwmAnnounceGetGasEstimateFailure (cwm, sid, CRYPTO_ERROR_FAILED) })
                }},

            funcSubmitTransaction: { (context, cwm, sid, network, transaction) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: ETH: SubmitTransaction: Missed {cwm}"); return }

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.submitTransactionAsETH (network: network,
                                                      transaction: asUTF8String(transaction!)) {
                                                        (res: Result<String, BlockChainDB.QueryError>) in
                                                        defer { cryptoWalletManagerGive (cwm!) }
                                                        res.resolve (
                                                            success: { cwmAnnounceSubmitTransferSuccessForHash (cwm, sid, $0) },
                                                            failure: { (_) in cwmAnnounceSubmitTransferFailure (cwm, sid) })
                }},

            funcGetTransactions: { (context, cwm, sid, network, address, begBlockNumber, endBlockNumber) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: ETH: GetTransactions: Missed {cwm}"); return }

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.getTransactionsAsETH (network: network,
                                                    address: asUTF8String (address!),
                                                    begBlockNumber: begBlockNumber,
                                                    endBlockNumber: endBlockNumber) {
                                                        (res: Result<[BlockChainDB.ETH.Transaction], BlockChainDB.QueryError>) in
                                                        defer { cryptoWalletManagerGive (cwm!) }
                                                        res.resolve(
                                                            success: { (txs: [BlockChainDB.ETH.Transaction]) in
                                                                txs.forEach { (tx: BlockChainDB.ETH.Transaction) in
                                                                    cwmAnnounceGetTransactionsItemETH (cwm, sid,
                                                                                                       tx.hash,
                                                                                                       tx.sourceAddr,
                                                                                                       tx.targetAddr,
                                                                                                       tx.contractAddr,
                                                                                                       tx.amount,
                                                                                                       tx.gasLimit,
                                                                                                       tx.gasPrice,
                                                                                                       tx.data,
                                                                                                       tx.nonce,
                                                                                                       tx.gasUsed,
                                                                                                       tx.blockNumber,
                                                                                                       tx.blockHash,
                                                                                                       tx.blockConfirmations,
                                                                                                       tx.blockTransactionIndex,
                                                                                                       tx.blockTimestamp,
                                                                                                       tx.isError)
                                                                }
                                                                cwmAnnounceGetTransactionsComplete(cwm, sid, CRYPTO_TRUE)
                                                        },
                                                            failure: { (_) in cwmAnnounceGetTransactionsComplete (cwm, sid, CRYPTO_FALSE) })
                }},

            funcGetLogs: { (context, cwm, sid, network, contract, address, event, begBlockNumber, endBlockNumber) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: ETH: GetLogs: Missed {cwm}"); return }

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.getLogsAsETH (network: network,
                                            contract: contract.map { asUTF8String($0) },
                                            address:  asUTF8String(address!),
                                            event:    asUTF8String(event!),
                                            begBlockNumber: begBlockNumber,
                                            endBlockNumber: endBlockNumber) {
                                                (res: Result<[BlockChainDB.ETH.Log], BlockChainDB.QueryError>) in
                                                defer { cryptoWalletManagerGive (cwm!) }
                                                res.resolve(
                                                    success: { (lgs: [BlockChainDB.ETH.Log]) in
                                                        lgs.forEach { (log: BlockChainDB.ETH.Log) in
                                                            let topicsCount = Int32 (log.topics.count)
                                                            var topics = log.topics.filter { !$0.isEmpty }.map { UnsafePointer<Int8>(strdup($0)) }
                                                            defer { topics.forEach { free(UnsafeMutablePointer(mutating: $0)) } }

                                                            cwmAnnounceGetLogsItem (cwm, sid,
                                                                                    log.hash,
                                                                                    log.contract,
                                                                                    topicsCount,
                                                                                    &topics,
                                                                                    log.data,
                                                                                    log.gasPrice,
                                                                                    log.gasUsed,
                                                                                    log.logIndex,
                                                                                    log.blockNumber,
                                                                                    log.blockTransactionIndex,
                                                                                    log.blockTimestamp) }
                                                        cwmAnnounceGetLogsComplete(cwm, sid, CRYPTO_TRUE)
                                                },
                                                    failure: { (_) in cwmAnnounceGetLogsComplete (cwm, sid, CRYPTO_FALSE) })
                }},

            funcGetBlocks: { (context, cwm, sid, network, address, interests, begBlockNumber, endBlockNumber) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: ETH: GetBlocks: Missed {cwm}"); return }

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.getBlocksAsETH (network: network,
                                              address: asUTF8String(address!),
                                              interests: interests,
                                              blockStart: begBlockNumber,
                                              blockStop:  endBlockNumber) {
                                                (res: Result<[UInt64], BlockChainDB.QueryError>) in
                                                defer { cryptoWalletManagerGive (cwm!) }
                                                res.resolve (
                                                    success: {
                                                        let numbersCount = Int32 ($0.count)
                                                        var numbers = $0
                                                        numbers.withUnsafeMutableBytes {
                                                            let bytesAsUInt8 = $0.baseAddress?.assumingMemoryBound(to: UInt64.self)
                                                            cwmAnnounceGetBlocksSuccess (cwm, sid, numbersCount, bytesAsUInt8)
                                                        }},
                                                    failure: { (_) in cwmAnnounceGetBlocksFailure (cwm, sid) })
                }},

            funcGetTokens: { (context, cwm, sid) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: ETH: GetTokens: Missed {cwm}"); return }

                manager.query.getTokensAsETH () {
                    (res: Result<[BlockChainDB.ETH.Token],BlockChainDB.QueryError>) in
                    defer { cryptoWalletManagerGive (cwm!) }
                    res.resolve(
                        success: { (tokens: [BlockChainDB.ETH.Token]) in
                            tokens.forEach { (token: BlockChainDB.ETH.Token) in
                                cwmAnnounceGetTokensItem (cwm, sid,
                                                          token.address,
                                                          token.symbol,
                                                          token.name,
                                                          token.description,
                                                          token.decimals,
                                                          token.defaultGasLimit,
                                                          token.defaultGasPrice) }
                            cwmAnnounceGetTokensComplete (cwm, sid, CRYPTO_TRUE)
                    },
                        failure: { (_) in cwmAnnounceGetTokensComplete (cwm, sid, CRYPTO_FALSE) })
                }},

            funcGetBlockNumber: { (context, cwm, sid, network) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: ETH: GetBlockNumber: Missed {cwm}"); return }

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.getBlockNumberAsETH (network: network) {
                    (res: Result<String, BlockChainDB.QueryError>) in
                    defer { cryptoWalletManagerGive (cwm!) }
                    res.resolve (
                        success: { cwmAnnounceGetBlockNumberSuccessAsString (cwm, sid, $0) },
                        failure: { (_) in cwmAnnounceGetBlockNumberFailure (cwm, sid) })
                }},

            funcGetNonce: { (context, cwm, sid, network, address) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: ETH: GetNonce: Missed {cwm}"); return }

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                guard let address = address.map (asUTF8String)
                    else { print ("SYS: ETH: GetNonce: Missed {address}"); return }

                 manager.query.getNonceAsETH (network: network, address: address) {
                    (res: Result<String, BlockChainDB.QueryError>) in
                    defer { cryptoWalletManagerGive (cwm!) }
                    res.resolve (
                        success: { cwmAnnounceGetNonceSuccess (cwm, sid, address, $0) },
                        failure: { (_) in cwmAnnounceGetNonceFailure (cwm, sid) })
                }})
    }
}

extension System {
    internal var clientGEN: BRCryptoCWMClientGEN {
        return BRCryptoCWMClientGEN (
            funcGetBlockNumber: { (context, cwm, sid) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: GEN: GetBlockNumber: Missed {cwm}"); return }
                print ("SYS: GEN: GetBlockNumber")

                manager.query.getBlockchain (blockchainId: manager.network.uids) {
                    (res: Result<BlockChainDB.Model.Blockchain, BlockChainDB.QueryError>) in
                    defer { cryptoWalletManagerGive(cwm) }
                    res.resolve (
                        success: { cwmAnnounceGetBlockNumberSuccessAsInteger (cwm, sid, $0.blockHeight!) },
                        failure: { (_) in cwmAnnounceGetBlockNumberFailure (cwm, sid) })
                }},

            funcGetTransactions: { (context, cwm, sid, address, begBlockNumber, endBlockNumber) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: GEN: GetTransaction: Missed {cwm}"); return }
                print ("SYS: GEN: GetTransactions: Blocks: {\(begBlockNumber), \(endBlockNumber)}")

                manager.query.getTransactions (blockchainId: manager.network.uids,
                                               addresses: [asUTF8String(address!)],
                                               begBlockNumber: begBlockNumber,
                                               endBlockNumber: endBlockNumber,
                                               includeRaw: true) {
                                                (res: Result<[BlockChainDB.Model.Transaction], BlockChainDB.QueryError>) in
                                                defer { cryptoWalletManagerGive(cwm) }
                                                res.resolve(
                                                    success: {
                                                        $0.forEach { (model: BlockChainDB.Model.Transaction) in
                                                            let timestamp = model.timestamp.map { UInt64 ($0.timeIntervalSince1970) } ?? 0
                                                            let height    = model.blockHeight ?? 0

                                                            if var data = model.raw {
                                                                let bytesCount = data.count
                                                                data.withUnsafeMutableBytes { (bytes: UnsafeMutableRawBufferPointer) -> Void in
                                                                    let bytesAsUInt8 = bytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                                                                    cwmAnnounceGetTransactionsItemGEN (cwm, sid,
                                                                                                       bytesAsUInt8,
                                                                                                       bytesCount,
                                                                                                       timestamp,
                                                                                                       height)
                                                                }
                                                            }
                                                        }
                                                        cwmAnnounceGetTransactionsComplete (cwm, sid, CRYPTO_TRUE) },
                                                    failure: { (_) in cwmAnnounceGetTransactionsComplete (cwm, sid, CRYPTO_FALSE) })

                }},

            funcSubmitTransaction: { (context, cwm, sid, transactionBytes, transactionBytesLength, hashAsHex) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: GEN: SubmitTransaction: Missed {cwm}"); return }
                print ("SYS: GEN: SubmitTransaction")

                let hash = asUTF8String (hashAsHex!)
                let data = Data (bytes: transactionBytes!, count: transactionBytesLength)
                manager.query.createTransaction (blockchainId: manager.network.uids, hashAsHex: hash, transaction: data) {
                    (res: Result<Void, BlockChainDB.QueryError>) in
                    defer { cryptoWalletManagerGive (cwm!) }
                    res.resolve(
                        success: { (_) in cwmAnnounceSubmitTransferSuccess (cwm, sid) },
                        failure: { (_) in cwmAnnounceSubmitTransferFailure (cwm, sid) })
                }
        })
    }
}

extension System {
    internal var cryptoClient: BRCryptoCWMClient {
        return BRCryptoCWMClient (context: systemContext,
                                  btc: clientBTC,
                                  eth: clientETH,
                                  gen: clientGEN)
    }
}

/// Support for Persistent Storage Migration.
///
/// Allow prior App version to migrate their SQLite database representations of BTC/BTC
/// transations, blocks and peers into 'Generic Crypto' - where these entities are persistently
/// stored in the file system (by BRFileSystem).
///
extension System {

    ///
    /// A Blob of Transaction Data
    ///
    /// - btc:
    ///
    public enum TransactionBlob {
        case btc (
            bytes: [UInt8],
            blockHeight: UInt32,
            timestamp: UInt32 // time interval since unix epoch (including '0'
        )
    }

    ///
    /// A BlockHash is 32-bytes of UInt8 data
    ///
    public typealias BlockHash = [UInt8]

    /// Validate `BlockHash`
    private static func validateBlockHash (_ hash: BlockHash) -> Bool {
        return 32 == hash.count
    }

    ///
    /// A Blob of Block Data
    ///
    /// - btc:
    ///
    public enum BlockBlob {
        case btc (
            hash: BlockHash,
            height: UInt32,
            nonce: UInt32,
            target: UInt32,
            txCount: UInt32,
            version: UInt32,
            timestamp: UInt32?,
            flags: [UInt8],
            hashes: [BlockHash],
            merkleRoot: BlockHash,
            prevBlock: BlockHash
        )
    }

    ///
    /// A Blob of Peer Data
    ///
    /// - btc:
    ///
    public enum PeerBlob {
        case btc (
            address: UInt32,  // UInt128 { .u32 = { 0, 0, 0xffff, <address> }}
            port: UInt16,
            services: UInt64,
            timestamp: UInt32?
        )
    }

    ///
    /// Migrate Errors
    ///
    enum MigrateError: Error {
        /// Migrate does not apply to this network
        case invalid

        /// Migrate couldn't access the file system
        case create

        /// Migrate failed to parse or to save a transaction
        case transaction

        /// Migrate failed to parse or to save a block
        case block

        /// Migrate failed to parse or to save a peer.
        case peer
    }

    ///
    /// Migrate the storage for a network given transaction, block and peer blobs.  The provided
    /// blobs must be consistent with `network`.  For exmaple, if `network` represents BTC or BCH
    /// then the blobs must be of type `.btc`; otherwise a MigrateError is thrown.
    ///
    /// - Parameters:
    ///   - network:
    ///   - transactionBlobs:
    ///   - blockBlobs:
    ///   - peerBlobs:
    ///
    /// - Throws: MigrateError
    ///
    public func migrateStorage (network: Network,
                                transactionBlobs: [TransactionBlob],
                                blockBlobs: [BlockBlob],
                                peerBlobs: [PeerBlob]) throws {
        guard migrateRequired (network: network)
            else { throw MigrateError.invalid }

        switch network.currency.code.lowercased() {
        case Currency.codeAsBTC,
             Currency.codeAsBCH:
            try migrateStorageAsBTC(network: network,
                                    transactionBlobs: transactionBlobs,
                                    blockBlobs: blockBlobs,
                                    peerBlobs: peerBlobs)
        default:
            throw MigrateError.invalid
        }
    }

    ///
    /// Migrate storage for BTC
    ///
    private func migrateStorageAsBTC (network: Network,
                                      transactionBlobs: [TransactionBlob],
                                      blockBlobs: [BlockBlob],
                                      peerBlobs: [PeerBlob]) throws {

        guard let migrator = cryptoWalletMigratorCreate (network.core, path)
            else { throw MigrateError.create }
        defer { cryptoWalletMigratorRelease (migrator) }

        try transactionBlobs.forEach { (blob: TransactionBlob) in
            guard case let .btc (blob) = blob
                else { throw MigrateError.transaction }

            var bytes = blob.bytes
            let status = cryptoWalletMigratorHandleTransactionAsBTC (migrator,
                                                                     &bytes, bytes.count,
                                                                     blob.blockHeight,
                                                                     blob.timestamp)
            if status.type != CRYPTO_WALLET_MIGRATOR_SUCCESS {
                throw MigrateError.transaction
            }
        }

        try blockBlobs.forEach { (blob: BlockBlob) in
            guard case let .btc (blob) = blob
                else { throw MigrateError.block }

            // On a `nil` timestamp, by definition skip out, don't migrate this blob
            guard nil != blob.timestamp
                else { return }

            guard blob.hashes.allSatisfy (System.validateBlockHash(_:)),
                System.validateBlockHash(blob.hash),
                System.validateBlockHash(blob.merkleRoot),
                System.validateBlockHash(blob.prevBlock)
                else { throw MigrateError.block }

            var flags  = blob.flags
            var hashes = blob.hashes
            let hashesCount = blob.hashes.count

            let hash: UInt256 = blob.hash.withUnsafeBytes { $0.load (as: UInt256.self) }
            let merkleRoot: UInt256 = blob.merkleRoot.withUnsafeBytes { $0.load (as: UInt256.self) }
            let prevBlock:  UInt256 = blob.prevBlock.withUnsafeBytes  { $0.load (as: UInt256.self) }

            try hashes.withUnsafeMutableBytes { (hashesBytes: UnsafeMutableRawBufferPointer) -> Void in
                let hashesAddr = hashesBytes.baseAddress?.assumingMemoryBound(to: UInt256.self)
                let status = cryptoWalletMigratorHandleBlockAsBTC (migrator,
                                                                   hash,
                                                                   blob.height,
                                                                   blob.nonce,
                                                                   blob.target,
                                                                   blob.txCount,
                                                                   blob.version,
                                                                   blob.timestamp!,
                                                                   &flags, flags.count,
                                                                   hashesAddr, hashesCount,
                                                                   merkleRoot,
                                                                   prevBlock)
                if status.type != CRYPTO_WALLET_MIGRATOR_SUCCESS {
                    throw MigrateError.block
                }
            }
        }

        try peerBlobs.forEach { (blob: PeerBlob) in
            guard case let .btc (blob) = blob
                else { throw MigrateError.peer }

            // On a `nil` timestamp, by definition skip out, don't migrate this blob
            guard nil != blob.timestamp
                else { return }

            let status = cryptoWalletMigratorHandlePeerAsBTC (migrator,
                                                              blob.address,
                                                              blob.port,
                                                              blob.services,
                                                              blob.timestamp!)

            if status.type != CRYPTO_WALLET_MIGRATOR_SUCCESS {
                throw MigrateError.peer
            }
        }
    }

    ///
    /// If migration is required, return the currency code; otherwise, return nil.
    ///
    /// - Note: it is not an error not to migrate.
    ///
    /// - Parameter network:
    ///
    /// - Returns: The currency code or nil
    ///
    public func migrateRequired (network: Network) -> Bool {
        let code = network.currency.code.lowercased()
        return code == Currency.codeAsBTC
            || code == Currency.codeAsBCH
    }

    /// Testing

    ///
    /// Convert `transfer` to a `TransactionBlob`.
    ///
    /// - Parameter transfer:
    ///
    /// - Returns: A `TransactionBlob` or nil (if the transfer's network does not require
    ///     migration, such as for an ETH network).
    ///
    internal func asBlob (transfer: Transfer) -> TransactionBlob? {
        let network = transfer.manager.network
        guard migrateRequired(network: network)
            else { return nil }

        switch network.currency.code.lowercased() {
        case Currency.codeAsBTC,
             Currency.codeAsBCH:
            var blockHeight: UInt32 = 0
            var timestamp:   UInt32 = 0
            var bytesCount:  size_t = 0
            var bytes: UnsafeMutablePointer<UInt8>? = nil
            defer { if nil != bytes { free(bytes) }}

            cryptoTransferExtractBlobAsBTC (transfer.core,
                                            &bytes, &bytesCount,
                                            &blockHeight,
                                            &timestamp)

            return TransactionBlob.btc (bytes: UnsafeMutableBufferPointer<UInt8> (start: bytes, count: bytesCount).map { $0 },
                                        blockHeight: blockHeight,
                                        timestamp: timestamp)
        default:
            return nil
        }
    }
}

extension BRCryptoTransferEventType: CustomStringConvertible {
    public var description: String {
        switch self {
        case CRYPTO_TRANSFER_EVENT_CREATED: return "Created"
        case CRYPTO_TRANSFER_EVENT_CHANGED: return "Changed"
        case CRYPTO_TRANSFER_EVENT_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BRCryptoWalletEventType: CustomStringConvertible {
    public var description: String {
        switch self {
        case CRYPTO_WALLET_EVENT_CREATED: return "Created"
        case CRYPTO_WALLET_EVENT_CHANGED: return "Changed"
        case CRYPTO_WALLET_EVENT_DELETED: return "Deleted"

        case CRYPTO_WALLET_EVENT_TRANSFER_ADDED:     return "Transfer Added"
        case CRYPTO_WALLET_EVENT_TRANSFER_CHANGED:   return "Transfer Changed"
        case CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED: return "Transfer Submitted"
        case CRYPTO_WALLET_EVENT_TRANSFER_DELETED:   return "Transfer Deleted"

        case CRYPTO_WALLET_EVENT_BALANCE_UPDATED:   return "Balance Updated"
        case CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED: return "FeeBasis Updated"
        case CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED: return "FeeBasis Estimated"

        default: return "<<unknown>>"
        }
    }
}

extension BRCryptoWalletManagerEventType: CustomStringConvertible {
    public var description: String {
        switch self {
        case CRYPTO_WALLET_MANAGER_EVENT_CREATED: return "Created"
        case CRYPTO_WALLET_MANAGER_EVENT_CHANGED: return "Changed"
        case CRYPTO_WALLET_MANAGER_EVENT_DELETED: return "Deleted"

        case CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED:   return "Wallet Added"
        case CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED: return "Wallet Changed"
        case CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED: return "Wallet Deleted"

        // wallet: added, ...
        case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED:   return "Sync Started"
        case CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES: return "Sync Continues"
        case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED:   return "Sync Stopped"

        case CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED: return "Block Height Updated"
        default: return "<<unknown>>"
        }
    }
}
