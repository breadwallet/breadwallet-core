//
//  WalletManagerViewController.swift
//  BRCryptoDemo
//
//  Created by Ed Gamble on 8/22/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import UIKit
import BRCrypto

class WalletManagerViewController: UIViewController {

    var manager: WalletManager!
    let modes = [WalletManagerMode.api_only,
                 WalletManagerMode.api_with_p2p_submit,
                 WalletManagerMode.p2p_with_api_sync,
                 WalletManagerMode.p2p_only]

    let addressSchemes = [AddressScheme.btcLegacy,
                          AddressScheme.btcSegwit,
                          AddressScheme.ethDefault,
                          AddressScheme.genDefault]

    override func viewDidLoad() {
        super.viewDidLoad()
    }

    func modeSegmentIsSelected (_ index: Int) -> Bool {
        return manager.mode == modes[index]
    }

    func modeSegmentIsEnabled (_ index: Int) -> Bool {
        return UIApplication.sharedSystem.supportsMode(network: manager.network, modes[index])
    }

    func addressSchemeIsSelected (_ index: Int) -> Bool {
        return manager.addressScheme == addressSchemes[index]
    }

    func addressSchemeIsEnabled (_ index: Int) -> Bool {
        return UIApplication.sharedSystem.supportsAddressScheme(network: manager.network, addressSchemes[index])
   }
 
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        for index in 0..<modes.count {
            modeSegmentedControl.setEnabled (modeSegmentIsEnabled(index), forSegmentAt: index)
            if modeSegmentIsSelected(index) {
                modeSegmentedControl.selectedSegmentIndex = index
            }
        }

        for index in 0..<addressSchemes.count {
            addressSchemeSegmentedControl.setEnabled(addressSchemeIsEnabled(index), forSegmentAt: index)
            if addressSchemeIsSelected(index) {
                addressSchemeSegmentedControl.selectedSegmentIndex = index
            }
        }
    }

    /*
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        // Get the new view controller using segue.destination.
        // Pass the selected object to the new view controller.
    }
    */
    @IBOutlet var modeSegmentedControl: UISegmentedControl!
    @IBAction func selectMode(_ sender: UISegmentedControl) {
        print ("WMVC: Mode: \(modes[sender.selectedSegmentIndex].description)")
        manager.mode = modes [sender.selectedSegmentIndex]
    }
    @IBOutlet var addressSchemeSegmentedControl: UISegmentedControl!
    @IBAction func selectAddressScheme(_ sender: UISegmentedControl) {
        print ("WMVC: AddressScheme: \(addressSchemes[sender.selectedSegmentIndex].description)")
        manager.addressScheme = addressSchemes[sender.selectedSegmentIndex]
    }
}

extension WalletManagerMode: CustomStringConvertible {
    public var description: String {
        switch self {
        case .api_only:
            return "api_only"
        case .api_with_p2p_submit:
            return "api_with_p2p_submit"
        case .p2p_with_api_sync:
            return "p2p_with_api_sync"
        case .p2p_only:
            return "p2p_only"
        }
    }
}
