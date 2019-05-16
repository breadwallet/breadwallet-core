//
//  BRWalletManager.swift
//  BRCrypto
//
//  Created by Ed Gamble on 4/5/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import BRCryptoC
import BRCore
import BRCore.Ethereum

class WalletManagerImplS: WalletManager {
    internal private(set) weak var listener: WalletManagerListener?

    internal var impl: Impl

    /// The owning system
    public unowned let system: System

    /// The account
    public let account: Account

    /// The network
    public var network: Network

    public static func create (system: System,
                               listener: WalletManagerListener,
                               account: Account,
                               network: Network,
                               mode: WalletManagerMode,
                               storagePath: String) -> WalletManagerImplS {
        let walletManager = WalletManagerImplS (system: system,
                                                listener: listener,
                                                account: account,
                                                network: network,
                                                mode: mode,
                                                storagePath: storagePath)
        walletManager.initialize();
        return walletManager;
    }

    /// The default unit - as the networks default unit
    internal lazy var unit: Unit = {
        return network.defaultUnitFor(currency: network.currency)!
    }()

    /// The primary wallet - this is typically the wallet where fees are applied which may or may
    /// not differ from the specific wallet used for a transfer (like BRD transfer => ETH fee)
    public lazy var primaryWallet: Wallet = {
        var walletImpl: WalletImplS.Impl! = nil

        switch impl {
        case let .ethereum (ewm):
            guard let core = ewmGetWallet(ewm)
                else { print ("SYS: WalletManager: missed ETH primary wallet"); precondition(false) }

            walletImpl = WalletImplS.Impl.ethereum(ewm: ewm, core: core)

        case let .bitcoin (bwm):
            guard let wid = BRWalletManagerGetWallet (bwm)
                else { print ("SYS: WalletManager: missed BTC primary wallet"); precondition(false) }

            walletImpl = WalletImplS.Impl.bitcoin(wid: wid)
        case .generic:
            break
        }

        precondition (nil != walletImpl)

        // Find a preexisting wallet (unlikely) or create one.
        return walletByImplOrCreate (walletImpl!,
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
            guard let wallet = wallet as? WalletImplS
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
                guard let wallet = wallet as? WalletImplS
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
    internal func walletBy (impl: WalletImplS.Impl) -> WalletImplS? {
        return wallets.first {
            ($0 as? WalletImplS)?.impl.matches (impl) ?? false
        } as? WalletImplS
    }

    ///
    /// Find a wallet by `impl` or, if `create` is `true`, create one.  This will maintain the
    /// manager <==> wallet constraints (See WalletImplS.init)
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
    internal func walletByImplOrCreate (_ impl: WalletImplS.Impl,
                                        listener: WalletListener?,
                                        manager: WalletManagerImplS,
                                        unit: Unit,
                                        create: Bool = false) -> WalletImplS? {
        return walletBy (impl: impl) ??
            (!create
                ? nil
                : WalletImplS (listener: listener,
                               manager: manager,
                               unit: unit,
                               impl: impl))
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
                        ($0 as? TransferImplS)?.announceEvent (TransferEvent.confirmation (count: confirmations))
                    }
            }
        }
    }

    /// The BlockChainDB for BRD Server Assisted queries.
    internal let query: BlockChainDB

    private init (system: System,
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

        switch network.currency.code {
        case Currency.codeAsBTC,
             Currency.codeAsBCH:

            let bwm:BRWalletManager = BRWalletManagerNew (system.coreBitcoinClient,
                                                          account.asBTC,
                                                          network.asBTC,
                                                          UInt32 (account.timestamp),
                                                          WalletManagerImplS.modeAsBTC(mode),
                                                          storagePath)

            self.impl = Impl.bitcoin (mid: bwm)

        case Currency.codeAsETH:
            let ewm:BREthereumEWM = ewmCreate (network.asETH,
                                               account.asETH,
                                               UInt64(account.timestamp),
                                               WalletManagerImplS.modeAsETH (mode),
                                               system.coreEthereumClient,
                                               storagePath)

            self.impl = Impl.ethereum (ewm: ewm)

        default:
            self.impl = Impl.generic
        }

        print ("SYS: WalletManager (\(name)): Init")
        system.add(manager: self)
    }

    internal func announceEvent (_ event: WalletManagerEvent) {
        self.listener?.handleManagerEvent (system: system,
                                           manager: self,
                                           event: event)
    }

    private func initialize() {
        impl.initialize(manager: self)
    }

    public func connect() {
        impl.connect ()
    }

    public func disconnect() {
        impl.disconnect ()
    }

    public func submit (transfer: Transfer, paperKey: String) {
        guard let wallet = transfer.wallet as? WalletImplS,
            let transfer = transfer as? TransferImplS
            else { precondition (false) }

        impl.submit (manager: self,
                     wallet: wallet,
                     transfer: transfer,
                     paperKey: paperKey)
    }

    public func sync() {
        impl.sync()
    }

    // Actually a Set/Dictionary by {Symbol}
    //    public private(set) var all: [EthereumToken] = []
    //
    //    internal func addToken (identifier: BREthereumToken) {
    //        let symbol = asUTF8String (tokenGetSymbol (identifier))
    //        if let currency = network.currencyBy (code: symbol) {
    //            let token = EthereumToken (identifier: identifier, currency: currency)
    //            all.append (token)
    //            //            self._listener.handleTokenEvent(manager: self, token: token, event: EthereumTokenEvent.created)
    //        }
    //    }
    //
    //    internal func remToken (identifier: BREthereumToken) {
    //        if let index = all.firstIndex (where: { $0.identifier == identifier}) {
    //            //            let token = all[index]
    //            all.remove(at: index)
    //            //            self._listener.handleTokenEvent(manager: self, token: token, event: EthereumTokenEvent.deleted)
    //        }
    //    }
    //
    //    internal func findToken (identifier: BREthereumToken) -> EthereumToken? {
    //        return all.first { $0.identifier == identifier }
    //    }

    private static func modeAsETH (_ mode: WalletManagerMode) -> BREthereumMode {
        switch mode {
        case .api_only: return BRD_ONLY
        case .api_with_p2p_submit: return BRD_WITH_P2P_SEND
        case .p2p_with_api_sync: return P2P_WITH_BRD_SYNC
        case .p2p_only: return P2P_ONLY
        }
    }

    private static func modeAsBTC (_ mode: WalletManagerMode) -> BRSyncMode {
        switch mode {
        case .api_only: return SYNC_MODE_BRD_ONLY
        case .api_with_p2p_submit: return SYNC_MODE_BRD_WITH_P2P_SEND
        case .p2p_with_api_sync: return SYNC_MODE_P2P_WITH_BRD_SYNC
        case .p2p_only: return SYNC_MODE_P2P_ONLY
        }
    }
    
    enum Impl {
        case bitcoin (mid: BRCoreWalletManager)
        case ethereum (ewm: BREthereumEWM)
        case generic

        internal var ewm: BREthereumEWM! {
            switch self {
            case .bitcoin: return nil
            case .ethereum (let ewm): return ewm
            case .generic: return nil
            }
        }

        internal var bwm: BRCoreWalletManager! {
            switch self {
            case .bitcoin (let mid): return mid
            case .ethereum: return nil
            case .generic: return nil
            }
        }

        internal func matches (_ that: Impl) -> Bool {
            switch (self, that) {
            case (let .bitcoin (bwm1), let .bitcoin (bwm2)):
                return bwm1 == bwm2
            case (let .ethereum (ewm1), let .ethereum (ewm2)):
                return ewm1 == ewm2
            default:
                return false
            }
        }

        internal func initialize(manager: WalletManagerImplS) {
            switch self {
            case let .bitcoin (bwm):
                BRWalletManagerInit (bwm)
                // Hacky?
                bwmAnnounceBlockNumber (bwm, 0, manager.network.height)
            default:
                break
            }
        }

        internal func connect() {
            switch self {
            case let .ethereum (ewm):
                ewmConnect (ewm)
            case let .bitcoin (mid):
                BRWalletManagerConnect (mid)
            case .generic:
                break
            }
        }
        internal func disconnect() {
            switch self {
            case let .ethereum (ewm):
                ewmDisconnect (ewm)
            case let .bitcoin (mid):
                BRWalletManagerDisconnect (mid)
            case .generic:
                break
            }
        }

        internal func sync() {
            switch self {
            case let .ethereum (ewm):
                ewmSync (ewm);
            case let .bitcoin (mid):
                BRWalletManagerScan (mid)
            case .generic:
                break
            }
        }
        
        internal func submit (manager: WalletManagerImplS,
                              wallet: WalletImplS,
                              transfer: TransferImplS,
                              paperKey: String) {
            switch self {
            case let .ethereum (ewm):
                ewmWalletSignTransferWithPaperKey(ewm, wallet.impl.eth, transfer.impl.eth, paperKey)
                ewmWalletSubmitTransfer (ewm, wallet.impl.eth, transfer.impl.eth)

            case let .bitcoin (mid):
                let coreWallet      = BRWalletManagerGetWallet      (mid)
                let corePeerManager = BRWalletManagerGetPeerManager (mid)

                var seed = Account.deriveSeed (phrase: paperKey)

                let  closure = CLangClosure { (error: Int32) in
                    let event = TransferEvent.changed (
                        old: transfer.state,
                        new: (0 == error
                            ? TransferState.submitted
                            : TransferState.failed(reason: asUTF8String(strerror(error)))))
                    transfer.listener?.handleTransferEvent (system: manager.system,
                                                            manager: manager,
                                                            wallet: transfer.wallet,
                                                            transfer: transfer,
                                                            event: event)
                }

                BRWalletSignTransaction (coreWallet, transfer.impl.btc, &seed, MemoryLayout<UInt512>.size)
                BRPeerManagerPublishTx (corePeerManager, transfer.impl.btc,
                                        Unmanaged<CLangClosure<(Int32)>>.passRetained(closure).toOpaque(),
                                        { (info, error) in
                                            guard let info = info else { return }
                                            let closure = Unmanaged<CLangClosure<(Int32)>>.fromOpaque(info).takeRetainedValue()
                                            closure.function (error)
                })

            case .generic:
                break
            }
        }
    }
}
