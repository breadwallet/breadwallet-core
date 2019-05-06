//
//  BRSystem.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import Foundation

import BRCryptoC
import BRCore
import BRCore.Ethereum


typealias BRCoreWallet = OpaquePointer
typealias BRCorePeerManager = OpaquePointer
typealias BRCoreWalletManager = BRWalletManager
typealias BRCoreTransaction = UnsafeMutablePointer<BRTransaction>

public final class SystemBase: System {
    /// The listener.  Gets all events for {Network, WalletManger, Wallet, Transfer}
    public private(set) weak var listener: SystemListener?

    /// The path for persistent storage
    public let path: String

    /// The 'blockchain DB' to use for BRD Server Assisted queries
    public let query: BlockChainDB

    /// The account
    public let account: Account

    /// A Dispatch Queue for 'race condition free' system changes.
    private let queue = DispatchQueue (label: "Crypto System")

    /// the networks, unsorted.
    public internal(set) var networks: [Network] = []

    ///
    /// Add `network` to `networks`
    ///
    /// - Parameter network: the network to add
    ///
    internal func add (network: Network) {
        networks.append (network)
        announceEvent (SystemEvent.networkAdded(network: network))
    }

    /// The system's Wallet Managers, unsorted
    public internal(set) var managers: [WalletManager] = [];

    ///
    /// Add `manager` to `managers`.  Will signal WalletManagerEvent.created and then
    /// SystemEvent.managerAdded is `manager` is added.
    ///
    /// - Parameter manager: the manager to add.
    ///
    internal func add (manager: WalletManager) {
        if !managers.contains(where: { $0 === manager }) {
            guard let manager = manager as? WalletManagerImplS
                else { precondition (false); return }

            managers.append (manager)
            manager.announceEvent (WalletManagerEvent.created)
            announceEvent (SystemEvent.managerAdded(manager: manager))

            // Hackily
            switch manager.currency.code {
            case Currency.codeAsETH:
                if let ewm = manager.impl.ewm {
                    ewmLock(ewm); defer { ewmUnlock(ewm) }
                    ewmUpdateTokens(ewm)
                }
                break
            default: break
            }
        }
    }

    internal func managerBy (impl: WalletManagerImplS.Impl) -> WalletManagerImplS? {
        return managers
            .first {
                ($0 as? WalletManagerImplS)?.impl.matches (impl) ?? false
            } as? WalletManagerImplS
    }

     public func createWalletManager (network: Network,
                                     mode: WalletManagerMode) {

        var manager: WalletManagerImplS! = nil

        switch network.currency.code {
        case Currency.codeAsBTC,
             Currency.codeAsBCH,
             Currency.codeAsETH:

            // Events will be generated here for {Wallet Manager Created, Wallet Created}.  The
            // events will be ignored because the array system.managers cannot possiby include
            // 'manager' - which must exist to properly dispatch the listener announcement.
            // Thus the events are lost.  We'll send them again.
            manager = WalletManagerImplS (system: self,
                                          listener: listener!,
                                          account: account,
                                          network: network,
                                          mode: mode,
                                          storagePath: path)

            if network.currency.code == Currency.codeAsETH {
                // think about adding tokens/currencies for ERC20, others.

                // For ETH: Add tokens based on the currencies
                // Note: the network holds the currencies but those currencies do not include the ETH
                // 'Smart Contract' address which must be provided in a token declarations.  The
                // address is part of the BlockChainDB 'currency' schema - but we don't have that.
                //
                //        network.currencies
                //            .filter { $0.type == "erc20" }
                //            .forEach {
                //                //                    let unit = network.defaultUnitFor(currency: $0)
                //                //                    let core: BREthereumToken! = nil // ewmToek
                //                //                    let token = EthereumToken (identifier: core, currency: $0)
                //        }
            }

        default:
            // Create a 'generic wallet manager' (maybe handled above)
            break
        }

        // Require a manager
        precondition (nil != manager)

        // Touch the primary wallet.  When the ETH or BTC wallet manager waw created, it likely
        // called-back WalletEvent.created for the primary wallet.  But, this manager was not held
        // by System's managers yet, so the event was ignored.  By touching the primaryWallet
        // here, we'll create a wallet.
        let _ = manager.primaryWallet
    }

    internal func announceEvent (_ event: SystemEvent) {
        listener?.handleSystemEvent (system: self,
                                     event: event)
    }

    /// The System instance - Systen is a singleton class.
    private static var instance: System?

    ///
    /// Create a system.  Must only be called once; otherwise a precondition failure occurs.
    ///
    /// - Parameters:
    ///   - listener: the system listener
    ///   - account: the account
    ///   - path: the path for persistent storage
    ///   - query: the query for BRD Server Assist
    ///
    /// - Returns: the system
    ///
    public static func create (listener: SystemListener,
                               account: Account,
                               path: String,
                               query: BlockChainDB) -> System {
        precondition (nil == instance)
        instance = SystemBase (listener: listener,
                               account: account,
                               path: path,
                               query: query)
        return instance!
    }

    private init (listener: SystemListener,
                   account: Account,
                   path: String,
                   query: BlockChainDB) {
        precondition (nil == SystemBase.instance)
        
        self.listener = listener
        self.account = account
        self.path = path
        self.query = query
        
        listener.handleSystemEvent (system: self, event: SystemEvent.created)
    }

    internal static func resetForTest () {
        instance = nil
    }
    
    /// Stop the system.  All managers are disconnected.
    public func stop () {
        managers.forEach { $0.disconnect() }
    }

    ///
    /// Start the system.  All manager are connected.  The first time this is called, the networks
    /// matching `networksNeeded` will be created.  As a network is created, its associated manager
    /// should be created too.
    ///
    /// - Parameter networksNeeded: Then needed networks
    ///
    public func start (networksNeeded: [String]) {
        if !networks.isEmpty {
            managers.forEach { $0.connect() }
            return
        }

        func currencyDenominationToBaseUnit (currency: Currency, model: BlockChainDB.Model.CurrencyDenomination) -> Unit {
            let uids = "\(currency.name)-\(model.code)"
            return Unit (currency: currency, uids: uids, name: model.name, symbol: model.symbol)
        }

        func currencyDenominationToUnit (currency: Currency, model: BlockChainDB.Model.CurrencyDenomination, base: Unit) -> Unit {
            let uids = "\(currency.name)-\(model.code)"
            return Unit (currency: currency, uids: uids, name: model.name, symbol: model.symbol, base: base, decimals: model.decimals)
        }

        // query blockchains
        self.query.getBlockchains { (blockchainResult: Result<[BlockChainDB.Model.Blockchain],BlockChainDB.QueryError>) in
            let blockChainModels = try! blockchainResult
                // On success, always merge `defaultBlockchains`
                .map { BlockChainDB.Model.defaultBlockchains.unionOf ($0) { $0.id }}
                // On error, use defaultBlockchains
                .recover { (error: BlockChainDB.QueryError) -> [BlockChainDB.Model.Blockchain] in
                    return BlockChainDB.Model.defaultBlockchains
                }.get()

            blockChainModels.filter { networksNeeded.contains($0.id) }
                .forEach { (blockchainModel: BlockChainDB.Model.Blockchain) in

                    // query currencies
                    self.query.getCurrencies (blockchainId: blockchainModel.id) { (currencyResult: Result<[BlockChainDB.Model.Currency],BlockChainDB.QueryError>) in
                        // Find applicable defaults by `blockchainID`
                        let defaults = BlockChainDB.Model.defaultCurrencies
                            .filter { $0.blockchainID == blockchainModel.id }

                        let currencyModels = try! currencyResult
                            // On success, always merge `defaultCurrencies`
                            .map { defaults.unionOf ($0) { $0.id }}
                            // On error, use `defaults`
                            .recover { (error: BlockChainDB.QueryError) -> [BlockChainDB.Model.Currency] in
                                return defaults
                            }.get()

                        var associations: [Currency : Network.Association] = [:]

                        // Update associations
                        currencyModels
                            // TODO: Only needed if getCurrencies returns the wrong stuff.
                            .filter { $0.blockchainID == blockchainModel.id }
                            .forEach { (currencyModel: BlockChainDB.Model.Currency) in
                                // TODO: Create the currency but don't create copies
                                let currency = Currency (uids: currencyModel.id,
                                                         name: currencyModel.name,
                                                         code: currencyModel.code,
                                                         type: currencyModel.type)

                                // Create the base unit
                                let baseUnit = currencyModel.demoninations.first { 0 == $0.decimals}
                                    .map { currencyDenominationToBaseUnit(currency: currency, model: $0) }!

                                // Create the other units
                                var units: [Unit] = [baseUnit]
                                units += currencyModel.demoninations.filter { 0 != $0.decimals }
                                    .map { currencyDenominationToUnit (currency: currency, model: $0, base: baseUnit) }

                                // Find the default unit
                                let maximumDecimals = units.reduce (0) { max ($0, $1.decimals) }
                                let defaultUnit = units.first { $0.decimals == maximumDecimals }!

                                // Update associations
                                associations[currency] = Network.Association (baseUnit: baseUnit,
                                                                              defaultUnit: defaultUnit,
                                                                              units: Set<Unit>(units))
                        }

                        // the default currency
                        let currency = associations.keys.first { $0.code == blockchainModel.currency.lowercased() }!

                        // define the network
                        let network = Network (uids: blockchainModel.id,
                                               name: blockchainModel.name,
                                               isMainnet: blockchainModel.isMainnet,
                                               currency: currency,
                                               height: blockchainModel.blockHeight,
                                               associations: associations)

                        // save the network
                        self.networks.append (network)

                        // Invoke callbacks.
                        self.listener?.handleNetworkEvent (system: self, network: network, event: NetworkEvent.created)
                        self.listener?.handleSystemEvent  (system: self, event: SystemEvent.networkAdded(network: network))
                    }
            }
        }
    }

    // Core Ethereum Client

    internal var coreEthereumClient: BREthereumClient {
        let this = self
        return BREthereumClient (
            context: Unmanaged<SystemBase>.passUnretained(this).toOpaque(),

            funcGetBalance: { (context, coreEWM, wid, address, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                // Non-NULL values from Core Ethereum
                guard let eid = coreEWM, let wid = wid
                    else { print ("SYS: ETH: GetBalance: Missed {eid, wid}"); return }

                if let manager = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)){

                    let address = asUTF8String(address!)
                    if nil == ewmWalletGetToken (eid, wid) {
                        manager.query.getBalanceAsETH (ewm: manager.impl.ewm,
                                                       wid: wid,
                                                       address: address,
                                                       rid: rid) { (wid, balance, rid) in
                                                        ewmAnnounceWalletBalance (manager.impl.ewm, wid, balance, rid)
                        }
                    }
                    else {
                        manager.query.getBalanceAsTOK (ewm: manager.impl.ewm,
                                                       wid: wid,
                                                       address: address,
                                                       rid: rid) { (wid, balance, rid) in
                                                        ewmAnnounceWalletBalance (manager.impl.ewm, wid, balance, rid)
                        }
                    }
                }},

            funcGetGasPrice: { (context, coreEWM, wid, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM, let wid = wid
                    else { print ("SYS: ETH: GetGasPrice: Missed {eid, wid}"); return }

                if let manager = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)) {
                    manager.query.getGasPriceAsETH (ewm: manager.impl.ewm,
                                                    wid: wid,
                                                    rid: rid) { (wid, gasPrice, rid) in
                                                        ewmAnnounceGasPrice (manager.impl.ewm, wid, gasPrice, rid)
                    }
                }},

            funcEstimateGas: { (context, coreEWM, wid, tid, from, to, amount, data, rid)  in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM, let wid = wid, let tid = tid
                    else { print ("SYS: ETH: EstimateGas: Missed {eid, wid, tid}"); return }

                if let manager = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)) {
                    let from = asUTF8String(from!)
                    let to = asUTF8String(to!)
                    let amount = asUTF8String(amount!)
                    let data = asUTF8String(data!)
                    manager.query.getGasEstimateAsETH (ewm: manager.impl.ewm,
                                                       wid: wid,
                                                       tid: tid,
                                                       from: from,
                                                       to: to,
                                                       amount: amount,
                                                       data: data,
                                                       rid: rid) { (wid, tid, gasEstimate, rid) in
                                                        ewmAnnounceGasEstimate (manager.impl.ewm, wid, tid, gasEstimate, rid)
                    }
                }},

            funcSubmitTransaction: { (context, coreEWM, wid, tid, transaction, rid)  in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM, let wid = wid, let tid = tid
                    else { print ("SYS: ETH: SubmitTransaction: Missed {eid, wid, tid}"); return }

                if let manager = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)) {
                    let transaction = asUTF8String (transaction!)
                    manager.query.submitTransactionAsETH (ewm: manager.impl.ewm,
                                                          wid: wid,
                                                          tid: tid,
                                                          transaction: transaction,
                                                          rid: rid) { (wid, tid, hash, errorCode, errorMessage, rid) in
                                                            ewmAnnounceSubmitTransfer (manager.impl.ewm,
                                                                                       wid,
                                                                                       tid,
                                                                                       hash,
                                                                                       errorCode,
                                                                                       errorMessage,
                                                                                       rid)
                    }
                }},

            funcGetTransactions: { (context, coreEWM, address, begBlockNumber, endBlockNumber, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: GetTransactions: Missed {eid}"); return }

                if let manager = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)) {
                    let address = asUTF8String(address!)
                    manager.query.getTransactionsAsETH (ewm: manager.impl.ewm,
                                                        address: address,
                                                        begBlockNumber: begBlockNumber,
                                                        endBlockNumber: endBlockNumber,
                                                        rid: rid,
                                                        done: { (success: Bool, rid: Int32) in
                                                            ewmAnnounceTransactionComplete (manager.impl.ewm,
                                                                                            rid,
                                                                                            (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                    },
                                                        each: { (res: BlockChainDB.ETH.Transaction) in
                                                            ewmAnnounceTransaction (manager.impl.ewm,
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
                    })
                }},

            funcGetLogs: { (context, coreEWM, contract, address, event, begBlockNumber, endBlockNumber, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: GetLogs: Missed {eid}"); return }

                if let manager = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)) {
                    let address  = asUTF8String(address!)
                    let contract = contract.map { asUTF8String ($0) }
                    let event    = asUTF8String (event!)
                    manager.query.getLogsAsETH (ewm: manager.impl.ewm,
                                                contract: contract,
                                                address: address,
                                                event: event,
                                                begBlockNumber: begBlockNumber,
                                                endBlockNumber: endBlockNumber,
                                                rid: rid,
                                                done: { (success: Bool, rid: Int32) in
                                                    ewmAnnounceLogComplete (manager.impl.ewm,
                                                                            rid,
                                                                            (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                    },
                                                each: { (res: BlockChainDB.ETH.Log) in
                                                    var cTopics = res.topics.map { UnsafePointer<Int8>(strdup($0)) }
                                                    defer {
                                                        cTopics.forEach { free (UnsafeMutablePointer(mutating: $0)) }
                                                    }

                                                    ewmAnnounceLog (manager.impl.ewm,
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
                    })
                }},

            funcGetBlocks: { (context, coreEWM, address, interests, blockStart, blockStop, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: GetBlocks: Missed {eid}"); return }

                if let manager = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)) {
                    let address = asUTF8String(address!)
                    manager.query.getBlocksAsETH (ewm: manager.impl.ewm,
                                                  address: address,
                                                  interests: interests,
                                                  blockStart: blockStart,
                                                  blockStop: blockStop,
                                                  rid: rid) { (blocks, rid) in
                                                    ewmAnnounceBlocks (manager.impl.ewm, rid,
                                                                       Int32(blocks.count),
                                                                       UnsafeMutablePointer<UInt64>(mutating: blocks))
                    }
                }},

            funcGetTokens: { (context, coreEWM, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: GetTokens: Missed {eid}"); return }

                if let manager = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)) {
                    manager.query.getTokensAsETH (ewm: manager.impl.ewm,
                                                  rid: rid,
                                                  done: { (success: Bool, rid: Int32) in
                                                    ewmAnnounceTokenComplete (manager.impl.ewm,
                                                                              rid,
                                                                              (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                    },
                                                  each: { (res: BlockChainDB.ETH.Token) in
                                                    ewmAnnounceToken (manager.impl.ewm,
                                                                      res.rid,
                                                                      res.address,
                                                                      res.symbol,
                                                                      res.name,
                                                                      res.description,
                                                                      res.decimals,
                                                                      res.defaultGasLimit,
                                                                      res.defaultGasPrice)
                    })
                }},

            funcGetBlockNumber: { (context, coreEWM, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: GetBlockNumber: Missed {eid}"); return }

                if let manager = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)) {
                    manager.query.getBlockNumberAsETH (ewm: manager.impl.ewm,
                                                       rid: rid) { (number, rid) in
                                                        ewmAnnounceBlockNumber (manager.impl.ewm, number, rid)
                    }
                }},

            funcGetNonce: { (context, coreEWM, address, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: GetNonce: Missed {eid}"); return }

                if let manager = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)) {
                    let address = asUTF8String(address!)
                    manager.query.getNonceAsETH (ewm: manager.impl.ewm,
                                                 address: address,
                                                 rid: rid) { (address, nonce, rid) in
                                                    ewmAnnounceNonce (manager.impl.ewm, address, nonce, rid)
                    }
                }},

            funcEWMEvent: { (context, coreEWM, event, status, message) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: EWM: \(event): Missed {eid}"); return }

                print ("SYS: ETH: Manager: \(event)")
                if let manager = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)) {
                    switch event {
                    case EWM_EVENT_CREATED:
                        break // elsewhere
                    case EWM_EVENT_SYNC_STARTED:
                        manager.state = WalletManagerState.syncing
                        manager.announceEvent(WalletManagerEvent.syncStarted)
                        break
                    case EWM_EVENT_SYNC_CONTINUES:
                        break
                    case EWM_EVENT_SYNC_STOPPED:
                        manager.announceEvent(WalletManagerEvent.syncEnded (error: message.map { asUTF8String ($0) }))
                        manager.state = WalletManagerState.connected
                        break
                    case EWM_EVENT_NETWORK_UNAVAILABLE:
                        break // pending
                    case EWM_EVENT_BLOCK_HEIGHT_UPDATED:
                        manager.height = ewmGetBlockHeight(eid)
                    case EWM_EVENT_DELETED:
                        break // elsewhere
                    default:
                        precondition (false)
                    }
                }},

            funcPeerEvent: { (context, coreEWM, event, status, message) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM
                    else { print ("SYS: ETH: Peer: \(event):  Missed {eid}"); return }

                if let _ = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)) {
                    switch event {
                    case PEER_EVENT_CREATED:
                        break
                    case PEER_EVENT_DELETED:
                        break
                    default:
                        precondition (false)
                    }
                }},

            funcWalletEvent: { (context, coreEWM, wid, event, status, message) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                // Non-NULL values from Core Ethereum
                guard let eid = coreEWM, let wid = wid
                    else { print ("SYS: ETH: Wallet: \(event): Missed {eid, wid}"); return }

                guard let manager  = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid))
                    else { print ("SYS: ETH: Wallet: \(event): Missed {manager, wallet}"); return }

                // We absolutely must have a currency for every `WID`.  Said another way, of course
                // ETH, but also every TOKEN must have a currency.

                // Get the token's symbol or ETH
                let code = ewmWalletGetToken (eid, wid)
                    .flatMap { asUTF8String (tokenGetSymbol ($0)) } ?? manager.network.currency.code

                guard let currency = manager.network.currencyBy (code: code),
                    let unit = manager.network.defaultUnitFor (currency: currency)
                    else { print ("SYS: ETH: Wallet (\(code)): \(event): precondition {currency, unit}"); return } //precondition (false) }

                guard let wallet = manager.walletByImplOrCreate (WalletImplS.Impl.ethereum(ewm: eid, core: wid),
                                                                 listener: system.listener,
                                                                 manager: manager,
                                                                 unit: unit,
                                                                 create: event == WALLET_EVENT_CREATED)
                    else { print ("SYS: ETH: Wallet: \(event): Missed {manager, wallet}"); return }

                print ("SYS: ETH: Wallet (\(wallet.name)): \(event)")

                switch event {
                case WALLET_EVENT_CREATED:
                    break

                case WALLET_EVENT_BALANCE_UPDATED:
                    wallet.upd (balance: wallet.balance)

                case WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED,
                     WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED:
                    wallet.defaultFeeBasis = wallet.impl.defaultFeeBasis(in: wallet.manager.baseUnit)

                case WALLET_EVENT_DELETED:
                    break

                default:
                    precondition (false)
                }},

            funcTokenEvent: { (context, coreEWM, token, event) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let eid = coreEWM, let token = token, let wid = ewmGetWalletHoldingToken (eid, token)
                    else { print ("SYS: ETH: Token: \(event): Missed {eid, token, wid}"); return }

                let name = asUTF8String (tokenGetSymbol (token))

                #if ETH_TOKEN_HANDLE_EVENTS
                // We get token events for Ethereum ERC20 tokens of interest.  This will be a
                // massive subset of all known ERC20 tokens.  So, create a wallet immediately.

                guard let manager  = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)),
                    let   currency = ewmWalletGetToken (coreEWM, wid)
                        .flatMap ({ manager.network.currencyBy (code: asUTF8String (tokenGetSymbol ($0))) }),
                    let   unit     = manager.network.defaultUnitFor (currency: currency)
                    else { print ("SYS: ETH: Token (\(name)): \(event): Missed {manager, currency, unit}"); return }

                guard let wallet = manager.walletByImplOrCreate (WalletImplS.Impl.ethereum(ewm: eid, core: wid),
                                                                 listener: system.listener,
                                                                 manager: manager,
                                                                 unit: unit,
                                                                 create: event == TOKEN_EVENT_CREATED)
                    else { print ("SYS: ETH: Token (\(name)): \(event): Missed {wallet}"); return }
                #endif

                print ("SYS: ETH: Token (\(name)): \(event)")
                switch event {
                case TOKEN_EVENT_CREATED:
                    // We don't create a wallet here... although perhaps we should.  Core knows
                    // about the token; a wallet will be created if/when a transfer for this
                    // token occurs.
                    break

                case TOKEN_EVENT_DELETED:
                    // TODO:
                    break
                    
                default:
                    precondition (false)
                }},

            //            funcBlockEvent: { (context, coreEWM, bid, event, status, message) in
            //                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
            //                    //                    ewm.listener.handleBlockEvent(ewm: ewm,
            //                    //                                                 block: ewm.findBlock(identifier: bid),
            //                    //                                                 event: EthereumBlockEvent (event))
            //                }},

            funcTransferEvent: { (context, coreEWM, wid, tid, event, status, message) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                // Non-NULL values from Core Ethereum
                guard let eid = coreEWM, let wid = wid, let tid = tid
                    else { print ("SYS: ETH: Transfer: \(event): Missed {eid, wid, tid}"); return }

                // Corresponding values in System
                guard let manager = system.managerBy(impl: WalletManagerImplS.Impl.ethereum(ewm: eid)),
                    let wallet    = manager.walletBy (impl: WalletImplS.Impl.ethereum(ewm: eid, core: wid)),
                    let transfer  = wallet.transferByImplOrCreate (TransferImplS.Impl.ethereum (ewm: eid, core: tid),
                                                                   listener: system.listener,
                                                                   create: TRANSFER_EVENT_CREATED == event)
                        else { print ("SYS: ETH: Transfer: \(event): Missed {manager, wallet, transfer}"); return }

                // Handle the event
                print ("SYS: ETH: Transfer (\(wallet.name)): \(event)")
                switch event {
                    case TRANSFER_EVENT_CREATED:
                        guard case TransferState.created = transfer.state
                            else { precondition(false); return }

                    case TRANSFER_EVENT_SIGNED:
                        transfer.state = TransferState.signed

                    case TRANSFER_EVENT_SUBMITTED:
                        transfer.state = TransferState.submitted

                    case TRANSFER_EVENT_INCLUDED:
                        var overflow: Int32 = 0
                        let ether = ewmTransferGetFee (coreEWM, tid, &overflow)
                        let confirmation = TransferConfirmation (
                            blockNumber: ewmTransferGetBlockNumber (coreEWM, tid),
                            transactionIndex: ewmTransferGetTransactionIndex (coreEWM, tid),
                            timestamp: ewmTransferGetBlockTimestamp (coreEWM, tid),
                            fee: Amount.createAsETH (ether.valueInWEI, manager.unit))

                        transfer.state = TransferState.included (confirmation: confirmation)

                    case TRANSFER_EVENT_ERRORED:
                        transfer.state = TransferState.failed (reason: message.flatMap { asUTF8String ($0) } ?? "<missing>")

                    case TRANSFER_EVENT_GAS_ESTIMATE_UPDATED:
                        break
                    case TRANSFER_EVENT_DELETED:
                        break
                    default:
                        precondition (false)
                }
        })
    }

    // Core Bitcoin Client
    
    internal var coreBitcoinClient: BRWalletManagerClient {
        let this = self
        return BRWalletManagerClient (
            context: Unmanaged<SystemBase>.passUnretained(this).toOpaque(),

            funcGetBlockNumber: { (context, bid, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let bid = bid
                    else { print ("SYS: BTC: GetBlockNumber: Missed {bid}"); return }

                print ("SYS: BTC: GetBlockNumber")
                if let this = system.managerBy (impl: WalletManagerImplS.Impl.bitcoin(mid: bid)) {
                    this.query.getBlockNumberAsBTC (bwm: bid,
                                                    blockchainId: this.network.uids,
                                                    rid: rid) {
                                                        (number: UInt64, rid: Int32) in
                                                        bwmAnnounceBlockNumber (bid, rid, number)
                    }
                }},

            funcGetTransactions: { (context, bid, begBlockNumber, endBlockNumber, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let bid = bid
                    else { print ("SYS: BTC: GetTransactions: {\(begBlockNumber), \(endBlockNumber)}: Missed {bid}"); return }

                print ("SYS: BTC: GetTransactions: Blocks: {\(begBlockNumber), \(endBlockNumber)}")
                if let manager = system.managerBy (impl: WalletManagerImplS.Impl.bitcoin(mid: bid)) {
                    // We query the BlockChainDB with an array of addresses.  If there are no
                    // transactions for those addresses, then we are done.  But, if there are
                    // we need to generate more addresses and keep trying to find additional
                    // transactions.
                    //
                    // So we'll repeatedly loop and accumulate transactions until no more
                    // transactions are found for the set of addresses
                    //
                    // In order to 'generate more addresses' we'll need to announce each
                    // transaction - which will register each transaction in the wallet

                    // TODO: Register early transactions even if later transactions fail?  Possible?

                    system.queue.async {
                        let semaphore = DispatchSemaphore (value: 0)

                        var transactionsError = false
                        var transactionsFound = false

                        repeat {
                            // Get a C pointer to `addressesLimit` BRAddress structures
                            let addressesLimit:Int = 25
                            let addressesPointer = BRWalletManagerGetUnusedAddrs (bid, UInt32(addressesLimit))
                            defer { free (addressesPointer) }

                            // Convert the C pointer into a Swift array of BRAddress
                            let addressesStructures:[BRAddress] = addressesPointer!.withMemoryRebound (to: BRAddress.self, capacity: addressesLimit) {
                                Array(UnsafeBufferPointer (start: $0, count: addressesLimit))
                            }

                            // Convert each BRAddress to a String
                            let addresses = addressesStructures.map {
                                return String(cString: UnsafeRawPointer([$0]).assumingMemoryBound(to: CChar.self))
                            }

                            transactionsFound = false
                            transactionsError = false

                            // Query the blockchainDB. Record each found transaction
                            manager.query.getTransactionsAsBTC (bwm: bid,
                                                                blockchainId: manager.network.uids,
                                                                addresses: addresses,
                                                                begBlockNumber: begBlockNumber,
                                                                endBlockNumber: endBlockNumber,
                                                                rid: rid,
                                                                done: { (success: Bool, rid: Int32) in
                                                                    transactionsError = !success
                                                                    semaphore.signal ()
                            },
                                                                each: { (res: BlockChainDB.BTC.Transaction) in
                                                                    transactionsFound = true
                                                                    bwmAnnounceTransaction (bid, res.rid, res.btc)
                            })

                            // Wait until the query is done
                            semaphore.wait()

                        } while !transactionsError && transactionsFound

                        // Announce done
                        bwmAnnounceTransactionComplete (bid, rid, (transactionsError ? 0 : 1))
                    }
                }},

            funcSubmitTransaction: { (context, bid, wid, tid, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let bid = bid, let wid = wid, let tid = tid
                    else { print ("SYS: BTC: SubmitTransaction: Missed {bid, wid, tid}"); return }


                guard let manager  = system.managerBy  (impl: WalletManagerImplS.Impl.bitcoin(mid: bid)),
                    let   wallet   = manager.walletBy  (impl: WalletImplS.Impl.bitcoin(wid: wid)),
                    let   _        = wallet.transferBy (impl: TransferImplS.Impl.bitcoin(wid: wid, tid: tid))
                    else { print ("SYS: BTC: SubmitTransaction: Missed {manager, wallet, transfer}"); return }

                let dataCount = BRTransactionSerialize (tid, nil, 0)
                var data = Data (count: dataCount)
                data.withUnsafeMutableBytes { (bytes: UnsafeMutableRawBufferPointer) -> Void in
                    let bytesAsUInt8 = bytes.baseAddress?.assumingMemoryBound(to: UInt8.self)
                    BRTransactionSerialize (tid, bytesAsUInt8, dataCount)
                }

                manager.query.putTransaction (blockchainId: manager.network.uids,
                                              transaction: data.base64EncodedData(),
                                              completion: { (res: Result<BlockChainDB.Model.Transaction, BlockChainDB.QueryError>) in
                                                var error: Int32 = 0
                                                if case let .failure(f) = res {
                                                    print ("SYS: BTC: SubmitTransaction: Error: \(f)")
                                                    error = 1
                                                }
                                                bwmAnnounceSubmit (bid, rid, tid, error)
                })},

            funcTransactionEvent: { (context, bid, wid, tid, event) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                // Non-NULL values from Core Ethereum
                guard let bid = bid, let wid = wid, let tid = tid
                    else { print ("SYS: BTC: Transfer: \(event.type): Missed {bid, wid, tid}"); return }

                // Corresponding values in System
                guard let manager  = system.managerBy (impl: WalletManagerImplS.Impl.bitcoin(mid: bid)),
                    let   wallet   = manager.walletBy (impl: WalletImplS.Impl.bitcoin(wid: wid)),
                    let   transfer = wallet.transferByImplOrCreate (TransferImplS.Impl.bitcoin(wid: wid, tid: tid),
                                                                    listener: system.listener,
                                                                    create: BITCOIN_TRANSACTION_ADDED == event.type)
                    else { print ("SYS: BTC: Transfer: \(event.type): Missed {manager, wallet, transfer}"); return }

                print ("SYS: BTC: Transfer (\(wallet.name)): \(event.type)")
                switch event.type {
                case BITCOIN_TRANSACTION_ADDED:
                    break
                    
                case BITCOIN_TRANSACTION_UPDATED:
                    let confirmation = TransferConfirmation (
                        blockNumber: UInt64(event.u.updated.blockHeight),
                        transactionIndex: 0,
                        timestamp: UInt64(event.u.updated.timestamp),
                        fee: Amount.createAsBTC (0, manager.unit))

                    transfer.state = TransferState.included (confirmation: confirmation)

                case BITCOIN_TRANSACTION_DELETED:
                    break

                default:
                    precondition(false)
                }},

            funcWalletEvent: { (context, bid, wid, event) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let bid = bid, let wid = wid
                    else { print ("SYS: BTC: Wallet: \(event.type): Missed {bid, wid}"); return }

                guard let manager = system.managerBy (impl: WalletManagerImplS.Impl.bitcoin(mid: bid)),
                    let   wallet  = manager.walletByImplOrCreate (WalletImplS.Impl.bitcoin(wid: wid),
                                                                  listener: system.listener,
                                                                  manager: manager,
                                                                  unit: manager.network.defaultUnitFor(currency: manager.network.currency)!,
                                                                  create: BITCOIN_WALLET_CREATED == event.type)
                    else { print ("SYS: BTC: Wallet: \(event.type): Missed {manager, wallet}"); return }

                print ("SYS: BTC: Wallet (\(wallet.name)): \(event.type)")
                switch event.type {
                case BITCOIN_WALLET_CREATED:
                    break

                case BITCOIN_WALLET_BALANCE_UPDATED:
                    wallet.upd (balance: wallet.balance)

                case BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                    guard let transfer = wallet.transferBy (impl: TransferImplS.Impl.bitcoin(wid: wid, tid: event.u.submitted.transaction))
                        else { print ("SYS: BTC: Wallet: \(event.type): Missed {transfer}"); return }
                    wallet.announceEvent (WalletEvent.transferSubmitted (transfer: transfer,
                                                                         success: 0 == event.u.submitted.error))

                case BITCOIN_WALLET_DELETED:
                    break

                default:
                    precondition(false)
                }},

            funcWalletManagerEvent: { (context, bid, event) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()

                guard let bid = bid
                    else { print ("SYS: BTC: WalletManager: \(event.type): Missed {bid}"); return }

                guard let manager = system.managerBy (impl: WalletManagerImplS.Impl.bitcoin(mid: bid))
                    else { print ("SYS: BTC: WalletWanager: \(event.type): Missed {manager}"); return }

                print ("SYS: BTC: WalletManager (\(manager.name)): \(event.type)")
                switch event.type {
                case BITCOIN_WALLET_MANAGER_CREATED:
                    break

                case BITCOIN_WALLET_MANAGER_CONNECTED:
                    manager.state = WalletManagerState.connected

                case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                    manager.state = WalletManagerState.disconnected

                case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                    manager.state = WalletManagerState.syncing
                    // not so much...
                    manager.listener?.handleManagerEvent (system: system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.syncStarted)

                case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                    // not so much either ...
                    manager.listener?.handleManagerEvent (system: system,
                                                          manager: manager,
                                                          event: WalletManagerEvent.syncEnded(error: nil))
                    manager.state = WalletManagerState.connected

                case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                    manager.height = UInt64(event.u.blockHeightUpdated.value)

                default:
                    precondition(false)
                }
        })
    }
}

extension BRWalletManagerEventType: CustomStringConvertible {
    public var description: String {
        switch self {
        case BITCOIN_WALLET_MANAGER_CREATED: return "Created"
        case BITCOIN_WALLET_MANAGER_CONNECTED: return "Connected"
        case BITCOIN_WALLET_MANAGER_DISCONNECTED: return "Disconnected"
        case BITCOIN_WALLET_MANAGER_SYNC_STARTED: return "Sync Started"
        case BITCOIN_WALLET_MANAGER_SYNC_STOPPED: return "Sync Stopped"
        case BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED: return "Block Height Updated"
        default: return "<<unknown>>"
        }
    }
}

extension BRWalletEventType: CustomStringConvertible {
    public var description: String {
        switch self {
        case BITCOIN_WALLET_CREATED: return "Created"
        case BITCOIN_WALLET_BALANCE_UPDATED: return "Balance Updated"
        case BITCOIN_WALLET_TRANSACTION_SUBMITTED: return "Transaction Submitted"
        case BITCOIN_WALLET_DELETED: return "Deleted"
        default: return "<<unknown>>"
       }
    }
}

extension BRTransactionEventType: CustomStringConvertible {
    public var description: String {
        switch self {
        case BITCOIN_TRANSACTION_ADDED: return "Added"
        case BITCOIN_TRANSACTION_UPDATED: return "Updated"
        case BITCOIN_TRANSACTION_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BREthereumTokenEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case TOKEN_EVENT_CREATED: return "Created"
        case TOKEN_EVENT_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BREthereumPeerEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case PEER_EVENT_CREATED: return "Created"
        case PEER_EVENT_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BREthereumTransferEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case TRANSFER_EVENT_CREATED: return "Created"
        case TRANSFER_EVENT_SIGNED: return "Signed"
        case TRANSFER_EVENT_SUBMITTED: return "Submitted"
        case TRANSFER_EVENT_INCLUDED: return "Included"
        case TRANSFER_EVENT_ERRORED: return "Errored"
        case TRANSFER_EVENT_GAS_ESTIMATE_UPDATED: return "Gas Estimate Updated"
        case TRANSFER_EVENT_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BREthereumWalletEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case WALLET_EVENT_CREATED: return "Created"
        case WALLET_EVENT_BALANCE_UPDATED: return "Balance Updated"
        case WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED: return "Default Gas Limit Updated"
        case WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED: return "Default Gas Price Updated"
        case WALLET_EVENT_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

extension BREthereumEWMEvent: CustomStringConvertible {
    public var description: String {
        switch self {
        case EWM_EVENT_CREATED: return "Created"
        case EWM_EVENT_SYNC_STARTED: return "Sync Started"
        case EWM_EVENT_SYNC_CONTINUES: return "Sync Continues"
        case EWM_EVENT_SYNC_STOPPED: return "Sync Stopped"
        case EWM_EVENT_NETWORK_UNAVAILABLE: return "Network Unavailable"
        case EWM_EVENT_BLOCK_HEIGHT_UPDATED: return "Block Height Updated"
        case EWM_EVENT_DELETED: return "Deleted"
        default: return "<<unknown>>"
        }
    }
}

