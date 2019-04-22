//
//  DetailViewController.swift
//  CoreDemo
//
//  Created by Ed Gamble on 7/26/18.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import UIKit
import BRCrypto

class WalletViewController: UITableViewController, TransferListener {

    /// The wallet viewed.
    var wallet : Wallet! {
        didSet {
//            UIApplication.sharedClient.addTransferListener(wallet: wallet, listener: self)
        }
    }

    /// The wallet's transfers
    var transfers : [Transfer] = []


    override func viewDidLoad() {
        // Seems `viewDidLoad()` is called many times... and the listener is added many times.
        // Should only be added once or should be removed (on viewWillDisappear())
        if let listener = UIApplication.sharedSystem.listener as? CoreDemoListener {
            listener.transferListeners.append (self)
        }

        super.viewDidLoad()
        self.tableView.rowHeight = 100
    }

    override func viewWillAppear(_ animated: Bool) {
        // If we have a wallet, then view transfers
        if wallet != nil {
            self.transfers = wallet.transfers;
            self.navigationItem.title = "Wallet: \(wallet.name)"
            self.tableView.reloadData()
        }
        super.viewWillAppear(animated)
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }

    // MARK: - Table view data source

    override func numberOfSections(in tableView: UITableView) -> Int {
        return 1
    }

    override func tableView (_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return transfers.count
    }

    // MARK: - Segues

    override func prepare (for segue: UIStoryboardSegue, sender: Any?) {
        switch segue.identifier {
        case "showTransfer":
            if let indexPath = tableView.indexPathForSelectedRow {
                let transfer = transfers[indexPath.row]
                let controller = (segue.destination as! UINavigationController).topViewController as! TransferViewController

                controller.wallet = wallet
                controller.transfer = transfer
                controller.navigationItem.leftBarButtonItem = splitViewController?.displayModeButtonItem
                controller.navigationItem.leftItemsSupplementBackButton = true
            }
            break
            
        case "createTransfer":
            print ("APP: WVC: Want to Create")
//            let controller = (segue.destination as! UINavigationController).topViewController as! TransferCreateController
//            controller.wallet = wallet
            break

        default:
            break;
        }
    }

     override func tableView (_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "TransferCell", for: indexPath) as! TransferTableViewCell

        cell.transfer = transfers[indexPath.row]
        cell.updateView()

        return cell
     }


    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
        DispatchQueue.main.async {
            print ("APP: WVC: TransferEvent: \(event)")
            guard self.wallet === wallet /* && view is visible */  else { return }
            switch event {
            case .created:
                self.transfers.append(transfer)

                let path = IndexPath (row: (self.transfers.count - 1), section: 0)
                self.tableView.insertRows (at: [path], with: .automatic)

            case .changed: //  (let old, let new)
                if let index = self.transfers.firstIndex (where: { $0 === transfer}) {
                    let path = IndexPath (row: index, section: 0)
                    self.tableView.reloadRows(at: [path], with: .automatic)
                }

            case .deleted:
                if let index = self.transfers.firstIndex (where: { $0 === transfer}) {
                    self.transfers.remove(at: index)

                    let path = IndexPath (row: index, section: 0)
                    self.tableView.deleteRows(at: [path], with: .automatic)
                }
            }
        }
    }
}
