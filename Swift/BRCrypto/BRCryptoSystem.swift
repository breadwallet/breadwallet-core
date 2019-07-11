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
import Darwin.C.stdatomic // atomic_fetch_add
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
//            manager.announceEvent (WalletManagerEvent.created)
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
                                     account: account,
                                     network: network,
                                     mode: mode,
                                     storagePath: path,
                                     listener: cryptoListener,
                                     client: cryptoClient   )
        
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

        let index = 1 + Int32(context.distance(to: UnsafeMutableRawPointer(bitPattern: 1)!))

        return systemLookup(index: index)
            .map { ($0, WalletManager (core: cwm, system: $0, take: true)) }
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
                    // We are only here in response to system.createWalletManager...
                    break

                case CRYPTO_WALLET_MANAGER_EVENT_CHANGED:
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
                    system.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.syncProgress (percentComplete: Double (event.u.sync.percentComplete)))

                case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED:
                    system.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.syncEnded(error: nil))

                case CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED:
                    manager.network.height = event.u.blockHeight.value

                default: precondition(false)
                }

/*
                internal func announceEvent (_ event: WalletManagerEvent) {
                    self.listener?.handleManagerEvent (system: system,
                                                       manager: self,
                                                       event: event)
                }
*/

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
                                         unit: wallet.unit,
                                         take: false)
                    system.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.balanceUpdated(amount: amount))

                case CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED:
                    let feeBasis = TransferFeeBasis (core: event.u.feeBasisUpdated.basis,
                                                     take: false)
                    system.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.feeBasisUpdated(feeBasis: feeBasis))

                default: precondition (false)
                }

/*
                internal func announceEvent (_ event: WalletEvent) {
                    self.listener?.handleWalletEvent (system: system,
                                                      manager: manager,
                                                      wallet: self,
                                                      event: event)
                }
*/

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

/*
                internal func announceEvent (_ event: TransferEvent) {
                    self.listener?.handleTransferEvent (system: system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        transfer: self,
                                                        event: event)
                }
*/

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
                        success: { cwmAnnounceGetBlockNumberSuccessAsInteger (manager.core, sid, $0.blockHeight) },
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
            /*
             // We query the BlockChainDB with an array of addresses.  If there are no
             // transactions for those addresses, then we are done.  But, if there are
             // we need to generate more addresses and keep trying to find additional
             // transactions.
             //
             // So we'll repeatedly loop and accumulate transactions until no more
             // transactions are found for the set of addresses
             //
             // In order to 'generate more addresses' we'll need to announce each
             // transaction - which will register each transaction in the wallet

             // TODO: Register early transactions even if later transactions fail?  Possible?

             manager.system.queue.async {
             let semaphore = DispatchSemaphore (value: 0)

             var transactionsError = false
             var transactionsFound = false

             repeat {
             // Get a C pointer to `addressesLimit` BRAddress structures
             let addressesLimit:Int = 25
             let addressesPointer = BRWalletManagerGetUnusedAddrsLegacy (bid, UInt32(addressesLimit))
             defer { free (addressesPointer) }

             // Convert the C pointer into a Swift array of BRAddress
             let addressesStructures:[BRAddress] = addressesPointer!.withMemoryRebound (to: BRAddress.self, capacity: addressesLimit) {
             Array(UnsafeBufferPointer (start: $0, count: addressesLimit))
             }

             // Convert each BRAddress to a String
             let addresses = addressesStructures.map {
             return String(cString: UnsafeRawPointer([$0]).assumingMemoryBound(to: CChar.self))
             }

             transactionsFound = false
             transactionsError = false

             // Query the blockchainDB. Record each found transaction
             manager.query.getTransactionsAsBTC (bwm: bid,
             blockchainId: manager.network.uids,
             addresses: addresses,
             begBlockNumber: begBlockNumber,
             endBlockNumber: endBlockNumber,
             rid: rid,
             done: { (success: Bool, rid: Int32) in
             transactionsError = !success
             semaphore.signal ()
             },
             each: { (res: BlockChainDB.BTC.Transaction) in
             transactionsFound = true
             bwmAnnounceTransaction (bid, res.rid, res.btc)
             })

             // Wait until the query is done
             semaphore.wait()

             } while !transactionsError && transactionsFound
             */

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

            funcEstimateGas: { (context, cwm, sid, network, from, to, amount, data) in
                precondition (nil != context  && nil != cwm)

                guard let (system, manager) = System.systemExtract (context, cwm)
                    else { print ("SYS: ETH: EstimateGas: Missed {cwm}"); return }

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
                                                        success: { cwmAnnounceGetGasEstimateSuccess (cwm, sid, $0) },
                                                        failure: { (_) in cwmAnnounceGetGasEstimateFailure (cwm, sid) })
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

                manager.query.getNonceAsETH (network: network, address: asUTF8String(address!)) {
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
                        success: { cwmAnnounceGetBlockNumberSuccessAsInteger (cwm, sid, $0.blockHeight) },
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

extension BRWalletManagerEventType: CustomStringConvertible {
    public var description: String {
        switch self {
        case BITCOIN_WALLET_MANAGER_CREATED: return "Created"
        case BITCOIN_WALLET_MANAGER_CONNECTED: return "Connected"
        case BITCOIN_WALLET_MANAGER_DISCONNECTED: return "Disconnected"
        case BITCOIN_WALLET_MANAGER_SYNC_STARTED: return "Sync Started"
        case BITCOIN_WALLET_MANAGER_SYNC_STOPPED: return "Sync Stopped"
        case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED: return "Block Height Updated"
        default: return "<<unknown>>"
        }
    }
}

extension BRWalletEventType: CustomStringConvertible {
    public var description: String {
        switch self {
        case BITCOIN_WALLET_CREATED: return "Created"
        case BITCOIN_WALLET_BALANCE_UPDATED: return "Balance Updated"
        case BITCOIN_WALLET_TRANSACTION_SUBMITTED: return "Transaction Submitted"
        case BITCOIN_WALLET_FEE_PER_KB_UPDATED: return "FeePerKB Updated"
        case BITCOIN_WALLET_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BRTransactionEventType: CustomStringConvertible {
    public var description: String {
        switch self {
        case BITCOIN_TRANSACTION_ADDED: return "Added"
        case BITCOIN_TRANSACTION_UPDATED: return "Updated"
        case BITCOIN_TRANSACTION_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BREthereumTokenEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case TOKEN_EVENT_CREATED: return "Created"
        case TOKEN_EVENT_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BREthereumPeerEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case PEER_EVENT_CREATED: return "Created"
        case PEER_EVENT_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BREthereumTransferEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case TRANSFER_EVENT_CREATED: return "Created"
        case TRANSFER_EVENT_SIGNED: return "Signed"
        case TRANSFER_EVENT_SUBMITTED: return "Submitted"
        case TRANSFER_EVENT_INCLUDED: return "Included"
        case TRANSFER_EVENT_ERRORED: return "Errored"
        case TRANSFER_EVENT_GAS_ESTIMATE_UPDATED: return "Gas Estimate Updated"
        case TRANSFER_EVENT_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BREthereumWalletEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case WALLET_EVENT_CREATED: return "Created"
        case WALLET_EVENT_BALANCE_UPDATED: return "Balance Updated"
        case WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED: return "Default Gas Limit Updated"
        case WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED: return "Default Gas Price Updated"
        case WALLET_EVENT_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BREthereumEWMEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case EWM_EVENT_CREATED: return "Created"
        case EWM_EVENT_SYNC_STARTED: return "Sync Started"
        case EWM_EVENT_SYNC_CONTINUES: return "Sync Continues"
        case EWM_EVENT_SYNC_STOPPED: return "Sync Stopped"
        case EWM_EVENT_NETWORK_UNAVAILABLE: return "Network Unavailable"
        case EWM_EVENT_BLOCK_HEIGHT_UPDATED: return "Block Height Updated"
        case EWM_EVENT_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BRCryptoTransferEventType: CustomStringConvertible {
    public var description: String {
        switch self {
        case CRYPTO_TRANSFER_EVENT_CREATED: return "Created"
        case CRYPTO_TRANSFER_EVENT_CHANGED: return "Changed"
        case CRYPTO_TRANSFER_EVENT_DELETED: return "Deleted"
        default: return "<>unknown>>"
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

        default: return "<>unknown>>"
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
        default: return "<>unknown>>"
        }
    }
}
