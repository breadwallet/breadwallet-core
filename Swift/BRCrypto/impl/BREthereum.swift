//
//  BREthereum.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//

import BRCryptoC
import BRCore.Ethereum

extension Amount {
    fileprivate var asETH: UInt64 {
        var overflow: BRCryptoBoolean = CRYPTO_FALSE
        let value = cryptoAmountGetIntegerRaw (self.core, &overflow)
        precondition(CRYPTO_FALSE == overflow)
        return value
    }

    static fileprivate func createAsETH (_ value: UInt256, _ unit: Unit) -> Amount {
        return Amount (core: cryptoAmountCreate(unit.currency.core, CRYPTO_FALSE, value),
                       unit: unit);
    }
}

extension Address {
    static fileprivate func createAsETH (_ eth: BREthereumAddress) -> Address  {
        return Address (core: cryptoAddressCreateAsETH (eth))
    }
}

extension Network {
    fileprivate var asETH: BREthereumNetwork! {
        switch self.impl {
        case let .ethereum(_, network): return network
        default: precondition(false); return nil
        }
    }
}

extension Account {
    fileprivate var asETH: BREthereumAccount {
        return cryptoAccountAsETH (self.core)
    }
}

///
/// An ERC20 Smart Contract Token
///
public class EthereumToken {

    /// A reference to the Core's BREthereumToken
    internal let identifier: BREthereumToken

    /// The currency
    public let currency: Currency

    /// The address of the token's ERC20 Smart Contract
    public let address: Address

    internal init (identifier: BREthereumToken,
                   currency: Currency) {
        self.identifier = identifier
        self.address = Address (core: cryptoAddressCreateAsETH(tokenGetAddressRaw(identifier)))
        self.currency = currency
    }
}

///
/// An EthereumtokenEvent represents a asynchronous announcment of a token's state change.
///
public enum EthereumTokenEvent {
    case created
    case deleted
}

extension EthereumTokenEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case .created: return "Created"
        case .deleted: return "Deleted"
        }
    }
}

///
///
///

class EthereumTransfer: Transfer {
    internal let identifier: BREthereumTransfer
    internal var core: BREthereumEWM {
        return _wallet._manager.core!
    }

    /// The EthereumWallet owning this transfer
    public unowned let _wallet: EthereumWallet

    public var wallet: Wallet {
        return _wallet
    }

    internal let unit: Unit

    public private(set) lazy var source: Address? = {
        return Address (core: cryptoAddressCreateAsETH (ewmTransferGetSource (self.core, self.identifier)))
    }()

    public private(set) lazy var target: Address? = {
        return Address (core: cryptoAddressCreateAsETH (ewmTransferGetTarget (self.core, self.identifier)))
    }()

    public private(set) lazy var amount: Amount = {
        let amount: BREthereumAmount = ewmTransferGetAmount (self.core, self.identifier)
        let value = (AMOUNT_ETHER == amount.type
            ? amount.u.ether.valueInWEI
            : amount.u.tokenQuantity.valueAsInteger)

        return Amount (core: cryptoAmountCreate (unit.currency.core, CRYPTO_FALSE, value),
                       unit: unit)
    } ()

    public private(set) lazy var fee: Amount = {
        var overflow: Int32 = 0;
        let amount: BREthereumEther = ewmTransferGetFee (self.core, self.identifier, &overflow)
        precondition (0 == overflow)

        let unit = wallet.manager.network.baseUnitFor(currency: wallet.manager.currency)!
        return Amount.createAsETH (amount.valueInWEI, unit)
    }()

    var feeBasis: TransferFeeBasis

    public private(set) var hash: TransferHash? = nil

    public private(set) var state: TransferState

    public let isSent: Bool

    internal init (wallet: EthereumWallet,
                   unit: Unit,
                   tid: BREthereumTransfer) {
        self._wallet = wallet
        self.identifier = tid
        self.unit = unit
        self.state = TransferState.created

        let gasUnit = wallet.manager.network.baseUnitFor(currency: wallet.manager.currency)!

        let gasPrice = Amount.createAsETH(createUInt256 (0), gasUnit)
        self.feeBasis = TransferFeeBasis.ethereum(gasPrice: gasPrice, gasLimit: 0)
        self.isSent = true
    }
}

///
///
///
class EthereumWallet: Wallet {
    internal let identifier: BREthereumWallet
    private var core: BREthereumEWM {
        return _manager.core!
    }

    public unowned let _manager: EthereumWalletManager

    public var manager: WalletManager {
        return _manager
    }

    public let name: String

    public let unit: Unit

    public var balance: Amount {
        let coreETH = ewmWalletGetBalance (self.core, self.identifier)
        let value = (coreETH.type == AMOUNT_ETHER
            ? coreETH.u.ether.valueInWEI
            : coreETH.u.tokenQuantity.valueAsInteger)
        return Amount.createAsETH (value, unit)
    }

    public private(set) var transfers: [Transfer] = []

    public func lookup (transfer: TransferHash) -> Transfer? {
        return nil
    }

    public internal(set) var state: WalletState

    public internal(set) var defaultFeeBasis: TransferFeeBasis

//    public var transferFactory: TransferFactory

    public let target: Address
    public let source: Address

    internal init (manager: EthereumWalletManager,
                   unit: Unit,
                   wid: BREthereumWallet) {
        self._manager = manager
        self.identifier = wid
        self.name = unit.currency.code
        self.unit = unit

        self.state = WalletState.created

        self.target = Address.createAsETH (accountGetPrimaryAddress (manager.account.asETH))
        self.source = self.target

        let gasUnit  = manager.network.baseUnitFor(currency: manager.currency)!
        let gasPrice = Amount.createAsETH(createUInt256 (0), gasUnit)
        self.defaultFeeBasis = TransferFeeBasis.ethereum(gasPrice: gasPrice, gasLimit: 0)

    }
}

///
///
///
class EthereumWalletManager: WalletManager {
    internal var core: BREthereumEWM! = nil

    public let account: Account
    public var network: Network

    internal lazy var unit: Unit = {
        return network.defaultUnitFor(currency: network.currency)!
    }()

    public lazy var primaryWallet: Wallet = {
        return EthereumWallet (manager: self,
                               unit: unit,
                               wid: ewmGetWallet(self.core))
    }()

    public lazy var wallets: [Wallet] = {
        return [primaryWallet]
    } ()


    public func createWalletFor (currency: Currency) -> Wallet? {
        guard let unit = network.defaultUnitFor (currency: currency) else { return nil }

        let core = ewmGetWallet(self.core)! // holding token

        let wallet =  EthereumWallet (manager: self,
                                      unit: unit,
                                      wid: core)
        wallets.append(wallet)
        return wallet
    }

//    internal func addWallet (identifier: BREthereumWallet) {
//        guard case .none = findWallet(identifier: identifier) else { return }
//
//        if let tokenId = ewmWalletGetToken (core, identifier) {
//            guard let token = findToken (identifier: tokenId),
//                let unit = network.defaultUnitFor(currency: token.currency)
//                else { precondition(false); return }
//
//            wallets.append (EthereumWallet (manager: self,
//                                            unit: unit,
//                                            wid: identifier))
//        }
//    }
//
//    internal func findWallet (identifier: BREthereumWallet) -> EthereumWallet? {
//        return wallets.first { identifier == ($0 as! EthereumWallet).identifier } as? EthereumWallet
//    }

    public var mode: WalletManagerMode

    public var path: String

    public var state: WalletManagerState

//    public var walletFactory: WalletFactory = EthereumWalletFactory()

    internal let query: BlockChainDB

    public init (//listener: EthereumListener,
                 account: Account,
                 network: Network,
                 mode: WalletManagerMode,
                 storagePath: String) {

        self.account = account
        self.network = network
        self.mode    = mode
        self.path    = storagePath
        self.state   = WalletManagerState.created
        self.query   = BlockChainDB()

        self.core = ewmCreate (network.asETH,
                               account.asETH,
                               UInt64(account.timestamp),
                               EthereumWalletManager.coreMode (mode),
                               coreEthereumClient,
                               storagePath)

//        EthereumWalletManager.managers.append(Weak (value: self))
//        self.listener.handleManagerEvent(manager: self, event: WalletManagerEvent.created)

    }

    public func connect() {
        ewmConnect (self.core)
    }

    public func disconnect() {
        ewmDisconnect (self.core)
    }

    public func sign (transfer: Transfer, paperKey: String) {
        guard let wallet = primaryWallet as? EthereumWallet,
            let transfer = transfer as? EthereumTransfer else { precondition(false); return }
        ewmWalletSignTransferWithPaperKey(core, wallet.identifier, transfer.identifier, paperKey)
    }

    public func submit (transfer: Transfer) {
        guard let wallet = primaryWallet as? EthereumWallet,
            let transfer = transfer as? EthereumTransfer else { precondition(false); return }
        ewmWalletSubmitTransfer(core, wallet.identifier, transfer.identifier)
    }

    public func sync() {
        ewmSync (core);
    }

    // Actually a Set/Dictionary by {Symbol}
    public private(set) var all: [EthereumToken] = []

    internal func addToken (identifier: BREthereumToken) {
        let symbol = asUTF8String (tokenGetSymbol (identifier))
        if let currency = network.currencyBy (code: symbol) {
            let token = EthereumToken (identifier: identifier, currency: currency)
            all.append (token)
//            self._listener.handleTokenEvent(manager: self, token: token, event: EthereumTokenEvent.created)
        }
    }

    internal func remToken (identifier: BREthereumToken) {
        if let index = all.firstIndex (where: { $0.identifier == identifier}) {
//            let token = all[index]
            all.remove(at: index)
//            self._listener.handleTokenEvent(manager: self, token: token, event: EthereumTokenEvent.deleted)
        }
    }

    internal func findToken (identifier: BREthereumToken) -> EthereumToken? {
        return all.first { $0.identifier == identifier }
    }

    private static func coreMode (_ mode: WalletManagerMode) -> BREthereumMode {
        switch mode {
        case .api_only: return BRD_ONLY
        case .api_with_p2p_submit: return BRD_WITH_P2P_SEND
        case .p2p_with_api_sync: return P2P_WITH_BRD_SYNC
        case .p2p_only: return P2P_ONLY
        }
    }

    private lazy var coreEthereumClient: BREthereumClient = {
        let this = self
        return BREthereumClient (
            // Unmanaged<System>.passRetained(self).toOpaque(),
            // Unmanaged<System>.fromOpaque(context!).takeRetainedValue()
            context: Unmanaged<EthereumWalletManager>.passUnretained(this).toOpaque(),

            funcGetBalance: { (context, coreEWM, wid, address, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getBalanceAsETH (ewm: this.core,
                                            wid: wid!,
                                            address: address,
                                            rid: rid) { (wid, balance, rid) in
                                                ewmAnnounceWalletBalance (this.core, wid, balance, rid)
                }},

            funcGetGasPrice: { (context, coreEWM, wid, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                this.query.getGasPriceAsETH (ewm: this.core,
                                             wid: wid!,
                                             rid: rid) { (wid, gasPrice, rid) in
                                                ewmAnnounceGasPrice (this.core, wid, gasPrice, rid)
                }},

            funcEstimateGas: { (context, coreEWM, wid, tid, from, to, amount, data, rid)  in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                    let from = asUTF8String(from!)
                    let to = asUTF8String(to!)
                    let amount = asUTF8String(amount!)
                    let data = asUTF8String(data!)
                this.query.getGasEstimateAsETH (ewm: this.core,
                                                wid: wid!,
                                                tid: tid!,
                                                from: from,
                                                to: to,
                                                amount: amount,
                                                data: data,
                                                rid: rid) { (wid, tid, gasEstimate, rid) in
                                                    ewmAnnounceGasEstimate (this.core, wid, tid, gasEstimate, rid)

                }},

            funcSubmitTransaction: { (context, coreEWM, wid, tid, transaction, rid)  in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                    let transaction = asUTF8String (transaction!)
                this.query.submitTransactionAsETH (ewm: this.core,
                                                   wid: wid!,
                                                   tid: tid!,
                                                   transaction: transaction,
                                                   rid: rid) { (wid, tid, hash, errorCode, errorMessage, rid) in
                                                    ewmAnnounceSubmitTransfer (this.core,
                                                                               wid,
                                                                               tid,
                                                                               hash,
                                                                               errorCode,
                                                                               errorMessage,
                                                                               rid)
                }},

            funcGetTransactions: { (context, coreEWM, address, begBlockNumber, endBlockNumber, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getTransactionsAsETH (ewm: this.core,
                                                 address: address,
                                                 begBlockNumber: begBlockNumber,
                                                 endBlockNumber: endBlockNumber,
                                                 rid: rid,
                                                 done: { (success: Bool, rid: Int32) in
                                                    ewmAnnounceTransactionComplete (this.core,
                                                                                    rid,
                                                                                    (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                },
                                                 each: { (res: BlockChainDB.ETH.Transaction) in
                                                    ewmAnnounceTransaction (this.core,
                                                                            res.rid,
                                                                            res.hash,
                                                                            res.sourceAddr,
                                                                            res.targetAddr,
                                                                            res.contractAddr,
                                                                            res.amount,
                                                                            res.gasLimit,
                                                                            res.gasPrice,
                                                                            res.data,
                                                                            res.nonce,
                                                                            res.gasUsed,
                                                                            res.blockNumber,
                                                                            res.blockHash,
                                                                            res.blockConfirmations,
                                                                            res.blockTransactionIndex,
                                                                            res.blockTimestamp,
                                                                            res.isError)
                })},

            funcGetLogs: { (context, coreEWM, contract, address, event, begBlockNumber, endBlockNumber, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getLogsAsETH (ewm: this.core,
                                         address: address,
                                         begBlockNumber: begBlockNumber,
                                         endBlockNumber: endBlockNumber,
                                         rid: rid,
                                         done: { (success: Bool, rid: Int32) in
                                            ewmAnnounceLogComplete (this.core,
                                                                    rid,
                                                                    (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                },
                                         each: { (res: BlockChainDB.ETH.Log) in
                                            var cTopics = res.topics.map { UnsafePointer<Int8>(strdup($0)) }
                                            defer {
                                                cTopics.forEach { free (UnsafeMutablePointer(mutating: $0)) }
                                            }

                                            ewmAnnounceLog (this.core,
                                                            res.rid,
                                                            res.hash,
                                                            res.contract,
                                                            Int32(res.topics.count),
                                                            &cTopics,
                                                            res.data,
                                                            res.gasPrice,
                                                            res.gasUsed,
                                                            res.logIndex,
                                                            res.blockNumber,
                                                            res.blockTransactionIndex,
                                                            res.blockTimestamp)
                })},

            funcGetBlocks: { (context, coreEWM, address, interests, blockStart, blockStop, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getBlocksAsETH (ewm: this.core,
                                           address: address,
                                           interests: interests,
                                           blockStart: blockStart,
                                           blockStop: blockStop,
                                           rid: rid) { (blocks, rid) in
                                            ewmAnnounceBlocks (this.core, rid,
                                                               Int32(blocks.count),
                                                               UnsafeMutablePointer<UInt64>(mutating: blocks))
                }},

            funcGetTokens: { (context, coreEWM, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                this.query.getTokensAsETH (ewm: this.core,
                                           rid: rid,
                                           done: { (success: Bool, rid: Int32) in
                                            ewmAnnounceTokenComplete (this.core,
                                                                      rid,
                                                                      (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                },
                                           each: { (res: BlockChainDB.ETH.Token) in
                                            ewmAnnounceToken (this.core,
                                                              res.rid,
                                                              res.address,
                                                              res.symbol,
                                                              res.name,
                                                              res.description,
                                                              res.decimals,
                                                              res.defaultGasLimit,
                                                              res.defaultGasPrice)
                })},
            
            funcGetBlockNumber: { (context, coreEWM, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                this.query.getBlockNumberAsETH (ewm: this.core,
                                                rid: rid) { (number, rid) in
                                                    ewmAnnounceBlockNumber (this.core, number, rid)
                }},

            funcGetNonce: { (context, coreEWM, address, rid) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
                let address = asUTF8String(address!)
                this.query.getNonceAsETH (ewm: this.core,
                                          address: address,
                                          rid: rid) { (address, nonce, rid) in
                                            ewmAnnounceNonce (this.core, address, nonce, rid)
                }},

            funcEWMEvent: { (context, coreEWM, event, status, message) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
//                    ewm.queue.async {
//                        ewm.listener.handleManagerEvent (manager: ewm,
//                                                         event: WalletManagerEvent(event))
//                    }
//                }},
        },
            funcPeerEvent: { (context, coreEWM, event, status, message) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
//                    ewm.queue.async {
//                        //                    ewm.listener.handlePeerEvent (ewm: ewm, event: EthereumPeerEvent (event))
//                    }
//                }},
        },
            funcWalletEvent: { (context, coreEWM, wid, event, status, message) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
 //               let ev = WalletEvent
                
                switch event {
                case WALLET_EVENT_CREATED:
                    break
                case WALLET_EVENT_BALANCE_UPDATED:
                    break
                case WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED:
                    break
                case WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED:
                    break
                case WALLET_EVENT_DELETED:
                    break
                default:
                    precondition (false)
                }
//                    ewm.queue.async {
//                        let event = WalletEvent (ewm: ewm, wid: wid!, event: event)
//                        if case .created = event,
//                            case .none = ewm.findWallet(identifier: wid!) {
//                            ewm.addWallet (identifier: wid!)
//                        }
//
//                        if let wallet = ewm.findWallet (identifier: wid!) {
//                            ewm.listener.handleWalletEvent(manager: ewm,
//                                                           wallet: wallet,
//                                                           event: event)
//                        }
//                    }
//                }},
        },
            funcTokenEvent: { (context, coreEWM, token, event) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
//                    ewm.queue.async {
//                        let event = EthereumTokenEvent (event)
//                        if case .created = event,
//                            case .none = ewm.findToken(identifier: token!) {
//                            ewm.addToken (identifier: token!)
//                        }
//
//                        if let token = ewm.findToken(identifier: token!) {
//                            ewm._listener.handleTokenEvent (manager: ewm,
//                                                            token: token,
//                                                            event: event)
//                        }
//                    }
//                }},
        },

            //            funcBlockEvent: { (context, coreEWM, bid, event, status, message) in
            //                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
            //                    //                    ewm.listener.handleBlockEvent(ewm: ewm,
            //                    //                                                 block: ewm.findBlock(identifier: bid),
            //                    //                                                 event: EthereumBlockEvent (event))
            //                }},

            funcTransferEvent: { (context, coreEWM, wid, tid, event, status, message) in
                let this = Unmanaged<EthereumWalletManager>.fromOpaque(context!).takeUnretainedValue()
//                    ewm.queue.async {
//                        if let wallet = ewm.findWallet(identifier: wid!) {
//                            // Create a transfer, if needed.
//                            if (TRANSFER_EVENT_CREATED == event) {
//                                if case .none = wallet.findTransfer(identifier: tid!) {
//                                    wallet.addTransfer (identifier: tid!)
//                                }
//                            }
//
//                            // Prepare a default transferEvent; we'll update this for `event`
//                            var transferEvent: TransferEvent?
//
//                            // Lookup the transfer
//                            if let transfer = wallet.findTransfer(identifier: tid!) {
//                                let oldTransferState = transfer.state
//                                var newTransferState = TransferState.created
//
//                                switch (event) {
//                                case TRANSFER_EVENT_CREATED:
//                                    transferEvent = TransferEvent.created
//                                    break
//
//                                // Transfer State
//                                case TRANSFER_EVENT_SIGNED:
//                                    newTransferState = TransferState.signed
//                                    break
//                                case TRANSFER_EVENT_SUBMITTED:
//                                    newTransferState = TransferState.submitted
//                                    break
//
//                                case TRANSFER_EVENT_INCLUDED:
//                                    let confirmation = TransferConfirmation.init(
//                                        blockNumber: 0,
//                                        transactionIndex: 0,
//                                        timestamp: 0,
//                                        fee: Amount (value: Int(0), unit: wallet.currency.baseUnit))
//                                    newTransferState = TransferState.included(confirmation: confirmation)
//                                    break
//
//                                case TRANSFER_EVENT_ERRORED:
//                                    newTransferState = TransferState.failed(reason: "foo")
//                                    break
//
//                                case TRANSFER_EVENT_GAS_ESTIMATE_UPDATED: break
//                                case TRANSFER_EVENT_BLOCK_CONFIRMATIONS_UPDATED: break
//
//                                case TRANSFER_EVENT_DELETED:
//                                    transferEvent = TransferEvent.deleted
//                                    break
//
//                                default:
//                                    break
//                                }
//
//                                transfer.state = newTransferState
//                                transferEvent = transferEvent ?? TransferEvent.changed(old: oldTransferState, new: newTransferState)
//
//                                // Announce updated transfer
//                                ewm.listener.handleTransferEvent(manager: ewm,
//                                                                 wallet: wallet,
//                                                                 transfer: transfer,
//                                                                 event: transferEvent!)
//                            }
//                        }
//                    }
//                }}
        })
    }()
}
