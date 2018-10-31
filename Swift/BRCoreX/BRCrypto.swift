//
//  BRCrypto.swift
//  BRCoreX
//
//  Created by Ed Gamble on 11/5/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//
import Foundation

///
/// MARK: Currency
///
public enum Currency: Equatable, CustomStringConvertible  {
    case bitcoin
    case bitcash
    case ethereum
    case token (code: String, symbol: String, name: String, description: String, unit: Unit!)
    // Note: 'fiat' is a token, albeit a "Gov't issued token".

    public var code: String {
        switch self {
        case .bitcoin:  return "BTC"
        case .bitcash:  return "BCH"
        case .ethereum: return "ETH"
        case .token(let code, _, _, _, _): return code
        }
    }

    public var symbol: String {
        switch self {
        case .bitcoin,
             .bitcash:
            if  #available(iOS 10, *) {
                return "₿"
            }
            else {
                return "Ƀ"
            }
        case .ethereum: return "Ξ"
        case .token(_, let symbol, _, _, _): return symbol
        }
    }

    public var name: String {
        switch self {
        case .bitcoin: return "Bitcoin"
        case .bitcash: return "Bitcash"
        case .ethereum: return "Ethereum"
        case .token(_, _, let name, _, _): return name
        }
    }

    public var defaultUnit: Unit {
        switch self {
        case .bitcoin:  return Unit.Bitcoin.BITCOIN
        case .bitcash:  return Unit.Bitcoin.BITCOIN
        case .ethereum: return Unit.Ethereum.ETHER
        case .token(_, _, _, _, let unit): return unit!
        }
    }

    public var description : String {
        switch self {
        case .bitcoin:  return "BTC"
        case .bitcash:  return "BCH"
        case .ethereum: return "ETH"
        case .token(_, _, _, let description, _): return description
        }
    }

    public static func == (lhs: Currency, rhs: Currency) -> Bool {
        switch (lhs, rhs) {
        case (.bitcoin, .bitcoin):   return true
        case (.bitcash, .bitcash):   return true
        case (.ethereum, .ethereum): return true
        case (.token(let s1, _, _, _, _), .token(let s2, _, _, _, _)): return s1 == s2
        default: return false
        }
    }

    /// Initialize a Currency.token() an create the 'integer' unit in the process
    public init (code: String, symbol: String, name: String, description: String,
                 unit: (name:String, symbol:String)) {
        self = .token (code: code, symbol: symbol, name: name, description: description, unit: nil)
        // Really?
        self = .token (code: code, symbol: symbol, name: name, description: description,
                        unit: Unit (unit.name, unit.symbol, self))
    }

    // Create a 'decimal' unit for token
    public func tokenDecimalUnit (name: String, symbol: String, decimals: UInt8) -> Unit? {
        switch self {
        case .bitcoin, .bitcash, .ethereum: return nil
        case .token(_, _, _, _, let unit):
            return Unit (name, symbol, UInt64(10).pow(decimals), base: unit!)
        }
    }
}

///
/// Unit
///
public class Unit {
    let currency: Currency
    let base: Unit!
    let name: String
    let symbol: String
    let scale: UInt64

    func isCompatible (_ that: Unit) -> Bool {
        return (self === that ||
            (self.base != nil && that.base != nil && self.base === that.base))
    }

    init (_ name: String, _ symbol: String, _ currency: Currency) {
        self.currency = currency
        self.base = nil
        self.name = name
        self.symbol = symbol
        self.scale = 1
    }

    init (_ name: String, _ symbol: String,  _ scale: UInt64, base: Unit) {
        self.currency = base.currency
        self.base = base
        self.name = name
        self.symbol = symbol
        self.scale = scale
    }

    public struct Bitcoin {
        static let SATOSHI = Unit ("sat",     "X", Currency.bitcoin)
        // TODO: ...
        static let BITCOIN = Unit ("bitcoin", "B", 100000000, base: SATOSHI)
    }

    public struct Ethereum {
        static let WEI   = Unit ("WEI",   "X", Currency.ethereum)
        // TODO: ...
        static let GEWI  = Unit ("GWEI",  "X",          1000000000, base: WEI)
        static let ETHER = Unit ("Ether", "X", 1000000000000000000, base: WEI)
    }

    public struct ERC20 {
        static let BRD = Currency (code: "BRD", symbol: "BRD", name: "Bread", description: "Bread",
                                   unit: (name: "BRDInteger", symbol: "BRDInteger"))
        public struct BRDUnits {
            static let BRDDecimal = BRD.tokenDecimalUnit(name: "BRD", symbol: "BRD", decimals: 18)
        }
    }

    public struct Fiat {
        static let US = Currency (code: "USD", symbol: "USD", name: "dollar", description: "US Currency",
                                  unit: (name: "cents", symbol: "c"))
        // TODO: Fill unit - better create default.
        public struct USD {
            static let Cent   = US.defaultUnit
            static let Dollar = US.tokenDecimalUnit(name: "dollar", symbol: "$", decimals: 2)
        }

        static let JP = Currency (code: "JPY", symbol: "JPY", name: "yen", description: "JP currency",
                                  unit: (name: "Yen", symbol: "Y"))
        public struct JPY {
            static let Yen = JP.defaultUnit
        }
    }
}


///
/// MARK: - Amount
///


///
/// An amount of currency.  This can be negative (as in, 'currency owed' rather then 'currency
/// owned').  Supports basic arithmetic operations (addition, subtraction, comparison); will
/// assert on !isCompatible for mismatched currency.
///
public protocol Amount {

    // The unit
    var unit: Unit { get }

    // The value as a Double, if representable
    var double: Double? { get }  // DecimalNumber

    // The value in `unit` as a Double, if representable
    func double (in unit: Unit) -> Double?

    // Add as `self + that`, converting `that` to self's unit.
    func add (_ that: Amount) -> Amount?

    // Subtrance as `self - that`, converting `that` to self's unit.
    func sub (_ that: Amount) -> Amount?

    func scale (by value: Double, unit: Unit) -> Amount?

    // If negative -> -1, if zero -> 0, if positive, +1
    var sign: Int { get }

    func equals (_ that: Amount) -> Bool

    func lessThan (_ that: Amount) -> Bool

    init<T: SignedInteger> (value:T, unit: Unit)
}

extension Amount {
    public var currency: Currency {
        return unit.currency
    }

    public func isCompatible (_ that: Amount) -> Bool {
        return unit.isCompatible(that.unit)
    }

    public var double: Double? {
        return double (in: unit)
    }

    public static func + (lhs: Amount, rhs: Amount) -> Amount? {
        return lhs.add (rhs)
    }

    public static func - (lhs: Amount, rhs: Amount) -> Amount? {
        return lhs.sub (rhs)
    }

    public static func == (lhs: Amount, rhs: Amount) -> Bool {
        return lhs.equals (rhs)
    }

    public static func < (lhs: Amount, rhs: Amount) -> Bool {
        return lhs.lessThan(rhs)
    }

    public static func <= (lhs: Amount, rhs: Amount) -> Bool {
        return lhs.lessThan(rhs) || lhs.equals(rhs)
    }

    // ...
}

///
/// MARK: - Currency Pair
///

/*
 * Ref: https://en.wikipedia.org/wiki/Currency_pair
 * A currency pair is the quotation of the relative value of a currency unit against the unit of
 * another currency in the foreign exchange market. The currency that is used as the reference is
 * called the counter currency, quote currency or currency and the currency that is quoted in
 * relation is called the base currency or transaction currency.
 *
 * The quotation EUR/USD 1.2500 means that one euro is exchanged for 1.2500 US dollars. Here, EUR
 * is the base currency and USD is the quote currency(counter currency).
 */
public protocol CurrencyPair: CustomStringConvertible {

    /// In EUR/USD=1.2500, the `baseCurrecny` is EUR.
    var baseCurrency: Unit { get }

    /// In EUR/USD=1.250, the `quoteCurrecny` is USD.
    var quoteCurrency: Unit { get }

    /// In EUR/USD=1.2500, the `exchangeRate` is 1.2500 which represents the number of USD that
    /// one EUR can be exchanged for.
    var exchangeRate: Double /* something */ { get }

    ///
    /// Apply `self` CurrencyPair to convert `asBase` (in `baseCurrency`) to `quoteCurrency`.  This
    /// is essentially `asBase * exchangeRate`
    ///
    /// - Parameter amount: the amount of `baseCurrency`
    ///
    /// - Returns: the amount as `quoteCurrency`
    ///
    func exchange (asBase amount: Amount) -> Amount

    ///
    /// Apply `self` CurrencyPair to convert `asQuote` (in `quoteCurrency`) to `baseCurrency`.  This
    /// is essentially `asQuote / exchangeRate`.
    ///
    /// - Parameter amount: the amount of `quoteCurrency`
    ///
    /// - Returns: the amount as `baseCurrency`
    ///
    func exchange (asQuote amount: Amount) -> Amount
}

extension CurrencyPair {
    public var description: String {
        return "\(baseCurrency)/\(quoteCurrency)=\(exchangeRate)"
    }
}


///
/// MARK: - Account
///

/// An `Accouint` represents the User's paperKey.  An App generally has one instace of an
/// account; that account generates bitcoin, ethereum, ... addresses
public protocol Account {

    // conceptually - for security reasaons, not stored.
    var paperKey : String { get }
    var keyPair : KeyPair { get }
}

///
/// MARK: - Key
///
public protocol Key {}

public protocol KeyPair {
    var privateKey : Key { get }
    var publicKey : Key { get }
}

///
/// MARK: - Network
///

/// The Network
public enum Network : Hashable {   // hashable
    case bitcoin  (main: Bool, name: String)
    case bitcash  (main: Bool, name: String)
    case ethereum (main: Bool, name: String, identifier: UInt)
//    case fiat?? currency by 'locale'

    public var hashValue: Int {
        switch self {
        case .bitcoin( _, let name): return name.hashValue
        case .bitcash( _, let name): return name.hashValue
        case .ethereum(_, let name, _): return name.hashValue
        }
    }

    public static func == (lhs: Network, rhs: Network) -> Bool {
        switch (lhs, rhs) {
        case (.bitcoin(_, let n1), .bitcoin(_, let n2)): return n1 == n2
        case (.bitcash(_, let n1), .bitcash(_, let n2)): return n1 == n2
        case (.ethereum (_, let n1, _), .ethereum (_, let n2, _)): return n1 == n2
        default: return false
        }
    }

    public var currency: Currency {
        switch self {
        case .bitcoin: return Currency.bitcoin
        case .bitcash: return Currency.bitcash
        case .ethereum: return Currency.ethereum
        }
    }
    struct Bitcoin {
        static let mainnet = Network.bitcoin (main: true,  name: "Bitcoin Mainnet")
        static let testnet = Network.bitcoin (main: false, name: "Bitcoin Testnet")
    }

    struct Bitcash {
        static let mainnet = Network.bitcash(main: true,  name: "Bitcash Mainnet")
        static let testnet = Network.bitcash(main: false, name: "Bitcash Testnet")
    }

    struct Ethereum {
        static let mainnet = Network.ethereum (main: true, name: "Foo", identifier: 1)
        // ...
    }
}

///
/// MARK: - Address
///


public protocol Address: CustomStringConvertible {
    // network
    // currency: bitcoin, bitcash, ethereum


}

///
/// MARK: - Transfer
///


/// A Transfer represents the transfer of an `amount` of currency from `source` to `target`.  A
/// Transfer is held in a `Wallet` (holding the amount's currency); the Transfer requires a `fee`
/// to complete.  Once the transfer is signed/submitted it can be identified by a `TransferHash`.
public protocol Transfer : class {

    /// The owning wallet
    var wallet: Wallet { get }

    /// The source pays the fee and sends the amount.
    var source: Address? { get }

    /// The target receives the amount
    var target: Address? { get }

    /// The amount to transfer
    var amount: Amount { get }

    /// The fee paid - before the transfer is confirmed, this is the estimated fee.
    var fee: Amount { get }

    /// The basis for the fee.
    var feeBasis: TransferFeeBasis { get }

    /// An optional confirmation.
    var confirmation: TransferConfirmation? { get }

    /// An optional hash
    var hash: TransferHash? { get }

    /// The current state
    var state: TransferState { get }
}

/// A TransferFeeBasis is use to estimate the fee to complete a transfer
public enum TransferFeeBasis {
    case bitcoin  (foo: Amount)
    case ethereum (gasPrice: Amount, gasLimit: UInt64)
}

/// A TransferConfirmation holds confirmation information.
public protocol TransferConfirmation {
    // block Number
    // transaction Index
    // timestamp
    // fee
}

/// A TransferHash uniquely identifies a transfer *among* the owning wallet's transfers.
public struct TransferHash: Hashable, CustomStringConvertible {
    public let description: String

    public var hashValue: Int {
        return description.hashValue
    }

    public static func == (lhs: TransferHash, rhs: TransferHash) -> Bool {
        return lhs.description == rhs.description
    }
}

/// A TransferState represents the states in Transfer's 'life-cycle'
public enum TransferState {
    case created
    case signed
    case submitted
    case pending
    case included (confirmation: TransferConfirmation)
    case failed (reason:String)
    case deleted
}

/// A TransferEvent represents a asynchronous announcment of a transfer's state change.
public enum TransferEvent {
    case created
    case changed (old: TransferState, new: TransferState)
    case deleted
}

///
/// A `TransferFectory` is a customization point for `Transfer` creation.
///
public protocol TransferFactory {
    ///
    /// Create a transfer in `wallet`
    ///
    /// - Parameters:
    ///   - source: The source spends 'amount + fee'
    ///   - target: The target receives 'amount'
    ///   - amount: The amount
    ///   - feeBasis: The basis for the 'fee'
    ///
    /// - Returns: A new transfer
    ///
    func createTransfer (wallet: Wallet,
                         source: Address,
                         target: Address,
                         amount: Amount,
                         feeBasis: TransferFeeBasis) -> Transfer
}

///
/// MARK: - Wallet
///

///
/// A Wallet holds a balance for a single currency and holds the transfers.
///
public protocol Wallet: class {

    /// The owning manager
    var manager: WalletManager { get }

    /// The name
    var name: String { get }

    /// The current balance for currency
    var balance: Amount { get }

    /// The transfers of currency yielding `balance`
    var transfers: [Transfer] { get }

    /// The current state.
    var state: WalletState { get }

    /// The default TransferFeeBasis for created transfers.
    var defaultFeeBasis: TransferFeeBasis { get set }

    /// The default TransferFactory for creating transfers.
    var transferFactory: TransferFactory { get set }

    // func sign (transfer: Transfer)
    // submit
    // ... cancel, replace - if appropriate
}

extension Wallet {
    ///
    /// Create a transfer for wallet.  Invokes the wallet's transferFactory to create a transfer.
    /// Generates events: TransferEvent.created and WalletEvent.transferAdded(transfer).
    ///
    /// - Parameters:
    ///   - source: The source spends 'amount + fee'
    ///   - target: The target receives 'amount
    ///   - amount: The amount
    ///   - feeBasis: Teh basis for 'fee'
    ///
    /// - Returns: A new transfer
    ///
    func createTransfer (source: Address,
                         target: Address,
                         amount: Amount,
                         feeBasis: TransferFeeBasis) -> Transfer {
        return transferFactory.createTransfer (wallet: self,
                                               source: source,
                                               target: target,
                                               amount: amount,
                                               feeBasis: feeBasis)
    }

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
    func createTransfer (source: Address,
                         target: Address,
                         amount: Amount) -> Transfer {
        return createTransfer (source: source,
                               target: target,
                               amount: amount,
                               feeBasis: defaultFeeBasis)
    }

    /// The currency held in wallet.
    var currency: Currency {
        return balance.currency
    }

    /// The (default) name derived from the currency
    var name: String {
        return currency.name
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
}

///
/// A WalletEvent represents a asynchronous announcment of a wallet's state change.
///
public enum WalletEvent {
    case created
    case transferAdded   (transfer: Transfer)
    case transferChanged (transfer: Transfer)
    case transferDeleted (transfer: Transfer)
    case balanceUpdated  (amount: Amount)
    case feeBasisUpdated (feeBasis: TransferFeeBasis)
    case deleted
}

///
/// A WalletFactory is a customization point for Wallet creation.
/// TODO: ?? AND HOW DOES THIS FIT WITH CoreWallet w/ REQUIRED INTERFACE TO Core ??
///
public protocol WalletFactory {
    ///
    /// Create a Wallet managed by `manager` and holding `currency`.  The wallet is initialized
    /// with a 0 balance,no transfers and some default feeBasis (appropriate for the `currency`).
    /// Generates events: WalletEvent.created (maybe others).
    ///
    /// - Parameters:
    ///   - manager: the Wallet's manager
    ///   - currency: The currency held
    ///
    /// - Returns: A new wallet
    ///
    func createWallet (manager: WalletManager,
                       currency: Currency) -> Wallet
}

///
/// MARK: -  WalletManager
///

///
/// A WallettManager manages one or more wallets one of which is designated the `primaryWallet`.
/// (For example, an EthereumWalletManager will manage an ETH wallet and one wallet for each
/// ERC20Token; the ETH wallet will be the primaryWallet.  A BitcoinWalletManager manages one
/// and only one wallet holding BTC.).
///
/// At least conceptuall, a WalletManager is an 'Active Object' (whereas Transfer and Wallet are
/// 'Passive Objects'
///
public protocol WalletManager : class {

    /// The client receives Wallet, Transfer and perhaps other asynchronous events.  In some cases,
    /// the client must perform some required manager functionality - for example, saving some
    /// state on behalf of the manager.
    var client: WalletManagerClient { get }

    /// The account
    var account: Account { get }

    /// The network
    var network: Network { get }

    /// The primaryWallet
    var primaryWallet: Wallet { get }

    /// The managed wallets - often will just be [primaryWallet]
    var wallets: [Wallet] { get }

    // The mode determines how the manager manages the account and wallets on network
    var mode: WalletManagerMode { get }

    // The file-system path to use for persistent storage.
    var path: String { get }  // persistent storage

    var state: WalletManagerState { get }

    /// The default WalletFactory for creating wallets.
    var walletFactory: WalletFactory { get set }

    /// Connect to network and begin managing wallets for account
    func connect ()

    /// Disconnect from the network.
    func disconnect ()

    /// isConnected
    /// sync(...)
    /// isSyncing

    /// sign(transfer)
    /// submit(transfer)
}

extension WalletManager {
    ///
    /// Create a wallet for `currency`.  Invokdes the manager's `walletFactory` to create the
    /// wallet.  Generates events: Wallet.created, WalletManager.walletAdded(wallet), perhaps
    /// others.
    ///
    /// - Parameter currency: the wallet's currency
    ///
    /// - Returns: a new wallet.
    ///
    func createWallet (currency: Currency) -> Wallet {
        return walletFactory.createWallet (manager: self,
                                           currency: currency)
    }

    /// The primaryWallet's currency.
    var currency: Currency {
        return primaryWallet.currency
    }

    /// A manager `isActive` if connected or syncing
    var isActive: Bool {
        return state == .connected || state == .syncing
    }
}

/// The WalletManagers state.
public enum WalletManagerState {
    case created
    case disconnected
    case connected
    case syncing
    case deleted
}

///
/// A WalletManager Event represents a asynchronous announcment of a managera's state change.
///
public enum WalletManagerEvent {
    case created
    case deleted

    case walletAdded (wallet: Wallet)
    case walletChanged (wallet: Wallet)
    case walletDeleted (wallet: Wallet)

    case syncStarted
    case syncProgress (percentComplete: Double)
    case syncEnded
}

/// The WalletManager's mode determines how the account and associated wallets are managed on
/// network.
///
/// - api_only: Use only the defined 'Cloud-Based API' to synchronize the account's transfers.
///
/// - api_with_p2p_submit: Use the defined 'Cloud-Based API' to synchronize the account's transfers
///      but submit transfers using the network's Peer-to-Peer protocol.
///
/// - p2p_with_api_sync: Use the network's Peer-to-Peer protocol to synchronize the account's
///      recents transfers but use the 'Cloud-Based API' to synchronize older transfers.
///
/// - p2p_only: Use the network's Peer-to-Peer protocol to synchronize the account's transfers.
///
public enum WalletManagerMode {
    case api_only
    case api_with_p2p_submit
    case p2p_with_api_sync
    case p2p_only
}


///
/// A WalletManagerClient recieves asynchronous events announcing state changes to Managers, to
/// Wallets and to Transfers.  This is an application's sole mechanism to learn of asynchronous
/// state changes.
///
/// Note: This must be 'class bound' as the WalletManger holds an 'unowned' reference
///
public protocol WalletManagerClient : class {
    ///
    /// Handle a WalletManagerEvent.
    ///
    /// - Parameters:
    ///   - manager: the manager
    ///   - event: the event
    ///
    func handleManagerEvent (manager: WalletManager,
                             event: WalletManagerEvent)

    ///
    /// Handle a WalletEvent
    ///
    /// - Parameters:
    ///   - manager: the manager
    ///   - wallet: the wallet
    ///   - event: the wallet event.
    ///
    func handleWalletEvent (manager: WalletManager,
                            wallet: Wallet,
                            event: WalletEvent)

    ///
    /// Handle a TranferEvent.
    ///
    /// - Parameters:
    ///   - manager: the manager
    ///   - wallet: the wallet
    ///   - transfer: the transfer
    ///   - event: the transfer event.
    ///
    func handleTransferEvent (manager: WalletManager,
                              wallet: Wallet,
                              transfer: Transfer,
                              event: TransferEvent)

    // NOTE: No save{Block,Transaction,Peer} function - handled in Core
    // NOTE: No get{Nonce,Block,Logs,Transfers} - handled in Core
}

///
/// MARK: - Currency-Specific Clients (if needed)
///
public protocol BitcoinWalletManagerClient : WalletManagerClient {}
public protocol EthereumWalletManagerClient : WalletManagerClient {}

public protocol EthereumWalletManager: WalletManager {
    init (client: WalletManagerClient,
          network: Network,
          // ...
          foo: Int)
}

