//
//  TransferTableViewCell.swift
//  CoreDemo
//
//  Created by Ed Gamble on 8/9/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import UIKit
import BRCore

class TransferTableViewCell: UITableViewCell {

    var transfer : EthereumTransfer? {
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
        NSLog ("Want to update transfer cell")
        if let transfer = transfer {
            let source = transfer.sourceAddress
            let target = transfer.targetAddress
            let address = UIApplication.sharedClient.node.address
            let date = "... 2018 ..."
            dateLabel.text = "\(address == source ? "Send" : "Recv"): \(date)"
            addrLabel.text = "Addr: \(address == source ? target : source)"
            amountLabel.text = transfer.amount.amount.trimmingCharacters(in: CharacterSet (charactersIn: "0 "))
            if amountLabel.text == "." || amountLabel.text == "" || amountLabel.text == "0." || amountLabel.text == ".0" {
                amountLabel.text = "0.0"
            }
            amountLabel.text = (address == source ? "-" : "+") + amountLabel.text! + " " + transfer.amount.symbol

        }
    }
    @IBOutlet var dateLabel: UILabel!
    @IBOutlet var amountLabel: UILabel!
    @IBOutlet var addrLabel: UILabel!
}
