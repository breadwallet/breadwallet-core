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
/// A Wallet holds the transfers and a balance for a single currency.
///
public final class Wallet {

    internal private(set) weak var listener: WalletListener?

    internal let core: BRCryptoWallet

    /// The owning manager
    public unowned let manager: WalletManager

    public var system: System {
        return manager.system
    }

    /// The base unit for the wallet's network.  This is used for `balance` and to derive the
    /// currency and name
    public let unit: Unit

    /// The currency held in wallet.
    public var currency: Currency {
        return unit.currency
    }

    /// The (default) name derived from the currency.  For example: BTC, ETH, or BRD.
    public var name: String {
        return unit.currency.code
    }

    /// The current balance for currency
    public var balance: Amount {
        return Amount (core: cryptoWalletGetBalance (core), unit: unit)
    }


    /// The transfers of currency yielding `balance`
    public var transfers: [Transfer] {
        let listener = manager.system.listener
        return (0..<cryptoWalletGetTransferCount(core))
            .map { Transfer (core: cryptoTransferTake (cryptoWalletGetTransfer (core, $0)),
                                  listener: listener,
                                  wallet: self,
                                  unit: unit) }
    }

    /// Use a hash to lookup a transfer
    public func transferBy (hash: TransferHash) -> Transfer? {
        return transfers
            .first { $0.hash.map { $0 == hash } ?? false }
    }

    internal func transferBy (core: BRCryptoTransfer) -> Transfer? {
        return transfers
            .first { $0.core == core }
    }

    internal func transferByCoreOrCreate (_ core: BRCryptoTransfer,
                                          listener: TransferListener?,
                                          create: Bool = false) -> Transfer? {
        return transferBy (core: core) ??
            (!create
                ? nil
                : Transfer (core: core,
                            listener: listener,
                            wallet: self,
                            unit: unit))
    }

    /// The current state.
    public var state: WalletState {
        return WalletState (core: cryptoWalletGetState(core))
    }

    /// The default TransferFeeBasis for created transfers.
    public var defaultFeeBasis: TransferFeeBasis {
        get {
            return TransferFeeBasis (core: cryptoWalletGetDefaultFeeBasis (core)) }
        set {
            let defaultFeeBasis = newValue // rename, for clarity
            cryptoWalletSetDefaultFeeBasis (core, defaultFeeBasis.core);
            announceEvent (WalletEvent.feeBasisUpdated (feeBasis: defaultFeeBasis))
        }
    }

    /// The default TransferFactory for creating transfers.
//    var transferFactory: TransferFactory { get set }

    /// An address suitable for a transfer target (receiving).  Uses the default Address Scheme
    public var target: Address {
        return Address (core: cryptoWalletGetAddress (core))
    }

    /// An address suitable for a transfer source (sending).  Uses the default AddressScheme
    public var source: Address {
        return Address (core: cryptoWalletGetAddress (core))
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
    func createTransfer (target: Address,
                         amount: Amount,
                         feeBasis: TransferFeeBasis) -> Transfer? {
        return cryptoWalletCreateTransfer (core, target.core, amount.core, feeBasis.core)
            .map {
                Transfer (core: $0,
                          listener: self.manager.system.listener,
                          wallet: self,
                          unit: amount.unit)
        }
    }

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
    func estimateFee (amount: Amount,
                      feeBasis: TransferFeeBasis?) -> Amount {
        precondition (amount.hasCurrency (currency))
        let unit = manager.network.baseUnitFor (currency: manager.currency)!
        return Amount (core: cryptoWalletEstimateFee (core, amount.core, feeBasis?.core, unit.core),
                       unit: unit)
    }


    internal init (core: BRCryptoWallet,
                   listener: WalletListener?,
                   manager: WalletManager,
                   unit: Unit) {
        self.core = core
        self.listener = listener
        self.manager = manager
        //self.name = unit.currency.code
        self.unit = unit
        // self.state = WalletState.created

        print ("SYS: Wallet (\(manager.name):\(name)): Init")
//        manager.add (wallet: self)
    }

    internal func announceEvent (_ event: WalletEvent) {
        self.listener?.handleWalletEvent (system: system,
                                          manager: manager,
                                          wallet: self,
                                          event: event)
    }

    deinit {
        cryptoWalletGive (core)
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
    public func createTransfer (target: Address,
                                amount: Amount) -> Transfer? {
        return createTransfer (target: target,
                               amount: amount,
                               feeBasis: defaultFeeBasis)
    }

}

///
/// The Wallet state
///
/// - created: The wallet was created (and remains in existence).
/// - deleted: The wallet was deleted.
///
public enum WalletState {
    case created
    case deleted

    internal init (core: BRCryptoWalletState) {
        switch core {
        case CRYPTO_WALLET_STATE_CREATED: self = .created
        case CRYPTO_WALLET_STATE_DELETED: self = .deleted
        default: self = .created; precondition(false)
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

    case balanceUpdated  (amount: Amount)
    case feeBasisUpdated (feeBasis: TransferFeeBasis)
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

