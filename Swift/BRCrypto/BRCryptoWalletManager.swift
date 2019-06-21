//
//  BRCrypto.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import Foundation // Data
import BRCryptoC

///
/// A WallettManager manages one or more wallets one of which is designated the `primaryWallet`.
/// (For example, an EthereumWalletManager will manage an ETH wallet and one wallet for each
/// ERC20Token; the ETH wallet will be the primaryWallet.  A BitcoinWalletManager manages one
/// and only one wallet holding BTC.).
///
/// At least conceptually, a WalletManager is an 'Active Object' (whereas Transfer and Wallet are
/// 'Passive Objects'
///
public final class WalletManager: Equatable {
    internal private(set) weak var listener: WalletManagerListener?

    internal private(set) var core: BRCryptoWalletManager! = nil

    /// The owning system
    public unowned let system: System

    /// The account
    public let account: Account

    /// The network
    public let network: Network

    /// The BlockChainDB for BRD Server Assisted queries.
    internal let query: BlockChainDB

    /// The default unit - as the networks default unit
    internal lazy var unit: Unit = {
        return network.defaultUnitFor(currency: network.currency)!
    }()

    /// The primaryWallet - holds the network's currency - this is typically the wallet where
    /// fees are applied which may or may not differ from the specific wallet used for a
    /// transfer (like BRD transfer => ETH fee)
    public lazy var primaryWallet: Wallet = {
        // Find a preexisting wallet (unlikely) or create one.
        let coreWallet = cryptoWalletManagerGetWallet(core)!
        return Wallet (core: coreWallet,
                       listener: system.listener,
                       manager: self,
                       take: false)
    }()

    /// The managed wallets - often will just be [primaryWallet]
    public var wallets: [Wallet] {
        let listener = system.listener
        return (0..<cryptoWalletManagerGetWalletsCount(core))
            .map {
                let coreWallet = cryptoWalletManagerGetWalletAtIndex (core, $0)!
                return Wallet (core: coreWallet,
                               listener: listener,
                               manager: self,
                               take: false)
        }
    }

    ///
    /// Find a wallet by `impl`
    ///
    /// - Parameter impl: the impl
    /// - Returns: The wallet, if found
    ///
    internal func walletBy (core: BRCryptoWallet) -> Wallet? {
        return (CRYPTO_FALSE == cryptoWalletManagerHasWallet (self.core, core)
            ? nil
            : Wallet (core: core,
                      listener: system.listener,
                      manager: self,
                      take: true))
    }

    internal func walletByCoreOrCreate (_ core: BRCryptoWallet,
                                          listener: WalletListener?,
                                          create: Bool = false) -> Wallet? {
        return walletBy (core: core) ??
            (!create
                ? nil
                : Wallet (core: core,
                          listener: listener,
                          manager: self,
                          take: true))
    }

    // The mode determines how the manager manages the account and wallets on network
    public private(set) lazy var mode: WalletManagerMode = {
        return WalletManagerMode (core: cryptoWalletManagerGetMode (core))
    }()

    // The file-system path to use for persistent storage.
    public private(set) lazy var path: String = {
        return asUTF8String (cryptoWalletManagerGetPath(core))
    }()

    public var state: WalletManagerState {
        return WalletManagerState (core: cryptoWalletManagerGetState (core))
    }

    internal var height: UInt64 {
        get { return network.height }
        set {
            network.height = newValue
            announceEvent (WalletManagerEvent.blockUpdated(height: newValue))
            wallets.flatMap { $0.transfers }
                .forEach {
                    if let confirmations = $0.confirmationsAt (blockHeight: newValue) {
                        $0.announceEvent (TransferEvent.confirmation (count: confirmations))
                    }
            }
        }
    }

    /// The default WalletFactory for creating wallets.
    //    var walletFactory: WalletFactory { get set }

    /// Connect to network and begin managing wallets for account
    public func connect () {
        cryptoWalletManagerConnect (core)
    }

    /// Disconnect from the network.
    public func disconnect () {
        cryptoWalletManagerDisconnect (core)
    }

    public func sync () {
        cryptoWalletManagerSync (core)
    }

    public func submit (transfer: Transfer, paperKey: String) {
        cryptoWalletManagerSubmit (core,
                                   transfer.wallet.core,
                                   transfer.core,
                                   paperKey)
    }

    public init (system: System,
                 listener: WalletManagerListener,
                 account: Account,
                 network: Network,
                 mode: WalletManagerMode,
                 storagePath: String) {

        print ("SYS: WalletManager (\(network.currency.code)): Init")

        self.system  = system
        self.account = account
        self.network = network
        self.query   = system.query

        self.listener = listener

        self.core = cryptoWalletManagerCreate (cryptoListener,
                                               cryptoClient,
                                               account.core,
                                               network.core,
                                               mode.asCore,
                                               storagePath)
    }

    internal func announceEvent (_ event: WalletManagerEvent) {
        self.listener?.handleManagerEvent (system: system,
                                           manager: self,
                                           event: event)
    }

    deinit {
        cryptoWalletManagerGive (core)
    }

    // Equatable
    public static func == (lhs: WalletManager, rhs: WalletManager) -> Bool {
        return lhs === rhs || lhs.core == rhs.core
    }
}

extension WalletManager {
    ///
    /// Create a wallet for `currency`.  Invokdes the manager's `walletFactory` to create the
    /// wallet.  Generates events: Wallet.created, WalletManager.walletAdded(wallet), perhaps
    /// others.
    ///
    /// - Parameter currency: the wallet's currency
    ///
    /// - Returns: a new wallet.
    ///
    //    func createWallet (currency: Currency) -> Wallet {
    //        return walletFactory.createWallet (manager: self,
    //                                           currency: currency)
    //    }

    /// The network's/primaryWallet's currency.  This is the currency used for transfer fees.
    var currency: Currency {
        return network.currency // don't reference `primaryWallet`; infinitely recurses
    }

    /// The name is simply the network currency's code - e.g. BTC, ETH
    public var name: String {
        return currency.code
    }

    /// The baseUnit for the network's currency.
    var baseUnit: Unit {
        return network.baseUnitFor(currency: network.currency)!
    }

    /// The defaultUnit for the network's currency.
    var defaultUnit: Unit {
        return network.defaultUnitFor(currency: network.currency)!
    }
    
    /// A manager `isActive` if connected or syncing
    var isActive: Bool {
        return state == .connected || state == .syncing
    }
}

///
/// The WalletManager state.
///
public enum WalletManagerState {
    case created
    case disconnected
    case connected
    case syncing
    case deleted

    internal init (core: BRCryptoWalletManagerState) {
        switch core {
        case CRYPTO_WALLET_MANAGER_STATE_CREATED:      self = .created
        case CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED: self = .disconnected
        case CRYPTO_WALLET_MANAGER_STATE_CONNECTED:    self = .connected
        case CRYPTO_WALLET_MANAGER_STATE_SYNCING:      self = .syncing
        case CRYPTO_WALLET_MANAGER_STATE_DELETED:      self = .deleted
        default: self = .created; precondition(false)
        }
    }
}

///
/// The WalletManager's mode determines how the account and associated wallets are managed.
///
/// - api_only: Use only the defined 'Cloud-Based API' to synchronize the account's transfers.
///
/// - api_with_p2p_submit: Use the defined 'Cloud-Based API' to synchronize the account's transfers
///      but submit transfers using the network's Peer-to-Peer protocol.
///
/// - p2p_with_api_sync: Use the network's Peer-to-Peer protocol to synchronize the account's
///      recents transfers but use the 'Cloud-Based API' to synchronize older transfers.
///
/// - p2p_only: Use the network's Peer-to-Peer protocol to synchronize the account's transfers.
///
public enum WalletManagerMode {
    case api_only
    case api_with_p2p_submit
    case p2p_with_api_sync
    case p2p_only

    internal init (core: BRSyncMode) {
        switch core {
        case SYNC_MODE_BRD_ONLY: self = .api_only
        case SYNC_MODE_BRD_WITH_P2P_SEND: self = .api_with_p2p_submit
        case SYNC_MODE_P2P_WITH_BRD_SYNC: self = .p2p_with_api_sync
        case SYNC_MODE_P2P_ONLY: self = .p2p_only
        default: self = .api_only; precondition (false)
        }
    }

    internal var asCore: BRSyncMode {
        switch self {
        case .api_only: return SYNC_MODE_BRD_ONLY
        case .api_with_p2p_submit: return SYNC_MODE_BRD_WITH_P2P_SEND
        case .p2p_with_api_sync: return SYNC_MODE_P2P_WITH_BRD_SYNC
        case .p2p_only: return SYNC_MODE_P2P_ONLY
        }
    }
}

///
/// A WalletManager Event represents a asynchronous announcment of a managera's state change.
///
public enum WalletManagerEvent {
    case created
    case changed (oldState: WalletManagerState, newState: WalletManagerState)
    case deleted

    case walletAdded (wallet: Wallet)
    case walletChanged (wallet: Wallet)
    case walletDeleted (wallet: Wallet)

    case syncStarted
    case syncProgress (percentComplete: Double)
    case syncEnded (error: String?)

    case blockUpdated (height: UInt64)
}

///
/// Listener For WalletManagerEvent
///
public protocol WalletManagerListener: class {
    ///
    /// Handle a WalletManagerEvent.
    ///
    /// - Parameters:
    ///   - system: the system
    ///   - manager: the manager
    ///   - event: the event
    ///
    func handleManagerEvent (system: System,
                             manager: WalletManager,
                             event: WalletManagerEvent)

}

public protocol WalletManagerFactory { }


extension WalletManager {
    internal var cryptoListener: BRCryptoCWMListener {
        let this = self
        return BRCryptoCWMListener (
            context: Unmanaged<WalletManager>.passUnretained(this).toOpaque(),

            walletManagerEventCallback: { (context, cwm, event) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()
                defer {
                    if let cwm = cwm { cryptoWalletManagerGive(cwm) }
                }

                guard let cwm = cwm
                    else { print ("SYS: Event: \(event.type): Missed {cwm}"); return }

                print ("SYS: Event: Manager (\(manager.name)): \(event.type)")

                switch event.type {
                case CRYPTO_WALLET_MANAGER_EVENT_CREATED:
                    // We are only here in response to system.createWalletManager...
                    break

                case CRYPTO_WALLET_MANAGER_EVENT_CHANGED:
                    manager.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.changed (oldState: WalletManagerState (core: event.u.state.oldValue),
                                                                                             newState: WalletManagerState (core: event.u.state.newValue)))

                case CRYPTO_WALLET_MANAGER_EVENT_DELETED:
                    manager.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.deleted)

                case CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED:
                    defer { if let wid = event.u.wallet.value { cryptoWalletGive (wid) }}
                    guard let wallet = manager.walletBy (core: event.u.wallet.value)
                        else { print ("SYS: Event: \(event.type): Missed (wallet)"); return }
                    manager.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.walletAdded (wallet: wallet))

                case CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED:
                    defer { if let wid = event.u.wallet.value { cryptoWalletGive (wid) }}
                    guard let wallet = manager.walletBy (core: event.u.wallet.value)
                        else { print ("SYS: Event: \(event.type): Missed (wallet)"); return }
                    manager.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.walletChanged(wallet: wallet))

                case CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED:
                    defer { if let wid = event.u.wallet.value { cryptoWalletGive (wid) }}
                    guard let wallet = manager.walletBy (core: event.u.wallet.value)
                        else { print ("SYS: Event: \(event.type): Missed (wallet)"); return }
                    manager.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.walletDeleted(wallet: wallet))

                case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED:
                    manager.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.syncStarted)

                case CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES:
                    manager.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.syncProgress (percentComplete: Double (event.u.sync.percentComplete)))

                case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED:
                    manager.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.syncEnded(error: nil))

                case CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED:
                    manager.height = event.u.blockHeight.value

                default: precondition(false)
                }
        },

            walletEventCallback: { (context, cwm, wid, event) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()
                if nil == manager.core { manager.core = cwm }
                defer {
                    if let cwm = cwm { cryptoWalletManagerGive(cwm) }
                    if let wid = wid { cryptoWalletGive(wid) }
                }

                guard let _ = cwm, let wid = wid
                    else { print ("SYS: Event: \(event.type): Missed {cwm, wid}"); return }

                let wallet = manager.walletByCoreOrCreate (wid,
                                                           listener: manager.system.listener,
                                                           create: true)!

                print ("SYS: Event: Wallet (\(wallet.name)): \(event.type)")

                switch event.type {
                case CRYPTO_WALLET_EVENT_CREATED:
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event:  WalletEvent.created)

                case CRYPTO_WALLET_EVENT_CHANGED:
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.changed (oldState: WalletState (core: event.u.state.oldState),
                                                                                    newState: WalletState (core: event.u.state.newState)))
                case CRYPTO_WALLET_EVENT_DELETED:
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.deleted)

                case CRYPTO_WALLET_EVENT_TRANSFER_ADDED:
                    defer { if let tid = event.u.transfer.value { cryptoTransferGive(tid) }}
                    guard let transfer = wallet.transferBy (core: event.u.transfer.value)
                        else { print ("SYS: Event: \(event.type): Missed (transfer)"); return }
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.transferAdded (transfer: transfer))

                case CRYPTO_WALLET_EVENT_TRANSFER_CHANGED:
                    defer { if let tid = event.u.transfer.value { cryptoTransferGive(tid) }}
                    guard let transfer = wallet.transferBy (core: event.u.transfer.value)
                        else { print ("SYS: Event: \(event.type): Missed (transfer)"); return }
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.transferChanged (transfer: transfer))
                case CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED:
                    defer { if let tid = event.u.transfer.value { cryptoTransferGive(tid) }}
                    guard let transfer = wallet.transferBy (core: event.u.transfer.value)
                        else { print ("SYS: Event: \(event.type): Missed (transfer)"); return }
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.transferSubmitted (transfer: transfer, success: true))

                case CRYPTO_WALLET_EVENT_TRANSFER_DELETED:
                    defer { if let tid = event.u.transfer.value { cryptoTransferGive(tid) }}
                   guard let transfer = wallet.transferBy (core: event.u.transfer.value)
                        else { print ("SYS: Event: \(event.type): Missed (transfer)"); return }
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.transferDeleted (transfer: transfer))

                case CRYPTO_WALLET_EVENT_BALANCE_UPDATED:
                    let amount = Amount (core: event.u.balanceUpdated.amount,
                                         unit: wallet.unit,
                                         take: false)
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.balanceUpdated(amount: amount))

                case CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED:
                    let feeBasis = TransferFeeBasis (core: event.u.feeBasisUpdated.basis,
                                                     take: false)
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.feeBasisUpdated(feeBasis: feeBasis))

                default: precondition (false)
                }
        },

            transferEventCallback: { (context, cwm, wid, tid, event) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()
                defer {
                    if let cwm = cwm { cryptoWalletManagerGive(cwm) }
                    if let wid = wid { cryptoWalletGive(wid) }
                    if let tid = tid { cryptoTransferGive(tid) }
                }

                guard let _ = cwm, let wid = wid, let tid = tid
                    else { print ("SYS: Event: \(event.type): Missed {cwm, wid, tid}"); return }

                guard let wallet = manager.walletBy (core: wid),
                    let transfer = wallet.transferByCoreOrCreate (tid,
                                                                  listener: manager.system.listener,
                                                                  create: CRYPTO_TRANSFER_EVENT_CREATED == event.type)
                    else { print ("SYS: Event: \(event.type): Missed (manager, wallet, transfer)"); return }

                print ("SYS: Event: Transfer (\(wallet.name) @ \(transfer.hash?.description ?? "pending")): \(event.type)")

                switch (event.type) {
                case CRYPTO_TRANSFER_EVENT_CREATED:
                    transfer.listener?.handleTransferEvent (system: manager.system,
                                                            manager: manager,
                                                            wallet: wallet,
                                                            transfer: transfer,
                                                            event: TransferEvent.created)

                case CRYPTO_TRANSFER_EVENT_CHANGED:
                    transfer.listener?.handleTransferEvent (system: manager.system,
                                                            manager: manager,
                                                            wallet: wallet,
                                                            transfer: transfer,
                                                            event: TransferEvent.changed (old: TransferState.init (core: event.u.state.old),
                                                                                          new: TransferState.init (core: event.u.state.new)))

                case CRYPTO_TRANSFER_EVENT_CONFIRMED:
                    transfer.listener?.handleTransferEvent (system: manager.system,
                                                            manager: manager,
                                                            wallet: wallet,
                                                            transfer: transfer,
                                                            event: TransferEvent.confirmation(count: 10))

                case CRYPTO_TRANSFER_EVENT_DELETED:
                    transfer.listener?.handleTransferEvent (system: manager.system,
                                                            manager: manager,
                                                            wallet: wallet,
                                                            transfer: transfer,
                                                            event: TransferEvent.deleted)
                default: precondition(false)
                }
        })
    }
}

extension WalletManager {
    internal var clientBTC: BRCryptoCWMClientBTC {
        return BRCryptoCWMClientBTC (
            funcGetBlockNumber: { (context, cwm, sid) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()
                precondition (nil != cwm, "SYS: BTC: GetBlockNumber: Missed {cwm}")

                print ("SYS: BTC: GetBlockNumber")
                manager.query.getBlockchain (blockchainId: manager.network.uids) { (res: Result<BlockChainDB.Model.Blockchain, BlockChainDB.QueryError>) in
                    res.resolve (
                        success: { cwmAnnounceGetBlockNumberSuccessAsInteger (manager.core, sid, $0.blockHeight) },
                        failure: { (_) in cwmAnnounceGetBlockNumberFailure (manager.core, sid) })
                }},

            funcGetTransactions: { (context, cwm, sid, addresses, addressesCount, begBlockNumber, endBlockNumber) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()
                precondition (nil != cwm, "SYS: BTC: GetTransactions: Missed {cwm}")

                print ("SYS: BTC: GetTransactions: Blocks: {\(begBlockNumber), \(endBlockNumber)}")
                let addresses = [String]()

                manager.query.getTransactions (blockchainId: manager.network.uids,
                                               addresses: addresses,
                                               begBlockNumber: begBlockNumber,
                                               endBlockNumber: endBlockNumber,
                                               includeRaw: true) {
                    (res: Result<[BlockChainDB.Model.Transaction], BlockChainDB.QueryError>) in
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

            funcSubmitTransaction: { (context, cwm, sid, transactionBytes, transactionBytesLength) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()
                precondition (nil != cwm, "SYS: BTC: SubmitTransaction: Missed {cwm}")

                print ("SYS: BTC: SubmitTransaction")
                let data = Data (bytes: transactionBytes!, count: transactionBytesLength)
                manager.query.putTransaction (blockchainId: manager.network.uids, transaction: data) {
                    (res: Result<BlockChainDB.Model.Transaction, BlockChainDB.QueryError>) in
                    res.resolve(
                        success: { (_) in cwmAnnounceSubmitTransferSuccess (cwm, sid) },
                        failure: { (_) in cwmAnnounceSubmitTransferFailure (cwm, sid) })
                }
        })
    }
}

extension WalletManager {
    internal var clientETH: BRCryptoCWMClientETH {
        return BRCryptoCWMClientETH (
            funcGetEtherBalance: { (context, cwm, sid, network, address) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network = asUTF8String (networkGetName (ewmGetNetwork (ewm)))
                let address = asUTF8String (address!)

                manager.query.getBalanceAsETH (network: network, address: address) {
                    (res: Result<String, BlockChainDB.QueryError>) in
                    res.resolve (
                        success: { cwmAnnounceGetBalanceSuccess (cwm, sid, $0) },
                        failure: { (_) in cwmAnnounceGetBalanceFailure (cwm, sid) })
                }},

            funcGetTokenBalance: { (context, cwm, sid, network, address, contract) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))
                let address  = asUTF8String (address!)
                let contract = asUTF8String (contract!)

                manager.query.getBalanceAsTOK (network: network, address: address, contract: contract) {
                    (res: Result<String, BlockChainDB.QueryError>) in
                    res.resolve (
                        success: { cwmAnnounceGetBalanceSuccess (cwm, sid, $0) },
                        failure: { (_) in cwmAnnounceGetBalanceFailure (cwm, sid) })
                }},

            funcGetGasPrice: { (context, cwm, sid, network) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.getGasPriceAsETH (network: network) {
                    (res: Result<String, BlockChainDB.QueryError>) in
                    res.resolve (
                        success: { cwmAnnounceGetGasPriceSuccess (cwm, sid, $0) },
                        failure: { (_) in cwmAnnounceGetGasPriceFailure (cwm, sid) })
                }},

            funcEstimateGas: { (context, cwm, sid, network, from, to, amount, data) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.getGasEstimateAsETH (network: network,
                                                   from:   asUTF8String(from!),
                                                   to:     asUTF8String(to!),
                                                   amount: asUTF8String(amount!),
                                                   data:   asUTF8String(data!)) {
                                                    (res: Result<String, BlockChainDB.QueryError>) in
                                                    res.resolve (
                                                        success: { cwmAnnounceGetGasEstimateSuccess (cwm, sid, $0) },
                                                        failure: { (_) in cwmAnnounceGetGasEstimateFailure (cwm, sid) })
                }},

            funcSubmitTransaction: { (context, cwm, sid, network, transaction) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.submitTransactionAsETH (network: network,
                                                      transaction: asUTF8String(transaction!)) {
                                                        (res: Result<String, BlockChainDB.QueryError>) in
                                                        res.resolve (
                                                            success: { cwmAnnounceSubmitTransferSuccessForHash (cwm, sid, $0) },
                                                            failure: { (_) in cwmAnnounceSubmitTransferFailure (cwm, sid) })
                }},

            funcGetTransactions: { (context, cwm, sid, network, address, begBlockNumber, endBlockNumber) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

               manager.query.getTransactionsAsETH (network: network,
                                                   address: asUTF8String (address!),
                                                   begBlockNumber: begBlockNumber,
                                                   endBlockNumber: endBlockNumber) {
                                                    (res: Result<[BlockChainDB.ETH.Transaction], BlockChainDB.QueryError>) in
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
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.getLogsAsETH (network: network,
                                            contract: contract.map { asUTF8String($0) },
                                            address:  asUTF8String(address!),
                                            event:    asUTF8String(event!),
                                            begBlockNumber: begBlockNumber,
                                            endBlockNumber: endBlockNumber) {
                                                (res: Result<[BlockChainDB.ETH.Log], BlockChainDB.QueryError>) in
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
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network  = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.getBlocksAsETH (network: network,
                                              address: asUTF8String(address!),
                                              interests: interests,
                                              blockStart: begBlockNumber,
                                              blockStop:  endBlockNumber) {
                                                (res: Result<[UInt64], BlockChainDB.QueryError>) in
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
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                manager.query.getTokensAsETH () {
                    (res: Result<[BlockChainDB.ETH.Token],BlockChainDB.QueryError>) in
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
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.getBlockNumberAsETH (network: network) {
                    (res: Result<String, BlockChainDB.QueryError>) in
                    res.resolve (
                        success: { cwmAnnounceGetBlockNumberSuccessAsString (cwm, sid, $0) },
                        failure: { (_) in cwmAnnounceGetBlockNumberFailure (cwm, sid) })
                }},

            funcGetNonce: { (context, cwm, sid, network, address) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                let ewm = cryptoWalletManagerAsETH (cwm);
                let network = asUTF8String (networkGetName (ewmGetNetwork (ewm)))

                manager.query.getNonceAsETH (network: network, address: asUTF8String(address!)) {
                    (res: Result<String, BlockChainDB.QueryError>) in
                    res.resolve (
                        success: { cwmAnnounceGetNonceSuccess (cwm, sid, address, $0) },
                        failure: { (_) in cwmAnnounceGetNonceFailure (cwm, sid) })
                }})
    }
}

extension WalletManager {
    internal var clientGEN: BRCryptoCWMClientGEN {
        return BRCryptoCWMClientGEN ()
    }
}

extension WalletManager {
    internal var cryptoClient: BRCryptoCWMClient {
        return BRCryptoCWMClient (context: Unmanaged<WalletManager>.passUnretained(self).toOpaque(),
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
        case CRYPTO_TRANSFER_EVENT_CONFIRMED: return "Confirmed"
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


