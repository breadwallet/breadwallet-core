//
//  BRSupport.swift
//  BRCrypto
//
//  Created by Ed Gamble on 11/5/18.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import Foundation

///
/// MARK: - Support
///
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

/** An AsEquatable is an Equatable, based on Value, for an arbitrary Item */
public struct AsEquatable<Value:Equatable, Item> : Equatable {
    
    /** The value for Equatable */
    public var value: Value
    
    /** The arbitrary item */
    public var item: Item
    
    public init (item: Item, value: Value) {
        self.value = value
        self.item  = item
    }
    
    public init (item: Item, toValue: (Item) -> Value) {
        self.init (item: item, value: toValue(item))
    }
    
    public static func == <Value:Equatable, Item> (lhs:AsEquatable<Value, Item>, rhs:AsEquatable<Value,Item>) -> Bool {
        return lhs.value == rhs.value
    }
}

/** An AsComparable is a Comparable, based on Value, for an arbitary Item */
public struct AsComparable<Value:Comparable, Item> : Comparable {
    
    /** The value for Comparable */
    public var value: Value
    
    /** The arbitrary item */
    public var item: Item
    
    public init (item: Item, value: Value) {
        self.value  = value
        self.item   = item
    }
    
    public init (item: Item, toValue: (Item) -> Value) {
        self.init (item: item, value: toValue(item))
    }
    
    public static func builder<Value:Comparable, Item> (toValue: @escaping (Item) -> Value) -> (Item) -> AsComparable<Value,Item> {
        return { AsComparable<Value, Item> (item: $0, toValue: toValue) }
    }
    
    // Comparable Operators
    
    public static func == <Value:Comparable, Item> (lhs:AsComparable<Value, Item>, rhs:AsComparable<Value,Item>) -> Bool {
        return lhs.value == rhs.value
    }
    
    public static func < <Value:Comparable, Item> (lhs:AsComparable<Value, Item>, rhs:AsComparable<Value,Item>) -> Bool {
        return lhs.value < rhs.value
    }
    
}

/** An AsComparableInvert is a Comparable that inverts the comparison */
public struct AsComparableInvert<Value:Comparable> : Comparable {
    
    /** The value for Comparable with invert */
    public var value : Value
    
    public init (value: Value) {
        self.value = value
    }
    
    public static func == <Value:Comparable> (lhs:AsComparableInvert<Value>, rhs:AsComparableInvert<Value>) -> Bool {
        return lhs.value == rhs.value
    }
    
    public static func < <Value:Comparable> (lhs:AsComparableInvert<Value>, rhs:AsComparableInvert<Value>) -> Bool {
        return !(lhs.value < rhs.value)
    }
}
/** An AsHashable is a Hashable, based on Value, for an arbitrary Item */
public struct AsHashable<Value:Hashable, Item> : Hashable {
    
    /** The value for Hashable */
    public var value: Value
    
    /** The arbitrary item */
    public var item: Item
    
    public init (item: Item, value: Value) {
        self.value = value
        self.item  = item
    }
    
    public init (item: Item, toValue: (Item) -> Value) {
        self.init (item: item, value: toValue(item))
    }

    public func hash (into hasher: inout Hasher) {
        hasher.combine (value)
    }

    public static func == <Value:Hashable, Item> (lhs:AsHashable<Value, Item>, rhs:AsHashable<Value,Item>) -> Bool {
        return lhs.value == rhs.value
    }
}
