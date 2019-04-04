//
//  BRBitcoin.swift
//  BRCrypto
//
//  Created by Ed Gamble on 4/4/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import BRCryptoC
import BRCore

extension Amount {
    fileprivate var asBTC: UInt64 {
        var overflow: BRCryptoBoolean = CRYPTO_FALSE
        let value = cryptoAmountGetIntegerRaw (self.core, &overflow)
        precondition(CRYPTO_FALSE == overflow)
        return value
    }

    static fileprivate func createAsBTC (_ value: UInt64, _ unit: Unit) -> Amount {
        return Amount (core: cryptoAmountCreate(unit.currency.core, CRYPTO_FALSE, createUInt256(value)),
                       unit: unit);
    }
}

extension Address {
    static fileprivate func createAsBTC (_ btc: BRAddress) -> Address  {
        return Address (core: cryptoAddressCreateAsBTC (btc))
    }
}

extension Network {
    fileprivate var asBTC: UnsafePointer<BRChainParams>! {
        switch self.impl {
        case let .bitcoin(_, chainParams): return chainParams
        default: precondition(false); return nil
        }
    }

    fileprivate var forkId: BRWalletForkId? {
        switch self.impl {
        case let .bitcoin(forkId, _): return BRWalletForkId (UInt32(forkId))
        case let .bitcash(forkId, _): return BRWalletForkId (UInt32(forkId))
        case .ethereum: return nil
        case .generic:  return nil
        }
    }
}

extension Account {
    fileprivate var asBTC: BRMasterPubKey {
        return cryptoAccountAsBTC (self.core)
    }
}

fileprivate struct BitcoinLegacyAddressScheme: AddressScheme {
    public func getAddress(for wallet: BitcoinWallet) -> Address {
        return Address.createAsBTC (BRWalletLegacyAddress(wallet.core))
    }
}

fileprivate struct BitcoinSegwitAddressScheme: AddressScheme {
    public func getAddress(for wallet: BitcoinWallet) -> Address {
        return Address.createAsBTC (BRWalletReceiveAddress(wallet.core))
    }
}

typealias BRCoreWallet = OpaquePointer
typealias BRCorePeerManager = OpaquePointer
typealias BRCoreWalletManager = BRWalletManager
typealias BRCoreTransaction = UnsafeMutablePointer<BRTransaction>

class BitcoinTransfer: Transfer {
    internal private(set) weak var listener: TransferListener?

    internal let core: BRCoreTransaction

    public unowned let _wallet: BitcoinWallet

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
        let inputs = [BRTxInput](UnsafeBufferPointer(start: self.core.pointee.inputs, count: self.core.pointee.inCount))
        let inputsContain = (isSent ? 1 : 0)
        return inputs
            // If we sent the transaction then we expect our wallet to include one or more inputs.
            // But if we didn't send it, then the inputs will be the sender's inputs.
            .first { inputsContain == BRWalletContainsAddress(_wallet.core, UnsafeRawPointer([$0.address]).assumingMemoryBound(to: CChar.self)) }
            .map { Address.createAsBTC (BRAddress (s: $0.address))}
    }()

    /// The addresses for all TxInputs
    public var sources: [Address] {
        let inputs = [BRTxInput](UnsafeBufferPointer(start: self.core.pointee.inputs, count: self.core.pointee.inCount))
        return inputs.map { Address.createAsBTC (BRAddress (s: $0.address)) }
    }

    public private(set) lazy var target: Address? = {
        // The target address is in a TxOutput; if not sent is it out address, otherwise anothers
        let outputs = [BRTxOutput](UnsafeBufferPointer(start: self.core.pointee.outputs, count: self.core.pointee.outCount))
        let outputsContain = (!self.isSent ? 1 : 0)
        return outputs
            // If we did not send the transaction then we expect our wallet to include one or more
            // outputs.  But if we did send it, then the outpus witll be the targets outputs.
            .first { outputsContain == BRWalletContainsAddress(_wallet.core, UnsafeRawPointer([$0.address]).assumingMemoryBound(to: CChar.self)) }
            .map { Address.createAsBTC (BRAddress (s: $0.address)) }
    }()
    
    /// The addresses for all TxOutputs
    public var targets: [Address] {
        let outputs = [BRTxOutput](UnsafeBufferPointer(start: self.core.pointee.outputs, count: self.core.pointee.outCount))
        return outputs.map { Address.createAsBTC (BRAddress (s: $0.address)) }
    }

   public var amount: Amount {
        var fees = UInt64(BRWalletFeeForTx (_wallet.core, core))
        if (fees == UINT64_MAX) { fees = 0 }

        let recv = Int64(BRWalletAmountReceivedFromTx(_wallet.core, core))
        let send = Int64(BRWalletAmountSentByTx (_wallet.core, core))   // includes fees

        // The value is always positive; it is the value sent from source to target.
        let value = (0 == fees
            ? recv - send
            : (send - Int64(fees)) - recv)

        return Amount.createAsBTC (UInt64(value) , wallet.unit)
    }

    public var fee: Amount {
        //        var transaction = core
        let fee = BRWalletFeeForTx (_wallet.core, core)
        let unit = wallet.manager.network.baseUnitFor(currency: wallet.manager.network.currency)!
        return Amount.createAsBTC (fee, unit)
    }

    public var feeBasis: TransferFeeBasis {
        get {
            return TransferFeeBasis.bitcoin(feePerKB: BRWalletFeePerKb(_wallet.core))
        }
        set (basis) {
            if case let .bitcoin (feePerKB) = basis {
                BRWalletSetFeePerKb(_wallet.core, feePerKB)
            }
        }
    }

    public var hash: TransferHash? {
        return TransferHash.bitcoin (core.pointee.txHash)
    }

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

    /// Flag to determine if the wallet's owner sent this transfer
    public private(set) lazy var isSent: Bool = {
        // Returns a 'fee' if 'all inputs are from wallet' (meaning, the bitcoin transaction is
        // composed of UTXOs from wallet)
        let fee = BRWalletFeeForTx (_wallet.core, core)
        return fee != UINT64_MAX // && fee != 0
    }()


    internal init (listener: TransferListener?,
                   wallet: BitcoinWallet,
                   unit: Unit,
                   tid: BRCoreTransaction) {
        self.listener = listener
        self._wallet = wallet
        self.core = tid
        self.unit = unit
        self.state = TransferState.created

        self.isSent = true
        self.feeBasis = TransferFeeBasis.bitcoin(feePerKB: BRWalletFeePerKb(_wallet.core))

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
class BitcoinWallet: Wallet {
    internal private(set) weak var listener: WalletListener?

    internal let core: BRCoreWallet

    public unowned let _manager: BitcoinWalletManager

    public var manager: WalletManager {
        return _manager
    }

    public var system: System {
        return _manager.system
    }

    public let name: String

    public let unit: Unit

    public var balance: Amount {
        return Amount.createAsBTC (BRWalletBalance(core), unit)
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

    internal func lookupTransfer (core: BRCoreTransaction?) -> Transfer? {
        guard let core = core else { return nil }
        return transfers.filter { $0 is EthereumTransfer }
            .first { ($0 as! BitcoinTransfer).core == core}
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
        defaultFeeBasis = TransferFeeBasis.bitcoin(feePerKB: BRWalletFeePerKb (core))
    }

    //    public var transferFactory: TransferFactory

    public var target: Address {
        return BitcoinLegacyAddressScheme().getAddress(for: self)
    }

    public var source: Address {
        return BitcoinLegacyAddressScheme().getAddress(for: self)
    }


    internal init (listener: WalletListener?,
                   manager: BitcoinWalletManager,
                   unit: Unit,
                   wid: BRCoreWallet) {
        self.listener = listener
        self._manager = manager
        self.core = wid
        self.name = unit.currency.code
        self.unit = unit

        self.state = WalletState.created
        self.defaultFeeBasis = TransferFeeBasis.bitcoin(feePerKB: BRWalletFeePerKb (core))

        self.listener?.handleWalletEvent (system: system,
                                          manager: manager,
                                          wallet: self,
                                          event: WalletEvent.created)
    }
}

///
///
///
class BitcoinWalletManager: WalletManager {
    internal private(set) weak var listener: WalletManagerListener?

    internal var core: BRCoreWalletManager! = nil

    /// The Core's BRCorePeerManager
    internal var corePeerManager: BRCorePeerManager {
        return BRWalletManagerGetPeerManager (core)
    }

    /// The Core's BRCoreWallet
    internal var coreWallet: BRCoreWallet {
        return BRWalletManagerGetWallet (core)
    }

    public unowned let system: System

    public let account: Account
    public var network: Network

    internal lazy var unit: Unit = {
        return network.defaultUnitFor(currency: network.currency)!
    }()

    public lazy var primaryWallet: Wallet = {
        let wallet = BitcoinWallet (listener: system.listener,
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

        let wallet =  BitcoinWallet (listener: system.listener,
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

        self.core = BRWalletManagerNew (coreBitcoinClient,
                                        account.asBTC,
                                        network.asBTC,
                                        UInt32 (account.timestamp),
                                        storagePath)

        listener.handleManagerEvent (system: system,
                                     manager: self,
                                     event: WalletManagerEvent.created)

        // system.add (manager: self)

        //        EthereumWalletManager.managers.append(Weak (value: self))
        //        self.listener.handleManagerEvent(manager: self, event: WalletManagerEvent.created)

    }

    public func connect() {
        BRWalletManagerConnect (self.core)
    }

    public func disconnect() {
        BRWalletManagerDisconnect (self.core)
    }

    public func sign(transfer: Transfer, paperKey: String) {
        guard let transfer = transfer as? BitcoinTransfer else { precondition(false); return }
        var seed = Account.deriveSeed (phrase: paperKey)
        BRWalletSignTransaction(coreWallet, transfer.core, &seed, MemoryLayout<UInt512>.size)
    }

    public func submit(transfer: Transfer) {
        guard let transfer = transfer as? BitcoinTransfer else { precondition(false); return }
        let  closure = CLangClosure { (error: Int32) in
            let event = TransferEvent.changed (
                old: transfer.state,
                new: (0 == error
                    ? TransferState.submitted
                    : TransferState.failed(reason: asUTF8String(strerror(error)))))
            transfer.listener?.handleTransferEvent (system: self.system,
                                                    manager: self,
                                                    wallet: transfer.wallet,
                                                    transfer: transfer,
                                                    event: event)
        }
        BRPeerManagerPublishTx (corePeerManager, transfer.core,
                                Unmanaged<CLangClosure<(Int32)>>.passRetained(closure).toOpaque(),
                                { (info, error) in
                                    guard let info = info else { return }
                                    let closure = Unmanaged<CLangClosure<(Int32)>>.fromOpaque(info).takeRetainedValue()
                                    closure.function (error)
        })
    }

    public func sync () {
        BRPeerManagerRescan (corePeerManager)
    }


//    private static func coreMode (_ mode: WalletManagerMode) -> BREthereumMode {
//        switch mode {
//        case .api_only: return BRD_ONLY
//        case .api_with_p2p_submit: return BRD_WITH_P2P_SEND
//        case .p2p_with_api_sync: return P2P_WITH_BRD_SYNC
//        case .p2p_only: return P2P_ONLY
//        }
//    }

    private lazy var coreBitcoinClient: BRWalletManagerClient = {
        let this = self
        return BRWalletManagerClient (
            context: Unmanaged<BitcoinWalletManager>.passUnretained(this).toOpaque(),
            funcTransactionEvent: { (context, bid, wid, tid, event) in
                let this = Unmanaged<BitcoinWalletManager>.fromOpaque(context!).takeUnretainedValue()
                if let wallet = this.lookupWallet (core: wid) as? BitcoinWallet {

                    if event.type == BITCOIN_TRANSACTION_ADDED, let tid = tid {
                        print ("Bitcoin: Transfer Created")
                        let transfer = BitcoinTransfer (listener: this.system.listener,
                                                        wallet: wallet,
                                                        unit: wallet.unit,
                                                        tid: tid)
                    }

                    if let transfer = wallet.lookupTransfer (core: tid) as? BitcoinTransfer {
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
                let this = Unmanaged<BitcoinWalletManager>.fromOpaque(context!).takeUnretainedValue()

                if event.type == BITCOIN_WALLET_CREATED, let wid = wid {
                    let wallet = BitcoinWallet (listener: this.system.listener,
                                                manager: this,
                                                unit: this.network.defaultUnitFor(currency: this.network.currency)!,
                                                wid: wid)
                    this.add (wallet: wallet)
                }

                if let wallet = this.lookupWallet (core: wid) as? BitcoinWallet {
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
                let this = Unmanaged<BitcoinWalletManager>.fromOpaque(context!).takeUnretainedValue()
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
/*
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
 */
    }()
}

