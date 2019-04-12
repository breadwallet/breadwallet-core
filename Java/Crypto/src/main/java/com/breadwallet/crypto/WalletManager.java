/*
 * WalletManager
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

public abstract class WalletManager {
    public interface Listener {

    }

    public interface PersistenceClient {

    }

    public interface BackendClient {

    }

    public final Account account;

    public final Network network;

    public final Listener listener;

/*
    /// The listener receives Wallet, Transfer and perhaps other asynchronous events.
    var listener: WalletManagerListener { get }

    /// The account
    var account: Account { get }

    /// The network
    var network: Network { get }

    /// The primaryWallet
    var primaryWallet: Wallet { get }

    /// The managed wallets - often will just be [primaryWallet]
    var wallets: [Wallet] { get }

    // The mode determines how the manager manages the account and wallets on network
    var mode: WalletManagerMode { get }

    // The file-system path to use for persistent storage.
    var path: String { get }  // persistent storage

    var state: WalletManagerState { get }

    #if false
    /// The default WalletFactory for creating wallets.
    var walletFactory: WalletFactory { get set }
    #endif

    /// Connect to network and begin managing wallets for account
    func connect ()

    /// Disconnect from the network.
    func disconnect ()

    /// isConnected
    /// sync(...)
    /// isSyncing

    /// sign(transfer)
    /// submit(transfer)
*/

    protected WalletManager (Listener listener, Account account, Network network) {
        this.listener = listener;
        this.account = account;
        this.network = network;
    }
}
