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

    func testEthereum() {
        runTests()
    }

    func testLES () {
//        runLEStests()
    }
    
    func testPerformanceExample() {
        self.measure {
        }
    }
    
}
