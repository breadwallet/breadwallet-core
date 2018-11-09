//
//  AppDelegate.swift
//  CoreXDemo
//
//  Created by Ed Gamble on 11/8/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import UIKit
import BRCrypto

protocol SharedListener {
    static var sharedListener : CoreXDemoListener { get }
}

extension UIApplication : SharedListener {
    static var sharedListener : CoreXDemoListener {
        return (UIApplication.shared.delegate as! AppDelegate).listener
    }
}

@UIApplicationMain
class AppDelegate: UIResponder, UIApplicationDelegate, UISplitViewControllerDelegate {

    var window: UIWindow?
    var listener: CoreXDemoListener!
    var btcManager: BitcoinWalletManager!
    var ethManager: EthereumWalletManager!
    var bchManager: BitcoinWalletManager!

    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        // Override point for customization after application launch.
        let splitViewController = window!.rootViewController as! UISplitViewController
        let navigationController = splitViewController.viewControllers[splitViewController.viewControllers.count-1] as! UINavigationController
        navigationController.topViewController!.navigationItem.leftBarButtonItem = splitViewController.displayModeButtonItem
        splitViewController.delegate = self

        let account = Account (phrase: "boring head harsh green empty clip fatal typical found crane dinner timber")

        self.listener = CoreXDemoListener ()

        self.btcManager = BitcoinWalletManager (listener: listener,
                                                account: account,
                                                network: Network.Bitcoin.mainnet,
                                                mode: WalletManagerMode.p2p_only,
                                                timestamp: 0)

        self.bchManager = BitcoinWalletManager (listener: listener,
                                                account: account,
                                                network: Network.Bitcash.testnet,
                                                mode: WalletManagerMode.p2p_only,
                                                timestamp: 0)

       self.ethManager = EthereumWalletManager (listener: listener,
                                                 account: account,
                                                 network: Network.Ethereum.mainnet,
                                                 mode: WalletManagerMode.p2p_only, // api_with_p2p_submit,
                                                 timestamp: 0)

        return true
    }

    func applicationWillResignActive(_ application: UIApplication) {
        // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
        // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
    }

    func applicationDidEnterBackground(_ application: UIApplication) {
        // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
        // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    }

    func applicationWillEnterForeground(_ application: UIApplication) {
        // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
    }

    func applicationDidBecomeActive(_ application: UIApplication) {
        // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
        DispatchQueue.global().async {
            sleep (5)
            self.btcManager.connect();
            self.bchManager.connect();
            self.ethManager.connect();
        }
    }

    func applicationWillTerminate(_ application: UIApplication) {
        // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
    }

    // MARK: - Split view

    func splitViewController(_ splitViewController: UISplitViewController, collapseSecondary secondaryViewController:UIViewController, onto primaryViewController:UIViewController) -> Bool {
        guard let secondaryAsNavController = secondaryViewController as? UINavigationController else { return false }
        guard let topAsDetailController = secondaryAsNavController.topViewController as? DetailViewController else { return false }
        if topAsDetailController.detailItem == nil {
            // Return true to indicate that we have handled the collapse by doing nothing; the secondary controller will be discarded.
            return true
        }
        return false
    }

}

