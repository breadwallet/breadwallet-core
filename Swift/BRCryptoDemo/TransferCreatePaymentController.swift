//
//  TransferCreatePaymentController.swift
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

class TransferCreatePaymentController: TransferCreateController {

    var payment: PaymentProtocolRequest! = nil

    override func viewDidLoad() {
        super.viewDidLoad()
        bitpayURLTextField.text = "https://test.bitpay.com/i/NzMSnqcd6uUuaVDyn6niVa" //  "https://test.bitpay.com/i/H2ExBjeUHhjm5zhTaNyazH"
        submitButton.isEnabled = false
    }
    
    @IBAction func submit(_ sender: Any) {
        print ("APP: TCP: Want to Pay")

        self.dismiss(animated: true) {}
    }
    
    @IBAction func cancel(_ sender: Any) {
        self.dismiss(animated: true) {}
    }

    func updateFailed () {
        DispatchQueue.main.async {
            self.submitButton.isEnabled = false
            self.updateButton.isEnabled = true
            self.parseLabel.text = ""
        }
    }

    @IBAction func updateButtonAction(_ sender: Any) {
        // Build a URL Request
        guard let url = bitpayURLTextField.text,
            var request = URLComponents (string: url)
                .flatMap ({ $0.url })
                .map ({ URLRequest (url: $0) })
            else { updateFailed(); return }
        request.httpMethod = "GET"
        request.addValue ("application/payment-request", forHTTPHeaderField: "Accept")
        request.addValue ("application/json", forHTTPHeaderField: "Content-Type")

        // Fetch the request
        URLSession (configuration: .default)
            .dataTask(with: request) {
                (data: Data?, res: URLResponse?, error: Error?) in

                guard nil == error else { self.updateFailed(); return }
                guard let res = res as? HTTPURLResponse else { self.updateFailed(); return }
                guard 200 == res.statusCode else { self.updateFailed(); return }
                guard let data = data else { self.updateFailed(); return }

                switch res.mimeType {
                case "application/bitcoin-paymentrequest":
                    self.payment = PaymentProtocolRequest.create(wallet: self.wallet, forBip70: data)
                case "application/payment-request":
                    self.payment = PaymentProtocolRequest.create(wallet: self.wallet, forBitPay: data)
                default:
                    self.updateFailed(); return
                }
                guard nil != self.payment else { self.updateFailed(); return }

                self.payment.estimateFee (fee: self.wallet.manager.network.minimumFee) {
                    (res: Result<TransferFeeBasis, Wallet.FeeEstimationError>) in
                    guard case let .success(basis) = res
                        else { return }
                    print ("Basis: \(basis)")
                }

                DispatchQueue.main.async {
                    self.parseLabel.text = "Yes"
                    // Populate the display
                    self.submitButton.isEnabled = true
                }
            }.resume()
    }

    @IBOutlet var submitButton: UIBarButtonItem!
    @IBOutlet var updateButton: UIButton!
    @IBOutlet var bitpayURLTextField: UITextField!
    @IBOutlet var parseLabel: UILabel!
}
