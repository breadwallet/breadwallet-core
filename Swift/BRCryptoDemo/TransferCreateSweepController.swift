//
//  TransferCreateSweepController.swift
//  BRCryptoDemo
//
//  Created by Ed Gamble on 8/6/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import UIKit
import BRCrypto

class TransferCreateSweepController: TransferCreateController {

    let bitcoinTestnetPrivateKey = "92ihfZg8rvDhyY2siUG5zTB9PvZju6KJQJWkeQPp99NSD93zPS3"
    var walletSweeper: WalletSweeper! = nil
    var walletSweeperFeeBasis: TransferFeeBasis! = nil

    var walletSweeperAmount: Amount? {
        return walletSweeper?.balance?.convert(to: wallet.manager.network.defaultUnitFor (currency: wallet.currency)!)
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        submitButton.isEnabled = false
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)

        privateKeyTextField.text = nil
        if nil != UIPasteboard.general.string {
            privateKeyTextField.text = UIPasteboard.general.string
        }
        else {
            switch wallet.currency.code.lowercased() {
            case "btc":
                if !wallet.manager.network.isMainnet {
                    privateKeyTextField.text = bitcoinTestnetPrivateKey
                }
            default: break
            }
        }
        updateView()
    }

    func alertError (_ message: String) {
        DispatchQueue.main.async {
            let alert = UIAlertController (title: "Sweep Error",
                                           message: message,
                                           preferredStyle: UIAlertController.Style.alert)

            alert.addAction(UIAlertAction (title: "Okay", style: UIAlertAction.Style.cancel) { (action) in
                self.dismiss (animated: true) {}
            })
            self.present (alert, animated: true) {}
        }
    }

    func updateView () {

        if let privateKey = privateKeyTextField.text,
            let key = Key.createFromString(asPrivate: privateKey) {

            DispatchQueue.main.async {
                self.updateButton.isEnabled = false
                self.updateSpinner.startAnimating()
                self.balanceLabel.text = nil
                self.feeLabel.text = nil
            }

            self.wallet.manager.createSweeper (wallet: wallet, key: key) {
                (res: Result<WalletSweeper, WalletSweeperError>) in
                guard case let .success (sweeper) = res
                    else {
                        if case let .failure (error) = res {
                            print ("APP: TCSC: Sweeper Error: \(error)")
                            self.alertError ("Sweeper Error: \(error)")
                        }
                        DispatchQueue.main.async {
                            self.updateButton.isEnabled = true
                            self.updateSpinner.stopAnimating()
                        }
                        return
                }
                self.walletSweeper = sweeper
                DispatchQueue.main.async {
                    self.balanceLabel.text = self.walletSweeperAmount?.description ?? "?"
                }

                sweeper.estimate (fee: self.wallet.manager.network.minimumFee) {
                    (res: Result<TransferFeeBasis, Wallet.FeeEstimationError>) in
                    guard case let .success (tfb) = res
                        else {
                            if case let .failure (error) = res {
                                print ("APP: TCSC: Sweeper FeeEstimate Error: \(error)")
                                self.alertError ("Sweeper FeeEstimate Error: \(error)")
                            }
                            DispatchQueue.main.async {
                                self.updateButton.isEnabled = true
                                self.updateSpinner.stopAnimating()
                                self.balanceLabel.text = nil
                           }
                            return
                    }
                    self.walletSweeperFeeBasis = tfb
                    DispatchQueue.main.async {
                        self.feeLabel.text = tfb.fee.description
                    }

                    // Update fees etc
                    print ("APP: TCSC: Update Fees View")
                    DispatchQueue.main.async {
                        self.submitButton.isEnabled = true
                        self.updateButton.isEnabled = true
                        self.updateSpinner.stopAnimating()
                    }
                }
            }
        }
    }

    @IBAction func submit(_ sender: Any) {
        print ("APP: TCSC: Want to Submit")

        let alert = UIAlertController (title: "Sweep for \(walletSweeperAmount?.description ?? "?") \(wallet.name)",
            message: "Are you sure?",
            preferredStyle: UIAlertController.Style.actionSheet)

        alert.addAction(UIAlertAction (title: "Yes", style: UIAlertAction.Style.destructive) { (action) in
            print ("APP: TCSC: Will Submit" )
            let _ = self.walletSweeper.submit(estimatedFeeBasis: self.walletSweeperFeeBasis)
            self.dismiss(animated: true) {}
        })

        alert.addAction(UIAlertAction (title: "No", style: UIAlertAction.Style.cancel) { (action) in
            print ("APP: TCSC: Will Cancel" )
        })

        self.present (alert, animated: true) {}
    }

    @IBAction func cancel(_ sender: Any) {
        self.dismiss (animated: true) {}
    }

    @IBAction func updatePrivateKey(_ sender: UIButton) {
        updateView()
    }

    @IBAction func cameraGetPrivateKey(_ sender: UIButton) {
        print ("APP: TCSC: Want to use Camera")
    }
    
    @IBOutlet var submitButton: UIBarButtonItem!
    @IBOutlet var privateKeyTextField: UITextField!
    @IBOutlet var updateButton: UIButton!
    @IBOutlet var updateSpinner: UIActivityIndicatorView!
    @IBOutlet var balanceLabel: UILabel!
    @IBOutlet var feeLabel: UILabel!
}
