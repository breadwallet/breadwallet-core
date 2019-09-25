//
//  BRCryptoEncryptor.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/18/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import Foundation
import BRCryptoC

public protocol Encrypter {
    func encrypt (data: Data) -> Data
    func decrypt (data: Data) -> Data
}

public final class CoreEncrypter: Encrypter {
    public static func aes_ecb(key: Data) -> CoreEncrypter {
        return key.withUnsafeBytes { (keyBytes: UnsafeRawBufferPointer) -> CoreEncrypter in
            let keyAddr = keyBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            let core = cryptoCipherCreateForAESECB(keyAddr, keyBytes.count)!
            return CoreEncrypter (core: core)
        }
    }

    public static func chacha20_poly1305(key: Key, nonce12: Data, ad: Data) -> CoreEncrypter {
        return nonce12.withUnsafeBytes { (nonce12Bytes: UnsafeRawBufferPointer) -> CoreEncrypter in
            let nonce12Addr  = nonce12Bytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            return ad.withUnsafeBytes { (adBytes: UnsafeRawBufferPointer) -> CoreEncrypter in
                let adAddr  = adBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                let core = cryptoCipherCreateForChacha20Poly1305(key.core,
                                                                 nonce12Addr, nonce12Bytes.count,
                                                                 adAddr, adBytes.count)!
                return CoreEncrypter (core: core)
            }
        }
    }

    public static func pigeon(privKey: Key, pubKey: Key, nonce12: Data) -> CoreEncrypter {
        return nonce12.withUnsafeBytes { (nonce12Bytes: UnsafeRawBufferPointer) -> CoreEncrypter in
            let nonce12Addr  = nonce12Bytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            let core = cryptoCipherCreateForPigeon(privKey.core,
                                                   pubKey.core,
                                                   nonce12Addr, nonce12Bytes.count)!
            return CoreEncrypter (core: core)
        }
    }

    // The Core representation
    internal let core: BRCryptoCipher

    deinit { cryptoCipherGive (core) }

    internal init (core: BRCryptoCipher) {
        self.core = core
    }

    public func encrypt (data source: Data) -> Data {
        return source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Data in
            let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            let sourceCount = sourceBytes.count

            let targetCount = cryptoCipherEncryptLength(self.core, sourceAddr, sourceCount)
            precondition (targetCount != 0)

            var target = Data (count: targetCount)
            target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                let result = cryptoCipherEncrypt (self.core, targetAddr, targetCount, sourceAddr, sourceCount)
                precondition(result == CRYPTO_TRUE)
            }

            return target
        }
    }

    public func decrypt (data source: Data) -> Data {
        return source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Data in
            let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: Int8.self)
            let sourceCount = sourceBytes.count

            let targetCount = cryptoCipherDecryptLength(self.core, sourceAddr, sourceCount)
            precondition (targetCount != 0)

            var target = Data (count: targetCount)
            target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                let result = cryptoCipherDecrypt (self.core, targetAddr, targetCount, sourceAddr, sourceCount)
                precondition(result == CRYPTO_TRUE)
            }

            return target
        }
    }
}
