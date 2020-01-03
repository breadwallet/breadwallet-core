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
import com.breadwallet.corenative.crypto.BRCryptoAddress;
import com.breadwallet.corenative.crypto.BRCryptoAddressScheme;
import com.breadwallet.corenative.crypto.BRCryptoCurrency;
import com.breadwallet.corenative.crypto.BRCryptoNetwork;
import com.breadwallet.corenative.crypto.BRCryptoNetworkCanonicalType;
import com.breadwallet.corenative.crypto.BRCryptoNetworkFee;
import com.breadwallet.corenative.crypto.BRCryptoSyncMode;
import com.breadwallet.crypto.AddressScheme;
import com.breadwallet.crypto.NetworkType;
import com.breadwallet.crypto.WalletManagerMode;
import com.google.common.base.Optional;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Objects;
import java.util.Set;

import static com.google.common.base.Preconditions.checkState;

/* package */
final class Network implements com.breadwallet.crypto.Network {

    /* package */
    static Optional<Network> findBuiltin (String uids) {
        return BRCryptoNetwork.findBuiltin(uids)
                .transform(Network::create);
    }

    static List<Network> installBuiltins () {
        List<BRCryptoNetwork> cores = BRCryptoNetwork.installBuiltins();
        List<Network> builtins = new ArrayList<>();
        for (BRCryptoNetwork core: cores) {
            builtins.add (Network.create(core));
        }
        return builtins;
    }

    static Network create(BRCryptoNetwork core) {
        Network network = new Network(core);
        ReferenceCleaner.register(network, core::give);
        return network;
    }

    /* package */
    static Network from(com.breadwallet.crypto.Network network) {
        if (network == null) {
            return null;
        }

        if (network instanceof Network) {
            return (Network) network;
        }

        throw new IllegalArgumentException("Unsupported network instance");
    }

    private final BRCryptoNetwork core;

    private final Supplier<String> uidsSupplier;
    private final Supplier<String> nameSupplier;
    private final Supplier<Boolean> isMainnetSupplier;
    private final Supplier<NetworkType> typeSupplier;
    private final Supplier<Currency> currencySupplier;
    private final Supplier<Set<Currency>> currenciesSupplier;

    private Network(BRCryptoNetwork core) {
        this.core = core;

        uidsSupplier = Suppliers.memoize(core::getUids);
        nameSupplier = Suppliers.memoize(core::getName);
        isMainnetSupplier = Suppliers.memoize(core::isMainnet);
        typeSupplier = Suppliers.memoize(() -> Utilities.networkTypeFromCrypto(core.getCanonicalType()));
        currencySupplier = Suppliers.memoize(() -> Currency.create(core.getCurrency()));

        currenciesSupplier = Suppliers.memoize(() -> {
            Set<Currency> currencies = new HashSet<>();
            UnsignedLong count = core.getCurrencyCount();
            for (UnsignedLong i = UnsignedLong.ZERO; i.compareTo(count) < 0; i = i.plus(UnsignedLong.ONE)) {
                currencies.add(Currency.create(core.getCurrency(i)));
            }
            return currencies;
        });
    }

    @Override
    public String getUids() {
        return uidsSupplier.get();
    }

    @Override
    public String getName() {
        return nameSupplier.get();
    }

    @Override
    public boolean isMainnet() {
        return isMainnetSupplier.get();
    }

    @Override
    public NetworkType getType() {
        return typeSupplier.get();
    }

    @Override
    public UnsignedLong getHeight() {
        return core.getHeight();
    }

    @Override
    public UnsignedInteger getConfirmationsUntilFinal() {
        return core.getConfirmationsUntilFinal();
    }

    @Override
    public Optional<NetworkPeer> createPeer(String address, UnsignedInteger port, @Nullable String publicKey) {
        return NetworkPeer.create(this, address, port, publicKey);
    }

    @Override
    public Currency getCurrency() {
        return currencySupplier.get();
    }

    @Override
    public Set<Currency> getCurrencies() {
        return new HashSet<>(currenciesSupplier.get());
    }

    @Override
    public Optional<Currency> getCurrencyByCode(String code) {
        for (Currency currency: getCurrencies()) {
            if (code.equals(currency.getCode())) {
                return Optional.of(currency);
            }
        }
        return Optional.absent();
    }

    @Override
    public Optional<Currency> getCurrencyByIssuer(String issuer) {
        String issuerLowercased = issuer.toLowerCase(Locale.ROOT);
        for (Currency currency: getCurrencies()) {
            if (issuerLowercased.equals(currency.getIssuer().orNull())) {
                return Optional.of(currency);
            }
        }
        return Optional.absent();
    }

    @Override
    public List<? extends NetworkFee> getFees() {
        List<NetworkFee> fees = new ArrayList<>();
        for (BRCryptoNetworkFee fee: core.getFees()) {
            fees.add(NetworkFee.create(fee));
        }
        return fees;
    }

    @Override
    public NetworkFee getMinimumFee() {
        NetworkFee minimumFee = null;
        for (NetworkFee fee: getFees()) {
            if (minimumFee == null || fee.getConfirmationTimeInMilliseconds().compareTo(minimumFee.getConfirmationTimeInMilliseconds()) > 0) {
                minimumFee = fee;
            }
        }
        return minimumFee;
    }

    @Override
    public boolean hasCurrency(com.breadwallet.crypto.Currency currency) {
        return core.hasCurrency(Currency.from(currency).getCoreBRCryptoCurrency());
    }

    @Override
    public Optional<Unit> baseUnitFor(com.breadwallet.crypto.Currency currency) {
        if (!hasCurrency(currency)) {
            return Optional.absent();
        }
        return core.getUnitAsBase(Currency.from(currency).getCoreBRCryptoCurrency()).transform(Unit::create);
    }

    @Override
    public Optional<Unit> defaultUnitFor(com.breadwallet.crypto.Currency currency) {
        if (!hasCurrency(currency)) {
            return Optional.absent();
        }
        return core.getUnitAsDefault(Currency.from(currency).getCoreBRCryptoCurrency()).transform(Unit::create);
    }

    @Override
    public Optional<Set<? extends com.breadwallet.crypto.Unit>> unitsFor(com.breadwallet.crypto.Currency currency) {
        if (!hasCurrency(currency)) {
            return Optional.absent();
        }

        Set<Unit> units = new HashSet<>();

        BRCryptoCurrency currencyCore = Currency.from(currency).getCoreBRCryptoCurrency();
        UnsignedLong count = core.getUnitCount(currencyCore);

        for (UnsignedLong i = UnsignedLong.ZERO; i.compareTo(count) < 0; i = i.plus(UnsignedLong.ONE)) {
            Optional<Unit> unit = core.getUnitAt(currencyCore, i).transform(Unit::create);
            if (!unit.isPresent()) {
                return Optional.absent();
            }

            units.add(unit.get());
        }

        return Optional.of(units);
    }

    @Override
    public Optional<Boolean> hasUnitFor(com.breadwallet.crypto.Currency currency, com.breadwallet.crypto.Unit unit) {
        return unitsFor(currency).transform(input -> input.contains(unit));
    }

    public void addCurrency(com.breadwallet.crypto.Currency currency,
                            com.breadwallet.crypto.Unit baseUnit,
                            com.breadwallet.crypto.Unit defaultUnit) {
        checkState (baseUnit.hasCurrency(currency));
        checkState (defaultUnit.hasCurrency(currency));
        if (!this.hasCurrency(currency)) {
            getCoreBRCryptoNetwork().addCurrency (
                    Currency.from(currency).getCoreBRCryptoCurrency(),
                    Unit.from(baseUnit).getCoreBRCryptoUnit(),
                    Unit.from(defaultUnit).getCoreBRCryptoUnit()
            );
        }
    }

    public void addUnitFor(com.breadwallet.crypto.Currency currency,
                           com.breadwallet.crypto.Unit unit) {
        checkState (unit.hasCurrency(currency));
        checkState (this.hasCurrency(currency));
        if (!this.hasUnitFor (currency, unit).isPresent()) {
            getCoreBRCryptoNetwork().addCurrencyUnit(
                    Currency.from(currency).getCoreBRCryptoCurrency(),
                    Unit.from(unit).getCoreBRCryptoUnit()
            );
        }
    }

    @Override
    public Optional<Address> addressFor(String address) {
        return Address.create(address, this);
    }

    @Override
    public AddressScheme getDefaultAddressScheme() {
        return Utilities.addressSchemeFromCrypto (getCoreBRCryptoNetwork().getDefaultAddressScheme());
    }

    @Override
    public List<AddressScheme> getSupportedAddressSchemes() {
        List<AddressScheme> schemes = new ArrayList<>();
        for (BRCryptoAddressScheme c: getCoreBRCryptoNetwork().getSupportedAddressSchemes())
            schemes.add(Utilities.addressSchemeFromCrypto(c));
        return schemes;
    }

    @Override
    public boolean supportsAddressScheme(AddressScheme addressScheme) {
        return getCoreBRCryptoNetwork().supportsAddressScheme(Utilities.addressSchemeToCrypto(addressScheme));
    }

    @Override
    public WalletManagerMode getDefaultWalletManagerMode() {
        return Utilities.walletManagerModeFromCrypto (getCoreBRCryptoNetwork().getDefaultSyncMode());
    }

    @Override
    public List<WalletManagerMode> getSupportedWalletManagerModes() {
        List<WalletManagerMode> modes = new ArrayList<>();
        for (BRCryptoSyncMode m: getCoreBRCryptoNetwork().getSupportedSyncModes())
            modes.add (Utilities.walletManagerModeFromCrypto(m));
        return modes;
    }

    @Override
    public boolean supportsWalletManagerMode(WalletManagerMode mode) {
        return getCoreBRCryptoNetwork().supportsSyncMode (Utilities.walletManagerModeToCrypto(mode));
    }

    @Override
    public boolean requiresMigration() {
        return getCoreBRCryptoNetwork().requiresMigration();
    }

    @Override
    public String toString() {
        return getName();
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof Network)) {
            return false;
        }

        Network network = (Network) object;
        return getUids().equals(network.getUids());
    }

    @Override
    public int hashCode() {
        return Objects.hash(getUids());
    }

    /* package */
    void setHeight(UnsignedLong height) {
        core.setHeight(height);
    }

    /* package */
    void setFees(List<NetworkFee> fees) {
        checkState(!fees.isEmpty());
        List<BRCryptoNetworkFee> cryptoFees = new ArrayList<>(fees.size());
        for (NetworkFee fee: fees) {
            cryptoFees.add(fee.getCoreBRCryptoNetworkFee());
        }
        core.setFees(cryptoFees);
    }

    /* package */
    BRCryptoNetwork getCoreBRCryptoNetwork() {
        return core;
    }
}
