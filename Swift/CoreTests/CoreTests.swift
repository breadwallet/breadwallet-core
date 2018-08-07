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

    //
    // Ethereum
    //
    func testEthereumRlp () {
        runRlpTests();
    }

    func testEthereumUtil () {
        runUtilTests();
    }

    func testEthereumEvent () {
        runEventTests ();
    }

    func testEthereumBlockChain () {
        runBcTests()
    }

    func testEthereumContract () {
        runContractTests ();
    }
    
    func testEWM () {
        runEWMTests();
    }

    func testLES () {
        runLEStests()
    }

    func testEthereumBasics() {
        runTests(0)
    }

    func testEthereumSync () {
        runSyncTest (2 * 60, 0);
        runSyncTest (1 * 60, 1);
    }
    
//    func testPerformanceExample() {
//        self.measure {
//        }
//    }
}
