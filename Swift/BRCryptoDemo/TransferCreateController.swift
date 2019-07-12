//
//  TransferCreateController.swift
//  CoreDemo
//
//  Created by Ed Gamble on 8/22/18.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import UIKit
import BRCrypto

class TransferCreateController: UIViewController, UITextViewDelegate {

    var wallet : Wallet!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view.
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    var isEthCurrency: Bool {
        return wallet.currency.code.lowercased() == Currency.codeAsETH
    }
    var isBitCurrency: Bool {
        return wallet.currency.code.lowercased() == Currency.codeAsBTC ||
            wallet.currency.code.lowercased() == Currency.codeAsBCH
    }

    var isTokCurrency: Bool {
        return !isEthCurrency && !isBitCurrency
    }

    var canUseFeeBasis: Bool = false

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated);

        let balance: Double = wallet.balance.double (as: wallet.unit) ?? 0.0

        oneEtherButton.setEnabled (isEthCurrency, forSegmentAt: 0)
        oneEtherButton.setEnabled (isEthCurrency || isTokCurrency || isBitCurrency, forSegmentAt: 1)
        oneEtherButton.setEnabled (isBitCurrency && balance >= 0.01, forSegmentAt: 2)
        oneEtherButton.selectedSegmentIndex = 1 // slider

        oneEtherSelected = 0 == oneEtherButton.selectedSegmentIndex
        oneBitcoinSelected = 2 == oneEtherButton.selectedSegmentIndex

        amountSlider.minimumValue = 0.0
        amountSlider.maximumValue = Float (balance)
        amountSlider.value = 0.0

        if nil != UIPasteboard.general.string {
            recvField.text = UIPasteboard.general.string
        }
        else {
            switch wallet.currency.code.lowercased() {
            case Currency.codeAsETH:
                recvField.text = (wallet.manager.network.isMainnet
                    ? "0x19454a70538bfbdbd7abf3ac8d274d5cb2514056" /* "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62" */
                    : "0xbDFdAd139440D2Db9BA2aa3B7081C2dE39291508");
            case Currency.codeAsBTC:
                recvField.text = (wallet.manager.network.isMainnet
                    ? ""
                    : "mv4rnyY3Su5gjcDNzbMLKBQkBicCtHUtFB")
            case Currency.codeAsBCH:
                recvField.text = (wallet.manager.network.isMainnet
                    ? ""
                    : "mv4rnyY3Su5gjcDNzbMLKBQkBicCtHUtFB")
            default:
                recvField.text = "Missed currency/network"
            }
        }

        walletAddrButton.setTitle(wallet.source.description, for: .normal)

        gasPriceSegmentedController.isEnabled = isEthCurrency && canUseFeeBasis
        gasLimitSegmentedController.isEnabled = isEthCurrency && canUseFeeBasis
        satPerKBSegmentedController.isEnabled = isBitCurrency && canUseFeeBasis
        updateView()
    }

    var oneEtherSelected = false
    var oneBitcoinSelected = false

    /*
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        // Get the new view controller using segue.destinationViewController.
        // Pass the selected object to the new view controller.
    }
    */
    @IBAction func submit(_ sender: UIBarButtonItem) {
        print ("APP: TCC: Want to submit")
        let value = amount()

        let alert = UIAlertController (title: "Submit Transaction for \(value) \(wallet.name)",
            message: "Are you sure?",
            preferredStyle: UIAlertController.Style.actionSheet)

        alert.addAction(UIAlertAction (title: "Yes", style: UIAlertAction.Style.destructive) { (action) in
            guard let target = Address.create (string: self.recvField.text!, network: self.wallet.manager.network)
                else {
                    let alert = UIAlertController (title: "Submit Transfer",
                                                   message: "Failed to create transfer - invalid target address",
                                                   preferredStyle: UIAlertController.Style.alert)
                    alert.addAction(UIAlertAction (title: "Okay", style: UIAlertAction.Style.cancel) { (action) in
                        self.dismiss(animated: true) {}
                    })

                    self.present (alert, animated: true) {}
                    return
            }

            let unit = self.wallet.unit
            let amount = Amount.create (double: Double(value), unit: unit)
            print ("APP: TVV: Submit \(self.isBitCurrency ? "BTC/BCH" : "ETH") Amount: \(amount)");

            // let amount = Amount (value: value, unit: self.wallet.currency.defaultUnit)
            guard let transfer = self.wallet.createTransfer (target: target,
                                                             amount: amount,
                                                             feeBasis: self.feeBasis())
                else {
                    let alert = UIAlertController (title: "Submit Transfer",
                                               message: "Failed to create transfer - balance too low?",
                                               preferredStyle: UIAlertController.Style.alert)
                    alert.addAction(UIAlertAction (title: "Okay", style: UIAlertAction.Style.cancel) { (action) in
                        self.dismiss(animated: true) {}
                    })

                    self.present (alert, animated: true) {}
                    return
            }

            // Will generate a WalletEvent.transferSubmitted (transfer, success)
            self.wallet.manager.submit (transfer: transfer,
                                        paperKey: UIApplication.paperKey);

            // Notify, close
            self.dismiss(animated: true) {}
        })
        alert.addAction(UIAlertAction (title: "No", style: UIAlertAction.Style.cancel) { (action) in
            print ("APP: TCC: Will Cancel" )
        })
        self.present(alert, animated: true) {}
    }

    @IBAction func cancel(_ sender: UIBarButtonItem) {
        self.dismiss(animated: true) {}
    }

    func updateView () {

        let fee = wallet.estimateFee (amount: wallet.balance, feeBasis: nil);
        feeLabel.text = "  \(fee) (estimated)"

        amountMinLabel.text = amountSlider.minimumValue.description
        amountMaxLabel.text = amountSlider.maximumValue.description
        amountLabel.text = amount().description
        amountSlider.isEnabled = !oneEtherSelected && !oneBitcoinSelected

        submitButton.isEnabled = (recvField.text != "" &&
            (0.0 != amountSlider.value || oneEtherSelected || oneBitcoinSelected))
    }

    @IBAction func amountChanged(_ sender: Any) {
        amountLabel.text = amountSlider.value.description
        submitButton.isEnabled = (recvField.text != "" &&
            (0.0 != amountSlider.value || oneEtherSelected || oneBitcoinSelected))
    }

    func feeBasis () -> TransferFeeBasis {
        return wallet.defaultFeeBasis
//        switch wallet.currency.code {
//        case Currency.codeAsETH:
//            let wei = wallet.manager.network.baseUnitFor(currency: wallet.currency)!
//            return TransferFeeBasis.ethereum (
//                gasPrice: Amount.create(integer: Int64 (gasPrice()), unit: wei),
//                gasLimit: gasLimit())
//        default:
//            return TransferFeeBasis.bitcoin(feePerKB: 1000)
//        }
    }

    func amount () -> Float {
        return (oneBitcoinSelected
                ? 0.01
                : (oneEtherSelected
                    ? 1
                    : amountSlider!.value))
    }

    // In WEI
    func gasPrice () -> UInt64 {
        switch (gasPriceSegmentedController.selectedSegmentIndex) {
        case 0: return   15 * 1000000000 // 15    GWEI
        case 1: return    5 * 1000000000 //  5    GWEI
        case 2: return 1001 *    1000000 // 1.001 GWEI
        default: return 5
        }
    }

    @IBAction func amountOneEther(_ sender: UISegmentedControl) {
        oneEtherSelected = 0 == oneEtherButton.selectedSegmentIndex
        oneBitcoinSelected = 2 == oneEtherButton.selectedSegmentIndex
        updateView()
    }

    @IBAction func gasPriceChanged(_ sender: UISegmentedControl) {
        updateView()
    }

    func gasLimit () -> UInt64 {
        switch (gasLimitSegmentedController.selectedSegmentIndex) {
        case 0: return 92000
        case 1: return 21000
        case 2: return  1000
        default: return 21000
        }
    }

    func satPerKB () -> UInt64 {
        switch (satPerKBSegmentedController.selectedSegmentIndex) {
        case 0: return 5000
        case 1: return 2000
        case 2: return 1000
        default: return 2000
        }
    }

    @IBAction func gasLimitChanged(_ sender: UISegmentedControl) {
        updateView()
    }

    @IBOutlet var submitButton: UIBarButtonItem!
    @IBOutlet var feeLabel: UILabel!
    @IBOutlet var amountSlider: UISlider!
    @IBOutlet var recvField: UITextField!
    @IBOutlet var amountMinLabel: UILabel!
    @IBOutlet var amountMaxLabel: UILabel!
    @IBOutlet var amountLabel: UILabel!
    @IBOutlet var gasPriceSegmentedController: UISegmentedControl!
    @IBOutlet var gasLimitSegmentedController: UISegmentedControl!
    @IBOutlet var oneEtherButton: UISegmentedControl!
    @IBOutlet var satPerKBSegmentedController: UISegmentedControl!
    @IBOutlet var walletAddrButton: UIButton!
    @IBAction func toPasteBoard(_ sender: UIButton) {
        UIPasteboard.general.string = sender.titleLabel?.text
    }
}
