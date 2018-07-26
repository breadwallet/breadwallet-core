//
//  BREthereum.swift
//  breadwallet
//
//  Created by Ed Gamble on 3/28/18.
//  Copyright © 2018 breadwallet LLC. All rights reserved.
//

import Core.Ethereum

public typealias EthereumReferenceId = Int32
public typealias EthereumWalletId = EthereumReferenceId
public typealias EthereumTransactionId = EthereumReferenceId
public typealias EthereumAccountId = EthereumReferenceId
public typealias EthereumAddressId = EthereumReferenceId
public typealias EthereumBlockId = EthereumReferenceId
public typealias EthereumListenerId = EthereumReferenceId

// Access to BRCore/BREthereum types
public typealias BRCoreEWM = OpaquePointer

// MARK: Reference

///
/// Core Ethereum *does not* allow direct access to Core 'C' Memory; instead we use references
/// within an `EthereumEWM`.  This allows the Core to avoid multiprocessing issues arising
/// from Client access in arbitrary threads.  Additionally, the Core is free to manage its own
/// memory w/o regard to a Client holding a reference (that the Core can never, even know about).
///
/// Attemping to access a Core reference that no longer exists in the Core results in an error.
/// But how so is TBD.
///
/// An EthereumReference is Equatable based on the `EthereumEWM` and the identifier (of the
/// reference).  Generally adopters of `EthereumReference` will be structures as all their
/// properties are computed via a C function on `EthereumEWM`.
///
public protocol EthereumReference : Equatable  {
    associatedtype ReferenceType : Equatable

    var ewm : EthereumWalletManager? { get }

    var identifier : ReferenceType { get }
}

extension EthereumReference {
    static public func == (lhs: Self, rhs: Self) -> Bool {
        return rhs.ewm === lhs.ewm && rhs.identifier == lhs.identifier
    }
}

///
/// An `EthereumReferenceWithDefaultUnit` is an `EthereumReference` with a default unit.  The
/// instances for `EthereumWallet` and `EthereumTransaction` use a `unit` when computing the
/// string representation for the balance nad amount, respectively.
///
public protocol EthereumReferenceWithDefaultUnit : EthereumReference {
    var unit : EthereumAmountUnit { get }
}

// MARK: - Pointer

///
/// An `EthereumPointer` holds an `OpaquePointer` to Ethereum Core memory.  This is used for
/// 'constant-like' memory references.
///
protocol EthereumPointer : Equatable {
    associatedtype PointerType : Equatable

    var core : PointerType { get }
}

extension EthereumPointer {
    static public func == (lhs: Self, rhs: Self) -> Bool {
        return lhs.core == rhs.core
    }
}

// MARK: - Account

///
/// An `EthereumAccount` is an `EthereumReference` for the User's account - with address,
/// publicKey and PrivateKey.
///
public struct EthereumAccount : EthereumReference {
    public weak var ewm : EthereumWalletManager?
    public let identifier : EthereumAccountId

    init (ewm: EthereumWalletManager, identifier: EthereumAccountId) {
        self.ewm = ewm
        self.identifier = identifier
    }

    static public func == (lhs: EthereumAccount, rhs: EthereumAccount) -> Bool {
        return lhs.ewm === rhs.ewm && lhs.identifier == rhs.identifier
    }

    var address : String {
        let cString = ethereumGetAccountPrimaryAddress (self.ewm!.core)
        let string = String (cString: cString!)
        free (cString)
        return string
    }

    // public key
    var addressPublicKey : BRKey {
        return ethereumGetAccountPrimaryAddressPublicKey (self.ewm!.core)
    }

    func addresssPrivateKey (paperKey: String) -> BRKey {
        return ethereumGetAccountPrimaryAddressPrivateKey (self.ewm!.core, paperKey)
    }
}

// MARK: - Network

///
/// An `EthereumNetwork` represents one of a handful of Ethereum (Blockchain) Networks such as:
/// mainnet, testnet/ropsten, rinkeby
///
public struct EthereumNetwork : EthereumPointer {
    let core : BREthereumNetwork

    private init (core: BREthereumNetwork) {
        self.core = core;
    }

    var chainId : Int {
        return Int(exactly: networkGetChainId (core))!
    }

    static public let mainnet = EthereumNetwork (core: ethereumMainnet)
    static public let testnet = EthereumNetwork (core: ethereumTestnet)
    static public let rinkeby = EthereumNetwork (core: ethereumRinkeby)
}

// MARK: - Token

///
/// An `Ethereum`Token` is a defined instantiation of an ERC20 Contract.
///
public struct EthereumToken : EthereumPointer {
    public let core : BREthereumToken

    init (core: BREthereumToken) {
        self.core = core
    }

    var address : String {
        return String (cString: tokenGetAddress (core))
    }

    var symbol : String {
        return String (cString: tokenGetSymbol (core))
    }

    var name : String {
        return String (cString: tokenGetName (core))
    }

    var description : String {
        return String (cString: tokenGetDescription (core))
    }

    var decimals : Int {
        return Int (exactly: tokenGetDecimals (core))!
    }

    var colorLeft : String {
        return String (cString: tokenGetColorLeft (core))
    }

    var colorRight : String {
        return String (cString: tokenGetColorRight (core))
    }

    static let BRD = EthereumToken(core: tokenBRD)

    static internal func lookup (core: BREthereumToken) -> EthereumToken {
        if core == BRD.core { return BRD }
        else { return EthereumToken (core: core) }
    }

    static func lookup (contract: String) -> EthereumToken? {
        return all.first {
            switch contract.caseInsensitiveCompare($0.address) {
            case .orderedSame: return true
            default: return false
            }
        }
    }

    static var all : [EthereumToken] = {
        let tokenIndex = Int(exactly: tokenCount())! - 1
        return (0...tokenIndex)
            .map { tokenGet (Int32($0)) }
            .map { EthereumToken (core: $0) }
    }()
}

// MARK: - Wallet

///
/// An `EthereumWallet` holds a balance with ETHER or a TOKEN and is associated with a Network
/// and an Account on that network.  An `EthereumWallet` as a default unit.
///
public struct EthereumWallet : EthereumReferenceWithDefaultUnit {
    public weak var ewm : EthereumWalletManager?
    public let identifier : EthereumWalletId
    public let unit : EthereumAmountUnit

    let account : EthereumAccount
    let network : EthereumNetwork
    let token   : EthereumToken?

    //
    // Gas Limit (in 'gas')
    //
    var defaultGasLimit : UInt64 {
        get {
            return ethereumWalletGetDefaultGasLimit (self.ewm!.core, self.identifier)
        }
        set (value) {
            ethereumWalletSetDefaultGasLimit (self.ewm!.core, self.identifier, value)
        }
    }

    func gasEstimate (transaction: EthereumTransaction) -> UInt64 {
        return ethereumWalletGetGasEstimate (self.ewm!.core, self.identifier, transaction.identifier)
    }

    //
    // Gas Price (ETHER in WEI)
    //
    static let maximumGasPrice : UInt64 = 100000000000000

    var defaultGasPrice : UInt64 {
        get {
            return ethereumWalletGetDefaultGasPrice (self.ewm!.core, self.identifier)
        }
        set (value) {
            precondition(value <= EthereumWallet.maximumGasPrice)
            ethereumWalletSetDefaultGasPrice (self.ewm!.core, self.identifier, WEI, value)
        }
    }

    //
    // Balance
    //
    var balance : EthereumAmount {
        let amount : BREthereumAmount = ethereumWalletGetBalance (self.ewm!.core, self.identifier)
        return (AMOUNT_ETHER == amount.type
            ? EthereumAmount.ether(amount.u.ether.valueInWEI, unit.coreForEther)
            : EthereumAmount.token (amount.u.tokenQuantity.valueAsInteger,
                                    self.token!,
                                    unit.coreForToken))
    }

    //
    // Constructors
    //
    internal init (ewm : EthereumWalletManager,
                   wid : EthereumWalletId) {
        self.ewm = ewm
        self.identifier = wid
        self.unit = EthereumAmountUnit.defaultUnit (true)
        self.account = ewm.account
        self.network = ewm.network
        self.token = nil
    }

    internal init (ewm : EthereumWalletManager,
                   wid : EthereumWalletId,
                   token : EthereumToken) {
        self.ewm = ewm
        self.identifier = wid
        self.unit = EthereumAmountUnit.defaultUnit (false)
        self.account =  ewm.account
        self.network = ewm.network
        self.token = token
    }

    //
    // Transaction
    //
    func createTransaction (recvAddress: String, amount: String, unit: EthereumAmountUnit) -> EthereumTransaction {
        var status : BRCoreParseStatus = CORE_PARSE_OK
        let amount = (unit.isEther
            ? ethereumCreateEtherAmountString (self.ewm!.core, amount, unit.coreForEther, &status)
            : ethereumCreateTokenAmountString (self.ewm!.core, token!.core, amount, unit.coreForToken, &status))
        // Sure, ignore `status`

        let tid = ethereumWalletCreateTransaction (self.ewm!.core,
                                                   self.identifier,
                                                   recvAddress,
                                                   amount)
        return EthereumTransaction (ewm: self.ewm!, identifier: tid)
    }


    func sign (transaction : EthereumTransaction, paperKey : String) {
        ethereumWalletSignTransaction (self.ewm!.core, self.identifier, transaction.identifier, paperKey)
    }

    func sign (transaction: EthereumTransaction, privateKey: BRKey) {
        ethereumWalletSignTransactionWithPrivateKey (self.ewm!.core, self.identifier, transaction.identifier, privateKey)
    }

    func submit (transaction : EthereumTransaction) {
        ethereumWalletSubmitTransaction (self.ewm!.core, self.identifier, transaction.identifier)
    }

    var transactions : [EthereumTransaction] {
        let count = ethereumWalletGetTransactionCount (self.ewm!.core, self.identifier)
        let identifiers = ethereumWalletGetTransactions (self.ewm!.core, self.identifier)
        return UnsafeBufferPointer (start: identifiers, count: Int(exactly: count)!)
            .map { self.ewm!.findTransaction(identifier: $0) }
    }

    var transactionsCount : Int {
        return Int (exactly: ethereumWalletGetTransactionCount (self.ewm!.core, self.identifier))!
    }
}

// MAKR: - Block

///
/// An `EthereumBlock` represents a  ...
//
public struct EthereumBlock : EthereumReference {
    public weak var ewm : EthereumWalletManager?
    public let identifier : EthereumWalletId

    init (ewm: EthereumWalletManager, identifier: EthereumAccountId) {
        self.ewm = ewm
        self.identifier = identifier
    }

    var number : UInt64 {
        return ethereumBlockGetNumber (self.ewm!.core, identifier)
    }

    var timestamp : UInt64 {
        return ethereumBlockGetTimestamp (self.ewm!.core, identifier)
    }

    var hash : String {
        return asUTF8String(ethereumBlockGetHash (self.ewm!.core, identifier))
    }
}

// MARK: - Transaction

///
/// An `EthereumTransaction` represents a transfer of ETHER or a specific TOKEN between two
/// accounts.
///
public struct EthereumTransaction : EthereumReferenceWithDefaultUnit {
    public weak var ewm : EthereumWalletManager?
    public let identifier : EthereumWalletId
    public let unit : EthereumAmountUnit

    internal init (ewm : EthereumWalletManager, identifier : EthereumTransactionId) {
        self.init (ewm: ewm,
                   identifier: identifier,
                   unit: EthereumAmountUnit.defaultUnit(
                    nil == ethereumTransactionGetToken (ewm.core, identifier)))
    }

    internal init (ewm : EthereumWalletManager, identifier : EthereumTransactionId, unit: EthereumAmountUnit) {
        self.ewm = ewm
        self.identifier = identifier
        self.unit = unit
    }

    var hash : String {
        return asUTF8String (ethereumTransactionGetHash (self.ewm!.core, self.identifier), true)
    }

    var sourceAddress : String {
        return asUTF8String (ethereumTransactionGetSendAddress (self.ewm!.core, self.identifier), true)
    }

    var targetAddress : String {
        return asUTF8String (ethereumTransactionGetRecvAddress (self.ewm!.core, self.identifier), true)
    }

    var amount : EthereumAmount {
        let amount : BREthereumAmount = ethereumTransactionGetAmount (self.ewm!.core, self.identifier)
        return (AMOUNT_ETHER == amount.type
            ? EthereumAmount.ether(amount.u.ether.valueInWEI, unit.coreForEther)
            : EthereumAmount.token (amount.u.tokenQuantity.valueAsInteger,
                                    EthereumToken.lookup (core: amount.u.tokenQuantity.token),
                                    unit.coreForToken))
    }

    var gasPrice : EthereumAmount {
        let price : BREthereumAmount = ethereumTransactionGetGasPriceToo (self.ewm!.core, self.identifier)
        return EthereumAmount.ether (price.u.ether.valueInWEI, WEI)
    }

    var gasLimit : UInt64 {
        return ethereumTransactionGetGasLimit(self.ewm!.core, self.identifier)
    }

    var gasUsed : UInt64 {
        return ethereumTransactionGetGasUsed (self.ewm!.core, self.identifier)
    }

    var nonce : UInt64 {
        return ethereumTransactionGetNonce (self.ewm!.core, self.identifier)
    }

    var blockNumber : UInt64 {
        return ethereumTransactionGetBlockNumber (self.ewm!.core, self.identifier)
    }

    // State
}

// MARK: - Client

public enum EthereumStatus {
    case success
}

public enum EthereumWalletEvent : Int {
    case created
    case balanceUpdated
    case defaultGasLimitUpdated
    case defaultGasPriceUpdated
    case deleted
}

public enum EthereumBlockEvent : Int {
    case created
    case chained
    case orphaned
    case deleted
}

public enum EthereumTransactionEvent : Int {
    case created
    case signed
    case submitted
    case blocked
    case errored

    case gasEstimateUpdated
    case blockConfirmationsUpdated

    case deleted
}

public enum EthereumPeerEvent : Int {
    case created
    case deleted

    init (_ event: BREthereumPeerEvent) {
        self.init(rawValue: Int(event.rawValue))!
    }
}

public enum EthereumEWMEvent : Int {
    case created
    case sync_started
    case sync_continues
    case sync_stopped
    case network_unavailable
    case deleted

    init (_ event: BREthereumEWMEvent) {
        self.init(rawValue: Int(event.rawValue))!
    }
}

///
/// An `EthereumClient` is a protocol defined with a set of functions that support an
/// EthereumEWM.
///
public protocol EthereumClient : class {
    func getGasPrice (ewm: EthereumWalletManager, wid: EthereumWalletId, rid: Int32) -> Void
    func getGasEstimate (wid: EthereumWalletId,
                         tid: EthereumTransactionId,
                         to: String,
                         amount: String,
                         data:  String,
                         rid: Int32) -> Void

    func getBalance (wid: EthereumWalletId, address: String, rid: Int32) -> Void
    // ewm.announceBalance (...)

    func submitTransaction (wid: EthereumWalletId,
                            tid: EthereumTransactionId,
                            rawTransaction: String,
                            rid: Int32) -> Void
    // ...
    func getTransactions (address: String, rid: Int32) -> Void

    func getLogs (address: String, event: String, rid: Int32) -> Void

    // ...
    // save

    // ...
    func handleWalletEvent (wallet: EthereumWallet,
                            event: EthereumWalletEvent) -> Void

    func handleBlockEvent (block: EthereumBlock,
                           event: EthereumBlockEvent) -> Void

    func handleTransactionEvent (wallet: EthereumWallet,
                                 transaction: EthereumTransaction,
                                 event: EthereumTransactionEvent) -> Void

    func handlePeerEvent (event : EthereumPeerEvent) -> Void
    func handleEWMEvent (event: EthereumEWMEvent) -> Void
}

// MARK: - EWM

///
/// An `EthereumEWM` is a SPV/LES (Simplified Payment Verification / Light Ethereum
/// Subprotocol) ewm in an Ethereum Network.
///
public class EthereumWalletManager {

    ///
    /// The OpaquePointer to the 'Core Ethereum EWM'.  We defer nearly all functions
    /// to this reference.
    ///
    let core : BRCoreEWM //  OpaquePointer // BREthereumEWM

    ///
    /// The client ...
    ///
    weak private(set) var client : EthereumClient?

    ///
    /// The network ...
    ///
    public let network : EthereumNetwork

    ///
    /// The account ....
    ///
    public lazy private(set) var account : EthereumAccount = {
        return EthereumAccount (ewm: self,
                                identifier: ethereumGetAccount (self.core))
    }()

    private init (core: BRCoreEWM,
                  client : EthereumClient,
                  network : EthereumNetwork) {
        self.core = core
        self.client = client
        self.network = network
//        self.coreClient = createCoreClient ()
    }

    public convenience init (client : EthereumClient,
          network : EthereumNetwork,
          paperKey : String) {

        self.init (core: ethereumCreate (network.core, paperKey, NODE_TYPE_LES, SYNC_MODE_FULL_BLOCKCHAIN,
                                         EthereumWalletManager.createCoreClient(client: client),
                                         nil,
                                         nil,
                                         nil,
                                         nil),
                    client: client,
                    network: network)
    }

    public convenience init (client : EthereumClient,
          network : EthereumNetwork,
          publicKey : BRKey) {
        self.init (core: ethereumCreateWithPublicKey (network.core, publicKey, NODE_TYPE_LES, SYNC_MODE_FULL_BLOCKCHAIN,
                                                      EthereumWalletManager.createCoreClient(client: client),
                                                      nil,
                                                      nil,
                                                      nil,
                                                      nil),
                    client: client,
                    network: network)
    }

    //
    // Wallets
    //
    func getWallet () -> EthereumWallet {
        return findWallet (identifier: ethereumGetWallet (core))
    }

    func getWallet (token: EthereumToken) -> EthereumWallet {
        return findWallet (identifier: ethereumGetWalletHoldingToken (core, token.core))
    }

    internal func findWallet (identifier: EthereumWalletId) -> EthereumWallet {
        let token = ethereumWalletGetToken (core, identifier)
        return (nil == token
            ? EthereumWallet (ewm: self, wid: identifier)
            : EthereumWallet (ewm: self, wid: identifier, token: EthereumToken.lookup (core: token!)))
    }

    //
    // Block
    //
    internal func findBlock (identifier: EthereumBlockId) -> EthereumBlock {
        return EthereumBlock (ewm: self, identifier: identifier);
    }

    var blockHeight : UInt64 {
        return ethereumGetBlockHeight (core)
    }
    
    //
    // Transactions
    //
    internal func findTransaction (identifier: EthereumTransactionId) -> EthereumTransaction {
        return EthereumTransaction (ewm: self, identifier: identifier)
    }

    //
    // Connect / Disconnect
    //
    var coreClient : BREthereumClient?

    public func connect () {
            ethereumConnect (self.core);
     }

    public func disconnect () {
        ethereumDisconnect (self.core)
    }

    public var address : String {
        return account.address
    }

    public func announceBalance (wid: EthereumWalletId, balance: String, rid: Int32) {
        ethereumClientAnnounceBalance (core, wid, balance, rid)
    }

    public func announceGasPrice (wid: EthereumWalletId, gasPrice: String, rid: Int32) {
        ethereumClientAnnounceGasPrice (core, wid, gasPrice, rid)
    }

    public func announceGasEstimate (wid: EthereumWalletId, tid: EthereumTransactionId, gasEstimate: String, rid: Int32) {
        ethereumClientAnnounceGasEstimate (core, wid, tid, gasEstimate, rid)
    }

    public func announceSubmitTransaction (wid: EthereumWalletId, tid: EthereumTransactionId, hash: String, rid: Int32) {
        ethereumClientAnnounceSubmitTransaction (core, wid, tid, hash, rid)
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
        ethereumClientAnnounceTransaction (core, rid,
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

        ethereumClientAnnounceLog (core, rid,
                              hash, contract, Int32(topics.count), &cTopics,
                              data, gasPrice, gasUsed,
                              logIndex,
                              blockNumber, blockTransactionIndex, blockTimestamp)
        cTopics.forEach { free (UnsafeMutablePointer(mutating: $0)) }
    }

    static func lookup (core: BREthereumEWM?) -> EthereumWalletManager? {
        return nil
    }
    ///
    /// Create an BREthereumEWMConfiguration for a JSON_RPC client.  The configuration
    /// will invoke Client functions for EWM callbacks, implementing, for example,
    /// getTransactions().  In this case, the client is expected to make a JSON_RPC call
    /// returning a list of JSON transactions and then to processing each transaction by
    /// calling announceTransaction().
    ///
    static func createCoreClient (client: EthereumClient) -> BREthereumClient {
        let client = AnyEthereumClient (base: client)
        return BREthereumClient (context: UnsafeMutableRawPointer (Unmanaged<AnyEthereumClient>.passUnretained(client).toOpaque()),

                                 funcGetBalance: { (this, core, wid, address, rid) in
                                    if let this = this.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }) {
                                        this.getBalance(wid: wid, address: asUTF8String(address!), rid: rid)
                                    }},

                                 funcGetGasPrice: { (this, core, wid, rid) in
                                    if let this = this.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                                        let ewm = EthereumWalletManager.lookup(core: core) {
                                        assert (ewm.client === this.base)
                                        this.getGasPrice (ewm: ewm, wid: wid, rid: rid)
                                    }},

                                 funcEstimateGas: { (this, core, wid, tid, to, amount, data, rid)  in
                                    if let this = this.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }) {
                                        this.getGasEstimate(wid: wid, tid: tid,
                                                                    to: asUTF8String(to!),
                                                                    amount: asUTF8String(amount!),
                                                                    data: asUTF8String(data!),
                                                                    rid: rid)
                                    }},

                                 funcSubmitTransaction: { (this, core, wid, tid, transaction, rid)  in
                                    if let this = this.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }) {
                                        this.submitTransaction(wid: wid, tid: tid,
                                                                       rawTransaction: asUTF8String(transaction!),
                                                                       rid: rid)
                                    }},

                                 funcGetTransactions: { (this, core, address, rid) in
                                    if let this = this.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }) {
                                        this.getTransactions(address: asUTF8String(address!), rid: rid)
                                    }},

                                 funcGetLogs: { (this, core, contract, address, event, rid) in
                                    if let this = this.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }) {
                                        this.getLogs (address: asUTF8String(address!),
                                                              event: asUTF8String(event!),
                                                              rid: rid)
                                    }},

                                 funcGetBlockNumber: { (this, ewm, rid) in

        },
                                 funcGetNonce: { (client, ewm, nonce, rid) in

        },
                                 funcSavePeers: { (client, ewm, data) in

        },
                                 funcSaveBlocks: { (client, ewm, data) in

        },
                                 funcChangeTransaction: { (client, ewm, change, data) in

        },
                                 funcChangeLog: { (client, ewm, change, data) in

        },
                                 funcEWMEvent: { (this, core, event, status, message) in
                                    if let this = this.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeRetainedValue() }) {
                                        this.handleEWMEvent (event: EthereumEWMEvent (event))
                                    }
        },
                                 funcPeerEvent: { (this, core, event, status, message) in
                                    if let this = this.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeRetainedValue() }) {
                                        this.handlePeerEvent (event: EthereumPeerEvent (event))
                                    }
        },
                                 funcWalletEvent: { (this, core, wid, event, status, message) in
                                    if let this = this.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeRetainedValue() }) {
//                                        this.handleWalletEvent (wallet: this.findWallet(identifier: wid),
//                                                                          event: EthereumWalletEvent (rawValue: Int(event.rawValue))!)

                                    }},
                                    funcBlockEvent: { (this, core, bid, event, status, message) in
                                        if let this = this.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeRetainedValue() }) {
//                                            this.handleBlockEvent (block: this.findBlock (identifier: bid),
//                                                                             event: EthereumBlockEvent (rawValue: Int(event.rawValue))!);
                                        }
                                        return },
                                    funcTransactionEvent: { (this, core, wid, tid, event, status, message) in
                                        if let this = this.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeRetainedValue() }) {
//                                            this.handleTransactionEvent (wallet: this.findWallet(identifier: wid),
//                                                                                   transaction: this.findTransaction(identifier: tid),
//                                                                                   event: EthereumTransactionEvent(rawValue: Int(event.rawValue))!)
                                        }
                                        return })
    }

    //
    // Nodes
    //

    static var ewms : [EthereumWalletManager] = []

    static func addNode (_ ewm: EthereumWalletManager) {
        ewms.append(ewm)
    }

    static func lookupNode (core: BRCoreEWM) -> EthereumWalletManager? {
        return ewms.first { $0.core == core }
    }
}

// MARK: - Amount
///
/// An Amount Unit (currently of Ethereeum currency units).
///
public enum EthereumAmountUnit {
    case ether (BREthereumEtherUnit)
    case token (BREthereumTokenQuantityUnit)
    // bitcoin
    // bitcash
    // fiat

    var isEther : Bool {
        switch (self) {
        case .ether: return true
        default: return false
        }
    }

    internal var coreForEther : BREthereumEtherUnit {
        switch (self) {
        case let .ether (u): return u
        case .token: return WEI // error
        }
    }

    internal var coreForToken : BREthereumTokenQuantityUnit {
        switch (self) {
        case .ether: return TOKEN_QUANTITY_TYPE_DECIMAL // error
        case let .token (u): return u
        }
    }

    static let defaultUnitEther = EthereumAmountUnit.ether (ETHER)
    static let defaultUnitToken = EthereumAmountUnit.token (TOKEN_QUANTITY_TYPE_DECIMAL)

    static func defaultUnit (_ forEther : Bool) -> EthereumAmountUnit {
        return forEther ? defaultUnitEther : defaultUnitToken
    }
}

///
/// An Amount (currently of Ethereum currencies - either ETHER or any TOKEN)
///
public enum EthereumAmount {
    ///
    /// An Ether Amount - with the raw `value` and the default `unit` for display
    ///
    case ether (UInt256, BREthereumEtherUnit)

    ///
    /// A Token Amount - with the raw `value`, the `token`, and the default `unit` for display
    ///
    case token (UInt256, EthereumToken, BREthereumTokenQuantityUnit)
    // bitcoin: BTC, SATOSHI
    // bitcash: BCH, SATOSHI
    // fiat: 'currency code'

    ///
    /// Danger, Danger - 'no naked numbers'
    ///
    var value : UInt256 {
        switch (self) {
        case let .ether (v, _): return v
        case let .token (v, _, _): return v
        }
    }

    var token : EthereumToken? {
        switch (self) {
        case .ether: return nil
        case let .token (_, t, _): return t;
        }
    }

    var isEther : Bool {
        switch (self) {
        case .ether: return true
        default: return false
        }
    }

    var isToken : Bool {
        switch (self) {
        case .token: return true
        default: return false
        }
    }

    var amount : String {
        switch (self) {
        case let .ether (_, unit): return getAmountAsEther (unit: unit)
        case let .token (_, _, unit): return getAmountAsToken (unit: unit)
        }
    }

    func getAmount (unit: EthereumAmountUnit) -> String? {
        switch (self) {
        case .ether:
            if (!unit.isEther) { return nil}
            else {
                return getAmountAsEther (unit: unit.coreForEther)
            }

        case .token:
            if (unit.isEther) { return nil }
            else {
                return getAmountAsToken (unit: unit.coreForToken)
            }
        }
    }

    internal func getAmountAsEther (unit: BREthereumEtherUnit) -> String {
        switch (self) {
        case let .ether (v, u):
            guard let strEther = etherGetValueString (etherCreate (v), u) else {
                return "" // error
            }
            let result = String (cString: strEther)
            free (strEther)
            return result
        case .token:
            return "" // error
        }
    }

    internal func getAmountAsToken (unit: BREthereumTokenQuantityUnit) -> String {
        switch (self) {
        case .ether:
            return "" // error
        case let .token (v, t, u):
            guard let strToken = tokenQuantityGetValueString (createTokenQuantity (t.core, v), u) else {
                return "" // error
            }
            let result = String (cString: strToken)
            free (strToken)
            return result
        }
    }
}

// MARK: - Any Client
//
// Concretize protocol EthereumClient
//
    class AnyEthereumClient : EthereumClient {
        let base : EthereumClient

        init (base: EthereumClient) {
            self.base = base
        }

        func getGasPrice(ewm: EthereumWalletManager, wid: EthereumWalletId, rid: Int32) {
            base.getGasPrice(ewm: ewm, wid: wid, rid: rid)
        }

        func getGasEstimate(wid: EthereumWalletId, tid: EthereumTransactionId, to: String, amount: String, data: String, rid: Int32) {
            base.getGasEstimate(wid: wid, tid: tid, to: to, amount: amount, data: data, rid: rid)
        }

        func getBalance(wid: EthereumWalletId, address: String, rid: Int32) {
            base.getBalance(wid: wid, address: address, rid: rid)
        }

        func submitTransaction(wid: EthereumWalletId, tid: EthereumTransactionId, rawTransaction: String, rid: Int32) {
            base.submitTransaction(wid: wid, tid: tid, rawTransaction: rawTransaction, rid: rid)
        }

        func getTransactions(address: String, rid: Int32) {
            base.getTransactions(address: address, rid: rid)
        }

        func getLogs(address: String, event: String, rid: Int32) {
            base.getLogs(address: address, event: event, rid: rid)
        }

        func handleWalletEvent(wallet: EthereumWallet, event: EthereumWalletEvent) {
            base.handleWalletEvent(wallet: wallet, event: event)
        }

        func handleBlockEvent(block: EthereumBlock, event: EthereumBlockEvent) {
            base.handleBlockEvent(block: block, event: event)
        }

        func handleTransactionEvent(wallet: EthereumWallet, transaction: EthereumTransaction, event: EthereumTransactionEvent) {
            base.handleTransactionEvent(wallet: wallet, transaction: transaction, event: event)
        }

        func handlePeerEvent(event: EthereumPeerEvent) {
            base.handlePeerEvent(event: event)
        }

        func handleEWMEvent(event: EthereumEWMEvent) {
            base.handleEWMEvent(event: event)
        }
    }


    // MARK: - Support
//
// Helpers
//

private func asUTF8String (_ chars: UnsafeMutablePointer<CChar>, _ release : Bool = false ) -> String {
    let result = String (cString: chars, encoding: .utf8)!
    if (release) { free (chars) }
    return result
}

private func asUTF8String (_ chars: UnsafePointer<CChar>) -> String {
    return String (cString: chars, encoding: .utf8)!
}

struct Weak<T:AnyObject> {
    weak var value : T?
    init (value: T) {
        self.value = value
    }
}
