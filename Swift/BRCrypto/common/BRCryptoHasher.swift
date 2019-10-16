//
//  BRCryptoHasher.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/18/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import Foundation // for Data
import BRCryptoC

public protocol HasherX {
    func hash (data: Data) -> Data?
}

public final class CoreHasher: HasherX {

    public static var sha1: CoreHasher {
        return CoreHasher (core: cryptoHasherCreate (CRYPTO_HASHER_SHA1)!)
    }

    public static var sha224: CoreHasher {
        return CoreHasher (core: cryptoHasherCreate (CRYPTO_HASHER_SHA224)!)
    }

    public static var sha256: CoreHasher {
        return CoreHasher (core: cryptoHasherCreate (CRYPTO_HASHER_SHA256)!)
    }

    public static var sha256_2: CoreHasher {
        return CoreHasher (core: cryptoHasherCreate (CRYPTO_HASHER_SHA256_2)!)
    }

    public static var sha384: CoreHasher {
        return CoreHasher (core: cryptoHasherCreate (CRYPTO_HASHER_SHA384)!)
    }

    public static var sha512: CoreHasher {
        return CoreHasher (core: cryptoHasherCreate (CRYPTO_HASHER_SHA512)!)
    }

    public static var sha3: CoreHasher {
        return CoreHasher (core: cryptoHasherCreate (CRYPTO_HASHER_SHA3)!)
    }

    public static var rmd160: CoreHasher {
        return CoreHasher (core: cryptoHasherCreate (CRYPTO_HASHER_RMD160)!)
    }

    public static var hash160: CoreHasher {
        return CoreHasher (core: cryptoHasherCreate (CRYPTO_HASHER_HASH160)!)
    }

    public static var keccak256: CoreHasher {
        return CoreHasher (core: cryptoHasherCreate (CRYPTO_HASHER_KECCAK256)!)
    }

    public static var md5: CoreHasher {
        return CoreHasher (core: cryptoHasherCreate (CRYPTO_HASHER_MD5)!)
    }

    // The Core representation
    internal let core: BRCryptoHasher

    deinit { cryptoHasherGive (core) }

    internal init (core: BRCryptoHasher) {
        self.core = core
    }

    public func hash (data source: Data) -> Data? {
        return source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Data? in
            let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            let sourceCount = sourceBytes.count

            let targetCount = cryptoHasherLength(self.core)
            guard targetCount != 0 else { return nil }

            var result = CRYPTO_FALSE
            var target = Data (count: targetCount)
            target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                result = cryptoHasherHash (self.core, targetAddr, targetCount, sourceAddr, sourceCount)
            }

            return result == CRYPTO_TRUE ? target : nil
        }
    }
 }

