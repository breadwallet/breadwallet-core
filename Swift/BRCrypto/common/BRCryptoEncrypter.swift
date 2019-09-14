//
//  BREncryptor.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/18/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import Foundation
import BRCryptoC

public protocol Encrypter {
    func encrypt (data: Data) -> Data
    func decrypt (data: Data) -> Data
}

public enum CoreEncrypter: Encrypter {
    case aes_ecb (key:Data) // count = 16, 24,or 32
    case chacha20_poly1305 (key:Key, nonce12:Data, ad:Data)
    case pigeon (privKey:Key, pubKey:Key, nonce12:Data)

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

        case let .chacha20_poly1305 (key, nonce12, ad):
            let sourceCount = source.count
             source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Void in
                let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                var secret = key.secret
                UnsafeMutablePointer (mutating: &secret)
                    .withMemoryRebound(to: UInt8.self, capacity: 32) { (secretBytes: UnsafeMutablePointer<UInt8>) -> Void in
                    nonce12.withUnsafeBytes { (nonceBytes: UnsafeRawBufferPointer) -> Void in
                        ad.withUnsafeBytes { (adBytes: UnsafeRawBufferPointer) -> Void in
                            let adCount = ad.count
                            let adAddr  = adBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                            let targetCount =
                                BRChacha20Poly1305AEADEncrypt (nil, 0,
                                                               secretBytes,
                                                               nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                                               sourceAddr, sourceCount,
                                                               adAddr, adCount)

                            target = Data (count: targetCount)
                            target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                                let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                                BRChacha20Poly1305AEADEncrypt (targetAddr, targetCount,
                                                               secretBytes,
                                                               nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                                               sourceAddr, sourceCount,
                                                               adAddr, adCount)
                            }
                        }
                    }
                }
                secret = UInt256() // Zero
            }

        case let .pigeon (privKey, pubKey, nonce12):
            let sourceCount = source.count
            source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Void in
                let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                nonce12.withUnsafeBytes { (nonceBytes: UnsafeRawBufferPointer) -> Void in
                    let targetCount =
                        BRKeyPigeonEncrypt (nil, nil, 0, nil,
                                            nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                            sourceAddr, sourceCount)

                    target = Data (count: targetCount)
                    target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                        let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                        BRKeyPigeonEncrypt (cryptoKeyGetCore(privKey.core),
                                            targetAddr, targetCount,
                                            cryptoKeyGetCore(pubKey.core),
                                            nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                            sourceAddr, sourceCount)
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

        case let .chacha20_poly1305 (key, nonce12, ad):
            let sourceCount = source.count
            source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Void in
                let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                var secret = key.secret
                UnsafeMutablePointer (mutating: &secret)
                    .withMemoryRebound(to: UInt8.self, capacity: 32) { (secretBytes: UnsafeMutablePointer<UInt8>) -> Void in
                    nonce12.withUnsafeBytes { (nonceBytes: UnsafeRawBufferPointer) -> Void in
                        ad.withUnsafeBytes { (adBytes: UnsafeRawBufferPointer) -> Void in
                            let adCount = ad.count
                            let adAddr  = adBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                            let targetCount =
                                BRChacha20Poly1305AEADDecrypt (nil, 0,
                                                               secretBytes,
                                                               nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                                               sourceAddr, sourceCount,
                                                               adAddr, adCount)

                            target = Data (count: targetCount)
                            target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                                let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                                BRChacha20Poly1305AEADDecrypt (targetAddr, targetCount,
                                                               secretBytes,
                                                               nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                                               sourceAddr, sourceCount,
                                                               adAddr, adCount)
                            }
                        }
                    }
                }
                secret = UInt256() // Zero
            }

        case let .pigeon (privKey, pubKey, nonce12):
            let sourceCount = source.count
            source.withUnsafeBytes { (sourceBytes: UnsafeRawBufferPointer) -> Void in
                let sourceAddr  = sourceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                nonce12.withUnsafeBytes { (nonceBytes: UnsafeRawBufferPointer) -> Void in
                    let targetCount =
                        BRKeyPigeonDecrypt (nil, nil, 0, nil,
                                            nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                            sourceAddr, sourceCount)

                    target = Data (count: targetCount)
                    target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                        let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                        BRKeyPigeonDecrypt (cryptoKeyGetCore(privKey.core),
                                            targetAddr, targetCount,
                                            cryptoKeyGetCore(pubKey.core),
                                            nonceBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                                            sourceAddr, sourceCount)
                    }
                }
            }
        }

        return target

    }
}
