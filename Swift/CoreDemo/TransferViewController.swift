//
//  TransferViewController.swift
//  CoreDemo
//
//  Created by Ed Gamble on 8/22/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import UIKit
import BRCore

class TransferViewController: UIViewController {

    var transfer : EthereumTransfer!

    override func viewDidLoad() {
        super.viewDidLoad()

        // Do any additional setup after loading the view.
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated);
        updateView ()
    }
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    func canonicalAmount (_ amount: EthereumAmount, sign: String, symbol: String) -> String {
        var result = amount.amount.trimmingCharacters(in: CharacterSet (charactersIn: "0 "))
        if result == "." || result == "" || result == "0." || result == ".0" {
            result = "0.0"
        }
        return sign + result + " " + symbol
    }

    func updateView () {
        let address = UIApplication.sharedClient.node.address

        amountLabel.text = canonicalAmount(transfer.amount, sign: (address == transfer.sourceAddress ? "-" : "+"), symbol: transfer.amount.symbol);
        feeLabel.text = canonicalAmount(transfer.fee, sign: "", symbol: "ETH")
        dateLabel.text = "TBD"
        sendLabel.text = transfer.sourceAddress
        recvLabel.text = transfer.targetAddress
        identifierLabel.text = transfer.hash
        confLabel.text = transfer.confirmationBlockNumber.map { "Yes @ \($0.description)" } ?? "No"
    }

    /*
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        // Get the new view controller using segue.destinationViewController.
        // Pass the selected object to the new view controller.
    }
    */

    @IBOutlet var amountLabel: UILabel!
    @IBOutlet var feeLabel: UILabel!
    @IBOutlet var dateLabel: UILabel!
    @IBOutlet var sendLabel: UILabel!
    @IBOutlet var recvLabel: UILabel!
    @IBOutlet var identifierLabel: UILabel!
    @IBOutlet var confLabel: UILabel!
}
