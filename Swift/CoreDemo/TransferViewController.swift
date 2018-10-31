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
        if let errorReason = transfer.stateErrorReason {
            stateLabel.text = "\(transfer.state.description): \(errorReason)"
        }
        else { stateLabel.text = transfer.state.description }

        switch transfer.state {
        case .errored:
            cancelButton.isEnabled = true
            resubmitButton.isEnabled = true
        default:
            cancelButton.isEnabled = false
            resubmitButton.isEnabled = false
       }

        nonceLabel.text = transfer.nonce.description
    }

    @IBAction func doResubmit(_ sender: UIButton) {
        NSLog ("Want to Resubmit")
    }

    /*
     * Canceling means generating a 0 ETH transaction to Your Own Address with the purpose of
     * preventing a previous transaction from "going through" / "being mined" / "being included in
     * the blockchain" / "being stuck"
     */
    @IBAction func doCancel(_ sender: UIButton) {
        NSLog ("Want to Cancel")
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
    @IBOutlet var sendLabel: CopyableLabel!
    @IBOutlet var recvLabel: CopyableLabel!
    @IBOutlet var identifierLabel: UILabel!
    @IBOutlet var confLabel: UILabel!
    @IBOutlet var stateLabel: UILabel!
    @IBOutlet var cancelButton: UIButton!
    @IBOutlet var resubmitButton: UIButton!
    @IBOutlet var nonceLabel: UILabel!
}

//
//
//
class CopyableLabel: UILabel {

    override init(frame: CGRect) {
        super.init(frame: frame)
        self.sharedInit()
    }

    required init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)
        self.sharedInit()
    }

    func sharedInit() {
        self.isUserInteractionEnabled = true
        self.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(self.showMenu)))
    }

    @objc func showMenu(sender: AnyObject?) {
        self.becomeFirstResponder()

        let menu = UIMenuController.shared

        if !menu.isMenuVisible {
            menu.setTargetRect(bounds, in: self)
            menu.setMenuVisible(true, animated: true)
        }
    }

    override func copy(_ sender: Any?) {
        let board = UIPasteboard.general

        board.string = text

        let menu = UIMenuController.shared

        menu.setMenuVisible(false, animated: true)
    }

    override var canBecomeFirstResponder: Bool {
        return true
    }

    override func canPerformAction(_ action: Selector, withSender sender: Any?) -> Bool {
        return action == #selector(UIResponderStandardEditActions.copy)
    }
}
