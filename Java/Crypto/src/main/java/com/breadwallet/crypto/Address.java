/*
 * Address
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.crypto.CoreBRCryptoAddress;
import com.breadwallet.crypto.jni.support.BRAddress;
import com.breadwallet.crypto.jni.ethereum.BREthereumAddress;
import com.google.common.base.Optional;

import java.util.Objects;

public final class Address {

    public static Optional<Address> create(String address, Network network) {
        return network.addressFor(address);
    }

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
        // TODO: Can we just create cryptoAddressCreateAsBTCString function in the C layer?
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
        // TODO: Can we just create cryptoAddressCreateAsETHString function in the C layer?
        BREthereumAddress.ByValue addressValue = new BREthereumAddress.ByValue(address);
        return new Address(CoreBRCryptoAddress.createAsEth(addressValue));
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
}
