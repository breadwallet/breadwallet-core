//
//  BRTransfer.swift
//  BRCrypto
//
//  Created by Ed Gamble on 4/4/19.
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


// ==================



class TransferImplS: Transfer {
    internal private(set) weak var listener: TransferListener?

    internal let impl: Impl

    public unowned let wallet: Wallet

    public var manager: WalletManager {
        return wallet.manager
    }

    public var system: System {
        return wallet.manager.system
    }

    internal let unit: Unit

    public private(set) lazy var source: Address? = {
        return impl.source (sent: self.isSent)
    }()

    public private(set) lazy var target: Address? = {
        return impl.target (sent: self.isSent)
    }()

    public private(set) lazy var amount: Amount = {
        return impl.amount (in: wallet.unit)
    } ()

    public private(set) lazy var fee: Amount = {
        let unit = wallet.manager.network.defaultUnitFor (currency: wallet.manager.currency)!
        return impl.fee (in: unit)
    }()

    var feeBasis: TransferFeeBasis

    public private(set) var hash: TransferHash? = nil

    public internal(set) var state: TransferState {
        didSet {
            let newValue = state
            listener?.handleTransferEvent (system: wallet.manager.system,
                                           manager: wallet.manager,
                                           wallet: wallet,
                                           transfer: self,
                                           event: TransferEvent.changed (old: oldValue,
                                                                         new: newValue))
        }
    }

    /// Flag to determine if the wallet's owner sent this transfer
    public private(set) lazy var isSent: Bool = {
        return impl.isSent
    }()

    internal init (listener: TransferListener?,
                   wallet: WalletImplS,
                   unit: Unit,
                   eth: BREthereumTransfer) {
        self.listener = listener
        self.wallet = wallet
        self.unit = unit
        self.state = TransferState.created
        self.impl = Impl.ethereum (ewm:wallet.impl.ewm, core: eth)

        let gasUnit = wallet.manager.network.baseUnitFor(currency: wallet.manager.currency)!
        let gasPrice = Amount.createAsETH(createUInt256 (0), gasUnit)
        self.feeBasis = TransferFeeBasis.ethereum (gasPrice: gasPrice, gasLimit: 0)
        self.isSent = true

        self.listener?.handleTransferEvent (system: system,
                                            manager: manager,
                                            wallet: wallet,
                                            transfer: self,
                                            event: TransferEvent.created)
    }

    internal init (listener: TransferListener?,
                   wallet: WalletImplS,
                   unit: Unit,
                   btc: BRCoreTransaction) {
        self.listener = listener
        self.wallet = wallet
        self.unit = unit
        self.state = TransferState.created
        self.impl = Impl.bitcoin (wid: wallet.impl.btc, tid: btc)

        let gasUnit = wallet.manager.network.baseUnitFor(currency: wallet.manager.currency)!

        let gasPrice = Amount.createAsETH(createUInt256 (0), gasUnit)
        self.feeBasis = TransferFeeBasis.ethereum(gasPrice: gasPrice, gasLimit: 0)

        self.listener?.handleTransferEvent (system: system,
                                            manager: manager,
                                            wallet: wallet,
                                            transfer: self,
                                            event: TransferEvent.created)
    }


    enum Impl {
        case bitcoin (wid: BRCoreWallet, tid: BRCoreTransaction)
        case ethereum (ewm: BREthereumEWM, core: BREthereumTransfer)

        internal var eth: BREthereumTransfer! {
            switch self {
            case .bitcoin: return nil
            case .ethereum (_, let core): return core
            }
        }

        internal var btc: BRCoreTransaction! {
            switch self {
            case .bitcoin (_, let tid): return tid
            case .ethereum: return nil
            }
        }

        internal func matches (eth: BREthereumTransfer) -> Bool {
            switch self {
            case .bitcoin: return false
            case .ethereum (_, let tid): return tid == eth
            }
        }

        internal func matches (btc: BRCoreTransaction) -> Bool {
            switch self {
            case .bitcoin (_, let tid): return tid == btc
            case .ethereum: return false
            }
        }

        internal func source (sent: Bool) -> Address? {
            switch self {
            case let .ethereum (ewm, core):
                return Address (core: cryptoAddressCreateAsETH (ewmTransferGetSource (ewm, core)))
            case let .bitcoin (wid, tid):
                let inputs = [BRTxInput](UnsafeBufferPointer(start: tid.pointee.inputs, count: tid.pointee.inCount))
                let inputsContain = (sent ? 1 : 0)
                return inputs
                    // If we sent the transaction then we expect our wallet to include one or more inputs.
                    // But if we didn't send it, then the inputs will be the sender's inputs.
                    .first { inputsContain == BRWalletContainsAddress (wid, UnsafeRawPointer([$0.address]).assumingMemoryBound(to: CChar.self)) }
                    .map { Address.createAsBTC (BRAddress (s: $0.address))}
            }
        }

        internal func target (sent: Bool) -> Address? {
            switch self {
            case let .ethereum (ewm, core):
                return Address (core: cryptoAddressCreateAsETH (ewmTransferGetTarget (ewm, core)))
            case let .bitcoin (wid, tid):
                // The target address is in a TxOutput; if not sent is it out address, otherwise anothers
                let outputs = [BRTxOutput](UnsafeBufferPointer(start: tid.pointee.outputs, count: tid.pointee.outCount))
                let outputsContain = (!sent ? 1 : 0)
                return outputs
                    // If we did not send the transaction then we expect our wallet to include one or more
                    // outputs.  But if we did send it, then the outpus witll be the targets outputs.
                    .first { outputsContain == BRWalletContainsAddress(wid, UnsafeRawPointer([$0.address]).assumingMemoryBound(to: CChar.self)) }
                    .map { Address.createAsBTC (BRAddress (s: $0.address)) }
            }
        }

        internal func amount (in unit: Unit) -> Amount {
            switch self {
            case let .ethereum (ewm, core):
                let amount: BREthereumAmount = ewmTransferGetAmount (ewm, core)
                let value = (AMOUNT_ETHER == amount.type
                    ? amount.u.ether.valueInWEI
                    : amount.u.tokenQuantity.valueAsInteger)

                return Amount (core: cryptoAmountCreate (unit.currency.core, CRYPTO_FALSE, value),
                               unit: unit)

            case let .bitcoin (wid, tid):
                var fees = UInt64(BRWalletFeeForTx (wid, tid))
                if (fees == UINT64_MAX) { fees = 0 }

                let recv = Int64(BRWalletAmountReceivedFromTx (wid, tid))
                let send = Int64(BRWalletAmountSentByTx (wid, tid))   // includes fees

                // The value is always positive; it is the value sent from source to target.
                let value = (0 == fees
                    ? recv - send
                    : (send - Int64(fees)) - recv)

                return Amount.createAsBTC (UInt64(value), unit)
            }

        }

        internal func fee (in unit: Unit) -> Amount {
            switch self {
            case let .ethereum (ewm, core):
                var overflow: Int32 = 0;
                let amount: BREthereumEther = ewmTransferGetFee (ewm, core, &overflow)
                precondition (0 == overflow)

                return Amount.createAsETH (amount.valueInWEI, unit)
            case let .bitcoin (wid, tid):
                //        var transaction = core
                let fee = BRWalletFeeForTx (wid, tid)
                return Amount.createAsBTC (fee, unit)
            }
        }

        internal var isSent: Bool {
            switch self {
            case .ethereum: // (ewm, core):
                return true

            case let .bitcoin (wid, tid):
                // Returns a 'fee' if 'all inputs are from wallet' (meaning, the bitcoin transaction is
                // composed of UTXOs from wallet)
                let fee = BRWalletFeeForTx (wid, tid)
                return fee != UINT64_MAX // && fee != 0
            }
        }
    }
}
