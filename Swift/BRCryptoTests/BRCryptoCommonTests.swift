//
//  BRCryptoCommonTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 7/18/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import XCTest
import Foundation
import BRCrypto

class BRCryptoCommonTests: XCTestCase {

    override func setUp() { }

    override func tearDown() { }

    func testHasher() {
        var d: Data!
        var a: Data!
        var r: Data! = nil

        // SHA1
        d = "Free online SHA1 Calculator, type text here..."
            .data(using: String.Encoding.utf8)
        a = Data ([0x6f, 0xc2, 0xe2, 0x51, 0x72, 0xcb, 0x15, 0x19, 0x3c, 0xb1, 0xc6, 0xd4, 0x8f, 0x60, 0x7d, 0x42, 0xc1, 0xd2, 0xa2, 0x15])
        r = CoreCryptoHasher.sha1.hash(data: d)
        XCTAssertEqual (a, r)

        // ...
    }

    func testEncoder () {
        var d: Data!
        var a: String
        var r: String

        // HEX
        d = Data([0xde, 0xad, 0xbe, 0xef])
        a = "deadbeef"
        r = CoreCryptoCoder.hex.encode(data: d)
        XCTAssertEqual (a, r)
        XCTAssertEqual (d, CoreCryptoCoder.hex.decode(string: r))

        // ...
    }

    func testEncryptor () {

        // aes_ecb
    }

    func testSigner () {

        // compact
    }
}
