//
//  BREthereum.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//

import BRCryptoC
import BRCore.Ethereum

extension Amount {
    fileprivate var asETH: UInt64 {
        var overflow: BRCryptoBoolean = CRYPTO_FALSE
        let value = cryptoAmountGetIntegerRaw (self.core, &overflow)
        precondition(CRYPTO_FALSE == overflow)
        return value
    }

    static fileprivate func createAsETH (_ value: UInt256, _ unit: Unit) -> Amount {
        return Amount (core: cryptoAmountCreate(unit.currency.core, CRYPTO_FALSE, value),
                       unit: unit);
    }
}

extension Address {
    static fileprivate func createAsETH (_ eth: BREthereumAddress) -> Address  {
        return Address (core: cryptoAddressCreateAsETH (eth))
    }
}

extension Network {
    fileprivate var asETH: BREthereumNetwork! {
        switch self.impl {
        case let .ethereum(_, network): return network
        default: precondition(false); return nil
        }
    }
}

extension Account {
    fileprivate var asETH: BREthereumAccount {
        return cryptoAccountAsETH (self.core)
    }
}

///
/// An ERC20 Smart Contract Token
///
public class EthereumToken {

    /// A reference to the Core's BREthereumToken
    internal let identifier: BREthereumToken

    /// The currency
    public let currency: Currency

    /// The address of the token's ERC20 Smart Contract
    public let address: Address

    internal init (identifier: BREthereumToken,
                   currency: Currency) {
        self.identifier = identifier
        self.address = Address (core: cryptoAddressCreateAsETH(tokenGetAddressRaw(identifier)))
        self.currency = currency
    }
}

///
/// An EthereumtokenEvent represents a asynchronous announcment of a token's state change.
///
public enum EthereumTokenEvent {
    case created
    case deleted
}

extension EthereumTokenEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case .created: return "Created"
        case .deleted: return "Deleted"
        }
    }
}

///
///
///

class EthereumTransfer: Transfer {
    internal private(set) weak var listener: TransferListener?

    internal let identifier: BREthereumTransfer
    internal var core: BREthereumEWM {
        return _wallet._manager.core!
    }

    /// The EthereumWallet owning this transfer
    public unowned let _wallet: EthereumWallet

    public var wallet: Wallet {
        return _wallet
    }

    public var manager: WalletManager {
        return _wallet.manager
    }

    public var system: System {
        return _wallet.manager.system
    }

    internal let unit: Unit

    public private(set) lazy var source: Address? = {
        return Address (core: cryptoAddressCreateAsETH (ewmTransferGetSource (self.core, self.identifier)))
    }()

    public private(set) lazy var target: Address? = {
        return Address (core: cryptoAddressCreateAsETH (ewmTransferGetTarget (self.core, self.identifier)))
    }()

    public private(set) lazy var amount: Amount = {
        let amount: BREthereumAmount = ewmTransferGetAmount (self.core, self.identifier)
        let value = (AMOUNT_ETHER == amount.type
            ? amount.u.ether.valueInWEI
            : amount.u.tokenQuantity.valueAsInteger)

        return Amount (core: cryptoAmountCreate (unit.currency.core, CRYPTO_FALSE, value),
                       unit: unit)
    } ()

    public private(set) lazy var fee: Amount = {
        var overflow: Int32 = 0;
        let amount: BREthereumEther = ewmTransferGetFee (self.core, self.identifier, &overflow)
        precondition (0 == overflow)

        let unit = wallet.manager.network.baseUnitFor(currency: wallet.manager.currency)!
        return Amount.createAsETH (amount.valueInWEI, unit)
    }()

    var feeBasis: TransferFeeBasis

    public private(set) var hash: TransferHash? = nil

    public internal(set) var state: TransferState {
        didSet {
            let newValue = state
            listener?.handleTransferEvent (system: wallet.manager.system,
                                           manager: wallet.manager,
                                           wallet: wallet,
                                           transfer: self,
                                           event: TransferEvent.changed (old: oldValue,
                                                                         new: newValue))
        }
    }

    public let isSent: Bool

    internal init (listener: TransferListener?,
                   wallet: EthereumWallet,
                   unit: Unit,
                   tid: BREthereumTransfer) {
        self.listener = listener
        self._wallet = wallet
        self.identifier = tid
        self.unit = unit
        self.state = TransferState.created

        let gasUnit = wallet.manager.network.baseUnitFor(currency: wallet.manager.currency)!

        let gasPrice = Amount.createAsETH(createUInt256 (0), gasUnit)
        self.feeBasis = TransferFeeBasis.ethereum(gasPrice: gasPrice, gasLimit: 0)
        self.isSent = true

        self.listener?.handleTransferEvent (system: system,
                                            manager: manager,
                                            wallet: wallet,
                                            transfer: self,
                                            event: TransferEvent.created)
    }
}

///
///
///
class EthereumWallet: Wallet {
    internal private(set) weak var listener: WalletListener?

    internal let identifier: BREthereumWallet
    private var core: BREthereumEWM {
        return _manager.core!
    }

    public unowned let _manager: EthereumWalletManager

    public var manager: WalletManager {
        return _manager
    }

    public var system: System {
        return _manager.system
    }

    public let name: String

    public let unit: Unit

    public var balance: Amount {
        let coreETH = ewmWalletGetBalance (self.core, self.identifier)
        let value = (coreETH.type == AMOUNT_ETHER
            ? coreETH.u.ether.valueInWEI
            : coreETH.u.tokenQuantity.valueAsInteger)
        return Amount.createAsETH (value, unit)
    }

    internal func upd (balance: Amount) {
        self.listener?.handleWalletEvent (system: system,
                                          manager: manager,
                                          wallet: self,
                                          event: WalletEvent.balanceUpdated(amount: balance))
    }
    
    public private(set) var transfers: [Transfer] = []

    internal func add (transfer: Transfer) {
        transfers.append (transfer)
        self.listener?.handleWalletEvent (system: system,
                                          manager: manager,
                                          wallet: self,
                                          event: WalletEvent.transferAdded (transfer: transfer))
    }

    internal func rem (transfer: Transfer) {
        transfers.firstIndex { $0 === transfer }
            .map {
                transfers.remove(at: $0)
                self.listener?.handleWalletEvent (system: system,
                                                  manager: manager,
                                                  wallet: self,
                                                  event: WalletEvent.transferDeleted (transfer: transfer))
        }
    }

    internal func upd (transfer: Transfer) {
        if transfers.contains(where: { $0 === transfer }) {
            self.listener?.handleWalletEvent (system: system,
                                              manager: manager,
                                              wallet: self,
                                              event: WalletEvent.transferChanged(transfer: transfer))
        }
    }

    public func lookup (transfer: TransferHash) -> Transfer? {
        return nil
    }

    internal func lookupTransfer (core: BREthereumTransfer?) -> Transfer? {
        guard let core = core else { return nil }
        return transfers.filter { $0 is EthereumTransfer }
            .first { ($0 as! EthereumTransfer).identifier == core}
    }

    public internal(set) var state: WalletState {
        didSet {
            let newValue = state
            self.listener?.handleWalletEvent (system: system,
                                              manager: manager,
                                              wallet: self,
                                              event: WalletEvent.changed(oldState: oldValue, newState: newValue))
        }
    }

    public internal(set) var defaultFeeBasis: TransferFeeBasis {
        didSet {
            let newValue = defaultFeeBasis
            self.listener?.handleWalletEvent (system: system,
                                              manager: manager,
                                              wallet: self,
                                              event: WalletEvent.feeBasisUpdated (feeBasis: newValue))
        }
    }

    internal func updateDefaultFeeBasis () {
        let coreGasPrice = ewmWalletGetDefaultGasPrice (self.core, self.identifier)
        let coreGasLimit = ewmWalletGetDefaultGasLimit (self.core, self.identifier)
        defaultFeeBasis = TransferFeeBasis.ethereum (
            gasPrice: Amount.createAsETH (coreGasPrice.etherPerGas.valueInWEI, _manager.unit),
            gasLimit: coreGasLimit.amountOfGas)
    }

    //    public var transferFactory: TransferFactory

    public let target: Address
    public let source: Address

    internal init (listener: WalletListener?,
                   manager: EthereumWalletManager,
                   unit: Unit,
                   wid: BREthereumWallet) {
        self.listener = listener
        self._manager = manager
        self.identifier = wid
        self.name = unit.currency.code
        self.unit = unit

        self.state = WalletState.created

        self.target = Address.createAsETH (accountGetPrimaryAddress (manager.account.asETH))
        self.source = self.target

        let coreGasPrice = ewmWalletGetDefaultGasPrice (_manager.core, wid)
        let coreGasLimit = ewmWalletGetDefaultGasLimit (_manager.core, wid)
        self.defaultFeeBasis = TransferFeeBasis.ethereum (
            gasPrice: Amount.createAsETH (coreGasPrice.etherPerGas.valueInWEI, _manager.unit),
            gasLimit: coreGasLimit.amountOfGas)

        self.listener?.handleWalletEvent (system: system,
                                          manager: manager,
                                          wallet: self,
                                          event: WalletEvent.created)
    }
}

///
///
///
class EthereumWalletManager: WalletManager {
    internal private(set) weak var listener: WalletManagerListener?

    internal var core: BREthereumEWM! = nil

    public unowned let system: System

    public let account: Account
    public var network: Network

    internal lazy var unit: Unit = {
        return network.defaultUnitFor(currency: network.currency)!
    }()

    public lazy var primaryWallet: Wallet = {
        let wallet = EthereumWallet (listener: system.listener,
                                     manager: self,
                                     unit: unit,
                                     wid: ewmGetWallet(self.core))
        add (wallet: wallet)
        return wallet
    }()

    var wallets: [Wallet] = []
//    public lazy var wallets: [Wallet] = {
//        return [primaryWallet]
//    } ()

    internal func add (wallet: Wallet) {
        wallets.append (wallet)
        self.listener?.handleManagerEvent (system: system,
                                           manager: self,
                                           event: WalletManagerEvent.walletAdded(wallet: wallet))
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

    internal func lookupWallet (core: BREthereumWallet?) -> Wallet? {
        guard let core = core else { return nil }
        return wallets.filter { $0 is EthereumWallet }
            .first { ($0 as! EthereumWallet).identifier == core}
    }

    public func createWalletFor (currency: Currency) -> Wallet? {
        guard let unit = network.defaultUnitFor (currency: currency) else { return nil }

        let core = ewmGetWallet(self.core)! // holding token

        let wallet =  EthereumWallet (listener: system.listener,
                                      manager: self,
                                      unit: unit,
                                      wid: core)
        wallets.append(wallet)
        return wallet
    }

//    internal func addWallet (identifier: BREthereumWallet) {
//        guard case .none = findWallet(identifier: identifier) else { return }
//
//        if let tokenId = ewmWalletGetToken (core, identifier) {
//            guard let token = findToken (identifier: tokenId),
//                let unit = network.defaultUnitFor(currency: token.currency)
//                else { precondition(false); return }
//
//            wallets.append (EthereumWallet (manager: self,
//                                            unit: unit,
//                                            wid: identifier))
//        }
//    }
//
//    internal func findWallet (identifier: BREthereumWallet) -> EthereumWallet? {
//        return wallets.first { identifier == ($0 as! EthereumWallet).identifier } as? EthereumWallet
//    }

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

        self.core = ewmCreate (network.asETH,
                               account.asETH,
                               UInt64(account.timestamp),
                               EthereumWalletManager.coreMode (mode),
                               coreEthereumClient,
                               storagePath)

        listener.handleManagerEvent (system: system,
                                     manager: self,
                                     event: WalletManagerEvent.created)

        // system.add (manager: self)

//        EthereumWalletManager.managers.append(Weak (value: self))
//        self.listener.handleManagerEvent(manager: self, event: WalletManagerEvent.created)

    }

    public func connect() {
        ewmConnect (self.core)
    }

    public func disconnect() {
        ewmDisconnect (self.core)
    }

    public func sign (transfer: Transfer, paperKey: String) {
        guard let wallet = primaryWallet as? EthereumWallet,
            let transfer = transfer as? EthereumTransfer else { precondition(false); return }
        ewmWalletSignTransferWithPaperKey(core, wallet.identifier, transfer.identifier, paperKey)
    }

    public func submit (transfer: Transfer) {
        guard let wallet = primaryWallet as? EthereumWallet,
            let transfer = transfer as? EthereumTransfer else { precondition(false); return }
        ewmWalletSubmitTransfer(core, wallet.identifier, transfer.identifier)
    }

    public func sync() {
        ewmSync (core);
    }

    // Actually a Set/Dictionary by {Symbol}
    public private(set) var all: [EthereumToken] = []

    internal func addToken (identifier: BREthereumToken) {
        let symbol = asUTF8String (tokenGetSymbol (identifier))
        if let currency = network.currencyBy (code: symbol) {
            let token = EthereumToken (identifier: identifier, currency: currency)
            all.append (token)
//            self._listener.handleTokenEvent(manager: self, token: token, event: EthereumTokenEvent.created)
        }
    }

    internal func remToken (identifier: BREthereumToken) {
        if let index = all.firstIndex (where: { $0.identifier == identifier}) {
//            let token = all[index]
            all.remove(at: index)
//            self._listener.handleTokenEvent(manager: self, token: token, event: EthereumTokenEvent.deleted)
        }
    }

    internal func findToken (identifier: BREthereumToken) -> EthereumToken? {
        return all.first { $0.identifier == identifier }
    }

    private static func coreMode (_ mode: WalletManagerMode) -> BREthereumMode {
        switch mode {
        case .api_only: return BRD_ONLY
        case .api_with_p2p_submit: return BRD_WITH_P2P_SEND
        case .p2p_with_api_sync: return P2P_WITH_BRD_SYNC
        case .p2p_only: return P2P_ONLY
        }
    }

    private lazy var coreEthereumClient: BREthereumClient = {
        let this = self
        return BREthereumClient (
            context: Unmanaged<EthereumWalletManager>.passUnretained(this).toOpaque(),

            funcGetBalance: { (context, coreEWM, wid, address, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getBalanceAsETH (ewm: this.core,
                                            wid: wid!,
                                            address: address,
                                            rid: rid) { (wid, balance, rid) in
                                                ewmAnnounceWalletBalance (this.core, wid, balance, rid)
                }},

            funcGetGasPrice: { (context, coreEWM, wid, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                this.query.getGasPriceAsETH (ewm: this.core,
                                             wid: wid!,
                                             rid: rid) { (wid, gasPrice, rid) in
                                                ewmAnnounceGasPrice (this.core, wid, gasPrice, rid)
                }},

            funcEstimateGas: { (context, coreEWM, wid, tid, from, to, amount, data, rid)  in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                    let from = asUTF8String(from!)
                    let to = asUTF8String(to!)
                    let amount = asUTF8String(amount!)
                    let data = asUTF8String(data!)
                this.query.getGasEstimateAsETH (ewm: this.core,
                                                wid: wid!,
                                                tid: tid!,
                                                from: from,
                                                to: to,
                                                amount: amount,
                                                data: data,
                                                rid: rid) { (wid, tid, gasEstimate, rid) in
                                                    ewmAnnounceGasEstimate (this.core, wid, tid, gasEstimate, rid)

                }},

            funcSubmitTransaction: { (context, coreEWM, wid, tid, transaction, rid)  in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                    let transaction = asUTF8String (transaction!)
                this.query.submitTransactionAsETH (ewm: this.core,
                                                   wid: wid!,
                                                   tid: tid!,
                                                   transaction: transaction,
                                                   rid: rid) { (wid, tid, hash, errorCode, errorMessage, rid) in
                                                    ewmAnnounceSubmitTransfer (this.core,
                                                                               wid,
                                                                               tid,
                                                                               hash,
                                                                               errorCode,
                                                                               errorMessage,
                                                                               rid)
                }},

            funcGetTransactions: { (context, coreEWM, address, begBlockNumber, endBlockNumber, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getTransactionsAsETH (ewm: this.core,
                                                 address: address,
                                                 begBlockNumber: begBlockNumber,
                                                 endBlockNumber: endBlockNumber,
                                                 rid: rid,
                                                 done: { (success: Bool, rid: Int32) in
                                                    ewmAnnounceTransactionComplete (this.core,
                                                                                    rid,
                                                                                    (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                },
                                                 each: { (res: BlockChainDB.ETH.Transaction) in
                                                    ewmAnnounceTransaction (this.core,
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
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getLogsAsETH (ewm: this.core,
                                         address: address,
                                         begBlockNumber: begBlockNumber,
                                         endBlockNumber: endBlockNumber,
                                         rid: rid,
                                         done: { (success: Bool, rid: Int32) in
                                            ewmAnnounceLogComplete (this.core,
                                                                    rid,
                                                                    (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                },
                                         each: { (res: BlockChainDB.ETH.Log) in
                                            var cTopics = res.topics.map { UnsafePointer<Int8>(strdup($0)) }
                                            defer {
                                                cTopics.forEach { free (UnsafeMutablePointer(mutating: $0)) }
                                            }

                                            ewmAnnounceLog (this.core,
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
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getBlocksAsETH (ewm: this.core,
                                           address: address,
                                           interests: interests,
                                           blockStart: blockStart,
                                           blockStop: blockStop,
                                           rid: rid) { (blocks, rid) in
                                            ewmAnnounceBlocks (this.core, rid,
                                                               Int32(blocks.count),
                                                               UnsafeMutablePointer<UInt64>(mutating: blocks))
                }},

            funcGetTokens: { (context, coreEWM, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                this.query.getTokensAsETH (ewm: this.core,
                                           rid: rid,
                                           done: { (success: Bool, rid: Int32) in
                                            ewmAnnounceTokenComplete (this.core,
                                                                      rid,
                                                                      (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                },
                                           each: { (res: BlockChainDB.ETH.Token) in
                                            ewmAnnounceToken (this.core,
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
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                this.query.getBlockNumberAsETH (ewm: this.core,
                                                rid: rid) { (number, rid) in
                                                    ewmAnnounceBlockNumber (this.core, number, rid)
                }},

            funcGetNonce: { (context, coreEWM, address, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getNonceAsETH (ewm: this.core,
                                          address: address,
                                          rid: rid) { (address, nonce, rid) in
                                            ewmAnnounceNonce (this.core, address, nonce, rid)
                }},

            funcEWMEvent: { (context, coreEWM, event, status, message) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
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
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                switch event {
                case PEER_EVENT_CREATED:
                    break
                case PEER_EVENT_DELETED:
                    break
                default:
                    precondition (false)
                }},

            funcWalletEvent: { (context, coreEWM, wid, event, status, message) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()

                if event == WALLET_EVENT_CREATED, let wid = wid {
                    print ("Wallet Created")
                    let currency = ewmWalletGetToken (coreEWM, wid)
                        .flatMap { this.network.currencyBy (code: asUTF8String (tokenGetSymbol ($0))) }
                        ?? this.network.currency

                    let wallet = EthereumWallet (listener: this.system.listener,
                                                 manager: this,
                                                 unit: this.network.defaultUnitFor (currency: currency)!,
                                                 wid: wid)

                    this.add (wallet: wallet)
                }

                if let wallet = this.lookupWallet (core: wid) as? EthereumWallet {
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
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
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
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                if let wallet = this.lookupWallet (core: wid) as? EthereumWallet {

                    if TRANSFER_EVENT_CREATED == event, let tid = tid {
                        print ("Transfer Created")
                       let transfer = EthereumTransfer (listener: this.system.listener,
                                                         wallet: wallet,
                                                         unit: wallet.unit,
                                                         tid: tid)
                        wallet.add (transfer: transfer)
                    }

                    if let transfer = wallet.lookupTransfer (core: tid) as? EthereumTransfer {
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
}
