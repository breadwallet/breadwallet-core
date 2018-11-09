//
//  CoreDemoEthereumClient.swift
//  CoreDemo
//
//  Created by Ed Gamble on 7/26/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

import BRCore

protocol WalletListener {
    func announceWalletEvent (ewm: EthereumWalletManager,
                             wallet: EthereumWallet,
                             event: EthereumWalletEvent)
}

protocol TransferListener {
    func announceTransferEvent (ewm: EthereumWalletManager,
                                wallet: EthereumWallet,
                                transfer: EthereumTransfer,
                                event: EthereumTransferEvent)
}

class CoreDemoEthereumClient : EthereumClient {
    let network : EthereumNetwork
    var node : EthereumWalletManager!

    // Pure convenience - see TransferCreateCopntroller
    let paperKey : String

    //
    // Constructors
    //
    init(network: EthereumNetwork, mode: EthereumMode, paperKey: String) {
        self.network = network
        self.paperKey = paperKey
        self.node = EthereumWalletManager (client: self,
                                           network: network,
                                           mode: mode,
                                           key: EthereumKey.paperKey(paperKey),
                                           timestamp: 0)
    }

    func getBalance(ewm: EthereumWalletManager, wid: EthereumWalletId, address: String, rid: Int32) {
        switch address.lowercased() {
        case "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62".lowercased():
            switch  ewm.findWallet (identifier: wid).name {
            case "ETH": ewm.announceBalance (wid: wid, balance: "0x203A46E4A56283" , rid: rid)
            case "BRD": ewm.announceBalance (wid: wid, balance: "0x16345785d8a0000", rid: rid)
            default:
                ewm.announceBalance (wid: wid, balance: "0xffc0", rid: rid)
            }

        default:
            // JSON_RPC -> JSON -> Result -> announceBalance()
            ewm.announceBalance (wid: wid, balance: "0xffc0", rid: rid)
        }
    }

    func getGasPrice(ewm: EthereumWalletManager, wid: EthereumWalletId, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceGasPrice()
        ewm.announceGasPrice (wid: wid, gasPrice: "0x77", rid: rid)
    }

    func getGasEstimate(ewm: EthereumWalletManager, wid: EthereumWalletId, tid: EthereumTransferId, to: String, amount: String, data: String, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceGasEstimate()
        ewm.announceGasEstimate (wid: wid, tid: tid, gasEstimate: "0x21000", rid: rid)
    }

    func submitTransaction(ewm: EthereumWalletManager, wid: EthereumWalletId, tid: EthereumTransferId, rawTransaction: String, rid: Int32) {
        // JSON_RPC -> JSON -> Result -> announceSubmitTransaction()
        ewm.announceSubmitTransaction (wid: wid, tid: tid, hash: "0xffaabb", rid: rid)
        return
    }

    func getTransactions(ewm: EthereumWalletManager, address: String, rid: Int32) {
        // JSON_RPC -> [JSON] -> forEach {Result -> announceSubmitTransaction()}
        switch address.lowercased() {
        case "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62".lowercased():
            /*
            ewm.announceTransaction(rid: rid,
                                    hash: "0xaa6dd4bc0c4411cc83778a1258c8c9f1a2a5e131cc50ddd2d4145a21564a8d0c",
                                    sourceAddr: "0x19454a70538bfbdbd7abf3ac8d274d5cb2514056",
                                    targetAddr:  ewm.address,
                                    contractAddr: "",
                                    amount: "0x242744C11F7283",
                                    gasLimit: "21000",
                                    gasPrice: "3960000000",
                                    data: "0x",
                                    nonce: "40",
                                    gasUsed: "21000",
                                    blockNumber: "6619285",
                                    blockHash: "0xd8a60bffeab9b89929b9ef71d6cc06ba92e24020a6c276e89e5b044d1d6babf2",
                                    blockConfirmations: "10",
                                    blockTransactionIndex: "114",
                                    blockTimestamp: "1516477482",
                                    isError: "0")

            ewm.announceTransaction(rid: rid,
                                    hash: "0x1888edcef94596a016185fa454bec2c5a6901d6c57746d175776734cb87eb807",
                                    sourceAddr: ewm.address,
                                    targetAddr:  "0x19454a70538bfbdbd7abf3ac8d274d5cb2514056",
                                    contractAddr: "",
                                    amount: "0x038d7ea4c68000",
                                    gasLimit: "21000",
                                    gasPrice: "5000000000",
                                    data: "0x",
                                    nonce: "0",
                                    gasUsed: "21000",
                                    blockNumber: "6619470",
                                    blockHash: "0x82f1139de4d8b857483fad5213036f07cf245cff2eaa32d0a8f4ddca34cc7853",
                                    blockConfirmations: "10",
                                    blockTransactionIndex: "38",
                                    blockTimestamp: "1516477482",
                                    isError: "0")
 */
            break
        default:
            ewm.announceTransaction(rid: rid,
                                    hash: "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
                                    sourceAddr: ewm.address,
                                    targetAddr: "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
                                    contractAddr: "",
                                    amount: "11113000000000",
                                    gasLimit: "21000",
                                    gasPrice: "21000000000",
                                    data: "0x",
                                    nonce: "118",
                                    gasUsed: "21000",
                                    blockNumber: "1627184",
                                    blockHash: "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                                    blockConfirmations: "339050",
                                    blockTransactionIndex: "3",
                                    blockTimestamp: "1516477482",
                                    isError: "0")
        }
    }

    func getLogs(ewm: EthereumWalletManager, address: String, event: String, rid: Int32) {
        switch address.lowercased() {
        case "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62".lowercased():
            ewm.announceLog(rid: rid,
                            hash: "0x3063e073c0b90693639fad94258797baf39c1e2b2a6e56b2e85010e5c963f3b3",
                            contract: (ewm.network == EthereumNetwork.mainnet
                                ? "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"
                                : "0x7108ca7c4718efa810457f228305c9c71390931a"),
                            topics: ["0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
                                     "0x00000000000000000000000049f4c50d9bcc7afdbcf77e0d6e364c29d5a660df",
                                     "0x000000000000000000000000b0f225defec7625c6b5e43126bdde398bd90ef62"],
                            data: "0x000000000000000000000000000000000000000000000000016345785d8a0000",
                            gasPrice: "0x21E66FB00",
                            gasUsed: "0xCC4D",
                            logIndex: "0x",
                            blockNumber: "0x5778A9",
                            blockTransactionIndex: "53",
                            blockTimestamp: "1528141560")

            break
        default:
            ewm.announceLog(rid: rid,
                            hash: "0xa37bd8bd8b1fa2838ef65aec9f401f56a6279f99bb1cfb81fa84e923b1b60f2b",
                            contract: (ewm.network == EthereumNetwork.mainnet
                                ? "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"
                                : "0x7108ca7c4718efa810457f228305c9c71390931a"),
                            topics: ["0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
                                     "0x0000000000000000000000000000000000000000000000000000000000000000",
                                     "0x000000000000000000000000bdfdad139440d2db9ba2aa3b7081c2de39291508"],
                            data: "0x0000000000000000000000000000000000000000000000000000000000002328",
                            gasPrice: "0xba43b7400",
                            gasUsed: "0xc64e",
                            logIndex: "0x",
                            blockNumber: "0x1e487e",
                            blockTransactionIndex: "0x",
                            blockTimestamp: "0x59fa1ac9")
        }
    }

    func getBlocks(ewm: EthereumWalletManager, address: String, interests: UInt32, blockStart: UInt64, blockStop: UInt64, rid: Int32) {
        var blockNumbers : [UInt64] = []
        switch address.lowercased() {
        case "0xb302B06FDB1348915599D21BD54A06832637E5E8".lowercased():
            if 0 != interests & UInt32 (1 << 3) /* CLIENT_GET_BLOCKS_LOGS_AS_TARGET */ {
                blockNumbers += [4847049,
                                 4847152,
                                 4894677,
                                 4965538,
                                 4999850,
                                 5029844]
            }
            
            if 0 != interests & UInt32 (1 << 2) /* CLIENT_GET_BLOCKS_LOGS_AS_SOURCE */ {
                blockNumbers += [5705175]
            }
            
            if 0 != interests & UInt32 (1 << 1) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET */ {
                blockNumbers += [4894027,
                                 4908682,
                                 4991227]
            }
            
            if 0 != interests & UInt32 (1 << 0) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE */ {
                blockNumbers += [4894330,
                                 4894641,
                                 4894677,
                                 4903993,
                                 4906377,
                                 4997449,
                                 4999850,
                                 4999875,
                                 5000000,
                                 5705175]
            }

        case "0xa9de3dbD7d561e67527bC1Ecb025c59D53b9F7Ef".lowercased():
            if 0 != interests & UInt32 (1 << 3) /* CLIENT_GET_BLOCKS_LOGS_AS_TARGET */ {
                blockNumbers += [5506607,
                                 5877545]
            }

            if 0 != interests & UInt32 (1 << 2) /* CLIENT_GET_BLOCKS_LOGS_AS_SOURCE */ {
                blockNumbers += [5506764,
                                 5509990,
                                 5511681]
            }

            if 0 != interests & UInt32 (1 << 1) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET */ {
                blockNumbers += [5506602]
            }

            if 0 != interests & UInt32 (1 << 0) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE */ {
                blockNumbers += [5506764,
                                 5509990,
                                 5511681,
                                 5539808]
            }

        case "0xb0F225defEc7625C6B5E43126bdDE398bD90eF62".lowercased():
            if 0 != interests & UInt32 (1 << 3) /* CLIENT_GET_BLOCKS_LOGS_AS_TARGET */ {
                blockNumbers += [5732521]
            }

            if 0 != interests & UInt32 (1 << 2) /* CLIENT_GET_BLOCKS_LOGS_AS_SOURCE */ {
                blockNumbers += []
            }

            if 0 != interests & UInt32 (1 << 1) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET */ {
                blockNumbers += [6619285]
            }

            if 0 != interests & UInt32 (1 << 0) /* CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE */ {
                blockNumbers += []
            }

        default:
            blockNumbers.append(contentsOf: [blockStart,
                                             (blockStart + blockStop) / 2,
                                             blockStop])
        }

        blockNumbers = blockNumbers.filter { blockStart < $0 && $0 < blockStop }
        ewm.announceBlocks(rid: rid, blockNumbers: blockNumbers)
    }

    func getTokens(ewm: EthereumWalletManager, rid: Int32) {
        ewm.announceToken (rid: rid,
                           address: (ewm.network == EthereumNetwork.mainnet
                                ? "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"
                                : "0x7108ca7c4718efa810457f228305c9c71390931a"),
                           symbol: "BRD",
                           name: "BRD Token",
                           description: "The BRD Token",
                           decimals: 18)

        ewm.announceToken (rid: rid,
                           address: (ewm.network == EthereumNetwork.mainnet
                            ? "0x9e3359f862b6c7f5c660cfd6d1aa6909b1d9504d"
                            : "0x6e67ccd648244b3b8e2f56149b40ba8de9d79b09"),
                           symbol: "CCC",
                           name: "Container Crypto Coin",
                           description: "",
                           decimals: 18)

        ewm.announceToken (rid: rid,
                           address: "0xdd974d5c2e2928dea5f71b9825b8b646686bd200",
                           symbol: "KNC",
                           name: "KNC Token",
                           description: "",
                           decimals: 18)

    }

    func getBlockNumber(ewm: EthereumWalletManager, rid: Int32) {
        ewm.announceBlockNumber(blockNumber: "5900000", rid: rid)
    }

    func getNonce(ewm: EthereumWalletManager, address: String, rid: Int32) {
        ewm.announceNonce(address: address, nonce: "28", rid: rid)
    }

    func saveNodes(ewm: EthereumWalletManager,
                   data: Dictionary<String, String>) {
        print ("TST: saveNodes")
    }

    func saveBlocks(ewm: EthereumWalletManager,
                    data: Dictionary<String, String>) {
        print ("TST: saveBlocks")
    }

    func changeTransaction (ewm: EthereumWalletManager,
                            change: EthereumClientChangeType,
                            hash: String,
                            data: String) {
        print ("TST: changeTransaction")
    }

    func changeLog (ewm: EthereumWalletManager,
                    change: EthereumClientChangeType,
                    hash: String,
                    data: String) {
        print ("TST: changeLog")
    }


    //
    // Listener Protocol
    //
    func handleEWMEvent(ewm: EthereumWalletManager, event: EthereumEWMEvent) {
        print ("TST: EWMEvent: \(event)")
    }

    func handlePeerEvent(ewm: EthereumWalletManager, event: EthereumPeerEvent) {
        print ("TST: PeerEvent: \(event)")
    }

    //
    // Wallet Event
    //
    var walletListeners = [WalletListener]()

    func addWalletListener (listener: WalletListener) {
        walletListeners.append(listener)
    }

    func handleWalletEvent(ewm: EthereumWalletManager,
                           wallet: EthereumWallet,
                           event: EthereumWalletEvent) {
        print ("TST: WalletEvent: \(event)")
        walletListeners.forEach {
            $0.announceWalletEvent(ewm: ewm, wallet: wallet, event: event)
        }
    }

    //
    // Block Event
    //
    func handleBlockEvent(ewm: EthereumWalletManager,
                          block: EthereumBlock,
                          event: EthereumBlockEvent) {
        // Get the block(?)... then
        switch (event) {
        default: // Never here
            break;
        }
    }

    //
    // Transfer Event
    //
    var transferListeners = [EthereumWallet : TransferListener]()

    func addTransferListener (wallet: EthereumWallet,
                              listener : TransferListener) {
        transferListeners[wallet] = listener
    }

    func handleTransferEvent(ewm: EthereumWalletManager,
                             wallet: EthereumWallet,
                             transfer: EthereumTransfer,
                             event: EthereumTransferEvent) {
        print ("TST: TransferEvent: \(event), Wallet: \(transfer.amount.symbol), Hash: \(transfer.hash)")
        if let listener = transferListeners[wallet] {
            listener.announceTransferEvent(ewm: ewm, wallet: wallet, transfer: transfer, event: event)
        }
    }
}
