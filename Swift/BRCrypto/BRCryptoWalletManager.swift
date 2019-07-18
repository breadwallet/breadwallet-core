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

    /// The Core representation
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
    internal let unit: Unit

    /// The mode determines how the manager manages the account and wallets on network
    public let mode: WalletManagerMode

    /// The file-system path to use for persistent storage.
    public let path: String

    /// The current state
    public var state: WalletManagerState {
        return WalletManagerState (core: cryptoWalletManagerGetState (core))
    }

    /// The current network block height
    internal var height: UInt64 {
        return network.height
    }

   /// The primaryWallet - holds the network's currency - this is typically the wallet where
    /// fees are applied which may or may not differ from the specific wallet used for a
    /// transfer (like BRD transfer => ETH fee)
    public lazy var primaryWallet: Wallet = {
        // Find a preexisting wallet (unlikely) or create one.
        let coreWallet = cryptoWalletManagerGetWallet(core)!
        return Wallet (core: coreWallet,
                       manager: self,
                       take: false)
    }()

    /// The managed wallets - often will just be [primaryWallet]
    public var wallets: [Wallet] {
        let listener = system.listener
        
        var walletsCount: size_t = 0
        let walletsPtr = cryptoWalletManagerGetWallets(core, &walletsCount);
        defer { if let ptr = walletsPtr { free (ptr) } }
        
        let wallets: [BRCryptoWallet] = walletsPtr?.withMemoryRebound(to: BRCryptoWallet.self, capacity: walletsCount) {
            Array(UnsafeBufferPointer (start: $0, count: walletsCount))
        } ?? []
        
        return wallets
            .map { Wallet (core: $0,
                           manager: self,
                           take: false) }
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
                      manager: self,
                      take: true))
    }

    internal func walletByCoreOrCreate (_ core: BRCryptoWallet,
                                          create: Bool = false) -> Wallet? {
        return walletBy (core: core) ??
            (!create
                ? nil
                : Wallet (core: core,
                          manager: self,
                          take: true))
    }

    public var defaultNetworkFee: NetworkFee? = nil

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

    internal init (core: BRCryptoWalletManager,
                   system: System,
                   take: Bool) {

        self.core   = take ? cryptoWalletManagerTake(core) : core
        self.system = system

        let network = Network (core: cryptoWalletManagerGetNetwork (core), take: false)

        self.account = Account (core: cryptoWalletManagerGetAccount(core), uids: "ignore", take: false)
        self.network = network
        self.unit    = network.defaultUnitFor (currency: network.currency)!
        self.path    = asUTF8String (cryptoWalletManagerGetPath(core))
        self.mode    = WalletManagerMode (core: cryptoWalletManagerGetMode (core))
        self.query   = system.query

        self.defaultNetworkFee = network.minimumFee
    }

    public convenience  init (system: System,
                              account: Account,
                              network: Network,
                              mode: WalletManagerMode,
                              storagePath: String,
                              listener: BRCryptoCWMListener,
                              client: BRCryptoCWMClient) {
        self.init (core: cryptoWalletManagerCreate (listener,
                                                    client,
                                                    account.core,
                                                    network.core,
                                                    mode.asCore,
                                                    storagePath),
                   system: system,
                   take: false)
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

    /// An event capturing a change in the block height of the network associated with a
    /// WalletManager. Developers should listen for this event when making use of
    /// Transfer::confirmations, as that value is calculated based on the associated network's
    /// block height. Displays or caches of that confirmation count should be updated when this
    /// event occurs.
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
