//
//  TransferViewController.swift
//  CoreDemo
//
//  Created by Ed Gamble on 8/22/18.
//  Copyright © 2018-2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import UIKit
import BRCrypto

class TransferViewController: UIViewController, TransferListener, WalletManagerListener {
    var transfer : Transfer!
    var wallet  : Wallet!

    var dateFormatter : DateFormatter!

    override func viewDidLoad() {
        super.viewDidLoad()

        if nil == dateFormatter {
            dateFormatter = DateFormatter()
            dateFormatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        }
    }

    override func viewWillAppear(_ animated: Bool) {
        if let listener = UIApplication.sharedSystem.listener as? CoreDemoListener {
            listener.add (managerListener: self)
            listener.add (transferListener: self)
        }
        
        super.viewWillAppear(animated);
        updateView ()
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        if let listener = UIApplication.sharedSystem.listener as? CoreDemoListener {
            listener.remove (managerListener: self)
            listener.remove (transferListener: self)
        }
        
        super.viewWillDisappear(animated)
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }

    func colorForState() -> UIColor {
        guard let state = transfer?.state else { return UIColor.black }
        switch state {
        case .created: return UIColor.gray
        case .submitted: return UIColor.yellow
        case .included:
            return transfer!.confirmation!.success ? UIColor.green : UIColor.red
        case .deleted: return UIColor.black

        case .signed: return UIColor.yellow
        case .pending: return UIColor.yellow
        case .failed /*(let reason )*/: return UIColor.red
        }
    }


    func updateView () {
//        let address = UIApplication.sharedClient.node.address
        let date: Date? = (nil == transfer.confirmation ? nil
            : Date (timeIntervalSince1970: TimeInterval(transfer.confirmation!.timestamp)))
        let hash = transfer.hash

        amountLabel.text = transfer.amountDirected.description
        feeLabel.text  =  transfer.fee.description
        dateLabel.text = date.map { dateFormatter.string(from: $0) } ?? "<pending>"
        sendButton.setTitle(transfer.source?.description ?? "<unknown>", for: .normal)
        recvButton.setTitle(transfer.target?.description ?? "<unknown>", for: .normal)

        identifierLabel.text = hash.map { $0.description } ?? "<pending>"

        confLabel.text = transfer.confirmation.map { "Yes @ \($0.blockNumber)" } ?? "No"
        confCountLabel.text = transfer.confirmations?.description ?? ""

        if ("eth" != wallet.currency.code) {
            nonceTitleLabel.isHidden = true
            nonceLabel.isHidden = true;
        }

        attributesLabel.text = transfer.attributes
            .map { "\($0.key)(\($0.isRequired ? "R" : "O")):\($0.value ?? "")" }
            .joined(separator: ", ")

        switch transfer.state {
        case .failed(let reason):
            stateLabel.text = "\(transfer.state.description): \(reason)"
        default:
            stateLabel.text = transfer.state.description
        }

        switch transfer.state {
//        case .errored:
//            cancelButton.isEnabled   = wallet.canCancelTransfer(transfer: transfer)
//            resubmitButton.isEnabled = wallet.canReplaceTransfer(transfer: transfer)
        default:
            cancelButton.isEnabled   = false
            resubmitButton.isEnabled = false
       }

        dotView.mainColor = colorForState()
        dotView.setNeedsDisplay()
    }

    @IBAction func doResubmit(_ sender: UIButton) {
        print ("APP: TVC: Want to Resubmit")
//        if case .failed(let error) = transfer.state {
//            var alertMessage: String = "Okay to resubmit?"
//            var alertAction: UIAlertAction?
//            var alert: UIAlertController!
//
//            switch error {
//            case .nonceTooLow:
//                alertMessage = "Okay to update nonce and resubmit?"
////                alertAction = UIAlertAction (title: "Yes", style: UIAlertAction.Style.default) { (action) in
////                    let replacement = self.wallet.createTransferToReplace (transfer: self.transfer,
////                                                                           updateNonce: true)
////                    self.wallet.sign(transfer: replacement,
////                                     paperKey: UIApplication.sharedClient.paperKey);
////                    self.wallet.submit(transfer: replacement);
////                    alert.dismiss(animated: true) {}
////                }
////
//            case .gasPriceTooLow:
//                alertMessage = "Okay to double gasPrice (fee doubles) and resubmit?"
////                alertAction = UIAlertAction (title: "Yes", style: UIAlertAction.Style.default) { (action) in
////                    let replacement = self.wallet.createTransferToReplace (transfer: self.transfer,
////                                                                           updateGasPrice: true)
////                    self.wallet.sign(transfer: replacement,
////                                     paperKey: UIApplication.sharedClient.paperKey);
////                    self.wallet.submit(transfer: replacement);
////                    alert.dismiss(animated: true) {}
////                }
////
//            case .gasTooLow:
//                alertMessage = "Okay to double gasLimit (fee doubles) and resubmit?"
////                alertAction = UIAlertAction (title: "Yes", style: UIAlertAction.Style.default) { (action) in
////                    let replacement = self.wallet.createTransferToReplace (transfer: self.transfer,
////                                                                           updateGasLimit: true)
////                    self.wallet.sign(transfer: replacement,
////                                     paperKey: UIApplication.sharedClient.paperKey);
////                    self.wallet.submit(transfer: replacement);
////                    alert.dismiss(animated: true) {}
////                }
////
////
//            case .invalidSignature,
//                 .replacementUnderPriced:
//                alertMessage = "Unsupported error";
////                alertAction = UIAlertAction (title: "Okay", style: UIAlertAction.Style.default) { (action) in
////                    alert.dismiss(animated: true) {}
////                }
////
//            case .balanceTooLow,  // money arrived?
//                 .dropped,
//                 .unknown:
//                alertMessage = "Okay to double fee and resubmit?"
////                alertAction = UIAlertAction (title: "Yes", style: UIAlertAction.Style.default) { (action) in
////                    let replacement = self.wallet.createTransferToReplace (transfer: self.transfer,
////                                                                           updateGasPrice: true)
////                    self.wallet.sign(transfer: replacement,
////                                     paperKey: UIApplication.sharedClient.paperKey);
////                    self.wallet.submit(transfer: replacement);
////                    alert.dismiss(animated: true) {}
////                }
//            }
//
//            if let actionOnOkay = alertAction {
//                alert = UIAlertController (title: "Resubmit",
//                                           message: alertMessage,
//                                           preferredStyle: UIAlertController.Style.alert)
//                alert.addAction (actionOnOkay)
//                alert.addAction(UIAlertAction (title: "No", style: UIAlertAction.Style.cancel))
//
//                self.present (alert, animated: true) {}
//            }
//        }
    }

    /*
     * Canceling means generating a 0 ETH transaction to Your Own Address with the purpose of
     * preventing a previous transaction from "going through" / "being mined" / "being included in
     * the blockchain" / "being stuck"
     */
    @IBAction func doCancel(_ sender: UIButton) {
        print ("APP: TVC: Want to Cancel")
        let alert = UIAlertController (title: "Cancel Transaction for <small-fee> ETH",
                                       message: "Are you sure?",
                                       preferredStyle: UIAlertController.Style.actionSheet)

//        alert.addAction(UIAlertAction (title: "Yes", style: UIAlertAction.Style.destructive) { (action) in
//            let replacement = self.wallet.createTransferToCancel(transfer: self.transfer);
//
//            self.wallet.sign(transfer: replacement,
//                             paperKey: UIApplication.sharedClient.paperKey);
//
//            self.wallet.submit(transfer: replacement);
//
//            self.dismiss(animated: true) {}
//        })

        alert.addAction(UIAlertAction (title: "No", style: UIAlertAction.Style.cancel))
        self.present(alert, animated: true) {}
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
    @IBOutlet var sendButton: UIButton!
    @IBOutlet var recvButton: UIButton!
    @IBOutlet var identifierLabel: UILabel!
    @IBOutlet var confLabel: UILabel!
    @IBOutlet var confCountLabel: UILabel!
    @IBOutlet var stateLabel: UILabel!
    @IBOutlet var cancelButton: UIButton!
    @IBOutlet var resubmitButton: UIButton!
    @IBOutlet var nonceTitleLabel: UILabel!
    @IBOutlet var nonceLabel: UILabel!
    @IBOutlet var attributesLabel: UILabel!
    @IBOutlet var dotView: Dot!
    @IBAction func toPasteBoard(_ sender: UIButton) {
        UIPasteboard.general.string = sender.titleLabel?.text
    }

    func handleManagerEvent (system: System, manager: WalletManager, event: WalletManagerEvent) {
        DispatchQueue.main.async {
            print ("APP: TVC: ManagerEvent: \(event)")
            guard self.wallet.manager == manager /* && view is visible */  else { return }
            switch event {
            case .blockUpdated:
                self.updateView()
            default:
                break
            }
        }
    }
    
    func handleTransferEvent(system: System, manager: WalletManager, wallet: Wallet, transfer: Transfer, event: TransferEvent) {
        DispatchQueue.main.async {
            print ("APP: TVC: TransferEvent: \(event)")
            guard self.wallet.manager == manager && self.wallet == wallet && self.transfer == transfer  else { return }
            self.updateView()
        }
    }
}

