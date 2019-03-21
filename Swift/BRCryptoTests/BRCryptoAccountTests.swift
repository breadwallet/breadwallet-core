//
//  BRCryptoAccountTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 3/21/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//

import XCTest
@testable import BRCrypto

class BRCryptoAccountTests: XCTestCase {

    override func setUp() {
    }

    override func tearDown() {
    }

    func testAddress () {

    }

    func testAccount () {
        let _ = Account (phrase: "ginger settle marine tissue robot crane night number ramp coast roast critic")
        let s1 = Account.deriveSeed(phrase: "ginger settle marine tissue robot crane night number ramp coast roast critic")
        let _ = Account (seed: s1)
    }

    func testAddress () {
        let r1 = Address (raw: "foo")
        let r2 = Address (raw: "bar")
        let r3 = Address (raw: "foo")

        XCTAssertEqual (r1, r3)
        XCTAssertNotEqual (r1, r2)

        let e1 = Address (ethereum: "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62")
        let e2 = Address (ethereum: "0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f")

        XCTAssertEqual(e1, e1)
        XCTAssertNotEqual (e1, e2)
        XCTAssertNotEqual (r1, e1)

        // hashable
        XCTAssertEqual (r1.hashValue, r1.hashValue);
        XCTAssertEqual (e1.hashValue, e1.hashValue);

        XCTAssertNotEqual (e1.hashValue, e2.hashValue);

        XCTAssertEqual("foo", r1.description)
        XCTAssertEqual("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", e1.description)
    }
}
