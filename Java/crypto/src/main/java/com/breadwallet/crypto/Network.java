/*
 * Network
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.lang.String;
import java.util.List;
import java.util.Set;

public interface Network {

    Optional<? extends Unit> baseUnitFor(Currency currency);

    Optional<? extends Unit> defaultUnitFor(Currency currency);

    Optional<Set<? extends Unit>> unitsFor(Currency currency);

    /**
     * @deprecated Replace with {@link Address#create(String, Network)}
     */
    @Deprecated
    Optional<? extends Address> addressFor(String address);

    Optional<Boolean> hasUnitFor(Currency currency, Unit unit);

    boolean hasCurrency(Currency currency);

    Currency getCurrency();

    Set<? extends Currency> getCurrencies();

    Optional<? extends Currency> getCurrencyByCode(String code);

    Optional<? extends Currency> getCurrencyByIssuer(String issuer);

    List<? extends NetworkFee> getFees();

    NetworkFee getMinimumFee();

    String getUids();

    String getName();

    boolean isMainnet();

    NetworkType getType();

    UnsignedLong getHeight();

    UnsignedInteger getConfirmationsUntilFinal();

    AddressScheme getDefaultAddressScheme();

    List<AddressScheme> getSupportedAddressSchemes();

    boolean supportsAddressScheme(AddressScheme addressScheme);

    WalletManagerMode getDefaultWalletManagerMode();

    List<WalletManagerMode> getSupportedWalletManagerModes();

    boolean supportsWalletManagerMode(WalletManagerMode mode);

    boolean requiresMigration();
    /**
     * Create a Network Peer for use in P2P modes when a WalletManager connects.
     *
     * @param address An numeric-dot-notation IP address
     * @param port A port number
     * @param publicKey An optional public key
     * @return A NetworkPeer if the address correctly parses; otherwise `absent`
     */
    Optional<? extends NetworkPeer> createPeer(String address, UnsignedInteger port, @Nullable String publicKey);

    boolean equals(Object o);

    int hashCode();
}
