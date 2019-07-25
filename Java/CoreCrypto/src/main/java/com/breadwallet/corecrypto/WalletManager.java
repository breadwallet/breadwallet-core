package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.BRCryptoCWMClient;
import com.breadwallet.corenative.crypto.BRCryptoCWMListener;
import com.breadwallet.corenative.crypto.CoreBRCryptoWallet;
import com.breadwallet.corenative.crypto.CoreBRCryptoWalletManager;
import com.breadwallet.crypto.AddressScheme;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.WalletManagerState;
import com.google.common.base.Optional;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

/* package */
final class WalletManager implements com.breadwallet.crypto.WalletManager {

    /* package */
    static AddressScheme defaultAddressScheme(com.breadwallet.crypto.Currency currency) {
        String code = currency.getCode();
        if (code.equals(com.breadwallet.crypto.Currency.CODE_AS_BTC)) {
            return AddressScheme.BTC_SEGWIT;

        } else if (code.equals(com.breadwallet.crypto.Currency.CODE_AS_BCH)) {
            return AddressScheme.BTC_LEGACY;

        } else if (code.equals(com.breadwallet.crypto.Currency.CODE_AS_ETH)) {
            return AddressScheme.ETH_DEFAULT;

        } else {
            return AddressScheme.GEN_DEFAULT;
        }
    }

    /* package */
    static List<AddressScheme> defaultAddressSchemes(com.breadwallet.crypto.Currency currency) {
        String code = currency.getCode();
        if (code.equals(com.breadwallet.crypto.Currency.CODE_AS_BTC)) {
            return Arrays.asList(AddressScheme.BTC_SEGWIT, AddressScheme.BTC_LEGACY);

        } else if (code.equals(com.breadwallet.crypto.Currency.CODE_AS_BCH)) {
            return Collections.singletonList(AddressScheme.BTC_LEGACY);

        } else if (code.equals(com.breadwallet.crypto.Currency.CODE_AS_ETH)) {
            return Collections.singletonList(AddressScheme.ETH_DEFAULT);

        } else {
            return Collections.singletonList(AddressScheme.GEN_DEFAULT);
        }
    }

    /* package */
    static WalletManager create(BRCryptoCWMListener.ByValue listener, BRCryptoCWMClient.ByValue client,
                                Account account, Network network, WalletManagerMode mode, AddressScheme addressScheme,
                                String path, System system) {
        CoreBRCryptoWalletManager core = CoreBRCryptoWalletManager.create(
                listener,
                client,
                account.getCoreBRCryptoAccount(),
                network.getCoreBRCryptoNetwork(),
                Utilities.walletManagerModeToCrypto(mode),
                Utilities.addressSchemeToCrypto(addressScheme),
                path);

        return new WalletManager(core, system);
    }

    /* package */
    static WalletManager create(BRCryptoCWMListener.ByValue listener, BRCryptoCWMClient.ByValue client,
                                Account account, Network network, WalletManagerMode mode,
                                String path, System system) {
        return create(listener, client, account, network, mode, defaultAddressScheme(network.getCurrency()), path, system);
    }

    /* package */
    static WalletManager create(CoreBRCryptoWalletManager core, System system) {
        return new WalletManager(core, system);
    }

    private final System system;
    private final Account account;
    private final Network network;
    private final Currency networkCurrency;
    private final Unit networkBaseUnit;
    private final Unit networkDefaultUnit;
    private final String path;
    private final WalletManagerMode mode;
    private final NetworkFee networkFee;

    private CoreBRCryptoWalletManager core;

    private WalletManager(CoreBRCryptoWalletManager core, System system) {
        this.system = system;
        this.account = Account.create(core.getAccount());
        this.network = Network.create(core.getNetwork());
        this.networkCurrency = network.getCurrency();
        this.mode =  Utilities.walletManagerModeFromCrypto(core.getMode());
        this.path = core.getPath();

        // TODO(fix): Unchecked get here
        this.networkBaseUnit = network.baseUnitFor(networkCurrency).get();
        this.networkDefaultUnit = network.defaultUnitFor(networkCurrency).get();
        this.networkFee = network.getMinimumFee();

        this.core = core;
    }

    @Override
    public void connect() {
        core.connect();
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
    public void submit(com.breadwallet.crypto.Transfer transfer, byte[] phraseUtf8) {
        Transfer cryptoTransfer = Transfer.from(transfer);
        Wallet cryptoWallet = cryptoTransfer.getWallet();
        core.submit(cryptoWallet.getCoreBRCryptoWallet(), cryptoTransfer.getCoreBRCryptoTransfer(), phraseUtf8);
    }

    @Override
    public boolean isActive() {
        WalletManagerState state = getState();
        return state == WalletManagerState.CREATED || state == WalletManagerState.SYNCING;
    }

    @Override
    public System getSystem() {
        return system;
    }

    @Override
    public Account getAccount() {
        return account;
    }

    @Override
    public Network getNetwork() {
        return network;
    }

    @Override
    public Wallet getPrimaryWallet() {
        return Wallet.create(core.getWallet(), this);
    }

    @Override
    public List<Wallet> getWallets() {
        List<Wallet> wallets = new ArrayList<>();

        for (CoreBRCryptoWallet wallet: core.getWallets()) {
            wallets.add(Wallet.create(wallet, this));
        }

        return wallets;
    }

    @Override
    public WalletManagerMode getMode() {
        return mode;
    }

    @Override
    public String getPath() {
        return path;
    }

    @Override
    public Currency getCurrency() {
        return networkCurrency;
    }

    @Override
    public String getName() {
        return networkCurrency.getCode();
    }

    @Override
    public Unit getBaseUnit() {
        return networkBaseUnit;
    }

    @Override
    public Unit getDefaultUnit() {
        return networkDefaultUnit;
    }

    @Override
    public NetworkFee getDefaultNetworkFee() {
        return networkFee;
    }

    @Override
    public WalletManagerState getState() {
        return Utilities.walletManagerStateFromCrypto(core.getState());
    }

    @Override
    public void setAddressScheme(AddressScheme scheme) {
        core.setAddressScheme(Utilities.addressSchemeToCrypto(scheme));
    }

    @Override
    public AddressScheme getAddressScheme() {
        return Utilities.addressSchemeFromCrypto(core.getAddressScheme());
    }

    @Override
    public List<AddressScheme> getAddressSchemes() {
        return defaultAddressSchemes(networkCurrency);
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

    /* package */
    Optional<Wallet> getWallet(CoreBRCryptoWallet wallet) {
        return core.containsWallet(wallet) ?
                Optional.of(Wallet.create(wallet, this)):
                Optional.absent();
    }

    /* package */
    Optional<Wallet> getWalletOrCreate(CoreBRCryptoWallet wallet) {
        Optional<Wallet> optional = getWallet(wallet);
        if (optional.isPresent()) {
            return optional;

        } else {
            return Optional.of(Wallet.create(wallet, this));
        }
    }
}
