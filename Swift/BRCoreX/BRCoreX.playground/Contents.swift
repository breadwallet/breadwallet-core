import UIKit
import BRCoreX

var str = "Hello, playground"

10

var btc = Currency.bitcoin

///
/// MARK: - App Implementation
///

public class BaseWalletManagerClient : WalletManagerClient {
    public func handleManagerEvent(manager: WalletManager,
                                   event: WalletManagerEvent) {
        print ("UI Handling WalletManager Event")
    }

    public func handleWalletEvent (manager: WalletManager,
                                   wallet: Wallet,
                                   event: WalletEvent) -> Void {
        print ("UI Handling Wallet Event")
    }

    public func handleTransferEvent (manager: WalletManager,
                                     wallet: Wallet,
                                     transfer: Transfer,
                                     event: TransferEvent) -> Void {
        print ("UI Handling Transfer Event")
    }
}

var bwmc = BaseWalletManagerClient()

/// =============================================================================================
/*
class BRDBitcoinWalletManagerClient : BaseWalletManagerClient,  BitcoinWalletManagerClient {}
class BRDEthereumWalletManagerClient : BaseWalletManagerClient,  EthereumWalletManagerClient {}

var walletManagers : [WalletManager] = [
    //    bitcoinWalletManager (client: BRDBitcoinWalletManagerClient()),
    //    ethereumWalletManager (client: BRDEthereumWalletManagerClient())
]

var wallets = walletManagers.flatMap { $0.wallets }
*/

/*
 ///
 /// MARK: - Bitcoin
 ///
 public protocol BitcoinWalletManagerClient : WalletManagerClient {}

 public class BitcoinClient : BaseWalletManagerClient, BitcoinWalletManagerClient {
 }

 public class BitcoinWalletManager : WalletManager {
 override init (client: WalletManagerClient) {
 assert (client is BitcoinClient)
 super.init(client: client)
 }
 }

 ///
 /// MARK: - Ethereum
 ///
 public protocol EthereumWalletManagerClient : WalletManagerClient {
 func getNonce (manager: WalletManager,
 address: String,
 rid: Int32) -> Void
 }

 public class EthereumClient : BaseWalletManagerClient, EthereumWalletManagerClient {
 public func getNonce (manager: WalletManager,
 address: String,
 rid: Int32) -> Void {
 print ("nonce")
 }
 }

 public class EthereumWalletManager : WalletManager {
 override init (client: WalletManagerClient) {
 assert (client is BitcoinClient)
 super.init(client: client)
 }
 }
 */

