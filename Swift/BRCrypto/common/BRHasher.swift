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
    case SHA1
    case SHA256
    case SHA224
    case Keccak256
    // ...

    public func hash (data source: Data) -> Data {
        return source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Data in
            let sourceAddr = sourceBytes.baseAddress
            let sourceCount = source.count

            var target: Data!
            switch self {

            case .SHA1:
                target = Data (capacity: 20)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRSHA1 (targetAddr, sourceAddr, sourceCount)
                }

            case .SHA256:
                target = Data (capacity: 32)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRSHA256 (targetAddr, sourceAddr, sourceCount)
                }

            case .SHA224:
                target = Data (capacity: 28)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRSHA224 (targetAddr, sourceAddr, sourceCount)
                }

            case .Keccak256:
                target = Data (capacity: 32)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr = targetBytes.baseAddress
                    BRKeccak256 (targetAddr, sourceAddr, sourceCount)
                }
            }

            return target
        }
    }
 }

