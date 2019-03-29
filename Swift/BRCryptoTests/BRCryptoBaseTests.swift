//
//  BRCryptoBaseTests.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 3/28/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//

import XCTest

class BRCryptoBaseTests: XCTestCase {

    static let PAPER_KEY_MAINNET = "paperKeysMainnet"
    static let PAPER_KEY_TESTNET = "paperKeysTestnet"

    let isMainnet = 1

    var paperKeys: [String] = []

    var paperKey: String! {
        return (paperKeys.count > 0
            ? paperKeys[0]
            : nil)
    }

    let configPath = Bundle(for: BRCryptoBaseTests.self).path(forResource: "CoreTestsConfig", ofType: "plist")!

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


    override func setUp() {
        super.setUp()

        #if TESTNET
        isMainnet = 0
        #endif

        // Eth Account for the non-compromised, mainnet paperKey "e...a"
//        var fakeEthAccount: String = "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62"

        if FileManager.default.fileExists(atPath: configPath) {
            let configFile = URL(fileURLWithPath: configPath)
            let configData = try! Data.init(contentsOf: configFile)
            let configPropertyList = try! PropertyListSerialization.propertyList(from: configData, options: [], format: nil) as! [String: [String]]

            #if TESTNET
            paperKeys = configPropertyList [BRCryptoBaseTests.PAPER_KEY_TESTNET]!
            #else
            paperKeys = configPropertyList [BRCryptoBaseTests.PAPER_KEY_MAINNET]!
            #endif
        }
        else if 0 == isMainnet /* testnet */ {
            // This is a compromised testnet paperkey
            paperKeys = ["ginger settle marine tissue robot crane night number ramp coast roast critic"]
//            fakeEthAccount = "0x8fB4CB96F7C15F9C39B3854595733F728E1963Bc"
        }

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
