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
/*
    #if false
    private lazy var coreEthereumClient: BREthereumClient = {
        let this = self
        return BREthereumClient (
            context: Unmanaged<WalletManagerImplS>.passUnretained(this).toOpaque(),

            funcGetBalance: { (context, coreEWM, wid, address, rid) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getBalanceAsETH (ewm: this.impl.ewm,
                                            wid: wid!,
                                            address: address,
                                            rid: rid) { (wid, balance, rid) in
                                                ewmAnnounceWalletBalance (this.impl.ewm, wid, balance, rid)
                }},

            funcGetGasPrice: { (context, coreEWM, wid, rid) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                this.query.getGasPriceAsETH (ewm: this.impl.ewm,
                                             wid: wid!,
                                             rid: rid) { (wid, gasPrice, rid) in
                                                ewmAnnounceGasPrice (this.impl.ewm, wid, gasPrice, rid)
                }},

            funcEstimateGas: { (context, coreEWM, wid, tid, from, to, amount, data, rid)  in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                let from = asUTF8String(from!)
                let to = asUTF8String(to!)
                let amount = asUTF8String(amount!)
                let data = asUTF8String(data!)
                this.query.getGasEstimateAsETH (ewm: this.impl.ewm,
                                                wid: wid!,
                                                tid: tid!,
                                                from: from,
                                                to: to,
                                                amount: amount,
                                                data: data,
                                                rid: rid) { (wid, tid, gasEstimate, rid) in
                                                    ewmAnnounceGasEstimate (this.impl.ewm, wid, tid, gasEstimate, rid)

                }},

            funcSubmitTransaction: { (context, coreEWM, wid, tid, transaction, rid)  in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                let transaction = asUTF8String (transaction!)
                this.query.submitTransactionAsETH (ewm: this.impl.ewm,
                                                   wid: wid!,
                                                   tid: tid!,
                                                   transaction: transaction,
                                                   rid: rid) { (wid, tid, hash, errorCode, errorMessage, rid) in
                                                    ewmAnnounceSubmitTransfer (this.impl.ewm,
                                                                               wid,
                                                                               tid,
                                                                               hash,
                                                                               errorCode,
                                                                               errorMessage,
                                                                               rid)
                }},

            funcGetTransactions: { (context, coreEWM, address, begBlockNumber, endBlockNumber, rid) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getTransactionsAsETH (ewm: this.impl.ewm,
                                                 address: address,
                                                 begBlockNumber: begBlockNumber,
                                                 endBlockNumber: endBlockNumber,
                                                 rid: rid,
                                                 done: { (success: Bool, rid: Int32) in
                                                    ewmAnnounceTransactionComplete (this.impl.ewm,
                                                                                    rid,
                                                                                    (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                },
                                                 each: { (res: BlockChainDB.ETH.Transaction) in
                                                    ewmAnnounceTransaction (this.impl.ewm,
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
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getLogsAsETH (ewm: this.impl.ewm,
                                         address: address,
                                         begBlockNumber: begBlockNumber,
                                         endBlockNumber: endBlockNumber,
                                         rid: rid,
                                         done: { (success: Bool, rid: Int32) in
                                            ewmAnnounceLogComplete (this.impl.ewm,
                                                                    rid,
                                                                    (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                },
                                         each: { (res: BlockChainDB.ETH.Log) in
                                            var cTopics = res.topics.map { UnsafePointer<Int8>(strdup($0)) }
                                            defer {
                                                cTopics.forEach { free (UnsafeMutablePointer(mutating: $0)) }
                                            }

                                            ewmAnnounceLog (this.impl.ewm,
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
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getBlocksAsETH (ewm: this.impl.ewm,
                                           address: address,
                                           interests: interests,
                                           blockStart: blockStart,
                                           blockStop: blockStop,
                                           rid: rid) { (blocks, rid) in
                                            ewmAnnounceBlocks (this.impl.ewm, rid,
                                                               Int32(blocks.count),
                                                               UnsafeMutablePointer<UInt64>(mutating: blocks))
                }},

            funcGetTokens: { (context, coreEWM, rid) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                this.query.getTokensAsETH (ewm: this.impl.ewm,
                                           rid: rid,
                                           done: { (success: Bool, rid: Int32) in
                                            ewmAnnounceTokenComplete (this.impl.ewm,
                                                                      rid,
                                                                      (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                },
                                           each: { (res: BlockChainDB.ETH.Token) in
                                            ewmAnnounceToken (this.impl.ewm,
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
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                this.query.getBlockNumberAsETH (ewm: this.impl.ewm,
                                                rid: rid) { (number, rid) in
                                                    ewmAnnounceBlockNumber (this.impl.ewm, number, rid)
                }},

            funcGetNonce: { (context, coreEWM, address, rid) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getNonceAsETH (ewm: this.impl.ewm,
                                          address: address,
                                          rid: rid) { (address, nonce, rid) in
                                            ewmAnnounceNonce (this.impl.ewm, address, nonce, rid)
                }},

            funcEWMEvent: { (context, coreEWM, event, status, message) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                switch event {
                case EWM_EVENT_CREATED:
                    // elsewhere
                    break
                case EWM_EVENT_SYNC_STARTED:
                    this.listener?.handleManagerEvent (system: this.system,
                                                       manager: this,
                                                       event: WalletManagerEvent.syncStarted)
                    break
                case EWM_EVENT_SYNC_CONTINUES:
                    break
                case EWM_EVENT_SYNC_STOPPED:
                    this.listener?.handleManagerEvent (system: this.system,
                                                       manager: this,
                                                       event: WalletManagerEvent.syncEnded (error: message.map { asUTF8String ($0) }))
                    break
                case EWM_EVENT_NETWORK_UNAVAILABLE:
                    // pending
                    break
                case EWM_EVENT_DELETED:
                    // elsewhere
                    break
                default:
                    precondition (false)
                }},

            funcPeerEvent: { (context, coreEWM, event, status, message) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                switch event {
                case PEER_EVENT_CREATED:
                    break
                case PEER_EVENT_DELETED:
                    break
                default:
                    precondition (false)
                }},

            funcWalletEvent: { (context, coreEWM, wid, event, status, message) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()

                if event == WALLET_EVENT_CREATED, let wid = wid {
                    print ("Wallet Created")
                    let currency = ewmWalletGetToken (coreEWM, wid)
                        .flatMap { this.network.currencyBy (code: asUTF8String (tokenGetSymbol ($0))) }
                        ?? this.network.currency

                    let wallet = WalletImplS (listener: this.system.listener,
                                              manager: this,
                                              unit: this.network.defaultUnitFor (currency: currency)!,
                                              eth: wid)

                    this.add (wallet: wallet)
                }

                if let wallet = this.lookupWallet (eth: wid) {
                    switch event {
                    case WALLET_EVENT_CREATED:
                        break
                    case WALLET_EVENT_BALANCE_UPDATED:
                        wallet.upd (balance: wallet.balance)

                    case WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED,
                         WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED:
                        wallet.updateDefaultFeeBasis ()

                    case WALLET_EVENT_DELETED:
                        break

                    default:
                        precondition (false)
                    }
                }},

            funcTokenEvent: { (context, coreEWM, token, event) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                switch event {
                case TOKEN_EVENT_CREATED:
                    break
                case TOKEN_EVENT_DELETED:
                    break
                default:
                    precondition (false)
                }},

            //            funcBlockEvent: { (context, coreEWM, bid, event, status, message) in
            //                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
            //                    //                    ewm.listener.handleBlockEvent(ewm: ewm,
            //                    //                                                 block: ewm.findBlock(identifier: bid),
            //                    //                                                 event: EthereumBlockEvent (event))
            //                }},

            funcTransferEvent: { (context, coreEWM, wid, tid, event, status, message) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                if let wallet = this.lookupWallet (eth: wid) {

                    if TRANSFER_EVENT_CREATED == event, let tid = tid {
                        print ("Transfer Created")
                        let transfer = TransferImplS (listener: this.system.listener,
                                                      wallet: wallet,
                                                      unit: wallet.unit,
                                                      eth: tid)
                        wallet.add (transfer: transfer)
                    }

                    if let transfer = wallet.lookupTransfer (eth: tid) as? TransferImplS {
                        switch event {
                        case TRANSFER_EVENT_CREATED:
                            break

                        case TRANSFER_EVENT_SIGNED:
                            transfer.state = TransferState.signed

                        case TRANSFER_EVENT_SUBMITTED:
                            transfer.state = TransferState.submitted

                        case TRANSFER_EVENT_INCLUDED:
                            var overflow: Int32 = 0
                            let ether = ewmTransferGetFee (coreEWM, tid, &overflow)
                            let confirmation = TransferConfirmation (
                                blockNumber: ewmTransferGetBlockNumber (coreEWM, tid),
                                transactionIndex: ewmTransferGetTransactionIndex (coreEWM, tid),
                                timestamp: ewmTransferGetBlockTimestamp (coreEWM, tid),
                                fee: Amount.createAsETH (ether.valueInWEI, this.unit))

                            transfer.state = TransferState.included(confirmation: confirmation)


                        case TRANSFER_EVENT_ERRORED:
                            transfer.state = TransferState.failed (reason: message.flatMap { asUTF8String ($0) } ?? "<missing>")

                        case TRANSFER_EVENT_GAS_ESTIMATE_UPDATED:
                            break
                        case TRANSFER_EVENT_BLOCK_CONFIRMATIONS_UPDATED:
                            break
                        case TRANSFER_EVENT_DELETED:
                            break
                        default:
                            precondition (false)
                        }}
                }})
    }()

    private lazy var coreBitcoinClient: BRWalletManagerClient = {
        let this = self
        return BRWalletManagerClient (
            context: Unmanaged<WalletManagerImplS>.passUnretained(this).toOpaque(),
            funcTransactionEvent: { (context, bid, wid, tid, event) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                if let wallet = this.lookupWallet (btc: wid) {

                    if event.type == BITCOIN_TRANSACTION_ADDED, let tid = tid {
                        print ("Bitcoin: Transfer Created")
                        let transfer = TransferImplS (listener: this.system.listener,
                                                      wallet: wallet,
                                                      unit: wallet.unit,
                                                      btc: tid)
                    }

                    if let transfer = wallet.lookupTransfer (btc: tid) as? TransferImplS {
                        switch event.type {
                        case BITCOIN_TRANSACTION_ADDED:
                            break
                        case BITCOIN_TRANSACTION_UPDATED:
                            let confirmation = TransferConfirmation (
                                blockNumber: UInt64(event.u.updated.blockHeight),
                                transactionIndex: 0,
                                timestamp: UInt64(event.u.updated.timestamp),
                                fee: Amount.createAsBTC (0, this.unit))

                            transfer.state = TransferState.included (confirmation: confirmation)

                        case BITCOIN_TRANSACTION_DELETED:
                            break
                        default:
                            precondition(false)
                        }}
                }},

            funcWalletEvent: { (context, bid, wid, event) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()

                if event.type == BITCOIN_WALLET_CREATED, let wid = wid {
                    let wallet = WalletImplS (listener: this.system.listener,
                                              manager: this,
                                              unit: this.network.defaultUnitFor(currency: this.network.currency)!,
                                              btc: wid)
                    this.add (wallet: wallet)
                }

                if let wallet = this.lookupWallet (btc: wid) {
                    switch event.type {
                    case BITCOIN_WALLET_CREATED:
                        break

                    case BITCOIN_WALLET_BALANCE_UPDATED:
                        wallet.upd (balance: wallet.balance)

                    case BITCOIN_WALLET_DELETED:
                        break

                    default:
                        precondition(false)
                    }
                }},

            funcWalletManagerEvent: { (context, bid, event) in
                let this = Unmanaged<WalletManagerImplS>.fromOpaque(context!).takeUnretainedValue()
                switch event.type {
                case BITCOIN_WALLET_MANAGER_CONNECTED:
                    this.state = WalletManagerState.connected

                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                    this.state = WalletManagerState.disconnected

                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                    this.state = WalletManagerState.syncing

                    // not so much...
                    this.listener?.handleManagerEvent (system: this.system,
                                                       manager: this,
                                                       event: WalletManagerEvent.syncStarted)

                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                    this.state = WalletManagerState.connected

                    // not so much either ...
                    this.listener?.handleManagerEvent (system: this.system,
                                                       manager: this,
                                                       event: WalletManagerEvent.syncEnded(error: nil))

                default:
                    precondition(false)
                }
        })
    }()

    #endif
*/
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
