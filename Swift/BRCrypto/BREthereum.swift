//
//  BREthereum.swift
//  BRCoreX
//
//  Created by Ed Gamble on 11/7/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import Foundation
import Core.Ethereum

public struct Ethereum {
    public static let currency = Currency (code: "ETH", symbol:  "Ξ", name: "Ethereum", decimals: 18,
                                           baseUnit: (name: "WEI",symbol: "wei"))
    public struct Units {
        public static let WEI: Unit = Ethereum.currency.baseUnit!
        public static let ETHER: Unit = Ethereum.currency.defaultUnit!
        // others
        public static let GWEI: Unit = Unit ("GWEI",  "gwei",          1000000000, base: WEI)
    }

    public struct Networks {
        public static let mainnet = Network.ethereum (name: "ETH Mainnet", chainId: 1, core: ethereumMainnet)
        public static let ropsten = Network.ethereum (name: "ETH Ropsten", chainId: 3, core: ethereumTestnet)
        public static let rinkeby = Network.ethereum (name: "ETH Rinkeby", chainId: 4, core: ethereumRinkeby)
        public static let foundation = mainnet
    }
}

extension WalletManagerEvent {
    init (_ core: BREthereumEWMEvent) {
        switch core {
        case EWM_EVENT_CREATED:        self = .created
        case EWM_EVENT_SYNC_STARTED:   self = .syncStarted
        case EWM_EVENT_SYNC_CONTINUES: self = .syncProgress(percentComplete: 50.0)
        case EWM_EVENT_SYNC_STOPPED:   self = .syncEnded
        case EWM_EVENT_NETWORK_UNAVAILABLE: self = .created
        case EWM_EVENT_DELETED:        self = .deleted
        default:
            self = .created
        }
    }
}

extension WalletEvent {
    init (_ core: BREthereumWalletEvent) {
        switch core {
        case WALLET_EVENT_CREATED: self = .created
        case WALLET_EVENT_DELETED: self = .deleted
        case WALLET_EVENT_BALANCE_UPDATED: self = .created // .balanceUpdated(amount: <#T##Amount#>)
        case WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED: self = .created // self = .feeBasisUpdated(feeBasis: <#T##TransferFeeBasis#>)
        case WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED: self = .created
        default: self = .created
        }
    }
}

extension TransferEvent {
    init (_ core: BREthereumTransferEvent) {
        switch core {
        case TRANSFER_EVENT_CREATED:   self = .created
        case TRANSFER_EVENT_SIGNED:    self = .changed(old: .created, new: .signed)
        case TRANSFER_EVENT_SUBMITTED: self = .changed(old: .signed, new: .submitted)
        case TRANSFER_EVENT_INCLUDED:  self = .changed(old: .submitted, new: .pending) // .included
        case TRANSFER_EVENT_ERRORED:   self = .changed(old: .submitted, new: .failed(reason: "something"))
        case TRANSFER_EVENT_DELETED:   self = .deleted
            
        case TRANSFER_EVENT_GAS_ESTIMATE_UPDATED: self = .created
        case TRANSFER_EVENT_BLOCK_CONFIRMATIONS_UPDATED: self = .created
        default:
            self = .created
        }
    }
}

public enum EthereumTokenEvent {
    case created
    case deleted
}

extension EthereumTokenEvent {
    init (_ core: BREthereumTokenEvent) {
        switch core {
        case TOKEN_EVENT_CREATED: self = .created
        case TOKEN_EVENT_DELETED: self = .deleted
        default:
            self = .created
        }
    }
}


/// An Ethereum Persistence Client adds the `changeLog()` interface to a Wallet Manager
/// Persistence Client.
public protocol EthereumPersistenceClient: WalletManagerPersistenceClient {
    func changeLog (manager: WalletManager,
                    change: WalletManagerPersistenceChangeType,
                    hash: String,
                    data: String) -> Void
}


/// An Ethereum Backend Client extends the Wallet Manager Backend Client to add numerous functions
/// specific to Ethereum, such as `getGasPrice()` and `getNonce()`.
public protocol EthereumBackendClient: WalletManagerBackendClient {
    func getGasPrice (ewm: EthereumWalletManager,
                      wid: BREthereumWallet,
                      rid: Int32) -> Void
    
    func getGasEstimate (ewm: EthereumWalletManager,
                         wid: BREthereumWallet,
                         tid: BREthereumTransfer,
                         to: String,
                         amount: String,
                         data:  String,
                         rid: Int32) -> Void
    
    func getBalance (ewm: EthereumWalletManager,
                     wid: BREthereumWallet,
                     address: String,
                     rid: Int32) -> Void
    // ewm.announceBalance (...)
    
    func submitTransaction (ewm: EthereumWalletManager,
                            wid: BREthereumWallet,
                            tid: BREthereumTransfer,
                            rawTransaction: String,
                            rid: Int32) -> Void
    // ...
    func getTransactions (ewm: EthereumWalletManager,
                          address: String,
                          rid: Int32) -> Void
    
    func getLogs (ewm: EthereumWalletManager,
                  address: String,
                  event: String,
                  rid: Int32) -> Void
    
    func getBlocks (ewm: EthereumWalletManager,
                    address: String,
                    interests: UInt32,
                    blockStart: UInt64,
                    blockStop: UInt64,
                    rid: Int32) -> Void
    
    func getTokens (ewm: EthereumWalletManager,
                    rid: Int32) -> Void
    
    func getBlockNumber (ewm: EthereumWalletManager,
                         rid: Int32) -> Void
    
    func getNonce (ewm: EthereumWalletManager,
                   address: String,
                   rid: Int32) -> Void
    
}

public class EthereumBlock {
}


/// An ERC20 Smart Contract Token
public class EthereumToken {

    /// A reference to the Core's BREthereumToken
    internal let identifier: BREthereumToken

    /// The currency
    public let currency: Currency

    /// The address of the token's ERC20 Smart Contract
    public let address: Address

    internal init (identifier: BREthereumToken) {
        self.identifier = identifier
        self.address = Address.ethereum(tokenGetAddressRaw(identifier))

        let symb = asUTF8String(tokenGetSymbol(identifier))
        let name = asUTF8String(tokenGetName(identifier))

        self.currency = Currency (code: symb,
                                  symbol: symb,
                                  name: name,
                                  decimals: UInt8(tokenGetDecimals(identifier)),
                                  baseUnit: (name: name, symbol: symb))
    }
}

public protocol EthereumListener: WalletManagerListener {
    func handleTokenEvent (manager: WalletManager,
                           token: EthereumToken,
                           event: EthereumTokenEvent)
}

/// An EthereumTransfer is a Transfer that represents an Ethereum transaction or log
public class EthereumTransfer: Transfer {

    /// A reference to the Core's BREthereumTransfer
    internal let identifier: BREthereumTransfer

    public unowned let _wallet: EthereumWallet

    public var wallet: Wallet {
        return _wallet
    }
    
    private var core: BREthereumEWM {
        return _wallet._manager.core!
    }
    
    public private(set) lazy var source: Address? = {
        return Address.ethereum (ewmTransferGetSource (self.core, self.identifier))
    }()
    
    public private(set) lazy var target: Address? = {
        return Address.ethereum (ewmTransferGetTarget(self.core, self.identifier))
    }()
    
    public private(set) lazy var amount: Amount = {
        let amount: BREthereumAmount = ewmTransferGetAmount (self.core, self.identifier)
        return (AMOUNT_ETHER == amount.type
            ? Amount (value: amount.u.ether.valueInWEI,
                      unit: wallet.currency.defaultUnit,
                      negative: false)
            : Amount (value: amount.u.tokenQuantity.valueAsInteger,
                      unit: wallet.currency.defaultUnit,
                      negative: false))
    }()
    
    public private(set) lazy var fee: Amount = {
        var overflow: Int32 = 0;
        let fee: BREthereumEther = ewmTransferGetFee (self.core, self.identifier, &overflow)
        return Amount (value: fee.valueInWEI,
                       unit: Ethereum.currency.defaultUnit,
                       negative: false)
    }()
    
    public private(set) lazy var feeBasis: TransferFeeBasis = {
        var status: BRCoreParseStatus = CORE_PARSE_OK
        let price = ewmTransferGetGasPrice (self.core, self.identifier, GWEI)
        //        let price = createUInt256Parse (priceStr, 10, &status)
        
        return TransferFeeBasis.ethereum(
            gasPrice: Amount (value: price.etherPerGas.valueInWEI, unit: Ethereum.Units.GWEI, negative: false),
            gasLimit: ewmTransferGetGasLimit (self.core, self.identifier).amountOfGas)
    }()
    
    public var confirmation: TransferConfirmation? {
        guard ETHEREUM_BOOLEAN_TRUE == ewmTransferIsConfirmed (self.core, self.identifier)
            else { return nil }
        
        var overflow: Int32 = 0
        let fee = ewmTransferGetFee (self.core, self.identifier, &overflow)
        return TransferConfirmation (
            blockNumber: ewmTransferGetBlockNumber (self.core, self.identifier),
            transactionIndex: ewmTransferGetTransactionIndex (self.core, self.identifier),
            timestamp: 0,
            fee: Amount (value: fee.valueInWEI, unit: Ethereum.Units.ETHER, negative: false))
    }
    
    public var hash: TransferHash? {
        guard ETHEREUM_BOOLEAN_TRUE == ewmTransferIsSubmitted (self.core, self.identifier)
            else { return nil }
        let hash = ewmTransferGetHash(self.core, self.identifier)
        return TransferHash (
            asUTF8String (hashAsString(hash), true))
    }
    
    public private(set) var state: TransferState
    
    init (wallet: EthereumWallet,
          tid: BREthereumTransfer) {
        self._wallet = wallet
        self.identifier = tid
        
        // TODO: Should be: included w/ TransferConfirmation
        self.state = TransferState.submitted
    }
    
    convenience init? (wallet: EthereumWallet,
                       target: Address,
                       amount: Amount,
                       feeBasis: TransferFeeBasis) {
        
        guard case let .ethereum(gasPriceBasis, gasLImitBasis) = feeBasis else { precondition(false) }
        
        let core = wallet._manager.core!
        
        let gasPrice = gasPriceCreate (etherCreate (gasPriceBasis.value))
        let gasLImit = gasCreate (gasLImitBasis)

        guard let asEther = amount.asEther else { return nil }

        let feeBasis = feeBasisCreate(gasLImit, gasPrice);

        self.init (wallet: wallet,
                   tid:  ewmWalletCreateTransferWithFeeBasis (core,
                                                              wallet.identifier,
                                                              target.description,
                                                              asEther,
                                                              feeBasis))
    }
}

public class EthereumTransferFactory: TransferFactory {
    public func createTransfer(wallet: Wallet, target: Address, amount: Amount, feeBasis: TransferFeeBasis) -> Transfer? {
        return EthereumTransfer (wallet: wallet as! EthereumWallet,
                                 target: target,
                                 amount: amount,
                                 feeBasis: feeBasis)
    }
}

///
/// MARK: - Wallet
///

public class EthereumWallet: Wallet {
    internal let identifier: BREthereumWallet
    
    public unowned let _manager: EthereumWalletManager
    
    public var manager: WalletManager {
        return _manager
    }
    
    private var core: BREthereumEWM {
        return _manager.core!
    }
    
    internal let currency: Currency
    
    public var balance: Amount {
        let amount: BREthereumAmount = ewmTransferGetAmount (self.core, self.identifier)
        return Amount (value: (AMOUNT_ETHER == amount.type ?  amount.u.ether.valueInWEI : amount.u.tokenQuantity.valueAsInteger),
                       unit: currency.defaultUnit,
                       negative: false)
    }
    
    public private(set) var transfers: [Transfer] = []
    
    internal func findTransfer (identifier: BREthereumTransfer) -> EthereumTransfer {
        return transfers.first { identifier == ($0 as! EthereumTransfer).identifier } as! EthereumTransfer
    }
    
    public func lookup(transfer: TransferHash) -> Transfer? {
        return transfers
            .first  { $0.hash.map { $0.string == transfer.string } ?? false }
    }
    
    
    public private(set) var state: WalletState
    
    public var defaultFeeBasis: TransferFeeBasis {
        get {
            let gasLimit = ewmWalletGetDefaultGasLimit (self.core, self.identifier)
            let gasPrice = ewmWalletGetDefaultGasPrice (self.core, self.identifier)
            return TransferFeeBasis.ethereum(
                gasPrice: Amount (value: gasPrice.etherPerGas.valueInWEI, unit: Ethereum.Units.GWEI, negative: false),
                gasLimit: gasLimit.amountOfGas)
        }
        set (basis) {
            guard case let .ethereum (gasPriceBasis, gasLimitBasis) = basis else { precondition(false) }
            
            var overflow: Int32 = 0
            let gasPrice = coerceUInt64 (gasPriceBasis.value, &overflow)
            
            ewmWalletSetDefaultGasLimit (self.core, self.identifier, gasCreate(gasLimitBasis))
            ewmWalletSetDefaultGasPrice (self.core, self.identifier,
                                         gasPriceCreate(etherCreateNumber(gasPrice, WEI)))
        }
    }
    
    public var transferFactory: TransferFactory = EthereumTransferFactory()
    
    public var target: Address {
        return Address.ethereum (accountGetPrimaryAddress (_manager.account.ethereumAccount))
    }
    
    internal init (manager:EthereumWalletManager,
                   wid: BREthereumWallet) {
        self._manager = manager
        self.identifier = wid
        
        self.currency = Ethereum.currency
        self.state = WalletState.created
    }
}

#if false
public class EthereumWalletFactory: WalletFactory {
    public func createWallet (manager: WalletManager, currency: Currency) -> Wallet {
        return EthereumWallet (manager: manager as! EthereumWalletManager,
                               currency: currency)
    }
}
#endif

public class EthereumWalletManager: WalletManager {

    internal var core: BREthereumEWM! = nil
    
    internal let backendClient: EthereumBackendClient
    internal let persistenceClient: EthereumPersistenceClient
    
    public let _listener : EthereumListener
    public var listener : WalletManagerListener {
        return _listener
    }

    lazy var queue : DispatchQueue = {
        return DispatchQueue (label: "Ethereum")
    }()

    public let account: Account
    
    //    public var address: EthereumAddress {
    //        return EthereumAddress ("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62")
    //    }
    
    public var network: Network
    
    public lazy var primaryWallet: Wallet = {
        return EthereumWallet (manager: self,
                               wid: ewmGetWallet(self.core))
    }()
    
    public lazy var wallets: [Wallet] = {
        return [primaryWallet]
    } ()
    
    internal func findWallet (identifier: BREthereumWallet) -> EthereumWallet {
        return wallets.first { identifier == ($0 as! EthereumWallet).identifier } as! EthereumWallet
    }
    
    public var mode: WalletManagerMode
    
    public var path: String
    
    public var state: WalletManagerState
    
    #if false
    public var walletFactory: WalletFactory = EthereumWalletFactory()
    #endif
    
    public func connect() {
        ewmConnect (self.core)
    }
    
    public func disconnect() {
        ewmDisconnect (self.core)
    }
    
    public init (listener: EthereumListener,
                 account: Account,
                 network: Network,
                 mode: WalletManagerMode,
                 timestamp: BREthereumTimestamp,
                 persistenceClient: EthereumPersistenceClient = DefaultEthereumPersistenceClient(),
                 backendClient: EthereumBackendClient = DefaultEthereumBackendClient()) {
        
        guard case let .ethereum (_, _, coreNetwork) = network else { precondition(false) }
        
        self.backendClient = backendClient
        self.persistenceClient = persistenceClient
        
        self._listener = listener
        self.account = account
        self.network = network
        self.mode = mode
        self.path = ""
        self.state = WalletManagerState.created
        
        //        let coreNetwork = (main ? ethereumMainnet : ethereumTestnet)
        
        let peers:        Dictionary<String,String> = [:]
        let blocks:       Dictionary<String,String> = [:]
        let transactions: Dictionary<String,String> = [:]
        let logs:         Dictionary<String,String> = [:]
        
        self.core = ewmCreate (coreNetwork,
                               account.ethereumAccount,
                               timestamp,
                               EthereumWalletManager.coreMode (mode),
                               coreEthereumClient,
                               EthereumWalletManager.asPairs(peers),
                               EthereumWalletManager.asPairs(blocks),
                               EthereumWalletManager.asPairs(transactions),
                               EthereumWalletManager.asPairs(logs))
    }

    // Actually a Set/Dictionary by {Symbol}
    public private(set) var all: [EthereumToken] = []

    internal func addToken (identifier: BREthereumToken) {
        let token = EthereumToken (identifier: identifier)
        all.append (token)
        self._listener.handleTokenEvent(manager: self, token: token, event: EthereumTokenEvent.created)
    }

    internal func remToken (identifier: BREthereumToken) {
        if let index = all.firstIndex (where: { $0.identifier == identifier}) {
            let token = all[index]
            all.remove(at: index)
            self._listener.handleTokenEvent(manager: self, token: token, event: EthereumTokenEvent.deleted)
        }
    }

    internal func findToken (identifier: BREthereumToken) -> EthereumToken? {
        return all.first { $0.identifier == identifier }
    }

    /// All known managers
    private static var managers : [Weak<EthereumWalletManager>] = []
    
    private static func lookup (core: BREthereumEWM?) -> EthereumWalletManager? {
        guard let core = core else { return nil }
        return managers
            .filter { nil != $0.value }         // expunge weak references
            .map { $0.value! }                  // get reference
            .first { $0.core == core }          // match
    }
    
    private lazy var coreEthereumClient: BREthereumClient = {
        let this = self
        return BREthereumClient (
            context: UnsafeMutableRawPointer (Unmanaged<EthereumWalletManager>.passRetained(this).toOpaque()),
            
            funcGetBalance: { (coreClient, coreEWM, wid, address, rid) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.backendClient.getBalance(ewm: ewm,
                                                     wid: wid!,
                                                     address: asUTF8String(address!),
                                                     rid: rid)
                    }
                }},
            
            funcGetGasPrice: { (coreClient, coreEWM, wid, rid) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.backendClient.getGasPrice (ewm: ewm,
                                                       wid: wid!,
                                                       rid: rid)
                    }
                }},
            
            funcEstimateGas: { (coreClient, coreEWM, wid, tid, to, amount, data, rid)  in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.backendClient.getGasEstimate(ewm: ewm,
                                                         wid: wid!,
                                                         tid: tid!,
                                                         to: asUTF8String(to!),
                                                         amount: asUTF8String(amount!),
                                                         data: asUTF8String(data!),
                                                         rid: rid)
                    }
                }},
            
            funcSubmitTransaction: { (coreClient, coreEWM, wid, tid, transaction, rid)  in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.backendClient.submitTransaction(ewm: ewm,
                                                            wid: wid!,
                                                            tid: tid!,
                                                            rawTransaction: asUTF8String(transaction!),
                                                            rid: rid)
                    }
                }},
            
            funcGetTransactions: { (coreClient, coreEWM, address, rid) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.backendClient.getTransactions(ewm: ewm,
                                                          address: asUTF8String(address!),
                                                          rid: rid)
                    }
                }},
            
            funcGetLogs: { (coreClient, coreEWM, contract, address, event, rid) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.backendClient.getLogs (ewm: ewm,
                                                   address: asUTF8String(address!),
                                                   event: asUTF8String(event!),
                                                   rid: rid)
                    }
                }},
            
            funcGetBlocks: { (coreClient, coreEWM, address, interests, blockStart, blockStop, rid) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.backendClient.getBlocks (ewm: ewm,
                                                     address: asUTF8String (address!),
                                                     interests: interests,
                                                     blockStart: blockStart,
                                                     blockStop: blockStop,
                                                     rid: rid)
                    }
                }},
            
            funcGetTokens: { (coreClient, coreEWM, rid) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.backendClient.getTokens (ewm: ewm, rid: rid)
                    }
                }},
            
            funcGetBlockNumber: { (coreClient, coreEWM, rid) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.backendClient.getBlockNumber(ewm: ewm, rid: rid)
                    }
                }},
            
            funcGetNonce: { (coreClient, coreEWM, address, rid) in
                if  let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.backendClient.getNonce(ewm: ewm,
                                                   address: asUTF8String(address!),
                                                   rid: rid)
                    }
                }},
            
            funcSaveNodes: { (coreClient, coreEWM, data) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.persistenceClient.savePeers(manager: ewm, data: EthereumWalletManager.asDictionary(data!))
                    }
                }},
            
            funcSaveBlocks: { (coreClient, coreEWM, data) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.persistenceClient.saveBlocks(manager: ewm, data: EthereumWalletManager.asDictionary(data!))
                    }
                }},
            
            funcChangeTransaction: { (coreClient, coreEWM, change, data) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        
                        let cStrHash = hashDataPairGetHashAsString (data)!
                        let cStrData = hashDataPairGetDataAsString (data)!
                        
                        ewm.persistenceClient.changeTransaction (manager: ewm,
                                                                 change: WalletManagerPersistenceChangeType(change),
                                                                 hash: String (cString: cStrHash),
                                                                 data: String (cString: cStrData))
                        
                        free (cStrHash); free (cStrData)
                    }
                }},
            
            funcChangeLog: { (coreClient, coreEWM, change, data) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        
                        let cStrHash = hashDataPairGetHashAsString (data)!
                        let cStrData = hashDataPairGetDataAsString (data)!
                        
                        ewm.persistenceClient.changeLog (manager: ewm,
                                                         change: WalletManagerPersistenceChangeType(change),
                                                         hash: String (cString: cStrHash),
                                                         data: String (cString: cStrData))
                        
                        free (cStrHash); free (cStrData)
                    }
                }},
            
            funcEWMEvent: { (coreClient, coreEWM, event, status, message) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        ewm.listener.handleManagerEvent (manager: ewm,
                                                         event: WalletManagerEvent(event))
                    }
                }},
            
            funcPeerEvent: { (coreClient, coreEWM, event, status, message) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        //                    ewm.listener.handlePeerEvent (ewm: ewm, event: EthereumPeerEvent (event))
                    }
                }},
            
            funcWalletEvent: { (coreClient, coreEWM, wid, event, status, message) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        
                        //                    let event = WalletManagerEvent.created
                        //                    ewm.listener.handleManagerEvent(manager: ewm,
                        //                                                       event: event)
                        ewm.listener.handleWalletEvent(manager: ewm,
                                                       wallet: ewm.findWallet(identifier: wid!),
                                                       event: WalletEvent (event))
                    }
                }},

            funcTokenEvent: { (coreClient, coreEWM, token, event) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        if let token = ewm.findToken(identifier: token!) {
                            ewm._listener.handleTokenEvent (manager: ewm,
                                                            token: token,
                                                            event: EthereumTokenEvent(event))
                        }
                    }
                }},

            //            funcBlockEvent: { (coreClient, coreEWM, bid, event, status, message) in
            //                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
            //                    //                    ewm.listener.handleBlockEvent(ewm: ewm,
            //                    //                                                 block: ewm.findBlock(identifier: bid),
            //                    //                                                 event: EthereumBlockEvent (event))
            //                }},
            
            funcTransferEvent: { (coreClient, coreEWM, wid, tid, event, status, message) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        
                        let wallet = ewm.findWallet(identifier: wid!)
                        let transfer = wallet.findTransfer(identifier: tid!)
                        ewm.listener.handleTransferEvent(manager: ewm,
                                                         wallet: wallet,
                                                         transfer: transfer,
                                                         event: TransferEvent (event))
                    }
                }})
    }()
    
    ///
    /// Private Support
    ///
    
    private static func coreMode (_ mode: WalletManagerMode) -> BREthereumMode {
        switch mode {
        case .api_only: return BRD_ONLY
        case .api_with_p2p_submit: return BRD_WITH_P2P_SEND
        case .p2p_with_api_sync: return P2P_WITH_BRD_SYNC
        case .p2p_only: return P2P_ONLY
        }
    }
    
    
    private static func asPairs (_ set: Dictionary<String,String>) -> OpaquePointer {
        let pairs = hashDataPairSetCreateEmpty(set.count)!
        set.forEach { (hash: String, data: String) in
            hashDataPairAdd (pairs, hash, data)
        }
        return pairs
    }
    
    private static func asDictionary (_ set: OpaquePointer) -> Dictionary<String,String> {
        var dict : [String:String] = [:]
        
        var pair : BREthereumHashDataPair? = nil
        while let p = OpaquePointer.init (BRSetIterate (set, &pair)) {
            let cStrHash = hashDataPairGetHashAsString (p)!
            let cStrData = hashDataPairGetDataAsString (p)!
            
            dict [String (cString: cStrHash)] = String (cString: cStrData)
            
            free (cStrHash); free (cStrData)
            
            pair = p
        }
        
        return dict
    }
    
    ///
    /// Ethereum Backend Announce Interface
    ///
    public func announceBalance (wid: BREthereumWallet, balance: String, rid: Int32) {
        ewmAnnounceWalletBalance (core, wid, balance, rid)
    }
    
    public func announceGasPrice (wid: BREthereumWallet, gasPrice: String, rid: Int32) {
        ewmAnnounceGasPrice (core, wid, gasPrice, rid)
    }
    
    public func announceGasEstimate (wid: BREthereumWallet, tid: BREthereumTransfer, gasEstimate: String, rid: Int32) {
        ewmAnnounceGasEstimate (core, wid, tid, gasEstimate, rid)
    }
    
    public func announceSubmitTransaction (wid: BREthereumWallet, tid: BREthereumTransfer, hash: String, rid: Int32) {
        ewmAnnounceSubmitTransfer (core, wid, tid, hash, rid)
    }
    
    public func announceTransaction (rid: Int32,
                                     hash: String,
                                     sourceAddr: String,
                                     targetAddr: String,
                                     contractAddr: String,
                                     amount: String,
                                     gasLimit: String,
                                     gasPrice: String,
                                     data: String,
                                     nonce: String,
                                     gasUsed: String,
                                     blockNumber: String,
                                     blockHash: String,
                                     blockConfirmations: String,
                                     blockTransactionIndex: String,
                                     blockTimestamp: String,
                                     isError: String) {
        ewmAnnounceTransaction (core, rid,
                                hash, sourceAddr, targetAddr, contractAddr,
                                amount, gasLimit, gasPrice,
                                data, nonce, gasUsed,
                                blockNumber, blockHash, blockConfirmations,
                                blockTransactionIndex, blockTimestamp,
                                isError)
    }
    
    public func announceLog (rid: Int32,
                             hash: String,
                             contract: String,
                             topics: [String],
                             data: String,
                             gasPrice: String,
                             gasUsed: String,
                             logIndex: String,
                             blockNumber: String,
                             blockTransactionIndex: String,
                             blockTimestamp: String) {
        var cTopics = topics.map { UnsafePointer<Int8>(strdup($0)) }
        
        ewmAnnounceLog (core, rid,
                        hash, contract, Int32(topics.count), &cTopics,
                        data, gasPrice, gasUsed,
                        logIndex,
                        blockNumber, blockTransactionIndex, blockTimestamp)
        cTopics.forEach { free (UnsafeMutablePointer(mutating: $0)) }
    }
    
    public func announceBlocks (rid: Int32,
                                blockNumbers: [UInt64]) {
        // TODO: blocks must be BRArrayOf(uint64_t) - change to add `count`
        ewmAnnounceBlocks (core, rid,
                           Int32(blockNumbers.count),
                           UnsafeMutablePointer<UInt64>(mutating: blockNumbers))
    }
    
    public func announceToken (rid: Int32,
                               address: String,
                               symbol: String,
                               name: String,
                               description: String,
                               decimals: UInt32) {
        ewmAnnounceToken(core, address, symbol, name, description, decimals, nil, nil, rid)
    }
    
    public func announceBlockNumber (blockNumber: String, rid: Int32) {
        ewmAnnounceBlockNumber(core, blockNumber, rid)
    }
    
    public func announceNonce (address: String, nonce: String, rid: Int32) {
        ewmAnnounceNonce(core, address, nonce, rid)
    }
    
}


public class DefaultEthereumBackendClient: EthereumBackendClient {
    public func getBalance(ewm: EthereumWalletManager, wid: BREthereumWallet, address: String, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceBalance()
        ewm.announceBalance (wid: wid, balance: "0xffc0", rid: rid)
    }
    
    public func getGasPrice(ewm: EthereumWalletManager, wid: BREthereumWallet, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceGasPrice()
        ewm.announceGasPrice (wid: wid, gasPrice: "0x77", rid: rid)
    }
    
    public func getGasEstimate(ewm: EthereumWalletManager, wid: BREthereumWallet, tid: BREthereumTransfer, to: String, amount: String, data: String, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceGasEstimate()
        ewm.announceGasEstimate (wid: wid, tid: tid, gasEstimate: "0x21000", rid: rid)
    }
    
    public func submitTransaction(ewm: EthereumWalletManager, wid: BREthereumWallet, tid: BREthereumTransfer, rawTransaction: String, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceSubmitTransaction()
        ewm.announceSubmitTransaction (wid: wid, tid: tid, hash: "0xffaabb", rid: rid)
        return
    }
    
    public func getTransactions(ewm: EthereumWalletManager, address: String, rid: Int32) {
        // JSON_RPC -> [JSON] -> forEach {Result -> announceSubmitTransaction()}
        ewm.announceTransaction(rid: rid,
                                hash: "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
                                sourceAddr: ewm.primaryWallet.target.description,
                                targetAddr: "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
                                contractAddr: "",
                                amount: "11113000000000",
                                gasLimit: "21000",
                                gasPrice: "21000000000",
                                data: "0x",
                                nonce: "118",
                                gasUsed: "21000",
                                blockNumber: "1627184",
                                blockHash: "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                                blockConfirmations: "339050",
                                blockTransactionIndex: "3",
                                blockTimestamp: "1516477482",
                                isError: "0")
        return
    }
    
    public func getLogs(ewm: EthereumWalletManager, address: String, event: String, rid: Int32) {
        ewm.announceLog(rid: rid,
                        hash: "0xa37bd8bd8b1fa2838ef65aec9f401f56a6279f99bb1cfb81fa84e923b1b60f2b",
                        contract: (ewm.network == Ethereum.Networks.mainnet
                            ? "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"
                            : "0x7108ca7c4718efa810457f228305c9c71390931a"),
                        topics: ["0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
                                 "0x0000000000000000000000000000000000000000000000000000000000000000",
                                 "0x000000000000000000000000bdfdad139440d2db9ba2aa3b7081c2de39291508"],
                        data: "0x0000000000000000000000000000000000000000000000000000000000002328",
                        gasPrice: "0xba43b7400",
                        gasUsed: "0xc64e",
                        logIndex: "0x",
                        blockNumber: "0x1e487e",
                        blockTransactionIndex: "0x",
                        blockTimestamp: "0x59fa1ac9")
        return
    }
    
    public func getBlocks(ewm: EthereumWalletManager, address: String, interests: UInt32, blockStart: UInt64, blockStop: UInt64, rid: Int32) {
        var blockNumbers : [UInt64] = []
        switch address.lowercased() {
        case "0xb302B06FDB1348915599D21BD54A06832637E5E8".lowercased():
            if 0 != interests & UInt32 (1 << 3) /* CLIENT_GET_BLOCKS_LOGS_AS_TARGET */ {
                blockNumbers += [4847049,
                                 4847152,
                                 4894677,
                                 4965538,
                                 4999850,
                                 5029844]
            }
            
            if 0 != interests & UInt32 (1 << 2) /* CLIENT_GET_BLOCKS_LOGS_AS_SOURCE */ {
                blockNumbers += [5705175]
            }
            
            if 0 != interests & UInt32 (1 << 1) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET */ {
                blockNumbers += [4894027,
                                 4908682,
                                 4991227]
            }
            
            if 0 != interests & UInt32 (1 << 0) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE */ {
                blockNumbers += [4894330,
                                 4894641,
                                 4894677,
                                 4903993,
                                 4906377,
                                 4997449,
                                 4999850,
                                 4999875,
                                 5000000,
                                 5705175]
            }
            
        case "0xa9de3dbD7d561e67527bC1Ecb025c59D53b9F7Ef".lowercased():
            if 0 != interests & UInt32 (1 << 3) /* CLIENT_GET_BLOCKS_LOGS_AS_TARGET */ {
                blockNumbers += [5506607,
                                 5877545]
            }
            
            if 0 != interests & UInt32 (1 << 2) /* CLIENT_GET_BLOCKS_LOGS_AS_SOURCE */ {
                blockNumbers += [5506764,
                                 5509990,
                                 5511681]
            }
            
            if 0 != interests & UInt32 (1 << 1) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET */ {
                blockNumbers += [5506602]
            }
            
            if 0 != interests & UInt32 (1 << 0) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE */ {
                blockNumbers += [5506764,
                                 5509990,
                                 5511681,
                                 5539808]
            }
            
        default:
            blockNumbers.append(contentsOf: [blockStart,
                                             (blockStart + blockStop) / 2,
                                             blockStop])
        }
        
        blockNumbers = blockNumbers.filter { blockStart < $0 && $0 < blockStop }
        ewm.announceBlocks(rid: rid, blockNumbers: blockNumbers)
    }
    
    public func getTokens(ewm: EthereumWalletManager, rid: Int32) {
        ewm.announceToken (rid: rid,
                           address: (ewm.network == Ethereum.Networks.mainnet
                            ? "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"
                            : "0x7108ca7c4718efa810457f228305c9c71390931a"),
                           symbol: "BRD",
                           name: "BRD Token",
                           description: "The BRD Token",
                           decimals: 18)
        
        ewm.announceToken (rid: rid,
                           address: (ewm.network == Ethereum.Networks.mainnet
                            ? "0x9e3359f862b6c7f5c660cfd6d1aa6909b1d9504d"
                            : "0x6e67ccd648244b3b8e2f56149b40ba8de9d79b09"),
                           symbol: "CCC",
                           name: "Container Crypto Coin",
                           description: "",
                           decimals: 18)
        
        ewm.announceToken (rid: rid,
                           address: "0xdd974d5c2e2928dea5f71b9825b8b646686bd200",
                           symbol: "KNC",
                           name: "KNC Token",
                           description: "",
                           decimals: 18)
        
    }
    
    public func getBlockNumber(ewm: EthereumWalletManager, rid: Int32) {
        ewm.announceBlockNumber(blockNumber: "5900000", rid: rid)
    }
    
    public func getNonce(ewm: EthereumWalletManager, address: String, rid: Int32) {
        ewm.announceNonce(address: address, nonce: "17", rid: rid)
    }
    
    public init () {}
}

public class DefaultEthereumPersistenceClient: EthereumPersistenceClient {
    public func savePeers(manager: WalletManager,
                          data: Dictionary<String, String>) {
        print ("TST: ETH: savePeers")
    }
    
    public func saveBlocks(manager: WalletManager,
                           data: Dictionary<String, String>) {
        print ("TST: ETH: saveBlocks")
    }
    
    public func changeTransaction (manager: WalletManager,
                                   change: WalletManagerPersistenceChangeType,
                                   hash: String,
                                   data: String) {
        print ("TST: ETH: changeTransaction")
    }
    
    public func changeLog (manager: WalletManager,
                           change: WalletManagerPersistenceChangeType,
                           hash: String,
                           data: String) {
        print ("TST: ETH: changeLog")
    }
    
    public init () {}
}
