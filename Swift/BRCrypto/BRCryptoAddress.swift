//
//  BRCryptoAddress.swift
//  BRCrypto
//
//  Created by Ed Gamble on 6/7/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import BRCryptoC

///
/// An Address for transferring an amount.  Addresses are network specific.
///
public final class Address: Equatable, CustomStringConvertible {
    let core: BRCryptoAddress

    internal init (core: BRCryptoAddress, take: Bool) {
        self.core = take ? cryptoAddressTake(core) : core
    }

    internal convenience init (core: BRCryptoAddress) {
        self.init (core: core, take: true)
    }

    public private(set) lazy var description: String = {
        return asUTF8String (cryptoAddressAsString (core), true)
    }()

    ///
    /// Create an Addres from `string` and `network`.  The provided `string` must be valid for
    /// the provided `network` - that is, an ETH address (as a string) differs from a BTC address
    /// and a BTC mainnet address differs from a BTC testnet address.  If `string` is not
    /// appropriate for `network`, then `nil` is returned.
    ///
    /// This function is typically used to convert User input - of a 'target address' as a string -
    /// into an Address.
    ///
    /// - Parameters:
    ///   - string: A string representing a crypto address
    ///   - network: The network for which the string is value
    ///
    /// - Returns: An address or nil if `string` is invalide for `network`
    ///
    public static func create (string: String, network: Network) -> Address? {
        return cryptoAddressCreateFromString (network.core, string)
            .map { Address (core: $0, take: false) }
    }

    deinit {
        cryptoAddressGive (core)
    }

    public static func == (lhs: Address, rhs: Address) -> Bool {
        return CRYPTO_TRUE == cryptoAddressIsIdentical (lhs.core, rhs.core)
    }
}

///
/// An AddressScheme determines the from of wallet-generated address.  For example, a Bitcoin wallet
/// can have a 'Segwit/BECH32' address scheme or a 'Legacy' address scheme.  The address, which is
/// ultimately a sequence of bytes, gets formatted as a string based on the scheme.
///
/// The WalletManager holds an array of AddressSchemes as well as the preferred AddressScheme.
///
public enum AddressScheme: Equatable, CustomStringConvertible {
    case btcLegacy
    case btcSegwit
    case ethDefault
    case genDefault

    internal init (core: BRCryptoAddressScheme) {
        switch core {
        case CRYPTO_ADDRESS_SCHEME_BTC_LEGACY:  self = .btcLegacy
        case CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT:  self = .btcSegwit
        case CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT: self = .ethDefault
        case CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT: self = .genDefault
        default: self = .genDefault;  preconditionFailure()
        }
    }

    internal var core: BRCryptoAddressScheme {
        switch self {
        case .btcLegacy:  return CRYPTO_ADDRESS_SCHEME_BTC_LEGACY
        case .btcSegwit:  return CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT
        case .ethDefault: return CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT
        case .genDefault: return CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT
        }
    }

    public var description: String {
        switch self {
        case .btcLegacy: return "BTC Legacy"
        case .btcSegwit: return "BTC Segwit"
        case .ethDefault: return "ETH Default"
        case .genDefault: return "GEN Default"
        }
    }
}
