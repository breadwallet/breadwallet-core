//
//  AppDelegate.swift
//  CoreDemo
//
//  Created by Ed Gamble on 7/26/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import UIKit
import BRCore

protocol SharedClient {
    static var sharedClient : CoreDemoEthereumClient { get }
}

extension UIApplication : SharedClient {
    static var sharedClient : CoreDemoEthereumClient {
        return (UIApplication.shared.delegate as! AppDelegate).client
    }
}

@UIApplicationMain
class AppDelegate: UIResponder, UIApplicationDelegate, UISplitViewControllerDelegate {

    var window: UIWindow?
    var client : CoreDemoEthereumClient!


    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        // Override point for customization after application launch.
        let splitViewController = window!.rootViewController as! UISplitViewController

        let summaryNavigationController = splitViewController.viewControllers[0] as! UINavigationController
        let walletNavigationController  = splitViewController.viewControllers[1] as! UINavigationController

        walletNavigationController.topViewController!.navigationItem.leftBarButtonItem = splitViewController.displayModeButtonItem
        splitViewController.delegate = self

        let summaryController = summaryNavigationController.topViewController as! SummaryViewController

        let paperKey = (CommandLine.argc > 1
            ? CommandLine.arguments[1]
            : "0xa9de3dbd7d561e67527bc1ecb025c59d53b9f7ef");
        //                                         paperKey: "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62")
        //                                         paperKey: "0x8975dbc1b8f25ec994815626d070899dda896511")
        //                                         paperKey: "0xb302B06FDB1348915599D21BD54A06832637E5E8")

        client = CoreDemoEthereumClient (network: EthereumNetwork.mainnet,
                                         type: EthereumType.les,
                                         mode: EthereumSyncMode.blockchain,
                                         paperKey:  paperKey)

        // Mini-race - on ETH wallet create event
        UIApplication.sharedClient.addWalletListener(listener: summaryController)

        return true
    }

    func applicationWillResignActive(_ application: UIApplication) {
        // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
        // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
        client.node.disconnect();
    }

    func applicationDidEnterBackground(_ application: UIApplication) {
        // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
        // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    }

    func applicationWillEnterForeground(_ application: UIApplication) {
        // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
    }

    func applicationDidBecomeActive(_ application: UIApplication) {
        // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface
        DispatchQueue.global().async {
            sleep (5)
            self.client.node.updateTokens()
            self.client.node.connect();
        }
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

