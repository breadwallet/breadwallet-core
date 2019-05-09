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

import com.breadwallet.crypto.jni.BRAddress;
import com.breadwallet.crypto.jni.BREthereumAddress;
import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoAddress;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoBoolean;
import com.breadwallet.crypto.jni.CryptoLibrary.BREthereumBoolean;
import com.breadwallet.crypto.network.Network;
import com.google.common.base.Optional;
import com.sun.jna.Native;
import com.sun.jna.Pointer;

import java.util.Objects;

public final class Address {

    private final BRCryptoAddress core;

    public static Optional<Address> create(String address, Network network) {
        return network.addressFor(address);
    }

    /* package */ static Optional<Address> createAsBtc(String address) {
        if (BRCryptoBoolean.CRYPTO_TRUE != CryptoLibrary.INSTANCE.BRAddressIsValid(address)) {
            return Optional.absent();
        }

        BRAddress.ByValue addressValue = new BRAddress.ByValue(BRAddress.addressFill(address).s);
        return Optional.of(new Address(CryptoLibrary.INSTANCE.cryptoAddressCreateAsBTC(addressValue)));
    }

    /* package */ static Address createAsBtc(BRAddress address) {
        // TODO: Can we just create cryptoAddressCreateAsBTCString function in the C layer?
        BRAddress.ByValue addressValue = new BRAddress.ByValue(address.s);
        return new Address(CryptoLibrary.INSTANCE.cryptoAddressCreateAsBTC(addressValue));
    }

    /* package */ static Optional<Address> createAsEth(String address) {
        if (BREthereumBoolean.ETHEREUM_BOOLEAN_TRUE != CryptoLibrary.INSTANCE.addressValidateString(address)) {
            return Optional.absent();
        }

        BREthereumAddress.ByValue addressValue = CryptoLibrary.INSTANCE.addressCreate(address);
        return Optional.of(new Address(CryptoLibrary.INSTANCE.cryptoAddressCreateAsETH(addressValue)));
    }

    /* package */ static Address createAsEth(BREthereumAddress address) {
        // TODO: Can we just create cryptoAddressCreateAsETHString function in the C layer?
        BREthereumAddress.ByValue addressValue = new BREthereumAddress.ByValue(address.s);
        return new Address(CryptoLibrary.INSTANCE.cryptoAddressCreateAsETH(addressValue));
    }

    private Address(BRCryptoAddress core) {
        this.core = core;
    }

    @Override
    protected void finalize() {
        CryptoLibrary.INSTANCE.cryptoAddressGive(core);
    }

    @Override
    public String toString() {
        Pointer addressPtr = CryptoLibrary.INSTANCE.cryptoAddressAsString(core);
        String addressStr = addressPtr.getString(0, "UTF-8");

        // TODO: Is it safe to use this? Should we have a cryptoAddressStringFree?
        Native.free(Pointer.nativeValue(addressPtr));
        return addressStr;
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
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAddressIsIdentical(core, address.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
