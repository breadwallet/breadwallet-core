//
//  BRHasher.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/18/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import Foundation // for Data
import BRCore

public protocol CryptoHasher {
    func hash (data: Data) -> Data
}


public enum CoreCryptoHasher: CryptoHasher {
    case sha1
    case sha256
    case sha224
//    case SHA512
//    case SHA3 // SHA3_256
//    case MD160
//    case MD5
//    case murmer3
//    case sip64
    case keccak256
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

            case .keccak256:
                target = Data (count: 32)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRKeccak256 (targetAddr, sourceAddr, sourceCount)
                }
            }

            return target
        }
    }
 }

