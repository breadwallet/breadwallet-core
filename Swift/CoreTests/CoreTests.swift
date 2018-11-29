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
        runNodeTests()
    }

    func testEthereumBasics() {
        runTests(0)
    }

    func testEthereumSync () {
        let paperKey = (CommandLine.argc > 1 && !CommandLine.arguments[1].starts(with: "-")
            ? CommandLine.arguments[1]
            : "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62");

        let mode = P2P_WITH_BRD_SYNC
        let timestamp : UInt64 = 0

        runSyncTest (paperKey, mode, timestamp, 10 * 60, 0);
        runSyncTest (paperKey, mode, timestamp,  1 * 60, 1);
    }

    func testBitcoinSync () {
        for _ in 1...10 {
            BRRunTestsSync (1);
        }
    }

    func testBitcoinWalletManagerSync () {
        let documents = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let coreDataDir = documents.appendingPathComponent("Core").path

        do {
            try FileManager.default.createDirectory (atPath: coreDataDir,
                                                     withIntermediateDirectories: true,
                                                     attributes: nil)
            BRRunTestWalletManagerSync(CommandLine.arguments[1], coreDataDir);
        }
        catch let error as NSError {
            print("Error: \(error.localizedDescription)")
        }
    }

    func testPerformanceExample() {
//        runTests(0);
        self.measure {
            runPerfTestsCoder (10, 0);
        }
    }
}

/*

// Many = 0 (w/ coverage)
 /Users/ebg/Bread/BreadWalletCore/Swift/CoreTests/CoreTests.swift:67: Test Case '-[CoreTests.CoreTests testPerformanceExample]'
 measured [Time, seconds] average: 0.335, relative standard deviation: 1.221%,
 values: [0.343131, 0.330704, 0.333884, 0.343019, 0.335021, 0.334110, 0.331773, 0.332601, 0.335724, 0.333770],
 performanceMetricID:com.apple.XCTPerformanceMetric_WallClockTime, baselineName: "", baselineAverage: ,
 maxPercentRegression: 10.000%, maxPercentRelativeStandardDeviation: 10.000%, maxRegression: 0.100, maxStandardDeviation: 0.100

 average: 0.335
 average: 0.344
 average: 0.326
 average: 0.322
 average: 0.319
 average: 0.320

// Many = 0 (w/o coverage)
 average: 0.237
 average: 0.236
 average: 0.239
 average: 0.239

// Many = 1
 /Users/ebg/Bread/BreadWalletCore/Swift/CoreTests/CoreTests.swift:67: Test Case '-[CoreTests.CoreTests testPerformanceExample]'
 measured [Time, seconds] average: 0.355, relative standard deviation: 2.112%,
 values: [0.359623, 0.357172, 0.352454, 0.358753, 0.357557, 0.362583, 0.348528, 0.365165, 0.337512, 0.353536],
 performanceMetricID:com.apple.XCTPerformanceMetric_WallClockTime, baselineName: "", baselineAverage: ,
 maxPercentRegression: 10.000%, maxPercentRelativeStandardDeviation: 10.000%, maxRegression: 0.100, maxStandardDeviation: 0.100

 average: 0.355
 average: 0.346
 average: 0.358
 average: 0.347

 // Many = 1 (w/o coverage)
 average: 0.248
 average: 0.249
 average: 0.246
 average: 0.245

*/

