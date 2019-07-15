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
import Foundation
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

    /// The 'blockchain DB' to use for BRD Server Assisted queries
    public let query: BlockChainDB

    internal let queue = DispatchQueue (label: "Crypto System")

    /// the networks, unsorted.
    public internal(set) var networks: [Network] = []

    ///
    /// Add `network` to `networks`
    ///
    /// - Parameter network: the network to add
    ///
    internal func add (network: Network) {
        networks.append (network)
        announceEvent (SystemEvent.networkAdded(network: network))
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
            manager.announceEvent (WalletManagerEvent.created)
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
                                     mode: WalletManagerMode) {
        
        let manager = WalletManager (system: self,
                                     listener: listener!,
                                     account: account,
                                     network: network,
                                     mode: mode,
                                     storagePath: path)
        
        self.add (manager: manager)
    }

    // Wallets - derived as a 'flatMap' of the managers' wallets.
    public var wallets: [Wallet] {
        return managers.flatMap { $0.wallets }
    }
    
    public init (listener: SystemListener,
                 account: Account,
                 path: String,
                 query: BlockChainDB) {
        self.listener = listener
        self.account = account
        self.path = path
        self.query = query
        
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

    ///
    /// Stop the system.  All managers are disconnected.  Will inhibit `System` processing.
    ///
    public func stop () {
        managers.forEach { $0.disconnect() }
    }

    ///
    /// Start the system.  This will query various BRD services, notably the BlockChainDB, to
    /// establish the available networks (aka blockchains) and their currencies.  If the
    /// `networksNeeded` array includes the name of an available network, then a `Network`
    /// will be created for that network.  (This generates a `SystemEvent` which can be used by
    /// the App to create a `WalletManager`)
    ///
    /// This method can be called repeatedly; however ONLY THE FIRST invocation will create
    /// Networks for needed networks.  Subsequent calls will simple restart `System` processing.
    ///
    /// - Parameter networksNeeded: Then needed networks
    ///
    public func start (networksNeeded: [String]) {
        if !networks.isEmpty {
            managers.forEach { $0.connect() }
            return
        }

        #if MAINNET
        var mainnet = true
        #endif

        #if TESTNET
        var mainnet = false
        #endif

        func currencyDenominationToBaseUnit (currency: Currency, model: BlockChainDB.Model.CurrencyDenomination) -> Unit {
            let uids = "\(currency.name)-\(model.code)"
            return Unit (currency: currency, uids: uids, name: model.name, symbol: model.symbol)
        }

        func currencyDenominationToUnit (currency: Currency, model: BlockChainDB.Model.CurrencyDenomination, base: Unit) -> Unit {
            let uids = "\(currency.name)-\(model.code)"
            return Unit (currency: currency, uids: uids, name: model.name, symbol: model.symbol, base: base, decimals: model.decimals)
        }

        // query blockchains
        self.query.getBlockchains (mainnet: mainnet) { (blockchainResult: Result<[BlockChainDB.Model.Blockchain],BlockChainDB.QueryError>) in
            let blockChainModels = try! blockchainResult
                // On success, always merge `defaultBlockchains`
                .map { $0.unionOf (BlockChainDB.Model.defaultBlockchains) { $0.id } }
                // On error, use defaultBlockchains
                .recover { (error: BlockChainDB.QueryError) -> [BlockChainDB.Model.Blockchain] in
                    return BlockChainDB.Model.defaultBlockchains
                }.get()

            blockChainModels.filter { networksNeeded.contains($0.id) }
                .forEach { (blockchainModel: BlockChainDB.Model.Blockchain) in

                    // query currencies
                    self.query.getCurrencies (blockchainId: blockchainModel.id) { (currencyResult: Result<[BlockChainDB.Model.Currency],BlockChainDB.QueryError>) in
                        // Find applicable defaults by `blockchainID`
                        let defaults = BlockChainDB.Model.defaultCurrencies
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
                            .forEach { (currencyModel: BlockChainDB.Model.Currency) in
                                // Create the currency
                                let currency = Currency (uids: currencyModel.id,
                                                         name: currencyModel.name,
                                                         code: currencyModel.code,
                                                         type: currencyModel.type,
                                                         issuer: currencyModel.address)

                                // Create the base unit
                                let baseUnit = currencyModel.demoninations.first { 0 == $0.decimals}
                                    .map { currencyDenominationToBaseUnit(currency: currency, model: $0) }!

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
                        guard let currency = associations.keys.first (where: { $0.code == blockchainModel.currency.lowercased() })
                            else { print ("SYS: START: Missed Currency (\(blockchainModel.currency)): defaultUnit"); return }

                        // define the network
                        let network = Network (uids: blockchainModel.id,
                                               name: blockchainModel.name,
                                               isMainnet: blockchainModel.isMainnet,
                                               currency: currency,
                                               height: blockchainModel.blockHeight,
                                               associations: associations)

                        // save the network
                        self.networks.append (network)

                        // Invoke callbacks.
                        self.listener?.handleNetworkEvent (system: self, network: network, event: NetworkEvent.created)
                        self.listener?.handleSystemEvent  (system: self, event: SystemEvent.networkAdded(network: network))
                    }
            }
        }
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

