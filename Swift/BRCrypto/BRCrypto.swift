//
//  BRCrypto.swift
//  BRCore
//
//  Created by Ed Gamble on 11/5/18.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import Foundation
import SystemConfiguration  // SCSystemReachability
import BRCore                 // UInt256, UInt512, MasterPubKey, BRAddress, ...
import BRCore.Ethereum        // BREthereum{Account,Address}, ...

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
public class Currency {

    /// The code
    public let code: String

    /// The symbol
    public let symbol: String

    /// The name
    public let name: String

    /// The decimals in the defaultUnit
    public let decimals: UInt8

    /// The baseUnit
    public private(set) var baseUnit: Unit! = nil

    /// The defaultUnit
    public private(set) var defaultUnit: Unit! = nil

    internal init (code: String, symbol: String, name: String, decimals: UInt8,
                   baseUnit: (name: String, symbol: String)) {
        self.code = code
        self.symbol = symbol
        self.name = name
        self.decimals = decimals

        // Note that `Unit` holds a `Currency` and `Currency
        /// The baseUnit.
        ///
        /// @Note: akdfja;dl
        ///
        self.baseUnit = Unit (baseUnit.name, baseUnit.symbol, self) // self has 'nil' for baseUnit

        let scale = pow (10.0, Double(decimals))
        self.defaultUnit = Unit (code, symbol, UInt64(scale),
                                 base: self.baseUnit)
    }
}

extension Currency: Equatable {
    public static func == (lhs: Currency, rhs: Currency) -> Bool {
        return lhs === rhs ||
            (lhs.code  == rhs.code &&
                lhs.symbol == rhs.symbol &&
                lhs.name == rhs.name &&
                lhs.decimals == rhs.decimals)
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
public class Unit {
    public let currency: Currency
    public let name: String
    public let symbol: String

    let scale: UInt64
    let base: Unit!

    ///
    /// Two units are compatible if they share the same currency
    ///
    /// - Parameter that: the other unit to compare
    /// - Returns: true if compatible, false otherwise
    ///
    public func isCompatible (_ that: Unit) -> Bool {
        return (self === that || self.currency == that.currency)
    }

    ///
    /// Initialize as a 'baseUnit'
    ///
    /// - Parameters:
    ///   - name: The name, such as SATOSHI
    ///   - symbol: The symbol, such as 'sat'
    ///   - currency: The currency
    ///
    init (_ name: String, _ symbol: String, _ currency: Currency) {
        self.currency = currency
        self.base = nil
        self.name = name
        self.symbol = symbol
        self.scale = 1
    }


    /// Initilize as a 'derivedUnit'
    ///
    /// - Parameters:
    ///   - name: The name, such a BTC
    ///   - symbol: The symbol, such as "B"
    ///   - scale: The scale, such as 10_000_000
    ///   - base: The base Unit
    ///
    init (_ name: String, _ symbol: String,  _ scale: UInt64, base: Unit) {
        precondition (nil == base.base)
        self.currency = base.currency
        self.base = base
        self.name = name
        self.symbol = symbol
        self.scale = scale
    }
}

///
/// An amount of currency.  This can be negative (as in, 'currency owed' rather then 'currency
/// owned').  Supports basic arithmetic operations (addition, subtraction, comparison); will
/// assert on !isCompatible for mismatched currency.
///
public struct Amount {

    // The value.  This is *always* the amount in the 'baseUnit' and thus an unsigned integer.
    // For example, if amount is 1BTC, then value is 1e8; if amount is 1ETH, then value is
    // 1e18.  This makes math operations trivial (no scaling is required).
    internal let value: UInt256

    // If negative
    public let negative: Bool

    // The unit.  This is used for 'display' purposes only.  For example, if Amount is 10 GWEI
    // then value is 10 * 10^9 and `double` produces `10.0`.
    public let unit: Unit

    // The value in `unit` as a Double, if representable
    public var double: Double? {
        let scale = unit.scale
        assert (scale > 0)

        var overflow: Int32 = 0
        var negative: Int32 = 0
        var remainder: Double = 0

        let value = mulUInt256_Double (self.value, 1/Double(scale), &overflow, &negative, &remainder)
        assert (0 == overflow && 0 == negative)

        let result = Double(coerceUInt64 (value, &overflow)) + remainder
        return 1 == overflow ? nil : (self.negative ? -result : result)
    }

    func scale (by scale: Double) -> Amount? {
        var overflow: Int32 = 0
        var negative: Int32 = 0
        var remainder: Double = 0

        let value = mulUInt256_Double (self.value, scale, &overflow, &negative, &remainder)
        return 1 == overflow ? nil : Amount (value: value, unit: self.unit, negative: self.negative != (1 == negative))
    }

    public func coerce (unit: Unit) -> Amount {
        precondition(self.unit.isCompatible(unit))
        return Amount (value: self.value, unit: unit, negative: self.negative)
    }

    public var currency: Currency {
        return unit.currency
    }

    public func isCompatible (_ that: Amount) -> Bool {
        return unit.isCompatible(that.unit)
    }

    public func describe (decimals: Int, withSymbol: Bool) -> String {
        if let value = self.double {
            var result = value;
            for _ in 0..<decimals { result *= 10 }
            result = floor (result)
            for _ in 0..<decimals { result /= 10 }
            return "\(result.description)\(withSymbol ? " \(unit.symbol)" : "")"
        }
        else { return "<nan>" }
    }
    
    public init (value: UInt64, unit: Unit) {
        self.init (value: value, unit: unit, negative: false)
    }

    public init (value: Int64, unit: Unit) {
        self.init (value: UInt64 (abs (value)), unit: unit, negative: value < 0)
    }

    public init (value: Int, unit: Unit) {
        self.init (value: UInt64(abs(value)), unit: unit, negative: value < 0)
    }
    
    public init (value: Double, unit: Unit) {
        let scale = unit.scale

        var overflow: Int32 = 0
        var negative: Int32 = 0
        var remainder: Double = 0

        let value = mulUInt256_Double (createUInt256(scale), value, &overflow, &negative, &remainder)

        self.init (value: value, unit: unit, negative: 1 == negative)
    }

    public init? (exactly value: Double, unit: Unit) {
        let scale = unit.scale

        var overflow: Int32 = 0
        var negative: Int32 = 0
        var remainder: Double = 0

        let value = mulUInt256_Double (createUInt256(scale), value, &overflow, &negative, &remainder)

        if (0 != remainder) { return nil }

        self.init (value: value, unit: unit, negative: 1 == negative)
    }

    //
    // Internal Init
    //
    internal init (value: UInt256, unit: Unit, negative: Bool) {
        self.value = value
        self.negative = negative
        self.unit = unit
    }

    internal init (value: UInt64, unit: Unit, negative: Bool) {
        let scale = unit.scale
        var overflow: Int32 = 0
        let value = (1 == scale
            ? createUInt256 (value)
            : mulUInt256_Overflow (createUInt256(value), createUInt256(scale), &overflow))
        assert (0 == overflow)

        self.init (value: value, unit: unit, negative: negative)
    }
}

extension Amount {
    public static func + (lhs: Amount, rhs: Amount) -> Amount? {
        guard lhs.isCompatible(rhs) else { return nil }

        var overflow: Int32 = 0
        let value = addUInt256_Overflow(lhs.value, rhs.value, &overflow)
        return 0 == overflow ? Amount (value: value, unit: lhs.unit, negative: false) : nil
    }

    public static func - (lhs: Amount, rhs: Amount) -> Amount? {
        guard lhs.isCompatible(rhs) else { return nil }

        var negative: Int32 = 0
        let value = subUInt256_Negative (lhs.value, rhs.value, &negative)
        return Amount (value: value, unit: lhs.unit, negative: 1 == negative)
    }
}

///
/// Note that incompatible units may return 'false' for all comparisons.  This violates the
/// expectation that `lhs` and `rhs` satisfy one of: ==, >, and <.  Caution.
///
extension Amount: Comparable {
    public static func == (lhs: Amount, rhs: Amount) -> Bool {
        guard lhs.isCompatible(rhs) else { return false }
        return 1 == eqUInt256 (lhs.value, rhs.value)
    }

    public static func < (lhs: Amount, rhs: Amount) -> Bool {
        guard lhs.isCompatible(rhs) else { return false }
        return 1 == ltUInt256 (lhs.value, rhs.value)
    }

    public static func <= (lhs: Amount, rhs: Amount) -> Bool {
        guard lhs.isCompatible(rhs) else { return false }
        return 1 == leUInt256 (lhs.value, rhs.value)
    }
}

extension Amount: CustomStringConvertible {
    public var description: String {
        return "\(self.double?.description ?? "<nan>") \(self.unit.symbol)"
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
        guard let amountValue = amount.coerce(unit: baseUnit).double else { return nil }
        return Amount (value: amountValue * exchangeRate, unit: quoteUnit)
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
        guard let amountValue = amount.coerce(unit: quoteUnit).double else { return nil }
        return Amount (value: amountValue / exchangeRate, unit: baseUnit)
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
public struct Account {

    /// The Bitcoin masterPublicKey - publically-accessible.
    let masterPublicKey: BRMasterPubKey

    /// The Ethereum account - publically-accessible
    let ethereumAccount: BREthereumAccount

    ///
    /// Initialize an Account from a paperKey pharse
    ///
    /// - Parameter phrase: The paperKey
    ///
    public init (phrase: String) {
        self.init (seed: Account.deriveSeed (phrase: phrase))
    }

    ///
    /// Initialize an Account from a seed.  The seed is not stored, only publically-accessible
    /// values, derived from the seed, are stored.
    ///
    /// - Parameter seed: The UInt512 seed
    ///
    internal init (seed: UInt512) {
        var seed = seed
        self.masterPublicKey = BRBIP32MasterPubKey (&seed, MemoryLayout<UInt512>.size)
        self.ethereumAccount = createAccountWithBIP32Seed (seed)
    }

    ///
    /// Derive a 'seed' from a paperKey phrase.  Used when signing (Bitcoin) transactions.
    ///
    /// - Parameter phrase: The PaperKey
    /// - Returns: The UInt512 seed.
    ///
    internal static func deriveSeed (phrase: String) -> UInt512 {
        var seed: UInt512 = zeroUInt512()
        BRBIP39DeriveKey (&seed.u8, phrase, nil); // no passphrase
        return seed
    }


    // func serialize() -> Data
    // init (serialization: Data)
}

///
/// A Key (unused)
///
public protocol Key {}

///
/// A KeyPair (unused)
///
public protocol KeyPair {
    var privateKey : Key { get }
    var publicKey : Key { get }
}

///
/// A Blockchain Network
///
/// - bitcoin: A bitcoin-specific network (mainnet, testnet)
/// - bitcash: A bitcash-specific network (mainnet, testnet)
/// - ethereum: An ethereum-specific network (mainnet/foundation, ropsten, rinkeby, ...)
///
public enum Network {
    case bitcoin  (name: String, forkId: UInt8, chainParams: UnsafePointer<BRChainParams>)
    case bitcash  (name: String, forkId: UInt8, chainParams: UnsafePointer<BRChainParams>)
    case ethereum (name: String, chainId: UInt, core: BREthereumNetwork)

    public var name: String {
        switch self {
        case .bitcoin  (let name, _, _): return name
        case .bitcash  (let name, _, _): return name
        case .ethereum (let name, _, _): return name
        }
    }

    public var currency: Currency {
        switch self {
        case .bitcoin: return Bitcoin.currency
        case .bitcash: return Bitcash.currency
        case .ethereum: return Ethereum.currency
        }
    }
}

extension Network: Hashable {
    public func hash (into hasher: inout Hasher) {
        switch self {
        case .bitcoin  (let name, _, _): hasher.combine (name)
        case .bitcash  (let name, _, _): hasher.combine (name)
        case .ethereum (let name, _, _): hasher.combine (name)
        }
    }

    public static func == (lhs: Network, rhs: Network) -> Bool {
        switch (lhs, rhs) {
        case (.bitcoin  (let n1, _, _), .bitcoin  (let n2, _, _)): return n1 == n2
        case (.bitcash  (let n1, _, _), .bitcash  (let n2, _, _)): return n1 == n2
        case (.ethereum (let n1, _, _), .ethereum (let n2, _, _)): return n1 == n2
        default: return false
        }
    }
}

extension Network: CustomStringConvertible {
    public var description: String {
        return name
    }
}

///
/// An Address for transferring an amount.
///
/// - bitcoin: A bitcon-specific address
/// - ethereum: An ethereum-specific address
///
public enum Address {
    case raw (String)
    case bitcoin  (BRAddress)
    case ethereum (BREthereumAddress)

    public init (raw string: String) {
        self = .raw (string)
    }

//    public init (bitcoin string: String) {
//        self = .bitcoin(BRAddress (s: string))
//    }
//
    public init (ethereum string: String) {
        self = .ethereum(addressCreate(string))
    }
}

extension Address: Hashable {
    public func hash (into hasher: inout Hasher) {
        switch self {
        case let .raw (addr): hasher.combine (addr)
        case var .bitcoin  (addr): hasher.combine (BRAddressHash (&addr))
        case let .ethereum (addr): hasher.combine (addressHashValue(addr))
        }
    }

    public static func == (lhs: Address, rhs: Address) -> Bool {
        switch (lhs, rhs) {
        case (.raw (let addr1), .raw (let addr2)):
            return addr1 == addr2
        case (.bitcoin (var addr1), .bitcoin (var addr2)):
            return 1 == BRAddressEq (&addr1, &addr2)
        case (.ethereum (let addr1), .ethereum (let addr2)):
            return 1 == addressHashEqual (addr1, addr2)
        default:
            return false
        }
    }
}

extension Address: CustomStringConvertible {
    public var description: String {
        switch self {
        case let .raw (addr):
            return addr
        case let .bitcoin (addr):
            return asUTF8String (UnsafeRawPointer([addr.s]).assumingMemoryBound(to: CChar.self))
        case let .ethereum (addr):
            return asUTF8String (addressGetEncodedString (addr, 1), true)
        }
    }
}

///
/// A Transfer represents the transfer of an `amount` of currency from `source` to `target`.  A
/// Transfer is held in a `Wallet` (holding the amount's currency); the Transfer requires a `fee`
/// to complete.  Once the transfer is signed/submitted it can be identified by a `TransferHash`.
/// Once the transfer has been included in the currency's blockchain it will have a
/// `TransferConfirmation`.
///
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

    var isSent: Bool { get }
    // var originator: Bool { get }
}

extension Transfer {
    public var confirmation: TransferConfirmation? {
        if case .included (let confirmation) = state { return confirmation }
        else { return nil }
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
public enum TransferHash: Hashable, CustomStringConvertible {
    case bitcoin (UInt256)
    case ethereum (BREthereumHash)

    public func hash (into hasher: inout Hasher) {
        switch self {
        case .bitcoin  (let core): hasher.combine (core.u32.0)
        case .ethereum (var core): hasher.combine (hashSetValue(&core))
        }
    }

    public static func == (lhs: TransferHash, rhs: TransferHash) -> Bool {
        switch (lhs, rhs) {
        case (.bitcoin(let c1), .bitcoin(let c2)):
            return 1 == eqUInt256 (c1, c2)
        case (.ethereum(var c1), .ethereum(var c2)):
            return 1 == hashSetEqual(&c1, &c2)
        default:
            return false
        }
    }

    public var description: String {
        switch self {
        case .bitcoin (let core):
            return asUTF8String (coerceUInt256HashToString(core), true)
        case .ethereum(let core):
            return asUTF8String (hashAsString(core))
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
public protocol Wallet: class {

    /// The owning manager
    var manager: WalletManager { get }

    /// The name
    var name: String { get }

    /// The current balance for currency
    var balance: Amount { get }

    /// The transfers of currency yielding `balance`
    var transfers: [Transfer] { get }

    /// Use a hash to lookup a transfer
    func lookup (transfer: TransferHash) -> Transfer?

    /// The current state.
    var state: WalletState { get }

    /// The default TransferFeeBasis for created transfers.
    var defaultFeeBasis: TransferFeeBasis { get set }

    /// The default TransferFactory for creating transfers.
    var transferFactory: TransferFactory { get set }

    /// An address suitable for a transfer target (receiving).  Uses the default Address Scheme
    var target: Address { get }
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
        return balance.currency
    }

    /// The (default) name derived from the currency
    public var name: String {
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
#if false
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
#endif

///
/// A WallettManager manages one or more wallets one of which is designated the `primaryWallet`.
/// (For example, an EthereumWalletManager will manage an ETH wallet and one wallet for each
/// ERC20Token; the ETH wallet will be the primaryWallet.  A BitcoinWalletManager manages one
/// and only one wallet holding BTC.).
///
/// At least conceptually, a WalletManager is an 'Active Object' (whereas Transfer and Wallet are
/// 'Passive Objects'
///
public protocol WalletManager : class {

    /// The listener receives Wallet, Transfer and perhaps other asynchronous events.
    var listener: WalletManagerListener { get }

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

    #if false
    /// The default WalletFactory for creating wallets.
    var walletFactory: WalletFactory { get set }
    #endif

    /// Connect to network and begin managing wallets for account
    func connect ()

    /// Disconnect from the network.
    func disconnect ()

     /// isConnected
    /// sync(...)
    /// isSyncing

    func sign (transfer: Transfer, paperKey: String)

    func submit (transfer: Transfer)

    func sync ()
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
    #if false
    func createWallet (currency: Currency) -> Wallet {
        return walletFactory.createWallet (manager: self,
                                           currency: currency)
    }
    #endif
    
    /// The primaryWallet's currency.
    var currency: Currency {
        return primaryWallet.currency
    }

    /// A manager `isActive` if connected or syncing
    var isActive: Bool {
        return state == .connected || state == .syncing
    }

    func signAndSubmit (transfer: Transfer, paperKey: String) {
        sign(transfer: transfer, paperKey: paperKey)
        submit(transfer: transfer)
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
/// A WalletManagerListener recieves asynchronous events announcing state changes to Managers, to
/// Wallets and to Transfers.  This is an application's sole mechanism to learn of asynchronous
/// state changes.
///
/// Note: This must be 'class bound' as the WalletManger holds an 'unowned' reference
///
public protocol WalletManagerListener : class {
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


    // TODO: handlePeerEvent ()
    // TODO: handleBlockEvent ()
}

///
/// The Wallet Manager Persistence Chagne Type identifes the type of wallet manager change for
/// persistent entities.  Such entityes include: peers, blocks, transactions and (Ethereum) logs.
///
/// - added: A Persistent entity was added
/// - updated: A Persistent entitye has chnaged
/// - deleted: A persistent entity was deleted
///
public enum WalletManagerPersistenceChangeType {
    case added
    case updated
    case deleted
}

///
/// The WalletManagerPersistenceClient defines the interface for persistent storage of internal
/// wallet manager state - such as peers, block, and transactions
///
public protocol WalletManagerPersistenceClient: class {

    func savePeers (manager: WalletManager,
                    data: Dictionary<String, String>) -> Void

    func saveBlocks (manager: WalletManager,
                     data: Dictionary<String, String>) -> Void

    func changeTransaction (manager: WalletManager,
                            change: WalletManagerPersistenceChangeType,
                            hash: String,
                            data: String) -> Void
}

///
/// The WalletManagerBackendClient defines the interface for backend support to the
/// wallet manager modes of API_ONLY, API_WITH_P2P_SUBMIT and P2P_WITH_API_SYNC.
///
public protocol WalletManagerBackendClient: class {
    func networkIsReachable () -> Bool
}

extension WalletManagerBackendClient {
    public func networkIsReachable() -> Bool {
        var zeroAddress = sockaddr()
        zeroAddress.sa_len = UInt8(MemoryLayout<sockaddr>.size)
        zeroAddress.sa_family = sa_family_t(AF_INET)

        guard let reachability = SCNetworkReachabilityCreateWithAddress (nil, &zeroAddress)
            else { return false }

        var flags: SCNetworkReachabilityFlags = []
        return SCNetworkReachabilityGetFlags(reachability, &flags) &&
            flags.contains(.reachable) &&
            !flags.contains(.connectionRequired)
    }
}
