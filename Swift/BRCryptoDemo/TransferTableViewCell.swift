//
//  TransferTableViewCell.swift
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
        case .included: return
            transfer!.confirmation!.success ? UIColor.green : UIColor.red
        case .deleted: return UIColor.black

        case .signed: return UIColor.yellow
        case .pending: return UIColor.yellow
        case .failed/* (let reason) */: return UIColor.red
        }
    }

    func updateView () {
        if let transfer = transfer {
            let date: Date? = transfer.confirmation.map {
                Date (timeIntervalSince1970: TimeInterval ($0.timestamp))
            }
            let hash = transfer.hash

            dateLabel.text = date.map { dateFormatter.string(from: $0) } ?? "<pending>"
            addrLabel.text = hash.map { $0.description } ?? "<pending>"
            amountLabel.text = transfer.amountDirected.description
            feeLabel.text = "Fee: \(transfer.fee)"
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

