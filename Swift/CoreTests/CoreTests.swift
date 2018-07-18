//
//  CoreTests.swift
//  CoreTests
//
//  Created by Ed Gamble on 5/18/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import XCTest

class CoreTests: XCTestCase {
    
    override func setUp() {
        super.setUp()
    }
    
    override func tearDown() {
        super.tearDown()
    }

    func testBitcoin () {
        BRRunTests()
    }

    func testEthereumBasics() {
        runTests(0)
    }

    func testLES () {
        runLEStests()
    }

    func testEthereumSync () {
        runSyncTest (3 * 60, 0);
//        runSyncTest (1 * 60, 1);
    }
    
//    func testPerformanceExample() {
//        self.measure {
//        }
//    }
}
