//
//  WalletTableViewCell.swift
//  CoreDemo
//
//  Created by Ed Gamble on 8/9/18.
//  Copyright © 2018 breadwallet. All rights reserved.
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

        textLabel?.text = "\(wallet.name) (\(wallet.manager.network))"

        // balance into defaultUnit
        let balance = wallet.balance.double

        detailTextLabel?.text = balance?.description ?? "??"

        var value : String = balance?.description.trimmingCharacters(in: CharacterSet (charactersIn: "0 ")) ?? ""
        if value == "." || value == "" || value == "0." || value == ".0" {
            value = "0.0"
        }
        detailTextLabel?.text = value + " " + wallet.currency.defaultUnit.symbol
    }
}
