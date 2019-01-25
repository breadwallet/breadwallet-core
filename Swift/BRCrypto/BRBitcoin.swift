//
//  BRBitcoin.swift
//  BRCoreX
//
//  Created by Ed Gamble on 11/7/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import Foundation // DispatchQueue
import Core

public struct Bitcoin {
    public static let currency = Currency (code: "BTC", symbol:  "₿", name: "Bitcoin", decimals: 8,
                                           baseUnit: (name: "SAT", symbol: "sat"))
    public struct Units {
        public static let SATOSHI = Bitcoin.currency.baseUnit!
        public static let BITCOIN = Bitcoin.currency.defaultUnit!
    }

    public struct Networks {
        public static let mainnet = Network.bitcoin (name: "BTC Mainnet", forkId: 0x00, chainParams: bitcoinParams (1))
        public static let testnet = Network.bitcoin (name: "BTC Testnet", forkId: 0x40, chainParams: bitcoinParams (0))
    }

    public struct AddressSchemes {
        public static let segwit = BitcoinSegwitAddressScheme()
        public static let legacy = BitcoinLegacyAddressScheme()

        public static let `default` = segwit
    }
}

public struct BitcoinLegacyAddressScheme: AddressScheme {
    public func getAddress(for wallet: BitcoinWallet) -> Address {
        return Address.bitcoin (BRWalletLegacyAddress(wallet.core))
    }
}

public struct BitcoinSegwitAddressScheme: AddressScheme {
    public func getAddress(for wallet: BitcoinWallet) -> Address {
        return Address.bitcoin (BRWalletReceiveAddress(wallet.core))
    }
}

extension Network {
    var bitcoinChainParams: UnsafePointer<BRChainParams>? {
        switch self {
        case let .bitcoin (_, _, params): return params
        case let .bitcash (_, _, params): return params
        case .ethereum: return nil
        }
    }

    var forkId: BRWalletForkId? {
        switch self {
        case let .bitcoin(_, forkId, _): return BRWalletForkId (UInt32(forkId))
        case let .bitcash(_, forkId, _): return BRWalletForkId (UInt32(forkId))
        case .ethereum: return nil
        }
    }
}

typealias BRCoreWallet = OpaquePointer
typealias BRCorePeerManager = OpaquePointer
typealias BRCoreWalletManager = BRWalletManager
typealias BRCoreTransaction = UnsafeMutablePointer<BRTransaction>

public protocol BitcoinListener: WalletManagerListener {}
public protocol BitcoinBackendClient: WalletManagerBackendClient {}
public protocol BitcoinPersistenceClient: WalletManagerPersistenceClient {}

public class BitcoinTransfer: Transfer {
    internal let core: UnsafeMutablePointer<BRTransaction>
    
    public unowned let _wallet: BitcoinWallet
    
    public var wallet: Wallet {
        return _wallet
    }

    /// Flag to determine if the wallet's owner sent this transfer
    public private(set) lazy var isSent: Bool = {
          // Returns a 'fee' if 'all inputs are from wallet' (meaning, the bitcoin transaction is
        // composed of UTXOs from wallet)
        let fee = BRWalletFeeForTx (_wallet.core, core)
        return fee != UINT64_MAX // && fee != 0
    }()

    public private(set) lazy var source: Address? = {
        let inputs = [BRTxInput](UnsafeBufferPointer(start: self.core.pointee.inputs, count: self.core.pointee.inCount))
        let inputsContain = (isSent ? 1 : 0)
        return inputs
            // If we sent the transaction then we expect our wallet to include one or more inputs.
            // But if we didn't send it, then the inputs will be the sender's inputs.
            .first { inputsContain == BRWalletContainsAddress(_wallet.core, UnsafeRawPointer([$0.address]).assumingMemoryBound(to: CChar.self)) }
            .map { Address.bitcoin (BRAddress (s: $0.address))}
    }()

    /// The addresses for all TxInputs
    public var sources: [Address] {
        let inputs = [BRTxInput](UnsafeBufferPointer(start: self.core.pointee.inputs, count: self.core.pointee.inCount))
        return inputs.map { Address.bitcoin (BRAddress (s: $0.address)) }
    }
    
    public private(set) lazy var target: Address? = {
        // The target address is in a TxOutput; if not sent is it out address, otherwise anothers
        let outputs = [BRTxOutput](UnsafeBufferPointer(start: self.core.pointee.outputs, count: self.core.pointee.outCount))
        let outputsContain = (!self.isSent ? 1 : 0)
        return outputs
            // If we did not send the transaction then we expect our wallet to include one or more
            // outputs.  But if we did send it, then the outpus witll be the targets outputs.
            .first { outputsContain == BRWalletContainsAddress(_wallet.core, UnsafeRawPointer([$0.address]).assumingMemoryBound(to: CChar.self)) }
            .map { Address.bitcoin (BRAddress (s: $0.address)) }
    }()

    /// The addresses for all TxOutputs
    public var targets: [Address] {
        let outputs = [BRTxOutput](UnsafeBufferPointer(start: self.core.pointee.outputs, count: self.core.pointee.outCount))
        return outputs.map { Address.bitcoin (BRAddress (s: $0.address)) }
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

        return Amount (value: value, unit: Bitcoin.Units.SATOSHI)
    }
    
    public var fee: Amount {
//        var transaction = core
        let fee = BRWalletFeeForTx (_wallet.core, core)
        return Amount (value: (fee == UINT64_MAX ? 0 : fee),
                       unit: Bitcoin.Units.SATOSHI)
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
    
    public var state: TransferState
    
    init (wallet: BitcoinWallet,
          core: UnsafeMutablePointer<BRTransaction>) {
        self._wallet = wallet
        self.core = core
        
        // TODO: Should be: included w/ TransferConfirmation
        self.state = TransferState.submitted
    }
    
    init? (wallet: BitcoinWallet,
           target: Address,
           amount: Amount,
           feeBasis: TransferFeeBasis) {
        
        self._wallet = wallet
        
        guard let transaction = BRWalletCreateTransaction (_wallet.core,
                                                           amount.value.u64.0,
                                                           target.description)
            else { return nil}
        
        self.core = transaction
        self.state = TransferState.created
        self.feeBasis = feeBasis
        
    }
}

public class BitcoinTransferFactory: TransferFactory {
    public func createTransfer(wallet: Wallet,
                               target: Address,
                               amount: Amount,
                               feeBasis: TransferFeeBasis) -> Transfer? {
        return BitcoinTransfer (wallet: wallet as! BitcoinWallet,
                                target: target,
                                amount: amount,
                                feeBasis: feeBasis)
    }
}



public class BitcoinWallet: Wallet {
    internal let core: BRCoreWallet
    
    public unowned let _manager: BitcoinWalletManager
    
    public var manager: WalletManager {
        return _manager
    }
    
    public var balance: Amount {
        return Amount (value: BRWalletBalance (core),
                       unit: currency.baseUnit)
        
    }
    
    public var transfers: [Transfer] = []
    
    public func lookup (transfer: TransferHash) -> Transfer? {
        return transfers
            .first  { $0.hash.map { $0 == transfer } ?? false }
    }

    public func lookup (coreTransaction: UnsafeMutablePointer<BRTransaction>) -> Transfer? {
        return transfers
            .first { ($0 as? BitcoinTransfer).map { $0.core == coreTransaction } ?? false }
    }

    public var state: WalletState
    
    public var defaultFeeBasis: TransferFeeBasis {
        get {
            return TransferFeeBasis.bitcoin(feePerKB: BRWalletFeePerKb(core))
        }
        set (basis) {
            if case let .bitcoin (feePerKB) = basis {
                BRWalletSetFeePerKb(core, feePerKB)
            }
        }
    }
    
    public var transferFactory: TransferFactory = BitcoinTransferFactory()
    
    internal let currency: Currency
    
    public var target: Address {
        return Bitcoin.AddressSchemes.`default`.getAddress(for: self)
    }

    internal init (manager: BitcoinWalletManager,
                   currency: Currency,
                   core:   BRCoreWallet) {
        self._manager = manager
        self.currency = currency
        self.core = core
        
        //        self.currency = Currency.ethereum
        self.state = WalletState.created
    }
}

///
/// A BitcoinWalletManager implements WalletManager using the Core's BRCoreWallet and
/// BRCorePeerManager.
///
public class BitcoinWalletManager: WalletManager {
    #if true
    internal let coreWalletManager: BRCoreWalletManager
    #endif
    
    /// The Core's BRCorePeerManager
    internal let corePeerManager: BRCorePeerManager

    /// The Core's BRCoreWallet
    internal let coreWallet: BRCoreWallet

    /// The bitcoin-specific backend client.
    internal let backendClient: BitcoinBackendClient

    /// The bitcoin-specific persistence client
    internal let persistenceClient: BitcoinPersistenceClient

    /// The bitcon-specific listener
    public let _listener : BitcoinListener

    public var listener: WalletManagerListener {
        return _listener
    }

    public let account: Account
    
    public var network: Network
    
    public private(set) lazy var primaryWallet: Wallet = {
        let wallet = BitcoinWallet (manager: self, currency: network.currency, core: coreWallet)
        self.listener.handleWalletEvent(manager: self, wallet: wallet, event: WalletEvent.created)
        self.listener.handleManagerEvent(manager: self, event: WalletManagerEvent.walletAdded(wallet: wallet))
        return wallet
    }()

    public lazy var wallets: [Wallet] = {
        return [primaryWallet]
    } ()
    
    public var mode: WalletManagerMode
    
    public var path: String
    
    public var state: WalletManagerState
    
    #if false
    public var walletFactory: WalletFactory = EthereumWalletFactory()
    #endif

    lazy var queue : DispatchQueue = {
        return DispatchQueue (label: "\(network.currency.code) WalletManager")
    }()

    internal static var managers: [BitcoinWalletManager] = [] // Weak<>
    
    internal static func lookup (manager: OpaquePointer?) -> BitcoinWalletManager? {
        return nil == manager ? nil :  managers.first { manager! == $0.coreWallet }
    }
    
    internal static func lookup (ptr: UnsafeMutableRawPointer?) -> BitcoinWalletManager? {
        return ptr.map { Unmanaged<BitcoinWalletManager>.fromOpaque($0).takeUnretainedValue() }
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
            self.listener.handleTransferEvent (manager: self,
                                               wallet: self.primaryWallet,
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

    public init (listener: BitcoinListener,
                 account: Account,
                 network: Network,
                 mode: WalletManagerMode,
                 timestamp: UInt64,
                 storagePath: String,
                 persistenceClient: BitcoinPersistenceClient = DefaultBitcoinPersistenceClient(),
                 backendClient: BitcoinBackendClient = DefaultBitcoinBackendClient()) {
        let params: UnsafePointer<BRChainParams>! = network.bitcoinChainParams
        precondition (nil != params)

        let forkId: BRWalletForkId! = network.forkId
        precondition (nil != forkId);

        self.backendClient = backendClient
        self.persistenceClient = persistenceClient
        
        self._listener = listener
        self.network = network
        self.account = account
        self.path = storagePath
        self.mode = mode
        self.state = WalletManagerState.created

#if true
        let client = BRWalletManagerClient (
            funcTransactionEvent: { (this, coreWallet, coreTransaction, event) in
                if let bwm  = BitcoinWalletManager.lookup (manager: this),
                    let coreTransaction = coreTransaction {
                    bwm.queue.async {
                        if let wallet = bwm.primaryWallet as? BitcoinWallet {
                            var transfer: Transfer!
                            var walletEvent: WalletEvent?
                            var transferEvent: TransferEvent?

                            switch event.type {
                            case BITCOIN_TRANSACTION_ADDED:
                                transfer = BitcoinTransfer (wallet: wallet,
                                                            core: coreTransaction)
                                wallet.transfers.append(transfer)
                                transferEvent = TransferEvent.created
                                walletEvent   = WalletEvent.transferAdded(transfer: transfer)

                            case BITCOIN_TRANSACTION_UPDATED:
                                transfer = wallet.lookup (coreTransaction: coreTransaction)
                                if nil != transfer {
                                    let oldState = transfer.state
                                    let newState = TransferState.submitted

                                    transferEvent = TransferEvent.changed(old: oldState, new: newState)
                                    walletEvent   = WalletEvent.transferChanged(transfer: transfer)
                                }

                            case BITCOIN_TRANSACTION_DELETED:
                                transfer = wallet.lookup (coreTransaction: coreTransaction)
                                if nil != transfer {
                                    if let index = wallet.transfers.firstIndex (where: {
                                        ($0 as? BitcoinTransfer).map { $0.core == coreTransaction } ?? false }) {
                                        wallet.transfers.remove(at: index)
                                    }

                                    transferEvent = TransferEvent.deleted
                                    walletEvent   = WalletEvent.transferDeleted(transfer: transfer)
                                }

                            default:
                                return
                            }

                            if let transfer = transfer {
                                transferEvent.map {
                                    bwm.listener.handleTransferEvent (manager: bwm,
                                                                      wallet: wallet,
                                                                      transfer: transfer,
                                                                      event: $0)
                                }

                                walletEvent.map {
                                    bwm.listener.handleWalletEvent (manager: bwm,
                                                                    wallet: wallet,
                                                                    event: $0)
                                }
                            }
                        }
                    }
                }},

            funcWalletEvent: { (this, coreWallet, event) in
                if let bwm = BitcoinWalletManager.lookup(manager: this) {
                    bwm.queue.async {
                        if let wallet = bwm.primaryWallet as? BitcoinWallet,
                            wallet.core == coreWallet {
                            switch event.type {
                            case BITCOIN_WALLET_CREATED:
                                break

                            case BITCOIN_WALLET_BALANCE_UPDATED:
                                let amount = Amount (value: event.u.balance.satoshi, unit: Bitcoin.Units.SATOSHI)
                                bwm.listener.handleWalletEvent (manager: bwm,
                                                                wallet: wallet,
                                                                event: WalletEvent.balanceUpdated(amount: amount))

                            case BITCOIN_WALLET_DELETED:
                                bwm.listener.handleWalletEvent (manager: bwm,
                                                                wallet: wallet,
                                                                event: WalletEvent.deleted)

                            default:
                                return
                            }
                        }
                    }
                }},

        funcWalletManagerEvent: { (this, event) in
                if let bwm = BitcoinWalletManager.lookup(manager: this) {
                    bwm.queue.async {
                        let oldState = bwm.state
                        var otherEvent : WalletManagerEvent? = nil

                        switch event.type {
                        case BITCOIN_WALLET_MANAGER_CONNECTED:
                            bwm.state = WalletManagerState.connected

                        case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                            bwm.state = WalletManagerState.disconnected

                       case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                        bwm.state = WalletManagerState.syncing
                        otherEvent = WalletManagerEvent.syncStarted

                        case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                            bwm.state = WalletManagerState.connected
                            otherEvent = WalletManagerEvent.syncEnded(error: nil)

                        default:
                            return
                        }

                        bwm.listener.handleManagerEvent (manager: bwm,
                                                         event: WalletManagerEvent.changed (oldState: oldState,
                                                                                            newState: bwm.state))

                        otherEvent.map {
                            bwm.listener.handleManagerEvent(manager: bwm, event: $0)
                        }
                    }
                }})

        self.coreWalletManager = BRWalletManagerNew (client,
                                                    account.masterPublicKey,
                                                    params,
                                                    UInt32 (timestamp),
                                                    path)

        self.corePeerManager = BRWalletManagerGetPeerManager (self.coreWalletManager)
        self.coreWallet = BRWalletManagerGetWallet(self.coreWalletManager)
#else

        self.coreWallet      = BRWalletNew (nil, 0, account.masterPublicKey, Int32(forkId.rawValue))
        self.corePeerManager = BRPeerManagerNew (params, coreWallet, UInt32(timestamp),
                                                 nil, 0,
                                                 nil, 0)
        
        BRWalletSetCallbacks (
            coreWallet,
            Unmanaged.passUnretained(self).toOpaque(),
            
            // balanceChanged
            { (this, balance) in
                if let bwm  = BitcoinWalletManager.lookup (ptr: this) {
                    bwm.queue.async {
                        if let wallet = bwm.primaryWallet as? BitcoinWallet {
                            let amount = Amount (value: balance, unit: Bitcoin.Units.SATOSHI)
                            bwm.listener.handleWalletEvent (manager: bwm,
                                                            wallet: wallet,
                                                            event: WalletEvent.balanceUpdated(amount: amount))
                        }
                    }
                }},
            
            // txAdded
            { (this, tx) in
                if let bwm  = BitcoinWalletManager.lookup (ptr: this), let tx = tx {
                    bwm.queue.async {
                        if let wallet = bwm.primaryWallet as? BitcoinWallet {
                            let transfer = BitcoinTransfer (wallet: wallet,
                                                            core: tx)
                            wallet.transfers.append(transfer)

                            bwm.listener.handleTransferEvent (manager: bwm,
                                                              wallet: bwm.primaryWallet,
                                                              transfer: transfer,
                                                              event: TransferEvent.created)

                            bwm.listener.handleWalletEvent (manager: bwm,
                                                            wallet: bwm.primaryWallet,
                                                            event: WalletEvent.transferAdded(transfer: transfer))
                        }
                    }
                }},

            // txUpdated
            { (this, txHashes, txCount, blockHeight, timestamp) in
                if let bwm  = BitcoinWalletManager.lookup (ptr: this) {
                    // Copy `txHashes`; they'll disappear.
                    let coreHashes = [UInt256](UnsafeBufferPointer(start: txHashes, count: txCount)).map { $0 }
                    bwm.queue.async {
                        if let wallet = bwm.primaryWallet as? BitcoinWallet {
                            for coreHash in coreHashes {
                                if let transfer = wallet.lookup(transfer: TransferHash.bitcoin(coreHash)) as? BitcoinTransfer {
                                    let confirmation =  TransferConfirmation (
                                        blockNumber: UInt64(blockHeight),
                                        transactionIndex: 0,
                                        timestamp: UInt64(timestamp),
                                        fee: transfer.fee)

                                    let oldState = transfer.state
                                    let newState = TransferState.included (confirmation: confirmation)

                                    transfer.state = newState

                                    bwm.listener.handleTransferEvent(manager: bwm,
                                                                     wallet: wallet,
                                                                     transfer: transfer,
                                                                     event: TransferEvent.changed(old: oldState, new: newState))

                                    bwm.listener.handleWalletEvent(manager: bwm,
                                                                   wallet: wallet,
                                                                   event: WalletEvent.transferChanged(transfer: transfer))
                                }
                            }
                        }
                    }
                }},
            
            // txDeleted
            { (this, txHash, notify, rescan) in
                if let bwm  = BitcoinWalletManager.lookup (ptr: this) {
                    bwm.queue.async {
                        if let wallet = bwm.primaryWallet as? BitcoinWallet {
                            let hash = TransferHash.bitcoin (txHash)
                            if let transfer = wallet.lookup(transfer: hash),
                                let index = wallet.transfers.firstIndex (where: { $0.hash == hash }) {
                                wallet.transfers.remove(at: index)

                                bwm.listener.handleTransferEvent(manager: bwm,
                                                                 wallet: wallet,
                                                                 transfer: transfer,
                                                                 event: TransferEvent.deleted)

                                bwm.listener.handleWalletEvent(manager: bwm,
                                                               wallet: wallet,
                                                               event: WalletEvent.transferDeleted(transfer: transfer))
                            }
                        }
                    }
                }}
        )
        
        BRPeerManagerSetCallbacks (
            corePeerManager,
            Unmanaged.passUnretained(self).toOpaque(),
            
            { (this) in // syncStarted
                if let bwm  = BitcoinWalletManager.lookup (ptr: this) {
                    bwm.listener.handleManagerEvent (manager: bwm,
                                                     event: WalletManagerEvent.syncStarted)
                }},
            
            { (this, error) in // syncStopped
                if let bwm  = BitcoinWalletManager.lookup (ptr: this) {
                    let reason = 0 == error ? nil : asUTF8String(strerror(error))
                    bwm.listener.handleManagerEvent (manager: bwm,
                                                     event: WalletManagerEvent.syncEnded(error: reason))
                }},
            
            { (this) in // txStatusUpdate
                if let _  = BitcoinWalletManager.lookup (ptr: this) {
                }},
            
            { (this, replace, blocks, blocksCount) in // saveBlocks
                if let bwm  = BitcoinWalletManager.lookup (ptr: this) {
                    // let blockRefs = [BRBlockRef?](UnsafeBufferPointer(start: blocks, count: blocksCount))
                    // replace != 0
                    let blocksDictionary: [String:String] = [:]
                    bwm.persistenceClient.saveBlocks (manager: bwm, data: blocksDictionary)
                }},
            
            
            { (this, replace, peers, peersCount) in // savePeers
                if let bwm  = BitcoinWalletManager.lookup (ptr: this) {
                    // let peerList = [BRPeer](UnsafeBufferPointer(start: peers, count: peersCount))
                    // replace != 0
                    let peersDictionary: [String:String] = [:]
                    bwm.persistenceClient.savePeers (manager: bwm, data: peersDictionary)
                }},
            
            { (this) -> Int32 in  // networkIsReachable
                if let bwm  = BitcoinWalletManager.lookup (ptr: this) {
                    return bwm.backendClient.networkIsReachable() ? 1 : 0
                }
                else {
                    return 0
                }},

            { (this) in // threadCleanup
                if let _  = BitcoinWalletManager.lookup (ptr: this) {
                }})
        #endif

        BitcoinWalletManager.managers.append(self)
        self.listener.handleManagerEvent(manager: self, event: WalletManagerEvent.created)
        let _ = self.primaryWallet
    }
    
    public func connect() {
        #if true
        BRWalletManagerConnect(self.coreWalletManager)
        #endif
        BRPeerManagerConnect (self.corePeerManager)
    }
    
    public func disconnect() {
        #if true
        BRWalletManagerDisconnect(self.coreWalletManager)
        #endif
        BRPeerManagerDisconnect (self.corePeerManager)
    }
    
    //hack to keep the swift compiler happy
    let a = BRBCashCheckpoints
    let b = BRBCashDNSSeeds
    let c = BRBCashVerifyDifficulty
    let d = BRBCashTestNetCheckpoints
    let e = BRBCashTestNetDNSSeeds
    let f = BRBCashTestNetVerifyDifficulty
    let g = BRMainNetDNSSeeds
    let h = BRMainNetCheckpoints
    let i = BRMainNetVerifyDifficulty
    let j = BRTestNetDNSSeeds
    let k = BRTestNetCheckpoints
    let l = BRTestNetVerifyDifficulty
}

