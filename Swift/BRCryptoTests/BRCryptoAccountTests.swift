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

    func testAccount () {
        let phrase  = "ginger settle marine tissue robot crane night number ramp coast roast critic"
        let address = "0x8fB4CB96F7C15F9C39B3854595733F728E1963Bc"

        guard let a1 = Account.createFrom (phrase: phrase)
            else { XCTAssert(false); return}

        XCTAssert (a1.addressAsETH == address)
        XCTAssert (0 == a1.timestamp)


        let d2 = Account.deriveSeed (phrase: phrase)
        guard let a2 = Account.createFrom (seed: d2) else { XCTAssert (false); return }
        XCTAssert (a2.addressAsETH == address)
    }

    func testAddress () {
        let e1 = Address.createAsETH ("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62")
        let e2 = Address.createAsETH ("0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f")

        XCTAssertNotNil (e1)
        XCTAssertNotNil (e2)

        XCTAssertEqual("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", e1?.description)
        XCTAssertEqual("0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f", e2?.description)

        let b1 = Address.createAsBTC ("1CC3X2gu58d6wXUWMffpuzN9JAfTUWu4Kj")

        XCTAssertNotNil (b1)
        XCTAssertEqual("1CC3X2gu58d6wXUWMffpuzN9JAfTUWu4Kj",  b1?.description)

        let e3 = Address.createAsETH ("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62")

        XCTAssertEqual (e1, e1)
        XCTAssertEqual (e1, e3)
        XCTAssertEqual (e3, e1)

        XCTAssertNotEqual (e1, e2)
        XCTAssertNotEqual (e2, e1)
        XCTAssertNotEqual (e1, b1)
        XCTAssertNotEqual (b1, e1)
    }
}
