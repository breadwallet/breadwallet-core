//
//  AppDelegate.swift
//  CoreXDemo
//
//  Created by Ed Gamble on 11/8/18.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import UIKit
import BRCrypto

protocol SharedSystem {
    static var sharedSystem: System { get }
}

extension UIApplication: SharedSystem {
    static var sharedSystem : System {
        return (UIApplication.shared.delegate as! CoreDemoAppDelegate).system
    }
}

@UIApplicationMain
class CoreDemoAppDelegate: UIResponder, UIApplicationDelegate, UISplitViewControllerDelegate {

    var window: UIWindow?
    var summaryController: SummaryViewController!

    var listener: CoreDemoListener!
    var system: System!
    var mainnet = true
    var accountSpecification: AccountSpecification!

    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        // Override point for customization after application launch.
        let splitViewController = window!.rootViewController as! UISplitViewController

        let walletNavigationController  = splitViewController.viewControllers[1] as! UINavigationController
        walletNavigationController.topViewController!.navigationItem.leftBarButtonItem = splitViewController.displayModeButtonItem
        splitViewController.delegate = self

        let summaryNavigationController = splitViewController.viewControllers[0] as! UINavigationController
        summaryController = (summaryNavigationController.topViewController as! SummaryViewController)

        print ("APP: Bundle Path       : \(Bundle(for: CoreDemoAppDelegate.self).bundlePath)")

        let accountSpecificationsPath = Bundle(for: CoreDemoAppDelegate.self).path(forResource: "CoreTestsConfig", ofType: "json")!
        let accountSpecifications     = AccountSpecification.loadFrom(configPath: accountSpecificationsPath)
        let accountIdentifier         = CommandLine.arguments[1]

        guard let accountSpecification = accountSpecifications.first (where: { $0.identifier == accountIdentifier })
            else {
                precondition (false, "No AccountSpecification: \(accountIdentifier)");
                return false
        }
        self.accountSpecification = accountSpecification

        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "yyyy-MM-dd"
        dateFormatter.locale = Locale(identifier: "en_US_POSIX") // set locale to reliable US_POSIX

        let walletId = UUID (uuidString: "5766b9fa-e9aa-4b6d-9b77-b5f1136e5e96")?.uuidString ?? "empty-wallet-id"

        guard let account = Account.createFrom (phrase: accountSpecification.paperKey,
                                                timestamp: accountSpecification.timestamp,
                                                uids: walletId) else {
            precondition(false, "No account")
            return false
        }

        mainnet = (accountSpecification.network == "mainnet")

        // Ensure the storage path
        let storagePath = FileManager.default
            .urls(for: .documentDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("Core").path

//        do {
//            if FileManager.default.fileExists(atPath: storagePath) {
//                try FileManager.default.removeItem(atPath: storagePath)
//            }
//
//            try FileManager.default.createDirectory (atPath: storagePath,
//                                                     withIntermediateDirectories: true,
//                                                     attributes: nil)
//        }
//        catch let error as NSError {
//            print("Error: \(error.localizedDescription)")
//        }

        print ("APP: Account PaperKey  : \(accountSpecification.paperKey.components(separatedBy: CharacterSet.whitespaces).first ?? "<missed>") ...")
        print ("APP: Account Timestamp : \(account.timestamp)")
        print ("APP: StoragePath       : \(storagePath)");
        print ("APP: Mainnet           : \(mainnet)")
        var currencies: [String] = ["btc", "eth", "brd" /*, "xrp"*/]

        if mainnet {

        }
        else {

        }

        print ("APP: Currencies        : \(currencies)")

        // Create the listener
        let listener = CoreDemoListener (currencyCodesNeeded: currencies,
                                         isMainnet: mainnet)

        // Create the BlockChainDB
        let query = BlockChainDB.createForTest ()

        // Create the system
        self.listener = listener
        self.system = System (listener: listener,
                              account: account,
                              onMainnet: mainnet,
                              path: storagePath,
                              query: query)
        
        // Subscribe to notificiations or not (Provide an endpoint if notifications are enabled).
        let subscriptionId = UIDevice.current.identifierForVendor!.uuidString
        let subscription = BlockChainDB.Subscription (id: subscriptionId, endpoint: nil);
        self.system.subscribe (using: subscription)

        self.system.configure(withCurrencyModels: [])

        return true
    }

    func applicationWillResignActive(_ application: UIApplication) {
        // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
        // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
    }

    func applicationDidEnterBackground(_ application: UIApplication) {
        // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
        // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
        system.stop()
    }

    func applicationWillEnterForeground(_ application: UIApplication) {
        // Called as part of the transition from the background to the active state; here you can
        // undo many of the changes made on entering the background.
        system.managers.forEach { $0.connect() }
    }

    func applicationDidBecomeActive(_ application: UIApplication) {
        // Restart any tasks that were paused (or not yet started) while the application was
        // inactive. If the application was previously in the background, optionally refresh the
        // user interface.
    }

    func applicationWillTerminate(_ application: UIApplication) {
        // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
    }

    // MARK: - Split view

    func splitViewController(_ splitViewController: UISplitViewController, collapseSecondary secondaryViewController:UIViewController, onto primaryViewController:UIViewController) -> Bool {
        guard let secondaryAsNavController = secondaryViewController as? UINavigationController else { return false }
        guard let topAsDetailController = secondaryAsNavController.topViewController as? WalletViewController else { return false }
        if topAsDetailController.wallet == nil {
            // Return true to indicate that we have handled the collapse by doing nothing; the secondary controller will be discarded.
            return true
        }
        return false
    }

}


extension UIApplication {
    static var paperKey: String {
        return (UIApplication.shared.delegate as! CoreDemoAppDelegate).accountSpecification.paperKey
    }

    static func sync () {
        guard let app = UIApplication.shared.delegate as? CoreDemoAppDelegate else { return }
        print ("APP: Syncing")
        app.system.managers.forEach { $0.sync() }
    }

    static func sleep() {
        guard let app = UIApplication.shared.delegate as? CoreDemoAppDelegate else { return }
        print ("APP: Disconnecting")
        app.system.managers.forEach { $0.disconnect() }
        DispatchQueue.main.asyncAfter(deadline: .now() + 15.0) {
            print ("APP: Connecting")
            app.system.managers.forEach { $0.connect() }
        }
    }

    static func reset () {
        guard let app = UIApplication.shared.delegate as? CoreDemoAppDelegate else { return }
        print ("APP: Resetting")

        // Create a new system
        let system = System (listener: app.system.listener!,
                             account: app.system.account,
                             onMainnet: app.system.onMainnet,
                             path: app.system.path,
                             query: app.system.query)

        // Stop the existing system
        app.system.stop()
        app.summaryController.reset()

        
        // Assign and then configure the new system
        app.system = system
        app.system.configure(withCurrencyModels: [])
    }
}

extension Network {
    var scheme: String? {
        switch self.currency.code {
        case Currency.codeAsBTC: return "bitcoin"
        case Currency.codeAsBCH: return "bitcoincash" // bchtest
        case Currency.codeAsETH: return "ethereum"
        default: return nil
        }
    }
}

