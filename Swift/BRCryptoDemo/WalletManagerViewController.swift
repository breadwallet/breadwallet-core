//
//  WalletManagerViewController.swift
//  BRCryptoDemo
//
//  Created by Ed Gamble on 8/22/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import UIKit
import BRCrypto

class WalletManagerViewController: UIViewController, WalletManagerListener {
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

    override func viewWillDisappear(_ animated: Bool) {
        if let listener = UIApplication.sharedSystem.listener as? CoreDemoListener {
            listener.remove (managerListener: self)
        }

        super.viewWillDisappear(animated)
    }

    func modeSegmentIsSelected (_ index: Int) -> Bool {
        return manager.mode == modes[index]
    }

    func modeSegmentIsEnabled (_ index: Int) -> Bool {
        return manager.network.supportsMode(modes[index])
    }

    func addressSchemeIsSelected (_ index: Int) -> Bool {
        return manager.addressScheme == addressSchemes[index]
    }

    func addressSchemeIsEnabled (_ index: Int) -> Bool {
        return manager.network.supportsAddressScheme(addressSchemes[index])
   }

    func connectStateIsSelected (_ index: Int, _ state: WalletManagerState? = nil) -> Bool {
        switch state ?? manager.state {
        case .disconnected: return index == 0
        case .connected:    return index == 1
        case .syncing:      return index == 2
        default: return false
        }
    }

    func connectStateIsEnabled (_ index: Int, _ state: WalletManagerState? = nil) -> Bool {
        switch state ?? manager.state {
        case .created:      return 1 == index
        case .disconnected: return 1 == index
        case .connected:    return 0 == index || 2 == index
        case .syncing:      return 1 == index
        default: return false
        }
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        self.navigationItem.title = "\(manager.name.uppercased()) Wallet Manager"
        if let listener = UIApplication.sharedSystem.listener as? CoreDemoListener {
            listener.add (managerListener: self)
        }
        
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

        for index in 0..<3 {
            connectStateSegmentedControl.setEnabled(connectStateIsEnabled(index), forSegmentAt: index)
            if connectStateIsSelected(index) {
                connectStateSegmentedControl.selectedSegmentIndex = index
            }
        }
        self.connectStateLabel.text = manager.state.description
        self.blockHeightLabel.text  = manager.network.height.description
        self.syncProgressLabel.text = ""
        updateView ()
    }

    func updateView () {
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
    @IBOutlet var connectStateSegmentedControl: UISegmentedControl!
    @IBAction func selectConnectState(_ sender: UISegmentedControl) {
        switch sender.selectedSegmentIndex {
        case 0: manager.disconnect()
        case 1: manager.connect()
        case 2: manager.sync()
        default: break
        }
    }
    @IBOutlet var connectStateLabel: UILabel!
    @IBOutlet var blockHeightLabel: UILabel!
    @IBOutlet var syncProgressLabel: UILabel!

    func handleManagerEvent(system: System, manager: WalletManager, event: WalletManagerEvent) {
        guard manager == self.manager else { return }
        DispatchQueue.main.async {
            switch event {
            case let .changed (_, newState):
                self.connectStateLabel.text = newState.description
                for index in 0..<3 {
                    self.connectStateSegmentedControl.setEnabled(self.connectStateIsEnabled(index, newState), forSegmentAt: index)
                    if self.connectStateIsSelected(index, newState) {
                        self.connectStateSegmentedControl.selectedSegmentIndex = index
                    }
                }
            case .syncProgress(_, let percentComplete):
                self.syncProgressLabel.text = percentComplete.description
            case .syncEnded(let reason):
                switch reason {
                case .complete: self.syncProgressLabel.text = "COMPLETE"
                case .requested: self.syncProgressLabel.text = ""
                case .unknown: self.syncProgressLabel.text = "<UNKNOWN>"
                case .posix(let errnum, let message): self.syncProgressLabel.text = "\(errnum): \(message ?? "")"
                }
            case .blockUpdated(let height):
                self.blockHeightLabel.text = height.description
            default:
                break
            }
        }
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

extension WalletManagerState: CustomStringConvertible {
    public var description: String {
        switch self {
        case .created:      return "Created"
        case .disconnected: return "Disconnected"
        case .connected:    return "Connected"
        case .syncing:      return "Syncing"
        case .deleted:      return "Deleted"
        }
    }
}
