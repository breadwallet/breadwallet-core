//
//  BRWalletManagerC.swift
//  BRCrypto
//
//  Created by Ed Gamble on 5/15/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import BRCryptoC

extension WalletManagerMode {
    var asBTC: BRSyncMode {
        switch self {
        case .api_only: return SYNC_MODE_BRD_ONLY
        case .api_with_p2p_submit: return SYNC_MODE_BRD_WITH_P2P_SEND
        case .p2p_with_api_sync: return SYNC_MODE_P2P_WITH_BRD_SYNC
        case .p2p_only: return SYNC_MODE_P2P_ONLY
        }
    }

    var asETH: BREthereumMode {
        switch self {
        case .api_only: return BRD_ONLY
        case .api_with_p2p_submit: return BRD_WITH_P2P_SEND
        case .p2p_with_api_sync: return P2P_WITH_BRD_SYNC
        case .p2p_only: return P2P_ONLY
        }
    }
}

class WalletManagerImplC: WalletManager {
    internal private(set) weak var listener: WalletManagerListener?

    internal let core: BRCryptoWalletManager

    /// The owning system
    public unowned let system: System

    /// The account
    public let account: Account

    /// The network
    public var network: Network


    /// The default unit - as the networks default unit
    internal lazy var unit: Unit = {
        return network.defaultUnitFor(currency: network.currency)!
    }()

    /// The primary wallet - this is typically the wallet where fees are applied which may or may
    /// not differ from the specific wallet used for a transfer (like BRD transfer => ETH fee)
    public lazy var primaryWallet: Wallet = {
        // Find a preexisting wallet (unlikely) or create one.
        return walletByCoreOrCreate (cryptoWalletManagerGetWallet(core),
                                     listener: system.listener,
                                     manager: self,
                                     unit: unit,
                                     create: true)!  // Using '!' is okay as `create` is true
    }()

    /// The manager's wallets, unsorted.
    var wallets: [Wallet] = []

    ///
    /// Add `wallet` to `wallets`.  Will signal WalletEvent.created and
    /// WalletManagerEvent.walletAdded.  If `wallets` already contains `wallet` then nothing
    /// is added.
    ///
    /// - Parameter wallet: the wallet to add
    ///
    internal func add (wallet: Wallet) {
        if !wallets.contains(where: { $0 === wallet }) {
            guard let wallet = wallet as? WalletImplC
                else { precondition (false); return }

            wallets.append (wallet)
            wallet.announceEvent (WalletEvent.created)
            announceEvent (WalletManagerEvent.walletAdded(wallet: wallet))
            (system as? SystemBase)?.updateSubscribedWallets()
        }
    }

    ///
    /// Remove `wallet` from `wallets`.  Will signal WalletManagerEvent.walletDeleted adn then
    //// WalletEvent.deleted.  If `wallets` already contains `wallet` then nothing
    /// is deleted
    ///
    /// - Parameter wallet: the wallet to remove
    ///
    internal func rem (wallet: Wallet) {
        wallets.firstIndex { $0 === wallet }
            .map {
                guard let wallet = wallet as? WalletImplC
                    else { precondition (false); return }

                announceEvent (WalletManagerEvent.walletDeleted (wallet: wallet))
                wallet.announceEvent (WalletEvent.deleted)
                wallets.remove(at: $0)
        }
    }

    internal func upd (wallet: Wallet) {
        if wallets.contains(where: { $0 === wallet }) {
            announceEvent (WalletManagerEvent.walletChanged(wallet: wallet))
        }
    }

    ///
    /// Find a wallet by `impl`
    ///
    /// - Parameter impl: the impl
    /// - Returns: The wallet, if found
    ///
    internal func walletBy (core: BRCryptoWallet) -> WalletImplC? {
        return wallets.first {
            ($0 as? WalletImplC).map { core == $0.core} ?? false
            } as? WalletImplC
    }

    ///
    /// Find a wallet by `impl` or, if `create` is `true`, create one.  This will maintain the
    /// manager <==> wallet constraints (See WalletImplC.init)
    ///
    /// - Parameters:
    ///   - impl: the impl
    ///   - listener: the listener (if created)
    ///   - manager: the manager (if created)
    ///   - unit: the default unit (if created)
    ///   - create: if not found by `impl`, create one if `true`
    ///
    /// - Returns: The wallet
    ///
    internal func walletByCoreOrCreate (_ core: BRCryptoWallet,
                                        listener: WalletListener?,
                                        manager: WalletManagerImplC,
                                        unit: Unit,
                                        create: Bool = false) -> WalletImplC? {
        return walletBy (core: core) ??
            (!create
                ? nil
                : WalletImplC (core: core,
                               listener: listener,
                               manager: manager,
                               unit: unit))
    }

    /// The mode
    public var mode: WalletManagerMode

    /// The path for persistent storage
    public var path: String

    /// The state.  Upon change, generates a WalletManagerEvent.changed event
    public var state: WalletManagerState {
        didSet {
            let newValue = state
            announceEvent (WalletManagerEvent.changed(oldState: oldValue, newState: newValue))
        }
    }

    internal var height: UInt64 {
        get { return network.height }
        set {
            network.height = newValue
            announceEvent (WalletManagerEvent.blockUpdated(height: newValue))
            wallets.flatMap { $0.transfers }
                .forEach {
                    if let confirmations = $0.confirmationsAt (blockHeight: newValue) {
                        ($0 as? TransferImplC)?.announceEvent (TransferEvent.confirmation (count: confirmations))
                    }
            }
        }
    }

    /// The BlockChainDB for BRD Server Assisted queries.
    internal let query: BlockChainDB

    public init (system: System,
                 listener: WalletManagerListener,
                 account: Account,
                 network: Network,
                 mode: WalletManagerMode,
                 storagePath: String) {
        let system = system as! SystemBase

        self.system  = system
        self.account = account
        self.network = network
        self.mode    = mode
        self.path    = storagePath
        self.state   = WalletManagerState.created
        self.query   = system.query

        self.listener = listener

        self.core = cryptoWalletManagerCreate (system.cryptoListener,
                                               system.cryptoClient,
                                               account.core,
                                               nil, // network
                                               mode.asBTC,
                                               storagePath)

//        switch network.currency.code {
//        case Currency.codeAsBTC,
//             Currency.codeAsBCH:
//
//            let bwm:BRWalletManager = BRWalletManagerNew (system.coreBitcoinClient,
//                                                          account.asBTC,
//                                                          network.asBTC,
//                                                          UInt32 (account.timestamp),
//                                                          WalletManagerImplC.modeAsBTC(mode),
//                                                          storagePath)
//            // Hacky?
//            bwmAnnounceBlockNumber (bwm, 0, network.height)
//
//            self.impl = Impl.bitcoin (mid: bwm)
//
//        case Currency.codeAsETH:
//            let ewm:BREthereumEWM = ewmCreate (network.asETH,
//                                               account.asETH,
//                                               UInt64(account.timestamp),
//                                               WalletManagerImplC.modeAsETH (mode),
//                                               system.coreEthereumClient,
//                                               storagePath)
//
//            self.impl = Impl.ethereum (ewm: ewm)
//
//        default:
//            self.impl = Impl.generic
//        }

        print ("SYS: WalletManager (\(name)): Init")
        system.add(manager: self)
    }

    internal func announceEvent (_ event: WalletManagerEvent) {
        self.listener?.handleManagerEvent (system: system,
                                           manager: self,
                                           event: event)
    }

    public func connect() {
        cryptoWalletManagerConnect(core)
    }

    public func disconnect() {
        cryptoWalletManagerDisconnect(core)
    }

    public func submit (transfer: Transfer, paperKey: String) {
        guard let wallet = transfer.wallet as? WalletImplC,
            let transfer = transfer as? TransferImplC
            else { precondition (false) }

        cryptoWalletManagerSubmit (core, wallet.core, transfer.core, paperKey)
    }

    public func sync() {
        cryptoWalletManagerSync(core)
    }
//
//        internal func submit (manager: WalletManagerImplC,
//                              wallet: WalletImplC,
//                              transfer: TransferImplC,
//                              paperKey: String) {
//            switch self {
//            case let .ethereum (ewm):
//                ewmWalletSignTransferWithPaperKey(ewm, wallet.impl.eth, transfer.impl.eth, paperKey)
//                ewmWalletSubmitTransfer (ewm, wallet.impl.eth, transfer.impl.eth)
//
//            case let .bitcoin (mid):
//                let coreWallet      = BRWalletManagerGetWallet      (mid)
//                let corePeerManager = BRWalletManagerGetPeerManager (mid)
//
//                var seed = Account.deriveSeed (phrase: paperKey)
//
//                let  closure = CLangClosure { (error: Int32) in
//                    let event = TransferEvent.changed (
//                        old: transfer.state,
//                        new: (0 == error
//                            ? TransferState.submitted
//                            : TransferState.failed(reason: asUTF8String(strerror(error)))))
//                    transfer.listener?.handleTransferEvent (system: manager.system,
//                                                            manager: manager,
//                                                            wallet: transfer.wallet,
//                                                            transfer: transfer,
//                                                            event: event)
//                }
//
//                BRWalletSignTransaction (coreWallet, transfer.impl.btc, &seed, MemoryLayout<UInt512>.size)
//                BRPeerManagerPublishTx (corePeerManager, transfer.impl.btc,
//                                        Unmanaged<CLangClosure<(Int32)>>.passRetained(closure).toOpaque(),
//                                        { (info, error) in
//                                            guard let info = info else { return }
//                                            let closure = Unmanaged<CLangClosure<(Int32)>>.fromOpaque(info).takeRetainedValue()
//                                            closure.function (error)
//                })
//
//            case .generic:
//                break
//            }
//        }
//    }
}
