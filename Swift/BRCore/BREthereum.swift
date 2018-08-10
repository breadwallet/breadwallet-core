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
public typealias EthereumTransferId = EthereumReferenceId
public typealias EthereumAccountId = EthereumReferenceId
public typealias EthereumAddressId = EthereumReferenceId
public typealias EthereumBlockId = EthereumReferenceId
public typealias EthereumListenerId = EthereumReferenceId

// Access to BRCore/BREthereum types
public typealias BRCoreEWM = BREthereumEWM // OpaquePointer

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

    func gasEstimate (transfer: EthereumTransfer) -> UInt64 {
        return ethereumWalletGetGasEstimate (self.ewm!.core, self.identifier, transfer.identifier)
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
    func createTransaction (recvAddress: String, amount: String, unit: EthereumAmountUnit) -> EthereumTransfer {
        var status : BRCoreParseStatus = CORE_PARSE_OK
        let amount = (unit.isEther
            ? ethereumCreateEtherAmountString (self.ewm!.core, amount, unit.coreForEther, &status)
            : ethereumCreateTokenAmountString (self.ewm!.core, token!.core, amount, unit.coreForToken, &status))
        // Sure, ignore `status`

        let tid = ethereumWalletCreateTransfer (self.ewm!.core,
                                                   self.identifier,
                                                   recvAddress,
                                                   amount)
        return EthereumTransfer (ewm: self.ewm!, identifier: tid)
    }


    func sign (transfer : EthereumTransfer, paperKey : String) {
        ethereumWalletSignTransfer (self.ewm!.core, self.identifier, transfer.identifier, paperKey)
    }

    func sign (transfer: EthereumTransfer, privateKey: BRKey) {
        ethereumWalletSignTransferWithPrivateKey (self.ewm!.core, self.identifier, transfer.identifier, privateKey)
    }

    func submit (transfer : EthereumTransfer) {
        ethereumWalletSubmitTransfer (self.ewm!.core, self.identifier, transfer.identifier)
    }

    public var transfers : [EthereumTransfer] {
        let count = ethereumWalletGetTransferCount (self.ewm!.core, self.identifier)
        let identifiers = ethereumWalletGetTransfers (self.ewm!.core, self.identifier)
        return UnsafeBufferPointer (start: identifiers, count: Int(exactly: count)!)
            .map { self.ewm!.findTransfers(identifier: $0) }
    }

    public var transfersCount : Int {
        return Int (exactly: ethereumWalletGetTransferCount (self.ewm!.core, self.identifier))!
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
public struct EthereumTransfer : EthereumReferenceWithDefaultUnit {
    public weak var ewm : EthereumWalletManager?
    public let identifier : EthereumWalletId
    public let unit : EthereumAmountUnit

    internal init (ewm : EthereumWalletManager, identifier : EthereumTransferId) {
        self.init (ewm: ewm,
                   identifier: identifier,
                   unit: EthereumAmountUnit.defaultUnit(
                    nil == ethereumTransferGetToken (ewm.core, identifier)))
    }

    internal init (ewm : EthereumWalletManager, identifier : EthereumTransferId, unit: EthereumAmountUnit) {
        self.ewm = ewm
        self.identifier = identifier
        self.unit = unit
    }

    var hash : String {
        return asUTF8String (ethereumTransferGetHash (self.ewm!.core, self.identifier), true)
    }

    var sourceAddress : String {
        return asUTF8String (ethereumTransferGetSendAddress (self.ewm!.core, self.identifier), true)
    }

    var targetAddress : String {
        return asUTF8String (ethereumTransferGetRecvAddress (self.ewm!.core, self.identifier), true)
    }

    var amount : EthereumAmount {
        let amount : BREthereumAmount = ethereumTransferGetAmount (self.ewm!.core, self.identifier)
        return (AMOUNT_ETHER == amount.type
            ? EthereumAmount.ether(amount.u.ether.valueInWEI, unit.coreForEther)
            : EthereumAmount.token (amount.u.tokenQuantity.valueAsInteger,
                                    EthereumToken.lookup (core: amount.u.tokenQuantity.token),
                                    unit.coreForToken))
    }

//    var gasPrice : EthereumAmount {
//        let price : BREthereumAmount = ethereumTransferGetGasPriceToo (self.ewm!.core, self.identifier)
//        return EthereumAmount.ether (price.u.ether.valueInWEI, WEI)
//    }
//
//    var gasLimit : UInt64 {
//        return ethereumTransactionGetGasLimit(self.ewm!.core, self.identifier)
//    }
//
//    var gasUsed : UInt64 {
//        return ethereumTransactionGetGasUsed (self.ewm!.core, self.identifier)
//    }
//
//    var nonce : UInt64 {
//        return ethereumTransactionGetNonce (self.ewm!.core, self.identifier)
//    }
//
//    var blockNumber : UInt64 {
//        return ethereumTransactionGetBlockNumber (self.ewm!.core, self.identifier)
//    }

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

    init (_ event: BREthereumWalletEvent) {
        self.init (rawValue: Int (event.rawValue))!
    }
}

public enum EthereumBlockEvent : Int {
    case created
    case chained
    case orphaned
    case deleted
    init (_ event: BREthereumBlockEvent) {
        self.init (rawValue: Int (event.rawValue))!
    }
}

public enum EthereumTransferEvent : Int {
    case created
    case signed
    case submitted
    case blocked
    case errored

    case gasEstimateUpdated
    case blockConfirmationsUpdated

    case deleted

    init (_ event: BREthereumTransferEvent) {
        self.init (rawValue: Int (event.rawValue))!
    }
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
        switch (event) {
        case EWM_EVENT_CREATED: self = .created
        case EWM_EVENT_SYNC_STARTED: self = .sync_started
        case EWM_EVENT_SYNC_CONTINUES: self = .sync_continues
        case EWM_EVENT_SYNC_STOPPED: self = .sync_stopped
        case EWM_EVENT_NETWORK_UNAVAILABLE: self = .network_unavailable
        case EWM_EVENT_DELETED: self = .deleted
        default:
            assert(false, "Uknown BREthereumEWMEvent: \(event)")
            self = .deleted
        }
    }
}

public enum EthereumClientChangeType {
    case added
    case updated
    case deleted

    init (_ event: BREthereumClientChangeType) {
        switch (event) {
        case CLIENT_CHANGE_ADD: self = .added
        case CLIENT_CHANGE_UPD: self = .updated
        case CLIENT_CHANGE_REM: self = .deleted
        default: self = .added
        }
    }
}
///
/// An `EthereumClient` is a protocol defined with a set of functions that support an
/// EthereumEWM.
///
public protocol EthereumClient : class {
    func getGasPrice (ewm: EthereumWalletManager,
                      wid: EthereumWalletId,
                      rid: Int32) -> Void

    func getGasEstimate (ewm: EthereumWalletManager,
                         wid: EthereumWalletId,
                         tid: EthereumTransferId,
                         to: String,
                         amount: String,
                         data:  String,
                         rid: Int32) -> Void

    func getBalance (ewm: EthereumWalletManager,
                     wid: EthereumWalletId,
                     address: String,
                     rid: Int32) -> Void
    // ewm.announceBalance (...)

    func submitTransaction (ewm: EthereumWalletManager,
                            wid: EthereumWalletId,
                            tid: EthereumTransferId,
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

    func getTokens (ewm: EthereumWalletManager,
                    rid: Int32) -> Void

    func getBlockNumber (ewm: EthereumWalletManager,
                         rid: Int32) -> Void

    func getNonce (ewm: EthereumWalletManager,
                   address: String,
                   rid: Int32) -> Void

    func savePeers (ewm: EthereumWalletManager /* data */) -> Void

    func saveBlocks (ewm: EthereumWalletManager /* data */) -> Void

    func changeTransaction (ewm: EthereumWalletManager,
                            change: EthereumClientChangeType /* data */) -> Void

    func changeLog (ewm: EthereumWalletManager,
                    change: EthereumClientChangeType /* data */) -> Void

    // ...
    func handleEWMEvent (ewm: EthereumWalletManager,
                         event: EthereumEWMEvent) -> Void

    func handlePeerEvent (ewm: EthereumWalletManager,
                          event : EthereumPeerEvent) -> Void

    func handleWalletEvent (ewm: EthereumWalletManager,
                            wallet: EthereumWallet,
                            event: EthereumWalletEvent) -> Void

    func handleBlockEvent (ewm: EthereumWalletManager,
                           block: EthereumBlock,
                           event: EthereumBlockEvent) -> Void

    func handleTransferEvent (ewm: EthereumWalletManager,
                                 wallet: EthereumWallet,
                                 transfer: EthereumTransfer,
                                 event: EthereumTransferEvent) -> Void
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
    let core : BREthereumEWM //  OpaquePointer // BREthereumEWM

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

    private init (core: BREthereumEWM,
                  client : EthereumClient,
                  network : EthereumNetwork) {
        self.core = core
        self.client = client
        self.network = network
        EthereumWalletManager.add(self)
    }

    public convenience init (client : EthereumClient,
          network : EthereumNetwork,
          paperKey : String) {
        let anyClient = AnyEthereumClient (base: client)
        self.init (core: ethereumCreate (network.core, paperKey,
                                         NODE_TYPE_LES,
                                         SYNC_MODE_FULL_BLOCKCHAIN,
                                         EthereumWalletManager.createCoreClient(client: client),
                                         nil,
                                         nil,
                                         nil,
                                         nil),
                   client: anyClient,
                   network: network)
    }


    public convenience init (client : EthereumClient,
                      network : EthereumNetwork,
                      publicKey : BRKey) {
        let anyClient = AnyEthereumClient (base: client)
        self.init (core: ethereumCreateWithPublicKey (network.core, publicKey,
                                                      NODE_TYPE_LES,
                                                      SYNC_MODE_FULL_BLOCKCHAIN,
                                                      EthereumWalletManager.createCoreClient(client: client),
                                                      nil,
                                                      nil,
                                                      nil,
                                                      nil),
                   client: anyClient,
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
    internal func findTransfers (identifier: EthereumTransferId) -> EthereumTransfer {
        return EthereumTransfer (ewm: self, identifier: identifier)
    }

    //
    // Connect / Disconnect
    //
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

    public func announceGasEstimate (wid: EthereumWalletId, tid: EthereumTransferId, gasEstimate: String, rid: Int32) {
        ethereumClientAnnounceGasEstimate (core, wid, tid, gasEstimate, rid)
    }

    public func announceSubmitTransaction (wid: EthereumWalletId, tid: EthereumTransferId, hash: String, rid: Int32) {
        ethereumClientAnnounceSubmitTransfer (core, wid, tid, hash, rid)
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

    public func announceToken (rid: Int32,
                               address: String,
                               symbol: String,
                               name: String,
                               description: String,
                               decimals: UInt32) {
        ethereumClientAnnounceToken(core, rid,
                                    address, symbol, name, description, decimals)
    }
    
    public func announceBlockNumber (blockNumber: String, rid: Int32) {
        ethereumClientAnnounceBlockNumber(core, blockNumber, rid)
    }

    public func announceNonce (address: String, nonce: String, rid: Int32) {
        ethereumClientAnnounceNonce(core, address, nonce, rid)
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
        return BREthereumClient (
            context: UnsafeMutableRawPointer (Unmanaged<AnyEthereumClient>.passRetained(client).toOpaque()),

            funcGetBalance: { (coreClient, coreEWM, wid, address, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.getBalance(ewm: ewm,
                                      wid: wid,
                                      address: asUTF8String(address!),
                                      rid: rid)
                }},

            funcGetGasPrice: { (coreClient, coreEWM, wid, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    assert (ewm.client === client.base)
                    client.getGasPrice (ewm: ewm,
                                        wid: wid,
                                        rid: rid)
                }},

            funcEstimateGas: { (coreClient, coreEWM, wid, tid, to, amount, data, rid)  in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.getGasEstimate(ewm: ewm,
                                          wid: wid,
                                          tid: tid,
                                          to: asUTF8String(to!),
                                          amount: asUTF8String(amount!),
                                          data: asUTF8String(data!),
                                          rid: rid)
                }},

            funcSubmitTransaction: { (coreClient, coreEWM, wid, tid, transaction, rid)  in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.submitTransaction(ewm: ewm,
                                             wid: wid,
                                             tid: tid,
                                             rawTransaction: asUTF8String(transaction!),
                                             rid: rid)
                }},

            funcGetTransactions: { (coreClient, coreEWM, address, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.getTransactions(ewm: ewm,
                                           address: asUTF8String(address!),
                                           rid: rid)
                }},

            funcGetLogs: { (coreClient, coreEWM, contract, address, event, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.getLogs (ewm: ewm,
                                    address: asUTF8String(address!),
                                    event: asUTF8String(event!),
                                    rid: rid)
                }},

            funcGetTokens: { (coreClient, coreEWM, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.getTokens (ewm: ewm, rid: rid)
                }},

            funcGetBlockNumber: { (coreClient, coreEWM, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.getBlockNumber(ewm: ewm, rid: rid)
                }},

            funcGetNonce: { (coreClient, coreEWM, address, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.getNonce(ewm: ewm,
                                    address: asUTF8String(address!),
                                    rid: rid)
                }},

            funcSavePeers: { (coreClient, coreEWM, data) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.savePeers(ewm: ewm)
                }},

            funcSaveBlocks: { (coreClient, coreEWM, data) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.saveBlocks(ewm: ewm)
                }},

            funcChangeTransaction: { (coreClient, coreEWM, change, data) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.changeTransaction(ewm: ewm, change: EthereumClientChangeType(change))
                }},

            funcChangeLog: { (coreClient, coreEWM, change, data) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.changeLog(ewm: ewm, change: EthereumClientChangeType(change))
                }},

            funcEWMEvent: { (coreClient, coreEWM, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.handleEWMEvent (ewm: ewm, event: EthereumEWMEvent (event))
                }},

            funcPeerEvent: { (coreClient, coreEWM, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.handlePeerEvent (ewm: ewm, event: EthereumPeerEvent (event))
                }},

            funcWalletEvent: { (coreClient, coreEWM, wid, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.handleWalletEvent(ewm: ewm,
                                             wallet: ewm.findWallet(identifier: wid),
                                             event: EthereumWalletEvent (event))
                }},

            funcBlockEvent: { (coreClient, coreEWM, bid, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.handleBlockEvent(ewm: ewm,
                                            block: ewm.findBlock(identifier: bid),
                                            event: EthereumBlockEvent (event))
                }},

            funcTransferEvent: { (coreClient, coreEWM, wid, tid, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    client.handleTransferEvent(ewm: ewm,
                                                  wallet: ewm.findWallet(identifier: wid),
                                                  transfer: ewm.findTransfers(identifier: tid),
                                                  event: EthereumTransferEvent(event))
                }})
    }

    //
    // All Ethereum Wallet Managers
    //

    private static var all : [Weak<EthereumWalletManager>] = []

    private static func add (_ ewm: EthereumWalletManager) {
        all.append(Weak(value: ewm))
    }

    private static func lookup (core: BREthereumEWM?) -> EthereumWalletManager? {
        guard let core = core else { return nil }
        return all
            .filter { nil != $0.value }
            .map { $0.value! }
            .first { $0.core == core }
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

    func getGasEstimate(ewm: EthereumWalletManager, wid: EthereumWalletId, tid: EthereumTransferId, to: String, amount: String, data: String, rid: Int32) {
        base.getGasEstimate(ewm: ewm, wid: wid, tid: tid, to: to, amount: amount, data: data, rid: rid)
    }

    func getBalance(ewm: EthereumWalletManager, wid: EthereumWalletId, address: String, rid: Int32) {
        base.getBalance(ewm: ewm, wid: wid, address: address, rid: rid)
    }

    func submitTransaction(ewm: EthereumWalletManager, wid: EthereumWalletId, tid: EthereumTransferId, rawTransaction: String, rid: Int32) {
        base.submitTransaction(ewm: ewm, wid: wid, tid: tid, rawTransaction: rawTransaction, rid: rid)
    }

    func getTransactions(ewm: EthereumWalletManager, address: String, rid: Int32) {
        base.getTransactions(ewm: ewm, address: address, rid: rid)
    }

    func getLogs(ewm: EthereumWalletManager, address: String, event: String, rid: Int32) {
        base.getLogs(ewm: ewm, address: address, event: event, rid: rid)
    }

    func getTokens (ewm: EthereumWalletManager, rid: Int32) {
        base.getTokens(ewm: ewm, rid: rid)
    }

    func getBlockNumber(ewm: EthereumWalletManager, rid: Int32) {
        base.getBlockNumber(ewm: ewm, rid: rid)
    }

    func getNonce(ewm: EthereumWalletManager, address: String, rid: Int32) {
        base.getNonce(ewm: ewm, address: address, rid: rid)
    }

    func savePeers(ewm: EthereumWalletManager) {
        base.savePeers(ewm: ewm)
    }

    func saveBlocks(ewm: EthereumWalletManager) {
        base.saveBlocks(ewm: ewm)
    }

    func changeTransaction(ewm: EthereumWalletManager, change: EthereumClientChangeType) {
        base.changeTransaction(ewm: ewm, change: change)
    }

    func changeLog(ewm: EthereumWalletManager, change: EthereumClientChangeType) {
        base.changeLog(ewm: ewm, change: change)
    }

    func handleEWMEvent(ewm: EthereumWalletManager, event: EthereumEWMEvent) {
        base.handleEWMEvent(ewm: ewm, event: event)
    }

    func handlePeerEvent(ewm: EthereumWalletManager, event: EthereumPeerEvent) {
        base.handlePeerEvent(ewm: ewm, event: event)
    }

    func handleWalletEvent(ewm: EthereumWalletManager, wallet: EthereumWallet, event: EthereumWalletEvent) {
        base.handleWalletEvent(ewm: ewm, wallet: wallet, event: event)
    }

    func handleBlockEvent(ewm: EthereumWalletManager, block: EthereumBlock, event: EthereumBlockEvent) {
        base.handleBlockEvent(ewm: ewm, block: block, event: event)
    }

    func handleTransferEvent(ewm: EthereumWalletManager, wallet: EthereumWallet, transfer: EthereumTransfer, event: EthereumTransferEvent) {
        base.handleTransferEvent(ewm: ewm, wallet: wallet, transfer: transfer, event: event)
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
