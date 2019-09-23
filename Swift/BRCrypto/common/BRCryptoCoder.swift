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
    func encode (data: Data) -> String
    func decode (string: String) -> Data?
}

public enum CoreCoder: Coder {
    case hex
    case base58
    case base58check

    internal var coreType: BRCryptoCoderType {
        switch self {
        case .hex: return CRYPTO_CODER_HEX
        case .base58: return CRYPTO_CODER_BASE58
        case .base58check: return CRYPTO_CODER_BASE58CHECK
        }
    }

    public func encode (data source: Data) -> String {
        return source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> String in
            let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            let sourceCount = sourceBytes.count

            let coder = cryptoCoderCreate(coreType)!
            defer { cryptoCoderGive (coder) }

            var result: String!

            let targetCount = cryptoCoderEncodeLength(coder, sourceAddr, sourceCount)
            var target = Data (count: targetCount)
            target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: Int8.self)
                cryptoCoderEncode (coder, targetAddr, targetCount, sourceAddr, sourceCount)
                result = String (cString: targetAddr!)
            }

            return result
        }
    }

    public func decode (string source: String) -> Data? {
        return source.withCString { (sourceAddr: UnsafePointer<Int8>) -> Data? in
            let sourceCount: Int = source.count

            let coder = cryptoCoderCreate(coreType)!
            defer { cryptoCoderGive (coder) }

            let targetCount = cryptoCoderDecodeLength(coder, sourceAddr, sourceCount)
            guard targetCount != 0 else { return nil }

            var target = Data (count: targetCount)

            target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                cryptoCoderDecode (coder, targetAddr, targetCount, sourceAddr, sourceCount)
            }

            return target
        }
    }
}

