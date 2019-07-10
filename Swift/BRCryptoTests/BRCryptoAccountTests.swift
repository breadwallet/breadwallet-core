//
//  BRCryptoAccountTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 3/21/19.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import XCTest
@testable import BRCrypto

class BRCryptoAccountTests: XCTestCase {

    let dateFormatter = DateFormatter()

    override func setUp() {
        dateFormatter.dateFormat = "yyyy-MM-dd"
        dateFormatter.locale = Locale(identifier: "en_US_POSIX") // set locale to reliable US_POSIX

    }

    override func tearDown() {
    }

    func testAccount () {
        let phrase  = "ginger settle marine tissue robot crane night number ramp coast roast critic"
        let address = "0x8fB4CB96F7C15F9C39B3854595733F728E1963Bc"
        let timestamp = dateFormatter.date(from: "2018-01-01")!

        let walletId = UUID (uuidString: "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96")?.uuidString ?? "empty-wallet-id"
        guard let a1 = Account.createFrom (phrase: phrase, timestamp: timestamp, uids: walletId)
            else { XCTAssert(false); return}

        XCTAssertEqual (a1.addressAsETH, address)
        XCTAssertEqual (timestamp, a1.timestamp)

        let serialization = a1.serialize
        guard let a2 = Account.createFrom (serialization: serialization, uids: walletId)
            else { XCTAssert(false); return }

        XCTAssertEqual (a2.addressAsETH, a1.addressAsETH);
    }

    func testAddressETH () {
        let eth = Currency (uids: "Ethereum", name: "Ethereum", code: "ETH", type: "native", issuer: nil)
        let network = Network (uids: "ethereum-mainnet",
                               name: "ethereum-name",
                               isMainnet: true,
                               currency: eth,
                               height: 100000,
                               associations: [:])

        let e1 = Address.create (string: "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", network: network)
        let e2 = Address.create (string: "0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f", network: network)

        XCTAssertNotNil (e1)
        XCTAssertNotNil (e2)

        XCTAssertEqual("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", e1?.description)
        XCTAssertEqual("0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f", e2?.description)

        let e3 = Address.create (string: "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", network: network)

        XCTAssertEqual (e1, e1)
        XCTAssertEqual (e1, e3)
        XCTAssertEqual (e3, e1)

        XCTAssertNotEqual (e1, e2)
        XCTAssertNotEqual (e2, e1)
    }

    func testAddressBTC () {
        let btc = Currency (uids: "Bitcoin",  name: "Bitcoin",  code: "BTC", type: "native", issuer: nil)
        let network = Network (uids: "bitcoin-mainnet",
                               name: "bitcoin-name",
                               isMainnet: true,
                               currency: btc,
                               height: 100000,
                               associations: [:])

        let b1 = Address.create (string: "1CC3X2gu58d6wXUWMffpuzN9JAfTUWu4Kj", network: network)

        XCTAssertNotNil (b1)
        XCTAssertEqual("1CC3X2gu58d6wXUWMffpuzN9JAfTUWu4Kj",  b1?.description)

    }

}
