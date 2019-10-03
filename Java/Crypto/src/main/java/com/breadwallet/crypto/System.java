/*
 * System
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.errors.NetworkFeeUpdateError;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.utility.CompletionHandler;

import java.util.List;
import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;

public interface System {

    static System create(ScheduledExecutorService executor, SystemListener listener, Account account, boolean isMainnet, String storagePath, BlockchainDb query) {
        return CryptoApi.getProvider().systemProvider().create(executor, listener, account, isMainnet,storagePath, query);
    }

    /**
     * Cease use of `system` and remove (aka 'wipe') its persistent storage.
     *
     * Caution is highly warranted; none of the System's references, be they Wallet Managers,
     * Wallets, Transfers, etc. should be *touched* once the system is wiped.
     *
     * Note: This function blocks until completed.  Be sure that all references are dereferenced
     *       *before* invoking this function and remove the reference to `system` after this
     *       returns.
     */
    static void wipe(System system) {
        CryptoApi.getProvider().systemProvider().wipe(system);
    }

    /**
     * Remove (aka 'wipe') the persistent storage associated with any and all systems located
     * within `atPath` except for a specified array of systems to preserve.  Generally, this
     * function should be called on startup after all systems have been created.  When called at
     * that time, any 'left over' systems will have their persistent storeage wiped.
     *
     * @param storagePath the file system path where system data is persistently stored
     * @param exemptSystems the list of systems that should not have their data wiped.
     */
    static void wipeAll(String storagePath, List<System> exemptSystems) {
        CryptoApi.getProvider().systemProvider().wipeAll(storagePath, exemptSystems);;
    }

    /**
     * Configure the system.  This will query various BRD services, notably the BlockChainDB, to
     * establish the available networks (aka blockchains) and their currencies.  For each
     * `Network` there will be `SystemEvent` which can be used by the App to create a
     * `WalletManager`.
     *
     * @param appCurrencies If the BlockChainDB does not return any currencies, then
     *                      use `applicationCurrencies` merged into the defaults.
     */
    void configure(List<com.breadwallet.crypto.blockchaindb.models.bdb.Currency> appCurrencies);

    /**
     * Create a wallet manager for `network` using `mode.
     *
     * Note: There are two preconditions - "network" must support "mode" and "addressScheme".
     *       Thus a fatal error arises if, for example, the network is BTC and the scheme is ETH.
     *
     * @param network the wallet manager's network
     * @param mode the wallet manager mode to use
     * @param addressScheme the address scheme to use
     * @param currencies the currencies to register.  A wallet will be created for each one.  It
     *                   is safe to pass currencies not in "network" as they will be filtered (but bad form
     *                   to do so). The "primaryWallet", for the network's currency, is always created; if
     *                   the primaryWallet's currency is in `currencies` then it is effectively ignored.
     *
     * @return true on success; false on failure.
     */
    boolean createWalletManager(Network network,
                                WalletManagerMode mode,
                                AddressScheme addressScheme,
                                Set<Currency> currencies);

    /**
     * Connect all wallet managers.
     *
     * They will be connected w/o an explict NetworkPeer.
     */
    void connectAll();

    /**
     * Disconnect all wallet managers.
     */
    void disconnectAll();

    void subscribe(String subscriptionToken);

    /**
     * Update the NetworkFees for all known networks.  This will query the `BlockChainDB` to
     * acquire the fee information and then update each of system's networks with the new fee
     * structure.  Each updated network will generate a NetworkEvent.feesUpdated event (even if
     * the actual fees did not change).
     *
     * And optional completion handler can be provided.  If provided the completion handler is
     * invoked with an array of the networks that were updated or with an error.
     *
     * It is appropriate to call this function anytime a network's fees are to be used, such as
     * when a transfer is created and the User can choose among the different fees.
     *
     * @param completion An optional completion handler
     */
    void updateNetworkFees(@Nullable CompletionHandler<List<Network>, NetworkFeeUpdateError> completion);

    /**
     * Set the network reachable flag for all managers.
     *
     * Setting or clearing this flag will NOT result in a connect/disconnect attempt by a {@link WalletManager}.
     * Callers must use the {@link WalletManager#connect()}/{@link WalletManager#disconnect()} methods to
     * change a WalletManager's connectivity state. Instead, WalletManagers MAY consult this flag when performing
     * network operations to determine viability.
     */
    void setNetworkReachable(boolean isNetworkReachable);

    Account getAccount();

    String getPath();

    List<? extends Network> getNetworks();

    List<? extends WalletManager> getWalletManagers();

    List<? extends Wallet> getWallets();

    AddressScheme getDefaultAddressScheme(Network network);

    List<AddressScheme> getSupportedAddressSchemes(Network network);

    boolean supportsAddressScheme(Network network, AddressScheme addressScheme);

    WalletManagerMode getDefaultWalletManagerMode(Network network);

    List<WalletManagerMode> getSupportedWalletManagerModes(Network network);

    boolean supportsWalletManagerMode(Network network, WalletManagerMode mode);
}
