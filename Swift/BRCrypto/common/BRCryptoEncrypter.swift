//
//  BREncryptor.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/18/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import Foundation
import BRCore

public protocol Encrypter {
    func encrypt (data: Data) -> Data
    func decrypt (data: Data) -> Data
}

public enum CoreEncrypter: Encrypter {
    case aes_ecb (key:Data) // count = 16, 24,or 32
    case chacha20_poly1305 (key32:Data, nonce12:Data, ad:Data)

    public func encrypt (data source: Data) -> Data {
        var target: Data!
        switch self {
        case let .aes_ecb (key):
            let keyCount = key.count
            key.withUnsafeBytes { (keyBytes: UnsafeRawBufferPointer) -> Void in
                let keyAddr  = keyBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                target = Data (source)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    BRAESECBEncrypt(targetAddr, keyAddr, keyCount)
                }
            }

        case let .chacha20_poly1305 (key32, nonce12, ad):
            let sourceCount = source.count
             source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Void in
                let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                key32.withUnsafeBytes { (keyBytes: UnsafeRawBufferPointer) -> Void in
                    nonce12.withUnsafeBytes { (nonceBytes: UnsafeRawBufferPointer) -> Void in
                        ad.withUnsafeBytes { (adBytes: UnsafeRawBufferPointer) -> Void in
                            let adCount = ad.count
                            let adAddr  = adBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                            let targetCount =
                                BRChacha20Poly1305AEADEncrypt (nil, 0,
                                                               keyBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                                               nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                                               sourceAddr, sourceCount,
                                                               adAddr, adCount)

                            target = Data (count: targetCount)
                            target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                                let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                                BRChacha20Poly1305AEADEncrypt (targetAddr, targetCount,
                                                               keyBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                                               nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                                               sourceAddr, sourceCount,
                                                               adAddr, adCount)
                            }
                        }
                    }
                }
            }
        }
        return target
    }

    public func decrypt (data source: Data) -> Data {
        var target: Data!
        switch self {
        case let .aes_ecb (key):
            let keyCount = key.count
            key.withUnsafeBytes { (keyBytes: UnsafeRawBufferPointer) -> Void in
                let keyAddr  = keyBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                target = Data (source)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    BRAESECBDecrypt(targetAddr, keyAddr, keyCount)
                }
            }

        case let .chacha20_poly1305 (key32, nonce12, ad):
            let sourceCount = source.count
            source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Void in
                let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                key32.withUnsafeBytes { (keyBytes: UnsafeRawBufferPointer) -> Void in
                    nonce12.withUnsafeBytes { (nonceBytes: UnsafeRawBufferPointer) -> Void in
                        ad.withUnsafeBytes { (adBytes: UnsafeRawBufferPointer) -> Void in
                            let adCount = ad.count
                            let adAddr  = adBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                            let targetCount =
                                BRChacha20Poly1305AEADDecrypt (nil, 0,
                                                               keyBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                                               nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                                               sourceAddr, sourceCount,
                                                               adAddr, adCount)

                            target = Data (count: targetCount)
                            target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                                let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                                BRChacha20Poly1305AEADDecrypt (targetAddr, targetCount,
                                                               keyBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                                               nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                                               sourceAddr, sourceCount,
                                                               adAddr, adCount)
                            }
                        }
                    }
                }
            }
        }

        return target

    }
}
