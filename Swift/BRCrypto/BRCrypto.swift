//
//  BRCrypto.swift
//  BRCoreX
//
//  Created by Ed Gamble on 11/5/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//
import Foundation
import SystemConfiguration  // SCSystemReachability
import BRCore               // Core Crypto

///
/// A currency is a medium for exchange.
///
/// Each currency has a `baseUnit` and a `defaultUnit`.  Because a `Unit` holds a Currency this
/// sets up recursive definition for both `Currency` and `Unit` - thus these abstractions must be
/// represented as reference types using `class`
///
/// A Currency has a number of decimals which define the currency's defaultUnit relative to the
/// baseUnit.
///
public final class Currency {
    internal let core: BRCryptoCurrency

    /// The code; e.g. BTC
    public var code: String {
        return asUTF8String (cryptoCurrencyGetCode(core))
    }

    /// The name; e.g. Bitcoin
    public var name: String {
        return asUTF8String (cryptoCurrencyGetName (core))
    }

    /// The type:
    public var type: String {
        return asUTF8String (cryptoCurrencyGetType (core))
    }

    internal init (core: BRCryptoCurrency) {
        self.core = core
    }

    internal convenience init (name: String,
                               code: String,
                               type: String) {
        self.init (core: cryptoCurrencyCreate(name, code, type))
    }

    deinit {
        cryptoCurrencyGive (core)
    }
}

extension Currency: Equatable {
    public static func == (lhs: Currency, rhs: Currency) -> Bool {
        return CRYPTO_TRUE == cryptoCurrencyIsIdentical (lhs.core, rhs.core)
    }
}

///
/// A unit of measure for a currency.  There can be multiple units for a given currency (analogous
/// to 'System International' units of (meters, kilometers, miles, ...) for a dimension of
/// 'length').  For example, Ethereum has units of: WEI, GWEI, ETHER, METHER, ... and Bitcoin of:
/// BTC, SATOSHI, ...
///
/// Each Currency has a 'baseUnit' - which is defined as the 'integer-ish' unit - such as SATOSHI
/// ane WEI for Bitcoin and Ethereum, respectively.  There can be multiple 'derivedUnits' - which
/// are derived by scaling off of a baseUnit.
///
public final class Unit {
    internal let core: BRCryptoUnit

    public let currency: Currency

    public var name: String {
        return asUTF8String (cryptoUnitGetName (core))
    }

    public var symbol: String {
        return asUTF8String (cryptoUnitGetSymbol (core))
    }

    public private(set) lazy var base: Unit = {
        let coreBaseUnit = cryptoUnitGetBaseUnit (self.core)!
        return (CRYPTO_TRUE == cryptoUnitIsIdentical(self.core, coreBaseUnit)
            ? self
            : Unit (core: coreBaseUnit)) // alloc - no
    }()

    public var decimals: UInt8 {
        return cryptoUnitGetBaseDecimalOffset (core)
    }
    
    public func isCompatible (with that: Unit) -> Bool {
        return CRYPTO_TRUE == cryptoUnitIsCompatible (self.core, that.core)
    }

    public func hasCurrency (_ currency: Currency) -> Bool {
        return currency.core == cryptoUnitGetCurrency (core)
    }

    internal init (core: BRCryptoUnit, currency: Currency) {
        self.core = core
        self.currency = currency
    }

    internal convenience init (core: BRCryptoUnit) {
        self.init (core: core,
                   currency: Currency (core: cryptoUnitGetCurrency (core)))
    }

    internal convenience init (currency: Currency,
                               name: String,
                               symbol: String) {
        self.init (core: cryptoUnitCreateAsBase (currency.core, name, symbol),
                   currency: currency)
    }

    internal convenience init (currency: Currency,
                               name: String,
                               symbol: String,
                               base: Unit,
                               decimals: UInt8) {
        self.init (core: cryptoUnitCreate (currency.core, name, symbol, base.core, decimals),
                   currency: currency)
    }

    deinit {
        cryptoUnitGive (core)
    }
}

extension Unit: Equatable {
    public static func == (lhs: Unit, rhs: Unit) -> Bool {
        return CRYPTO_TRUE == cryptoUnitIsIdentical (lhs.core, rhs.core)
    }
}


///
/// An amount of currency.  This can be negative (as in, 'currency owed' rather then 'currency
/// owned').  Supports basic arithmetic operations (addition, subtraction, comparison); will
/// assert on !isCompatible for mismatched currency.
///
public final class Amount {
    /// The underlying Core memory reference
    internal let core: BRCryptoAmount

    /// The (default) unit.  Without this there is no reasonable implementation of
    /// CustomeStringConvertable.
    public let unit: Unit

    /// The currency
    public var currency: Currency {
        return unit.currency
    }

    public var isNegative: Bool {
        return CRYPTO_TRUE == cryptoAmountIsNegative (core)
    }

    internal func double (as unit: Unit) -> Double? {
        var overflow: BRCryptoBoolean = CRYPTO_FALSE
        let value = cryptoAmountGetDouble(core, unit.core, &overflow)
        return CRYPTO_TRUE == overflow ? nil : value
    }

     public func string (as unit: Unit) -> String? {
        return double (as: unit)
            .flatMap { self.formatterWith (unit: unit).string(from: NSNumber(value: $0)) }
    }

    public func string (pair: CurrencyPair) -> String? {
        return pair.exchange (asBase: self)?.string (as: pair.quoteUnit)
    }

    public func isCompatible (with that: Amount) -> Bool {
        return CRYPTO_TRUE == cryptoAmountIsCompatible (self.core, that.core)
    }

    public func hasCurrency (_ currency: Currency) -> Bool {
        return currency.core == cryptoAmountGetCurrency (core)
    }

    public func add (_ that: Amount) -> Amount? {
        precondition (isCompatible(with: that))
        return cryptoAmountAdd (self.core, that.core)
            .map { Amount (core: $0, unit: self.unit) }
    }

    public func sub (_ that: Amount) -> Amount? {
        precondition (isCompatible(with: that))
        return cryptoAmountSub (self.core, that.core)
            .map { Amount (core: $0, unit: self.unit) }
    }

    internal init (core: BRCryptoAmount,
                   unit: Unit) {
        self.core = core
        self.unit = unit
    }

    static func create (double: Double, unit: Unit) -> Amount {
        return Amount (core: cryptoAmountCreateDouble (double, unit.core),
                       unit: unit)
    }

    static func create (integer: Int64, unit: Unit) -> Amount {
        return Amount (core: cryptoAmountCreateInteger (integer, unit.core),
                       unit: unit)
    }

    // static func create (exactly: Double, unit: Unit) -> Amount  ==> No remainder
    //   nil == Amount.create (exactly: 1.5, unit: SATOSHI)  // remainder is 0.5

    private func formatterWith (unit: Unit) -> NumberFormatter {
        let formatter = NumberFormatter()
        formatter.locale = Locale.current
        formatter.numberStyle = .currency
        formatter.currencySymbol = unit.symbol;
        formatter.generatesDecimalNumbers = 0 != unit.decimals
        formatter.maximumFractionDigits = Int(unit.decimals)
        return formatter
    }

    deinit {
        cryptoAmountGive (core)
    }
}

extension Amount {
    public static func + (lhs: Amount, rhs: Amount) -> Amount? {
        return lhs.add(rhs)
    }

    public static func - (lhs: Amount, rhs: Amount) -> Amount? {
        return lhs.sub(rhs)
    }
}

///
/// Note that incompatible units may return 'false' for all comparisons.  This violates the
/// expectation that `lhs` and `rhs` satisfy one of: ==, >, and <.  Caution.
///
extension Amount: Comparable {
    public static func == (lhs: Amount, rhs: Amount) -> Bool {
        return CRYPTO_COMPARE_EQ == cryptoAmountCompare (lhs.core, rhs.core)
    }

     public static func < (lhs: Amount, rhs: Amount) -> Bool {
        return CRYPTO_COMPARE_LT == cryptoAmountCompare (lhs.core, rhs.core)
    }

    public static func > (lhs: Amount, rhs: Amount) -> Bool {
        return CRYPTO_COMPARE_GT == cryptoAmountCompare (lhs.core, rhs.core)
    }

    public static func != (lhs: Amount, rhs: Amount) -> Bool {
        return CRYPTO_COMPARE_EQ != cryptoAmountCompare (lhs.core, rhs.core)
    }

    public static func <= (lhs: Amount, rhs: Amount) -> Bool {
        return CRYPTO_COMPARE_GT != cryptoAmountCompare (lhs.core, rhs.core)
    }

    public static func >= (lhs: Amount, rhs: Amount) -> Bool {
        return CRYPTO_COMPARE_LT != cryptoAmountCompare (lhs.core, rhs.core)
    }
}

extension Amount: CustomStringConvertible {
    public var description: String {
        return string (as: unit) ?? "<nan>"
    }
}

///
/// "A currency pair is the quotation of the relative value of a currency unit against the unit of
/// another currency in the foreign exchange market. The currency that is used as the reference is
/// called the counter currency, quote currency or currency and the currency that is quoted in
/// relation is called the base currency or transaction currency.
///
/// "The quotation EUR/USD 1.2500 means that one euro is exchanged for 1.2500 US dollars. Here, EUR
/// is the base currency and USD is the quote currency(counter currency)."
///
/// Ref: https://en.wikipedia.org/wiki/Currency_pair
///
/// Thus BTC/USD=1000 means that one BTC is changed for $1,000.  Here, BTC is the base currency
/// and USD is the quote currency.  You would create such an exchange with:
///
///    let BTC_USD_Pair = CurrencyPair (baseUnit:  Bitcoin.Units.BTC,
///                                     quoteUnit: Fiat.USD.Dollar,
///                                     exchangeRate: 1000.0)
///
/// and then use it to find the value of 2 BTC with:
///
///    BTC_USD_Pair.exchange (asBase: Amount (value: 2.0, unit: Bitcoin.Units.BTC))
///
/// which would return: $2,000  (as Amount of 2000.0 in Fiat.USD.Dollar)
///

//public struct CurrencyPair {
//    public func exchange (asBase amount: Amount) -> Amount? {
//        return nil
//    }
//}

public struct CurrencyPair {

    /// In EUR/USD=1.2500, the `baseCurrecny` is EUR.
    public let baseUnit: Unit

    /// In EUR/USD=1.250, the `quoteCurrecny` is USD.
    public let quoteUnit: Unit

    /// In EUR/USD=1.2500, the `exchangeRate` is 1.2500 which represents the number of USD that
    /// one EUR can be exchanged for.
    public let exchangeRate: Double

    ///
    /// Apply `self` CurrencyPair to convert `asBase` (in `baseCurrency`) to `quoteCurrency`.  This
    /// is essentially `asBase * exchangeRate`
    ///
    /// - Parameter amount: the amount of `baseCurrency`
    ///
    /// - Returns: the amount as `quoteCurrency`
    ///
    public func exchange(asBase amount: Amount) -> Amount? {
        return amount.double(as: baseUnit)
            .map { Amount.create (double: $0 * exchangeRate, unit: quoteUnit) }
    }

    ///
    /// Apply `self` CurrencyPair to convert `asQuote` (in `quoteCurrency`) to `baseCurrency`.  This
    /// is essentially `asQuote / exchangeRate`.
    ///
    /// - Parameter amount: the amount of `quoteCurrency`
    ///
    /// - Returns: the amount as `baseCurrency`
    ///
    public func exchange(asQuote amount: Amount) -> Amount? {
        return amount.double (as: quoteUnit)
            .map { Amount.create (double: $0 / exchangeRate, unit: baseUnit) }
    }
}

extension CurrencyPair: CustomStringConvertible {
    public var description: String {
        return "\(baseUnit.name)/\(quoteUnit.name)=\(exchangeRate)"
    }
}

///
/// An `Accouint` represents the User's paperKey.  An App generally has one instace of an
/// account; that account generates bitcoin, ethereum, ... addresses
///
public final class Account {

    let core: BRCryptoAccount

    internal init (core: BRCryptoAccount) {
        self.core = core
    }

    public var serialize: Data {
        return Data (count: 0)
    }

    static func createFrom (phrase: String) -> Account? {
        return cryptoAccountCreate (phrase)
            .map { Account (core: $0) }
    }

    static func createFrom (serialization: Data) -> Account? {
        return nil
    }
}

///
/// A Key (unused)
///
#if false
public protocol Key {}

///
/// A KeyPair (unused)
///
public protocol KeyPair {
    var privateKey : Key { get }
    var publicKey : Key { get }
}
#endif

///
/// A Blockchain Network.  Networks are created based from a cross-product of block chain and
/// network type.  Specifically {BTC, BCH, ETH, ...} x {Mainnet, Testnet, ...}.  Thus there will
/// be networks of [BTC-Mainnet, BTC-Testnet, ..., ETH-Mainnet, ETH-Testnet, ETH-Rinkeby, ...]
///
public class Network {
    internal let core: BRCryptoNetwork

    public var name: String {
        return asUTF8String (cryptoNetworkGetName (core))
    }

    /// The native currency.  Multiple networks will have the same currency; for example,
    /// BTC-Mainnet and BTC-Testnet share the BTC currency.
    public let currency: Currency

    /// All currencies.  Multiple networks will have the same currencies.
    public let currencies: [Currency]  // when hashable =>  Set<Currency>

    public func hasCurrency (_ that: Currency) -> Bool {
        return currencies.contains(that)
    }

    public func baseUnitFor (currency: Currency) -> Unit {
        // cache these
        return Unit (core: cryptoNetworkGetUnitAsBase (core, currency.core),
                     currency: currency)
    }

    public func defaultUnitFor (currency: Currency) -> Unit {
        // cache these
        return Unit (core: cryptoNetworkGetUnitAsDefault (core, currency.core),
                     currency: currency)
    }

    public func unitsFor (currency: Currency) -> [Unit] { // Set<Unit>
        // cache these
        let _ /* coreUnits */ = cryptoNetworkGetUnits (core, currency.core);
        return []
    }

    public func hasUnitFor (currency: Currency, unit: Unit) -> Bool {
        return unitsFor (currency: currency).contains (unit)
    }

    internal init (core: BRCryptoNetwork) {
        let _ /* coreCurrencies */ = cryptoNetworkGetCurrencies (core)
        let coreCurrency   = cryptoNetworkGetCurrency (core)!

        self.currencies = [] // from coreCurrencies
        self.currency = Currency (core: coreCurrency) // from currencies
        self.core = core
    }
}

extension Network: CustomStringConvertible {
    public var description: String {
        return name
    }
}

public enum NetworkEvent {
    case created
}

///
/// An Address for transferring an amount.
///
/// - bitcoin: A bitcon-specific address
/// - ethereum: An ethereum-specific address
///
public struct Address {
    let core: BRCryptoAddress

    internal init (core: BRCryptoAddress) {
        self.core = core
    }

    ///
    /// Create an Addres from `string` and `network`.  The provided `string` must be valid for
    /// the provided `network` - that is, an ETH address (as a string) differs from a BTC address
    /// and a BTC mainnet address differs from a BTC testnet address.
    ///
    /// In practice, 'target' addresses (for receiving crypto) are generated from the wallet and
    /// 'source' addresses (for sending crypto) are a User input.
    ///
    /// - Parameters:
    ///   - string: A string representing a crypto address
    ///   - network: The network for which the string is value
    ///
    /// - Returns: An address or nil if `string` is invalide for `network`
    ///
    static func create (string: String, network: Network) -> Address? {
        return nil
    }
}

extension Address: CustomStringConvertible {
    public var description: String {
        return asUTF8String (cryptoAddressAsString (core))
    }
}

///
/// A Transfer represents the transfer of an `amount` of currency from `source` to `target`.  A
/// Transfer is held in a `Wallet` (holding the amount's currency); the Transfer requires a `fee`
/// to complete.  Once the transfer is signed/submitted it can be identified by a `TransferHash`.
/// Once the transfer has been included in the currency's blockchain it will have a
/// `TransferConfirmation`.
///
open class Transfer {
    internal let core: BRCryptoTransfer

    /// The owning wallet
    public let wallet: Wallet

    /// The source pays the fee and sends the amount.
    public let source: Address?

    /// The target receives the amount
    public let target: Address?

    /// The amount to transfer
    public let amount: Amount

    /// The fee paid - before the transfer is confirmed, this is the estimated fee.
    public internal(set) var fee: Amount

    /// The basis for the fee.
    public let feeBasis: TransferFeeBasis

    /// An optional confirmation.
    public var confirmation: TransferConfirmation? {
        if case .included (let confirmation) = state { return confirmation }
        else { return nil }
    }

    /// An optional hash
    public internal(set) var hash: TransferHash?

    /// The current state
    public internal(set) var state: TransferState

    var isSent: Bool {
        return false
    }
    // var originator: Bool

    internal init (core: BRCryptoTransfer,
                   wallet: Wallet,
                   source: Address,
                   target: Address,
                   amount: Amount,
                   fee: Amount,
                   feeBasis: TransferFeeBasis,
                   hash: TransferHash?,
                   state: TransferState) {
        self.core = core
        self.wallet = wallet
        self.source = source
        self.target = target
        self.amount = amount
        self.fee = fee
        self.feeBasis = feeBasis
        self.hash = hash
        self.state = state
    }
}

///
/// A TransferFeeBasis is use to estimate the fee to complete a transfer
///
public enum TransferFeeBasis {
    case bitcoin  (feePerKB: UInt64) // in satoshi
    case ethereum (gasPrice: Amount, gasLimit: UInt64)
}

///
/// A TransferConfirmation holds confirmation information.
///
public struct TransferConfirmation {
    public let blockNumber: UInt64
    public let transactionIndex: UInt64
    public let timestamp: UInt64
    public let fee: Amount
}

///
/// A TransferHash uniquely identifies a transfer *among* the owning wallet's transfers.
///
public struct TransferHash: Equatable {  // hashable, equatable, custom..
    let core: BRCryptoTransferHash


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
    case deleted
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
    func createTransfer (wallet: Wallet,
                         target: Address,
                         amount: Amount,
                         feeBasis: TransferFeeBasis) -> Transfer? // T
}

///
/// A Wallet holds the transfers and a balance for a single currency.
///
open class Wallet {
    internal let core: BRCryptoWallet

    /// The owning manager
    public let manager: WalletManager

    /// The base unit for the wallet's network.  This is used for `balance` and to derive the
    /// currency and name
    internal let unit: Unit

    /// The current balance for currency
    public var balance: Amount {
        return Amount (core: cryptoWalletGetBalance (core), unit: unit)
    }

    /// The transfers of currency yielding `balance`
    public internal(set) var transfers: [Transfer] = []

    /// Use a hash to lookup a transfer
    func lookup (hash: TransferHash) -> Transfer? {
        return transfers.first { $0.hash ==  hash }
    }

    /// The current state.
    public internal(set) var state: WalletState

    /// The default TransferFeeBasis for created transfers.
    public var defaultFeeBasis: TransferFeeBasis

    /// The default TransferFactory for creating transfers.
    public var transferFactory: TransferFactory

    /// An address suitable for a transfer target (receiving).  Uses the default Address Scheme
    public var target: Address {
        return Address (core: cryptoWalletGetAddress (core))
    }

    internal init (core: BRCryptoWallet,
                   manager: WalletManager,
                   name: String,
                   unit: Unit,
                   state: WalletState,
                   defaultFeeBasis: TransferFeeBasis,
                   transferFactory: TransferFactory) {
        self.core = core
        self.manager = manager
        self.unit = unit
        self.state = state
        self.defaultFeeBasis = defaultFeeBasis
        self.transferFactory = transferFactory
    }
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
    public func createTransfer (target: Address,
                                amount: Amount,
                                feeBasis: TransferFeeBasis) -> Transfer? {
        return transferFactory.createTransfer (wallet: self,
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
    public func createTransfer (target: Address,
                                amount: Amount) -> Transfer? {
        return createTransfer (target: target,
                               amount: amount,
                               feeBasis: defaultFeeBasis)
    }

    /// The currency held in wallet.
    public var currency: Currency {
        return unit.currency
    }

    /// The (default) name derived from the currency.  For example: BTC, ETH, or BRD.
    public var name: String {
        return unit.currency.code
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

extension WalletEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case .created:         return "Created"
        case .transferAdded:   return "TransferAdded"
        case .transferChanged: return "TransferChanged"
        case .transferDeleted: return "TransferDeleted"
        case .balanceUpdated:  return "BalanceUpdated"
        case .feeBasisUpdated: return "FeeBasisUpdated"
        case .deleted:         return "Deleted"
        }
    }
}

///
/// An AddressScheme generates addresses for a wallet.  Depending on the scheme, a given wallet may
/// generate different address.  For example, a Bitcoin wallet can have a 'Segwit/BECH32' address
/// scheme or a 'Legacy' address scheme.
///
public protocol AddressScheme {
    associatedtype W: Wallet

    // Generate a 'receive' (aka target') address for wallet.
    func getAddress (for wallet: W) -> Address
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
    func createWallet (manager: WalletManager,
                       currency: Currency) -> Wallet
}

///
/// A WallettManager manages one or more wallets one of which is designated the `primaryWallet`.
/// (For example, an EthereumWalletManager will manage an ETH wallet and one wallet for each
/// ERC20Token; the ETH wallet will be the primaryWallet.  A BitcoinWalletManager manages one
/// and only one wallet holding BTC.).
///
/// At least conceptually, a WalletManager is an 'Active Object' (whereas Transfer and Wallet are
/// 'Passive Objects'
///
open class WalletManager {
    internal let core: BRCryptoWalletManager

    /// The account
    public let account: Account

    /// The network
    public let network: Network

    /// The primaryWallet
    public let primaryWallet: Wallet

    /// The managed wallets - often will just be [primaryWallet]
    public let wallets: [Wallet]

    // The mode determines how the manager manages the account and wallets on network
    public let mode: WalletManagerMode

    // The file-system path to use for persistent storage.
    public let path: String

    public internal(set) var state: WalletManagerState

    /// The default WalletFactory for creating wallets.
    public var walletFactory: WalletFactory

    /// Connect to network and begin managing wallets for account
    func connect () {
        cryptoWalletManagerConnect (core)
    }

    /// Disconnect from the network.
    func disconnect () {
        cryptoWalletManagerDisconnect (core)
    }

     /// isConnected
    /// isSyncing

     func sync () {
        cryptoWalletManagerSync (core)
    }

    func sign (transfer: Transfer, paperKey: String) {

    }

    func submit (transfer: Transfer) {

    }

    internal init (core: BRCryptoWalletManager,
                   network: Network,
                   account: Account,
                   wallet: Wallet,
                   wallets: [Wallet],
                   mode: WalletManagerMode,
                   path: String,
                   state: WalletManagerState,
                   factory: WalletFactory) {
        self.core = core
        self.network = network
        self.account = account
        self.primaryWallet = wallet
        self.wallets = wallets
        self.mode = mode
        self.path = path
        self.state = state
        self.walletFactory = factory
        }
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

    func signAndSubmit (transfer: Transfer, paperKey: String) {
        sign (transfer: transfer, paperKey: paperKey)
        submit (transfer: transfer)
    }
}

///
/// The WalletManager state.
///
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
    case changed (oldState: WalletManagerState, newState: WalletManagerState)
    case deleted

    case walletAdded (wallet: Wallet)
    case walletChanged (wallet: Wallet)
    case walletDeleted (wallet: Wallet)

    case syncStarted
    case syncProgress (percentComplete: Double)
    case syncEnded (error: String?)
}

///
/// The WalletManager's mode determines how the account and associated wallets are managed.
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
/// A SystemListener recieves asynchronous events announcing state changes to Networks, to Managers,
/// to Wallets and to Transfers.  This is an application's sole mechanism to learn of asynchronous
/// state changes.
///
/// Note: This must be 'class bound' as System  hold an 'unowned' reference (for GC reasons).
///
public protocol SystemListener : class {

    ///
    /// Handle a NetworkEvent
    ///
    /// - Parameters:
    ///   - system: the system
    ///   - network: the network
    ///   - event: the event
    ///
    func handleNetworkEvent (system: System,
                             network: Network,
                             event: NetworkEvent)

    ///
    /// Handle a WalletManagerEvent.
    ///
    /// - Parameters:
    ///   - system: the system
    ///   - manager: the manager
    ///   - event: the event
    ///
    func handleManagerEvent (system: System,
                             manager: WalletManager,
                             event: WalletManagerEvent)

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


    // TODO: handlePeerEvent ()
    // TODO: handleBlockEvent ()
}


/// Singleton
public final class System {

    /// The listener.  Gets all events for {Network, WalletManger, Wallet, Transfer}
    public private(set) weak var listener: SystemListener?

    /// The path for persistent storage
    public let path: String

    /// The URL for the 'blockchain DB'
    public let url: URL

    public internal(set) var networks: [Network] = []

    internal func lookup (network core: BRCryptoNetwork) -> Network? {
        return networks.first { $0.core == core }
    }

    internal func lookup (manager core: BRCryptoWalletManager) -> WalletManager? {
        return nil
    }

    internal func lookup (wallet core: BRCryptoWallet, manager: WalletManager) -> Wallet? {
        return nil
    }

    internal func lookup (transfer core: BRCryptoTransfer, wallet: Wallet) -> Transfer? {
        return  nil
    }

    private var coreNetworkListener: BRCryptoNetworkListener! = nil
    private var coreWalletManagerListener: BRCryptoCWMListener! = nil

    private static var instance: System?

    init (listener: SystemListener,
          persistencePath: String,
          blockchainURL: URL) {
        precondition (nil == System.instance)

        self.listener = listener
        self.path = persistencePath
        self.url = blockchainURL
        System.instance = self

        self.coreNetworkListener = BRCryptoNetworkListener (
            context: Unmanaged<System>.passRetained(self).toOpaque(),
            announce: { (context: BRCryptoNetworkListenerContext?, core: BRCryptoNetwork?) in
                precondition (nil != context)
                precondition (nil != core)
                let system = Unmanaged<System>.fromOpaque(context!).takeRetainedValue()
                let network = Network (core: core!)

                system.networks.append (network)
                system.listener?.handleNetworkEvent(
                    system: system,
                    network: network,
                    event: NetworkEvent.created)
        })
        cryptoNetworkDeclareListener(self.coreNetworkListener)

        self.coreWalletManagerListener = BRCryptoCWMListener (
            context: Unmanaged<System>.passRetained(self).toOpaque(),

            walletManagerEventCallback: { (context: BRCryptoCWMListenerContext?, coreCWM: BRCryptoWalletManager?) in
                precondition (nil != context && nil != coreCWM)
                let system  = Unmanaged<System>.fromOpaque(context!).takeRetainedValue()
                if let manager = system.lookup(manager: coreCWM!) {
                    let event = WalletManagerEvent.created

                    system.listener?.handleManagerEvent(
                        system: system,
                        manager: manager,
                        event: event)
                }
        },
            walletEventCallback: { (context: BRCryptoCWMListenerContext?, coreCWM: BRCryptoWalletManager?, coreWallet: BRCryptoWallet?) in
                precondition (nil != context && nil != coreCWM && nil != coreWallet)
                let system = Unmanaged<System>.fromOpaque(context!).takeRetainedValue()
                if let manager = system.lookup (manager: coreCWM!),
                    let wallet = system.lookup(wallet: coreWallet!, manager: manager) {

                    let event = WalletEvent.created

                    system.listener?.handleWalletEvent(
                        system: system,
                        manager: manager,
                        wallet: wallet,
                        event: event)
                }
        },

            transferEventCallback: { (context: BRCryptoCWMListenerContext?, coreCWM: BRCryptoWalletManager?, coreWallet: BRCryptoWallet?, coreTransfer: BRCryptoTransfer?) in
                precondition (nil != context && nil != coreCWM && nil != coreWallet && nil != coreTransfer)
                let system = Unmanaged<System>.fromOpaque(context!).takeRetainedValue()
                if let manager = system.lookup (manager: coreCWM!),
                    let wallet = system.lookup (wallet: coreWallet!, manager: manager),
                    let transfer = system.lookup (transfer: coreTransfer!, wallet: wallet) {

                    let event = TransferEvent.created

                    system.listener?.handleTransferEvent(
                        system: system,
                        manager: manager,
                        wallet: wallet,
                        transfer: transfer,
                        event: event)
                }
        })
        cryptoWalletManagerDeclareListener (self.coreWalletManagerListener)

        // Start communicating with URL
        //   Load currencies
        //   Load blockchains
    }
}

