//
//  BRCrypto.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
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
    internal private(set) weak var listener: TransferListener?

    internal let core: BRCryptoTransfer

    /// The owning wallet
    public let wallet: Wallet

    public private(set) lazy var manager: WalletManager = {
        return wallet.manager
    }()

    public private(set) lazy var system: System = {
        return wallet.manager.system
    }()

    public let unit: Unit

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
    public private(set) lazy var amount: Amount = {
        return Amount (core: cryptoTransferGetAmount (core), unit: wallet.unit, take: false)
    } ()

    /// The amount to transfer after considering the direction.  If we received the transfer,
    /// the amount will be positive; if we sent the transfer, the amount will be negative; if
    /// the transfer is 'self directed', the amount will be zero.
    public private(set) lazy var amountDirected: Amount = {
        return Amount (core: cryptoTransferGetAmountDirected(core), unit: wallet.unit, take: false)
    }()

    /// The fee paid - before the transfer is confirmed, this is the estimated fee.
    public private(set) lazy var fee: Amount = {
        let unit = wallet.manager.network.defaultUnitFor (currency: wallet.manager.currency)!
        return Amount (core: cryptoTransferGetFee (core), unit: unit, take: false)
    }()


    /// The basis for the fee.
    var feeBasis: TransferFeeBasis {
        return cryptoTransferGetFeeBasis (core)
            .map { TransferFeeBasis (core: $0, take: false) }!
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


    internal init (core: BRCryptoTransfer,
                   listener: TransferListener?,
                   wallet: Wallet,
                   unit: Unit,
                   take: Bool) {

        self.core = take ? cryptoTransferTake(core) : core
        self.listener = listener
        self.wallet = wallet
        self.unit = unit
    }


    // var originator: Bool { get }

    func identical (that: Transfer) -> Bool {
        return  CRYPTO_TRUE == cryptoTransferEqual (self.core, that.core)
    }

    deinit {
        cryptoTransferGive (core)
    }

    internal func announceEvent (_ event: TransferEvent) {
        self.listener?.handleTransferEvent (system: system,
                                            manager: manager,
                                            wallet: wallet,
                                            transfer: self,
                                            event: event)
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

    /// The confirmations of transfer at the current network `height`.
    public var confirmations: UInt64? {
        return confirmationsAt (blockHeight: wallet.manager.network.height)
    }
}

public enum TransferDirection {
    case sent
    case received
    case recovered

    internal init (core: BRCryptoTransferDirection) {
        switch core {
        case CRYPTO_TRANSFER_SENT:      self = .sent
        case CRYPTO_TRANSFER_RECEIVED:  self = .received
        case CRYPTO_TRANSFER_RECOVERED: self = .recovered
        default: self = .sent;  precondition(false)
        }
    }
}

///
/// A TransferFeeBasis is use to estimate the fee to complete a transfer
///
public class TransferFeeBasis {
    internal let core: BRCryptoFeeBasis

//    case bitcoin  (feePerKB: UInt64) // in satoshi
//    case ethereum (gasPrice: Amount, gasLimit: UInt64) // Amount in ETH

    internal init (core: BRCryptoFeeBasis, take: Bool) {
        self.core = take ? cryptoFeeBasisTake (core) : core
    }

    deinit {
        cryptoFeeBasisGive (core)
    }
}

///
/// A TransferConfirmation holds confirmation information.
///
public struct TransferConfirmation {
    public let blockNumber: UInt64
    public let transactionIndex: UInt64
    public let timestamp: UInt64
    public let fee: Amount?  // Optional, for now
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

///
/// A TransferState represents the states in Transfer's 'life-cycle'
///
public enum TransferState {
    case created
    case signed
    case submitted
    case pending
    case included (confirmation: TransferConfirmation)
    case failed (reason:String)
    case deleted
    
    internal init (core: BRCryptoTransferState) {
        switch core.type {
        case CRYPTO_TRANSFER_STATE_CREATED:   self = .created
        case CRYPTO_TRANSFER_STATE_SIGNED:    self = .signed
        case CRYPTO_TRANSFER_STATE_SUBMITTED: self = .submitted
        case CRYPTO_TRANSFER_STATE_INCLUDED:  self = .included (
            confirmation: TransferConfirmation (blockNumber: core.u.included.blockNumber,
                                                transactionIndex: core.u.included.transactionIndex,
                                                timestamp: core.u.included.timestamp,
                                                fee: nil))
        case CRYPTO_TRANSFER_STATE_ERRORRED:  self = .failed(reason: asUTF8String(cryptoTransferStateGetErrorMessage (core)))
        case CRYPTO_TRANSFER_STATE_DELETED:   self = .deleted
        default: /* ignore this */ self = .pending; precondition(false)
        }
    }
}

extension TransferState: CustomStringConvertible {
    public var description: String {
        switch self {
        case .created:   return "Created"
        case .signed:    return "Signed"
        case .submitted: return "Submitted"
        case .pending:   return "Pending"
        case .included:  return "Included"
        case .failed:    return "Failed"
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
    case confirmation (count: UInt64)
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
