//
//  BRCryptoKey.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/22/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import Foundation // Data
import BRCryptoC

public final class Key {
    static var wordList: [String]?

    static func createFrom (phrase: String, words: [String]? = wordList) -> Key? {
        guard var words = words?.map ({ UnsafePointer<Int8> (strdup($0)) }) else { return nil }
        defer { words.forEach { free(UnsafeMutablePointer (mutating: $0)) } }

        guard 1 == BRBIP39PhraseIsValid (&words, phrase) else { return nil }

        var seed = UInt512.self.init()
        var key  = BRKey.self.init()

        BRBIP39DeriveKey (&seed, phrase, nil)
        defer { seed = UInt512() }

        var smallSeed = UInt256 (u64: (seed.u64.0, seed.u64.1, seed.u64.2, seed.u64.3))
        defer { smallSeed = UInt256() }

        BRKeySetSecret (&key, &smallSeed, 0)
        defer { BRKeyClean (&key) }

        //        UInt512 seed;
        //        BRBIP39DeriveKey (seed.u8, phrase, NULL);
        //        return seed;
        //        int BRKeySetSecret(BRKey *key, const UInt256 *secret, int compressed);

        return Key (core: key)
    }

    static func createForPigeonFrom (key: Key, nonce: Data) -> Key {
        return nonce.withUnsafeBytes { (nonceBytes: UnsafeRawBufferPointer) -> Key in
            let nonceAddr  = nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

            var pairingKey = BRKey()
            defer { BRKeyClean (&pairingKey) }

            var privateKey = key.core

            BRKeyPigeonPairingKey (&privateKey, &pairingKey, nonceAddr, nonce.count)

            //        let nonce = Array(remoteIdentifier)
            //        var privKey = authKey
            //        var pairingKey = BRKey()
            //        BRKeyPigeonPairingKey(&privKey, &pairingKey, nonce, nonce.count)

            return Key (core: pairingKey)
        }
    }

    static func createForBIP32ApiAuth (phrase: String, words: [String]? = wordList) -> Key? {
        guard var words = words?.map ({ UnsafePointer<Int8> (strdup($0)) }) else { return nil }
        defer { words.forEach { free(UnsafeMutablePointer (mutating: $0)) } }

        guard 1 == BRBIP39PhraseIsValid (&words, phrase) else { return nil }

        var seed = UInt512.self.init()
        var key  = BRKey.self.init()

        BRBIP39DeriveKey (&seed, phrase, nil)
        defer { seed = UInt512() }

        BRBIP32APIAuthKey (&key, &seed, MemoryLayout<UInt512>.size)
        defer { BRKeyClean (&key) }

        //        BRBIP39DeriveKey(&seed, phrase, nil)
        //        BRBIP32APIAuthKey(&key, &seed, MemoryLayout<UInt512>.size)
        //        seed = UInt512() // clear seed
        //        let pkLen = BRKeyPrivKey(&key, nil, 0)
        //        var pkData = CFDataCreateMutable(secureAllocator, pkLen) as Data
        //        pkData.count = pkLen
        //        guard pkData.withUnsafeMutableBytes({ BRKeyPrivKey(&key, $0.baseAddress?.assumingMemoryBound(to: Int8.self), pkLen) }) == pkLen else { return nil }
        //        key.clean()

        return Key (core: key)
    }

    static func createForBIP32BitID (phrase: String, index: Int, uri:String, words: [String]? = wordList) -> Key? {
        guard var words = words?.map ({ UnsafePointer<Int8> (strdup($0)) }) else { return nil }
        defer { words.forEach { free(UnsafeMutablePointer (mutating: $0)) } }

        guard 1 == BRBIP39PhraseIsValid (&words, phrase) else { return nil }

        var seed = UInt512.self.init()
        var key  = BRKey.self.init()

        BRBIP39DeriveKey (&seed, phrase, nil)
        defer { seed = UInt512() }

        BRBIP32BitIDKey (&key, &seed, MemoryLayout<UInt512>.size, UInt32(index), uri)
        defer { BRKeyClean (&key) }

        return Key (core: key)
    }

    // The Core representation
    var core: BRKey

    deinit { BRKeyClean (&core) }

    var isCompressed: Bool {
        get { return 1 == core.compressed}
        set { core.compressed = (newValue ? 1 : 0) }
    }

    var hasSecret: Bool {
        // seed not all zero
        return false
    }

    var serializedPublicKey: Data {
        return Data (count:0)
    }

    var serializedPrivateKey: Data? {
        guard hasSecret else { return nil }
        return nil
    }

    internal init (core: BRKey) {
        self.core = core
        // Ensure that the public key is provided
        BRKeyPubKey (&self.core, nil, 0)
    }
}
