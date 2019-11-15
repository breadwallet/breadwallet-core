//
//  BRCryptoWallet.swift
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
/// A Wallet holds the transfers and a balance for a single currency.
///
public final class Wallet: Equatable {

    /// The Core representation
    internal let core: BRCryptoWallet

    /// The owning manager
    public let manager: WalletManager

    /// The owning system
    public var system: System {
        return manager.system
    }

    internal var callbackCoordinator: SystemCallbackCoordinator
    
    /// The unit for display of the wallet's balance
    public let unit: Unit

    /// The currency held in wallet (as balance).
    public var currency: Currency {
        return unit.currency
    }

    public let unitForFee: Unit

    /// The (default) name derived from the currency.  For example: BTC, ETH, or BRD.
    public var name: String {
        return unit.currency.code
    }

    /// The current balance for currency
    public var balance: Amount {
        return Amount (core: cryptoWalletGetBalance (core), take: false)
    }

    /// The current state.
    public var state: WalletState {
        return WalletState (core: cryptoWalletGetState(core))
    }

    /// The default TransferFeeBasis for created transfers.
//    public var defaultFeeBasis: TransferFeeBasis {
//        get {
//            return TransferFeeBasis (core: cryptoWalletGetDefaultFeeBasis (core), take: false) }
//        set {
//            let defaultFeeBasis = newValue // rename, for clarity
//            cryptoWalletSetDefaultFeeBasis (core, defaultFeeBasis.core);
//        }
//    }

    /// The default TransferFactory for creating transfers.
    //    var transferFactory: TransferFactory { get set }

    /// An address suitable for a transfer target (receiving).  Uses the default Address Scheme
    public var target: Address {
        return targetForScheme (manager.addressScheme)
    }

    public func targetForScheme (_ scheme: AddressScheme) -> Address {
        return Address (core: cryptoWalletGetAddress (core, scheme.core), take: false)
    }

    /// TODO: `var targets: [Address]`

    /// TODO: Remove `source
    /// An address suitable for a transfer source (sending).  Uses the default AddressScheme
    public var source: Address {
        return Address (core: cryptoWalletGetAddress (core, manager.addressScheme.core), take: false)
    }

    /// TODO: `var sources: [Address]`

    /// The transfers of currency yielding `balance`
    public var transfers: [Transfer] {
        var transfersCount: size_t = 0
        let transfersPtr = cryptoWalletGetTransfers(core, &transfersCount);
        defer { if let ptr = transfersPtr { free (ptr) } }
        
        let transfers: [BRCryptoTransfer] = transfersPtr?.withMemoryRebound(to: BRCryptoTransfer.self, capacity: transfersCount) {
            Array(UnsafeBufferPointer (start: $0, count: transfersCount))
        } ?? []
        
        return transfers
            .map { Transfer (core: $0,
                             wallet: self,
                             take: false) }
    }

    /// Use a hash to lookup a transfer
    public func transferBy (hash: TransferHash) -> Transfer? {
        return transfers
            .first { $0.hash.map { $0 == hash } ?? false }
    }

    internal func transferBy (core: BRCryptoTransfer) -> Transfer? {
        return (CRYPTO_FALSE == cryptoWalletHasTransfer (self.core, core)
            ? nil
            : Transfer (core: core,
                        wallet: self,
                        take: true))
    }

    internal func transferByCoreOrCreate (_ core: BRCryptoTransfer,
                                          create: Bool = false) -> Transfer? {
        return transferBy (core: core) ??
            (!create
                ? nil
                : Transfer (core: core,
                            wallet: self,
                            take: true))
    }

    // address scheme

    ///
    /// Create a transfer for wallet.  Invokes the wallet's transferFactory to create a transfer.
    /// Generates events: TransferEvent.created and WalletEvent.transferAdded(transfer).
    ///
    /// - Parameters:
    ///   - listener: The transfer listener
    ///   - source: The source spends 'amount + fee'
    ///   - target: The target receives 'amount
    ///   - amount: The amount
    ///   - feeBasis: Teh basis for 'fee'
    ///
    /// - Returns: A new transfer
    ///
    public func createTransfer (target: Address,
                                amount: Amount,
                                estimatedFeeBasis: TransferFeeBasis) -> Transfer? {
        return cryptoWalletCreateTransfer (core, target.core, amount.core, estimatedFeeBasis.core)
            .map { Transfer (core: $0,
                             wallet: self,
                             take: false)
        }
    }

    internal func createTransfer(sweeper: WalletSweeper,
                                 estimatedFeeBasis: TransferFeeBasis) -> Transfer? {
        return cryptoWalletCreateTransferForWalletSweep(self.core, sweeper.core, estimatedFeeBasis.core)
            .map { Transfer (core: $0,
                             wallet: self,
                             take: false)
        }
    }

    internal func createTransfer(request: PaymentProtocolRequest,
                                 estimatedFeeBasis: TransferFeeBasis) -> Transfer? {
        return cryptoWalletCreateTransferForPaymentProtocolRequest(self.core, request.core, estimatedFeeBasis.core)
            .map { Transfer (core: $0,
                             wallet: self,
                             take: false)
        }
    }

    /// A `WalletEstimateFeeHandler` is a function to handle the result of a Wallet.estimateFee.
    public typealias EstimateFeeHandler = (Result<TransferFeeBasis,FeeEstimationError>) -> Void

    ///
    /// Estimate the fee for a transfer with `amount` from `wallet`.  If provided use the `feeBasis`
    /// otherwise use the wallet's `defaultFeeBasis`
    ///
    /// - Parameters:
    ///   - amount: the transfer amount MUST BE GREATER THAN 0
    ///   - feeBasis: the feeBasis to use, if provided
    ///
    /// - Returns: transfer fee
    ///
    public func estimateFee (target: Address,
                             amount: Amount,
                             fee: NetworkFee,
                             completion: @escaping Wallet.EstimateFeeHandler) {
        cryptoWalletEstimateFeeBasis (self.core,
                                      callbackCoordinator.addWalletFeeEstimateHandler(completion),
                                      target.core,
                                      amount.core,
                                      fee.core)
    }

    internal func estimateFee (sweeper: WalletSweeper,
                               fee: NetworkFee,
                               completion: @escaping (Result<TransferFeeBasis,FeeEstimationError>) -> Void) {
        cryptoWalletEstimateFeeBasisForWalletSweep(self.core,
                                                   callbackCoordinator.addWalletFeeEstimateHandler(completion),
                                                   sweeper.core,
                                                   fee.core)
    }

    internal func estimateFee (request: PaymentProtocolRequest,
                               fee: NetworkFee,
                               completion: @escaping (Result<TransferFeeBasis,FeeEstimationError>) -> Void) {
        cryptoWalletEstimateFeeBasisForPaymentProtocolRequest(self.core,
                                                              callbackCoordinator.addWalletFeeEstimateHandler(completion),
                                                              request.core,
                                                              fee.core)
    }

    public enum FeeEstimationError: Error {
        case ServiceUnavailable
        case ServiceError
        case InsufficientFunds

        static func fromStatus (_ status: BRCryptoStatus) -> FeeEstimationError {
            switch status {
            case CRYPTO_ERROR_FAILED: return .ServiceError
            default: return .ServiceError // preconditionFailure ("Unknown FeeEstimateError")
            }
        }
    }

    ///
    /// Create a `TransferFeeBasis` using a `pricePerCostFactor` and `costFactor`.
    ///
    /// - Note: This is 'private' until the parameters are described.  Meant for testing for now.
    ///
    /// - Parameters:
    ///   - pricePerCostFactor:
    ///   - costFactor:
    ///
    /// - Returns: An optional TransferFeeBasis
    ///
    public func createTransferFeeBasis (pricePerCostFactor: Amount,
                                        costFactor: Double) -> TransferFeeBasis? {
        return cryptoWalletCreateFeeBasis (core, pricePerCostFactor.core, costFactor)
            .map { TransferFeeBasis (core: $0, take: false) }
    }
    
    ///
    /// Create a wallet
    ///
    /// - Parameters:
    ///   - core: the BRCryptoWallet basis
    ///   - listener: an optional listener
    ///   - manager: the manager
    ///   - take: a boolean to indicate if `core` needs to be taken (for reference counting)
    ///
    internal init (core: BRCryptoWallet,
                   manager: WalletManager,
                   callbackCoordinator: SystemCallbackCoordinator,
                   take: Bool) {
        self.core = take ? cryptoWalletTake (core) : core
        self.manager = manager
        self.callbackCoordinator = callbackCoordinator
        self.unit = Unit (core: cryptoWalletGetUnit(core), take: false)
        self.unitForFee = Unit (core: cryptoWalletGetUnitForFee(core), take: false)
    }

    deinit {
        cryptoWalletGive (core)
    }

    // Equatable
    public static func == (lhs: Wallet, rhs: Wallet) -> Bool {
        return lhs === rhs || lhs.core == rhs.core
    }
}

extension Wallet {
    // Default implementation, using `transferFactory`
    //    public func createTransfer (listener: TransferListener,
    //                                target: Address,
    //                                amount: Amount,
    //                                feeBasis: TransferFeeBasis) -> Transfer? {
    //        return transferFactory.createTransfer (listener: listener,
    //                                               wallet: self,
    //                                               target: target,
    //                                               amount: amount,
    //                                               feeBasis: feeBasis)
    //    }

    ///
    /// Create a transfer for wallet using the `defaultFeeBasis`.  Invokes the wallet's
    /// `transferFactory` to create a transfer.  Generates events: TransferEvent.created and
    /// WalletEvent.transferAdded(transfer).
    ///
    /// - Parameters:
    ///   - source: The source spends 'amount + fee'
    ///   - target: The target receives 'amount'
    ///   - amount: The amouunt
    ///
    /// - Returns: A new transfer
    ///
//    public func createTransfer (target: Address,
//                                amount: Amount) -> Transfer? {
//        return createTransfer (target: target,
//                               amount: amount,
//                               feeBasis: defaultFeeBasis)
//    }

}

///
/// The Wallet state
///
/// - created: The wallet was created (and remains in existence).
/// - deleted: The wallet was deleted.
///
public enum WalletState: Equatable {
    case created
    case deleted

    internal init (core: BRCryptoWalletState) {
        switch core {
        case CRYPTO_WALLET_STATE_CREATED: self = .created
        case CRYPTO_WALLET_STATE_DELETED: self = .deleted
        default: self = .created; preconditionFailure()
        }
    }
}

///
/// A WalletEvent represents a asynchronous announcment of a wallet's state change.
///
public enum WalletEvent {
    case created
    case changed (oldState: WalletState, newState: WalletState)
    case deleted

    case transferAdded     (transfer: Transfer)
    case transferChanged   (transfer: Transfer)
    case transferDeleted   (transfer: Transfer)
    case transferSubmitted (transfer: Transfer, success: Bool)

    case balanceUpdated    (amount: Amount)
    case feeBasisUpdated   (feeBasis: TransferFeeBasis)
    case feeBasisEstimated (feeBasis: TransferFeeBasis)
}

extension WalletEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case .created:           return "Created"
        case .changed:           return "StateChanged"
        case .deleted:           return "Deleted"
        case .transferAdded:     return "TransferAdded"
        case .transferChanged:   return "TransferChanged"
        case .transferDeleted:   return "TransferDeleted"
        case .transferSubmitted: return "TransferSubmitted"
        case .balanceUpdated:    return "BalanceUpdated"
        case .feeBasisUpdated:   return "FeeBasisUpdated"
        case .feeBasisEstimated: return "FeeBasisEstimated"
        }
    }
}

///
/// Listener for WalletEvent
///
public protocol WalletListener: class {
    ///
    /// Handle a WalletEvent
    ///
    /// - Parameters:
    ///   - system: the system
    ///   - manager: the manager
    ///   - wallet: the wallet
    ///   - event: the wallet event.
    ///
    func handleWalletEvent (system: System,
                            manager: WalletManager,
                            wallet: Wallet,
                            event: WalletEvent)
}
/// A Functional Interface for a Handler
public typealias WalletEventHandler = (System, WalletManager, Wallet, WalletEvent) -> Void


///
/// A WalletFactory is a customization point for Wallet creation.
/// TODO: ?? AND HOW DOES THIS FIT WITH CoreWallet w/ REQUIRED INTERFACE TO Core ??
///
public protocol WalletFactory {
    ///
    /// Create a Wallet managed by `manager` and holding `currency`.  The wallet is initialized
    /// with no balance, no transfers and some default feeBasis (appropriate for the `currency`).
    /// Generates events: WalletEvent.created (and maybe others).
    ///
    /// - Parameters:
    ///   - manager: the Wallet's manager
    ///   - currency: The currency held
    ///
    /// - Returns: A new wallet
    ///
    //    func createWallet (manager: WalletManager,
    //                       currency: Currency) -> Wallet
}

