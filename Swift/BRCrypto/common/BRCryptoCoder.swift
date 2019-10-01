//
//  BRCryptoCoder.swift
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

public protocol Coder {
    func encode (data: Data) -> String?
    func decode (string: String) -> Data?
}

public final class CoreCoder: Coder {

    public static var hex: CoreCoder {
        return CoreCoder (core: cryptoCoderCreate (CRYPTO_CODER_HEX)!)
    }

    public static var base58: CoreCoder {
        return CoreCoder (core: cryptoCoderCreate (CRYPTO_CODER_BASE58)!)
    }

    public static var base58check: CoreCoder {
        return CoreCoder (core: cryptoCoderCreate (CRYPTO_CODER_BASE58CHECK)!)
    }

    // The Core representation
    internal let core: BRCryptoCoder

    deinit { cryptoCoderGive (core) }

    internal init (core: BRCryptoCoder) {
        self.core = core
    }

    public func encode (data source: Data) -> String? {
        return source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> String? in
            let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            let sourceCount = sourceBytes.count

            let targetCount = cryptoCoderEncodeLength(self.core, sourceAddr, sourceCount)
            guard targetCount != 0 else { return nil }

            var target = Data (count: targetCount)
            return target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> String? in
                let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: Int8.self)

                let result = cryptoCoderEncode (self.core, targetAddr, targetCount, sourceAddr, sourceCount)
                
                return result == CRYPTO_TRUE ? String (cString: targetAddr!) : nil
            }
        }
    }

    public func decode (string source: String) -> Data? {
        return source.withCString { (sourceAddr: UnsafePointer<Int8>) -> Data? in
            let targetCount = cryptoCoderDecodeLength(self.core, sourceAddr)
            guard targetCount != 0 else { return nil }

            var result = CRYPTO_FALSE
            var target = Data (count: targetCount)
            target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                result = cryptoCoderDecode (self.core, targetAddr, targetCount, sourceAddr)
            }

            return result == CRYPTO_TRUE ? target : nil
        }
    }
}

