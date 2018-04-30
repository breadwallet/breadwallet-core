//
//  Support.swift
//  BRDcore
//
//  Created by Ed Gamble on 5/3/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import Foundation
import Core

extension String {
    var isValidAddress: Bool {
        guard lengthOfBytes(using: .utf8) > 0 else { return false }
        return BRAddressIsValid(self) != 0
    }

    var isValidBCHAddress: Bool {
        return bitcoinAddr.isValidAddress
    }

    var bCashAddr: String {
        var addr = [CChar](repeating: 0, count: 55)
        BRBCashAddrEncode(&addr, self)
        return String(cString: addr)
    }

    var bitcoinAddr: String {
        var addr = [CChar](repeating: 0, count: 36)
        BRBCashAddrDecode(&addr, self)
        return String(cString: addr)
    }

    var isValidEthAddress: Bool {
        let pattern = "^0[xX][0-9a-fA-F]{40}$"
        return range(of: pattern, options: .regularExpression) != nil
    }

    var sanitized: String {
        return applyingTransform(.toUnicodeName, reverse: false) ?? ""
    }

    func ltrim(_ chars: Set<Character>) -> String {
        if let index = self.index(where: {!chars.contains($0)}) {
            return String(self[index..<self.endIndex])
        } else {
            return ""
        }
    }

    func rtrim(_ chars: Set<Character>) -> String {
        if let index = self.reversed().index(where: {!chars.contains($0)}) {
            return String(self[self.startIndex...self.index(before: index.base)])
        } else {
            return ""
        }
    }

    func nsRange(from range: Range<Index>) -> NSRange {
        let location = utf16.distance(from: utf16.startIndex, to: range.lowerBound)
        let length = utf16.distance(from: range.lowerBound, to: range.upperBound)
        return NSRange(location: location, length: length)
    }
}

extension String {
    var hexToData: Data? {
        let scalars = unicodeScalars
        var bytes = Array<UInt8>(repeating: 0, count: (scalars.count + 1) >> 1)
        for (index, scalar) in scalars.enumerated() {
            guard var nibble = scalar.nibble else { return nil }
            if index & 1 == 0 {
                nibble <<= 4
            }
            bytes[index >> 1] |= nibble
        }
        return Data(bytes: bytes)
    }

    var withoutHexPrefix: String {
        guard self.hasPrefix("0x") else { return self }
        return String(self.dropFirst(2))
    }

    var withHexPrefix: String {
        guard !self.hasPrefix("0x") else { return self }
        return "0x\(self)"
    }

    var trimmedLeadingZeros: String {
        let trimmed = self.ltrim(["0"])
        return trimmed.count > 0 ? trimmed : "0"
    }

    func leftPadding(toLength: Int, withPad character: Character) -> String {
        if count < toLength {
            return String(repeatElement(character, count: toLength - count)) + self
        } else {
            return String(self[index(self.startIndex, offsetBy: count - toLength)...])
        }
    }

    /// Hex string padded to 32-bytes
    var paddedHexString: String {
        return self.withoutHexPrefix.leftPadding(toLength: 64, withPad: "0").withHexPrefix
    }

    var unpaddedHexString: String {
        return self.withoutHexPrefix.trimmedLeadingZeros.withHexPrefix
    }

}

extension UnicodeScalar {
    var nibble: UInt8? {
        if 48 <= value && value <= 57 {
            return UInt8(value - 48)
        }
        else if 65 <= value && value <= 70 {
            return UInt8(value - 55)
        }
        else if 97 <= value && value <= 102 {
            return UInt8(value - 87)
        }
        return nil
    }
}

// UInt256 support
extension String {
    init(_ value: UInt256, radix: Int = 10) {
        self = value.string(radix: radix)
    }
}
