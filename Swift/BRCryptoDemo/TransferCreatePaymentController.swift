//
//  TransferCreatePaymentController.swift
//  BRCryptoDemo
//
//  Created by Ed Gamble on 8/6/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import UIKit

class TransferCreatePaymentController: TransferCreateController {

    override func viewDidLoad() {
        super.viewDidLoad()

        submitButton.isEnabled = false
    }
    
    @IBAction func submit(_ sender: Any) {
        print ("APP: TCP: Want to Pay")
        self.dismiss(animated: true) {}
    }
    
    @IBAction func cancel(_ sender: Any) {
        self.dismiss(animated: true) {}
    }
    
    @IBOutlet var submitButton: UIBarButtonItem!
}
