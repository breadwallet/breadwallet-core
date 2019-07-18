//
//  BREncoder.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/18/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import Foundation
import BRCore

public protocol CryptoCoder {
    func encode (data: Data) -> String
    func decode (string: String) -> Data
}

public enum CoreCryptoCoder: CryptoCoder {
    case hex
    case base58
    case base58check

    public func encode (data source: Data) -> String {
        let sourceCount = source.count
        return source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> String in
            let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

            var target: Data!
            var result: String!
            switch self {
            case .hex:
                let targetCount = 2 * sourceCount + 1
                target = Data (count: targetCount)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: Int8.self)
                    encodeHex (targetAddr, targetCount, sourceAddr, sourceCount)
                    result = String (cString: targetAddr!)
                }

            case .base58:
                let targetCount = BRBase58Encode(nil, 0, sourceAddr, sourceCount)
                target = Data (count: targetCount)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: Int8.self)
                    BRBase58Encode (targetAddr, targetCount, sourceAddr, sourceCount)
                    result = String (cString: targetAddr!)
               }

            case .base58check:
                let targetCount = BRBase58CheckEncode (nil, 0, sourceAddr, sourceCount)
                target = Data (count: targetCount)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: Int8.self)
                    BRBase58CheckEncode (targetAddr, targetCount, sourceAddr, sourceCount)
                    result = String (cString: targetAddr!)
               }
            }

            return result
        }
    }

    public func decode (string source: String) -> Data {
        let sourceCount: Int = source.count
        return source.withCString { (sourceAddr: UnsafePointer<Int8>) -> Data in
            var target: Data!
            switch self {
            case .hex:
                let targetCount = sourceCount / 2
                target = Data (count: targetCount)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    decodeHex (targetAddr, targetCount, sourceAddr, sourceCount)
                }
                print ("HEX: Decode: \(target)")

            case .base58:
                let targetCount = BRBase58Decode(nil, 0, sourceAddr)
                target = Data (count: targetCount)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    BRBase58Decode (targetAddr, targetCount, sourceAddr)
                }

            case .base58check:
                let targetCount = BRBase58CheckDecode (nil, 0, sourceAddr)
                target = Data (count: targetCount)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    BRBase58CheckDecode (targetAddr, targetCount, sourceAddr)
                }
            }
            return target
        }
    }
}

