//
//  BRCryptoKey.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/22/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import Foundation // Data
import BRCryptoC

public final class Key {
    static public var wordList: [String]?

    ///
    /// Check if a private key `string` is a valid passphrase-protected private key. The string
    /// must be BIP38 format.
    ///
    static public func isProtected(asPrivate string: String) -> Bool {
        return CRYPTO_TRUE == cryptoKeyIsProtectedPrivate (string)
    }

    ///
    /// Create `Key` from a UTF8-encoded BIP-39 paper key
    ///
    /// - Parameters:
    ///   - paperKey: A UTF8-encoded 12 word paper key
    ///   - words: Official BIP-39 list of words, with 2048 entries, in the language for `paperKey`
    ///
    /// - Returns: A Key, if the phrase if valid
    ///
    static public func createFrom (paperKey: Data, words: [String]? = wordList) -> Key? {
        precondition(paperKey.lastIndex(of: 0) != nil) // must be null terminated!

        guard var words = words?.map ({ UnsafePointer<Int8> (strdup($0)) }) else { return nil }
        defer { words.forEach { free(UnsafeMutablePointer (mutating: $0)) } }

        return paperKey.withUnsafeBytes { (paperKeyBytes: UnsafeRawBufferPointer) -> Key? in
            let paperKeyAddr  = paperKeyBytes.baseAddress?.assumingMemoryBound(to: Int8.self)
            return cryptoKeyCreateFromPhraseWithWords (paperKeyAddr, &words)
                .map { Key (core: $0)}
        }
    }

    ///
    /// Create `Key` from `string` using the passphrase to decrypt it. The string must be BIP38
    /// format. Different crypto currencies have different implementations; this function will
    /// look for a valid string using BITCOIN mainnet and BITCOIN testnet.
    ///
    /// - Parameter string
    /// - Parameter passphrase
    ///
    /// - Returns: A Key if one exists
    ///
    static public func createFromString (asPrivate string: String, withPassphrase: String) -> Key? {
        return cryptoKeyCreateFromStringProtectedPrivate (string, withPassphrase)
            .map { Key (core: $0) }
    }

    ///
    /// Create `Key` from `string`.  The string must be wallet import format (WIF), mini private
    /// key format, or hex string for example: 5HxWvvfubhXpYYpS3tJkw6fq9jE9j18THftkZjHHfmFiWtmAbrj
    /// Different crypto currencies have different formats; this function will look for a valid
    /// string using BITCOIN mainnet and BITCOIN testnet.
    ///
    /// - Parameter string
    ///
    /// - Returns: A Key if one exists
    ///
    static public func createFromString (asPrivate string: String) -> Key? {
        return cryptoKeyCreateFromStringPrivate (string)
            .map { Key (core: $0) }
    }

    ///
    /// Create `Key`, as a public key, from `string`.  The string must be the hex-encoded
    /// DER-encoded public key that is produced by `encodeAsPublic`
    ///
    /// - Parameter string:
    ///
    /// - Returns: A Key, if one exists
    ///
    static public func createFromString (asPublic string: String) -> Key? {
        return cryptoKeyCreateFromStringPublic (string)
            .map { Key (core: $0) }
    }


    static public func createForPigeonFrom (key: Key, nonce: Data) -> Key {
        let nonceCount = nonce.count
        var nonce = nonce
        return nonce.withUnsafeMutableBytes { (nonceBytes: UnsafeMutableRawBufferPointer) -> Key in
            let nonceAddr  = nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

            return cryptoKeyCreateForPigeon (key.core, nonceAddr, nonceCount)
                .map { Key (core: $0) }!
        }
    }

    static public func createForBIP32ApiAuth (phrase: String, words: [String]? = wordList) -> Key? {
        guard var words = words?.map ({ UnsafePointer<Int8> (strdup($0)) }) else { return nil }
        defer { words.forEach { free(UnsafeMutablePointer (mutating: $0)) } }

        return cryptoKeyCreateForBIP32ApiAuth (phrase, &words)
            .map { Key (core: $0) }
    }

    static public func createForBIP32BitID (phrase: String, index: Int, uri:String, words: [String]? = wordList) -> Key? {
        guard var words = words?.map ({ UnsafePointer<Int8> (strdup($0)) }) else { return nil }
        defer { words.forEach { free(UnsafeMutablePointer (mutating: $0)) } }

        return cryptoKeyCreateForBIP32BitID (phrase, Int32(index), uri, &words)
            .map { Key (core: $0) }
    }

//    static public func createFromSerialization (asPrivate data: Data) -> Key? {
//        var data = data
//
//        let dataCount = data.count
//        return data.withUnsafeMutableBytes { (dataBytes: UnsafeMutableRawBufferPointer) -> Key? in
//            let dataAddr = dataBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
//
//            return cryptoKeyCreateFromSerializationPrivate (dataAddr, dataCount)
//                .map { Key (core: $0)}
//        }
//    }
//
//    static public func createFromSerialization (asPublic data: Data) -> Key? {
//        var data = data
//
//        let dataCount = data.count
//        return data.withUnsafeMutableBytes { (dataBytes: UnsafeMutableRawBufferPointer) -> Key? in
//            let dataAddr = dataBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
//
//            return cryptoKeyCreateFromSerializationPublic (dataAddr, dataCount)
//                .map { Key (core: $0)}
//        }
//    }


    // The Core representation
    internal let core: BRCryptoKey

    deinit { cryptoKeyGive (core) }

    public var hasSecret: Bool {
        return 1 == cryptoKeyHasSecret  (self.core)
    }

    /// Return the WIF-encoded private key
    public var encodeAsPrivate: String {
        return asUTF8String(cryptoKeyEncodePrivate(self.core), true)
    }

    /// Return the hex-encoded, DER-encoded public key
    public var encodeAsPublic: String {
        return asUTF8String (cryptoKeyEncodePublic (self.core), true)
    }

    public var secret: UInt256 {
        return cryptoKeyGetSecret (self.core)
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
    internal init (core: BRCryptoKey) {
        self.core = core
        cryptoKeyProvidePublicKey (core, 0, 0)
    }

    ///
    /// Initialize based on `secret` to produce a private+public key pair
    ///
    /// - Parameter secret: the secret
    ///
    internal convenience init (secret: UInt256) {
        self.init (core: cryptoKeyCreateFromSecret (secret))
    }

//    // Serialization - Public Key
//
//    public enum PublicEncoding {
//        case derCompressed
//        case derUncompressed
//    }
//    
//    public func serialize (asPublic: PublicEncoding) -> Data {
//        let dataCount = cryptoKeySerializePublic (core, /* compressed */ nil, 0)
//        var data = Data (count: dataCount)
//        data.withUnsafeMutableBytes { (dataBytes: UnsafeMutableRawBufferPointer) -> Void in
//            let dataAddr = dataBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
//            cryptoKeySerializePublic (core, /* compressed */ dataAddr, dataCount)
//        }
//
//        return data
//    }
//
// // Serialization - Private Key
//
//    public enum PrivateEncoding {
//        case wifCompressed
//        case wifUncompressed
//    }
//
//    public func serialize (asPrivate: PrivateEncoding) -> Data? {
//        let dataCount = cryptoKeySerializePrivate (core, /* compressed */ nil, 0)
//        var data = Data (count: dataCount)
//        data.withUnsafeMutableBytes { (dataBytes: UnsafeMutableRawBufferPointer) -> Void in
//            let dataAddr = dataBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
//            cryptoKeySerializePrivate (core, /* compressed */ dataAddr, dataCount)
//        }
//
//        return data
//    }
}
