//
//  BREncryptor.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/18/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import Foundation
import BRCore

public protocol CryptoEncrypter {
    func encrypt (data: Data) -> Data
    func decrypt (data: Data) -> Data
}

public enum CoreCryptoEncrypter: CryptoEncrypter {
    case aes_ecb
    case chacha20_poly1305 (key32:Data, nonce12:Data, ad:Data)

    public func encrypt (data source: Data) -> Data {
        let sourceCount = source.count
        return source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Data in
            let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

            var target: Data!
            switch self {
            case .aes_ecb:
                target = Data (count: 16)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    BRAESECBEncrypt(targetAddr, sourceAddr, sourceCount)
                }

            case let .chacha20_poly1305 (key32, nonce12, ad):
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

                            target = Data (count: 16)
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

            return target
        }
    }

    public func decrypt (data source: Data) -> Data {
        let sourceCount = source.count
        return source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Data in
            let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

            var target: Data!
            switch self {
            case .aes_ecb:
                target = Data (count: 16)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    BRAESECBDecrypt(targetAddr, sourceAddr, sourceCount)
                }

            case let .chacha20_poly1305 (key32, nonce12, ad):
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

                            target = Data (count: 16)
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

            return target
        }
    }
}
