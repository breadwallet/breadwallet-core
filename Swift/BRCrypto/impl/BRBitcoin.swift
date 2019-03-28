//
//  BRBitcoin.swift
//  BRCrypto
//
//  Created by Ed Gamble on 11/7/18.
//  Copyright © 2018 breadwallet. All rights reserved.
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

fileprivate struct BitcoinLegacyAddressScheme: AddressScheme {
      public func getAddress(for wallet: Wallet) -> Address {
        guard let w = wallet as? BitcoinWallet else { precondition(false) }
        return Address.createAsBTC (BRWalletLegacyAddress(w.core))
    }
}

fileprivate struct BitcoinSegwitAddressScheme: AddressScheme {
    public func getAddress(for wallet: Wallet) -> Address {
        guard let w = wallet as? BitcoinWallet else { precondition(false) }
        return Address.createAsBTC (BRWalletReceiveAddress(w.core))
    }
}

extension Network {
    fileprivate var bitcoinChainParams: UnsafePointer<BRChainParams>? {
        switch self.impl {
        case let .bitcoin (_, params): return params
        case let .bitcash (_, params): return params
        case .ethereum: return nil
        case .generic:  return nil
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
    
    public var state: TransferState
    
    init (wallet: BitcoinWallet,
          core: UnsafeMutablePointer<BRTransaction>) {
        self._wallet = wallet
        self.core = core
        
        self.state = TransferState.created
    }
    
    init? (wallet: BitcoinWallet,
           target: Address,
           amount: Amount,
           feeBasis: TransferFeeBasis) {
        
        self._wallet = wallet
        
        guard let transaction = BRWalletCreateTransaction (_wallet.core,
                                                           amount.asBTC,
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

    public let unit: Unit

    public var balance: Amount {
        return Amount.createAsBTC(BRWalletBalance(core), unit)
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
        return BitcoinLegacyAddressScheme().getAddress(for: self)
    }

    public var source: Address {
        return BitcoinLegacyAddressScheme().getAddress(for: self)
    }
    
    internal init (manager: BitcoinWalletManager,
                   unit: Unit,
                   core: BRCoreWallet) {
        self._manager = manager
        self.unit = unit
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
    internal let coreWalletManager: BRCoreWalletManager

    /// The Core's BRCorePeerManager
    internal var corePeerManager: BRCorePeerManager {
        return BRWalletManagerGetPeerManager (coreWalletManager)
    }

    /// The Core's BRCoreWallet
    internal var coreWallet: BRCoreWallet {
        return BRWalletManagerGetWallet (coreWalletManager)
    }

    /// The bitcoin-specific backend client.
//    internal let backendClient: BitcoinBackendClient

    /// The bitcoin-specific persistence client
//    internal let persistenceClient: BitcoinPersistenceClient

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
    
//    public var walletFactory: WalletFactory = EthereumWalletFactory()

//    lazy var queue : DispatchQueue = {
//        return DispatchQueue (label: "\(network.currency.code) WalletManager")
//    }()

    public func createWalletFor (currency: Currency) -> Wallet? {
        guard let unit = network.defaultUnitFor (currency: currency) else { return nil }

        let wallet = BitcoinWallet (manager: self,
                                    unit: unit,
                                    core: self.coreWallet)

        wallets.append(wallet)
        return wallet
    }

//    internal static var managers: [BitcoinWalletManager] = [] // Weak<>
//
//    #if true
//    internal static func lookup (manager: OpaquePointer?) -> BitcoinWalletManager? {
//        return nil == manager ? nil :  managers.first { manager! == $0.coreWalletManager }
//    }
//    #else
//    internal static func lookup (ptr: UnsafeMutableRawPointer?) -> BitcoinWalletManager? {
//        return ptr.map { Unmanaged<BitcoinWalletManager>.fromOpaque($0).takeUnretainedValue() }
//    }
//    #endif

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

    public func sync () {
        BRPeerManagerRescan (corePeerManager)
    }

    public init (//listener: BitcoinListener,
                 account: Account,
                 network: Network,
                 mode: WalletManagerMode,
                 storagePath: String
//                 persistenceClient: BitcoinPersistenceClient = DefaultBitcoinPersistenceClient(),
//                 backendClient: BitcoinBackendClient = DefaultBitcoinBackendClient()
        ) {
        let params: UnsafePointer<BRChainParams>! = network.bitcoinChainParams
        precondition (nil != params)

        let forkId: BRWalletForkId! = network.forkId
        precondition (nil != forkId);

//        self.backendClient = backendClient
//        self.persistenceClient = persistenceClient
        
        self._listener = listener
        self.network = network
        self.account = account
        self.path = storagePath
        self.mode = mode
        self.state = WalletManagerState.created

        let client = BRWalletManagerClient (
            
            funcTransactionEvent: { (this, coreWallet, coreTransaction, event) in // this => BRWalletManager?
                if let bwm  = BitcoinWalletManager.lookup (manager: this),
                    let coreTransaction = coreTransaction {
                    bwm.queue.async {
                        if let wallet = bwm.primaryWallet as? BitcoinWallet {
                            var transfer: BitcoinTransfer!
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
                                transfer = wallet.lookup (coreTransaction: coreTransaction) as? BitcoinTransfer
                                if nil != transfer {
                                    let oldState = transfer.state
                                    let newState = TransferState.included (confirmation:
                                        TransferConfirmation (blockNumber: 500000,
                                                              transactionIndex: 0,
                                                              timestamp: 1543190400,
                                                              fee: Amount (value: 0, unit: Bitcoin.Units.SATOSHI)))

                                    transfer.state = newState

                                    transferEvent = TransferEvent.changed(old: oldState, new: newState)
                                    walletEvent   = WalletEvent.transferChanged(transfer: transfer)
                                }

                            case BITCOIN_TRANSACTION_DELETED:
                                transfer = wallet.lookup (coreTransaction: coreTransaction) as? BitcoinTransfer
                                if nil != transfer {
                                    if let index = wallet.transfers.firstIndex (where: {
                                        ($0 as? BitcoinTransfer).map { $0.core == coreTransaction } ?? false }) {
                                        wallet.transfers.remove(at: index)
                                    }

                                    transfer.state = TransferState.deleted

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
                                                    UInt32 (account.timestamp),
                                                    path)

        self.listener.handleManagerEvent(manager: self, event: WalletManagerEvent.created)
        let _ = self.primaryWallet
    }
    
    public func connect() {
        BRWalletManagerConnect(self.coreWalletManager)
    }
    
    public func disconnect() {
        BRWalletManagerDisconnect(self.coreWalletManager)
    }
}

