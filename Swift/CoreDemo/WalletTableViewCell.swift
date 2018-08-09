//
//  WalletTableViewCell.swift
//  CoreDemo
//
//  Created by Ed Gamble on 8/9/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import UIKit
import BRCore

class WalletTableViewCell: UITableViewCell {

    var wallet : EthereumWallet? {
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
        NSLog ("Want to update view")
        textLabel?.text = wallet?.name ?? "..."
        detailTextLabel?.text = wallet?.balance.amount ?? "??"
    }
}
