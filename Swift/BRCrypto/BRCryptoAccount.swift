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
import Foundation // Data
import BRCryptoC

///
///
///
public final class Account {
    let core: BRCryptoAccount

    // A 'globally unique' ID String for account.  For BlockchainDB this will be the 'walletId'
    let uids: String

    public var timestamp: UInt64 {
        get { return cryptoAccountGetTimestamp (core) }
//        set { cryptoAccountSetTimestamp (core, newValue) }
    }

    internal init (core: BRCryptoAccount, uids: String) {
        self.core = core
        self.uids = uids
    }

    public static func createFrom (phrase: String, timestamp: Date, uids: String) -> Account? {
        let timestampAsInt = UInt64 (timestamp.timeIntervalSince1970)
        return cryptoAccountCreate (phrase, timestampAsInt)
            .map { Account (core: $0, uids: uids) }
    }

    public static func createFrom (seed: Data, timestamp: Date, uids: String) -> Account? {
        let timestampAsInt = UInt64 (timestamp.timeIntervalSince1970)
        let bytes = [UInt8](seed)
        return cryptoAccountCreateFromSeedBytes (bytes, timestampAsInt)
            .map { Account (core: $0, uids: uids) }
    }

    public static func deriveSeed (phrase: String) -> Data {
        var seed = cryptoAccountDeriveSeed(phrase)
        return Data (bytes: &seed, count: MemoryLayout<UInt512>.size);
    }

    deinit {
        cryptoAccountGive (core)
    }

    // Test Only
    internal var addressAsETH: String {
        return asUTF8String (cryptoAccountAddressAsETH(core)!)
    }
}
