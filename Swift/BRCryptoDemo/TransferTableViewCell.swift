//
//  TransferTableViewCell.swift
//  CoreDemo
//
//  Created by Ed Gamble on 8/9/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import UIKit
import BRCrypto

class TransferTableViewCell: UITableViewCell {

    var transfer : Transfer? {
        didSet {
            updateView()
        }
    }

    var dateFormatter : DateFormatter!

    override func awakeFromNib() {
        super.awakeFromNib()
        // Initialization code
        if nil == dateFormatter {
            dateFormatter = DateFormatter()
            dateFormatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        }
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
//       case .errored:  return UIColor.red
//       case .cancelled: return UIColor.blue
//       case .replaced: return UIColor.blue
        case .deleted: return UIColor.black

        case .signed: return UIColor.yellow
        case .pending: return UIColor.yellow
        case .failed/* (let reason) */: return UIColor.red
        }
    }

    func canonicalAmount (_ amount: Amount, sign: String) -> String {
        let amount = amount.coerce(unit: amount.currency.defaultUnit)

        var result = amount.double?.description.trimmingCharacters(in: CharacterSet (charactersIn: "0 ")) ?? ""
        if result == "." || result == "" || result == "0." || result == ".0" {
            result = "0.0"
        }
        return sign + result + " " + amount.unit.symbol
    }
    
    func updateView () {
        if let transfer = transfer {
            let date: Date? = (nil == transfer.confirmation ? nil
                : Date (timeIntervalSince1970: TimeInterval(transfer.confirmation!.timestamp)))
            let hash = transfer.hash

            dateLabel.text = date.map { dateFormatter.string(from: $0) } ?? "<pending>"
            addrLabel.text = "Hash: \(hash.map { $0.description } ?? "<pending>")"
            amountLabel.text = canonicalAmount(transfer.amount, sign: (transfer.isSent ? "-" : "+"))
            feeLabel.text = "Fee: \(canonicalAmount(transfer.fee, sign: ""))"
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

