//
//  SummaryViewController.swift
//  CoreDemo
//
//  Created by Ed Gamble on 7/26/18.
//  Copyright © 2018-2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import UIKit
import BRCrypto

class SummaryViewController: UITableViewController, WalletListener {

    // The wallets currently displayed.
    var wallets = [Wallet]()

    var detailViewController: WalletViewController? = nil

    func updateTitle () {
        self.navigationItem.title = "\(UIApplication.sharedSystem.onMainnet ? "Mainnet" : "Testnet") Wallets (\(UIApplication.paperKey.components(separatedBy: " ").first!))"
    }
    
    func reset () {
        DispatchQueue.main.async {
            self.wallets = []
            self.detailViewController.map{ $0.reset() }
            self.updateTitle()
            self.tableView.reloadData()
        }
    }

    func update () {
        DispatchQueue.main.async {
            self.wallets = UIApplication.sharedSystem.wallets
            self.updateTitle()
            self.tableView.reloadData()
        }
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        if let split = splitViewController {
            let controllers = split.viewControllers
            detailViewController = (controllers[controllers.count-1] as! UINavigationController).topViewController as? WalletViewController
        }
    }

    override func viewWillAppear (_ animated: Bool) {
        super.viewWillAppear(animated)
        self.updateTitle()

        if let listener = UIApplication.sharedSystem.listener as? CoreDemoListener {
            listener.add (walletListener: self)
        }
        
        clearsSelectionOnViewWillAppear = splitViewController!.isCollapsed

        // Reload the data - don't miss any wallets
        update ()
    }

    override func viewWillDisappear(_ animated: Bool) {
        if let listener = UIApplication.sharedSystem.listener as? CoreDemoListener {
            listener.remove (walletListener: self)
        }
        
        super.viewWillDisappear(animated)
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }

    // MARK: - Segues

    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        switch segue.identifier {
        case "showWallet":
            if let indexPath = tableView.indexPathForSelectedRow {
                let wallet = wallets[indexPath.row]
                let controller = (segue.destination as! UINavigationController).topViewController as! WalletViewController

                controller.wallet = wallet;
                controller.navigationItem.leftBarButtonItem = splitViewController?.displayModeButtonItem
                controller.navigationItem.leftItemsSupplementBackButton = true
            }

        case "showManagers":
            let controller = (segue.destination as! UINavigationController).topViewController as! WalletManagersTableViewController
            controller.managers = UIApplication.sharedSystem.managers

        default:
            break;
        }
    }


    @IBAction func doAct (_ sender: Any) {
        let alert = UIAlertController (title: "Act",
                                       message: nil,
                                       preferredStyle: UIAlertController.Style.alert)

        alert.addAction (UIAlertAction (title: "Reset", style: UIAlertAction.Style.default) { (action) in
            UIApplication.doReset()
            alert.dismiss(animated: true) {}
        })

        alert.addAction (UIAlertAction (title: "Wipe", style: UIAlertAction.Style.default) { (action) in
            UIApplication.doWipe()
            alert.dismiss(animated: true) {}
        })

        alert.addAction (UIAlertAction (title: "Sync", style: UIAlertAction.Style.default) { (action) in
            UIApplication.doSync()
            alert.dismiss(animated: true) {}
        })

        alert.addAction (UIAlertAction (title: "Update Fees", style: UIAlertAction.Style.default) { (action) in
            UIApplication.doUpdateFees()
            alert.dismiss(animated: true) {}
        })

        alert.addAction(UIAlertAction (title: "Show Wallet Managers", style: UIAlertAction.Style.default) { (action) in
            self.showManagersButton.sendActions (for: .touchUpInside)
            alert.dismiss(animated: true) {}
        })

        alert.addAction (UIAlertAction (title: "Sleep Eth", style: UIAlertAction.Style.default) { (action) in
            UIApplication.doSleep()
            alert.dismiss(animated: true) {}
        })

        alert.addAction (UIAlertAction (title: "Cancel", style: UIAlertAction.Style.cancel))

        self.present (alert, animated: true) {}
    }
    
    @IBAction func doAddWallet(_ sender: Any) {
        let system = UIApplication.sharedSystem
        let currencies = system.managers
            .flatMap { $0.network.currencies }
            // Remove any currency already in `wallets`
            .filter { (c) in !wallets.contains { c == $0.currency } }

        let alert = UIAlertController (title: "Add Wallet",
                                       message: nil,
                                       preferredStyle: UIAlertController.Style.actionSheet)

        currencies.forEach { (c) in
            // The manager for (c)`
            if let manager = system.managers.first (where: { $0.network.hasCurrency(c) }) {
                alert.addAction (UIAlertAction (title: "\(c.code) (\(manager.name))", style: .default) { (action) in
                    let _ = manager.registerWalletFor(currency: c)
                    alert.dismiss(animated: true) {}
                })
            }
        }

        alert.addAction (UIAlertAction (title: "Cancel", style: UIAlertAction.Style.cancel))
        self.present (alert, animated: true) {}
        
    }
    // MARK: - Table View

    override func numberOfSections(in tableView: UITableView) -> Int {
        return 1
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return wallets.count
    }

    override func tableView (_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell (withIdentifier: "WalletCell", for: indexPath) as! WalletTableViewCell
        cell.wallet = wallets[indexPath.row]
        return cell
    }

    override func tableView (_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
        return true
    }

    override func tableView (_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
        switch editingStyle {
        case .delete:
            wallets.remove (at: indexPath.row)
            tableView.deleteRows(at: [indexPath], with: .fade)

        case .insert:
            // impossible
            break

        default:
            break
        }
    }

    func handleWalletEvent (system: System,
                            manager: WalletManager,
                            wallet: Wallet,
                            event: WalletEvent) {
        DispatchQueue.main.async {
            print ("APP: SVC: WalletEvent (\(manager.name):\(wallet.name)): \(event)")
            // if visible ...
            switch event {
            case .created:
                precondition (!self.wallets.contains (wallet))

                self.wallets.append (wallet)

                let path = IndexPath (row: (self.wallets.count - 1), section: 0)
                self.tableView.insertRows (at: [path], with: .automatic)

            case .balanceUpdated:
                if let index = self.wallets.firstIndex (of: wallet) {
                    let path = IndexPath (row: index, section: 0)
                    if let cell = self.tableView.cellForRow(at: path) as? WalletTableViewCell {
                        cell.updateView ()
                    }
                }

            case .deleted:
                if let index = self.wallets.firstIndex (of: wallet) {
                    self.wallets.remove (at: index)

                    let path = IndexPath (row: index, section: 0)
                    self.tableView.deleteRows(at: [path], with: .automatic)
                }

            default:
                break
            }
        }
    }
    
    @IBOutlet var showManagersButton: UIButton!
}

