//
//  BRCryptoTransfer.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import BRCryptoC

///
/// A Transfer represents the transfer of an `amount` of currency from `source` to `target`.  A
/// Transfer is held in a `Wallet` (holding the amount's currency); the Transfer requires a `fee`
/// to complete.  Once the transfer is signed/submitted it can be identified by a `TransferHash`.
/// Once the transfer has been included in the currency's blockchain it will have a
/// `TransferConfirmation`.
///
/// A Transfer is Equatable but not Hashable; Hashable would naturally be implmeneted in terms of
/// the TransferHash however that hash isn't available until after a transfer is signed.
///
public final class Transfer: Equatable {

    /// The Core representation
    internal let core: BRCryptoTransfer

    /// The owning wallet
    public let wallet: Wallet

    /// The owning manager
    public private(set) lazy var manager: WalletManager = {
        return wallet.manager
    }()

    /// The owning system
    public private(set) lazy var system: System = {
        return wallet.manager.system
    }()

    /// The unit for display of the transfer amount
    public let unit: Unit

    /// The unit for display of the transfer fee.
    public let unitForFee: Unit
    
    /// The source pays the fee and sends the amount.
    public private(set) lazy var source: Address? = {
        cryptoTransferGetSourceAddress (core)
            .map { Address (core: $0, take: false) }
    }()

    /// The target receives the amount
    public private(set) lazy var target: Address? = {
        cryptoTransferGetTargetAddress (core)
            .map { Address (core: $0, take: false) }
    }()

    /// The amount to transfer - always positive (from source to target)
    public let amount: Amount

    /// The amount to transfer after considering the direction.  If we received the transfer,
    /// the amount will be positive; if we sent the transfer, the amount will be negative; if
    /// the transfer is 'self directed', the amount will be zero.
    public let amountDirected: Amount

    /// TODO: Determine if the estimatedFeeBasis applies across program instantiations
    ///
    /// The basis for the estimated fee.  This is only not-nil if we have created the transfer
    /// IN THIS MEMORY INSTANCE (assume this for now).
    public var estimatedFeeBasis: TransferFeeBasis? {
        return cryptoTransferGetEstimatedFeeBasis (core)
            .map { TransferFeeBasis (core: $0, take: false) }
    }

    /// The basis for the confirmed fee.
    public var confirmedFeeBasis: TransferFeeBasis? {
        return cryptoTransferGetConfirmedFeeBasis (core)
            .map { TransferFeeBasis (core: $0, take: false) }
    }

    /// The fee paid - before the transfer is confirmed, this is the estimated fee.
    public var fee: Amount {
        guard let feeBasis = confirmedFeeBasis ?? estimatedFeeBasis
            else { preconditionFailure ("Missed confirmed+estimated feeBasis") }

        return feeBasis.fee
    }

    /// An optional confirmation.
    public var confirmation: TransferConfirmation? {
        if case .included (let confirmation) = state { return confirmation }
        else { return nil }
    }

    /// An optional hash
    public var hash: TransferHash? {
        return cryptoTransferGetHash (core)
            .map { TransferHash (core: $0) }
    }

    /// The current state
    public var state: TransferState {
        return TransferState (core: cryptoTransferGetState (core))
    }

    /// The direction
    public private(set) lazy var direction: TransferDirection = {
        return TransferDirection (core: cryptoTransferGetDirection (self.core))
    }()

    //
    // The Set of TransferAttributes.  These are the attributes used when the transfer was
    // created.  The attributes in the Set should be considered immutable; any mutations to the
    // attribtues will not impact the transfer.  The returned set is possibly, likely emptyh.
    //
    public private(set) lazy var attributes: Set<TransferAttribute> = {
        let coreAttributes = (0..<cryptoTransferGetAttributeCount(core))
            .map { cryptoTransferGetAttributeAt (core, $0)! }
        defer { coreAttributes.forEach (cryptoTransferAttributeGive) }

        // Make a copy so that any mutations of the attributes in the returned Set do not
        // make it back into this transfer's attributes.  The attributes themselves are reference
        // types and thus, even if the Set is immutable, it's elements won't be.
        return Set (coreAttributes.map { TransferAttribute (core: cryptoTransferAttributeCopy($0), take: false) })
    }()

    internal init (core: BRCryptoTransfer,
                   wallet: Wallet,
                   take: Bool) {

        self.core   = take ? cryptoTransferTake(core) : core
        self.wallet = wallet

        self.unit       = Unit (core: cryptoTransferGetUnitForAmount (core), take: false)
        self.unitForFee = Unit (core: cryptoTransferGetUnitForFee (core),    take: false)

        // Other properties
        self.amount         = Amount (core: cryptoTransferGetAmount (core),        take: false)
        self.amountDirected = Amount (core: cryptoTransferGetAmountDirected(core), take: false)
    }


    // var originator: Bool { get }

    func identical (that: Transfer) -> Bool {
        return  CRYPTO_TRUE == cryptoTransferEqual (self.core, that.core)
    }

    deinit {
        cryptoTransferGive (core)
    }

    // Equatable
    public static func == (lhs: Transfer, rhs: Transfer) -> Bool {
        return lhs === rhs || lhs.core == rhs.core
    }
}

extension Transfer {

    ///
    /// The confirmations of transfer at a provided `blockHeight`.  If the transfer has not been
    /// confirmed or if the `blockHeight` is less than the confirmation height then `nil` is
    /// returned.  The minimum returned value is 1 - if `blockHeight` is the same as the
    /// confirmation block, then the transfer has been confirmed once.
    ///
    /// - Parameter blockHeight:
    ///
    /// - Returns: the number of confirmations
    ///
    public func confirmationsAt (blockHeight: UInt64) -> UInt64? {
        return confirmation
            .flatMap { blockHeight >= $0.blockNumber ? (1 + blockHeight - $0.blockNumber) : nil }
    }

    /// The confirmations of transfer at the current network `height`. Since this value is
    /// calculated based on the associated network's `height`, it is recommended that a
    /// user refreshes any cache of it in response to WalletManagerEvent::blockUpdated events
    /// on the owning WalletManager, in addition to further TransferEvent::changed events
    /// on this Transfer.
    public var confirmations: UInt64? {
        return confirmationsAt (blockHeight: wallet.manager.network.height)
    }
}

public enum TransferDirection: Equatable {
    case sent
    case received
    case recovered

    internal init (core: BRCryptoTransferDirection) {
        switch core {
        case CRYPTO_TRANSFER_SENT:      self = .sent
        case CRYPTO_TRANSFER_RECEIVED:  self = .received
        case CRYPTO_TRANSFER_RECOVERED: self = .recovered
        default: self = .sent;  preconditionFailure()
        }
    }

    internal var core: BRCryptoTransferDirection {
        switch self {
        case .sent:      return CRYPTO_TRANSFER_SENT
        case .received:  return CRYPTO_TRANSFER_RECEIVED
        case .recovered: return CRYPTO_TRANSFER_RECOVERED
        }
    }
}

///
/// A TransferFeeBasis summarizes the fee required to complete a transfer.  This is used both
/// when created a transfer (the 'estimated fee basis') and once a transfer is confirmed (the
/// 'confirmed fee basis').
///
/// The provided properties allow the App to present detailed information - specifically the
/// 'cost factor' and the 'price per cost factor'.
///
public class TransferFeeBasis: Equatable {
    /// The Core representation
    internal let core: BRCryptoFeeBasis

    /// The unit for both the pricePerCostFactor and fee.
    public let unit: Unit

    /// The fee basis currency; this should/must be the Network's currency
    public var currency: Currency {
        return unit.currency
    }

    /// The pricePerCostFactor as an amount in currency
    public let pricePerCostFactor: Amount

    /// The costFactor which is an arbitrary scale on the pricePerCostFactor
    public let costFactor: Double

    /// The fee, computed as `pricePerCostFactor * costFactor`
    public let fee: Amount

    /// Initialize based on Core
    internal init (core: BRCryptoFeeBasis, take: Bool) {
        self.core = take ? cryptoFeeBasisTake (core) : core

        self.unit = Unit (core: cryptoFeeBasisGetPricePerCostFactorUnit(core), take: false)
        self.pricePerCostFactor = Amount (core: cryptoFeeBasisGetPricePerCostFactor(core), take: false)
        self.costFactor  = cryptoFeeBasisGetCostFactor (core)

        // TODO: The Core fee calculation might overflow.
        guard let fee = cryptoFeeBasisGetFee (core)
            .map ({ Amount (core: $0, take: false) })
            else { print ("Missed Fee"); preconditionFailure () }

        self.fee = fee
    }

    deinit {
        cryptoFeeBasisGive (core)
    }

    public static func == (lhs: TransferFeeBasis, rhs: TransferFeeBasis) -> Bool {
        return CRYPTO_TRUE == cryptoFeeBasisIsIdentical (lhs.core, rhs.core)
    }
}

///
/// A TransferConfirmation holds confirmation information.
///
public struct TransferConfirmation: Equatable {
    public let blockNumber: UInt64
    public let transactionIndex: UInt64
    public let timestamp: UInt64
    public let fee: Amount?  // Optional, for now
    public let success: Bool
    public let error: String?
}

///
/// A TransferHash uniquely identifies a transfer *among* the owning wallet's transfers.
///
public class TransferHash: Hashable, CustomStringConvertible {
    internal let core: BRCryptoHash

    init (core: BRCryptoHash) {
        self.core = core
    }

    deinit {
        cryptoHashGive (core)
    }

    public func hash (into hasher: inout Hasher) {
        hasher.combine (cryptoHashGetHashValue (core))
    }

    public static func == (lhs: TransferHash, rhs: TransferHash) -> Bool {
        return CRYPTO_TRUE == cryptoHashEqual (lhs.core, rhs.core)
    }

    public var description: String {
        return asUTF8String (cryptoHashString (core), true)
    }
}

public enum TransferSubmitError: Equatable, Error {
    case unknown
    case posix(errno: Int32, message: String?)

    internal init (core: BRCryptoTransferSubmitError) {
        switch core.type {
        case CRYPTO_TRANSFER_SUBMIT_ERROR_UNKNOWN:
            self = .unknown
        case CRYPTO_TRANSFER_SUBMIT_ERROR_POSIX:
            var c = core
            self = .posix(errno: core.u.posix.errnum,
                          message: cryptoTransferSubmitErrorGetMessage (&c).map{ asUTF8String($0, true) } )
        default: self = .unknown; preconditionFailure()
        }
    }
}

extension TransferSubmitError: CustomStringConvertible {
    public var description: String {
        switch self {
        case .unknown: return ".unknown"
        case let .posix(errno, message): return ".posix(\(errno):\(message ?? ""))"
        }
    }
}

///
/// A TransferAttribute is an arbitary {key, value} pair associated with a Transfer; the attribute
/// may be either required or optional.  The `key` and `isRequired` fields in a `TransferAttribute`
/// are immutable; the `value` is mutable and optional.
///
/// The `Transfer` field `attributes` is an immutable list of the attributes.
/// The `Wallet`   field `attributes` is an immutable list of the wallet's supported attributes.
///
/// The `Wallet` function `createTransfer` takes an optional Set of TransferAttributes.
///
/// TransferAttributes exist to provide for blockchain specific customizations to Transfers.
/// Specifically, for example, XRP provides a 'DestinationTag'.  Without this attributes concept
/// there is no way to augment an XRP transfer with such a tag.
///
public class TransferAttribute: Hashable {
    internal let core: BRCryptoTransferAttribute

    public let key: String

    public var value: String? {
        get { return cryptoTransferAttributeGetValue (core).map (asUTF8String) }
        set { cryptoTransferAttributeSetValue (core, newValue)}
    }

    public let isRequired: Bool

    deinit {
        cryptoTransferAttributeGive (core)
    }

    internal init (core: BRCryptoTransferAttribute, take: Bool) {
        self.core = (take ? cryptoTransferAttributeTake (core) : core)

        self.key = asUTF8String (cryptoTransferAttributeGetKey (core))
        self.isRequired = CRYPTO_TRUE == cryptoTransferAttributeIsRequired (core)
    }

    public static func == (lhs: TransferAttribute, rhs: TransferAttribute) -> Bool {
        return lhs.key == rhs.key
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine (key)
    }
}

extension TransferAttribute: CustomStringConvertible {
    public var description: String {
        return "\(key)(\(isRequired ? "R" : "O")):\(value ?? "")"
    }
}

public enum TransferAttributeValidationError {
    case requiredButNotProvided
    case mismatchedType
    case relationshipInconsistency

    internal var core: BRCryptoTransferAttributeValidationError {
        switch self {
        case .requiredButNotProvided:
            return CRYPTO_TRANSFER_ATTRIBUTE_VALIDATION_ERROR_REQUIRED_BUT_NOT_PROVIDED
        case .mismatchedType:
            return CRYPTO_TRANSFER_ATTRIBUTE_VALIDATION_ERROR_MISMATCHED_TYPE
        case .relationshipInconsistency:
            return CRYPTO_TRANSFER_ATTRIBUTE_VALIDATION_ERROR_RELATIONSHIP_INCONSISTENCY
        }
    }

    internal init (core: BRCryptoTransferAttributeValidationError) {
        switch core {
        case CRYPTO_TRANSFER_ATTRIBUTE_VALIDATION_ERROR_REQUIRED_BUT_NOT_PROVIDED:
            self = .requiredButNotProvided
        case CRYPTO_TRANSFER_ATTRIBUTE_VALIDATION_ERROR_MISMATCHED_TYPE:
            self = .mismatchedType
        case CRYPTO_TRANSFER_ATTRIBUTE_VALIDATION_ERROR_RELATIONSHIP_INCONSISTENCY:
            self = .relationshipInconsistency
        default:
            preconditionFailure()
        }
    }
}

///
/// A TransferState represents the states in Transfer's 'life-cycle'
///
public enum TransferState {
    case created
    case signed
    case submitted
    case pending
    case included (confirmation: TransferConfirmation)
    case failed (error: TransferSubmitError)
    case deleted

    internal init (core: BRCryptoTransferState) {
        defer {  var mutableCore = core; cryptoTransferStateRelease (&mutableCore) }
        switch core.type {
        case CRYPTO_TRANSFER_STATE_CREATED:   self = .created
        case CRYPTO_TRANSFER_STATE_SIGNED:    self = .signed
        case CRYPTO_TRANSFER_STATE_SUBMITTED: self = .submitted
        case CRYPTO_TRANSFER_STATE_INCLUDED:
            var coreError = core.u.included.error
            let error = CRYPTO_TRUE == core.u.included.success
                ? nil
                : asUTF8String(&coreError.0)

            self = .included (
                confirmation: TransferConfirmation (blockNumber: core.u.included.blockNumber,
                                                transactionIndex: core.u.included.transactionIndex,
                                                timestamp: core.u.included.timestamp,
                                                fee: core.u.included.feeBasis
                                                    .map { cryptoFeeBasisGetFee ($0) }
                                                    .map { Amount (core: $0, take: false) },
                                                success: CRYPTO_TRUE == core.u.included.success,
                                                error: error))
        case CRYPTO_TRANSFER_STATE_ERRORED:   self = .failed(error: TransferSubmitError (core: core.u.errored.error))
        case CRYPTO_TRANSFER_STATE_DELETED:   self = .deleted
        default: /* ignore this */ self = .pending; preconditionFailure()
        }
    }
}

extension TransferState: Equatable {}

extension TransferState: CustomStringConvertible {
    public var description: String {
        switch self {
        case .created:   return "Created"
        case .signed:    return "Signed"
        case .submitted: return "Submitted"
        case .pending:   return "Pending"
        case .included:  return "Included"
        case .failed (let error): return "Failed (\(error))"
        case .deleted:   return "Deleted"
        }
    }
}

///
/// A TransferEvent represents a asynchronous announcment of a transfer's state change.
///
public enum TransferEvent {
    case created
    case changed (old: TransferState, new: TransferState)
    case deleted
}

///
/// Listener for TransferEvent
///
public protocol TransferListener: class {
    ///
    /// Handle a TranferEvent.
    ///
    /// - Parameters:
    ///   - system: the system
    ///   - manager: the manager
    ///   - wallet: the wallet
    ///   - transfer: the transfer
    ///   - event: the transfer event.
    ///
    func handleTransferEvent (system: System,
                              manager: WalletManager,
                              wallet: Wallet,
                              transfer: Transfer,
                              event: TransferEvent)
}

/// A Functional Interface for a Handler
public typealias TransferEventHandler = (System, WalletManager, Wallet, Transfer, TransferEvent) -> Void

///
/// A `TransferFectory` is a customization point for `Transfer` creation.
///
public protocol TransferFactory {
    /// associatedtype T: Transfer

    ///
    /// Create a transfer in `wallet`
    ///
    /// - Parameters:
    ///   - target: The target receives 'amount'
    ///   - amount: The amount
    ///   - feeBasis: The basis for the 'fee'
    ///
    /// - Returns: A new transfer
    ///
//    func createTransfer (listener: TransferListener,
//                         wallet: Wallet,
//                         target: Address,
//                         amount: Amount,
//                         feeBasis: TransferFeeBasis) -> Transfer? // T
}
