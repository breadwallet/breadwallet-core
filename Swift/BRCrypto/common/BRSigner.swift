//
//  BRSigner.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/18/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import Foundation
import BRCore

public final class CryptoKey {
    let core: BRKey

    init (core: BRKey) {
        self.core = core
    }
}

public protocol CryptoSigner {
    func sign (data32: Data, using: CryptoKey) -> Data
    func recover (data32: Data, signature: Data) -> CryptoKey?
}

public enum CoreCryptoSigner: CryptoSigner {
    case basic
    case compact

    public func sign (data32 digest: Data, using privateKey: CryptoKey) -> Data {
        let sourceCount = digest.count
        return digest.withUnsafeBytes { (digestBytes: UnsafeRawBufferPointer) -> Data in
            let digestAddr  = digestBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

            var target: Data!
            switch self {
            case .basic:
                break
            case .compact:
                break
            }

            return target
        }
    }

    public func recover (data32 digest: Data, signature: Data) -> CryptoKey? {
        let sourceCount = digest.count
        return digest.withUnsafeBytes { (digestBytes: UnsafeRawBufferPointer) -> CryptoKey? in
            let digestAddr  = digestBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

            var key: CryptoKey! = nil
            switch self {
            case .basic: break
            case .compact:
                // ...
                break
            }

            return key
        }
    }
}
