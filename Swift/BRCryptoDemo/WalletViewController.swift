//
//  DetailViewController.swift
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

class WalletViewController: UITableViewController, TransferListener, WalletManagerListener {

    /// The wallet viewed.
    var wallet : Wallet! {
        didSet {
//            UIApplication.sharedClient.addTransferListener(wallet: wallet, listener: self)
        }
    }

    /// The wallet's transfers
    var transfers : [Transfer] = []

    func reset () {
        DispatchQueue.main.async {
            self.wallet = nil
            self.transfers = []
            self.tableView.reloadData()
        }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        self.tableView.rowHeight = 100
    }

    override func viewWillAppear(_ animated: Bool) {
        // If we have a wallet, then view transfers
        if wallet != nil {
            self.transfers = wallet.transfers.sorted { $0.isBefore ($1) }
            self.navigationItem.title = "Wallet: \(wallet.name)"
            self.tableView.reloadData()
        }
        
        if let listener = UIApplication.sharedSystem.listener as? CoreDemoListener {
            listener.add (managerListener: self)
            listener.add (transferListener: self)
        }
        
        super.viewWillAppear(animated)
    }

    override func viewWillDisappear(_ animated: Bool) {
        if let listener = UIApplication.sharedSystem.listener as? CoreDemoListener {
            listener.remove (managerListener: self)
            listener.remove (transferListener: self)
        }
        
        super.viewWillDisappear(animated)
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
            
//        case "createTransfer":
//            print ("APP: WVC: Want to Create")
//            let controller = (segue.destination as! UINavigationController).topViewController as! TransferCreateController
//            controller.wallet = wallet
//            controller.fee    = wallet.manager.defaultNetworkFee
//            break

        default:
            break;
        }
    }

    func showCreateTransferController (named: String) {
        if let navigationController = self.storyboard?.instantiateViewController(withIdentifier: named) as? UINavigationController,
            let controller = navigationController.topViewController as? TransferCreateController {
            controller.wallet = self.wallet
            controller.fee    = self.wallet.manager.defaultNetworkFee
            self.present (navigationController, animated: true, completion: nil)
        }
    }

    func addAlertAction (alert: UIAlertController, _ canDisable: Bool, _ action: UIAlertAction) {
        if canDisable {
            action.isEnabled = (NetworkType.btc == wallet.manager.network.type)
        }
        alert.addAction(action);
    }

    @IBAction func createTransfer (_ sender: UIBarButtonItem) {
        let alert = UIAlertController (title: "Create Transfer",
                                       message: nil,
                                       preferredStyle: UIAlertController.Style.actionSheet)

        addAlertAction(alert: alert, false, UIAlertAction (title: "Send", style: UIAlertAction.Style.default) { (action) in
            print ("APP: WVC: Want to Send")
            self.showCreateTransferController(named: "createTransferSendNC")
            alert.dismiss(animated: true) {}
        })

        addAlertAction(alert: alert, false, UIAlertAction (title: "Receive", style: UIAlertAction.Style.default) { (action) in
            print ("APP: WVC: Want to Receive")
            self.showCreateTransferController(named: "createTransferRecvNC")
           alert.dismiss(animated: true) {}
        })

        addAlertAction(alert: alert, true, UIAlertAction (title: "Payment", style: UIAlertAction.Style.default) { (action) in
            print ("APP: WVC: Want to Pay")
            self.showCreateTransferController(named: "createTransferPayNC")
           alert.dismiss(animated: true) {}
        })

        addAlertAction(alert: alert, true, UIAlertAction (title: "Sweep", style: UIAlertAction.Style.default) { (action) in
            print ("APP: WVC: Want to Sweep")
            self.showCreateTransferController(named: "createTransferSweepNC")
           alert.dismiss(animated: true) {}
        })

        alert.addAction (UIAlertAction (title: "Cancel", style: UIAlertAction.Style.cancel))

        self.present (alert, animated: true) {}
    }

    override func tableView (_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "TransferCell", for: indexPath) as! TransferTableViewCell

        cell.transfer = transfers[indexPath.row]
        cell.updateView()

        return cell
     }
    
    func handleManagerEvent (system: System, manager: WalletManager, event: WalletManagerEvent) {
        DispatchQueue.main.async {
            print ("APP: WVC: ManagerEvent: \(event)")
            guard self.wallet.manager == manager /* && view is visible */  else { return }
            switch event {
            case .blockUpdated:
                self.tableView.reloadData()
            default:
                break
            }
        }
    }

    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
        DispatchQueue.main.async {
            print ("APP: WVC: TransferEvent: \(event)")
            guard self.wallet == wallet /* && view is visible */  else { return }
            switch event {
            case .created:
                precondition(!self.transfers.contains(transfer))

                // Simple, but inefficient
                self.transfers.insert(transfer, at: 0)
                self.transfers.sort { $0.isBefore ($1) }
                guard let index = self.transfers.firstIndex (of: transfer) else {
                    preconditionFailure()
                }

                let path = IndexPath (row: index, section: 0)
                self.tableView.insertRows (at: [path], with: .automatic)

            case .changed: //  (let old, let new)
                if let index = self.transfers.firstIndex (of: transfer) {
                    let path = IndexPath (row: index, section: 0)
                    self.tableView.reloadRows(at: [path], with: .automatic)
                }

            case .deleted:
                if let index = self.transfers.firstIndex (of: transfer) {
                    self.transfers.remove(at: index)

                    let path = IndexPath (row: index, section: 0)
                    self.tableView.deleteRows(at: [path], with: .automatic)
                }
            }
        }
    }
}

extension TransferConfirmation: Comparable {
    public static func < (lhs: TransferConfirmation, rhs: TransferConfirmation) -> Bool {
        return lhs.blockNumber < rhs.blockNumber
            || (lhs.blockNumber == rhs.blockNumber && lhs.transactionIndex < rhs.transactionIndex)
    }

    public static func == (lhs: TransferConfirmation, rhs: TransferConfirmation) -> Bool {
        return lhs.blockNumber == rhs.blockNumber
            && lhs.transactionIndex == rhs.transactionIndex
            && lhs.timestamp == rhs.timestamp
    }
}

extension Transfer {
    func isBefore (_ that: Transfer) -> Bool  {
        switch (self.confirmation, that.confirmation) {
        case (.none, .none): return true
        case (.none, .some): return true
        case (.some, .none): return false
        case (let .some(sc), let .some(tc)):
            return sc >= tc
        }
    }
}
