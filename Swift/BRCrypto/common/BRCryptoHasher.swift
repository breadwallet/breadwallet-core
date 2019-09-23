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
    func hash (data: Data) -> Data
}


public enum CoreHasher: HasherX {
    case sha1
    case sha256
    case sha224
    case sha256_2
    case sha384
    case sha512
    case rmd160
    case hash160
    case sha3
    case keccak256
    case md5
    //    case murmur3 (seed: UInt32)
    //    case sip64 (key16: Data)
    // ...

    internal var coreType: BRCryptoHasherType {
        switch self {
        case .sha1: return CRYPTO_HASHER_SHA1
        case .sha256: return CRYPTO_HASHER_SHA256
        case .sha224: return CRYPTO_HASHER_SHA224
        case .sha256_2: return CRYPTO_HASHER_SHA256_2

        case .sha384: return CRYPTO_HASHER_SHA384
        case .sha512: return CRYPTO_HASHER_SHA512
        case .rmd160: return CRYPTO_HASHER_RMD160
        case .hash160: return CRYPTO_HASHER_HASH160

        case .sha3: return CRYPTO_HASHER_SHA3
        case .keccak256: return CRYPTO_HASHER_KECCAK256
        case .md5: return CRYPTO_HASHER_MD5
        }
    }

    public func hash (data source: Data) -> Data {
        return source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Data in
            let sourceAddr = sourceBytes.baseAddress
            let sourceCount = sourceBytes.count

            let hasher = cryptoHasherCreate(coreType)!
            defer { cryptoHasherGive (hasher) }

            var target = Data (count: cryptoHasherLength(hasher))
            target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                let targetAddr = targetBytes.baseAddress
                cryptoHasherHash (hasher, targetAddr, sourceAddr, sourceCount)
            }

            return target
        }
    }
 }

