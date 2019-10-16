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
            return y == 0 ? r : recurse (x, y - 1, x * r)
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

    ///
    /// Return a function to build `AsComparable` from a `toValue` function.  For example, given
    /// a `struct Person { let age:UInt; let name:String }` one can produce a function to produce
    /// AsComparable<UInt,Person> based on `age` with `AsComparable.builder { $0.age }`
    ///
    /// - Parameter toValue: A function as (Item) -> Value
    ///
    /// - Returns: A function as (Item) -> AsComparable<Value,Item> that uses `toValue` to derive
    ///     the Value from Item.
    ///
    public static func builder<Value:Comparable, Item> (toValue: @escaping (Item) -> Value) -> (Item) -> AsComparable<Value,Item> {
        return { AsComparable<Value,Item> (item: $0, toValue: toValue) }
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

extension Result {
    public func recover (_ transform: (Failure) -> Success) -> Result<Success, Failure> {
        switch self {
        case .success: return self
        case let .failure(f): return Result.success(transform (f))
        }
    }

    public func resolve (success: (Success) -> Void, failure: (Failure) -> Void) {
        switch self {
        case let .success (value): success(value)
        case let .failure (error): failure(error)
        }
    }

    public func getWithRecovery (_ transform: (Failure) -> Success) -> Success {
        switch self {
        case let .success(success): return success
        case let .failure(failure): return transform (failure)
        }
    }
}

extension Array {
    ///
    /// Merge `that` into `self`, as a 'union', using `toIdentifier` to determine uniqueness
    ///
    /// - Parameters:
    ///   - that: array to merge into `self`
    ///   - toIdentifier: function determining uniqueness, returns `Equatable`
    ///
    /// - Returns: new array as union of 'this into self'
    ///
    public func unionOf<ID: Equatable> (_ that: [Iterator.Element], using toIdentifier: (Iterator.Element) -> ID) -> [Iterator.Element] {
        return that.reduce (self) { (result, element) -> [Iterator.Element] in // that into self
            let elementId = toIdentifier (element)
            return result.contains { toIdentifier($0) == elementId }
                ? result
                : (result + [element])
        }
    }
}


extension Array {
    // https://www.hackingwithswift.com/example-code/language/how-to-split-an-array-into-chunks
    func chunked(into size: Int) -> [[Element]] {
        return stride(from: 0, to: count, by: size).map {
            Array(self[$0 ..< Swift.min($0 + size, count)])
        }
    }
}

extension Date {
    var asUnixTimestamp: UInt64 {
        let since1970 = self.timeIntervalSince1970
        return UInt64 (since1970 >= 0 ? since1970 : 0)
    }
}

public struct AccountSpecification {
    public let identifier: String
    public let network: String
    public let paperKey: String
    public let timestamp: Date

    init (dict: [String: String]) {
        self.identifier = dict["identifier"]! //as! String
        self.network    = dict["network"]!
        self.paperKey   = dict["paperKey"]!

        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "yyyy-MM-dd"
        dateFormatter.locale = Locale(identifier: "en_US_POSIX")

        self.timestamp = dateFormatter.date(from: dict["timestamp"]!)!
    }

    static public func loadFrom (configPath: String, defaultSpecification: AccountSpecification? = nil) -> [AccountSpecification] {
        if FileManager.default.fileExists(atPath: configPath) {
            let configFile = URL(fileURLWithPath: configPath)
            let configData = try! Data.init(contentsOf: configFile)
            let json = try! JSONSerialization.jsonObject(with: configData, options: []) as! [[String:String]]
            return json.map { AccountSpecification (dict: $0) }
        }
        else if let defaultSpecification = defaultSpecification {
            return [defaultSpecification]
        }
        else {
            return []
        }
    }
}

