//
//  TransferCreateController.swift
//  CoreDemo
//
//  Created by Ed Gamble on 8/6/19.
//  Copyright © 2018-2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import UIKit
import BRCrypto

class TransferCreateSendController: TransferCreateController,
UITextViewDelegate, UIPickerViewDelegate, UIPickerViewDataSource {

    var fees: [NetworkFee] = []
    var target: Address?
    var minimum: Amount!
    var maximum: Amount!
    
     override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view.
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    var isEthCurrency: Bool {
        return NetworkType.eth == wallet.manager.network.type
    }
    var isBitCurrency: Bool {
        switch wallet.manager.network.type {
        case .btc, .bch: return true
        default: return false
        }
    }

    var isTokCurrency: Bool {
        return !isEthCurrency && !isBitCurrency
    }

    //
    // Can't select gasPrice, gasLimit nor SAT/KB because we can't create a TransferFeeBasis
    // from them - no exposed interface.
    //
    let canUseFeeBasis: Bool = true
    var disableFeeEstimate: Bool = false

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated);

        self.fees = wallet.manager.network.fees.sorted { $0.timeIntervalInMilliseconds < $1.timeIntervalInMilliseconds }
        self.fee = fees[0]

        self.minimum = Amount.create (double: 0.0, unit: wallet.unit)
        self.maximum = wallet.balance;

        if nil == recvField.text || "" == recvField.text! {
            recvField.text = UIPasteboard.general.string
        }
        target = Address.create (string: self.recvField.text!, network: self.wallet.manager.network)

        gasPriceSegmentedController.isEnabled = isEthCurrency && canUseFeeBasis
        gasLimitSegmentedController.isEnabled = isEthCurrency && canUseFeeBasis
        satPerKBSegmentedController.isEnabled = isBitCurrency && canUseFeeBasis

        priorityPicker.dataSource = self
        priorityPicker.delegate = self

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
    private func submitTransferFailed (_ message: String) {
        let alert = UIAlertController (title: "Submit Transfer",
                                       message: "Failed to submit transfer - \(message)",
                                       preferredStyle: UIAlertController.Style.alert)
        alert.addAction(UIAlertAction (title: "Okay", style: UIAlertAction.Style.cancel) { (action) in
            self.dismiss(animated: true) {}
        })

        self.present (alert, animated: true) {}
    }

    @IBAction func submit(_ sender: UIBarButtonItem) {
        print ("APP: TCC: Want to submit")
        let value = amount()

        let alert = UIAlertController (title: "Submit Transaction for \(value) \(wallet.name)",
            message: "Are you sure?",
            preferredStyle: UIAlertController.Style.actionSheet)

        alert.addAction(UIAlertAction (title: "Yes", style: UIAlertAction.Style.destructive) { (action) in
            guard let target = Address.create (string: self.recvField.text!, network: self.wallet.manager.network)
                else { self.submitTransferFailed("invalid target address"); return }

            let unit = self.wallet.unit
            let amount = Amount.create (double: Double(value), unit: unit)
            print ("APP: TVV: Submit \(self.isBitCurrency ? "BTC/BCH" : "ETH") Amount: \(amount)");

            guard let transferFeeBasis = self.feeBasis
                else { self.submitTransferFailed ("no fee basis"); return }

            var attributes: Set<TransferAttribute> = Set()

            if let destinationTagAttribute = self.wallet.transferAttributesFor (target: target)
                .first (where: { "DestinationTag" == $0.key }) {
                destinationTagAttribute.value = (self.recvField.text! == "rw2ciyaNshpHe7bCHo4bRWq6pqqynnWKQg"
                    ? "739376465"
                    : nil)
                attributes.insert (destinationTagAttribute)
            }

            guard let transfer = self.wallet.createTransfer (target: target,
                                                             amount: amount,
                                                             estimatedFeeBasis: transferFeeBasis,
                                                             attributes: attributes)
                else { self.submitTransferFailed("balance too low?"); return }

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

    func updateLimits () {
        guard let target = target else { return }

        wallet.estimateLimitMinimum (target: target, fee: fee) {
            (res: Result<Amount, Wallet.LimitEstimationError>) in
            switch res {
            case .success (let minimum):
                DispatchQueue.main.async {
                    self.minimum = minimum

                    self.amountSlider.minimumValue = Float (minimum.double (as: self.wallet.unitForFee) ?? 0.0)
                    self.amountMinLabel.text = self.minimum.string(as: self.wallet.unit)

                    self.amountSlider.value = max (self.amountSlider.minimumValue, self.amountSlider.value)
                    self.amountLabel.text = self.amount().description

                }

            case .failure:
                break
            }
        }

        wallet.estimateLimitMaximum (target: target, fee: fee) {
            (res: Result<Amount, Wallet.LimitEstimationError>) in
            switch res {
            case .success (let maximum):
                DispatchQueue.main.async {
                    self.maximum = maximum

                    self.amountSlider.maximumValue = Float (maximum.double (as: self.wallet.unitForFee) ?? 0.0)
                    self.amountMaxLabel.text = self.maximum.string(as: self.wallet.unit)

                    self.amountSlider.value = min (self.amountSlider.maximumValue, self.amountSlider.value)
                    self.amountLabel.text = self.amount().description
                }

            case .failure:
                break
            }
        }

    }

    func updateFee () {
        if self.disableFeeEstimate {
            precondition(nil != self.feeBasis)
            return
        }

        guard let target = Address.create (string: self.recvField.text!, network: self.wallet.manager.network)
            else { return }

        let amount = Amount.create (double: Double(self.amount()), unit: wallet.unit)

        wallet.estimateFee (target: target, amount: amount, fee: fee) {
            (result: Result<TransferFeeBasis, Wallet.FeeEstimationError>) in
            guard case let .success(feeBasis) = result
                else { return }

            self.feeBasis = feeBasis
            DispatchQueue.main.async {
                self.feeLabel.text = "  \(feeBasis.fee) (estimated)"
            }
        }
    }
    
    func updateView () {

        self.amountSlider.value = min (self.amountSlider.maximumValue, self.amountSlider.value)
        self.amountSlider.value = max (self.amountSlider.minimumValue, self.amountSlider.value)

        amountLabel.text = amount().description

        updateLimits()

        submitButton.isEnabled = (recvField.text != "" &&
            (0.0 != amountSlider.value))

        updateFee()
    }

    @IBAction func amountChanged(_ sender: Any) {
        amountLabel.text = amountSlider.value.description
        submitButton.isEnabled = (recvField.text != "" &&
            (0.0 != amountSlider.value))
        updateFee()
    }

    func amount () -> Float {
        return amountSlider!.value
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

    func gasLimit () -> UInt64 {
        switch (gasLimitSegmentedController.selectedSegmentIndex) {
        case 0: return 92000
        case 1: return 21000
        case 2: return  1000
        default: return 21000
        }
    }

    func updateFeeBasisETH () {
        guard let pricePerCostFactorUnit = wallet.manager.network.baseUnitFor (currency: wallet.unitForFee.currency)
            else { return }

        let pricePerCostFactor = Amount.create(integer: Int64(gasPrice()), unit: pricePerCostFactorUnit)
        let costFactor = Double (gasLimit())

        if let feeBasis = wallet.createTransferFeeBasis(pricePerCostFactor: pricePerCostFactor, costFactor: costFactor) {
            self.feeBasis = feeBasis
            self.disableFeeEstimate = true
            DispatchQueue.main.async {
                self.feeLabel.text = "  \(feeBasis.fee)"
            }
        }
    }

    @IBAction func gasPriceChanged(_ sender: UISegmentedControl) {
        updateFeeBasisETH()
     }

    @IBAction func gasLimitChanged(_ sender: UISegmentedControl) {
        updateFeeBasisETH()
    }

    func satPerKB () -> UInt64 {
        switch (satPerKBSegmentedController.selectedSegmentIndex) {
        case 0: return 5000
        case 1: return 2000
        case 2: return 1000
        default: return 2000
        }
    }

    @IBAction func satPerKBChanged(_ sender: UISegmentedControl) {
        guard let pricePerCostFactorUnit = wallet.manager.network.baseUnitFor (currency: wallet.unitForFee.currency)
            else { return }
        
        let pricePerCostFactor = Amount.create(integer: Int64(satPerKB()), unit: pricePerCostFactorUnit)
        let costFactor = Double (1) // sizeInKB
        
        if let feeBasis = wallet.createTransferFeeBasis(pricePerCostFactor: pricePerCostFactor, costFactor: costFactor) {
            self.feeBasis = feeBasis
            self.disableFeeEstimate = true
            DispatchQueue.main.async {
                self.feeLabel.text = "  \(feeBasis.fee)"
            }
        }
    }

    @IBAction func doUseCoinbase(_ sender: Any) {
        recvField.text = "rw2ciyaNshpHe7bCHo4bRWq6pqqynnWKQg";
        updateView()
    }
    // Network Fee Picker

    func numberOfComponents(in pickerView: UIPickerView) -> Int {
        return 1
    }

    func pickerView(_ pickerView: UIPickerView, numberOfRowsInComponent component: Int) -> Int {
        return 0 == component ? fees.count : 0
    }

    func pickerView(_ pickerView: UIPickerView,
                    titleForRow row: Int,
                    forComponent component: Int) -> String? {
        guard component == 0 && row < fees.count else { return nil }
        let timeInMillis = fees[row].timeIntervalInMilliseconds
        return "\(Double(timeInMillis)/1000.0) seconds"
    }

    func pickerView(_ pickerView: UIPickerView,
                    didSelectRow row: Int,
                    inComponent component: Int) {
        guard component == 0 && row < fees.count else { return }
        self.fee = fees[row]

        // Update min/max and fee
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
    @IBOutlet var priorityPicker: UIPickerView!
    @IBOutlet var satPerKBSegmentedController: UISegmentedControl!
    @IBAction func toPasteBoard(_ sender: UIButton) {
        UIPasteboard.general.string = sender.titleLabel?.text
    }
}
