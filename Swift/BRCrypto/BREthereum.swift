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

    public struct AddressSchemes {
        public static let `default` = EthereumAddressScheme()
    }
}

public struct EthereumAddressScheme: AddressScheme {
    public func getAddress(for wallet: EthereumWallet) -> Address {
        let account = ewmGetAccount(wallet._manager.core)
        return Address.ethereum(accountGetPrimaryAddress (account))
    }
}

extension Amount {
    var asEther: BREthereumAmount? {
        return self.currency != Ethereum.currency
            ? nil
            : amountCreateEther (etherCreate (self.value))
    }
}

extension Network {
    var ethereumCore: BREthereumNetwork? {
        if case .ethereum (_, _, let core) = self { return core }
        else { return nil }
    }
}

extension TransferFeeBasis {
    var ethereumValues: (gasPrice: Amount, gasLimit: UInt64)? {
        if case let .ethereum (gasPrice, gasLimit) = self { return (gasPrice: gasPrice, gasLimit: gasLimit) }
        else { return nil }
    }
}

///
/// An Ethereum Persistence Client adds the `changeLog()` interface to a Wallet Manager
/// Persistence Client.
///
public protocol EthereumPersistenceClient: WalletManagerPersistenceClient {
    func changeLog (manager: WalletManager,
                    change: WalletManagerPersistenceChangeType,
                    hash: String,
                    data: String) -> Void
}

///
/// An Ethereum Backend Client extends the Wallet Manager Backend Client to add numerous functions
/// specific to Ethereum, such as `getGasPrice()` and `getNonce()`.
///
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

///
///
///
public class EthereumBlock {
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
/// An EthereumListener extends a WalletManagerListener for handling token events
///
public protocol EthereumListener: WalletManagerListener {
    func handleTokenEvent (manager: WalletManager,
                           token: EthereumToken,
                           event: EthereumTokenEvent)
}

///
/// An EthereumTransfer is a Transfer that represents an Ethereum transaction or log
///
public class EthereumTransfer: Transfer {
    /// A reference to the Core's BREthereumTransfer
    internal let identifier: BREthereumTransfer

    /// The EthereumWallet owning this transfer
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
    
    public var hash: TransferHash? {
        guard ETHEREUM_BOOLEAN_TRUE == ewmTransferIsSubmitted (self.core, self.identifier)
            else { return nil }
        let hash = ewmTransferGetHash(self.core, self.identifier)
        return TransferHash.ethereum (hash)
    }
    
    public private(set) var state: TransferState

    public var isSent: Bool {
        let account = _wallet._manager.account.ethereumAccount
        if let source = source,  case .ethereum (let address) = source {
            return ETHEREUM_BOOLEAN_TRUE == addressEqual ( accountGetPrimaryAddress (account), address)
        }
        else { return false }
    }
    
    init (wallet: EthereumWallet,
          tid: BREthereumTransfer) {
        self._wallet = wallet
        self.identifier = tid
        self.state = TransferState (ewm: wallet._manager.core, tid: tid)
    }
    
    convenience init? (wallet: EthereumWallet,
                       target: Address,
                       amount: Amount,
                       feeBasis: TransferFeeBasis) {

        guard case let .ethereum(gasPriceBasis, gasLImitBasis) = feeBasis else { return nil }
        
        let core = wallet._manager.core!
        
        let gasPrice = gasPriceCreate (etherCreate (gasPriceBasis.value))
        let gasLImit = gasCreate (gasLImitBasis)

        guard let asEther = amount.asEther else { return nil }

        let coreFeeBasis = feeBasisCreate(gasLImit, gasPrice);

        self.init (wallet: wallet,
                   tid:  ewmWalletCreateTransferWithFeeBasis (core,
                                                              wallet.identifier,
                                                              target.description,
                                                              asEther,
                                                              coreFeeBasis))
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
        let amount: BREthereumAmount =  ewmWalletGetBalance (self.core, self.identifier)
        return Amount (value: (AMOUNT_ETHER == amount.type ?  amount.u.ether.valueInWEI : amount.u.tokenQuantity.valueAsInteger),
                       unit: currency.defaultUnit,
                       negative: false)
    }
    
    public private(set) var transfers: [Transfer] = []

    internal func addTransfer (identifier: BREthereumTransfer) {
        if nil == findTransfer(identifier: identifier) {
            transfers.append(EthereumTransfer (wallet: self, tid: identifier))
        }
    }

    internal func findTransfer (identifier: BREthereumTransfer) -> EthereumTransfer? {
        return transfers.first { identifier == ($0 as! EthereumTransfer).identifier } as? EthereumTransfer
    }
    
    public func lookup (transfer: TransferHash) -> Transfer? {
        return transfers
            .first  { $0.hash.map { $0 == transfer } ?? false }
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
            guard case let .ethereum (gasPriceBasis, gasLimitBasis) = basis else { precondition(false); return }
            
            var overflow: Int32 = 0
            let gasPrice = coerceUInt64 (gasPriceBasis.value, &overflow)
            
            ewmWalletSetDefaultGasLimit (self.core, self.identifier, gasCreate(gasLimitBasis))
            ewmWalletSetDefaultGasPrice (self.core, self.identifier,
                                         gasPriceCreate(etherCreateNumber(gasPrice, WEI)))
        }
    }
    
    public var transferFactory: TransferFactory = EthereumTransferFactory()
    
    public var target: Address {
        return Ethereum.AddressSchemes.`default`.getAddress(for: self)
    }
    
    internal init (manager:EthereumWalletManager,
                   currency: Currency,
                   wid: BREthereumWallet) {
        self._manager = manager
        self.identifier = wid
        self.currency = currency
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
        return DispatchQueue (label: "\(network.currency.code) WalletManager")

    }()

    public let account: Account
    
    //    public var address: EthereumAddress {
    //        return EthereumAddress ("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62")
    //    }
    
    public var network: Network
    
    public lazy var primaryWallet: Wallet = {
        return EthereumWallet (manager: self,
                               currency: Ethereum.currency,
                               wid: ewmGetWallet(self.core))
    }()
    
    public lazy var wallets: [Wallet] = {
        return [primaryWallet]
    } ()

    internal func addWallet (identifier: BREthereumWallet) {
        guard case .none = findWallet(identifier: identifier) else { return }

        if let tokenId = ewmWalletGetToken (core, identifier) {
            guard let token = findToken (identifier: tokenId) else { precondition(false); return }
            wallets.append (EthereumWallet (manager: self,
                                            currency: token.currency,
                                            wid: identifier))
        }
    }

    internal func findWallet (identifier: BREthereumWallet) -> EthereumWallet? {
        return wallets.first { identifier == ($0 as! EthereumWallet).identifier } as? EthereumWallet
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

    public init (listener: EthereumListener,
                 account: Account,
                 network: Network,
                 mode: WalletManagerMode,
                 timestamp: BREthereumTimestamp,
                 storagePath: String,
                 persistenceClient: EthereumPersistenceClient = DefaultEthereumPersistenceClient(),
                 backendClient: EthereumBackendClient = DefaultEthereumBackendClient()) {

        let coreNetwork: BREthereumNetwork! = network.ethereumCore
        precondition (nil != coreNetwork)

        self.backendClient = backendClient
        self.persistenceClient = persistenceClient
        
        self._listener = listener
        self.account = account
        self.network = network
        self.mode = mode
        self.path = storagePath
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

        EthereumWalletManager.managers.append(Weak (value: self))
        self.listener.handleManagerEvent(manager: self, event: WalletManagerEvent.created)
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
                    let address = asUTF8String(address!)
                    ewm.queue.async {
                        ewm.backendClient.getBalance(ewm: ewm,
                                                     wid: wid!,
                                                     address: address,
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
                    let to = asUTF8String(to!)
                    let amount = asUTF8String(amount!)
                    let data = asUTF8String(data!)
                    ewm.queue.async {
                        ewm.backendClient.getGasEstimate(ewm: ewm,
                                                         wid: wid!,
                                                         tid: tid!,
                                                         to: to,
                                                         amount: amount,
                                                         data: data,
                                                         rid: rid)
                    }
                }},
            
            funcSubmitTransaction: { (coreClient, coreEWM, wid, tid, transaction, rid)  in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    let transaction = asUTF8String (transaction!)
                    ewm.queue.async {
                        ewm.backendClient.submitTransaction(ewm: ewm,
                                                            wid: wid!,
                                                            tid: tid!,
                                                            rawTransaction: transaction,
                                                            rid: rid)
                    }
                }},
            
            funcGetTransactions: { (coreClient, coreEWM, address, rid) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    let address = asUTF8String(address!)
                    ewm.queue.async {
                        ewm.backendClient.getTransactions(ewm: ewm,
                                                          address: address,
                                                          rid: rid)
                    }
                }},
            
            funcGetLogs: { (coreClient, coreEWM, contract, address, event, rid) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    let address = asUTF8String(address!)
                   ewm.queue.async {
                        ewm.backendClient.getLogs (ewm: ewm,
                                                   address: address,
                                                   event: asUTF8String(event!),
                                                   rid: rid)
                    }
                }},
            
            funcGetBlocks: { (coreClient, coreEWM, address, interests, blockStart, blockStop, rid) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    let address = asUTF8String(address!)
                    ewm.queue.async {
                        ewm.backendClient.getBlocks (ewm: ewm,
                                                     address: address,
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
                    let address = asUTF8String(address!)
                    ewm.queue.async {
                        ewm.backendClient.getNonce(ewm: ewm,
                                                   address: address,
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
                    let cStrHash = hashDataPairGetHashAsString (data)!
                    let cStrData = hashDataPairGetDataAsString (data)!

                    ewm.queue.async {
                        ewm.persistenceClient.changeTransaction (manager: ewm,
                                                                 change: WalletManagerPersistenceChangeType(change),
                                                                 hash: String (cString: cStrHash),
                                                                 data: String (cString: cStrData))
                        
                        free (cStrHash); free (cStrData)
                    }
                }},
            
            funcChangeLog: { (coreClient, coreEWM, change, data) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {

                    let cStrHash = hashDataPairGetHashAsString (data)!
                    let cStrData = hashDataPairGetDataAsString (data)!

                   ewm.queue.async {
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
                        let event = WalletEvent (ewm: ewm, wid: wid!, event: event)
                        if case .created = event,
                            case .none = ewm.findWallet(identifier: wid!) {
                            ewm.addWallet (identifier: wid!)
                        }

                        if let wallet = ewm.findWallet (identifier: wid!) {
                            ewm.listener.handleWalletEvent(manager: ewm,
                                                           wallet: wallet,
                                                           event: event)
                        }
                    }
                }},

            funcTokenEvent: { (coreClient, coreEWM, token, event) in
                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        let event = EthereumTokenEvent (event)
                        if case .created = event,
                            case .none = ewm.findToken(identifier: token!) {
                            ewm.addToken (identifier: token!)
                        }

                        if let token = ewm.findToken(identifier: token!) {
                            ewm._listener.handleTokenEvent (manager: ewm,
                                                            token: token,
                                                            event: event)
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
                        if let wallet = ewm.findWallet(identifier: wid!) {
                            let event  = TransferEvent (event)
                            if case .created = event,
                                case .none = wallet.findTransfer(identifier: tid!) {
                                wallet.addTransfer (identifier: tid!)
                            }

                            if let transfer = wallet.findTransfer(identifier: tid!) {
                                ewm.listener.handleTransferEvent(manager: ewm,
                                                                 wallet: wallet,
                                                                 transfer: transfer,
                                                                 event: event)
                            }
                        }
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

    /// MARK: - Ethereum Update Interface

    public func updateTokens () {
        ewmUpdateTokens(core)
    }

    public func updateGasPrice (wallet: EthereumWallet) {
        ewmUpdateGasPrice (core, wallet.identifier)
    }

    public func updateX (wallet: EthereumWallet,
                         transfer: EthereumTransfer) {
        ewmUpdateGasEstimate(core, wallet.identifier, transfer.identifier)
    }

    /// MARK: - Ethereum Backend Announce Interface

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


extension WalletManagerEvent {
    ///
    /// Create a WalletManagerEvent from a BREthereumEWMEvent
    ///
    /// - Parameter core: the core event
    ///
    init (_ core: BREthereumEWMEvent) {
        switch core {
        case EWM_EVENT_CREATED:        self = .created
        case EWM_EVENT_SYNC_STARTED:   self = .syncStarted
        case EWM_EVENT_SYNC_CONTINUES: self = .syncProgress(percentComplete: 50.0)
        case EWM_EVENT_SYNC_STOPPED:   self = .syncEnded(error: nil)
        case EWM_EVENT_NETWORK_UNAVAILABLE: self = .created
        case EWM_EVENT_DELETED:        self = .deleted
        default:
            self = .created
        }
    }
}

extension WalletEvent {
    ///
    /// Create a WalletEvent from a BREthereumWalletEvent
    ///
    /// - Parameter core: the core event
    ///
    init (ewm: EthereumWalletManager,
          wid: BREthereumWallet,
          event: BREthereumWalletEvent) {
        switch event {
        case WALLET_EVENT_CREATED: self = .created
        case WALLET_EVENT_DELETED: self = .deleted
        case WALLET_EVENT_BALANCE_UPDATED:
            let coreBalance = ewmWalletGetBalance(ewm.core, wid)
            var amount: Amount?

            switch amountGetType(coreBalance) {
            case AMOUNT_TOKEN:
                let token: EthereumToken! = ewm.findToken (identifier: coreBalance.u.tokenQuantity.token)
                precondition (nil != token)

                amount = Amount (value: coreBalance.u.tokenQuantity.valueAsInteger,
                                 unit:  token.currency.baseUnit,
                                 negative: false)
            case AMOUNT_ETHER: fallthrough
            default:
                amount = Amount (value: coreBalance.u.ether.valueInWEI,
                                 unit:  Ethereum.currency.baseUnit,
                                 negative: false)
            }

            self = .balanceUpdated (amount: amount!)

        case WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED,
             WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED:
            let gasLimit = ewmWalletGetDefaultGasLimit (ewm.core, wid)
            let gasPrice = ewmWalletGetDefaultGasPrice (ewm.core, wid)
            let feeBasis = TransferFeeBasis.ethereum(
                gasPrice: Amount (value: gasPrice.etherPerGas.valueInWEI, unit: Ethereum.Units.GWEI, negative: false),
                gasLimit: gasLimit.amountOfGas)
            self = .feeBasisUpdated(feeBasis: feeBasis)

        default: self = .created
        }
    }
}

extension TransferEvent {
    ///
    /// Create a TransferEvent from a BREthereumTransferEvent
    ///
    /// - Parameter core: the core event
    ///
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

extension EthereumTokenEvent {
    ///
    /// Create an EthereumTokenEvent from a BREthereumTokenEvent
    ///
    /// - Parameter core: the core event
    ///
    init (_ core: BREthereumTokenEvent) {
        switch core {
        case TOKEN_EVENT_CREATED: self = .created
        case TOKEN_EVENT_DELETED: self = .deleted
        default:
            self = .created
        }
    }
}

extension TransferState {
    init (ewm: BREthereumEWM,
          tid: BREthereumTransfer) {
        switch ewmTransferGetStatus(ewm, tid) {
        case TRANSFER_STATUS_CREATED: self = .created
        case TRANSFER_STATUS_SUBMITTED: self = .submitted

        case TRANSFER_STATUS_INCLUDED:
            var overflow: Int32 = 0
            let fee = ewmTransferGetFee (ewm, tid, &overflow)
            let confirmation =  TransferConfirmation (
                blockNumber: ewmTransferGetBlockNumber (ewm, tid),
                transactionIndex: ewmTransferGetTransactionIndex (ewm, tid),
                timestamp: ewmTransferGetBlockTimestamp (ewm, tid),
                fee: Amount (value: fee.valueInWEI, unit: Ethereum.Units.ETHER, negative: false))

            self = .included(confirmation: confirmation)

        case TRANSFER_STATUS_ERRORED:
            let reasonBytes = ewmTransferStatusGetError(ewm, tid)
            self = .failed (reason: asUTF8String(reasonBytes!))

        case TRANSFER_STATUS_CANCELLED: self = .created
        case TRANSFER_STATUS_REPLACED: self = .created
        case TRANSFER_STATUS_DELETED: self = .deleted
        default:
            self = .created
        }
    }
}

