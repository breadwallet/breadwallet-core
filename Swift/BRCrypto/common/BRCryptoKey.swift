//
//  BRCryptoKey.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/22/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import Foundation // Data
import BRCryptoC

public final class CryptoKey {
    static public var wordList: [String]?

    static public func createFrom (phrase: String, words: [String]? = wordList) -> CryptoKey? {
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

        return CryptoKey (core: key)
    }

    static public func createFromSerialization (asPrivate data: Data) -> CryptoKey? {
        let str = String (data: data, encoding: .utf8)!

        return str.withCString { (strPtr: UnsafePointer<Int8>) -> CryptoKey? in
            guard 1 == BRPrivKeyIsValid (strPtr) else { return nil }

            var key = BRKey()
            defer { BRKeyClean (&key) }

            BRKeySetPrivKey (&key, strPtr)

            return CryptoKey (core: key)
        }
    }

    static public func createFromSerialization (asPublic data: Data) -> CryptoKey? {
        var data = data

        let dataCount = data.count
        return data.withUnsafeMutableBytes { (dataBytes: UnsafeMutableRawBufferPointer) -> CryptoKey? in
            let dataAddr = dataBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

            var key = BRKey()
            defer { BRKeyClean (&key) }


            return (1 == BRKeySetPubKey (&key, dataAddr, dataCount)
                ? CryptoKey (core: key)
                : nil)
        }
    }

    static public func createForPigeonFrom (key: CryptoKey, nonce: Data) -> CryptoKey {
        return nonce.withUnsafeBytes { (nonceBytes: UnsafeRawBufferPointer) -> CryptoKey in
            let nonceAddr  = nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

            var pairingKey = BRKey()
            defer { BRKeyClean (&pairingKey) }

            var privateKey = key.core

            BRKeyPigeonPairingKey (&privateKey, &pairingKey, nonceAddr, nonce.count)

            //        let nonce = Array(remoteIdentifier)
            //        var privKey = authKey
            //        var pairingKey = BRKey()
            //        BRKeyPigeonPairingKey(&privKey, &pairingKey, nonce, nonce.count)

            return CryptoKey (core: pairingKey)
        }
    }

    static public func createForBIP32ApiAuth (phrase: String, words: [String]? = wordList) -> CryptoKey? {
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

        return CryptoKey (core: key)
    }

    static public func createForBIP32BitID (phrase: String, index: Int, uri:String, words: [String]? = wordList) -> CryptoKey? {
        guard var words = words?.map ({ UnsafePointer<Int8> (strdup($0)) }) else { return nil }
        defer { words.forEach { free(UnsafeMutablePointer (mutating: $0)) } }

        guard 1 == BRBIP39PhraseIsValid (&words, phrase) else { return nil }

        var seed = UInt512.self.init()
        var key  = BRKey.self.init()

        BRBIP39DeriveKey (&seed, phrase, nil)
        defer { seed = UInt512() }

        BRBIP32BitIDKey (&key, &seed, MemoryLayout<UInt512>.size, UInt32(index), uri)
        defer { BRKeyClean (&key) }

        return CryptoKey (core: key)
    }

    // The Core representation
    var core: BRKey

    deinit { BRKeyClean (&core) }

    public var hasSecret: Bool {
        return !CryptoKey.secretMatch (core.secret, UInt256()) // non-all-zeros => has secret
    }

    // Serialization - Public Key

    public enum PublicEncoding {
        case derCompressed
        case derUncompressed
    }

    public func serialize (asPublic: PublicEncoding) -> Data {
        switch asPublic {
        case .derCompressed:
            return serializePublicKeyDER (key: self.core, compressed: true)

        case .derUncompressed:
            return serializePublicKeyDER (key: self.core, compressed: false)
        }
    }

    // Serialization - Private Key

    public enum PrivateEncoding {
        case wifCompressed
        case wifUncompressed
    }

    public func serialize (asPrivate: PrivateEncoding) -> Data? {
        guard hasSecret else { return nil }
        switch asPrivate {
        case .wifCompressed:
            return serializePrivateKeyWIF (key: core, compressed: true)

        case .wifUncompressed:
            return serializePrivateKeyWIF (key: core, compressed: false)
        }
    }

    ///
    /// Initialize based on a Core BRKey - the provided BRKey might be private+public or just
    /// a public key (such as one that is recovered from the signature.
    ///
    /// - Parameter core: The Core representaion
    ///
    internal init (core: BRKey) {
        self.core = core

        // Ensure that the public key is provided
        self.core.compressed = 0 // uncompressed
        BRKeyPubKey (&self.core, nil, 0)
    }

    ///
    /// Check if `self` and `that` have an identical public key
    ///
    /// - Parameter that: the other CryptoKey
    ///
    /// - Returns: If identical `true`; otherwise `false`
    ///
    public func publicKeyMatch (_ that: CryptoKey) -> Bool {
        let selfPub = self.core.pubKey
        let thatPub = that.core.pubKey
        return withUnsafePointer (to: selfPub) { (selfPtr) -> Bool in
            return withUnsafePointer (to: thatPub) { (thatPtr) -> Bool in
                return 0 == memcmp (selfPtr, thatPtr, MemoryLayout.size (ofValue: selfPub))
            }
        }
    }

    internal func privateKeyMatch (_ that: CryptoKey) -> Bool {
        return CryptoKey.secretMatch (self.core.secret, that.core.secret)
    }

    ///
    /// Initialize based on `secret` to produce a private+public key pair
    ///
    /// - Parameter secret: the secret
    ///
    internal convenience init (secret: UInt256) {
        var core = BRKey.init()
        defer { BRKeyClean (&core) }

        var secret = secret
        defer { secret = UInt256() }

        BRKeySetSecret (&core, &secret, 1)
        self.init (core: core)
    }

    ///
    /// Serialize a DER-encoded public key
    ///
    /// - Parameters:
    ///   - key: the key
    ///   - compressed: encode is compressed
    ///
    /// - Returns: the serialization as `Data`
    ///
    private func serializePublicKeyDER (key: BRKey, compressed: Bool) -> Data {
        var key = key
        key.compressed = (compressed ? 1 : 0)
        let dataCount = BRKeyPubKey (&key, nil, 0)
        var data = Data (count: dataCount)

        data.withUnsafeMutableBytes { (dataBytes: UnsafeMutableRawBufferPointer) -> Void in
            let dataAddr = dataBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            BRKeyPubKey (&key, dataAddr, dataCount)
        }

        return data
    }

    ///
    /// Serialize a WIF-encoded private key
    ///
    /// - Parameters:
    ///   - key: the key
    ///   - compressed: encode is compressed
    ///
    /// - Returns: the serialization as `Data`
    ///
    private func serializePrivateKeyWIF (key: BRKey, compressed: Bool) -> Data {
        var key = key
        key.compressed = (compressed ? 1 : 0)
        defer { BRKeyClean (&key) }

        let dataCount = BRKeyPrivKey (&key, nil, 0)
        var data = Data (count: dataCount)
        
        return data.withUnsafeMutableBytes { (dataBytes: UnsafeMutableRawBufferPointer) -> Data in
            let dataAddr = dataBytes.baseAddress?.assumingMemoryBound(to: CChar.self)
            BRKeyPrivKey (&key, dataAddr, dataCount)
            // `dataAddr` has a trailing zero - which we don't think we want... So, convert to
            // a zero-terminated string and then back to the required Data
            return String (cString: dataAddr!).data(using: .utf8)!
        }
    }

    ///
    /// Check equality of `this` and `that` UInt256 values
    ///
    /// - Parameters:
    ///   - this:
    ///   - that:
    ///
    /// - Returns: If identical `true`; otherwise `false`
    ///
    static private func secretMatch (_ this: UInt256, _ that: UInt256) -> Bool {
        return withUnsafePointer (to: this) { (thisPtr) -> Bool in
            return withUnsafePointer (to: that) { (thatPtr) -> Bool in
                // defer - zero thisPtr, thatPtr
                return 0 == memcmp (thisPtr, thatPtr, MemoryLayout.size (ofValue: this))
            }
        }
    }
}
