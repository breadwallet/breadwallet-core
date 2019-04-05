//
//  BRWallet.swift
//  BRCrypto
//
//  Created by Ed Gamble on 4/5/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import BRCryptoC
import BRCore.Ethereum

fileprivate struct BitcoinLegacyAddressScheme: AddressScheme {
    public func getAddress(for wallet: WalletImplS) -> Address {
        return Address.createAsBTC (BRWalletLegacyAddress(wallet.impl.btc))
    }
}

fileprivate struct BitcoinSegwitAddressScheme: AddressScheme {
    public func getAddress(for wallet: WalletImplS) -> Address {
        return Address.createAsBTC (BRWalletReceiveAddress(wallet.impl.btc))
    }
}

//
// WalletImplS
//
class WalletImplS: Wallet {
    internal private(set) weak var listener: WalletListener?

    internal let impl: Impl

    public unowned let manager: WalletManager

    public var system: System {
        return manager.system
    }

    public let name: String

    public let unit: Unit

    public var balance: Amount {
        return impl.balance(in: unit)
    }

    internal func upd (balance: Amount) {
        self.listener?.handleWalletEvent (system: system,
                                          manager: manager,
                                          wallet: self,
                                          event: WalletEvent.balanceUpdated(amount: balance))
    }

    public private(set) var transfers: [Transfer] = []

    internal func add (transfer: Transfer) {
        if !transfers.contains (where: { $0 === transfer }) {
            transfers.append (transfer)
            self.listener?.handleWalletEvent (system: system,
                                              manager: manager,
                                              wallet: self,
                                              event: WalletEvent.transferAdded (transfer: transfer))
        }
    }

    internal func rem (transfer: Transfer) {
        transfers.firstIndex { $0 === transfer }
            .map {
                transfers.remove(at: $0)
                self.listener?.handleWalletEvent (system: system,
                                                  manager: manager,
                                                  wallet: self,
                                                  event: WalletEvent.transferDeleted (transfer: transfer))
        }
    }

    internal func upd (transfer: Transfer) {
        if transfers.contains(where: { $0 === transfer }) {
            self.listener?.handleWalletEvent (system: system,
                                              manager: manager,
                                              wallet: self,
                                              event: WalletEvent.transferChanged(transfer: transfer))
        }
    }

    public func lookup (transfer: TransferHash) -> Transfer? {
        return nil
    }

    internal func lookupTransfer (eth: BREthereumTransfer?) -> Transfer? {
        guard let eth = eth else { return nil }
        return transfers
            .filter { $0 is TransferImplS }
            .first { ($0 as! TransferImplS).impl.matches (eth: eth) }
    }

    internal func lookupTransfer (btc: BRCoreTransaction?) -> Transfer? {
        guard let btc = btc else { return nil }
        return transfers
            .filter { $0 is TransferImplS }
            .first { ($0 as! TransferImplS).impl.matches (btc: btc) }
    }

    public internal(set) var state: WalletState {
        didSet {
            let newValue = state
            self.listener?.handleWalletEvent (system: system,
                                              manager: manager,
                                              wallet: self,
                                              event: WalletEvent.changed(oldState: oldValue, newState: newValue))
        }
    }

    public internal(set) var defaultFeeBasis: TransferFeeBasis {
        didSet {
            let newValue = defaultFeeBasis
            self.listener?.handleWalletEvent (system: system,
                                              manager: manager,
                                              wallet: self,
                                              event: WalletEvent.feeBasisUpdated (feeBasis: newValue))
        }
    }

    internal func updateDefaultFeeBasis () {
        defaultFeeBasis = impl.defaultFeeBasis (in: unit)
     }

    //    public var transferFactory: TransferFactory

    public var target: Address {
        switch impl {
        case .bitcoin:
            return BitcoinLegacyAddressScheme().getAddress(for: self)
        case .ethereum:
            return Address.createAsETH (accountGetPrimaryAddress (manager.account.asETH))
        }
    }

    public var source: Address {
        switch impl {
        case .bitcoin:
            return BitcoinLegacyAddressScheme().getAddress(for: self)
        case .ethereum:
            return Address.createAsETH (accountGetPrimaryAddress (manager.account.asETH))
        }
    }

    internal init (listener: WalletListener?,
                   manager: WalletManagerImplS,
                   unit: Unit,
                   eth: BREthereumWallet) {
        self.listener = listener
        self.manager = manager
        self.name = unit.currency.code
        self.unit = unit
        self.impl = Impl.ethereum (ewm: manager.impl.ewm, core: eth)

        self.state = WalletState.created

        let coreGasPrice = ewmWalletGetDefaultGasPrice (manager.impl.ewm, eth)
        let coreGasLimit = ewmWalletGetDefaultGasLimit (manager.impl.ewm, eth)
        self.defaultFeeBasis = TransferFeeBasis.ethereum (
            gasPrice: Amount.createAsETH (coreGasPrice.etherPerGas.valueInWEI, manager.unit),
            gasLimit: coreGasLimit.amountOfGas)

        self.listener?.handleWalletEvent (system: system,
                                          manager: manager,
                                          wallet: self,
                                          event: WalletEvent.created)
    }

    internal init (listener: WalletListener?,
                   manager: WalletManagerImplS,
                   unit: Unit,
                   btc: BRCoreWallet) {
        self.listener = listener
        self.manager = manager
        self.name = unit.currency.code
        self.unit = unit
        self.impl = Impl.bitcoin (wid: btc)

        self.state = WalletState.created
        self.defaultFeeBasis = TransferFeeBasis.bitcoin(feePerKB: BRWalletFeePerKb (btc))

        self.listener?.handleWalletEvent (system: system,
                                          manager: manager,
                                          wallet: self,
                                          event: WalletEvent.created)
    }


    enum Impl {
        case bitcoin (wid: BRCoreWallet)
        case ethereum (ewm: BREthereumEWM, core: BREthereumWallet)

        internal var ewm: BREthereumEWM! {
            switch self {
            case .bitcoin: return nil
            case .ethereum (let ewm, _): return ewm
            }
        }

        internal var eth: BREthereumWallet! {
            switch self {
            case .bitcoin: return nil
            case .ethereum (_, let eth): return eth
            }
        }

        internal var btc: BRCoreWallet! {
            switch self {
            case .bitcoin (let btc): return btc
            case .ethereum: return nil
            }
        }

        internal func matches (eth: BREthereumWallet) -> Bool {
            switch self {
            case .bitcoin: return false
            case .ethereum (_, let core): return core == eth
            }
        }

        internal func matches (btc: BRCoreWallet) -> Bool {
            switch self {
            case .bitcoin (let wid): return wid == btc
            case .ethereum: return false
            }
        }

        internal func balance (in unit: Unit) -> Amount {
            switch self {
            case let .ethereum (ewm, core):
                let coreETH = ewmWalletGetBalance (ewm, core)
                let value = (coreETH.type == AMOUNT_ETHER
                    ? coreETH.u.ether.valueInWEI
                    : coreETH.u.tokenQuantity.valueAsInteger)
                return Amount.createAsETH (value, unit)
            case let .bitcoin (wid):
                return Amount.createAsBTC (BRWalletBalance (wid), unit)
            }
        }

        internal func defaultFeeBasis (in unit: Unit) -> TransferFeeBasis {
            switch self {
            case let .ethereum (ewm, core):
                let coreGasPrice = ewmWalletGetDefaultGasPrice (ewm, core)
                let coreGasLimit = ewmWalletGetDefaultGasLimit (ewm, core)
                return TransferFeeBasis.ethereum (
                    gasPrice: Amount.createAsETH (coreGasPrice.etherPerGas.valueInWEI, unit),
                    gasLimit: coreGasLimit.amountOfGas)
            case let .bitcoin (wid):
                return TransferFeeBasis.bitcoin(feePerKB: BRWalletFeePerKb (wid))
            }
        }
    }
}

