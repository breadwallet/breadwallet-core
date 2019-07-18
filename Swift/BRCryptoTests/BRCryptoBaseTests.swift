//
//  BRCryptoBaseTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 3/28/19.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import XCTest
import BRCrypto

class BRCryptoBaseTests: XCTestCase {

    struct AccountSpecification {
        let identifier: String
        let network: String
        let paperKey: String
        let timestamp: Date

        init (dict: [String: String]) {
            self.identifier = dict["identifier"]! //as! String
            self.network    = dict["network"]!
            self.paperKey   = dict["paperKey"]!

            let dateFormatter = DateFormatter()
            dateFormatter.dateFormat = "yyyy-MM-dd"
            dateFormatter.locale = Locale(identifier: "en_US_POSIX")

            self.timestamp = dateFormatter.date(from: dict["timestamp"]!)!
        }
    }

    var accountSpecifications: [AccountSpecification] = []
    var accountSpecification: AccountSpecification! {
        return accountSpecifications.count > 0
            ? accountSpecifications[0]
            : nil
    }

    var isMainnet = true

    let configPath = Bundle(for: BRCryptoBaseTests.self).path(forResource: "CoreTestsConfig", ofType: "json")!

    var coreDataDir: String!

    func coreDirClear () {
        do {
            if FileManager.default.fileExists(atPath: coreDataDir) {
                try FileManager.default.removeItem(atPath: coreDataDir)
            }
        }
        catch {
            print ("Error: \(error)")
            XCTAssert(false)
        }
    }

    func coreDirCreate () {
        do {
            try FileManager.default.createDirectory (atPath: coreDataDir,
                                                     withIntermediateDirectories: true,
                                                     attributes: nil)
        }
        catch {
            XCTAssert(false)
        }
    }

    var account: Account!

    override func setUp() {
        super.setUp()

        #if TESTNET
        isMainnet = false
        #endif

        // Get the paperKey from `configPath`
        if FileManager.default.fileExists(atPath: configPath) {
            let configFile = URL(fileURLWithPath: configPath)
            let configData = try! Data.init(contentsOf: configFile)
            let json = try! JSONSerialization.jsonObject(with: configData, options: []) as! [[String:String]]
            accountSpecifications = json
                .map { AccountSpecification (dict: $0) }
                .filter { $0.network == (isMainnet ? "mainnet" : "testnet") }
        }
        else {
            accountSpecifications = [
                AccountSpecification (dict: [
                    "identifier": "ginger",
                    "paperKey":   "ginger settle marine tissue robot crane night number ramp coast roast critic",
                    "timestamp":  "2018-01-01",
                    "network":    (isMainnet ? "mainnet" : "testnet")
                ])
            ]
        }

        let specifiction = accountSpecification!

        /// Create the account
        let walletId = UUID (uuidString: "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96")?.uuidString ?? "empty-wallet-id"
        account = Account.createFrom (phrase: specifiction.paperKey,
                                      timestamp: specifiction.timestamp,
                                      uids: walletId)

        /// Create the 'storagePath'
        coreDataDir = FileManager.default
            .urls(for: .documentDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("Core").path

        coreDirCreate()
        coreDirClear()
   }

    func testBase() {
        XCTAssert (nil != coreDataDir)
    }

    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }
}
