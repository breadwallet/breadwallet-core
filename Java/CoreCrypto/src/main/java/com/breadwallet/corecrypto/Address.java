/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.CoreBRCryptoAddress;
import com.breadwallet.corenative.ethereum.BREthereumAddress;
import com.breadwallet.corenative.support.BRAddress;
import com.google.common.base.Optional;

import java.util.Objects;

/* package */
final class Address implements com.breadwallet.crypto.Address {

    /* package */
    static Optional<Address> createAsBtc(String address) {
        if (!BRAddress.isValid(address)) {
            return Optional.absent();
        }

        CoreBRCryptoAddress cryptoAddress = CoreBRCryptoAddress.createAsBtc(address);
        return Optional.of(new Address(cryptoAddress));
    }

    /* package */
    static Address createAsBtc(BRAddress address) {
        BRAddress.ByValue addressValue = new BRAddress.ByValue(address);
        return new Address(CoreBRCryptoAddress.createAsBtc(addressValue));
    }

    /* package */
    static Optional<Address> createAsEth(String address) {
        if (!BREthereumAddress.isValid(address)) {
            return Optional.absent();
        }

        CoreBRCryptoAddress cryptoAddress = CoreBRCryptoAddress.createAsEth(address);
        return Optional.of(new Address(cryptoAddress));
    }

    /* package */
    static Address createAsEth(BREthereumAddress address) {
        BREthereumAddress.ByValue addressValue = new BREthereumAddress.ByValue(address);
        return new Address(CoreBRCryptoAddress.createAsEth(addressValue));
    }

    /* package */
    static Address create(CoreBRCryptoAddress core) {
        return new Address(core);
    }

    /* package */
    static Address from(com.breadwallet.crypto.Address target) {
        if (target instanceof Address) {
            return (Address) target;
        }
        throw new IllegalArgumentException("Unsupported address instance");
    }

    private final CoreBRCryptoAddress core;

    private Address(CoreBRCryptoAddress core) {
        this.core = core;
    }

    @Override
    public String toString() {
        return core.toString();
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
        return Objects.hash(core);
    }

    /* package */
    CoreBRCryptoAddress getCoreBRCryptoAddress() {
        return core;
    }
}
