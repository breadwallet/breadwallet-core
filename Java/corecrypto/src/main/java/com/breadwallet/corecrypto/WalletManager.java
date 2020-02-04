/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoCWMClient;
import com.breadwallet.corenative.crypto.BRCryptoCWMListener;
import com.breadwallet.corenative.crypto.BRCryptoWallet;
import com.breadwallet.corenative.crypto.BRCryptoWalletManager;
import com.breadwallet.crypto.AddressScheme;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.WalletManagerState;
import com.breadwallet.crypto.WalletManagerSyncDepth;
import com.breadwallet.crypto.errors.WalletSweeperError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

import static com.google.common.base.Preconditions.checkState;

/* package */
final class WalletManager implements com.breadwallet.crypto.WalletManager {

    /* package */
    static void wipe(Network network, String storagePath) {
        BRCryptoWalletManager.wipe(network.getCoreBRCryptoNetwork(), storagePath);
    }

    /* package */
    static Optional<WalletManager> create(BRCryptoCWMListener listener,
                                          BRCryptoCWMClient client,
                                          Account account,
                                          Network network,
                                          WalletManagerMode mode,
                                          AddressScheme addressScheme,
                                          String storagePath,
                                          System system,
                                          SystemCallbackCoordinator callbackCoordinator) {
        return BRCryptoWalletManager.create(
                listener,
                client,
                account.getCoreBRCryptoAccount(),
                network.getCoreBRCryptoNetwork(),
                Utilities.walletManagerModeToCrypto(mode),
                Utilities.addressSchemeToCrypto(addressScheme),
                storagePath
        ).transform(
                cwm -> WalletManager.create(cwm, system, callbackCoordinator)
        );
    }

    /* package */
    static WalletManager takeAndCreate(BRCryptoWalletManager core, System system, SystemCallbackCoordinator callbackCoordinator) {
        return WalletManager.create(core.take(), system, callbackCoordinator);
    }

    /* package */
    static WalletManager create(BRCryptoWalletManager core, System system, SystemCallbackCoordinator callbackCoordinator) {
        WalletManager manager = new WalletManager(core, system, callbackCoordinator);
        ReferenceCleaner.register(manager, core::give);
        return manager;
    }

    private BRCryptoWalletManager core;
    private final System system;
    private final SystemCallbackCoordinator callbackCoordinator;

    private final Supplier<Account> accountSupplier;
    private final Supplier<Network> networkSupplier;
    private final Supplier<Currency> networkCurrencySupplier;
    private final Supplier<String> pathSupplier;
    private final Supplier<NetworkFee> networkFeeSupplier;
    private final Supplier<Unit> networkBaseUnitSupplier;
    private final Supplier<Unit> networkDefaultUnitSupplier;

    private WalletManager(BRCryptoWalletManager core, System system, SystemCallbackCoordinator callbackCoordinator) {
        this.core = core;
        this.system = system;
        this.callbackCoordinator = callbackCoordinator;

        this.accountSupplier = Suppliers.memoize(() -> Account.create(core.getAccount()));
        this.networkSupplier = Suppliers.memoize(() -> Network.create(core.getNetwork()));
        this.networkCurrencySupplier = Suppliers.memoize(() -> getNetwork().getCurrency());
        this.pathSupplier = Suppliers.memoize(core::getPath);

        this.networkFeeSupplier = Suppliers.memoize(() -> getNetwork().getMinimumFee());
        this.networkBaseUnitSupplier = Suppliers.memoize(() -> {
            Optional<Unit> maybeUnit = getNetwork().baseUnitFor(getCurrency());
            checkState(maybeUnit.isPresent());
            return maybeUnit.get();
        });
        this.networkDefaultUnitSupplier = Suppliers.memoize(() -> {
            Optional<Unit> maybeUnit = getNetwork().defaultUnitFor(getCurrency());
            checkState(maybeUnit.isPresent());
            return maybeUnit.get();
        });
    }

    @Override
    public void createSweeper(com.breadwallet.crypto.Wallet wallet,
                              com.breadwallet.crypto.Key key,
                              CompletionHandler<com.breadwallet.crypto.WalletSweeper, WalletSweeperError> completion) {
        WalletSweeper.create(this, Wallet.from(wallet), Key.from(key), system.getBlockchainDb(), completion);
    }

    @Override
    public void connect(@Nullable com.breadwallet.crypto.NetworkPeer peer) {
        checkState(null == peer || getNetwork().equals(peer.getNetwork()));
        core.connect(peer == null ? null : NetworkPeer.from(peer).getBRCryptoPeer());
    }

    @Override
    public void disconnect() {
        core.disconnect();
    }

    @Override
    public void sync() {
        core.sync();
    }

    @Override
    public void stop() {
        core.stop();
    }

    @Override
    public void syncToDepth(WalletManagerSyncDepth depth) {
        core.syncToDepth(Utilities.syncDepthToCrypto(depth));
    }

    /* package */
    boolean sign(com.breadwallet.crypto.Transfer transfer, byte[] phraseUtf8) {
        Transfer cryptoTransfer = Transfer.from(transfer);
        Wallet cryptoWallet = cryptoTransfer.getWallet();
        return core.sign(cryptoWallet.getCoreBRCryptoWallet(), cryptoTransfer.getCoreBRCryptoTransfer(), phraseUtf8);
    }

    @Override
    public void submit(com.breadwallet.crypto.Transfer transfer, byte[] phraseUtf8) {
        Transfer cryptoTransfer = Transfer.from(transfer);
        Wallet cryptoWallet = cryptoTransfer.getWallet();
        core.submit(cryptoWallet.getCoreBRCryptoWallet(), cryptoTransfer.getCoreBRCryptoTransfer(), phraseUtf8);
    }

    /* package */
    void submit(com.breadwallet.crypto.Transfer transfer, Key key) {
        Transfer cryptoTransfer = Transfer.from(transfer);
        Wallet cryptoWallet = cryptoTransfer.getWallet();
        core.submit(cryptoWallet.getCoreBRCryptoWallet(), cryptoTransfer.getCoreBRCryptoTransfer(), key.getBRCryptoKey());
    }

    /* package */
    void submit(com.breadwallet.crypto.Transfer transfer) {
        Transfer cryptoTransfer = Transfer.from(transfer);
        Wallet cryptoWallet = cryptoTransfer.getWallet();
        core.submit(cryptoWallet.getCoreBRCryptoWallet(), cryptoTransfer.getCoreBRCryptoTransfer());
    }

    @Override
    public boolean isActive() {
        WalletManagerState state = getState();
        WalletManagerState.Type type = state.getType();
        return type == WalletManagerState.Type.CREATED || type == WalletManagerState.Type.SYNCING;
    }

    @Override
    public System getSystem() {
        return system;
    }

    @Override
    public Account getAccount() {
        return accountSupplier.get();
    }

    @Override
    public Network getNetwork() {
        return networkSupplier.get();
    }

    @Override
    public Wallet getPrimaryWallet() {
        return Wallet.create(core.getWallet(), this, callbackCoordinator);
    }

    @Override
    public List<Wallet> getWallets() {
        List<Wallet> wallets = new ArrayList<>();

        for (BRCryptoWallet wallet: core.getWallets()) {
            wallets.add(Wallet.create(wallet, this, callbackCoordinator));
        }

        return wallets;
    }

    @Override
    public Optional<Wallet> registerWalletFor(com.breadwallet.crypto.Currency currency) {
        checkState(getNetwork().hasCurrency(currency));
        return core
                .registerWallet(Currency.from(currency).getCoreBRCryptoCurrency())
                .transform(w -> Wallet.create(w, this, callbackCoordinator));
    }

    @Override
    public WalletManagerMode getMode() {
        return Utilities.walletManagerModeFromCrypto(core.getMode());
    }

    @Override
    public void setMode(WalletManagerMode mode) {
        core.setMode(Utilities.walletManagerModeToCrypto(mode));
    }

    @Override
    public String getPath() {
        return pathSupplier.toString();
    }

    @Override
    public Currency getCurrency() {
        return networkCurrencySupplier.get();
    }

    @Override
    public String getName() {
        return getCurrency().getCode();
    }

    @Override
    public Unit getBaseUnit() {
        return networkBaseUnitSupplier.get();
    }

    @Override
    public Unit getDefaultUnit() {
        return networkDefaultUnitSupplier.get();
    }

    @Override
    public NetworkFee getDefaultNetworkFee() {
        return networkFeeSupplier.get();
    }

    @Override
    public WalletManagerState getState() {
        return Utilities.walletManagerStateFromCrypto(core.getState());
    }

    @Override
    public void setAddressScheme(AddressScheme scheme) {
        checkState(getNetwork().supportsAddressScheme(scheme));
        core.setAddressScheme(Utilities.addressSchemeToCrypto(scheme));
    }

    @Override
    public AddressScheme getAddressScheme() {
        return Utilities.addressSchemeFromCrypto(core.getAddressScheme());
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof WalletManager)) {
            return false;
        }

        WalletManager that = (WalletManager) object;
        return core.equals(that.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }

    @Override
    public String toString() {
        return getName();
    }

    /* package */
    void setNetworkReachable(boolean isNetworkReachable) {
        core.setNetworkReachable(isNetworkReachable);
    }

    /* package */
    Optional<Wallet> getWallet(BRCryptoWallet wallet) {
        return core.containsWallet(wallet) ?
                Optional.of(Wallet.takeAndCreate(wallet, this, callbackCoordinator)):
                Optional.absent();
    }

    /* package */
    Wallet createWallet(BRCryptoWallet wallet) {
        return Wallet.takeAndCreate(wallet, this, callbackCoordinator);
    }

    /* package */
    BRCryptoWalletManager getCoreBRCryptoWalletManager() {
        return core;
    }
}
