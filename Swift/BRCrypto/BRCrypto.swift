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
import Foundation
import BRCryptoC

///
/// A currency is a medium for exchange.
///
public final class Currency: Hashable {

    internal let core: BRCryptoCurrency

    /// A 'Unique Identifier
    internal let uids: String

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

    internal init (core: BRCryptoCurrency,
                   uids: String) {
        self.core = core
        self.uids = uids
    }

    internal convenience init (uids: String,
                               name: String,
                               code: String,
                               type: String) {
        self.init (core: cryptoCurrencyCreate(name, code, type),
                   uids: uids);
    }

    deinit {
        cryptoCurrencyGive (core)
    }

    public static func == (lhs: Currency, rhs: Currency) -> Bool {
        return lhs === rhs || lhs.uids == rhs.uids
    }

    public func hash (into hasher: inout Hasher) {
        hasher.combine (uids)
    }

    /// Used to map Currency -> Built-In-Blockchain-Network
    public static let codeAsBTC = "btc"
    public static let codeAsBCH = "bch"
    public static let codeAsETH = "eth"
}

///
/// A unit of measure for a currency.  There can be multiple units for a given currency (analogous
/// to 'System International' units of (meters, kilometers, miles, ...) for a dimension of
/// 'length').  For example, Ethereum has units of: WEI, GWEI, ETHER, METHER, ... and Bitcoin of:
/// BTC, SATOSHI, ...
///
/// Each Currency has a 'baseUnit' - which is defined as the 'integer-ish' unit - such as SATOSHI
/// ane WEI for Bitcoin and Ethereum, respectively.  There can be multiple 'derivedUnits' - which
/// are derived by scaling off of a baseUnit.  For example, BTC and ETHER respectively.
///
public final class Unit: Hashable {
    internal let core: BRCryptoUnit

    internal let uids: String

    public let currency: Currency

    public var name: String {
        return asUTF8String (cryptoUnitGetName (core))
    }

    public var symbol: String {
        return asUTF8String (cryptoUnitGetSymbol (core))
    }

    public private(set) unowned var base: Unit! = nil // unowned?

    public var decimals: UInt8 {
        return cryptoUnitGetBaseDecimalOffset (core)
    }

    public func isCompatible (with that: Unit) -> Bool {
        return CRYPTO_TRUE == cryptoUnitIsCompatible (self.core, that.core)
    }

    public func hasCurrency (_ currency: Currency) -> Bool {
        return currency.core == cryptoUnitGetCurrency (core)
    }

    internal init (core: BRCryptoUnit,
                   currency: Currency,
                   uids: String,
                   base: Unit?) {
        self.core = core
        self.currency = currency
        self.uids = uids
        self.base = base ?? self
    }

    internal convenience init (currency: Currency,
                               uids: String,
                               name: String,
                               symbol: String) {
        self.init (core: cryptoUnitCreateAsBase (currency.core, name, symbol),
                   currency: currency,
                   uids: uids,
                   base: nil)
    }

    internal convenience init (currency: Currency,
                               uids: String,
                               name: String,
                               symbol: String,
                               base: Unit,
                               decimals: UInt8) {
        self.init (core: cryptoUnitCreate (currency.core, name, symbol, base.core, decimals),
                   currency: currency,
                   uids: uids,
                   base: base)
    }

    deinit {
        cryptoUnitGive (core)
    }

    public static func == (lhs: Unit, rhs: Unit) -> Bool {
        return lhs === rhs || lhs.uids == rhs.uids
    }

    public func hash (into hasher: inout Hasher) {
        hasher.combine (uids)
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
    /// CustomeStringConvertable.  This property is only used in the `description` function
    /// and to ascertain the Amount's currency typically for consistency in add/sub functions.
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

    public func string (as unit: Unit, formatter: NumberFormatter? = nil) -> String? {
        return double (as: unit)
            .flatMap { (formatter ?? self.formatterWith (unit: unit))
                .string (from: NSNumber(value: $0)) }
    }

    public func string (pair: CurrencyPair, formatter: NumberFormatter? = nil) -> String? {
        return pair.exchange (asBase: self)?
            .string (as: pair.quoteUnit, formatter: formatter)
    }

    ///
    /// Return a 'raw string' (as an integer in self's base unit) using `base` and `preface`.
    /// Caution is warranted: this is a string w/o any context (currency in a particular Unit).
    /// Don't use this tp subvert the other `string(as Unit: Unit, ...)` function.  Should only be
    /// used for 'partner' interfaces when their API requires a string value in the base unit.
    ///
    /// - Note: The amount's sign is utterly ignored.
    /// - Note: Unless there is a 'preface', the result may have leading zeros.
    /// - Note: For base 16, lowercased hex digits are returned.
    ///
    /// - Parameters:
    ///   - base: the numeric base - one of {2, 10, 16}.  Defaults to 16
    ///   - preface: a strig preface, defaults to '0x'
    ///
    /// - Returns: the amount in the base unit as a 'raw string'
    ///
    public func string (base: UInt8 = 16, preface: String = "0x") -> String {
        let value = cryptoAmountGetValue (self.core)
        let chars = coerceStringPrefaced (value, Int32(base), preface)
        defer { free (chars) }

        return asUTF8String (chars!)
    }

    public func isCompatible (with that: Amount) -> Bool {
        return CRYPTO_TRUE == cryptoAmountIsCompatible (self.core, that.core)
    }

    public func hasCurrency (_ currency: Currency) -> Bool {
        return currency.core == cryptoAmountGetCurrency (core)
    }

    public func add (_ that: Amount) -> Amount? {
        precondition (isCompatible (with: that))
        return cryptoAmountAdd (self.core, that.core)
            .map { Amount (core: $0, unit: self.unit) }
    }

    public func sub (_ that: Amount) -> Amount? {
        precondition (isCompatible (with: that))
        return cryptoAmountSub (self.core, that.core)
            .map { Amount (core: $0, unit: self.unit) }
    }

    public var negate: Amount {
        return Amount (core: cryptoAmountNegate (core), unit: unit)
    }
    
    internal init (core: BRCryptoAmount,
                   unit: Unit) {
        self.core = core
        self.unit = unit
    }

    public static func create (double: Double, unit: Unit) -> Amount {
        return Amount (core: cryptoAmountCreateDouble (double, unit.core),
                       unit: unit)
    }

    public static func create (integer: Int64, unit: Unit) -> Amount {
        return Amount (core: cryptoAmountCreateInteger (integer, unit.core),
                       unit: unit)
    }

    ///
    /// Parse `string` into an `Amount`.  The string has some limitations:
    ///  * it cannot start with '-' or '+' (no sign character)
    ///  * if it starts with '0x', it is interpreted as a 'hex string'
    ///  * if it has a decimal point, it is interpreted as a 'decimal string'
    ///  * otherwise, it is interpreted as an 'integer string'
    ///
    /// If it is a 'decimal string' and the string includes values after the decimal point, then
    /// the number of values must be less than or equal to the unit's decimals.  For example a
    /// string of "1.1" in BTC_SATOSHI won't parse as BTC_SATOSHI has 0 decimals (it is a base unit
    /// and thus must be an integer).  Whereas, a string of "1.1" in BTC_BTC will parse as BTC_BTC
    /// has 8 decimals and the string has but one.  ("0.123456789" won't parse as it has 9 digits
    /// after the decimal; both "1." and "1.0" will parse.)
    ///
    /// Additionally, `string` cannot have any extraneous starting or ending characters.  Said
    /// another way, `string` must be fully consumed.  Thus "10w" and "w10" and "1.1w" won't parse.
    ///
    /// - Parameters:
    ///   - string: the string to parse
    ///   - negative: true if negative; false otherwise
    ///   - unit: the string's unit
    ///
    /// - Returns: The `Amount` if the string can be parsed.
    ///
    public static func create (string: String, negative: Bool = false, unit: Unit) -> Amount? {
        let core = cryptoAmountCreateString (string, (negative ? CRYPTO_TRUE : CRYPTO_FALSE), unit.core)
        return nil == core ? nil : Amount (core: core!, unit: unit)
    }

    ///
    /// Produce a default NumberFormatter for `unit`.  Uses the User's current locale, a number
    /// style of `.currency`, a currency symbol of `unit.symbol`, and factional digits of
    /// `unit.decimals` (if non-zero).
    ///
    /// - Parameter unit: the unit
    ///
    /// - Returns: the formatter for unit
    ///
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

extension Amount {

    // ETH

    internal var asETH: UInt64 {
        var overflow: BRCryptoBoolean = CRYPTO_FALSE
        let value = cryptoAmountGetIntegerRaw (self.core, &overflow)
        precondition(CRYPTO_FALSE == overflow)
        return value
    }

    internal static func createAsETH (_ value: UInt256, _ unit: Unit) -> Amount {
        return Amount (core: cryptoAmountCreate(unit.currency.core, CRYPTO_FALSE, value),
                       unit: unit);
    }

    // BTC

    internal var asBTC: UInt64 {
        var overflow: BRCryptoBoolean = CRYPTO_FALSE
        let value = cryptoAmountGetIntegerRaw (self.core, &overflow)
        precondition(CRYPTO_FALSE == overflow)
        return value
    }

    internal static func createAsBTC (_ value: UInt64, _ unit: Unit) -> Amount {
        return Amount (core: cryptoAmountCreate(unit.currency.core, CRYPTO_FALSE, createUInt256(value)),
                       unit: unit);
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
            .flatMap { Amount.create (double: $0 * exchangeRate, unit: quoteUnit) }
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
            .flatMap { Amount.create (double: $0 / exchangeRate, unit: baseUnit) }
    }
}

extension CurrencyPair: CustomStringConvertible {
    public var description: String {
        return "\(baseUnit.name)/\(quoteUnit.name)=\(exchangeRate)"
    }
}

///
///
///
public final class Account {
    let core: BRCryptoAccount

    // A 'globally unique' ID String for account.  For BlockchainDB this will be the 'walletId'
    let uids: String

    public var timestamp: UInt64 {
        get { return cryptoAccountGetTimestamp (core) }
        set { cryptoAccountSetTimestamp (core, newValue) }
    }

    internal init (core: BRCryptoAccount, uids: String) {
        self.core = core
        self.uids = uids
    }

    public static func createFrom (phrase: String, uids: String) -> Account? {
        return cryptoAccountCreate (phrase)
            .map { Account (core: $0, uids: uids) }
    }

    public static func createFrom (seed: Data, uids: String) -> Account? {
        let bytes = [UInt8](seed)
        return cryptoAccountCreateFromSeedBytes (bytes)
            .map { Account (core: $0, uids: uids) }
    }

    public static func deriveSeed (phrase: String) -> Data {
        var seed = cryptoAccountDeriveSeed(phrase)
        return Data (bytes: &seed, count: MemoryLayout<UInt512>.size);
    }

    //
    // Implementation Private
    //

    internal var asETH: BREthereumAccount {
        return cryptoAccountAsETH (self.core)
    }

    internal var asBTC: BRMasterPubKey {
        return cryptoAccountAsBTC (self.core)
    }

    deinit {
        cryptoAccountGive (core)
    }

    // Test Only
    internal var addressAsETH: String {
        return asUTF8String (cryptoAccountAddressAsETH(core)!)
    }
}

///
/// A Blockchain Network.  Networks are created based from a cross-product of block chain and
/// network type.  Specifically {BTC, BCH, ETH, ...} x {Mainnet, Testnet, ...}.  Thus there will
/// be networks of [BTC-Mainnet, BTC-Testnet, ..., ETH-Mainnet, ETH-Testnet, ETH-Rinkeby, ...]
///
public final class Network: CustomStringConvertible {
    let core: BRCryptoNetwork

    /// A unique-identifer-string
    public let uids: String
    
    /// The name
    public let name: String

    /// If 'mainnet' then true, otherwise false
    public let isMainnet: Bool

    /// The native currency.  Multiple networks will have the same currency; for example,
    /// BTC-Mainnet and BTC-Testnet share the BTC currency.
    public let currency: Currency

    /// All currencies.  Multiple networks will have the same currencies.
    public var currencies: Set<Currency>

    func currencyBy (code: String) -> Currency? {
        return currencies.first { $0.code == code }
    }

    public func hasCurrency(_ that: Currency) -> Bool {
        return currencies.contains(that)
    }

    /// An association mapping Currency => { baseUnit, defaultUnit, Units }
    internal var associations: Dictionary<Currency, Association> = [:]

    public func baseUnitFor(currency: Currency) -> Unit? {
        return associations[currency]?.baseUnit
    }

    public func defaultUnitFor(currency: Currency) -> Unit? {
        return associations[currency]?.defaultUnit
    }

    public func unitsFor(currency: Currency) -> Set<Unit>? {
        return associations[currency]?.units
    }

    public func hasUnitFor(currency: Currency, unit: Unit) -> Bool? {
        return unitsFor(currency: currency)?.contains(unit)
    }

    /// The current height of the blockChain network.  On a reorganization, this might go backwards.
    /// (No guarantee that this monotonically increases)
    public internal(set) var height: UInt64

    public struct Association {
        let baseUnit: Unit
        let defaultUnit: Unit
        let units: Set<Unit>
    }

    internal init (uids: String,
                   name: String,
                   isMainnet: Bool,
                   currency: Currency,
                   height: UInt64,
                   associations: Dictionary<Currency, Association>,
                   core: BRCryptoNetwork) {
        self.uids = uids
        self.name = name
        self.isMainnet = isMainnet
        self.currency = currency
        self.associations = associations

        self.currencies = associations.keys.reduce (into: Set<Currency>()) {
            $0.insert($1)
        }

        self.height = height
        self.core = core
    }

    public convenience init (uids: String,
                             name: String,
                             isMainnet: Bool,
                             currency: Currency,
                             height: UInt64,
                             associations: Dictionary<Currency, Association>) {
        var core: BRCryptoNetwork!

        switch currency.code {
        case Currency.codeAsBTC:
            core = cryptoNetworkCreateAsBTC (name,
                                             (isMainnet ? 0x00 : 0x40),
                                             (isMainnet ? BRMainNetParams : BRTestNetParams))

        case Currency.codeAsBCH:
            core = cryptoNetworkCreateAsBTC (name,
                                             (isMainnet ? 0x00 : 0x40),
                                             (isMainnet ? BRBCashParams : BRBCashTestNetParams))

        case Currency.codeAsETH:
            if uids.contains("mainnet") {
                core = cryptoNetworkCreateAsETH (name, 1, ethereumMainnet)
            }
            else if uids.contains("testnet") || uids.contains("ropsten") {
                core = cryptoNetworkCreateAsETH (name, 3, ethereumTestnet)
            }
            else if uids.contains ("rinkeby") {
                core = cryptoNetworkCreateAsETH (name, 4, ethereumRinkeby)
            }
        default:
            core = cryptoNetworkCreateAsGEN (name)
            break
        }

        cryptoNetworkSetCurrency (core, currency.core)
        cryptoNetworkAddCurrency (core,
                                  currency.core,
                                  associations[currency]?.baseUnit.core,
                                  associations[currency]?.defaultUnit.core,
                                  nil)

        self.init (uids: uids,
                   name: name,
                   isMainnet: isMainnet,
                   currency: currency,
                   height: height,
                   associations: associations,
                   core: core)
    }

    public var description: String {
        return name
    }

    deinit {
        cryptoNetworkGive (core)
    }

    public var supportedModes: [WalletManagerMode] {
        switch cryptoNetworkGetType (core) {
        case BLOCK_CHAIN_TYPE_BTC:
            return [WalletManagerMode.p2p_only]
        case BLOCK_CHAIN_TYPE_ETH:
            return [WalletManagerMode.api_only,
                    WalletManagerMode.api_with_p2p_submit]
        case BLOCK_CHAIN_TYPE_GEN:
            return [WalletManagerMode.api_only]
        default: precondition (false)
        }
    }

    /// Should use the network's/manager's default address scheme
    public func addressFor (_ string: String) -> Address? {
        switch cryptoNetworkGetType (core) {
        case BLOCK_CHAIN_TYPE_BTC:
            return Address.createAsBTC(string)
        case BLOCK_CHAIN_TYPE_ETH:
            return Address.createAsETH (string)
        case BLOCK_CHAIN_TYPE_GEN:
            return nil
        default: precondition (false)
        }
    }

    // address schemes

}

public enum NetworkEvent {
    case created
}

///
/// Listener for NetworkEvent
///
public protocol NetworkListener: class {

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

}

///
/// An Address for transferring an amount.
///
/// - bitcoin: A bitcon-specific address
/// - ethereum: An ethereum-specific address
///
public final class Address: Equatable, CustomStringConvertible {
    let core: BRCryptoAddress

    internal init (core: BRCryptoAddress) {
        self.core = core
    }

    public private(set) lazy var description: String = {
        return asUTF8String (cryptoAddressAsString (core))
    }()

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
    public static func create (string: String, network: Network) -> Address? {
        return network.addressFor(string)
    }

    internal static func createAsBTC (_ string: String) -> Address? {
        guard 1 == BRAddressIsValid (string)
            else { return nil }
        return Address (core: cryptoAddressCreateAsBTC(BRAddressFill (string)))
    }

    internal static func createAsETH (_ string: String) -> Address? {
        guard ETHEREUM_BOOLEAN_TRUE == addressValidateString (string)
            else { return nil }
        return Address (core: cryptoAddressCreateAsETH (addressCreate(string)))
    }

    internal static func createAsETH (_ eth: BREthereumAddress) -> Address  {
        return Address (core: cryptoAddressCreateAsETH (eth))
    }

    internal static func createAsBTC (_ btc: BRAddress) -> Address  {
        return Address (core: cryptoAddressCreateAsBTC (btc))
    }

    deinit {
        cryptoAddressGive (core)
    }

//    class EthereumAddress: Address {
//        let core: BREthereumAddress
//
//        internal init (_ core: BREthereumAddress) {
//            self.core = core
//        }
//
//        static func create(string: String, network: Network) -> Address? {
//            return (ETHEREUM_BOOLEAN_FALSE == addressValidateString(string)
//                ? nil
//                : EthereumAddress (addressCreate (string)))
//        }
//
//        var description: String {
//            return asUTF8String(addressGetEncodedString (core, 1))
//        }
//    }

    public static func == (lhs: Address, rhs: Address) -> Bool {
        return CRYPTO_TRUE == cryptoAddressIsIdentical (lhs.core, rhs.core)
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
/// A Transfer represents the transfer of an `amount` of currency from `source` to `target`.  A
/// Transfer is held in a `Wallet` (holding the amount's currency); the Transfer requires a `fee`
/// to complete.  Once the transfer is signed/submitted it can be identified by a `TransferHash`.
/// Once the transfer has been included in the currency's blockchain it will have a
/// `TransferConfirmation`.
///
/// A Transfer is Equatable but not Hashable; Hashable would naturally be implmeneted in terms of
/// the TransferHash however that hash isn't available until after a transfer is signed.
///
public protocol Transfer : class {

    /// The owning wallet
    var wallet: Wallet { get }

    /// The source pays the fee and sends the amount.
    var source: Address? { get }

    /// The target receives the amount
    var target: Address? { get }

    /// The amount to transfer - always positive (from source to target)
    var amount: Amount { get }

    /// The amount to transfer after considering the direction.  If we received the transfer,
    /// the amount will be positive; if we sent the transfer, the amount will be negative; if
    /// the transfer is 'self directed', the amount will be zero.
    var amountDirected: Amount { get }

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

    /// The direction
    var direction: TransferDirection { get }

    // var originator: Bool { get }

    func identical (that: Transfer) -> Bool
}

extension Transfer {
    public var confirmation: TransferConfirmation? {
        if case .included (let confirmation) = state { return confirmation }
        else { return nil }
    }

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

    internal init (core: BRCryptoFeeBasis) {
        self.core = core
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
    public let fee: Amount
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

    internal init (core: BRCryptoTransferState, included: TransferConfirmation? = nil, failed: String? = nil) {
        switch core {
        case CRYPTO_TRANSFER_STATE_CREATED:   self = .created
        case CRYPTO_TRANSFER_STATE_SIGNED:    self = .signed
        case CRYPTO_TRANSFER_STATE_SUBMITTED: self = .submitted
        case CRYPTO_TRANSFER_STATE_INCLUDED:  self = .included(confirmation: included!)
        case CRYPTO_TRANSFER_STATE_ERRORRED:  self = .failed(reason: failed!)
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


///
/// A Wallet holds the transfers and a balance for a single currency.
///
public protocol Wallet: class {

    /// The owning manager
    var manager: WalletManager { get }

    /// The name
    var name: String { get }

    /// The base unit for the wallet's network.  This is used for `balance` and to derive the
    /// currency and name
    var unit: Unit { get }

    /// The current balance for currency
    var balance: Amount { get }

    /// The transfers of currency yielding `balance`
    var transfers: [Transfer] { get }

    /// Use a hash to lookup a transfer
    func transferBy (hash: TransferHash) -> Transfer?

    /// The current state.
    var state: WalletState { get }

    /// The default TransferFeeBasis for created transfers.
    var defaultFeeBasis: TransferFeeBasis { get set }

    /// The default TransferFactory for creating transfers.
//    var transferFactory: TransferFactory { get set }

    /// An address suitable for a transfer target (receiving).  Uses the default Address Scheme
    var target: Address { get }

    /// An address suitable for a transfer source (sending).  Uses the default AddressScheme
    var source: Address { get }

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
                         feeBasis: TransferFeeBasis) -> Transfer?

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
                      feeBasis: TransferFeeBasis?) -> Amount
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

    /// The owning system
    var system: System { get }

    /// The account
    var account: Account { get }

    /// The network
    var network: Network { get }

    /// The primaryWallet - holds the network's currency
    var primaryWallet: Wallet { get }

    /// The managed wallets - often will just be [primaryWallet]
    var wallets: [Wallet] { get }

    // The mode determines how the manager manages the account and wallets on network
    var mode: WalletManagerMode { get }

    // The file-system path to use for persistent storage.
    var path: String { get }  // persistent storage

    var state: WalletManagerState { get }

    /// The default WalletFactory for creating wallets.
    //    var walletFactory: WalletFactory { get set }

    /// Connect to network and begin managing wallets for account
    func connect ()

    /// Disconnect from the network.
    func disconnect ()

    /// isConnected
    /// sync(...)
    /// isSyncing

    func submit (transfer: Transfer, paperKey: String)

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
    //    func createWallet (currency: Currency) -> Wallet {
    //        return walletFactory.createWallet (manager: self,
    //                                           currency: currency)
    //    }

    /// The network's/primaryWallet's currency.  This is the currency used for transfer fees.
    var currency: Currency {
        return network.currency // don't reference `primaryWallet`; infinitely recurses
    }

    /// The name is simply the network currency's code - e.g. BTC, ETH
    public var name: String {
        return currency.code
    }

    /// The baseUnit for the network's currency.
    var baseUnit: Unit {
        return network.baseUnitFor(currency: network.currency)!
    }

    /// The defaultUnit for the network's currency.
    var defaultUnit: Unit {
        return network.defaultUnitFor(currency: network.currency)!
    }
    
    /// A manager `isActive` if connected or syncing
    var isActive: Bool {
        return state == .connected || state == .syncing
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

    internal init (core: BRCryptoWalletManagerState) {
        switch core {
        case CRYPTO_WALLET_MANAGER_STATE_CREATED:      self = .created
        case CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED: self = .disconnected
        case CRYPTO_WALLET_MANAGER_STATE_CONNECTED:    self = .connected
        case CRYPTO_WALLET_MANAGER_STATE_SYNCING:      self = .syncing
        case CRYPTO_WALLET_MANAGER_STATE_DELETED:      self = .deleted
        default: self = .created; precondition(false)
        }
    }
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

    case blockUpdated (height: UInt64)
}

///
/// Listener For WalletManagerEvent
///
public protocol WalletManagerListener: class {
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

}

public protocol WalletManagerFactor {

}

///
/// System (a singleton)
///
public protocol System: class {
    
    /// The listener.  Gets all events for {Network, WalletManger, Wallet, Transfer}
    var listener: SystemListener? { get }

    /// The account
    var account: Account { get }

    /// The path for persistent storage
    var path: String { get }

    /// The 'blockchain DB'
    var query: BlockChainDB { get }

    /// Networks
    var networks: [Network] { get }

    /// Wallet Managers
    var managers: [WalletManager] { get }

    // Wallets - derived as a 'flatMap' of the managers' wallets.
    var wallets: [Wallet] { get }

    ///
    /// Start the system.  This will query various BRD services, notably the BlockChainDB, to
    /// establish the available networks (aka blockchains) and their currencies.  If the
    /// `networksNeeded` array includes the name of an available network, then a `Network`
    /// will be created for that network.  (This generates a `SystemEvent` which can be used by
    /// the App to create a `WalletManager`)
    ///
    /// This method can be called repeatedly; however ONLY THE FIRST invocation will create
    /// Networks for needed networks.  Subsequent calls will simple restart `System` processing.
    ///
    /// - Parameter networksNeeded: Array of network names of interest.
    ///
    func start (networksNeeded: [String])

    ///
    /// Stop the system.  Will inhibit `System` processing.
    ///
    func stop ()

    ///
    /// Subscribe (or unsubscribe) to BlockChainDB notifications.  Notifications provide an
    /// asynchronous announcement of DB changes pertinent to the User and are used to avoid
    /// polling the DB for such changes.
    ///
    /// The Subscription includes an `endpoint` which is optional.  If provided, subscriptions
    /// are enabled; if not provided, subscriptions are disabled.  Disabling a sbuscription is
    /// required, even though polling in undesirable, because Notifications are user configured.
    ///
    /// - Parameter subscription: the subscription to enable or to disable notifications
    ///
    func subscribe (using subscription: BlockChainDB.Subscription)

    ///
    /// Announce a BlockChainDB transaction.  This should be called upon the "System User's"
    /// receipt of a BlockchainDB notification.
    ///
    /// - Parameters:
    ///   - transaction: the transaction id which can be used in `getTransfer` to query the
    ///         blockchainDB for details on the transaction
    ///   - data: The transaction JSON data (a dictionary) if available
    ///
    func announce (transaction id: String, data: [String:Any])


    /// Create a system.
    ///
    /// - Parameters:
    ///   - listener: the system listener
    ///   - account: the system account
    ///   - path: the file system path for persistent storage
    ///   - query: the blockchainDB for queries
    ///
    /// - Returns: A System
    ///
    static func create (listener: SystemListener,
                        account: Account,
                        path: String,
                        query: BlockChainDB) -> System

    ///
    /// Create a wallet manager for `network` using `mode.
    ///
    /// - Parameters:
    ///   - network: the wallet manager's network
    ///   - mode: the mode to use
    ///
    func createWalletManager (network: Network,
                              mode: WalletManagerMode)
}

extension System {
    public var wallets: [Wallet] {
        return managers.flatMap { $0.wallets }
    }
}

public enum SystemEvent {
    case created
    case networkAdded (network: Network)
    case managerAdded (manager: WalletManager)
}
///
/// A SystemListener recieves asynchronous events announcing state changes to Networks, to Managers,
/// to Wallets and to Transfers.  This is an application's sole mechanism to learn of asynchronous
/// state changes.
///
/// Note: This must be 'class bound' as System holds a 'weak' reference (for GC reasons).
///
public protocol SystemListener : /* class, */ WalletManagerListener, WalletListener, TransferListener, NetworkListener {
    ///
    /// Handle a System Event
    ///
    /// - Parameters:
    ///   - system: the system
    ///   - event: the event
    ///
    func handleSystemEvent (system: System,
                            event: SystemEvent)
}
