//
//  DetailViewController.swift
//  CoreDemo
//
//  Created by Ed Gamble on 7/26/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import UIKit
import BRCore

class WalletViewController: UITableViewController, TransferListener {
    var wallet : EthereumWallet! {
        didSet {
            UIApplication.sharedClient.addTransferListener(wallet: wallet, listener: self)
        }
    }
    
    var transfers : [EthereumTransfer] = []

    override func viewDidLoad() {
        super.viewDidLoad()
        self.tableView.rowHeight = 100


        // Uncomment the following line to preserve selection between presentations
        // self.clearsSelectionOnViewWillAppear = false

        // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
        // self.navigationItem.rightBarButtonItem = self.editButtonItem
    }

    override func viewWillAppear(_ animated: Bool) {
        // clearsSelectionOnViewWillAppear = splitViewController!.isCollapsed
        if wallet != nil { // here on UI changes, like rotation before initialization of sharedClient.
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

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return transfers.count
    }

    // MARK: - Segues

    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
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
            
        case "createTransfer":
            let controller = (segue.destination as! UINavigationController).topViewController as! TransferCreateController
            controller.wallet = wallet
            
        default:
            break;
        }
    }

     override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "TransferCell", for: indexPath) as! TransferTableViewCell
        let transfer = transfers[indexPath.row]

        cell.transfer = transfer
        cell.updateView()
        return cell
     }

    /*
     // Override to support conditional editing of the table view.
     override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
     // Return false if you do not want the specified item to be editable.
     return true
     }
     */

    /*
     // Override to support editing the table view.
     override func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCellEditingStyle, forRowAt indexPath: IndexPath) {
     if editingStyle == .delete {
     // Delete the row from the data source
     tableView.deleteRows(at: [indexPath], with: .fade)
     } else if editingStyle == .insert {
     // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
     }
     }
     */

    /*
     // Override to support rearranging the table view.
     override func tableView(_ tableView: UITableView, moveRowAt fromIndexPath: IndexPath, to: IndexPath) {

     }
     */

    /*
     // Override to support conditional rearranging of the table view.
     override func tableView(_ tableView: UITableView, canMoveRowAt indexPath: IndexPath) -> Bool {
     // Return false if you do not want the item to be re-orderable.
     return true
     }
     */

    /*
     // MARK: - Navigation

     // In a storyboard-based application, you will often want to do a little preparation before navigation
     override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
     // Get the new view controller using segue.destinationViewController.
     // Pass the selected object to the new view controller.
     }
     */

    func announceTransferEvent(ewm: EthereumWalletManager, wallet: EthereumWallet, transfer: EthereumTransfer, event: EthereumTransferEvent) {
        switch event {
        case .created:
//            assert (!transfers.contains(transfer))
            DispatchQueue.main.async {
                if !self.transfers.contains(transfer) {
                    self.transfers.append(transfer)
                    let path = IndexPath (row: (self.transfers.count - 1), section: 0)
                    self.tableView.insertRows (at: [path], with: .automatic)
                }
            }

        case .deleted:
            DispatchQueue.main.async {
                if let index = self.transfers.firstIndex(of: transfer) {
                    self.transfers.remove(at: index)
                    let path = IndexPath (row: index, section: 0)
                    self.tableView.deleteRows(at: [path], with: .automatic)
                }
            }

        default:
            DispatchQueue.main.async {
                self.tableView.reloadData()
//                if let index = self.transfers.firstIndex(of: transfer) {
//                    let path = IndexPath (row: index, section: 0)
//                    self.tableView.reloadRows(at: [path], with: .automatic)
//                }
            }
        }
    }
}
