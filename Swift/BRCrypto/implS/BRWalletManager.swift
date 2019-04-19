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

    // Internal
    internal var impl: Impl

    //    internal var core: BREthereumEWM! = nil

    public unowned let system: System
    public let account: Account
    public var network: Network

    internal lazy var unit: Unit = {
        return network.defaultUnitFor(currency: network.currency)!
    }()

    public lazy var primaryWallet: Wallet = {
        var wallet: Wallet! = nil

        switch impl {
        case let .ethereum (ewm):
            let eth = ewmGetWallet(ewm)
            if let w = lookupWallet(eth: eth) { return w }

            wallet = WalletImplS (listener: system.listener,
                                  manager: self,
                                  unit: unit,
                                  eth: eth!)

        case let .bitcoin (bwm):
            let bwm = BRWalletManagerGetWallet (bwm)
            if let w = lookupWallet(btc: bwm) { return w }
            wallet = WalletImplS (listener: system.listener,
                                  manager: self,
                                  unit: unit,
                                  btc: bwm!)

        case .generic:
            break
        }

        precondition (nil != wallet)

        add (wallet: wallet)
        return wallet
    }()

    var wallets: [Wallet] = []

    internal func add (wallet: Wallet) {
        if !wallets.contains(where: { $0 === wallet }) {
            wallets.append (wallet)
            self.listener?.handleManagerEvent (system: system,
                                               manager: self,
                                               event: WalletManagerEvent.walletAdded(wallet: wallet))
        }
    }

    internal func rem (wallet: Wallet) {
        wallets.firstIndex { $0 === wallet }
            .map {
                wallets.remove(at: $0)
                self.listener?.handleManagerEvent (system: system,
                                                   manager: self,
                                                   event: WalletManagerEvent.walletAdded(wallet: wallet))
        }
    }

    internal func upd (wallet: Wallet) {
        if wallets.contains(where: { $0 === wallet }) {
            self.listener?.handleManagerEvent (system: system,
                                               manager: self,
                                               event: WalletManagerEvent.walletChanged(wallet: wallet))
        }
    }

    internal func lookupWallet (eth: BREthereumWallet?) -> WalletImplS? {
        guard let eth = eth else { return nil }
        return wallets
            .filter { $0 is WalletImplS }
            .first { ($0 as! WalletImplS).impl.matches (eth: eth) } as? WalletImplS
    }

    internal func lookupWallet (btc: BRCoreWallet?) -> WalletImplS? {
        guard let btc = btc else { return nil }
        return wallets
            .filter { $0 is WalletImplS }
            .first { ($0 as! WalletImplS).impl.matches (btc: btc) } as? WalletImplS
    }

    public var mode: WalletManagerMode

    public var path: String

    public var state: WalletManagerState {
        didSet {
            let newValue = state
            self.listener?.handleManagerEvent (system: system,
                                               manager: self,
                                               event: WalletManagerEvent.changed(oldState: oldValue, newState: newValue))
        }
    }

    //    public var walletFactory: WalletFactory = EthereumWalletFactory()

    internal let query: BlockChainDB

    public init (system: System,
                 listener: WalletManagerListener,
                 account: Account,
                 network: Network,
                 mode: WalletManagerMode,
                 storagePath: String) {

        self.system  = system
        self.account = account
        self.network = network
        self.mode    = mode
        self.path    = storagePath
        self.state   = WalletManagerState.created
        self.query   = BlockChainDB()


        let system = system as! SystemBase

        switch network.currency.code {
        case Currency.codeAsBTC,
             Currency.codeAsBCH:

            let bwm:BRWalletManager = BRWalletManagerNew (system.coreBitcoinClient,
                                                          account.asBTC,
                                                          network.asBTC,
                                                          UInt32 (account.timestamp),
                                                          WalletManagerImplS.modeAsBTC(mode),
                                                          storagePath)
            // Hacky?
            bwmAnnounceBlockNumber (bwm, 0, network.height)

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

        listener.handleManagerEvent (system: system,
                                     manager: self,
                                     event: WalletManagerEvent.created)
    }

    public func connect() {
        impl.connect ()
    }

    public func disconnect() {
        impl.disconnect ()
    }

    public func sign (transfer: Transfer, paperKey: String) {
        impl.sign (paperKey: paperKey,
                   wallet: transfer.wallet as! WalletImplS,
                   transfer: transfer as! TransferImplS)
    }

    public func submit (transfer: Transfer) {
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
        
        internal func sign (paperKey: String, wallet: WalletImplS, transfer: TransferImplS) {
            switch self {
            case let .ethereum (ewm):
//                guard let wallet = primaryWallet as? WalletImplS,
//                    let transfer = transfer as? TransferImplS else { precondition(false); return }
                ewmWalletSignTransferWithPaperKey(ewm, wallet.impl.eth, transfer.impl.eth, paperKey)

            case let .bitcoin (mid):
//                guard let transfer = transfer as? TransferImplS else { precondition(false); return }

                let coreWallet      = BRWalletManagerGetWallet      (mid)

                var seed = Account.deriveSeed (phrase: paperKey)
                BRWalletSignTransaction (coreWallet, transfer.impl.btc, &seed, MemoryLayout<UInt512>.size)

            case .generic:
                break
            }

        }

        internal func submit (manager: WalletManagerImplS, wallet: WalletImplS, transfer: TransferImplS) {
            switch self {
            case let .ethereum (ewm):
//                guard let wallet = primaryWallet as? WalletImplS,
//                    let transfer = transfer as? TransferImplS else { precondition(false); return }
                ewmWalletSubmitTransfer (ewm, wallet.impl.eth, transfer.impl.eth)

            case let .bitcoin (mid):
//                guard let transfer = transfer as? TransferImplS else { precondition(false); return }

                let corePeerManager = BRWalletManagerGetPeerManager (mid)

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
