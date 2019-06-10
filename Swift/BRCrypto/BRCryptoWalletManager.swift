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
public final class WalletManager {
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
        return Wallet (core: cryptoWalletTake (cryptoWalletManagerGetWallet(core)),
                       listener: system.listener,
                       manager: self,
                       unit: unit)
    }()

    /// The managed wallets - often will just be [primaryWallet]
    public var wallets: [Wallet] {
        let listener = system.listener
        return (0..<cryptoWalletManagerGetWalletsCount(core))
            .map { Wallet (core: cryptoWalletTake (cryptoWalletManagerGetWalletAtIndex (core, $0)),
                           listener: listener,
                           manager: self,
                           unit: unit) }
    }

    ///
    /// Find a wallet by `impl`
    ///
    /// - Parameter impl: the impl
    /// - Returns: The wallet, if found
    ///
    internal func walletBy (core: BRCryptoWallet) -> Wallet? {
        return (0..<cryptoWalletManagerGetWalletsCount(core))
            .map { cryptoWalletManagerGetWalletAtIndex (self.core, $0) }
            .first { core == $0 }
            .map { Wallet (core: cryptoWalletTake($0),
                           listener: system.listener,
                           manager: self,
                           unit: unit) }
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

    func submit (transfer: Transfer, paperKey: String) {
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

        print ("SYS: WalletManager (\(name)): Init")
        system.add(manager: self)
    }

    internal func announceEvent (_ event: WalletManagerEvent) {
        self.listener?.handleManagerEvent (system: system,
                                           manager: self,
                                           event: event)
    }

    deinit {
        cryptoWalletManagerGive (core)
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

                guard let _ = cwm
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
                    guard let wallet = manager.walletBy (core: event.u.wallet.value)
                        else { print ("SYS: Event: \(event.type): Missed (wallet)"); return }
                    manager.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.walletAdded (wallet: wallet))

                case CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED:
                    guard let wallet = manager.walletBy (core: event.u.wallet.value)
                        else { print ("SYS: Event: \(event.type): Missed (wallet)"); return }
                    manager.listener?.handleManagerEvent (system: manager.system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.walletChanged(wallet: wallet))

                case CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED:
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

                guard let _ = cwm, let wid = wid
                    else { print ("SYS: Event: \(event.type): Missed {cwm, wid}"); return }

                guard let wallet   = manager.walletBy  (core: wid)
                    else { print ("SYS: Event: \(event.type): Missed (manaager, wallet)"); return }

                print ("SYS: Event: Wallet (\(wallet.name)): \(event.type)")

                switch event.type {
                case CRYPTO_WALLET_EVENT_CREATED:
                    break

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
                    guard let transfer = wallet.transferBy(core: event.u.transfer.value)
                        else { print ("SYS: Event: \(event.type): Missed (transfer)"); return }
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.transferAdded (transfer: transfer))

                case CRYPTO_WALLET_EVENT_TRANSFER_CHANGED:
                    guard let transfer = wallet.transferBy(core: event.u.transfer.value)
                        else { print ("SYS: Event: \(event.type): Missed (transfer)"); return }
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.transferChanged (transfer: transfer))
                case CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED:
                    guard let transfer = wallet.transferBy(core: event.u.transfer.value)
                        else { print ("SYS: Event: \(event.type): Missed (transfer)"); return }
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.transferSubmitted (transfer: transfer, success: true))

                case CRYPTO_WALLET_EVENT_TRANSFER_DELETED:
                    guard let transfer = wallet.transferBy(core: event.u.transfer.value)
                        else { print ("SYS: Event: \(event.type): Missed (transfer)"); return }
                    wallet.listener?.handleWalletEvent (system: manager.system,
                                                        manager: manager,
                                                        wallet: wallet,
                                                        event: WalletEvent.transferDeleted (transfer: transfer))

                case CRYPTO_WALLET_EVENT_BALANCE_UPDATED:
                    break
                case CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED:
                    break

                default: precondition (false)
                }
        },

            transferEventCallback: { (context, cwm, wid, tid, event) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let wid = wid, let tid = tid
                    else { print ("SYS: Event: \(event.type): Missed {cwm, wid, tid}"); return }

                guard let wallet   = manager.walletBy  (core: wid),
                    let transfer = wallet.transferBy (core: tid)
                    else { print ("SYS: Event: \(event.type): Missed (manaager, wallet, transfer)"); return }

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
            funcGetBlockNumber: { (context, bid, rid) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let bid = bid
                    else { print ("SYS: BTC: GetBlockNumber: Missed {bid}"); return }

                print ("SYS: BTC: GetBlockNumber")
                manager.query.getBlockNumberAsBTC (bwm: bid,
                                                   blockchainId: manager.network.uids,
                                                   rid: rid) {
                                                    (number: UInt64, rid: Int32) in
                                                    bwmAnnounceBlockNumber (bid, rid, number)
                }},

            funcGetTransactions: { (context, bid, begBlockNumber, endBlockNumber, rid) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let bid = bid
                    else { print ("SYS: BTC: GetTransactions: {\(begBlockNumber), \(endBlockNumber)}: Missed {bid}"); return }

                print ("SYS: BTC: GetTransactions: Blocks: {\(begBlockNumber), \(endBlockNumber)}")
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

                    // Announce done
                    bwmAnnounceTransactionComplete (bid, rid, (transactionsError ? 0 : 1))
                }},

            funcSubmitTransaction: { (context, bid, wid, tid, rid) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let bid = bid, let _ = wid, let tid = tid
                    else { print ("SYS: BTC: SubmitTransaction: Missed {bid, wid, tid}"); return }


                let dataCount = BRTransactionSerialize (tid, nil, 0)
                var data = Data (count: dataCount)

                data.withUnsafeMutableBytes { (bytes: UnsafeMutableRawBufferPointer) -> Void in
                    let bytesAsUInt8 = bytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    BRTransactionSerialize (tid, bytesAsUInt8, dataCount)
                }

                manager.query.putTransaction (blockchainId: manager.network.uids,
                                              transaction: data.base64EncodedData(),
                                              completion: { (res: Result<BlockChainDB.Model.Transaction, BlockChainDB.QueryError>) in
                                                var error: Int32 = 0
                                                if case let .failure(f) = res {
                                                    print ("SYS: BTC: SubmitTransaction: Error: \(f)")
                                                    error = 1
                                                }
                                                bwmAnnounceSubmit (bid, rid, tid, error)
                })})
    }
}

extension WalletManager {
    internal var clientETH: BRCryptoCWMClientETH {
        return BRCryptoCWMClientETH (
            funcGetBalance: { (context, coreEWM, wid, address, rid) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                // Non-NULL values from Core Ethereum
                guard let eid = coreEWM, let wid = wid
                    else { print ("SYS: ETH: GetBalance: Missed {eid, wid}"); return }

                let address = asUTF8String(address!)
                if nil == ewmWalletGetToken (eid, wid) {
                    manager.query.getBalanceAsETH (ewm: eid,
                                                   wid: wid,
                                                   address: address,
                                                   rid: rid) { (wid, balance, rid) in
                                                    ewmAnnounceWalletBalance (eid, wid, balance, rid)
                    }
                }
                else {
                    manager.query.getBalanceAsTOK (ewm: eid,
                                                   wid: wid,
                                                   address: address,
                                                   rid: rid) { (wid, balance, rid) in
                                                    ewmAnnounceWalletBalance (eid, wid, balance, rid)
                    }
                }},

            funcGetGasPrice: { (context, coreEWM, wid, rid) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM, let wid = wid
                    else { print ("SYS: ETH: GetGasPrice: Missed {eid, wid}"); return }

                manager.query.getGasPriceAsETH (ewm: eid,
                                                wid: wid,
                                                rid: rid) { (wid, gasPrice, rid) in
                                                    ewmAnnounceGasPrice (eid, wid, gasPrice, rid)
                }},

            funcEstimateGas: { (context, coreEWM, wid, tid, from, to, amount, data, rid)  in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM, let wid = wid, let tid = tid
                    else { print ("SYS: ETH: EstimateGas: Missed {eid, wid, tid}"); return }

                let from = asUTF8String(from!)
                let to = asUTF8String(to!)
                let amount = asUTF8String(amount!)
                let data = asUTF8String(data!)
                manager.query.getGasEstimateAsETH (ewm: eid,
                                                   wid: wid,
                                                   tid: tid,
                                                   from: from,
                                                   to: to,
                                                   amount: amount,
                                                   data: data,
                                                   rid: rid) { (wid, tid, gasEstimate, rid) in
                                                    ewmAnnounceGasEstimate (eid, wid, tid, gasEstimate, rid)
                }},

            funcSubmitTransaction: { (context, coreEWM, wid, tid, transaction, rid)  in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM, let wid = wid, let tid = tid
                    else { print ("SYS: ETH: SubmitTransaction: Missed {eid, wid, tid}"); return }

                let transaction = asUTF8String (transaction!)
                manager.query.submitTransactionAsETH (ewm: eid,
                                                      wid: wid,
                                                      tid: tid,
                                                      transaction: transaction,
                                                      rid: rid) { (wid, tid, hash, errorCode, errorMessage, rid) in
                                                        ewmAnnounceSubmitTransfer (eid,
                                                                                   wid,
                                                                                   tid,
                                                                                   hash,
                                                                                   errorCode,
                                                                                   errorMessage,
                                                                                   rid)
                }},

            funcGetTransactions: { (context, coreEWM, address, begBlockNumber, endBlockNumber, rid) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: GetTransactions: Missed {eid}"); return }

                let address = asUTF8String(address!)
                manager.query.getTransactionsAsETH (ewm: eid,
                                                    address: address,
                                                    begBlockNumber: begBlockNumber,
                                                    endBlockNumber: endBlockNumber,
                                                    rid: rid,
                                                    done: { (success: Bool, rid: Int32) in
                                                        ewmAnnounceTransactionComplete (eid,
                                                                                        rid,
                                                                                        (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                },
                                                    each: { (res: BlockChainDB.ETH.Transaction) in
                                                        ewmAnnounceTransaction (eid,
                                                                                res.rid,
                                                                                res.hash,
                                                                                res.sourceAddr,
                                                                                res.targetAddr,
                                                                                res.contractAddr,
                                                                                res.amount,
                                                                                res.gasLimit,
                                                                                res.gasPrice,
                                                                                res.data,
                                                                                res.nonce,
                                                                                res.gasUsed,
                                                                                res.blockNumber,
                                                                                res.blockHash,
                                                                                res.blockConfirmations,
                                                                                res.blockTransactionIndex,
                                                                                res.blockTimestamp,
                                                                                res.isError)
                })},

            funcGetLogs: { (context, coreEWM, contract, address, event, begBlockNumber, endBlockNumber, rid) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: GetLogs: Missed {eid}"); return }

                let address  = asUTF8String(address!)
                let contract = contract.map { asUTF8String ($0) }
                let event    = asUTF8String (event!)
                manager.query.getLogsAsETH (ewm: eid,
                                            contract: contract,
                                            address: address,
                                            event: event,
                                            begBlockNumber: begBlockNumber,
                                            endBlockNumber: endBlockNumber,
                                            rid: rid,
                                            done: { (success: Bool, rid: Int32) in
                                                ewmAnnounceLogComplete (eid,
                                                                        rid,
                                                                        (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                },
                                            each: { (res: BlockChainDB.ETH.Log) in
                                                var cTopics = res.topics.map { UnsafePointer<Int8>(strdup($0)) }
                                                defer {
                                                    cTopics.forEach { free (UnsafeMutablePointer(mutating: $0)) }
                                                }

                                                ewmAnnounceLog (eid,
                                                                res.rid,
                                                                res.hash,
                                                                res.contract,
                                                                Int32(res.topics.count),
                                                                &cTopics,
                                                                res.data,
                                                                res.gasPrice,
                                                                res.gasUsed,
                                                                res.logIndex,
                                                                res.blockNumber,
                                                                res.blockTransactionIndex,
                                                                res.blockTimestamp)
                })},

            funcGetBlocks: { (context, coreEWM, address, interests, blockStart, blockStop, rid) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: GetBlocks: Missed {eid}"); return }

                let address = asUTF8String(address!)
                manager.query.getBlocksAsETH (ewm: eid,
                                              address: address,
                                              interests: interests,
                                              blockStart: blockStart,
                                              blockStop: blockStop,
                                              rid: rid) { (blocks, rid) in
                                                ewmAnnounceBlocks (eid,
                                                                   rid,
                                                                   Int32(blocks.count),
                                                                   UnsafeMutablePointer<UInt64>(mutating: blocks))
                }},

            funcGetTokens: { (context, coreEWM, rid) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: GetTokens: Missed {eid}"); return }

                manager.query.getTokensAsETH (ewm: eid,
                                              rid: rid,
                                              done: { (success: Bool, rid: Int32) in
                                                ewmAnnounceTokenComplete (eid,
                                                                          rid,
                                                                          (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                },
                                              each: { (res: BlockChainDB.ETH.Token) in
                                                ewmAnnounceToken (eid,
                                                                  res.rid,
                                                                  res.address,
                                                                  res.symbol,
                                                                  res.name,
                                                                  res.description,
                                                                  res.decimals,
                                                                  res.defaultGasLimit,
                                                                  res.defaultGasPrice)
                })},

            funcGetBlockNumber: { (context, coreEWM, rid) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: GetBlockNumber: Missed {eid}"); return }

                manager.query.getBlockNumberAsETH (ewm: eid,
                                                   rid: rid) { (number, rid) in
                                                    ewmAnnounceBlockNumber (eid, number, rid)
                }},

            funcGetNonce: { (context, coreEWM, address, rid) in
                let manager = Unmanaged<WalletManager>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: GetNonce: Missed {eid}"); return }

                let address = asUTF8String(address!)
                manager.query.getNonceAsETH (ewm: eid,
                                             address: address,
                                             rid: rid) { (address, nonce, rid) in
                                                ewmAnnounceNonce (eid, address, nonce, rid)
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
