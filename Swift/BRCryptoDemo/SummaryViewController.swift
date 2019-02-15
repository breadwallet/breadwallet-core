//
//  SummaryViewController.swift
//  CoreDemo
//
//  Created by Ed Gamble on 7/26/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import UIKit
import BRCrypto

class SummaryViewController: UITableViewController, WalletListener {
    var wallets = [Wallet]()

    var detailViewController: WalletViewController? = nil

    override func viewDidLoad() {
        super.viewDidLoad()

        // TODO: Prefer here - but done earlier to avoid race on ETH Wallet Create
        // UIApplication.sharedClient.addWalletListener(listener: self)

        // Do any additional setup after loading the view, typically from a nib.

        if let split = splitViewController {
            let controllers = split.viewControllers
            detailViewController = (controllers[controllers.count-1] as! UINavigationController).topViewController as? WalletViewController
        }
    }

    override func viewWillAppear(_ animated: Bool) {
        clearsSelectionOnViewWillAppear = splitViewController!.isCollapsed
        self.wallets = UIApplication.sharedListener.wallets
        super.viewWillAppear(animated)
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    // MARK: - Segues

    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        if segue.identifier == "showWallet" {
            if let indexPath = tableView.indexPathForSelectedRow {
                let wallet = wallets[indexPath.row]
                let controller = (segue.destination as! UINavigationController).topViewController as! WalletViewController
                controller.wallet = wallet;
                controller.navigationItem.leftBarButtonItem = splitViewController?.displayModeButtonItem
                controller.navigationItem.leftItemsSupplementBackButton = true
            }
        }
    }

    @IBAction func doAct(_ sender: Any) {
        UIApplication.sync()
        let alert = UIAlertController (title: "Act",
                                       message: nil,
                                       preferredStyle: UIAlertController.Style.alert)

        alert.addAction (UIAlertAction (title: "Sync", style: UIAlertAction.Style.default) { (action) in
            UIApplication.sync()
            alert.dismiss(animated: true) {}
        })

        alert.addAction (UIAlertAction (title: "Sleep Eth", style: UIAlertAction.Style.default) { (action) in
            UIApplication.sleep()
            alert.dismiss(animated: true) {}
        })

        alert.addAction(UIAlertAction (title: "Cancel", style: UIAlertAction.Style.cancel))

        self.present (alert, animated: true) {}
    }
    
    // MARK: - Table View

    override func numberOfSections(in tableView: UITableView) -> Int {
        return 1
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return wallets.count
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "WalletCell", for: indexPath) as! WalletTableViewCell
        let wallet = wallets[indexPath.row]
        cell.wallet = wallet
        return cell
    }

    override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
        // Return false if you do not want the specified item to be editable.
        return true
    }

    override func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
        if editingStyle == .delete {
            wallets.remove(at: indexPath.row)
            tableView.deleteRows(at: [indexPath], with: .fade)
        } else if editingStyle == .insert {
            // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view.
        }
    }

    func announceWalletEvent(manager: WalletManager, wallet: Wallet, event: WalletEvent) {
        switch event {
        case .created:
            guard !wallets.contains(where: { $0 === wallet}) else { return }
            DispatchQueue.main.async {
                self.wallets.append(wallet)
                let path = IndexPath (row: (self.wallets.count - 1), section: 0)
                self.tableView.insertRows (at: [path], with: .automatic)
            }
        case .balanceUpdated:
            guard wallets.contains(where: { $0 === wallet }) else { return }
            DispatchQueue.main.async {
                let index = self.wallets.index(where: { $0 === wallet})!
                let path = IndexPath (row: index, section: 0)
                let cell = self.tableView.cellForRow(at: path) as! WalletTableViewCell
                cell.updateView()
            }
        case .deleted:
            guard wallets.contains(where: { $0 === wallet} ) else { return }
            DispatchQueue.main.async {
                let index = self.wallets.index(where: {$0 === wallet} )!
                self.wallets.remove(at: index)

                let path = IndexPath (row: index, section: 0)
                self.tableView.deleteRows(at: [path], with: .automatic)
            }
        default:
            break
        }
    }
}

