//
//  BRSystem.swift
//  BRCrypto
//
//  Created by Ed Gamble on 3/27/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//

import Foundation
import BRCore.Ethereum

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

    /// Wallet Managers
    public internal(set) var managers: [WalletManager] = [];

    public func stop () {
    }

    public func createWalletManager (network: Network,
                                     mode: WalletManagerMode) {
        if let network =  network as? EthereumNetwork {
            let manager = EthereumWalletManager (account: account,
                                                 network: network,
                                                 mode: mode,
                                                 storagePath: "\(path)/eth/\(network.name.lowercased())")
            managers.append(manager)
            listener?.handleManagerEvent(system: self, manager: manager, event: WalletManagerEvent.created)
            listener?.handleSystemEvent(system: self, event: SystemEvent.managerAdded(manager: manager))

            network.currencies
                .filter { $0.type == "erc20" }
                .forEach {
//                    let unit = network.defaultUnitFor(currency: $0)
//                    let core: BREthereumToken! = nil // ewmToek
//                    let token = EthereumToken (identifier: core, currency: $0)
            }
        }
    }

    public func createWallet(manager: WalletManager, currency: Currency) {
        if let manager = manager as? EthereumWalletManager {
            let core = ewmGetWalletHoldingToken (manager.core, nil)
            let wallet = EthereumWallet (manager: manager,
                                         currency: currency,
                                         wid: core!)

            manager.wallets.append (wallet)
            listener?.handleWalletEvent (system: self, manager: manager, wallet: wallet, event: WalletEvent.created)
            listener?.handleManagerEvent(system: self, manager: manager, event: WalletManagerEvent.walletAdded(wallet: wallet))
        }
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

    public init (listener: SystemListener,
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

    public func start () {
        let ETH = Currency (uids: "ETH", name: "Ethereum", code: "ETH", type: "native")
        let ETH_WEI   = Unit (currency: ETH, uids: "ETH_WEI",   name: "WEI",   symbol: "wei")
        let ETH_GWEI  = Unit (currency: ETH, uids: "ETH_GWEI",  name: "GWEI",  symbol: "gwei", base: ETH_WEI, decimals: 9)
        let ETH_ETHER = Unit (currency: ETH, uids: "ETH_ETHER", name: "ETHER", symbol: "Ξ",    base: ETH_WEI, decimals: 18)

        let BRD = Currency (uids: "ERC20 BRD", name: "BRD Token", code: "BRD", type: "erc20")
        let BRD_INT = Unit (currency: BRD, uids: "ERC20 BRD INT", name: "BRD INT", symbol: "BRD INT")
        let BRD_BRD = Unit (currency: BRD, uids: "ERC20_BRD_BRD", name: "BRD", symbol: "BRD", base: BRD_INT, decimals: 18)

        let ETH_ASSOCIATIONS = [
                ETH: NetworkBase.Association (baseUnit: ETH_WEI, defaultUnit: ETH_ETHER, units: Set<Unit> (arrayLiteral: ETH_WEI, ETH_GWEI, ETH_ETHER)),
                BRD: NetworkBase.Association (baseUnit: BRD_INT, defaultUnit: BRD_BRD,   units: Set<Unit> (arrayLiteral: BRD_INT, BRD_BRD))
        ]

        let ETH_Mainnet = EthereumNetwork (core: ethereumMainnet,
                                           name: "Mainnet",
                                           isMainnet: true,
                                           currency: ETH,
                                           associations: ETH_ASSOCIATIONS)
        networks.append(ETH_Mainnet)
        listener?.handleNetworkEvent(system: self, network: ETH_Mainnet, event: NetworkEvent.created)
        listener?.handleSystemEvent (system: self, event: SystemEvent.networkAdded(network: ETH_Mainnet))

        let ETH_Testnet = EthereumNetwork (core: ethereumTestnet,
                                           name: "Testnet",
                                           isMainnet: false,
                                           currency: ETH,
                                           associations: ETH_ASSOCIATIONS)
        networks.append(ETH_Testnet)
        listener?.handleNetworkEvent(system: self, network: ETH_Testnet, event: NetworkEvent.created)
        listener?.handleSystemEvent(system: self, event: SystemEvent.networkAdded(network: ETH_Testnet))
    }

        // Query the BlockchainDB - determine the networks.

//        BRCryptoNetwork networks[] = {
//            // Bitcoin
//            cryptoNetworkCreateAsBTC("BTC Mainnet", 0x00, BRMainNetParams),
//            cryptoNetworkCreateAsBTC("BTC Testnet", 0x40, BRTestNetParams),
//
//            // Bitcash
//            cryptoNetworkCreateAsBTC("BCH Mainnet", 0x00, BRBCashParams),
//            cryptoNetworkCreateAsBTC("BCH Testnet", 0x40, BRBCashTestNetParams),
//
//            // Ethereum
//            cryptoNetworkCreateAsETH ("ETH Mainnet", 1, ethereumMainnet),
//            cryptoNetworkCreateAsETH ("ETH Ropsten", 3, ethereumTestnet),
//            cryptoNetworkCreateAsETH ("ETH Rinkeby", 4, ethereumRinkeby)
//        };
//
//        BRArrayOf(BRCryptoUnit) units;
//        array_new (units, 3);
//
//        BRCryptoCurrency BTC = cryptoCurrencyCreate ("Bitcoin", "BTC", "native");
//        BRCryptoUnit BTC_SAT = cryptoUnitCreateAsBase (BTC, "SAT", "sat");
//        BRCryptoUnit BTC_BTC = cryptoUnitCreate (BTC, "BTC", "₿", BTC_SAT, 8);
//        BRArrayOf(BRCryptoUnit) BTC_UNITS;
//        array_new (BTC_UNITS, 2);
//        array_add (BTC_UNITS, BTC_SAT);
//        array_add (BTC_UNITS, BTC_BTC);
//
//        BRCryptoCurrency BCH = cryptoCurrencyCreate ("Bitcash", "BCH", "native");
//        BRCryptoUnit BCH_SAT = cryptoUnitCreateAsBase (BCH, "SAT", "sat");
//        BRCryptoUnit BCH_BTC = cryptoUnitCreate (BCH, "BTH", "₿", BCH_SAT, 8);
//        BRArrayOf(BRCryptoUnit) BCH_UNITS;
//        array_new (BCH_UNITS, 2);
//        array_add (BCH_UNITS, BCH_SAT);
//        array_add (BCH_UNITS, BCH_BTC);
//
//        BRCryptoCurrency ETH   = cryptoCurrencyCreate ("Ethereum", "ETH", "native");
//        BRCryptoUnit ETH_WEI   = cryptoUnitCreateAsBase (ETH, "WEI", "wei");
//        BRCryptoUnit ETH_GWEI  = cryptoUnitCreate (ETH, "GWEI",  "WEI", ETH_WEI,  9);
//        BRCryptoUnit ETH_ETHER = cryptoUnitCreate (ETH, "ETHER", "Ξ",   ETH_WEI, 18);
//        BRArrayOf(BRCryptoUnit) ETH_UNITS;
//        array_new (ETH_UNITS, 3);
//        array_add (ETH_UNITS, ETH_WEI);
//        array_add (ETH_UNITS, ETH_GWEI);
//        array_add (ETH_UNITS, ETH_ETHER);
//
//        BRCryptoCurrency BRD = cryptoCurrencyCreate("BRD", "BRD", "erc20");
//        BRCryptoUnit BRD_INTEGER = cryptoUnitCreateAsBase (BRD, "BRD_INTEGER", "BRD_INT");
//        BRCryptoUnit BRD_BRD     = cryptoUnitCreate (BRD, "BRD", "BRD", BRD_INTEGER, 18);
//        BRArrayOf(BRCryptoUnit) BRD_UNITS;
//        array_new (BRD_UNITS, 2);
//        array_add (BRD_UNITS, BRD_INTEGER);
//        array_add (BRD_UNITS, BRD_BRD);
//
//
//        array_new (units, 3);
//        array_add_array (units, BTC_UNITS, array_count(BTC_UNITS));
//        cryptoNetworkAddCurrency (networks[0], BTC, BTC_SAT, BTC_BTC, units);
//        cryptoNetworkSetCurrency (networks[0], BTC);
//
//        array_new (units, 3);
//        array_add_array (units, BTC_UNITS, array_count(BTC_UNITS));
//        cryptoNetworkAddCurrency (networks[1], BTC, BTC_SAT, BTC_BTC, units);
//        cryptoNetworkSetCurrency (networks[1], BTC);
//
//        array_new (units, 3);
//        array_add_array (units, BCH_UNITS, array_count(BCH_UNITS));
//        cryptoNetworkAddCurrency (networks[2], BCH, BCH_SAT, BCH_BTC, units);
//        cryptoNetworkSetCurrency (networks[2], BCH);
//
//        array_new (units, 3);
//        array_add_array (units, BCH_UNITS, array_count(BCH_UNITS));
//        cryptoNetworkAddCurrency (networks[3], BCH, BCH_SAT, BCH_BTC, units);
//        cryptoNetworkSetCurrency (networks[3], BCH);
//
//        array_new (units, 3);
//        array_add_array (units, ETH_UNITS, array_count(ETH_UNITS));
//        cryptoNetworkAddCurrency (networks[4], ETH, ETH_WEI, ETH_ETHER, units);
//        array_new (units, 3);
//        array_add_array (units, BRD_UNITS, array_count(BRD_UNITS));
//        cryptoNetworkAddCurrency (networks[4], BRD, BRD_INTEGER, BRD_BRD, units);
//        cryptoNetworkSetCurrency (networks[4], ETH);
//
//        array_new (units, 3);
//        array_add_array (units, ETH_UNITS, array_count(ETH_UNITS));
//        cryptoNetworkAddCurrency (networks[5], ETH, ETH_WEI, ETH_ETHER, units);
//        array_new (units, 3);
//        array_add_array (units, BRD_UNITS, array_count(BRD_UNITS));
//        cryptoNetworkAddCurrency (networks[5], BRD, BRD_INTEGER, BRD_BRD, units);
//        cryptoNetworkSetCurrency (networks[5], ETH);
//
//        array_new (units, 3);
//        array_add_array (units, ETH_UNITS, array_count(ETH_UNITS));
//        cryptoNetworkAddCurrency (networks[6], ETH, ETH_WEI, ETH_ETHER, units);
//        // No BRD
//        cryptoNetworkSetCurrency (networks[6], ETH);
//
//        array_free (units);
//        array_free (BRD_UNITS); array_free (ETH_UNITS); array_free (BCH_UNITS); array_free (BTC_UNITS);
//
//        // Announce Networks
//        for (size_t index = 0; index <= 6; index++)
//        cryptoNetworkAnnounce(networks[index]);

}
        // Network Listener

//        self.coreNetworkListener = BRCryptoNetworkListener (
//            context: Unmanaged<System>.passRetained(self).toOpaque(),
//            created: { (context: BRCryptoNetworkListenerContext?, coreNetwork: BRCryptoNetwork?) in
//                precondition (nil != context && nil != coreNetwork)
//                let system = Unmanaged<System>.fromOpaque(context!).takeRetainedValue()
//                let network = Network (core: coreNetwork!)
//
//                system.networks.append (network)
//                system.listener?.handleNetworkEvent(
//                    system: system,
//                    network: network,
//                    event: NetworkEvent.created)
//        } // ,
//            //            currencyAdded: { (context: BRCryptoNetworkListenerContext?, coreNetwork: BRCryptoNetwork?, coreCurrency: BRCryptoCurrency?) in
//            //                precondition (nil != context && nil != coreNetwork && nil != coreCurrency)
//            //                let system = Unmanaged<System>.fromOpaque(context!).takeRetainedValue()
//            //                if let network = system.lookup (network: coreNetwork!) {
//            //                    let currency = Currency (core: coreCurrency!)
//            //                    // add to 'network'
//            //                    system.listener?.handleNetworkEvent(
//            //                        system: system,
//            //                        network: network,
//            //                        event: NetworkEvent.currencyAdded(currency: currency))
//            //                }
//            //        },
//            //            currencyDeleted: { (context: BRCryptoNetworkListenerContext?, coreNetwork: BRCryptoNetwork?, coreCurrency: BRCryptoCurrency?) in
//            //                precondition (nil != context && nil != coreNetwork && nil != coreCurrency)
//            //                let system = Unmanaged<System>.fromOpaque(context!).takeRetainedValue()
//            //                if let network = system.lookup(network: coreNetwork!) {
//            //                    let currency = Currency (core: coreCurrency!)
//            //                    // rem from 'network'
//            //                    system.listener?.handleNetworkEvent(
//            //                        system: system,
//            //                        network: network,
//            //                        event: NetworkEvent.currencyDeleted(currency: currency))
//            //                }
//            //        }
//        )
//        cryptoNetworkDeclareListener(self.coreNetworkListener)
//
//        // Wallet Listener
//
//        self.coreWalletManagerListener = BRCryptoCWMListener (
//            context: Unmanaged<System>.passRetained(self).toOpaque(),
//
//            walletManagerEventCallback: { (context: BRCryptoCWMListenerContext?, coreCWM: BRCryptoWalletManager?) in
//                precondition (nil != context && nil != coreCWM)
//                let system  = Unmanaged<System>.fromOpaque(context!).takeRetainedValue()
//                if let manager = system.lookup(manager: coreCWM!) {
//                    let event = WalletManagerEvent.created
//
//                    system.listener?.handleManagerEvent(
//                        system: system,
//                        manager: manager,
//                        event: event)
//                }
//        },
//            walletEventCallback: { (context: BRCryptoCWMListenerContext?, coreCWM: BRCryptoWalletManager?, coreWallet: BRCryptoWallet?) in
//                precondition (nil != context && nil != coreCWM && nil != coreWallet)
//                let system = Unmanaged<System>.fromOpaque(context!).takeRetainedValue()
//                if let manager = system.lookup (manager: coreCWM!),
//                    let wallet = system.lookup(wallet: coreWallet!, manager: manager) {
//
//                    let event = WalletEvent.created
//
//                    system.listener?.handleWalletEvent(
//                        system: system,
//                        manager: manager,
//                        wallet: wallet,
//                        event: event)
//                }
//        },
//
//            transferEventCallback: { (context: BRCryptoCWMListenerContext?, coreCWM: BRCryptoWalletManager?, coreWallet: BRCryptoWallet?, coreTransfer: BRCryptoTransfer?) in
//                precondition (nil != context && nil != coreCWM && nil != coreWallet && nil != coreTransfer)
//                let system = Unmanaged<System>.fromOpaque(context!).takeRetainedValue()
//                if let manager = system.lookup (manager: coreCWM!),
//                    let wallet = system.lookup (wallet: coreWallet!, manager: manager),
//                    let transfer = system.lookup (transfer: coreTransfer!, wallet: wallet) {
//
//                    let event = TransferEvent.created
//
//                    system.listener?.handleTransferEvent(
//                        system: system,
//                        manager: manager,
//                        wallet: wallet,
//                        transfer: transfer,
//                        event: event)
//                }
//        })
//        cryptoWalletManagerDeclareListener (self.coreWalletManagerListener)
//
//        // Start communicating with URL
//        //   Load currencies
//        //   Load blockchains
//
//    }
//
//    internal func lookup (network core: BRCryptoNetwork) -> Network? {
//        return networks.first { $0.core == core }
//    }
//
//    internal func lookup (manager core: BRCryptoWalletManager) -> WalletManager? {
//        return managers.first { $0.core == core }
//    }
//
//    internal func lookup (wallet core: BRCryptoWallet, manager: WalletManager) -> Wallet? {
//        return manager.wallets.first { $0.core == core }
//    }
//
//    internal func lookup (transfer core: BRCryptoTransfer, wallet: Wallet) -> Transfer? {
//        return wallet.transfers.first { $0.core == core }
//    }
//
//    // No - define 'system listener'
//    private var coreNetworkListener: BRCryptoNetworkListener! = nil
//    private var coreWalletManagerListener: BRCryptoCWMListener! = nil
//}


