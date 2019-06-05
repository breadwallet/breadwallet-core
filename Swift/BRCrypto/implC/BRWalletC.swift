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

//
// WalletImplC
//
class WalletImplC: Wallet {
    internal private(set) weak var listener: WalletListener?

    internal let core: BRCryptoWallet

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
        return Amount (core: cryptoWalletGetBalance (core), unit: unit)
    }

    internal func upd (balance: Amount) {
        announceEvent (WalletEvent.balanceUpdated(amount: balance))
    }

    //
    // Transfers
    //
    public var transfers: [Transfer] {
        let listener = manager.system.listener
        return (0..<cryptoWalletGetTransferCount(core))
            .map { TransferImplC (core: cryptoTransferTake (cryptoWalletGetTransfer (core, $0)),
                                  listener: listener,
                                  wallet: self,
                                  unit: unit) }
    }

//    internal func add (transfer: Transfer) {
//        if !transfers.contains (where: { $0 === transfer }) {
//            guard let transfer = transfer as? TransferImplS
//                else { precondition (false); return }
//
//            transfers.append (transfer)
//            transfer.announceEvent(TransferEvent.created)
//            announceEvent(WalletEvent.transferAdded (transfer: transfer))
//        }
//    }
//
//    internal func rem (transfer: Transfer) {
//        transfers.firstIndex { $0 === transfer }
//            .map {
//                guard let transfer = transfer as? TransferImplS
//                    else { precondition (false); return }
//
//                announceEvent (WalletEvent.transferDeleted (transfer: transfer))
//                transfer.announceEvent (TransferEvent.deleted)
//                transfers.remove(at: $0)
//        }
//    }
//
//    internal func upd (transfer: Transfer) {
//        if transfers.contains(where: { $0 === transfer }) {
//            announceEvent (WalletEvent.transferChanged(transfer: transfer))
//        }
//    }

    public func transferBy (hash: TransferHash) -> Transfer? {
        return transfers
            .first { $0.hash.map { $0 == hash } ?? false }
    }

    internal func transferBy (core: BRCryptoTransfer) -> Transfer? {
        return transfers
            .first { ($0 as? TransferImplC).map { $0.core == core } ?? false }
    }

    internal func transferByCoreOrCreate (_ core: BRCryptoTransfer,
                                          listener: TransferListener?,
                                          create: Bool = false) -> TransferImplC? {
        return transferBy (core: core) as? TransferImplC ??
            (!create
                ? nil
                : TransferImplC (core: core,
                                 listener: listener,
                                 wallet: self,
                                 unit: unit))
    }

    func createTransfer (target: Address,
                         amount: Amount,
                         feeBasis: TransferFeeBasis) -> Transfer? {
        return cryptoWalletCreateTransfer (core, target.core, amount.core, feeBasis.core)
            .map {
                TransferImplC (core: $0,
                               listener: self.manager.system.listener,
                               wallet: self,
                               unit: amount.unit)
        }
    }

    public internal(set) var state: WalletState {
        didSet {
            let newValue = state // rename, for clarity; analogous to `oldValue`
            announceEvent (WalletEvent.changed (oldState: oldValue, newState: newValue))
        }
    }

    public internal(set) var defaultFeeBasis: TransferFeeBasis {
        get {
            return TransferFeeBasis (core: cryptoWalletGetDefaultFeeBasis (core)) }
        set {
            let defaultFeeBasis = newValue // rename, for clarity
            cryptoWalletSetDefaultFeeBasis (core, defaultFeeBasis.core);
            announceEvent (WalletEvent.feeBasisUpdated (feeBasis: defaultFeeBasis))
        }
    }

    public var target: Address {
        return Address (core: cryptoWalletGetAddress (core))
    }

    public var source: Address {
        return Address (core: cryptoWalletGetAddress (core))
    }

    func estimateFee (amount: Amount,
                      feeBasis: TransferFeeBasis?) -> Amount {
        precondition (amount.hasCurrency (currency))
        let unit = manager.network.baseUnitFor (currency: manager.currency)!
        return Amount (core: cryptoWalletEstimateFee (core, amount.core, feeBasis?.core, unit.core),
                       unit: unit)
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
    internal init (core: BRCryptoWallet,
                   listener: WalletListener?,
                   manager: WalletManagerImplC,
                   unit: Unit) {
        self.core = core
        self.listener = listener
        self.manager = manager
        self.name = unit.currency.code
        self.unit = unit
        self.state = WalletState.created

        print ("SYS: Wallet (\(manager.name):\(name)): Init")
        manager.add (wallet: self)
     }

    internal func announceEvent (_ event: WalletEvent) {
        self.listener?.handleWalletEvent (system: system,
                                          manager: manager,
                                          wallet: self,
                                          event: event)
    }

    deinit {
        cryptoWalletGive (core)
    }
}

