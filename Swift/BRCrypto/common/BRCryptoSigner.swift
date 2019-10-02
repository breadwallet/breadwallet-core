//
//  BRCryptoSigner.swift
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

///
/// Sign 32-byte data with a private key to return a signature; optional recover the public key
///
public protocol Signer {

    ///
    /// Create a signature from 32-byte data using the CryptoKey (with private key provided)
    ///
    /// - Parameters:
    ///   - data32: the data to be signed
    ///   - using: the prive key
    ///
    /// - Returns: the signature
    ///
    func sign (data32: Data, using: Key) -> Data?

    ///
    /// Recover the CryptoKey (only the public key portion) from the signed data and the signature.
    /// If the key cannot be revoved, because the signing algorithm doesn't support recover, then
    /// nil is returned.
    ///
    /// - Parameters:
    ///   - data32: the original data used create the signature
    ///   - signature: the signature
    ///
    /// - Returns: A CryptoKey with only the public key provided.
    ///
    func recover (data32: Data, signature: Data) -> Key?
}

public final class CoreSigner: Signer {

    /// Does not support 'recovery'
    public static var basicDER: CoreSigner {
        return CoreSigner (core: cryptoSignerCreate (CRYPTO_SIGNER_BASIC_DER)!)
    }

    /// Does not support 'recovery'
    public static var basicJOSE: CoreSigner {
        return CoreSigner (core: cryptoSignerCreate (CRYPTO_SIGNER_BASIC_JOSE)!)
    }

    /// Does support 'recovery'
    public static var compact: CoreSigner {
        return CoreSigner (core: cryptoSignerCreate (CRYPTO_SIGNER_COMPACT)!)
    }

    // The Core representation
    internal let core: BRCryptoSigner

    deinit { cryptoSignerGive (core) }

    internal init (core: BRCryptoSigner) {
        self.core = core
    }

    public func sign (data32 digest: Data, using privateKey: Key) -> Data? {
        // Copy the key - prep to pass to Core C functions
        let key = privateKey.core

        return digest.withUnsafeBytes { (digestBytes: UnsafeRawBufferPointer) -> Data? in
            let digestAddr  = digestBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            let digestCount = digestBytes.count

            // Confirm `data32 digest` is 32 bytes
            precondition (32 == digestCount)

            let targetCount = cryptoSignerSignLength(self.core, key, digestAddr, digestCount)
            guard targetCount != 0 else { return nil }

            var result = CRYPTO_FALSE
            var target = Data (count: targetCount)
            target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                result = cryptoSignerSign(self.core, key, targetAddr, targetCount, digestAddr, digestCount)
            }

            return result == CRYPTO_TRUE ? target : nil
        }
    }

    public func recover (data32 digest: Data, signature: Data) -> Key? {
        return digest.withUnsafeBytes { (digestBytes: UnsafeRawBufferPointer) -> Key? in
            let digestAddr  = digestBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            let digestCount = digestBytes.count

            // Confirm `data32 digest` is 32 bytes
            precondition (32 == digestCount)

            let signatureCount = signature.count
            return signature.withUnsafeBytes { (signatureBytes: UnsafeRawBufferPointer) -> Key? in
                let signatureAddr  = signatureBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)

                return cryptoSignerRecover (self.core, digestAddr, digestCount, signatureAddr, signatureCount)
                    .map { Key (core: $0) }
            }
        }
    }
}
