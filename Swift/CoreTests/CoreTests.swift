//
//  CoreTests.swift
//  CoreTests
//
//  Created by Ed Gamble on 5/18/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//
import XCTest

class CoreTests: XCTestCase {

    static let PAPER_KEY_MAINNET = "paperKeysMainnet"
    static let PAPER_KEY_TESTNET = "paperKeysTestnet"

    var isMainnet: Int32! = 1
    var isBTC: Int32! = 1

    var paperKeys: [String] = []

    var paperKey: String! {
        return (paperKeys.count > 0
            ? paperKeys[0]
            : nil)
    }

    var account: BREthereumAccount!

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
        var fakeEthAccount: String = "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62"

        let configPath = (CommandLine.argc > 1 && !CommandLine.arguments[1].starts(with: "-")
            ? CommandLine.arguments[1]
            : "/Users/ebg/.brdCoreTestConfig.plist")

        if FileManager.default.fileExists(atPath: configPath) {
            let configFile = URL(fileURLWithPath: configPath)
            let configData = try! Data.init(contentsOf: configFile)
            let configPropertyList = try! PropertyListSerialization.propertyList(from: configData, options: [], format: nil) as! [String: [String]]

            #if TESTNET
            paperKeys = configPropertyList [CoreTests.PAPER_KEY_TESTNET]!
            #else
            paperKeys = configPropertyList [CoreTests.PAPER_KEY_MAINNET]!
            #endif
        }
        else if 0 == isMainnet /* testnet */ {
            // This is a compromised testnet paperkey
            paperKeys = ["ginger settle marine tissue robot crane night number ramp coast roast critic"]
            fakeEthAccount = "0x8fB4CB96F7C15F9C39B3854595733F728E1963Bc"
        }

        account = createAccount (nil != paperKey ? paperKey : fakeEthAccount)

        coreDataDir = FileManager.default
            .urls(for: .documentDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("Core").path

        coreDirCreate()
        coreDirClear()

    }
    
    override func tearDown() {
        super.tearDown()
    }

    func testBitcoinSupport () {
        XCTAssert(1 == BRRunSupTests ())
    }
    
    func testBitcoin () {
        XCTAssert(1 == BRRunTests())
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
    
    func testEthereumBasics() {
        runTests(0)
    }

    func testEWM () {
        runEWMTests(paperKey);
    }

    func testLES () {
        runLESTests(paperKey)
        runNodeTests()
    }


    /// Run an Etheruem Sync.  Two syncs are run back-to-back with the second sync meant to
    /// start from the saved state of the first sync.
    ///
    /// Note: The first sync saves state to the file system..
    ///
    /// - Throws: something
    ///
    func testEthereumSyncStorage () throws {
        let mode = BRD_WITH_P2P_SEND; // BRD_ONLY;  P2P_WITH_BRD_SYNC; // P2P_ONLY,  BRD_WITH_P2P_SEND
        let timestamp : UInt64 = 0

        let network = (isMainnet == 1 ? ethereumMainnet : ethereumTestnet)

        print ("ETH: TST: Core Dir: \(coreDataDir!)")
        coreDirClear()
        runSyncTest (network, account, mode, timestamp, 5 * 60, coreDataDir, 0);
        runSyncTest (network, account, mode, timestamp, 1 * 60, coreDataDir, 1);
    }

    /// Run a single bitcoin sync using the provided paperKey
    ///
    func testBitcoinSyncOne() {
        BRRunTestsSync (paperKey, isBTC, isMainnet);
    }

    /// Run 25 simultaneous bitcoin syncs using the provided paperKeys and random keys after
    /// that.
    ///
    func testBitcoinSyncMany () {
        let group = DispatchGroup.init()
        for i in 1...25 {
            DispatchQueue.init(label: "Sync \(i)")
                .async {
                    group.enter()
                    let paperKey = i <= self.paperKeys.count ? self.paperKeys[i - 1] : nil
                    BRRunTestsSync (paperKey, self.isBTC, self.isMainnet);
                    group.leave()
            }
        }
        group.wait()
    }

    ///
    /// Run a bitcoin sync using the (new) BRWalletManager which encapsulates BRWallet and
    /// BRPeerManager with 'save' callbacks using the file system.
    ///
    func testBitcoinWalletManagerSync () {
        print ("ETH: TST: Core Dir: \(coreDataDir!)")
        coreDirClear()
        BRRunTestWalletManagerSync (paperKey, coreDataDir, isBTC, isMainnet);
        BRRunTestWalletManagerSync (paperKey, coreDataDir, isBTC, isMainnet);
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

