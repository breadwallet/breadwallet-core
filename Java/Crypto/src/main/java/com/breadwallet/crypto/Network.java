/*
 * Network
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.libcrypto.bitcoin.BRChainParams;
import com.google.common.base.Optional;

import java.util.List;
import java.util.Set;

// TODO(discuss): This class exposes Core data types via the asBtc/asEth methods. If we want a pure
//                API, we need to expose the raw data, not the core data types (i.e. btcChainParams, ethNetwork)

// TODO(discuss): This could be split into a parent class as well as child classes for the network types.
//                This way, we can put the btcChainParams and ethNetwork in the appropriate class. We can use the
//                the visitor pattern to move from the parent to the children as needed.

public interface Network {

    Optional<Unit> baseUnitFor(Currency currency);

    Optional<Unit> defaultUnitFor(Currency currency);

    Optional<Set<Unit>> unitsFor(Currency currency);

    Optional<Address> addressFor(String address);

    boolean hasUnitFor(Currency currency, Unit unit);

    boolean hasCurrency(Currency currency);

    Currency getCurrency();

    List<WalletManagerMode> getSupportedModes();

    String getUids();

    boolean isMainnet();

    long getHeight();

    // TODO(fix): This shouldn't be in the interface
    void setHeight(long height);

    // TODO(discuss): The native type shouldn't be in the interface
    BRChainParams asBtc();
}
