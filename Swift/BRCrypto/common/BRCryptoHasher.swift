//
//  BRHasher.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/18/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import Foundation // for Data
import BRCore

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

    public func hash (data source: Data) -> Data {
        return source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Data in
            let sourceAddr = sourceBytes.baseAddress
            let sourceCount = source.count

            var target: Data!
            switch self {

            case .sha1:
                target = Data (count: 20)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRSHA1 (targetAddr, sourceAddr, sourceCount)
                }

            case .sha256:
                target = Data (count: 32)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRSHA256 (targetAddr, sourceAddr, sourceCount)
                }

            case .sha224:
                target = Data (count: 28)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRSHA224 (targetAddr, sourceAddr, sourceCount)
                }

            case .sha256_2:
                target = Data (count: 32)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRSHA256_2 (targetAddr, sourceAddr, sourceCount)
                }

            case .sha384:
                target = Data (count: 48)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRSHA384 (targetAddr, sourceAddr, sourceCount)
                }

            case .sha512:
                target = Data (count: 64)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRSHA512 (targetAddr, sourceAddr, sourceCount)
                }

            case .rmd160:
                target = Data (count: 20)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRRMD160 (targetAddr, sourceAddr, sourceCount)
                }

            case .hash160:
                target = Data (count: 20)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRHash160 (targetAddr, sourceAddr, sourceCount)
                }

            case .sha3:
                target = Data (count: 32)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRSHA3_256 (targetAddr, sourceAddr, sourceCount)
                }

          case .keccak256:
                target = Data (count: 32)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRKeccak256 (targetAddr, sourceAddr, sourceCount)
                }

            case .md5:
                target = Data (count: 16)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRMD5 (targetAddr, sourceAddr, sourceCount)
                }
            }

            return target
        }
    }
 }

