//
//  BREthereum.swift
//  breadwallet
//
//  Created by Ed Gamble on 3/28/18.
//  Copyright © 2018 breadwallet LLC. All rights reserved.
//

import Core.Ethereum

public typealias EthereumReferenceId = Optional<OpaquePointer>
public typealias EthereumWalletId = EthereumReferenceId
public typealias EthereumTransferId = EthereumReferenceId
public typealias EthereumAccountId = EthereumReferenceId
public typealias EthereumAddressId = EthereumReferenceId
public typealias EthereumBlockId = EthereumReferenceId
public typealias EthereumListenerId = EthereumReferenceId

// Access to BRCore/BREthereum types
public typealias BRCoreEWM = BREthereumEWM // OpaquePointer

///
/// MARK: Reference
///

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

///
/// MARK: - Pointer
///

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

///
/// MARK: - Account
///

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
        let cString = ewmGetAccountPrimaryAddress (self.ewm!.core)
        let string = String (cString: cString!)
        free (cString)
        return string
    }

    // public key
    var addressPublicKey : BRKey {
        return ewmGetAccountPrimaryAddressPublicKey (self.ewm!.core)
    }

    func addresssPrivateKey (paperKey: String) -> BRKey {
        return ewmGetAccountPrimaryAddressPrivateKey (self.ewm!.core, paperKey)
    }
}

///
/// MARK: - Network
///

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

extension EthereumNetwork : Equatable {
    static public func == (lhs: EthereumNetwork, rhs: EthereumNetwork) -> Bool {
        return lhs.core == rhs.core
    }
}

///
/// MARK: - Token
///

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

    static internal func lookup (core: BREthereumToken) -> EthereumToken {
        return EthereumToken (core: core)
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
        return []
//        var refs = tokenGetAll()
//        var tokens = refs.map {  EthereumToken (core: $0) }
//        free (refs);
//        return tokens
//
//        return  tokenGetAll().
//        var refs = tokenGetAll()
//        let tokenIndex = Int(exactly: tokenCount())! - 1
//        return (0...tokenIndex)
//            .map { tokenGet (Int32($0)) }
//            .map { EthereumToken (core: $0) }
    }()
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
    #if false
    var number : UInt64 {
        return ewmBlockGetNumber (self.ewm!.core, identifier)
    }

    var timestamp : UInt64 {
        return ewmBlockGetTimestamp (self.ewm!.core, identifier)
    }

    var hash : String {
        return asUTF8String(ewmBlockGetHash (self.ewm!.core, identifier))
    }
    #endif
}

///
/// MARK: - Transfer
///

///
/// An `EthereumTransaction` represents a transfer of ETHER or a specific TOKEN between two
/// accounts.
///
public struct EthereumTransfer : EthereumReferenceWithDefaultUnit {
    public weak var ewm : EthereumWalletManager?
    public let identifier : EthereumTransferId
    public let unit : EthereumAmountUnit

    internal init (ewm : EthereumWalletManager, identifier : EthereumTransferId) {
        self.init (ewm: ewm,
                   identifier: identifier,
                   unit: EthereumAmountUnit.defaultUnit(
                    nil == ewmTransferGetToken (ewm.core, identifier)))
    }

    internal init (ewm : EthereumWalletManager, identifier : EthereumTransferId, unit: EthereumAmountUnit) {
        self.ewm = ewm
        self.identifier = identifier
        self.unit = unit
    }

    public var hash : String {
        let hash = ewmTransferGetHash (self.ewm!.core, self.identifier)
        return asUTF8String (hashAsString(hash), true)
    }

    public var sourceAddress : String {
        return asUTF8String (addressGetEncodedString(ewmTransferGetSource(self.ewm!.core, self.identifier), 1))
    }

    public var targetAddress : String {
        return asUTF8String (addressGetEncodedString(ewmTransferGetTarget(self.ewm!.core, self.identifier), 1))
    }

    public var amount : EthereumAmount {
        let amount : BREthereumAmount = ewmTransferGetAmount (self.ewm!.core, self.identifier)
        return (AMOUNT_ETHER == amount.type
            ? EthereumAmount.ether(amount.u.ether.valueInWEI, unit.coreForEther)
            : EthereumAmount.token (amount.u.tokenQuantity.valueAsInteger,
                                    EthereumToken.lookup (core: amount.u.tokenQuantity.token),
                                    unit.coreForToken))
    }

    public var fee : EthereumAmount {
        var overflow : Int32 = 0
        let fee : BREthereumEther = ewmTransferGetFee(self.ewm!.core, self.identifier, &overflow);
        return EthereumAmount.ether(fee.valueInWEI, unit.coreForEther)
    }

    public var confirmations : UInt64? {
        let confirmations = ewmTransferGetBlockConfirmations(self.ewm!.core, self.identifier)
        return confirmations > 0 ? confirmations : nil
    }

    public var confirmationBlockNumber : UInt64? {
        let number = ewmTransferGetBlockNumber(self.ewm!.core, self.identifier)
        return number > 0 ? number : nil
    }

    public var state : State {
        return State (ewmTransferGetStatus(self.ewm!.core, self.identifier))
    }

    public var stateError : Error? {
        let type = ewmTransferStatusGetErrorType(self.ewm!.core, self.identifier)
        return -1 != type ? Error (type) : nil
    }

    public var stateErrorReason : String? {
        if let coreReason = ewmTransferStatusGetError(self.ewm!.core, self.identifier) {
            return asUTF8String(coreReason, true)
        }
        return nil
    }

    public var nonce : UInt64 {
        return ewmTransferGetNonce(self.ewm!.core, self.identifier)
    }
    
    //    var gasPrice : EthereumAmount {
    //        let price : BREthereumAmount = ewmTransferGetGasPriceToo (self.ewm!.core, self.identifier)
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
    public enum State : CustomStringConvertible {
        case created
        case submitted
        case included
        case errored
        case cancelled
        case replaced
        case deleted

        init (_ event: BREthereumTransferStatus) {
            switch (event) {
            case TRANSFER_STATUS_CREATED: self = .created
            case TRANSFER_STATUS_SUBMITTED: self = .submitted
            case TRANSFER_STATUS_INCLUDED: self = .included
            case TRANSFER_STATUS_ERRORED: self = .errored
            case TRANSFER_STATUS_CANCELLED: self = .cancelled
            case TRANSFER_STATUS_REPLACED: self = .replaced
            case TRANSFER_STATUS_DELETED: self = .deleted
            default: self = .created
            }
        }

        public var description: String {
            switch self {
            case .created: return "created"
            case .submitted: return "submitted"
            case .included:  return "included"
            case .errored:   return "errored"
            case .cancelled: return "cancelled"
            case .replaced: return "replaced"
            case .deleted: return "deleted"
            }
        }
    }

    public enum Error {
        case invalidSignature
        case nonceTooLow
        case balanceTooLow
        case gasPriceTooLow
        case gasTooLow
        case replacementUnderPriced
        case dropped
        case unknown

        init (_ error: Int32) {
            switch error {
            case 0: self = .invalidSignature
            case 1: self = .nonceTooLow
            case 2: self = .balanceTooLow
            case 3: self = .gasPriceTooLow
            case 4: self = .gasTooLow
            case 5: self = .replacementUnderPriced
            case 6: self = .dropped
            case 7: self = .unknown
            default:
                self = .unknown
            }
        }
    }


}

///
/// MARK: - Wallet
///

///
/// An `EthereumWallet` holds a balance with ETHER or a TOKEN and is associated with a Network
/// and an Account on that network.  An `EthereumWallet` as a default unit.
///
public struct EthereumWallet : EthereumReferenceWithDefaultUnit, Hashable {

    public weak var ewm : EthereumWalletManager?
    public let identifier : EthereumWalletId
    public let unit : EthereumAmountUnit

    let account : EthereumAccount
    let network : EthereumNetwork
    public let token   : EthereumToken?

    public var name : String {
        return token?.symbol ?? "ETH"
    }

    //
    // Gas Limit (in 'gas')
    //
    var defaultGasLimit : UInt64 {
        get {
            return ewmWalletGetDefaultGasLimit (self.ewm!.core, self.identifier).amountOfGas
        }
        set (value) {
            ewmWalletSetDefaultGasLimit (self.ewm!.core, self.identifier, gasCreate (value))
        }
    }

    func gasEstimate (transfer: EthereumTransfer) -> UInt64 {
        return ewmWalletGetGasEstimate (self.ewm!.core, self.identifier, transfer.identifier).amountOfGas
    }

    //
    // Gas Price (ETHER in WEI)
    //
    static let maximumGasPrice : UInt64 = 100000000000000

    var defaultGasPrice : UInt64 {
        get {
            return ewmWalletGetDefaultGasPrice (self.ewm!.core, self.identifier).etherPerGas.valueInWEI.u64.0
        }
        set (value) {
            precondition(value <= EthereumWallet.maximumGasPrice)
            ewmWalletSetDefaultGasPrice (self.ewm!.core, self.identifier,
                                         gasPriceCreate(etherCreateNumber(value, WEI)));
        }
    }

    //
    // Balance
    //
    public var balance : EthereumAmount {
        let amount : BREthereumAmount = ewmWalletGetBalance (self.ewm!.core, self.identifier)
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
    // Transfer
    //
    public func createTransfer (recvAddress: String, amount: String, unit: EthereumAmountUnit) -> EthereumTransfer {
        var status : BRCoreParseStatus = CORE_PARSE_OK
        let amount = (unit.isEther
            ? ewmCreateEtherAmountString (self.ewm!.core, amount, unit.coreForEther, &status)
            : ewmCreateTokenAmountString (self.ewm!.core, token!.core, amount, unit.coreForToken, &status))
        // Sure, ignore `status`

        let tid = ewmWalletCreateTransfer (self.ewm!.core,
                                                self.identifier,
                                                recvAddress,
                                                amount)
        return EthereumTransfer (ewm: self.ewm!, identifier: tid)
    }

    public func createTransfer (recvAddress: String,
                                amount: String, unit: EthereumAmountUnit,
                                gasPrice: UInt64, gasPriceUnit: EthereumAmountUnit,
                                gasLimit: UInt64) -> EthereumTransfer {
        var status : BRCoreParseStatus = CORE_PARSE_OK
        let amount = (unit.isEther
            ? ewmCreateEtherAmountString (self.ewm!.core, amount, unit.coreForEther, &status)
            : ewmCreateTokenAmountString (self.ewm!.core, token!.core, amount, unit.coreForToken, &status))
        // Sure, ignore `status`

        let gasPrice = ewmCreateGasPrice (gasPrice, gasPriceUnit.coreForEther)
        let gasLimit = ewmCreateGas (gasLimit)

        let tid = ewmWalletCreateTransferWithFeeBasis (self.ewm!.core,
                                                            self.identifier,
                                                            recvAddress,
                                                            amount,
                                                            feeBasisCreate(gasLimit, gasPrice))
        return EthereumTransfer (ewm: self.ewm!, identifier: tid)
    }


    public func sign (transfer : EthereumTransfer, paperKey : String) {
        ewmWalletSignTransferWithPaperKey (self.ewm!.core, self.identifier, transfer.identifier, paperKey)
    }

    public func sign (transfer: EthereumTransfer, privateKey: BRKey) {
        ewmWalletSignTransfer (self.ewm!.core, self.identifier, transfer.identifier, privateKey)
    }

    public func submit (transfer : EthereumTransfer) {
        ewmWalletSubmitTransfer (self.ewm!.core, self.identifier, transfer.identifier)
    }

//    public func submitCancel (transfer : EthereumTransfer, paperKey: String) {
//        ewmWalletSubmitTransferCancel(self.ewm!.core, self.identifier, transfer.identifier, paperKey)
//    }

//    public func submitAgain(transfer : EthereumTransfer, paperKey: String) {
//        ewmWalletSubmitTransferAgain(self.ewm!.core, self.identifier, transfer.identifier, paperKey)
//    }

    public func canCancelTransfer (transfer: EthereumTransfer) -> Bool {
        return ETHEREUM_BOOLEAN_TRUE == ewmWalletCanCancelTransfer (self.ewm!.core, self.identifier, transfer.identifier)
    }

    public func createTransferToCancel (transfer: EthereumTransfer) -> EthereumTransfer {
        let tid = ewmWalletCreateTransferToCancel (self.ewm!.core, self.identifier, transfer.identifier);
        return EthereumTransfer (ewm: self.ewm!, identifier: tid)
    }

    public func canReplaceTransfer (transfer: EthereumTransfer) -> Bool {
        return ETHEREUM_BOOLEAN_TRUE == ewmWalletCanReplaceTransfer (self.ewm!.core, self.identifier, transfer.identifier)
    }

    public func createTransferToReplace (transfer: EthereumTransfer,
                                         updateNonce: Bool = false,
                                         updateRecvAddress: String? = nil,
                                         updateAmount: EthereumAmount? = nil,
                                         updateGasPrice: Bool = false,
                                         updateGasLimit: Bool = false) -> EthereumTransfer {

        let tid = ewmWalletCreateTransferToReplace (
            self.ewm!.core,
            self.identifier,
            transfer.identifier,
            //   updateRecvAddress ?? transfer.targetAddress,
            //   updateAmount ?? transfer.amount,
            (updateGasPrice ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE),
            (updateGasLimit ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE),
            (updateNonce ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
        return EthereumTransfer (ewm: self.ewm!, identifier: tid)
    }

    public func estimateFee (amount : String, unit: EthereumAmountUnit) -> EthereumAmount {
        var overflow : Int32 = 0
        var status : BRCoreParseStatus = CORE_PARSE_OK
        let fee = ewmWalletEstimateTransferFee (self.ewm!.core,
                                                     self.identifier,
                                                     (unit.isEther
                                                        ? ewmCreateEtherAmountString (self.ewm!.core, amount, unit.coreForEther, &status)
                                                        : ewmCreateTokenAmountString (self.ewm!.core, token!.core, amount, unit.coreForToken, &status)),
                                                     &overflow)
        return EthereumAmount.ether(fee.valueInWEI, unit.coreForEther)

    }

    public var transfers : [EthereumTransfer] {
        let count = ewmWalletGetTransferCount (self.ewm!.core, self.identifier)
        let identifiers = ewmWalletGetTransfers (self.ewm!.core, self.identifier)
        return UnsafeBufferPointer (start: identifiers, count: Int(exactly: count)!)
            .map { self.ewm!.findTransfers(identifier: $0) }
    }

    public var transfersCount : Int {
        return Int (exactly: ewmWalletGetTransferCount (self.ewm!.core, self.identifier))!
    }

    public var hashValue: Int {
        return name.hashValue
    }
}

///
/// MARK: - Client
///

public enum EthereumStatus {
    case success
}

public enum EthereumMode {
    case brd_only
    case brd_with_p2p_send
    case p2p_with_brd_sync
    case p2p_only

    init (_ mode: BREthereumMode) {
        switch (mode) {
        case BRD_ONLY: self = .brd_only
        case BRD_WITH_P2P_SEND: self = .brd_with_p2p_send
        case P2P_WITH_BRD_SYNC: self = .p2p_with_brd_sync
        case P2P_ONLY: self = .p2p_only
        default:
            self = .p2p_only
        }
    }

    var core : BREthereumMode {
        switch (self) {
        case .brd_only: return BRD_ONLY
        case .brd_with_p2p_send: return BRD_WITH_P2P_SEND
        case .p2p_with_brd_sync: return P2P_WITH_BRD_SYNC
        case .p2p_only: return P2P_ONLY
        }
    }
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

    // case cancelled
    // case replaced

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

    func saveNodes (ewm: EthereumWalletManager,
                    data: Dictionary<String, String>) -> Void

    func saveBlocks (ewm: EthereumWalletManager,
                     data: Dictionary<String, String>) -> Void

    func changeTransaction (ewm: EthereumWalletManager,
                            change: EthereumClientChangeType,
                            hash: String,
                            data: String) -> Void

    func changeLog (ewm: EthereumWalletManager,
                    change: EthereumClientChangeType,
                    hash: String,
                    data: String) -> Void

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

public enum EthereumKey {
    case paperKey (String)
    case publicKey (BRKey)
}

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
                                identifier: ewmGetAccount (self.core))
    }()

    lazy var queue : DispatchQueue = {
        return DispatchQueue (label: "Ethereum")
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
                             mode: EthereumMode,
                             key: EthereumKey,
                             timestamp: UInt64) {
        self.init (client: client,
                   network: network,
                   mode: mode,
                   key: key,
                   timestamp: timestamp,
                   peers: [:],
                   blocks: [:],
                   transactions: [:],
                   logs: [:])
    }

    public convenience init (client : EthereumClient,
                             network : EthereumNetwork,
                             mode: EthereumMode,
                             key: EthereumKey,
                             timestamp: UInt64,
                             peers: Dictionary<String,String>,
                             blocks: Dictionary<String,String>,
                             transactions: Dictionary<String,String>,
                             logs: Dictionary<String,String>) {
        let anyClient = AnyEthereumClient (base: client)
        var core : BREthereumEWM

        switch key {
        case let .paperKey(key):
            core = ewmCreateWithPaperKey (network.core, key, timestamp,
                              mode.core,
                              EthereumWalletManager.createCoreClient(client: client),
                              EthereumWalletManager.asPairs (peers),
                              EthereumWalletManager.asPairs (blocks),
                              EthereumWalletManager.asPairs (transactions),
                              EthereumWalletManager.asPairs (logs))
        case let .publicKey(key):
            core = ewmCreateWithPublicKey (network.core, key, timestamp,
                              mode.core,
                              EthereumWalletManager.createCoreClient(client: client),
                              EthereumWalletManager.asPairs (peers),
                              EthereumWalletManager.asPairs (blocks),
                              EthereumWalletManager.asPairs (transactions),
                              EthereumWalletManager.asPairs (logs))
        }
        
        self.init (core: core,
                   client: anyClient,
                   network: network)
    }

    //
    // Wallets
    //
    public var wallets : [EthereumWallet] {
        let count = ewmGetWalletsCount (self.core)
        let identifiers = ewmGetWallets(self.core)
        return UnsafeBufferPointer (start: identifiers, count: Int(exactly: count)!)
            .map { self.findWallet (identifier: $0) }
    }
    
    public func getWallet () -> EthereumWallet {
        return findWallet (identifier: ewmGetWallet (core))
    }

    public func getWallet (token: EthereumToken) -> EthereumWallet {
        return findWallet (identifier: ewmGetWalletHoldingToken (core, token.core))
    }

    public func findWallet (identifier: EthereumWalletId) -> EthereumWallet {
        let token = ewmWalletGetToken (core, identifier)
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
        return ewmGetBlockHeight (core)
    }
    
    //
    // Transactions
    //
    internal func findTransfers (identifier: EthereumTransferId) -> EthereumTransfer {
        return EthereumTransfer (ewm: self, identifier: identifier)
    }

    //
    // Tokens
    //
    public func updateTokens () {
        ewmUpdateTokens(core)
    }
    
    //
    // Connect / Disconnect
    //
    public func connect () {
        ewmConnect (self.core);
    }

    public func disconnect () {
        ewmDisconnect (self.core)
    }

    public var address : String {
        return account.address
    }

    public func announceBalance (wid: EthereumWalletId, balance: String, rid: Int32) {
        ewmAnnounceWalletBalance (core, wid, balance, rid)
    }

    public func announceGasPrice (wid: EthereumWalletId, gasPrice: String, rid: Int32) {
        ewmAnnounceGasPrice (core, wid, gasPrice, rid)
    }

    public func announceGasEstimate (wid: EthereumWalletId, tid: EthereumTransferId, gasEstimate: String, rid: Int32) {
        ewmAnnounceGasEstimate (core, wid, tid, gasEstimate, rid)
    }

    public func announceSubmitTransaction (wid: EthereumWalletId, tid: EthereumTransferId, hash: String, rid: Int32) {
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
        ewmAnnounceToken(core,
                                    address, symbol, name, description, decimals, nil, nil, rid)
    }
    
    public func announceBlockNumber (blockNumber: String, rid: Int32) {
        ewmAnnounceBlockNumber(core, blockNumber, rid)
    }

    public func announceNonce (address: String, nonce: String, rid: Int32) {
        ewmAnnounceNonce(core, address, nonce, rid)
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
                    ewm.queue.async {
                        client.getBalance(ewm: ewm,
                                          wid: wid,
                                          address: asUTF8String(address!),
                                          rid: rid)
                    }
                }},

            funcGetGasPrice: { (coreClient, coreEWM, wid, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    assert (ewm.client === client.base)
                    ewm.queue.async {
                        client.getGasPrice (ewm: ewm,
                                            wid: wid,
                                            rid: rid)
                    }
                }},

            funcEstimateGas: { (coreClient, coreEWM, wid, tid, to, amount, data, rid)  in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.getGasEstimate(ewm: ewm,
                                              wid: wid,
                                              tid: tid,
                                              to: asUTF8String(to!),
                                              amount: asUTF8String(amount!),
                                              data: asUTF8String(data!),
                                              rid: rid)
                    }
                }},

            funcSubmitTransaction: { (coreClient, coreEWM, wid, tid, transaction, rid)  in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.submitTransaction(ewm: ewm,
                                                 wid: wid,
                                                 tid: tid,
                                                 rawTransaction: asUTF8String(transaction!),
                                                 rid: rid)
                    }
                }},

            funcGetTransactions: { (coreClient, coreEWM, address, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.getTransactions(ewm: ewm,
                                               address: asUTF8String(address!),
                                               rid: rid)
                    }
                }},

            funcGetLogs: { (coreClient, coreEWM, contract, address, event, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.getLogs (ewm: ewm,
                                        address: asUTF8String(address!),
                                        event: asUTF8String(event!),
                                        rid: rid)
                    }
                }},

            funcGetBlocks: { (coreClient, coreEWM, address, interests, blockStart, blockStop, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.getBlocks (ewm: ewm,
                                          address: asUTF8String (address!),
                                          interests: interests,
                                          blockStart: blockStart,
                                          blockStop: blockStop,
                                          rid: rid)
                    }
                }},

            funcGetTokens: { (coreClient, coreEWM, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.getTokens (ewm: ewm, rid: rid)
                    }
                }},

            funcGetBlockNumber: { (coreClient, coreEWM, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.getBlockNumber(ewm: ewm, rid: rid)
                    }
                }},

            funcGetNonce: { (coreClient, coreEWM, address, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.getNonce(ewm: ewm,
                                        address: asUTF8String(address!),
                                        rid: rid)
                    }
                }},

            funcSaveNodes: { (coreClient, coreEWM, data) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.saveNodes(ewm: ewm, data: EthereumWalletManager.asDictionary(data!))
                    }
                }},

            funcSaveBlocks: { (coreClient, coreEWM, data) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.saveBlocks(ewm: ewm, data: EthereumWalletManager.asDictionary(data!))
                    }
                }},

            funcChangeTransaction: { (coreClient, coreEWM, change, data) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {

                        let cStrHash = hashDataPairGetHashAsString (data)!
                        let cStrData = hashDataPairGetDataAsString (data)!

                        client.changeTransaction (ewm: ewm,
                                                  change: EthereumClientChangeType(change),
                                                  hash: String (cString: cStrHash),
                                                  data: String (cString: cStrData))

                        free (cStrHash); free (cStrData)
                    }
                }},

            funcChangeLog: { (coreClient, coreEWM, change, data) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {

                        let cStrHash = hashDataPairGetHashAsString (data)!
                        let cStrData = hashDataPairGetDataAsString (data)!

                        client.changeLog (ewm: ewm,
                                          change: EthereumClientChangeType(change),
                                          hash: String (cString: cStrHash),
                                          data: String (cString: cStrData))

                        free (cStrHash); free (cStrData)
                    }
                }},

            funcEWMEvent: { (coreClient, coreEWM, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.handleEWMEvent (ewm: ewm, event: EthereumEWMEvent (event))
                    }
                }},

            funcPeerEvent: { (coreClient, coreEWM, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.handlePeerEvent (ewm: ewm, event: EthereumPeerEvent (event))
                    }
                }},

            funcWalletEvent: { (coreClient, coreEWM, wid, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.handleWalletEvent(ewm: ewm,
                                                 wallet: ewm.findWallet(identifier: wid),
                                                 event: EthereumWalletEvent (event))
                    }
                }},

            //            funcBlockEvent: { (coreClient, coreEWM, bid, event, status, message) in
            //                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
            //                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
            //                    client.handleBlockEvent(ewm: ewm,
            //                                            block: ewm.findBlock(identifier: bid),
            //                                            event: EthereumBlockEvent (event))
            //                }},

            funcTransferEvent: { (coreClient, coreEWM, wid, tid, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
                    ewm.queue.async {
                        client.handleTransferEvent(ewm: ewm,
                                                   wallet: ewm.findWallet(identifier: wid),
                                                   transfer: ewm.findTransfers(identifier: tid),
                                                   event: EthereumTransferEvent(event))
                    }
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

    //
    // Hash Data Pair Set
    //
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

    static public let defaultUnitEther = EthereumAmountUnit.ether (ETHER)
    static public let defaultUnitToken = EthereumAmountUnit.token (TOKEN_QUANTITY_TYPE_DECIMAL)

    static public let etherGWEI = EthereumAmountUnit.ether (GWEI);
    static public let etherWEI  = EthereumAmountUnit.ether (WEI);

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

    public var token : EthereumToken? {
        switch (self) {
        case .ether: return nil
        case let .token (_, t, _): return t;
        }
    }

    public var isEther : Bool {
        switch (self) {
        case .ether: return true
        default: return false
        }
    }

    public var isToken : Bool {
        switch (self) {
        case .token: return true
        default: return false
        }
    }

    public var amount : String {
        switch (self) {
        case let .ether (_, unit): return getAmountAsEther (unit: unit)
        case let .token (_, _, unit): return getAmountAsToken (unit: unit)
        }
    }

    public var symbol : String {
        switch self {
        case .ether: return "ETH"
        case let .token (_, token, _): return token.symbol
        }
    }

    public func getAmount (unit: EthereumAmountUnit) -> String? {
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

    func getBlocks (ewm: EthereumWalletManager, address: String, interests: UInt32, blockStart: UInt64, blockStop: UInt64,  rid: Int32) {
        base.getBlocks (ewm: ewm, address: address, interests: interests, blockStart: blockStart, blockStop: blockStop, rid: rid)
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

    func saveNodes(ewm: EthereumWalletManager,
                   data: Dictionary<String, String>) {
        base.saveNodes(ewm: ewm, data: data)
    }

    func saveBlocks(ewm: EthereumWalletManager,
                    data: Dictionary<String, String>) {
        base.saveBlocks(ewm: ewm, data: data)
    }

    func changeTransaction(ewm: EthereumWalletManager,
                           change: EthereumClientChangeType,
                           hash: String,
                           data: String) {
        base.changeTransaction(ewm: ewm, change: change, hash: hash, data: data)
    }

    func changeLog(ewm: EthereumWalletManager,
                   change: EthereumClientChangeType,
                   hash: String,
                   data: String) {
        base.changeLog (ewm: ewm, change: change, hash: hash, data: data)
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
