//
//  TransferCreateController.swift
//  CoreDemo
//
//  Created by Ed Gamble on 8/22/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import UIKit
import BRCore

class TransferCreateController: UIViewController, UITextViewDelegate {

    var wallet : EthereumWallet!
    
    override func viewDidLoad() {
        super.viewDidLoad()

        // Do any additional setup after loading the view.
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated);
        amountSlider.minimumValue = 0.0
        amountSlider.maximumValue = Float (wallet.balance.amount)!
        amountSlider.value = 0.0
        recvField.text = (UIApplication.sharedClient.network == EthereumNetwork.mainnet
            ? "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62"
            : "0xbDFdAd139440D2Db9BA2aa3B7081C2dE39291508");
        updateView()
    }

    /*
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        // Get the new view controller using segue.destinationViewController.
        // Pass the selected object to the new view controller.
    }
    */
    @IBAction func submit(_ sender: UIBarButtonItem) {
        NSLog ("Want to submit")

        let alert = UIAlertController (title: "Submit Transaction", message: "Are you sure?", preferredStyle: UIAlertControllerStyle.actionSheet)
        alert.addAction(UIAlertAction (title: "Yes", style: UIAlertActionStyle.destructive) { (action) in
            let transfer = self.wallet.createTransfer (recvAddress: self.recvField.text!,
                                                             amount: self.amountSlider.value.description,
                                                             unit: EthereumAmountUnit.defaultUnitEther);
            self.wallet.sign(transfer: transfer,
                             paperKey: "boring head harsh green empty clip fatal typical found crane dinner timber");

            self.wallet.submit(transfer: transfer);
            // Notify, close
            self.dismiss(animated: true) {}
        })
        alert.addAction(UIAlertAction (title: "No", style: UIAlertActionStyle.cancel) { (action) in
            NSLog ("Will Cancel" )
        })
        self.present(alert, animated: true) {}
    }

    @IBAction func cancel(_ sender: UIBarButtonItem) {
        self.dismiss(animated: true) {}
    }

    func canonicalAmount (_ amount: EthereumAmount, sign: String, symbol: String) -> String {
        var result = amount.amount.trimmingCharacters(in: CharacterSet (charactersIn: "0 "))
        if result == "." || result == "" || result == "0." || result == ".0" {
            result = "0.0"
        }
        return sign + result + " " + symbol
    }

    func updateView () {
        let amount = amountSlider.value
        feeLabel.text = canonicalAmount (wallet.estimateFee(amount: amount.description, unit: wallet.unit),
                                         sign: "",
                                         symbol: "ETH")

        amountMinLabel.text = amountSlider.minimumValue.description
        amountMaxLabel.text = amountSlider.maximumValue.description
        amountLabel.text = amountSlider.value.description

        submitButton.isEnabled = (0.0 != amountSlider.value && recvField.text != "")
    }

    @IBAction func amountChanged(_ sender: Any) {
        amountLabel.text = amountSlider.value.description
        submitButton.isEnabled = (0.0 != amountSlider.value && recvField.text != "")
    }
    
    @IBOutlet var submitButton: UIBarButtonItem!
    @IBOutlet var feeLabel: UILabel!
    @IBOutlet var amountSlider: UISlider!
    @IBOutlet var recvField: UITextField!
    @IBOutlet var amountMinLabel: UILabel!
    @IBOutlet var amountMaxLabel: UILabel!
    @IBOutlet var amountLabel: UILabel!
}
