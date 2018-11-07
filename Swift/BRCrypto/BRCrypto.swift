//
//  BRCrypto.swift
//  BRCoreX
//
//  Created by Ed Gamble on 11/5/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//
import Foundation
import Core  // UInt256, UInt512, MasterPubKey
import Core.Ethereum // BREthereum{Account,Address}

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

    public var description : String {
        switch self {
        case .bitcoin:  return "BTC"
        case .bitcash:  return "BCH"
        case .ethereum: return "ETH"
        case .token(_, _, _, let description, _): return description
        }
    }

    public var baseUnit: Unit {
        switch self {
        case .bitcoin:  return Unit.Bitcoin.SATOSHI
        case .bitcash:  return Unit.Bitcoin.SATOSHI
        case .ethereum: return Unit.Ethereum.WEI
        case .token (_, _, _, _, let unit): return unit!
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

    public static func == (lhs: Currency, rhs: Currency) -> Bool {
        switch (lhs, rhs) {
        case (.bitcoin, .bitcoin):   return true
        case (.bitcash, .bitcash):   return true
        case (.ethereum, .ethereum): return true
        case (.token(let s1, _, _, _, _), .token(let s2, _, _, _, _)): return s1 == s2
        default: return false
        }
    }

    /// Initialize a Currency.token() and create the 'integer' unit in the process
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
        precondition (nil == base.base)
        self.currency = base.currency
        self.base = base
        self.name = name
        self.symbol = symbol
        self.scale = scale
    }

    public struct Bitcoin {
        static let SATOSHI = Unit ("sat",     "X", Currency.bitcoin)
        // TODO: ...
        static let BITCOIN = Unit ("BTC", "B", 100000000, base: SATOSHI)
    }

    public struct Ethereum {
        public static let WEI   = Unit ("WEI",   "X", Currency.ethereum)
        // TODO: ...
        public static let GWEI  = Unit ("GWEI",  "X",          1000000000, base: WEI)
        public static let ETHER = Unit ("Ether", "X", 1000000000000000000, base: WEI)
    }

    public struct ERC20 {
        static let BRD = Currency (code: "BRD", symbol: "BRD", name: "Bread", description: "Bread",
                                   unit: (name: "BRDInteger", symbol: "BRDInteger"))
        public struct BRDUnits {
            static let BRDDecimal = BRD.tokenDecimalUnit(name: "BRD", symbol: "BRD", decimals: 18)!
        }
    }

    public struct Fiat {
        public static let US = Currency (code: "USD", symbol: "USD", name: "dollar", description: "US Currency",
                                         unit: (name: "cents", symbol: "c"))
        // TODO: Fill unit - better create default.
        public struct USD {
            public static let Cent   = US.defaultUnit
            public static let Dollar = US.tokenDecimalUnit(name: "USD", symbol: "$", decimals: 2)!
        }

        public static let JP = Currency (code: "JPY", symbol: "JPY", name: "yen", description: "JP currency",
                                         unit: (name: "Yen", symbol: "Y"))
        public struct JPY {
            public static let Yen = JP.defaultUnit
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
public struct Amount {

    // The value.  This is *always* the amount in the 'baseUnit' and thus an unsigned integer.
    // For example, if amount is 1BTC, then value is 1e8; if amount is 1ETH, then value is
    // 1e18.  This makes math operations trivial (no scaling is required).
    internal let value: UInt256

    // If negative
    internal let negative: Bool

    // The unit.  This is used for 'display' purposes only.  For example, if Amount is 10 GWEI
    // then value is 10 * 10^9 and `double` produces `10.0`.
    let unit: Unit

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
        precondition(lhs.isCompatible(rhs))

        var overflow: Int32 = 0
        let value = addUInt256_Overflow(lhs.value, rhs.value, &overflow)
        return 0 == overflow ? Amount (value: value, unit: lhs.unit, negative: false) : nil
    }

    public static func - (lhs: Amount, rhs: Amount) -> Amount? {
        precondition(lhs.isCompatible(rhs))

        var negative: Int32 = 0
        let value = subUInt256_Negative (lhs.value, rhs.value, &negative)
        return Amount (value: value, unit: lhs.unit, negative: 1 == negative)
    }
}

extension Amount: Comparable {
    public static func == (lhs: Amount, rhs: Amount) -> Bool {
        precondition(lhs.isCompatible(rhs))
        return 1 == eqUInt256 (lhs.value, rhs.value)
    }

    public static func < (lhs: Amount, rhs: Amount) -> Bool {
        precondition(lhs.isCompatible(rhs))
        return 1 == ltUInt256 (lhs.value, rhs.value)
    }

    public static func <= (lhs: Amount, rhs: Amount) -> Bool {
        precondition(lhs.isCompatible(rhs))
        return 1 == leUInt256 (lhs.value, rhs.value)
    }
}

extension Amount: CustomStringConvertible {
    public var description: String {
        return "\(self.double?.description ?? "<nan>") \(self.currency.symbol)"
    }
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
public struct CurrencyPair: CustomStringConvertible {

    /// In EUR/USD=1.2500, the `baseCurrecny` is EUR.
    let baseCurrency: Unit

    /// In EUR/USD=1.250, the `quoteCurrecny` is USD.
    let quoteCurrency: Unit

    /// In EUR/USD=1.2500, the `exchangeRate` is 1.2500 which represents the number of USD that
    /// one EUR can be exchanged for.
    let exchangeRate: Double

    ///
    /// Apply `self` CurrencyPair to convert `asBase` (in `baseCurrency`) to `quoteCurrency`.  This
    /// is essentially `asBase * exchangeRate`
    ///
    /// - Parameter amount: the amount of `baseCurrency`
    ///
    /// - Returns: the amount as `quoteCurrency`
    ///
    public func exchange(asBase amount: Amount) -> Amount? {
        guard let amountValue = amount.coerce(unit: baseCurrency).double else { return nil }
        return Amount (value: amountValue * exchangeRate, unit: quoteCurrency)
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
        guard let amountValue = amount.coerce(unit: quoteCurrency).double else { return nil }
        return Amount (value: amountValue / exchangeRate, unit: baseCurrency)
    }
}

extension CurrencyPair {
    public var description: String {
        return "\(baseCurrency.name)/\(quoteCurrency.name)=\(exchangeRate)"
    }
}


///
/// MARK: - Account
///

/// An `Accouint` represents the User's paperKey.  An App generally has one instace of an
/// account; that account generates bitcoin, ethereum, ... addresses
public struct Account {

    let masterPublicKey: BRMasterPubKey
    let ethereumAccount: BREthereumAccount

    // conceptually - for security reasaons, not stored.
    //    var paperKey : String { get }
    //    var keyPair : KeyPair { get }

    public init (phrase: String) {
        self.init (seed: Account.deriveSeed (phrase: phrase))
    }

    internal init (seed: UInt512) {
        var seed = seed
        self.masterPublicKey = BRBIP32MasterPubKey (&seed, MemoryLayout<UInt512>.size)
        self.ethereumAccount = createAccountWithBIP32Seed (seed)
    }

    private static func deriveSeed (phrase: String) -> UInt512 {
        var seed: UInt512 = zeroUInt512()
        BRBIP39DeriveKey (&seed.u8, phrase, nil); // no passphrase
        return seed
    }

    //    public static func generate (entropy: UInt128, words: [String]) -> String? {
    //        assert (words.count == 2048)
    //        var entropy = entropy
    //        var words = words.map { $0.utf8CString }
    //
    //        let phraseLen = BRBIP39Encode (nil, 0, &words, &entropy, MemoryLayout<UInt128>.size)
    //        var phrase = Data (count: phraseLen)
    //
    //        phrase.withUnsafeMutableBytes {
    //            BRBIP39Encode ($0, phraseLen, &words, &entropy, MemoryLayout<UInt128>.size)
    //        }
    //        return String (data: phrase, encoding: String.Encoding.utf8)
    //    }
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
    case bitcoin  (main: Bool, name: String)      // OpaquePointer: BRMainNetParams, BRTestNetParams
    case bitcash  (main: Bool, name: String)
    case ethereum (main: Bool, name: String, chainId: UInt)
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

    public struct Bitcoin {
        public static let mainnet = Network.bitcoin (main: true,  name: "BTC Mainnet")
        public static let testnet = Network.bitcoin (main: false, name: "BTC Testnet")
    }

    public struct Bitcash {
        public static let mainnet = Network.bitcash(main: true,  name: "BCH Mainnet")
        public static let testnet = Network.bitcash(main: false, name: "BCH Testnet")
    }

    public struct Ethereum {
        public static let mainnet = Network.ethereum (main: true,  name: "ETH Mainnet", chainId: 1)
        public static let ropsten = Network.ethereum (main: false, name: "ETH Ropsten", chainId: 3)
        public static let rinkeby = Network.ethereum (main: false, name: "ETH Rinkeby", chainId: 4)
        public static let foundation = mainnet
        // ...
    }
}

///
/// MARK: - Address
///


public enum Address: /* Hashable */ CustomStringConvertible {
    case bitcoin  (BRAddress)
    case ethereum (BREthereumAddress)


    // network
    // currency: bitcoin, bitcash, ethereum

    public var description: String {
        switch self {
        case var .bitcoin (addr):
            return "abc" //  asUTF8String(addr.s, false)
        case let .ethereum (addr):
            return asUTF8String (addressGetEncodedString (addr, 1), true)
        }
    }
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
    case bitcoin  (feePerKB: UInt64) // in satoshi
    case ethereum (gasPrice: Amount, gasLimit: UInt64)
}

/// A TransferConfirmation holds confirmation information.
public struct TransferConfirmation {
    var blockNumber: UInt64
    var transactionIndex: UInt64
    var timestamp: UInt64
    var fee: Amount
}

/// A TransferHash uniquely identifies a transfer *among* the owning wallet's transfers.
public struct TransferHash: Hashable, CustomStringConvertible {
    let string: String

    init (_ string: String) {
        self.string = string
    }
    
    public var description: String {
        return string
    }

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
/// MARK: - Wallet
///

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

    // func sign (transfer: Transfer)
    // submit
    // ... cancel, replace - if appropriate

    /// An address suitable for a transfer target (receiving).
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
}

/// The WalletManager state.
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
///
/// - added:
/// - updated:
/// - deleted:
public enum WalletManagerPersistenceChangeType {
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

    // init - bitcoin
}


/// The WalletManagerPersistenceClient defines the interface for persistent storage of internal
/// wallet manager state - such as peers, block, and transactions
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

/// The WalletManagerBackendClient defines the interface for backend support to the
/// wallet manager modes of API_ONLY, API_WITH_P2P_SUBMIT and P2P_WITH_API_SYNC.
public protocol WalletManagerBackendClient: class {
}
