//
//  WalletManagersTableViewController.swift
//  BRCryptoDemo
//
//  Created by Ed Gamble on 8/22/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import UIKit
import BRCrypto

class WalletManagersTableViewController: UITableViewController {

    var managers: [WalletManager] = []

    override func viewDidLoad() {
        super.viewDidLoad()
    }

    override func numberOfSections(in tableView: UITableView) -> Int {
        return 1
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return managers.count
    }

    override func tableView (_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "WalletManagerCell", for: indexPath)
        let manager = managers[indexPath.row]

        cell.textLabel?.text = manager.name
        return cell
    }

    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        switch segue.identifier {
        case "showManager":
            if let indexPath = tableView.indexPathForSelectedRow {
                let manager = managers[indexPath.row]
                let controller = (segue.destination as! UINavigationController).topViewController as! WalletManagerViewController

                controller.manager = manager
            }

        default:
            break;
        }
    }

}
