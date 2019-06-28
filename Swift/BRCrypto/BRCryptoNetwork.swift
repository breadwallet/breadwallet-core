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
/// A Blockchain Network.  Networks are created based from a cross-product of block chain and
/// network type.  Specifically {BTC, BCH, ETH, ...} x {Mainnet, Testnet, ...}.  Thus there will
/// be networks of [BTC-Mainnet, BTC-Testnet, ..., ETH-Mainnet, ETH-Testnet, ETH-Rinkeby, ...]
///
public final class Network: CustomStringConvertible {
    let core: BRCryptoNetwork

    /// A unique-identifer-string
    internal var uids: String {
        return asUTF8String (cryptoNetworkGetUids (core))
    }
    
    /// The name
    public var name: String {
        return asUTF8String (cryptoNetworkGetName (core))
    }

    /// If 'mainnet' then true, otherwise false
    public var isMainnet: Bool {
        return CRYPTO_TRUE == cryptoNetworkIsMainnet (core)
    }

    /// The current height of the blockChain network.  On a reorganization, this might go backwards.
    /// (No guarantee that this monotonically increases)
    public var height: UInt64 {
        get { return cryptoNetworkGetHeight (core) }
        set { cryptoNetworkSetHeight (core, newValue) }
    }

    /// The native currency.
    public var currency: Currency {
        return Currency (core: cryptoNetworkGetCurrency(core), take: false)
    }

    /// All currencies - at least those we are handling/interested-in.
    public var currencies: Set<Currency> {
        return Set ((0..<cryptoNetworkGetCurrencyCount(core))
            .map { cryptoNetworkGetCurrencyAt (core, $0) }
            .map { Currency (core: $0, take: false)}
        )
    }

    func currencyBy (code: String) -> Currency? {
        return currencies.first { $0.code == code } // sloppily
    }

    public func hasCurrency(_ currency: Currency) -> Bool {
        return CRYPTO_TRUE == cryptoNetworkHasCurrency (core, currency.core)
    }

    public func baseUnitFor (currency: Currency) -> Unit? {
        guard hasCurrency(currency) else { return nil }
        return cryptoNetworkGetUnitAsBase (core, currency.core)
            .map { Unit (core: $0, take: false) }
    }

    public func defaultUnitFor (currency: Currency) -> Unit? {
        guard hasCurrency (currency) else { return nil }
        return cryptoNetworkGetUnitAsDefault (core, currency.core)
            .map { Unit (core: $0, take: false) }
    }

    public func unitsFor (currency: Currency) -> Set<Unit>? {
        guard hasCurrency (currency) else { return nil }
        return Set ((0..<cryptoNetworkGetUnitCount (core, currency.core))
            .map { cryptoNetworkGetUnitAt (core, currency.core, $0) }
            .map { Unit (core: $0, take: false) }
        )
    }

    public func hasUnitFor (currency: Currency, unit: Unit) -> Bool? {
        return unitsFor (currency: currency)?.contains(unit)
    }

    public struct Association {
        let baseUnit: Unit
        let defaultUnit: Unit
        let units: Set<Unit>
    }

    internal init (core: BRCryptoNetwork,
                   associations: Dictionary<Currency, Association>) {
        self.core = core

        associations.forEach {
            let (currency, association) = $0
            cryptoNetworkAddCurrency (core,
                                      currency.core,
                                      association.baseUnit.core,
                                      association.defaultUnit.core)
            association.units.forEach {
                cryptoNetworkAddCurrencyUnit (core,
                                              currency.core,
                                              $0.core)
            }
        }
    }

    /// Create a Network
    ///
    /// - Parameters:
    ///   - uids: A unique identifier string amoung all networks.  This parameter must include a
    ///       substring of {"mainnet", "testnet", "ropsten", "rinkeby"} to indicate the type
    ///       of the network.  [This following the BlockChainDB convention]
    ///   - name: the name
    ///   - isMainnet: if mainnet, then true
    ///   - currency: the currency
    ///   - height: the height
    ///   - associations: An association between one or more currencies and the units
    ///      for those currency.  The network currency should be included.
    ///
    public convenience init (uids: String,
                             name: String,
                             isMainnet: Bool,
                             currency: Currency,
                             height: UInt64,
                             associations: Dictionary<Currency, Association>) {
        var core: BRCryptoNetwork!

        switch currency.code.lowercased() {
        case Currency.codeAsBTC:
            core = cryptoNetworkCreateAsBTC (uids, name,
                                             (isMainnet ? 0x00 : 0x40),
                                             (isMainnet ? BRMainNetParams : BRTestNetParams))

        case Currency.codeAsBCH:
            core = cryptoNetworkCreateAsBTC (uids, name,
                                             (isMainnet ? 0x00 : 0x40),
                                             (isMainnet ? BRBCashParams : BRBCashTestNetParams))

        case Currency.codeAsETH:
            if uids.contains("mainnet") {
                core = cryptoNetworkCreateAsETH (uids, name, 1, ethereumMainnet)
            }
            else if uids.contains("testnet") || uids.contains("ropsten") {
                core = cryptoNetworkCreateAsETH (uids, name, 3, ethereumTestnet)
            }
            else if uids.contains ("rinkeby") {
                core = cryptoNetworkCreateAsETH (uids, name, 4, ethereumRinkeby)
            }
            else {
                precondition (false)
            }

        default:
            core = cryptoNetworkCreateAsGEN (uids, name)
            break
        }

        cryptoNetworkSetHeight (core, height);
        cryptoNetworkSetCurrency (core, currency.core)

        self.init (core: core, associations: associations)
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
            guard 1 == BRAddressIsValid (string)
                else { return nil }
            return Address (core: cryptoAddressCreateAsBTC (BRAddressFill (string)), take: false)

        case BLOCK_CHAIN_TYPE_ETH:
            guard ETHEREUM_BOOLEAN_TRUE == addressValidateString (string)
                else { return nil }
            return Address (core: cryptoAddressCreateAsETH (addressCreate(string)), take: false)

        case BLOCK_CHAIN_TYPE_GEN:
            return nil
        default: precondition (false)
        }
    }

    // address schemes

}

extension Network: Hashable {
    public static func == (lhs: Network, rhs: Network) -> Bool {
        return lhs.uids == rhs.uids
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(uids)
    }
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

