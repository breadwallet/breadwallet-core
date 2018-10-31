//
//  BRCore.swift
//  BRCoreX
//
//  Created by Ed Gamble on 11/5/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import Foundation
import Core
import Core.Ethereum

typealias BRBitcoinWallet = OpaquePointer
typealias BRBitcoinTransaction = BRTransaction

///
/// MARK: - Core Amount
///
public struct CoreAmount: Amount {
    enum Core {
        case bitcoin(UInt64)
        case ethereum(UInt256)
    }
    private var core: Core

    public var unit: Unit
    public private(set) var sign: Int

    public func add(_ that: Amount) -> Amount? {
        guard let that = that as? CoreAmount else { return nil }
        switch (self.core, that.core) {
        default: return nil
        }
    }

    public func sub(_ that: Amount) -> Amount? {
        <#code#>
    }

    public func scale(by value: Double, unit: Unit) -> Amount? {
        <#code#>
    }

    public func equals(_ that: Amount) -> Bool {
        guard let that = that as? CoreAmount else { return false }

    }

    public func lessThan(_ that: Amount) -> Bool {
        guard let that = that as? CoreAmount else { return false }

    }

    public init<T>(value: T, unit: Unit) where T : SignedInteger {
        <#code#>
    }

    public func double(in unit: Unit) -> Double? {
        if self.unit === unit {
        }
    }

}

///
/// MARK: - Core CurrencyPair
///
public struct CoreCurrencyPair: CurrencyPair {
    public var baseCurrency: Unit

    public var quoteCurrency: Unit

    public var exchangeRate: Double

    public func exchange(asBase amount: Amount) -> Amount {
        assert (amount.unit.isCompatible(baseCurrency))
        // convert asBase to baseCurrency
        let amountInBase = amount
        return amountInBase.scale (by: exchangeRate, unit: quoteCurrency)!
    }

    public func exchange(asQuote amount: Amount) -> Amount {
        assert (amount.unit.isCompatible(quoteCurrency))
        let amountInQuote = amount
        return amountInQuote.scale (by: 1/exchangeRate, unit: baseCurrency)!
    }
}

///
/// MARK: - Core Transfer
///
public class CoreTransfer : Transfer {
    enum Core {
        case bitcoin (transaction: BRBitcoinTransaction)
        case ethereum (transfer: BREthereumTransferId)
        // case ...
    }

    private var core: Core

    public unowned let wallet: Wallet

    public var source: Address? {
        switch core {
        case .bitcoin(let transaction):
            return /* ... */ nil
        case .ethereum (let transfer):
            return nil
        }
    }

    public var target: Address? {
        return nil
    }

    public var amount: Amount {
        switch core {

        case .bitcoin(let transaction):
            break

        case .ethereum(let transfer):
            let ewm: BREthereumEWM? = nil
            ethereumTransferGetAmountEther (ewm, transfer, WEI)
        }
        // ...
    }

    public var fee: Amount {
        if let confirmation = confirmation {
            // get actual fee
        }
        else {
            // get estimated fee from fee basis
        }
    }

    public let feeBasis: TransferFeeBasis

    public private(set) var confirmation: TransferConfirmation?
    public private(set) var hash: TransferHash?
    public private(set) var state: TransferState


    deinit {
        // Release Core resources
    }

    internal init (wallet: Wallet,
                  source: Address,
                  target: Address,
                  amount: Amount,
                  feeBasis: TransferFeeBasis) {

        self.wallet = wallet
        self.feeBasis = feeBasis

        switch amount.currency {
        case .bitcoin:
            self.core = Core.bitcoin(transaction: nil)
        case .bitcash:
              break
        case .ethereum:
            self.core = Core.ethereum(transfer: nil)
        case .token(let code, let symbol, let name, let description, let unit):
            break
        }
    }
}

public class CoreTransferFactory : TransferFactory {
    public func createTransfer(wallet: Wallet,
                               source: Address,
                               target: Address,
                               amount: Amount,
                               feeBasis: TransferFeeBasis) -> Transfer {
        return CoreTransfer (wallet: wallet,
                             source: source,
                             target: target,
                             amount: amount,
                             feeBasis: feeBasis)
    }
}

///
/// MARK: - Core Wallet
///
public class CoreWallet : Wallet {
    enum Core {
        case bitcoin (BRBitcoinWallet)
        case ethereum (wallet: BREthereumWalletId)
    }

    private var core : Core

    public unowned let manager: WalletManager

    public let name: String

    public var balance: Amount {
        switch core {
        case .bitcoin (let wallet):
            return
        case .ethereum(let wallet):
            return
        }
    }

    public var transfers: [Transfer] {
        return []
    }

    public private(set) var state: WalletState

    public var defaultFeeBasis: TransferFeeBasis

    public var transferFactory: TransferFactory = CoreTransferFactory()

    deinit {
    }

    internal init (manager: WalletManager,
                   currency: Currency) {
        self.manager = manager
        self.name = "Wallet-of-\(currency.name)"

        // balance

        // state
        // defaultFeeBasis

        // Core
    }
}

public class CoreDefaultWalletFactory : WalletFactory {
    public func createWallet(manager: WalletManager,
                             currency: Currency) -> Wallet {
        return CoreWallet (manager: manager,
                           currency: currency)
    }
}

///
/// MARK: Core Wallet Manager
///
public class CoreWalletManager : WalletManager {
    public unowned var client: WalletManagerClient

    public let account: Account

    public let network: Network

    /// The primaryWallet's currency is determined from the network
    public private(set) lazy var primaryWallet: Wallet = {
        return walletFactory.createWallet (manager: self,
                                           currency: network.currency)
    }()

    public private(set) lazy var wallets: [Wallet] = {
        return [primaryWallet]
    }()

    public private(set) var mode: WalletManagerMode

    public let path: String

    public private(set) var state: WalletManagerState

    public var walletFactory: WalletFactory = CoreDefaultWalletFactory()

    enum Core {
        case bitcoin (bwm: BRBitcoinBWM)
        case bitcash
        case ethereum (ewm: BREthereumEWM)
    }

    private lazy var core: Core = {
        switch network {
        case .bitcoin(let main, let name):
            return Core.bitcoin(bwm: "BRBitcoinBWM")

        case .bitcash(let main, let name):
            return Core.bitcash

        case .ethereum(let main, let name, let identifier):
            let coreNetwork   = (main ? ethereumMainnet : ethereumTestnet)
            let coreTimestamp = UInt64(ETHEREUM_TIMESTAMP_UNKNOWN)

            let peers:        Dictionary<String,String> = [:]
            let blocks:       Dictionary<String,String> = [:]
            let transactions: Dictionary<String,String> = [:]
            let logs:         Dictionary<String,String> = [:]

            return Core.ethereum (ewm: ethereumCreate (coreNetwork,
                                   "ginger ...",
                                   coreTimestamp,
                                   coreMode,
                                   coreEthereumClient,
                                   CoreWalletManager.asPairs(peers),
                                   CoreWalletManager.asPairs(blocks),
                                   CoreWalletManager.asPairs(transactions),
                                   CoreWalletManager.asPairs(logs)))
        }
    }()

    deinit {
        switch core {
        case .bitcoin(_):
            break
        case .bitcash:
            break
        case .ethereum(let ewm):
            ethereumDestroy (ewm)
        }
    }

    public func connect () {
        switch core {
        case .bitcoin:
            break
        case .bitcash:
            break
        case .ethereum(let ewm):
            ethereumConnect (ewm)
        }
    }

    public func disconnect() {
        switch core {
        case .bitcoin:
            break
        case .bitcash:
            break
        case .ethereum(let ewm):
            ethereumDisconnect (ewm)
        }
    }

    init (client: WalletManagerClient,
          account: Account,
          network: Network,
          mode: WalletManagerMode,
          path: String) {
        self.client = client
        self.account = account
        self.network = network
        self.mode = mode
        self.path = path

        CoreWalletManager.managers.append(Weak(value: self))
    }

    ///
    /// Private
    ///
    private var coreMode: BREthereumMode {
        switch mode {
        case .api_only: return BRD_ONLY
        case .api_with_p2p_submit: return BRD_WITH_P2P_SEND
        case .p2p_with_api_sync: return P2P_WITH_BRD_SYNC
        case .p2p_only: return P2P_ONLY
        }
    }

    typealias BRBitcoinClient = String
    typealias BRBitcoinBWM = String

    private lazy var coreBitcoinClient: BRBitcoinClient = {
        return "BitcoinClient"
    }()

    /// All known managers
    private static var managers : [Weak<CoreWalletManager>] = []

    private static func lookup (core: BREthereumEWM?) -> CoreWalletManager? {
        guard let core = core else { return nil }
        return managers
            .filter { nil != $0.value }         // remove weak references
            .map { $0.value! }
            .first {
                if case let .ethereum(ewm) = $0.core, ewm == core {
                    return true
                }
                else { return false }
        }
    }

    private lazy var coreEthereumClient: BREthereumClient = {
        typealias AnyEthereumClient = CoreWalletManager
        typealias EthereumWalletManager = CoreWalletManager

        let client = self
        return BREthereumClient (
            context: UnsafeMutableRawPointer (Unmanaged<AnyEthereumClient>.passRetained(client).toOpaque()),

            funcGetBalance: { (coreClient, coreEWM, wid, address, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.getBalance(ewm: ewm,
//                                      wid: wid,
//                                      address: asUTF8String(address!),
//                                      rid: rid)
                }},

            funcGetGasPrice: { (coreClient, coreEWM, wid, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    assert (ewm.client === client.base)
//                    client.getGasPrice (ewm: ewm,
//                                        wid: wid,
//                                        rid: rid)
                }},

            funcEstimateGas: { (coreClient, coreEWM, wid, tid, to, amount, data, rid)  in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.getGasEstimate(ewm: ewm,
//                                          wid: wid,
//                                          tid: tid,
//                                          to: asUTF8String(to!),
//                                          amount: asUTF8String(amount!),
//                                          data: asUTF8String(data!),
//                                          rid: rid)
                }},

            funcSubmitTransaction: { (coreClient, coreEWM, wid, tid, transaction, rid)  in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.submitTransaction(ewm: ewm,
//                                             wid: wid,
//                                             tid: tid,
//                                             rawTransaction: asUTF8String(transaction!),
//                                             rid: rid)
                }},

            funcGetTransactions: { (coreClient, coreEWM, address, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.getTransactions(ewm: ewm,
//                                           address: asUTF8String(address!),
//                                           rid: rid)
                }},

            funcGetLogs: { (coreClient, coreEWM, contract, address, event, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.getLogs (ewm: ewm,
//                                    address: asUTF8String(address!),
//                                    event: asUTF8String(event!),
//                                    rid: rid)
                }},

            funcGetBlocks: { (coreClient, coreEWM, address, interests, blockStart, blockStop, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.getBlocks (ewm: ewm,
//                                      address: asUTF8String (address!),
//                                      interests: interests,
//                                      blockStart: blockStart,
//                                      blockStop: blockStop,
//                                      rid: rid)
                }},

            funcGetTokens: { (coreClient, coreEWM, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.getTokens (ewm: ewm, rid: rid)
                }},

            funcGetBlockNumber: { (coreClient, coreEWM, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.getBlockNumber(ewm: ewm, rid: rid)
                }},

            funcGetNonce: { (coreClient, coreEWM, address, rid) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.getNonce(ewm: ewm,
//                                    address: asUTF8String(address!),
//                                    rid: rid)
                }},

            funcSaveNodes: { (coreClient, coreEWM, data) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.saveNodes(ewm: ewm, data: EthereumWalletManager.asDictionary(data!))
                }},

            funcSaveBlocks: { (coreClient, coreEWM, data) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.saveBlocks(ewm: ewm, data: EthereumWalletManager.asDictionary(data!))
                }},

            funcChangeTransaction: { (coreClient, coreEWM, change, data) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//
//                    let cStrHash = ethereumHashDataPairGetHash (data)!
//                    let cStrData = ethereumHashDataPairGetData (data)!
//
//                    client.changeTransaction (ewm: ewm,
//                                              change: EthereumClientChangeType(change),
//                                              hash: String (cString: cStrHash),
//                                              data: String (cString: cStrData))
//
//                    free (cStrHash); free (cStrData)
                }},

            funcChangeLog: { (coreClient, coreEWM, change, data) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//
//                    let cStrHash = ethereumHashDataPairGetHash (data)!
//                    let cStrData = ethereumHashDataPairGetData (data)!
//
//                    client.changeLog (ewm: ewm,
//                                      change: EthereumClientChangeType(change),
//                                      hash: String (cString: cStrHash),
//                                      data: String (cString: cStrData))
//
//                    free (cStrHash); free (cStrData)
                }},

            funcEWMEvent: { (coreClient, coreEWM, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.handleEWMEvent (ewm: ewm, event: EthereumEWMEvent (event))
                }},

            funcPeerEvent: { (coreClient, coreEWM, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.handlePeerEvent (ewm: ewm, event: EthereumPeerEvent (event))
                }},

            funcWalletEvent: { (coreClient, coreEWM, wid, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {

                    let event = WalletManagerEvent.created
                    ewm.client.handleManagerEvent(manager: ewm,
                                                  event: event)
//                    client.handleWalletEvent(ewm: ewm,
//                                             wallet: ewm.findWallet(identifier: wid),
//                                             event: EthereumWalletEvent (event))
                }},

            funcBlockEvent: { (coreClient, coreEWM, bid, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.handleBlockEvent(ewm: ewm,
//                                            block: ewm.findBlock(identifier: bid),
//                                            event: EthereumBlockEvent (event))
                }},

            funcTransferEvent: { (coreClient, coreEWM, wid, tid, event, status, message) in
                if let client = coreClient.map ({ Unmanaged<AnyEthereumClient>.fromOpaque($0).takeUnretainedValue() }),
                    let ewm = EthereumWalletManager.lookup(core: coreEWM) {
//                    client.handleTransferEvent(ewm: ewm,
//                                               wallet: ewm.findWallet(identifier: wid),
//                                               transfer: ewm.findTransfers(identifier: tid),
//                                               event: EthereumTransferEvent(event))
                }})
    }()

    ///
    /// Private Support
    ///

    private static func asPairs (_ set: Dictionary<String,String>) -> OpaquePointer {
        let pairs = ethereumHashDataPairSetCreate()!
        set.forEach { (hash: String, data: String) in
            ethereumHashDataPairAdd (pairs, hash, data)
        }
        return pairs
    }

    private static func asDictionary (_ set: OpaquePointer) -> Dictionary<String,String> {
        var dict : [String:String] = [:]

        var pair : BREthereumHashDataPair? = nil
        while let p = OpaquePointer.init (BRSetIterate (set, &pair)) {
            let cStrHash = ethereumHashDataPairGetHash (p)!
            let cStrData = ethereumHashDataPairGetData (p)!

            dict [String (cString: cStrHash)] = String (cString: cStrData)

            free (cStrHash); free (cStrData)

            pair = p
        }

        return dict
    }
}

