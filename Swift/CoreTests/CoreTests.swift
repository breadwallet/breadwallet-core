//
//  CoreTests.swift
//  CoreTests
//
//  Created by Ed Gamble on 5/18/18.
//  Copyright © 2018-2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import XCTest

class CoreTests: XCTestCase {

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
    var paperKey: String! = nil

    var isMainnet = true
    var isBTC: Int32! = 1

    let configPath = Bundle(for: CoreTests.self).path(forResource: "CoreTestsConfig", ofType: "json")!

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

        let fakeEthAccount: String = (isMainnet
            ? "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62"
            : "0x8fB4CB96F7C15F9C39B3854595733F728E1963Bc")


        paperKey = (nil != accountSpecification
            ? accountSpecification.paperKey
            : "ginger settle marine tissue robot crane night number ramp coast roast critic")

        account = createAccount (nil != accountSpecification
            ? accountSpecification.paperKey
            : fakeEthAccount)

        #if false

        // Eth Account for the non-compromised, mainnet paperKey "e...a"
        let fakeEthAccount: String = (isMainnet
            ? "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62"
            : "0x8fB4CB96F7C15F9C39B3854595733F728E1963Bc")

        if FileManager.default.fileExists(atPath: configPath) {
            let configFile = URL(fileURLWithPath: configPath)
            let configData = try! Data.init(contentsOf: configFile)
            let configPropertyList = try! PropertyListSerialization.propertyList(from: configData, options: [], format: nil) as! [String: [String]]

            paperKeys = configPropertyList [CoreTests.PAPER_KEY_MAINNET]!
        }
        else if 0 == isMainnet /* testnet */ {
            // This is a compromised testnet paperkey
            paperKeys = ["ginger settle marine tissue robot crane night number ramp coast roast critic"]
            fakeEthAccount = "0x8fB4CB96F7C15F9C39B3854595733F728E1963Bc"
        }

        account = createAccount (nil != paperKey ? paperKey : fakeEthAccount)

        #endif
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
        XCTAssert(1 == BRRunTestsBWM (paperKey, coreDataDir, isBTC, (isMainnet ? 1 : 0)));
    }

    //
    func testCrypto () {
        runCryptoTests ()
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

    func testEthereumBase () {
        runBaseTests()
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
        runEWMTests(paperKey, coreDataDir);
    }

    func testLES () {
        runLESTests(paperKey)
        runNodeTests()
    }

    ///
    /// Ripple
    ///
    func testRipple () {
        runRippleTest ()
    }

    /// Run an Etheruem Sync.  Two syncs are run back-to-back with the second sync meant to
    /// start from the saved state of the first sync.
    ///
    /// Note: The first sync saves state to the file system..
    ///
    /// - Throws: something
    ///
    func testEthereumSyncStorage () throws {
        let mode = SYNC_MODE_P2P_ONLY;
        let timestamp : UInt64 = 0

        let network = (isMainnet ? ethereumMainnet : ethereumTestnet)

        print ("ETH: TST: Core Dir: \(coreDataDir!)")
        coreDirClear()
        runSyncTest (network, account, mode, timestamp, 5 * 60, coreDataDir);
        runSyncTest (network, account, mode, timestamp, 1 * 60, coreDataDir);
    }

    /// Run a single bitcoin sync using the provided paperKey
    ///
    func testBitcoinSyncOne() {
        BRRunTestsSync (paperKey, isBTC, (isMainnet ? 1 : 0));
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
                    let paperKey = i <= self.accountSpecifications.count ? self.accountSpecifications[i - 1].paperKey : nil
                    BRRunTestsSync (paperKey, self.isBTC, (self.isMainnet ? 1 : 0));
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
        BRRunTestWalletManagerSync (paperKey, coreDataDir, isBTC, (isMainnet ? 1 : 0));
        BRRunTestWalletManagerSync (paperKey, coreDataDir, isBTC, (isMainnet ? 1 : 0));
    }

    func testBitcoinWalletManagerSyncModes() {
        var success: Int32 = 0
        var mainnet: Int32 = 0
        var btc: Int32 = 0
        var blockHeight: UInt64 = 0

        // BTC mainnet
        btc = 1
        mainnet = 1;
        blockHeight = 500000;

        coreDirClear();
        success = BRRunTestWalletManagerSyncStress(paperKey, coreDataDir, 0, blockHeight, btc, mainnet);
        XCTAssertEqual(1, success)

        // BTC testnet
        btc = 1
        mainnet = 0;
        blockHeight = 1500000;

        coreDirClear();
        success = BRRunTestWalletManagerSyncStress(paperKey, coreDataDir, 0, blockHeight, btc, mainnet);
        XCTAssertEqual(1, success)

        // BCH mainnet
        btc = 0
        mainnet = 1;
        blockHeight = 500000;

        coreDirClear();
        success = BRRunTestWalletManagerSyncStress(paperKey, coreDataDir, 0, blockHeight, btc, mainnet);
        XCTAssertEqual(1, success)

        // BCH testnet
        btc = 0
        mainnet = 0;
        blockHeight = 1500000;

        coreDirClear();
        success = BRRunTestWalletManagerSyncStress(paperKey, coreDataDir, 0, blockHeight, btc, mainnet);
        XCTAssertEqual(1, success)
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

