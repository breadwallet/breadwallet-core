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
    static public var wordList: [String]?

    static public func createFrom (phrase: String, words: [String]? = wordList) -> Key? {
        guard var words = words?.map ({ UnsafePointer<Int8> (strdup($0)) }) else { return nil }
        defer { words.forEach { free(UnsafeMutablePointer (mutating: $0)) } }

        return cryptoKeyCreateFromPhraseWithWords (phrase, &words)
            .map { Key (core: $0, needPublicKey: true, compressedPublicKey: true)}
    }

    static public func createFromSerialization (asPrivate data: Data) -> Key? {
        var data = data

        let dataCount = data.count
        return data.withUnsafeMutableBytes { (dataBytes: UnsafeMutableRawBufferPointer) -> Key? in
            let dataAddr = dataBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

            return cryptoKeyCreateFromSerializationPrivate (dataAddr, dataCount)
                .map { Key (core: $0, needPublicKey: true, compressedPublicKey: false)}
        }
    }

    static public func createFromSerialization (asPublic data: Data) -> Key? {
        var data = data

        let dataCount = data.count
        return data.withUnsafeMutableBytes { (dataBytes: UnsafeMutableRawBufferPointer) -> Key? in
            let dataAddr = dataBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

            return cryptoKeyCreateFromSerializationPublic (dataAddr, dataCount)
                .map { Key (core: $0, needPublicKey: false, compressedPublicKey: false)}
        }
    }

    static public func createForPigeonFrom (key: Key, nonce: Data) -> Key {
        let nonceCount = nonce.count
        var nonce = nonce
        return nonce.withUnsafeMutableBytes { (nonceBytes: UnsafeMutableRawBufferPointer) -> Key in
            let nonceAddr  = nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

            return cryptoKeyCreateForPigeon (key.core, nonceAddr, nonceCount)
                .map { Key (core: $0, needPublicKey: true, compressedPublicKey: false) }!
        }
    }

    static public func createForBIP32ApiAuth (phrase: String, words: [String]? = wordList) -> Key? {
        guard var words = words?.map ({ UnsafePointer<Int8> (strdup($0)) }) else { return nil }
        defer { words.forEach { free(UnsafeMutablePointer (mutating: $0)) } }

        return cryptoKeyCreateForBIP32ApiAuth (phrase, &words)
            .map { Key (core: $0, needPublicKey: true, compressedPublicKey: false) }
    }

    static public func createForBIP32BitID (phrase: String, index: Int, uri:String, words: [String]? = wordList) -> Key? {
        guard var words = words?.map ({ UnsafePointer<Int8> (strdup($0)) }) else { return nil }
        defer { words.forEach { free(UnsafeMutablePointer (mutating: $0)) } }

        return cryptoKeyCreateForBIP32BitID (phrase, Int32(index), uri, &words)
            .map { Key (core: $0, needPublicKey: true, compressedPublicKey: false) }
    }

    // The Core representation
    internal let core: BRCryptoKey

    deinit { cryptoKeyGive (core) }

    public var hasSecret: Bool {
        return 1 == cryptoKeyHasSecret  (self.core)
    }

    ///
    /// Check if `self` and `that` have an identical public key
    ///
    /// - Parameter that: the other CryptoKey
    ///
    /// - Returns: If identical `true`; otherwise `false`
    ///
    public func publicKeyMatch (_ that: Key) -> Bool {
        return 1 == cryptoKeyPublicMatch (core, that.core)
    }

    internal func privateKeyMatch (_ that: Key) -> Bool {
        return 1 == cryptoKeySecretMatch (core, that.core)
    }



    ///
    /// Initialize based on a Core BRKey - the provided BRKey might be private+public or just
    /// a public key (such as one that is recovered from the signature.
    ///
    /// - Parameter core: The Core representaion
    ///
    internal init (core: BRCryptoKey, needPublicKey: Bool, compressedPublicKey: Bool) {
        self.core = core

        if needPublicKey { cryptoKeyProvidePublicKey (core, (compressedPublicKey ? 1 : 0)) }
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
        // Serialization - Public Key

        BRKeySetSecret (&core, &secret, 1)
        self.init (core: cryptoKeyCreateFromKey (&core), needPublicKey: true, compressedPublicKey: false)
    }

    // Serialization - Public Key

    public enum PublicEncoding {
        case derCompressed
        case derUncompressed
    }
    
    public func serialize (asPublic: PublicEncoding) -> Data {
        let dataCount = cryptoKeySerializePublic (core, /* compressed */ nil, 0)
        var data = Data (count: dataCount)
        data.withUnsafeMutableBytes { (dataBytes: UnsafeMutableRawBufferPointer) -> Void in
            let dataAddr = dataBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            cryptoKeySerializePublic (core, /* compressed */ dataAddr, dataCount)
        }

        return data
    }

 // Serialization - Private Key

    public enum PrivateEncoding {
        case wifCompressed
        case wifUncompressed
    }

    public func serialize (asPrivate: PrivateEncoding) -> Data? {
        let dataCount = cryptoKeySerializePrivate (core, /* compressed */ nil, 0)
        var data = Data (count: dataCount)
        data.withUnsafeMutableBytes { (dataBytes: UnsafeMutableRawBufferPointer) -> Void in
            let dataAddr = dataBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            cryptoKeySerializePrivate (core, /* compressed */ dataAddr, dataCount)
        }

        return data
    }
}
