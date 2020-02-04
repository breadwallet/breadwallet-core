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
import com.breadwallet.crypto.errors.MigrateError;
import com.breadwallet.crypto.errors.NetworkFeeUpdateError;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.migration.BlockBlob;
import com.breadwallet.crypto.migration.PeerBlob;
import com.breadwallet.crypto.migration.TransactionBlob;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;

import java.util.List;
import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;

public interface System {

    /**
     * Create a new system.
     *
     * @param executor
     * @param listener the listener for handling events.
     * @param account the account, derived from a paper key, that will be used for all networks.
     * @param isMainnet flag to indicate if the system is for mainnet or for testnet; as blockchains
     *                  are announced, we'll filter them to be for mainent or testnet.
     * @param storagePath the path to use for persistent storage of data, such as for blocks, peers, transactions and
     *                    logs.
     * @param query the BlockchainDB query engine.
     */
    static System create(ScheduledExecutorService executor, SystemListener listener, Account account, boolean isMainnet, String storagePath, BlockchainDb query) {
        return CryptoApi.getProvider().systemProvider().create(executor, listener, account, isMainnet,storagePath, query);
    }

    /**
     * Create a BlockChainDB.Model.Currency to be used in the event that the BlockChainDB does
     * not provide its own currency model.
     *
     * @param uids the currency uids (ex: "ethereum-mainnet:0x558ec3152e2eb2174905cd19aea4e34a23de9ad6")
     * @param name the currency name (ex: "BRD Token"
     * @param code the currency code (ex: "code")
     * @param type the currency type (ex: "erc20" or "native")
     * @param decimals the number of decimals for the currency's default unit (ex: 18)
     * @return a currency mode for us with {@link #configure(List)}; {@link Optional#absent()} otherwise
     */
    static Optional<com.breadwallet.crypto.blockchaindb.models.bdb.Currency> asBlockChainDBModelCurrency(String uids,
                                                                                                         String name,
                                                                                                         String code,
                                                                                                         String type,
                                                                                                         UnsignedInteger decimals) {
        return CryptoApi.getProvider().systemProvider().asBDBCurrency(uids, name, code, type, decimals);

    }

    /**
     * Re-encrypt ciphertext blobs that were encrypted using the BRCoreKey::encryptNative routine.
     *
     * The ciphertext will be decrypted using the previous decryption routine. The plaintext from that
     * operation will be encrypted using the current {@link Cipher#encrypt(byte[])} routine with a
     * {@link Cipher#createForChaCha20Poly1305(Key, byte[], byte[])} cipher. That updated ciphertext
     * is then returned and should be used to immediately overwrite the old ciphertext blob so that
     * it can be properly decrypted using the {@link Cipher#decrypt(byte[])} routine going forward.
     *
     * @param key The cipher key
     * @param nonce12 The 12 byte nonce data
     * @param authenticatedData The authenticated data
     * @param ciphertext The ciphertext to update the encryption on
     *
     * @return The updated ciphertext, if decryption and re-encryption succeeds; absent otherwise.
     */
    static Optional<byte[]> migrateBRCoreKeyCiphertext(Key key, byte[] nonce12, byte[] authenticatedData, byte[] ciphertext) {
        return CryptoApi.getProvider().systemProvider().migrateBRCoreKeyCiphertext(key, nonce12, authenticatedData, ciphertext);
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
     * within `storagePath` except for a specified array of systems to preserve.  Generally, this
     * function should be called on startup after all systems have been created.  When called at
     * that time, any 'left over' systems will have their persistent storeage wiped.
     *
     * Note: This function will perform no action if `storagePath` does not exist or is
     *       not a directory.
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
     * Remove (aka 'wipe') the persistent storage associated with `network` at `path`.
     *
     * This should be used solely to recover from a failure of `createWalletManager`.  A failure
     * to create a wallet manager is most likely due to corruption of the persistently stored data
     * and the only way to recover is to wipe that data.
     *
     * @param network the network to wipe data for
     */
    void wipe(Network network);

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
     * Callers must use the {@link WalletManager#connect(NetworkPeer)}/{@link WalletManager#disconnect()} methods to
     * change a WalletManager's connectivity state. Instead, WalletManagers MAY consult this flag when performing
     * network operations to determine viability.
     */
    void setNetworkReachable(boolean isNetworkReachable);

    Account getAccount();

    String getPath();

    List<? extends Network> getNetworks();

    List<? extends WalletManager> getWalletManagers();

    List<? extends Wallet> getWallets();

    /**
     * If migration is required, return the currency code; otherwise, return nil.
     *
     * Note: it is not an error not to migrate.
     */
    boolean migrateRequired(Network network);

    /**
     * Migrate the storage for a network given transaction, block and peer blobs.
     *
     * Support for persistent storage migration to allow prior App versions to migrate their SQLite
     * database representations of BTC/BTC transations, blocks and peers into 'Generic Crypto' - where
     * these entities are persistently
     *
     * The provided blobs must be consistent with `network`.  For exmaple, if `network` represents BTC or BCH
     * then the blobs must be of type {@link TransactionBlob.Btc}; otherwise a MigrateError is thrown.
     */
    void migrateStorage (Network network, List<TransactionBlob> transactionBlobs, List<BlockBlob> blockBlobs,
                         List<PeerBlob> peerBlobs) throws MigrateError;
}
