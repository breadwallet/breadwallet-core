//
//  BRSupportTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 8/12/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import XCTest
@testable import BRCrypto

struct Person {
    let age: UInt
    let name: String
}

class BRSupportTests: XCTestCase {
    let p1 = Person (age: 10, name: "Kai")
    let p2 = Person (age:  8, name: "Mitsi")
    let p3 = Person (age: 10, name: "Austin")


    override func setUp() {
    }

    override func tearDown() {
    }

    func testUInt64 () {
        XCTAssertEqual (UInt64(1  ), UInt64(10).pow(0))
        XCTAssertEqual (UInt64(10 ), UInt64(10).pow(1))
        XCTAssertEqual (UInt64(100), UInt64(10).pow(2))
        XCTAssertEqual (UInt64(1000000), UInt64(10).pow(6))

        XCTAssertEqual (UInt64(1  ), UInt64(2).pow(0))
        XCTAssertEqual (UInt64(2  ), UInt64(2).pow(1))
        XCTAssertEqual (UInt64(8  ), UInt64(2).pow(3))
        XCTAssertEqual (UInt64(256), UInt64(2).pow(8))
    }


    func testAsEquatable () {
        XCTAssertEqual    (AsEquatable (item: p1, value: p1.age), AsEquatable (item: p3, value: p3.age))
        XCTAssertNotEqual (AsEquatable (item: p1, value: p1.age), AsEquatable (item: p2, value: p2.age))

        XCTAssertNotEqual (AsEquatable (item: p1, toValue: { $0.name }), AsEquatable (item: p3, toValue: { $0.name }))
    }

    func testAsComparable () {
        XCTAssertTrue (AsComparable (item: p1, value: p1.age) == AsComparable (item: p3, value: p3.age))
        XCTAssertTrue (AsComparable (item: p1, value: p1.age) >  AsComparable (item: p2, value: p2.age))

        XCTAssertTrue (AsComparable (item: p1, toValue: { $0.age }) == AsComparable (item: p3, toValue: { $0.age }))
        XCTAssertTrue (AsComparable (item: p1, toValue: { $0.age }) >  AsComparable (item: p2, toValue: { $0.age }))

        let builder = AsComparable<UInt,Person>.builder { (p:Person) -> UInt in p.age }
        XCTAssertTrue (builder (p1) == builder (p3))
    }

    func testAsComparableInvert () {
        XCTAssertFalse (AsComparableInvert(value: AsComparable (item: p1, value: p1.age)) >  AsComparableInvert (value: AsComparable (item: p2, value: p2.age)))
        XCTAssertTrue  (AsComparableInvert(value: AsComparable (item: p1, value: p1.age)) <= AsComparableInvert (value: AsComparable (item: p2, value: p2.age)))
        XCTAssertTrue  (AsComparableInvert(value: AsComparable (item: p1, value: p1.age)) == AsComparableInvert (value: AsComparable (item: p3, value: p3.age)))
    }

    func testAsHashable () {
        let personToAgeMap: [AsHashable<String,Person>:UInt] = [
            AsHashable (item: p1, toValue: { $0.name }): p1.age,
            AsHashable (item: p2, toValue: { $0.name }): p2.age,
            AsHashable (item: p3, toValue: { $0.name }): p3.age
            ]

        XCTAssertEqual(3, personToAgeMap.count)
    }

    enum TestResultError: Error {
        case TE1
        case TE2
    }

    func testResult () {
        let f1: Result<Int,TestResultError> = Result.failure(TestResultError.TE1)

        if case .failure = f1 {} else { XCTAssertTrue (false) }
        XCTAssertEqual (1, try f1.recover { (e:TestResultError) -> Int in return 1 }.get())

        var res: Int = 0
        f1.resolve (success: { (ignore:Int) in res = 0 },
                    failure: { (ignore:BRSupportTests.TestResultError) in res = 10})
        XCTAssertEqual(10, res)
    }

}
