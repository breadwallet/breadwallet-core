//
//  BRSigner.swift
//  BRCrypto
//
//  Created by Ed Gamble on 7/18/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import Foundation
import BRCore

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
    func sign (data32: Data, using: Key) -> Data

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

public enum CoreSigner: Signer {
    // Does not support 'recovery'
    case basic

    /// Does support 'recovery'
    case compact

    public func sign (data32 digest: Data, using privateKey: Key) -> Data {
        // Copy the key - prep to pass to Core C functions
        var key = privateKey.core

        // Confirm `data32 digest` is 32 bytes
        let sourceCount = digest.count
        precondition (32 == sourceCount)

        return digest.withUnsafeBytes { (digestBytes: UnsafeRawBufferPointer) -> Data in
            let digestAddr  = digestBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            let digestUInt256 = digestAsUInt256 (digestAddr!)

            var target: Data!
            switch self {
            case .basic:
                var keySignRequiresANonNULLSignatue = Data (count: 72);
                // TODO: Above not needed?
                
                let targetCount = keySignRequiresANonNULLSignatue.withUnsafeMutableBytes { (bytes: UnsafeMutableRawBufferPointer) -> Int in
                    let addr  = bytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    return BRKeySign (&key, addr, 72, digestUInt256)
                }

                if 0 == targetCount { /* error */ }
                target = Data (count: targetCount)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    BRKeySign (&key, targetAddr, targetCount, digestUInt256)
                }

            case .compact:
                let targetCount = BRKeyCompactSign (&key, nil, 0, digestUInt256)
                if 0 == targetCount { /* error */ }
                target = Data (count: targetCount)
                target.withUnsafeMutableBytes { (targetBytes: UnsafeMutableRawBufferPointer) -> Void in
                    let targetAddr  = targetBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    BRKeyCompactSign (&key, targetAddr, targetCount, digestUInt256)
                }
            }

            return target
        }
    }

    public func recover (data32 digest: Data, signature: Data) -> Key? {
        let sourceCount = digest.count
        precondition (32 == sourceCount)

        return digest.withUnsafeBytes { (digestBytes: UnsafeRawBufferPointer) -> Key? in
            let digestAddr  = digestBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
            let digestUInt256 = digestAsUInt256 (digestAddr!) // : UInt256 = createUInt256(0)

            switch self {
            case .basic:
                return nil

            case .compact:
                let signatureCount = signature.count
                return signature.withUnsafeBytes { (signatureBytes: UnsafeRawBufferPointer) -> Key? in
                    var key = BRKey.self.init()
                    let signatureAddr  = signatureBytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    let success: Bool = 1 == BRKeyRecoverPubKey (&key, digestUInt256, signatureAddr, signatureCount)
                    return success ? Key (core: key, needPublicKey: false, compressedPublicKey: false) : nil
                }
            }
        }
    }

    private func digestAsUInt256 (_ dataAddr: UnsafePointer<UInt8>) -> UInt256 {
        var digest: UInt256 = UInt256.self.init()
        return UnsafeMutablePointer(mutating: &digest)
            .withMemoryRebound(to: UInt8.self, capacity: 32) { (digestBytes: UnsafeMutablePointer<UInt8>) -> UInt256 in
                for index in 0..<32 {
                    digestBytes[index] = dataAddr[index]
                }
                return digest
        }
    }
}
