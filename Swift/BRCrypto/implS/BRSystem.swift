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

    /// The 'blockchain DB'
    public let query: BlockChainDB

    public let account: Account

    /// Networks
    public internal(set) var networks: [Network] = []

    internal func add (network: Network) {
        networks.append (network)
        listener?.handleSystemEvent (system: self,
                                     event: SystemEvent.networkAdded(network: network))
    }

    /// Wallet Managers
    public internal(set) var managers: [WalletManager] = [];

    internal func add (manager: WalletManager) {
        if !managers.contains(where: { $0 === manager }) {
            managers.append (manager)
            listener?.handleSystemEvent (system: self,
                                         event: SystemEvent.managerAdded(manager: manager))
        }
    }

    internal func lookupManager (eth: BREthereumEWM) -> WalletManagerImplS? {
        return managers.first {
            ($0 as? WalletManagerImplS).map { $0.impl.ewm == eth } ?? false
            } as? WalletManagerImplS
    }

    internal func lookupManager (btc: BRCoreWalletManager) -> WalletManagerImplS? {
        return managers.first {
            ($0 as? WalletManagerImplS).map { $0.impl.bwm == btc } ?? false
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
            // events will be ignored because system.managers cannot possiby include 'manager'.
            // Thus the events are lost.
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

        // No point annoncing any
        // Announce manager creation
        //        listener?.handleManagerEvent(system: self, manager: manager, event: WalletManagerEvent.created)

        // Save the manager - will announce the SystemEvent.managerAdded(...
        add (manager: manager)

        // Touch the primary wallet
        let _ = manager.primaryWallet

    }

    private static var instance: System?

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

    internal init (listener: SystemListener,
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

    public func stop () {
        managers.forEach { $0.disconnect() }
    }

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
                .map { BlockChainDB.Model.unionBlockchain (BlockChainDB.Model.defaultBlockchains, $0) }
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
                            .map { BlockChainDB.Model.unionCurrency (defaults, $0) }
                            // On error, use `defaults`
                            .recover { (error: BlockChainDB.QueryError) -> [BlockChainDB.Model.Currency] in
                                return defaults
                            }.get()

                        var associations: [Currency : Network.Association] = [:]

                        // Update associations
                        currencyModels.forEach { (currencyModel: BlockChainDB.Model.Currency) in
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
                if let this = system.lookupManager(eth: coreEWM!) {

                    let address = asUTF8String(address!)
                    this.query.getBalanceAsETH (ewm: this.impl.ewm,
                                                wid: wid!,
                                                address: address,
                                                rid: rid) { (wid, balance, rid) in
                                                    ewmAnnounceWalletBalance (this.impl.ewm, wid, balance, rid)
                    }
                }},

            funcGetGasPrice: { (context, coreEWM, wid, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                if let this = system.lookupManager(eth: coreEWM!) {
                    this.query.getGasPriceAsETH (ewm: this.impl.ewm,
                                                 wid: wid!,
                                                 rid: rid) { (wid, gasPrice, rid) in
                                                    ewmAnnounceGasPrice (this.impl.ewm, wid, gasPrice, rid)
                    }
                }},

            funcEstimateGas: { (context, coreEWM, wid, tid, from, to, amount, data, rid)  in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                if let this = system.lookupManager(eth: coreEWM!) {
                    let from = asUTF8String(from!)
                    let to = asUTF8String(to!)
                    let amount = asUTF8String(amount!)
                    let data = asUTF8String(data!)
                    this.query.getGasEstimateAsETH (ewm: this.impl.ewm,
                                                    wid: wid!,
                                                    tid: tid!,
                                                    from: from,
                                                    to: to,
                                                    amount: amount,
                                                    data: data,
                                                    rid: rid) { (wid, tid, gasEstimate, rid) in
                                                        ewmAnnounceGasEstimate (this.impl.ewm, wid, tid, gasEstimate, rid)
                    }
                }},

            funcSubmitTransaction: { (context, coreEWM, wid, tid, transaction, rid)  in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                if let this = system.lookupManager(eth: coreEWM!) {
                    let transaction = asUTF8String (transaction!)
                    this.query.submitTransactionAsETH (ewm: this.impl.ewm,
                                                       wid: wid!,
                                                       tid: tid!,
                                                       transaction: transaction,
                                                       rid: rid) { (wid, tid, hash, errorCode, errorMessage, rid) in
                                                        ewmAnnounceSubmitTransfer (this.impl.ewm,
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
                if let this = system.lookupManager(eth: coreEWM!) {
                    let address = asUTF8String(address!)
                    this.query.getTransactionsAsETH (ewm: this.impl.ewm,
                                                     address: address,
                                                     begBlockNumber: begBlockNumber,
                                                     endBlockNumber: endBlockNumber,
                                                     rid: rid,
                                                     done: { (success: Bool, rid: Int32) in
                                                        ewmAnnounceTransactionComplete (this.impl.ewm,
                                                                                        rid,
                                                                                        (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                    },
                                                     each: { (res: BlockChainDB.ETH.Transaction) in
                                                        ewmAnnounceTransaction (this.impl.ewm,
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
                if let this = system.lookupManager(eth: coreEWM!) {
                    let address = asUTF8String(address!)
                    this.query.getLogsAsETH (ewm: this.impl.ewm,
                                             address: address,
                                             begBlockNumber: begBlockNumber,
                                             endBlockNumber: endBlockNumber,
                                             rid: rid,
                                             done: { (success: Bool, rid: Int32) in
                                                ewmAnnounceLogComplete (this.impl.ewm,
                                                                        rid,
                                                                        (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                    },
                                             each: { (res: BlockChainDB.ETH.Log) in
                                                var cTopics = res.topics.map { UnsafePointer<Int8>(strdup($0)) }
                                                defer {
                                                    cTopics.forEach { free (UnsafeMutablePointer(mutating: $0)) }
                                                }

                                                ewmAnnounceLog (this.impl.ewm,
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
                if let this = system.lookupManager(eth: coreEWM!) {
                    let address = asUTF8String(address!)
                    this.query.getBlocksAsETH (ewm: this.impl.ewm,
                                               address: address,
                                               interests: interests,
                                               blockStart: blockStart,
                                               blockStop: blockStop,
                                               rid: rid) { (blocks, rid) in
                                                ewmAnnounceBlocks (this.impl.ewm, rid,
                                                                   Int32(blocks.count),
                                                                   UnsafeMutablePointer<UInt64>(mutating: blocks))
                    }
                }},

            funcGetTokens: { (context, coreEWM, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                if let this = system.lookupManager(eth: coreEWM!) {
                    this.query.getTokensAsETH (ewm: this.impl.ewm,
                                               rid: rid,
                                               done: { (success: Bool, rid: Int32) in
                                                ewmAnnounceTokenComplete (this.impl.ewm,
                                                                          rid,
                                                                          (success ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE))
                    },
                                               each: { (res: BlockChainDB.ETH.Token) in
                                                ewmAnnounceToken (this.impl.ewm,
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
                if let this = system.lookupManager(eth: coreEWM!) {
                    this.query.getBlockNumberAsETH (ewm: this.impl.ewm,
                                                    rid: rid) { (number, rid) in
                                                        ewmAnnounceBlockNumber (this.impl.ewm, number, rid)
                    }
                }},

            funcGetNonce: { (context, coreEWM, address, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                if let this = system.lookupManager(eth: coreEWM!) {
                    let address = asUTF8String(address!)
                    this.query.getNonceAsETH (ewm: this.impl.ewm,
                                              address: address,
                                              rid: rid) { (address, nonce, rid) in
                                                ewmAnnounceNonce (this.impl.ewm, address, nonce, rid)
                    }
                }},

            funcEWMEvent: { (context, coreEWM, event, status, message) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                //                NSLog ("System: Ethereum: Manager: \(event)")
                if let this = system.lookupManager(eth: coreEWM!) {
                    switch event {
                    case EWM_EVENT_CREATED:
                        // elsewhere
                        break
                    case EWM_EVENT_SYNC_STARTED:
                        this.listener?.handleManagerEvent (system: this.system,
                                                           manager: this,
                                                           event: WalletManagerEvent.syncStarted)
                        break
                    case EWM_EVENT_SYNC_CONTINUES:
                        break
                    case EWM_EVENT_SYNC_STOPPED:
                        this.listener?.handleManagerEvent (system: this.system,
                                                           manager: this,
                                                           event: WalletManagerEvent.syncEnded (error: message.map { asUTF8String ($0) }))
                        break
                    case EWM_EVENT_NETWORK_UNAVAILABLE:
                        // pending
                        break
                    case EWM_EVENT_DELETED:
                        // elsewhere
                        break
                    default:
                        precondition (false)
                    }
                }},

            funcPeerEvent: { (context, coreEWM, event, status, message) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                if let _ = system.lookupManager(eth: coreEWM!) {
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
                //                NSLog ("System: Ethereum: Wallet: \(event)")
                if let this = system.lookupManager(eth: coreEWM!) {

                    if event == WALLET_EVENT_CREATED, nil == this.lookupWallet(eth: wid), let wid = wid {
                        //                        NSLog ("System: Ethereum: Wallet: Created")
                        let currency = ewmWalletGetToken (coreEWM, wid)
                            .flatMap { this.network.currencyBy (code: asUTF8String (tokenGetSymbol ($0))) }
                            ?? this.network.currency

                        let wallet = WalletImplS (listener: this.system.listener,
                                                  manager: this,
                                                  unit: this.network.defaultUnitFor (currency: currency)!,
                                                  eth: wid)

                        this.add (wallet: wallet)
                    }

                    if let wallet = this.lookupWallet (eth: wid) {
                        switch event {
                        case WALLET_EVENT_CREATED:
                            break
                        case WALLET_EVENT_BALANCE_UPDATED:
                            wallet.upd (balance: wallet.balance)

                        case WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED,
                             WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED:
                            wallet.updateDefaultFeeBasis ()

                        case WALLET_EVENT_DELETED:
                            break

                        default:
                            precondition (false)
                        }
                    }
                }},

            funcTokenEvent: { (context, coreEWM, token, event) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                if let _ = system.lookupManager(eth: coreEWM!) {
                    switch event {
                    case TOKEN_EVENT_CREATED:
                        break
                    case TOKEN_EVENT_DELETED:
                        break
                    default:
                        precondition (false)
                    }
                }},

            //            funcBlockEvent: { (context, coreEWM, bid, event, status, message) in
            //                if let ewm = EthereumWalletManager.lookup(core: coreEWM) {
            //                    //                    ewm.listener.handleBlockEvent(ewm: ewm,
            //                    //                                                 block: ewm.findBlock(identifier: bid),
            //                    //                                                 event: EthereumBlockEvent (event))
            //                }},

            funcTransferEvent: { (context, coreEWM, wid, tid, event, status, message) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                //               NSLog ("System: Ethereum: Transfer: \(event)")
                if let this = system.lookupManager(eth: coreEWM!),
                    let wallet = this.lookupWallet (eth: wid) {

                    if TRANSFER_EVENT_CREATED == event, let tid = tid {
                        //                        NSLog ("System: Ethereum: Transfer Created")
                        let transfer = TransferImplS (listener: this.system.listener,
                                                      wallet: wallet,
                                                      unit: wallet.unit,
                                                      eth: tid)
                        wallet.add (transfer: transfer)
                    }

                    if let transfer = wallet.lookupTransfer (eth: tid) as? TransferImplS {
                        switch event {
                        case TRANSFER_EVENT_CREATED:
                            break

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
                                fee: Amount.createAsETH (ether.valueInWEI, this.unit))

                            transfer.state = TransferState.included(confirmation: confirmation)


                        case TRANSFER_EVENT_ERRORED:
                            transfer.state = TransferState.failed (reason: message.flatMap { asUTF8String ($0) } ?? "<missing>")

                        case TRANSFER_EVENT_GAS_ESTIMATE_UPDATED:
                            break
                        case TRANSFER_EVENT_BLOCK_CONFIRMATIONS_UPDATED:
                            break
                        case TRANSFER_EVENT_DELETED:
                            break
                        default:
                            precondition (false)
                        }}
                }})
    }

    // Core Bitcoin Client
    
    internal var coreBitcoinClient: BRWalletManagerClient {
        let this = self
        return BRWalletManagerClient (
            context: Unmanaged<SystemBase>.passUnretained(this).toOpaque(),

            funcGetBlockNumber: { (context, bid, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                NSLog ("System: Bitcoin: GetBlockNumber")
                if let this = system.lookupManager(btc: bid!) {
                    this.query.getBlockNumberAsBTC (bwm: bid!,
                                                    blockchainId: this.network.uids,
                                                    rid: rid) {
                                                        (number: UInt64, rid: Int32) in
                                                        bwmAnnounceBlockNumber (bid, rid, number)
                    }
                }},

            funcGetTransactions: { (context, bid, begBlockNumber, endBlockNumber, rid) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                NSLog ("System: Bitcoin: GetTransaction: {\(begBlockNumber), \(endBlockNumber)}")
                if let this = system.lookupManager(btc: bid!) {
                    // We query the BlockChainDB with an array of addresses.  If there are no
                    // transactions for those addresses, then we are done.  But, if there are
                    // we need to generate more address and keep trying to find additional
                    // transactions.

                    // Get a C pointer to `addressesLimit` BRAddress structures
                    let addressesLimit:Int = 3
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

                    this.query.getTransactionsAsBTC (bwm: bid!,
                                                     blockchainId: this.network.uids,
                                                     addresses: addresses,
                                                     begBlockNumber: begBlockNumber,
                                                     endBlockNumber: endBlockNumber,
                                                     rid: rid,
                                                     done: { (success: Bool, rid: Int32) in
                                                        bwmAnnounceTransactionComplete (bid, rid, (success ? 1 : 0))
                    },
                                                     each: { (res: BlockChainDB.BTC.Transaction) in
                                                        bwmAnnounceTransaction (bid, res.rid, res.btc)
                    })
                }},

            funcTransactionEvent: { (context, bid, wid, tid, event) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                //                NSLog ("System: Bitcoin: Transfer: \(event.type)")
                if let this = system.lookupManager(btc: bid!),
                    let wallet = this.lookupWallet (btc: wid) {

                    if event.type == BITCOIN_TRANSACTION_ADDED, let tid = tid {
                        //                        NSLog ("System: Bitcoin: Transfer Created")
                        let transfer = TransferImplS (listener: system.listener,
                                                      wallet: wallet,
                                                      unit: wallet.unit,
                                                      btc: tid)

                        wallet.add(transfer: transfer)
                    }

                    if let transfer = wallet.lookupTransfer (btc: tid) as? TransferImplS {
                        switch event.type {
                        case BITCOIN_TRANSACTION_ADDED:
                            break
                        case BITCOIN_TRANSACTION_UPDATED:
                            let confirmation = TransferConfirmation (
                                blockNumber: UInt64(event.u.updated.blockHeight),
                                transactionIndex: 0,
                                timestamp: UInt64(event.u.updated.timestamp),
                                fee: Amount.createAsBTC (0, this.unit))

                            transfer.state = TransferState.included (confirmation: confirmation)

                        case BITCOIN_TRANSACTION_DELETED:
                            break
                        default:
                            precondition(false)
                        }}
                }},

            funcWalletEvent: { (context, bid, wid, event) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                //                NSLog ("System: Bitcoin: Wallet: \(event.type)")
                if let this = system.lookupManager(btc: bid!) {

                    if event.type == BITCOIN_WALLET_CREATED, let wid = wid {
                        //                        NSLog ("System: Bitcoin: Wallet: Created")
                        let wallet = WalletImplS (listener: system.listener,
                                                  manager: this,
                                                  unit: this.network.defaultUnitFor(currency: this.network.currency)!,
                                                  btc: wid)
                        this.add (wallet: wallet)
                    }

                    if let wallet = this.lookupWallet (btc: wid) {
                        switch event.type {
                        case BITCOIN_WALLET_CREATED:
                            break

                        case BITCOIN_WALLET_BALANCE_UPDATED:
                            wallet.upd (balance: wallet.balance)

                        case BITCOIN_WALLET_DELETED:
                            break

                        default:
                            precondition(false)
                        }
                    }
                }},

            funcWalletManagerEvent: { (context, bid, event) in
                let system = Unmanaged<SystemBase>.fromOpaque(context!).takeUnretainedValue()
                //                NSLog ("System: Bitcoin: Manager: \(event.type)")
                if let this = system.lookupManager(btc: bid!) {
                    switch event.type {
                    case BITCOIN_WALLET_MANAGER_CREATED:
                        break

                    case BITCOIN_WALLET_MANAGER_CONNECTED:
                        this.state = WalletManagerState.connected

                    case BITCOIN_WALLET_MANAGER_DISCONNECTED:
                        this.state = WalletManagerState.disconnected

                    case BITCOIN_WALLET_MANAGER_SYNC_STARTED:
                        this.state = WalletManagerState.syncing

                        // not so much...
                        this.listener?.handleManagerEvent (system: this.system,
                                                           manager: this,
                                                           event: WalletManagerEvent.syncStarted)

                    case BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                        this.state = WalletManagerState.connected

                        // not so much either ...
                        this.listener?.handleManagerEvent (system: this.system,
                                                           manager: this,
                                                           event: WalletManagerEvent.syncEnded(error: nil))

                    default:
                        precondition(false)
                    }
                }
        })
    }
}

