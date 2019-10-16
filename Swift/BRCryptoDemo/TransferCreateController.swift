//
//  TransferCreateController.swift
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

class TransferCreateController: UIViewController {

    var wallet : Wallet!
    var fee: NetworkFee!
    var feeBasis: TransferFeeBasis?


    override func viewDidLoad() {
        super.viewDidLoad()
    }
}
