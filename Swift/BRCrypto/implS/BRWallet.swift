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

    //
    // Balance
    //
    public var balance: Amount {
        return impl.balance(in: unit)
    }

    internal func upd (balance: Amount) {
        announceEvent (WalletEvent.balanceUpdated(amount: balance))
    }

    //
    // Transfers
    //
    public private(set) var transfers: [Transfer] = []

    internal func add (transfer: Transfer) {
        if !transfers.contains (where: { $0 === transfer }) {
            guard let transfer = transfer as? TransferImplS
                else { precondition (false); return }

            transfers.append (transfer)
            transfer.announceEvent(TransferEvent.created)
            announceEvent(WalletEvent.transferAdded (transfer: transfer))
        }
    }

    internal func rem (transfer: Transfer) {
        transfers.firstIndex { $0 === transfer }
            .map {
                guard let transfer = transfer as? TransferImplS
                    else { precondition (false); return }

                announceEvent (WalletEvent.transferDeleted (transfer: transfer))
                transfer.announceEvent (TransferEvent.deleted)
                transfers.remove(at: $0)
        }
    }

    internal func upd (transfer: Transfer) {
        if transfers.contains(where: { $0 === transfer }) {
            announceEvent (WalletEvent.transferChanged(transfer: transfer))
        }
    }

    public func transferBy (hash: TransferHash) -> Transfer? {
        return transfers
            .first { $0.hash.map { $0 == hash } ?? false }
    }

    internal func transferBy (impl: TransferImplS.Impl) -> Transfer? {
        return transfers
            .first { ($0 as? TransferImplS)?.impl.matches (impl) ?? false }
    }

    internal func transferByImplOrCreate (_ impl: TransferImplS.Impl,
                                          listener: TransferListener?,
                                          create: Bool = false) -> TransferImplS? {
        return transferBy (impl: impl) as? TransferImplS ??
            (!create
                ? nil
                : TransferImplS (listener: listener,
                                 wallet: self,
                                 unit: unit,
                                 impl: impl))
    }

    func createTransfer (listener: TransferListener,
                         target: Address,
                         amount: Amount,
                         feeBasis: TransferFeeBasis) -> Transfer? {
        return impl.createTransfer (target: target, amount: amount, feeBasis: feeBasis)
            .map {
                TransferImplS (listener: listener,
                               wallet: self,
                               unit: amount.unit,
                               impl: $0)
        }
    }

    public internal(set) var state: WalletState {
        didSet {
            let newValue = state
            announceEvent (WalletEvent.changed (oldState: oldValue, newState: newValue))
        }
    }

    public internal(set) var defaultFeeBasis: TransferFeeBasis {
        didSet {
            let newValue = defaultFeeBasis
            announceEvent (WalletEvent.feeBasisUpdated (feeBasis: newValue))
        }
    }

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

    ///
    /// Create a wallet using `impl` *and* add it to the manager.  By adding it to the manager
    /// we ensure that `manager <==> wallet` constraints are maintained.
    ///
    /// This will generate a Wallet.created event and then aWalletManager.walletAdded event.
    ///
    /// - Parameters:
    ///   - listener: The wallet listener
    ///   - manager: this wallet's manager
    ///   - unit: the default unit
    ///   - impl: a reference to the underlying 'C' implementation
    ///
    internal init (listener: WalletListener?,
                   manager: WalletManagerImplS,
                   unit: Unit,
                   impl: Impl) {
        let feeUnit = manager.network.baseUnitFor (currency: manager.currency)!

        self.listener = listener
        self.manager = manager
        self.name = unit.currency.code
        self.unit = unit
        self.state = WalletState.created
        self.impl = impl
        self.defaultFeeBasis = impl.defaultFeeBasis(in: feeUnit)

        print ("SYS: Wallet (\(manager.name):\(name)): Init")
        manager.add (wallet: self)
     }

    internal func announceEvent (_ event: WalletEvent) {
        self.listener?.handleWalletEvent (system: system,
                                          manager: manager,
                                          wallet: self,
                                          event: event)
    }

    ///
    /// Impl
    ///
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

        internal func matches (_ that: Impl) -> Bool {
            switch (self, that) {
            case (let .bitcoin (wid1), let .bitcoin (wid2)):
                return wid1 == wid2
            case (let .ethereum (ewm1, c1), let .ethereum (ewm2, c2)):
                return ewm1 == ewm2 && c1 == c2
            default:
                return false
            }
        }
        
        internal func balance (in unit: Unit) -> Amount {
            switch self {
            case let .ethereum (ewm, core):
                let coreETH = ewmWalletGetBalance (ewm, core)
                let value = (coreETH.type == AMOUNT_ETHER
                    ? coreETH.u.ether.valueInWEI
                    : coreETH.u.tokenQuantity.valueAsInteger)
                return Amount.create (uint256: value, unit)
            case let .bitcoin (wid):
                return Amount.create(uint64: BRWalletBalance(wid), unit)
            }
        }

        internal func defaultFeeBasis (in unit: Unit) -> TransferFeeBasis {
            switch self {
            case let .ethereum (ewm, core):
                let coreGasPrice = ewmWalletGetDefaultGasPrice (ewm, core)
                let coreGasLimit = ewmWalletGetDefaultGasLimit (ewm, core)
                return TransferFeeBasis.ethereum (
                    gasPrice: Amount.create(uint256: coreGasPrice.etherPerGas.valueInWEI, unit),
                    gasLimit: coreGasLimit.amountOfGas)
            case let .bitcoin (wid):
                return TransferFeeBasis.bitcoin(feePerKB: BRWalletFeePerKb (wid))
            }
        }

        internal func createTransfer (target: Address,
                                      amount: Amount,
                                      feeBasis: TransferFeeBasis) -> TransferImplS.Impl? {
            let addr = cryptoAddressAsString (target.core)
            switch self {
            case let .ethereum (ewm, wid):
                guard case let .ethereum (gasPrice, gasLimit) = feeBasis
                    else { return nil }

                let ethValue  = cryptoAmountGetValue (amount.core)
                let ethAmount = ewmWalletGetToken (ewm, wid)
                    .map { amountCreateToken (createTokenQuantity ($0, ethValue))}
                ?? amountCreateEther (etherCreate(ethValue))

                let ethGasLimit = gasCreate (gasLimit)
                let ethGasPrice = gasPriceCreate (etherCreate (cryptoAmountGetValue (gasPrice.core)))
                let ethFeeBasis = feeBasisCreate (ethGasLimit, ethGasPrice)

                return ewmWalletCreateTransferWithFeeBasis (ewm, wid, addr, ethAmount, ethFeeBasis)
                    .map { TransferImplS.Impl.ethereum (ewm: ewm, core: $0) }

            case let .bitcoin (wid):
                var overflow: BRCryptoBoolean = CRYPTO_FALSE
                let value = cryptoAmountGetIntegerRaw(amount.core, &overflow)
                if CRYPTO_TRUE == overflow { return nil }

                return BRWalletCreateTransaction (wid, value, addr)
                    .map { TransferImplS.Impl.bitcoin(wid: wid, tid: $0) }
            }
        }
    }
}

