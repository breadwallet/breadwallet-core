/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoAddress;
import com.google.common.base.Optional;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

import java.util.Objects;

/* package */
final class Address implements com.breadwallet.crypto.Address {

    /* package */
    static Optional<Address> create(String address, com.breadwallet.crypto.Network network) {
        Network cryptoNetwork = Network.from(network);
        Optional<BRCryptoAddress> core = BRCryptoAddress.create(address, cryptoNetwork.getCoreBRCryptoNetwork());
        return core.transform(Address::create);
    }

    /* package */
    static Address create(BRCryptoAddress core) {
        Address address = new Address(core);
        ReferenceCleaner.register(address, core::give);
        return address;
    }

    /* package */
    static Address from(com.breadwallet.crypto.Address target) {
        if (target instanceof Address) {
            return (Address) target;
        }
        throw new IllegalArgumentException("Unsupported address instance");
    }

    private final BRCryptoAddress core;

    private final Supplier<String> toStringSupplier;

    private Address(BRCryptoAddress core) {
        this.core = core;

        this.toStringSupplier = Suppliers.memoize(core::toString);
    }

    @Override
    public String toString() {
        return toStringSupplier.get();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }

        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        Address address = (Address) o;
        return core.isIdentical(address.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(toString());
    }

    /* package */
    BRCryptoAddress getCoreBRCryptoAddress() {
        return core;
    }
}
