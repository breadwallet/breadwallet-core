//
//  BRSupport.swift
//  BRCrypto
//
//  Created by Ed Gamble on 11/5/18.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import Foundation

// Support

public func asUTF8String (_ chars: UnsafeMutablePointer<CChar>, _ release : Bool = false ) -> String {
    let result = String (cString: chars, encoding: .utf8)!
    if (release) { free (chars) }
    return result
}

public func asUTF8String (_ chars: UnsafePointer<CChar>) -> String {
    return String (cString: chars, encoding: .utf8)!
}

struct Weak<T:AnyObject> {
    weak var value : T?
    init (value: T) {
        self.value = value
    }
}

extension UInt64 {
    func pow (_ y: UInt8) -> UInt64 {
        func recurse (_ x: UInt64, _ y: UInt8, _ r: UInt64) -> UInt64 {
            return y == 0 ? r : recurse (x, y - 1, x * UInt64(y))
        }
        return recurse (self, y, 1)
    }
}

public class CLangClosure<Args> {
    public let function: (Args) -> ()
    init (_ function: @escaping (Args) -> ()) {
        self.function = function
    }
}
