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


///
/// Implementaton (in Swift) of Transfer
///
/// A Transfer can be created in one of two ways - User initiated or Sync initiated.  When the
/// transfer is sync initiated the underlying C code will callback with the details and the handler
/// must construct a transfer.  For a User initiated we endeavor to use the same flow; therefore,
/// we create the specific type of C entity and wait for the callback to construct the Transfer
/// itself.
///
/// This class is defined recursively with `Wallet`.  The implementation is careful to ensure that
/// the constraints between Transfer and Wallet are maintained - particularly in the announcement
/// of Transfer.created and Wallet.transferAdded.
///
/// An alternate implementation in C would use BRCryptoTransfer (as `TransferImplC`)
///
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
        return impl.source
    }()

    public private(set) lazy var target: Address? = {
        return impl.target
    }()

    public private(set) lazy var amount: Amount = {
        return impl.amount (in: wallet.unit)
    } ()

    public private(set) lazy var amountDirected: Amount = {
        switch direction {
        case .sent: return amount.negate
        case .received: return amount
        case .recovered: return Amount.create(integer: 0, unit: unit)
        }
    }()

    public private(set) lazy var fee: Amount = {
        let unit = wallet.manager.network.defaultUnitFor (currency: wallet.manager.currency)!
        return impl.fee (in: unit)
    }()

    var feeBasis: TransferFeeBasis

    public var hash: TransferHash? {
        return impl.hash
    }

    public internal(set) var state: TransferState {
        didSet {
            let newValue = state
            announceEvent (TransferEvent.changed (old: oldValue,
                                                  new: newValue))
        }
    }

    public private(set) lazy var direction: TransferDirection = {
        return impl.direction
    }()

    internal init (listener: TransferListener?,
                   wallet: WalletImplS,
                   unit: Unit,
                   impl: Impl) {
        let feeUnit = wallet.manager.network.baseUnitFor(currency: wallet.manager.currency)!

        self.listener = listener
        self.wallet = wallet
        self.unit = unit
        self.state = TransferState.created
        self.impl = impl
        self.feeBasis = impl.feeBasis(in: feeUnit)

        wallet.add(transfer: self)

    }

    internal func announceEvent (_ event: TransferEvent) {
        self.listener?.handleTransferEvent (system: system,
                                            manager: manager,
                                            wallet: wallet,
                                            transfer: self,
                                            event: event)
    }

//    internal convenience init (listener: TransferListener?,
//                               wallet: WalletImplS,
//                               unit: Unit,
//                               eth: BREthereumTransfer) {
//        self.init (listener: listener,
//                   wallet: wallet,
//                   unit: unit,
//                   impl: Impl.ethereum (ewm:wallet.impl.ewm, core: eth))
//    }
//
//    internal convenience init (listener: TransferListener?,
//                               wallet: WalletImplS,
//                               unit: Unit,
//                               btc: BRCoreTransaction) {
//        self.init (listener: listener,
//                   wallet: wallet,
//                   unit: unit,
//                   impl: Impl.bitcoin (wid: wallet.impl.btc, tid: btc))
//    }


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

        internal func matches (_ that: Impl) -> Bool {
            switch (self, that) {
            case (let .bitcoin (wid1, tid1), let .bitcoin (wid2, tid2)):
                return wid1 == wid2 && tid1 == tid2
            case (let .ethereum (ewm1, c1), let .ethereum (ewm2, c2)):
                return ewm1 == ewm2 && c1 == c2
            default:
                return false
            }
        }
        
        internal var source: Address? {
            switch self {
            case let .ethereum (ewm, core):
                return Address (core: cryptoAddressCreateAsETH (ewmTransferGetSource (ewm, core)))
            case let .bitcoin (wid, tid):
                let sent = UINT64_MAX != BRWalletFeeForTx (wid, tid)

                let inputs = [BRTxInput](UnsafeBufferPointer(start: tid.pointee.inputs, count: tid.pointee.inCount))
                let inputsContain = (sent ? 1 : 0)
                return inputs
                    // If we sent the transaction then we expect our wallet to include one or more inputs.
                    // But if we didn't send it, then the inputs will be the sender's inputs.
                    .first { inputsContain == BRWalletContainsAddress (wid, UnsafeRawPointer([$0.address]).assumingMemoryBound(to: CChar.self)) }
                    .map { Address.createAsBTC (BRAddress (s: $0.address))}
            }
        }

        internal var target: Address? {
            switch self {
            case let .ethereum (ewm, core):
                return Address (core: cryptoAddressCreateAsETH (ewmTransferGetTarget (ewm, core)))
            case let .bitcoin (wid, tid):
                let sent = UINT64_MAX != BRWalletFeeForTx (wid, tid)

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
                return Amount.createAsBTC (fee == UINT64_MAX ? 0 : fee, unit)
            }
        }

        internal func feeBasis (in unit: Unit) -> TransferFeeBasis {
            switch self {
            case .ethereum:
                let gasPrice = Amount.createAsETH (createUInt256 (0), unit)
                return TransferFeeBasis.ethereum (gasPrice: gasPrice, gasLimit: 0)

            case .bitcoin:
                return TransferFeeBasis.bitcoin(feePerKB: 0)
            }
        }

        internal var direction: TransferDirection {
            switch self {
            case let .ethereum (ewm, core):
                let source  = ewmTransferGetSource (ewm, core)
                let target  = ewmTransferGetTarget (ewm, core)
                let address = accountGetPrimaryAddress (ewmGetAccount(ewm))

                switch (addressEqual (source, address), addressEqual (target, address)) {
                case (ETHEREUM_BOOLEAN_TRUE,  ETHEREUM_BOOLEAN_TRUE ): return .recovered
                case (ETHEREUM_BOOLEAN_TRUE,  ETHEREUM_BOOLEAN_FALSE): return .sent
                case (ETHEREUM_BOOLEAN_FALSE, ETHEREUM_BOOLEAN_TRUE ): return .received
                default: precondition(false)
                }

            case let .bitcoin (wid, tid):
                // Returns a 'fee' if 'all inputs are from wallet' (meaning, the bitcoin transaction is
                // composed of UTXOs from wallet). We paid a fee, we sent it.
                let fees = BRWalletFeeForTx (wid, tid)
                if fees != UINT64_MAX { return .sent }

                let recv = BRWalletAmountReceivedFromTx (wid, tid)
                let send = BRWalletAmountSentByTx (wid, tid)   // includes fees

                return send > 0 && (recv + fees) == send
                    ? .recovered
                    : .received
            }
        }

        internal var hash: TransferHash? {
            switch self {
            case let .ethereum (ewm, core):
                let coreHash = ewmTransferGetOriginatingTransactionHash (ewm, core)
                return ETHEREUM_BOOLEAN_TRUE == hashEqual(coreHash, hashCreateEmpty())
                    ? nil
                    : TransferHash.ethereum(coreHash)

            case let .bitcoin (_, tid):
                let coreHash = tid.pointee.txHash
                return 1 == UInt256IsZero(coreHash)
                    ? nil
                    : TransferHash.bitcoin(coreHash)
            }
        }
    }
}
