//
//  WalletTableViewCell.swift
//  CoreDemo
//
//  Created by Ed Gamble on 8/9/18.
//  Copyright © 2018-2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import UIKit
import BRCrypto

class WalletTableViewCell: UITableViewCell {

    var wallet : Wallet? {
        didSet {
            updateView()
        }
    }

    override func awakeFromNib() {
        super.awakeFromNib()
        // Initialization code
    }

    override func setSelected(_ selected: Bool, animated: Bool) {
        super.setSelected(selected, animated: animated)

        // Configure the view for the selected state
    }

    func updateView () {
        guard let wallet = wallet else { return }
        let balance = wallet.balance

        textLabel?.text = "\(wallet.name) (\(wallet.manager.network))"
        detailTextLabel?.text = balance.string(as: balance.unit) ?? "---"
    }
}
