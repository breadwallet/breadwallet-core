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
/// An Address for transferring an amount.
///
/// - bitcoin: A bitcon-specific address
/// - ethereum: An ethereum-specific address
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

