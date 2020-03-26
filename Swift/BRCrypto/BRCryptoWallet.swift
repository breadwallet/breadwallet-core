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

    /// The maximum balance
    public var balanceMaximum: Amount? {
        return cryptoWalletGetBalanceMaximum (core)
            .map { Amount (core: $0, take: false) }
    }

    /// The minimum balance
    public var balanceMinimum: Amount? {
        return cryptoWalletGetBalanceMinimum (core)
            .map { Amount (core: $0, take: false) }
    }

    /// The current state.
    public var state: WalletState {
        return WalletState (core: cryptoWalletGetState(core))
    }

    /// The default TransferFactory for creating transfers.
    ///    var transferFactory: TransferFactory { get set }

    /// An address suitable for a transfer target (receiving).  Uses the default Address Scheme
    public var target: Address {
        return targetForScheme (manager.addressScheme)
    }

    public func targetForScheme (_ scheme: AddressScheme) -> Address {
        return Address (core: cryptoWalletGetAddress (core, scheme.core), take: false)
    }

    /// TODO: `var {targets,sources}: [Address]` - for query needs?

    ///
    /// Check if `address` is in `wallet`.  The address is considered in wallet if: a) it
    /// has been used in a transaction or b) is the target address
    ///
    /// - Parameter address: the address to check
    ///
    public func hasAddress (_ address: Address) -> Bool {
        return CRYPTO_TRUE == cryptoWalletHasAddress (core, address.core);
    }

    ///
    /// The Set of TransferAttributes applicable to Transfers created for this Wallet.  Every
    /// attribute in the returned Set has a `nil` value.  Pass a subset of these to the
    /// `createTransfer()` function.  Transfer creation and attribute validation will fail if
    /// any of the _required_ attributes have a `nil` value or if any `value` is not valid itself.
    ///
    public lazy private(set) var transferAttributes: Set<TransferAttribute> = {
        return transferAttributesFor(target: nil)
    }()

    public func transferAttributesFor (target: Address?) -> Set<TransferAttribute> {
        let coreAttributes = (0..<cryptoWalletGetTransferAttributeCount(core, target?.core))
            .map { cryptoWalletGetTransferAttributeAt (core, target?.core, $0)! }
        defer { coreAttributes.forEach (cryptoTransferAttributeGive) }

        return Set (coreAttributes.map { TransferAttribute (core: cryptoTransferAttributeCopy($0), take: false) })
    }
    ///
    /// Validate a TransferAttribute.  This returns `true` if the attributes value is valid and,
    /// if the attribute's value is required, if is it not `nil`.
    ///
    /// - Parameter attribute: The attribute to validate
    ///
    public func validateTransferAttribute (_ attribute: TransferAttribute) -> TransferAttributeValidationError? {
        let coreAttribute = attribute.core

        var validates: BRCryptoBoolean = CRYPTO_TRUE
        let error = cryptoWalletValidateTransferAttribute (core, coreAttribute, &validates)
        return CRYPTO_TRUE == validates ? nil : TransferAttributeValidationError (core: error)
    }

    ///
    /// Validate a Set of TransferAttributes.  This should be called prior to `createTransfer`
    /// (otherwise `createTransfer` will fail).  This checks the Set as a whole given that their
    /// might be relationships between the attributes
    ///
    /// - Note: Relationships between attributes are not explicitly provided in the interface
    ///
    /// - Parameter attributes: the set of attributes to validate
    ///
    public func validateTransferAttributes (_ attributes: Set<TransferAttribute>) -> TransferAttributeValidationError? {
        let coreAttributesCount = attributes.count
        var coreAttributes: [BRCryptoTransferAttribute?] = attributes.map { $0.core }

        var validates: BRCryptoBoolean = CRYPTO_TRUE
        let error = cryptoWalletValidateTransferAttributes (core,
                                                            coreAttributesCount,
                                                            &coreAttributes,
                                                            &validates)
        return CRYPTO_TRUE == validates ? nil : TransferAttributeValidationError (core: error)
    }


    /// The transfers of currency yielding `balance`
    public var transfers: [Transfer] {
        var transfersCount: BRCryptoCount = 0
        let transfersPtr = cryptoWalletGetTransfers(core, &transfersCount);
        defer { if let ptr = transfersPtr { cryptoMemoryFree (ptr) } }
        
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
    /// Create a transfer for wallet.  If attributes are provided and they don't validate, then
    /// `nil` is returned.  Creation will fail if the amount exceeds the wallet's balance.
    ///
    /// Generates events: TransferEvent.created and WalletEvent.transferAdded(transfer).
    ///
    /// - Parameters:
    ///   - listener: The transfer listener
    ///   - source: The source spends 'amount + fee'
    ///   - target: The target receives 'amount
    ///   - amount: The amount
    ///   - feeBasis: The basis for 'fee'
    ///   - attributes: Optional transfer attributes.
    ///
    /// - Returns: A new transfer
    ///
    public func createTransfer (target: Address,
                                amount: Amount,
                                estimatedFeeBasis: TransferFeeBasis,
                                attributes: Set<TransferAttribute>? = nil) -> Transfer? {
        if nil != attributes && nil != self.validateTransferAttributes(attributes!) {
            return nil
        }

        let coreAttributesCount = attributes?.count ?? 0
        var coreAttributes: [BRCryptoTransferAttribute?] = attributes?.map { $0.core } ?? []

        return cryptoWalletManagerCreateTransfer (manager.core, core, target.core, amount.core,
                                                  estimatedFeeBasis.core,
                                                  coreAttributesCount,
                                                  &coreAttributes)
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

    /// MARK: Estimate Limit

    ///
    /// A `Wallet.EstimateLimitHandler` is a function th handle the result of `Wallet.estimateLimit`
    /// with return type of `Amount`.
    ///
    public typealias EstimateLimitHandler = (Result<Amount,LimitEstimationError>) -> Void

    ///
    /// Estimate the maximum amount that can be transfered from Wallet.  This value does not
    /// include the fee, however, a fee estimate has been performed and the maximum has been
    /// adjusted to be (nearly) balance = amount + fee.  That is, the maximum amount is what you
    /// can safe transfer to 'zero out' the wallet
    ///
    /// In cases where `balance < fee` then .insufficientFunds is returned.  This can occur for
    /// an ERC20 transfer where the ETH wallet's balance is not enough to pay the fee.  That is,
    /// the .insufficientFunds check respects the wallet from which fees are extracted.  Both
    /// BTC and ETH transfer might have an insufficient balance to pay a fee.
    ///
    /// This is an synchronous function that returns immediately but will call `completion` once
    /// the maximum has been determined.
    ///
    /// The returned Amount is always in the wallet's currencyh.
    ///
    /// - Parameters:
    ///   - target: the target address
    ///   - fee: the network fees
    ///   - completion: the handler for the results
    ///
    public func estimateLimitMaximum (target: Address,
                                      fee: NetworkFee,
                                      completion: @escaping Wallet.EstimateLimitHandler) {
        estimateLimit (asMaximum: true, target: target, fee: fee, completion: completion)
    }

    ///
    /// Estimate the minimum amount that can be transfered from Wallet.  This value does not
    /// include the fee, however, a fee estimate has been performed.  Generally the minimum
    /// amount in zero; however, some currencies have minimum values, below which miners will
    /// reject.  In those casaes the minimum amount is above zero.
    ///
    /// In cases where `balance < amount + fee` then .insufficientFunds is returned.  The
    /// .insufficientFunds check respects the wallet from which fees are extracted.
    ///
    /// This is an synchronous function that returns immediately but will call `completion` once
    /// the maximum has been determined.
    ///
    /// The returned Amount is always in the wallet's currencyh.
    ///
    /// - Parameters:
    ///   - target: the target address
    ///   - fee: the network fees
    ///   - completion: the handler for the results
    ///
    public func estimateLimitMinimum (target: Address,
                                      fee: NetworkFee,
                                      completion: @escaping Wallet.EstimateLimitHandler) {
        estimateLimit (asMaximum: false, target: target, fee: fee, completion: completion)
    }

    ///
    /// Internal function to handle limit estimation
    ///
    internal func estimateLimit (asMaximum: Bool,
                                 target: Address,
                                 fee: NetworkFee,
                                 completion: @escaping Wallet.EstimateLimitHandler) {
        var needFeeEstimate: BRCryptoBoolean = CRYPTO_TRUE
        var isZeroIfInsuffientFunds: BRCryptoBoolean = CRYPTO_FALSE;

        // This `amount` is in the `unit` of `wallet`
        guard let amount = cryptoWalletManagerEstimateLimit (self.manager.core,
                                                             self.core,
                                                             (asMaximum ? CRYPTO_TRUE : CRYPTO_FALSE),
                                                             target.core,
                                                             fee.core,
                                                             &needFeeEstimate,
                                                             &isZeroIfInsuffientFunds)
            .map ({ Amount (core: $0, take: false)})
            else {
                // This is extraneous as `cryptoWalletEstimateLimit()` always returns an amount
                estimateLimitCompleteInQueue (completion,
                                              Result.failure (LimitEstimationError.insufficientFunds))
                return;
        }

        // If we don't need an estimate, then we invoke `completion` and skip out immediately.  But
        // include a check on a zero amount - which indicates insufficient funds.
        if CRYPTO_FALSE == needFeeEstimate {
            estimateLimitCompleteInQueue (completion,
                                          (CRYPTO_TRUE == isZeroIfInsuffientFunds && amount.isZero
                                            ? Result.failure (LimitEstimationError.insufficientFunds)
                                            : Result.success (amount)))
            return
        }

        // We need an estimate of the fees.

        // The currency for the fee
        let currencyForFee = fee.pricePerCostFactor.currency

        guard let walletForFee = self.manager.wallets
            .first (where: { $0.currency == currencyForFee })
            else {
                estimateLimitCompleteInQueue(completion, Result.failure (LimitEstimationError.serviceError))
                return

        }

        // Skip out immediately if we've no balance.
        if walletForFee.balance.isZero {
            estimateLimitCompleteInQueue (completion, Result.failure (Wallet.LimitEstimationError.insufficientFunds))
            return
        }

        //
        // If the `walletForFee` differs from `wallet` then we just need to estimate the fee
        // once.  Get the fee estimate and just ensure that walletForFee has sufficient balance
        // to pay the fee.
        //
        if self != walletForFee {
            // This `amount` will not unusually be zero.
            // TODO: Does ETH fee estimation work if the ERC20 amount is zero?
            self.estimateFee (target: target, amount: amount, fee: fee) {
                (res: Result<TransferFeeBasis, Wallet.FeeEstimationError>) in
                switch res {
                case .success (let feeBasis):
                    completion (walletForFee.balance >= feeBasis.fee
                        ? Result.success(amount)
                        : Result.failure(LimitEstimationError.insufficientFunds))

                case .failure (let error):
                    completion (Result.failure (LimitEstimationError.fromFeeEstimationError(error)))
                }
            }
            return
        }

        // The `fee` is in the same unit as the `wallet`

        //
        // If we are estimating the minimum, then get the fee and ensure that the wallet's
        // balance is enough to cover the (minimum) amount plus the fee
        //
        if !asMaximum {
            self.estimateFee (target: target, amount: amount, fee: fee) {
                (res: Result<TransferFeeBasis, Wallet.FeeEstimationError>) in
                switch res {
                case .success (let feeBasis):
                    guard let transactionAmount = amount + feeBasis.fee
                        else { preconditionFailure() }

                    completion (self.balance >= transactionAmount
                        ? Result.success (amount)
                        : Result.failure (LimitEstimationError.insufficientFunds))

                case .failure (let error):
                    completion (Result.failure (LimitEstimationError.fromFeeEstimationError(error)))
                }
            }
            return
        }

        // If the `walletForFee` and `wallet` are identical, then we need to iteratively estimate
        // the fee and adjust the amount until the fee stabilizes.
        var transferFee = Amount.create (integer: 0, unit: self.unit)

        // We'll limit the number of iterations
        let estimationCompleterRecurseLimit = 3
        var estimationCompleterRecurseCount = 0

        // This function will be recursively defined
        func estimationCompleter (res: Result<TransferFeeBasis, Wallet.FeeEstimationError>) {
            // Another estimation completed
            estimationCompleterRecurseCount += 1

            // Check the result
            switch res {
            case .success (let feeBasis):
                // The estimated transfer fee
                let newTransferFee = feeBasis.fee

                // The estimated transfer amount, updated with the transferFee
                guard let newTransferAmount = amount.sub (newTransferFee)
                    else { preconditionFailure() }

                // If the two transfer fees match, then we have converged
                if transferFee == newTransferFee {
                    guard let transactionAmount = newTransferAmount + newTransferFee
                        else { preconditionFailure() }
                    
                    completion (self.balance >= transactionAmount
                        ? Result.success (newTransferAmount)
                        : Result.failure (Wallet.LimitEstimationError.insufficientFunds))

                }

                else if estimationCompleterRecurseCount < estimationCompleterRecurseLimit {
                    // but is they haven't converged try again with the new amount
                    transferFee = newTransferFee
                    self.estimateFee (target: target, amount: newTransferAmount, fee: fee, completion: estimationCompleter)
                }

                else {
                    // We've tried too many times w/o convergence; abort
                    completion (Result.failure (Wallet.LimitEstimationError.serviceError))
                }

            case .failure (let error):
                completion (Result.failure (LimitEstimationError.fromFeeEstimationError(error)))
            }
        }

        estimateFee (target: target, amount: amount, fee: fee, completion: estimationCompleter)
    }

    private func estimateLimitCompleteInQueue (_ completion: @escaping Wallet.EstimateLimitHandler,
                                               _ result: Result<Amount, Wallet.LimitEstimationError>) {
        system.queue.async {
            completion (result)
        }
    }

    public enum LimitEstimationError: Error {
        case serviceUnavailable
        case serviceError
        case insufficientFunds

        static func fromStatus (_ status: BRCryptoStatus) -> LimitEstimationError {
            switch status {
            case CRYPTO_ERROR_FAILED: return .serviceError
            default: return .serviceError // preconditionFailure ("Unknown FeeEstimateError")
            }
        }

        static func fromFeeEstimationError (_ error: FeeEstimationError) -> LimitEstimationError{
            switch error {
            case .ServiceUnavailable: return .serviceUnavailable
            case .ServiceError:       return .serviceError
            case .InsufficientFunds:  return .insufficientFunds
            }
        }
    }


    /// MARK: Estimate Fee

    /// A `Wallet.EstimateFeeHandler` is a function to handle the result of a Wallet.estimateFee.
    public typealias EstimateFeeHandler = (Result<TransferFeeBasis,FeeEstimationError>) -> Void

    ///
    /// Estimate the fee for a transfer with `amount` from `wallet`.  If provided use the `feeBasis`
    /// otherwise use the wallet's `defaultFeeBasis`
    ///
    /// - Parameters:
    ///   - target: the transfer's target address
    ///   - amount: the transfer amount MUST BE GREATER THAN 0
    ///   - fee: the network fee (aka priority)
    ///   - completion: handler function
    ///
    public func estimateFee (target: Address,
                             amount: Amount,
                             fee: NetworkFee,
                             completion: @escaping Wallet.EstimateFeeHandler) {
        // 'Redirect' up to the 'manager'
        cryptoWalletManagerEstimateFeeBasis (self.manager.core,
                                             self.core,
                                             callbackCoordinator.addWalletFeeEstimateHandler(completion),
                                             target.core,
                                             amount.core,
                                             fee.core)
    }
    
    internal func estimateFee (sweeper: WalletSweeper,
                               fee: NetworkFee,
                               completion: @escaping EstimateFeeHandler) {
        cryptoWalletManagerEstimateFeeBasisForWalletSweep (self.manager.core,
                                                           self.core,
                                                           callbackCoordinator.addWalletFeeEstimateHandler(completion),
                                                           sweeper.core,
                                                           fee.core)
    }
    
    internal func estimateFee (request: PaymentProtocolRequest,
                               fee: NetworkFee,
                               completion: @escaping EstimateFeeHandler) {
        cryptoWalletManagerEstimateFeeBasisForPaymentProtocolRequest (self.manager.core,
                                                                      self.core,
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

