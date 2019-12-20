//
//  BRCryptoCurrency.swift
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
/// A currency is a medium for exchange.
///
public final class Currency: Hashable {

    internal let core: BRCryptoCurrency

    /// A 'Unique Identifier
    public var uids: String {
        return asUTF8String (cryptoCurrencyGetUids (core))
    }

    /// The code; e.g. BTC
    public var code: String {
        return asUTF8String (cryptoCurrencyGetCode (core))
    }

    /// The name; e.g. Bitcoin
    public var name: String {
        return asUTF8String (cryptoCurrencyGetName (core))
    }

    /// The type:
    public var type: String {
        return asUTF8String (cryptoCurrencyGetType (core))
    }

    /// The issuer, if present.  This is generally an ERC20 address.
    public var issuer: String? {
        return cryptoCurrencyGetIssuer (core).map { asUTF8String($0) }
    }

    internal init (core: BRCryptoCurrency, take: Bool) {
        self.core = take ? cryptoCurrencyTake (core) : core
    }

    internal convenience init (core: BRCryptoCurrency) {
        self.init (core: core, take: true)
    }

    internal convenience init (uids: String,
                               name: String,
                               code: String,
                               type: String,
                               issuer: String?) {
        self.init (core: cryptoCurrencyCreate(uids, name, code, type, issuer), take: false)
    }

    deinit {
        cryptoCurrencyGive (core)
    }

    public static func == (lhs: Currency, rhs: Currency) -> Bool {
        return lhs === rhs || CRYPTO_TRUE == cryptoCurrencyIsIdentical (lhs.core, rhs.core)
    }

    public func hash (into hasher: inout Hasher) {
        hasher.combine (uids)
    }
}
