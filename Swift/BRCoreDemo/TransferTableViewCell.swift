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

    func colorForState() -> UIColor {
        guard let state = transfer?.state else { return UIColor.black }
        switch state {
        case .created: return UIColor.gray
        case .submitted: return UIColor.yellow
        case .included: return UIColor.green
        case .errored:  return UIColor.red
        case .cancelled: return UIColor.blue
        case .replaced: return UIColor.blue
        case .deleted: return UIColor.black
        }
    }
    func canonicalAmount (_ amount: EthereumAmount, sign: String, symbol: String) -> String {
        var result = amount.amount.trimmingCharacters(in: CharacterSet (charactersIn: "0 "))
        if result == "." || result == "" || result == "0." || result == ".0" {
            result = "0.0"
        }
        return sign + result + " " + symbol
    }
    
    func updateView () {
        if let transfer = transfer {
            let source = transfer.sourceAddress
//          let target = transfer.targetAddress
            let address = UIApplication.sharedClient.node.address
            let date = "... 2018 ..."
            let hash = transfer.hash
            dateLabel.text = "\(address == source ? "Send" : "Recv"): \(date)"
//            addrLabel.text = "Addr: \(address == source ? target : source)"

            addrLabel.text = "Hash: \(hash.hasPrefix("0x000") ? "<pending>" : hash)"
            amountLabel.text = canonicalAmount(transfer.amount, sign: (address == source ? "-" : "+"), symbol: transfer.amount.symbol);
            feeLabel.text = "Fee: \(canonicalAmount(transfer.fee, sign: "", symbol: "ETH"))"
            dotView.mainColor = colorForState()
            dotView.setNeedsDisplay()
        }
    }
    @IBOutlet var dateLabel: UILabel!
    @IBOutlet var amountLabel: UILabel!
    @IBOutlet var addrLabel: UILabel!
    @IBOutlet var feeLabel: UILabel!
    @IBOutlet var dotView: Dot!
}

