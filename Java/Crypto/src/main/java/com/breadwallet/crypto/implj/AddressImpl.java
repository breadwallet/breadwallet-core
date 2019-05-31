/*
 * Address
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.libcrypto.crypto.CoreBRCryptoAddress;
import com.breadwallet.crypto.libcrypto.ethereum.BREthereumAddress;
import com.breadwallet.crypto.libcrypto.support.BRAddress;
import com.google.common.base.Optional;

import java.util.Objects;

public final class AddressImpl implements Address {

    /* package */
    static Optional<AddressImpl> createAsBtc(String address) {
        if (!BRAddress.isValid(address)) {
            return Optional.absent();
        }

        CoreBRCryptoAddress cryptoAddress = CoreBRCryptoAddress.createAsBtc(address);
        return Optional.of(new AddressImpl(cryptoAddress));
    }

    /* package */
    static AddressImpl createAsBtc(BRAddress address) {
        BRAddress.ByValue addressValue = new BRAddress.ByValue(address);
        return new AddressImpl(CoreBRCryptoAddress.createAsBtc(addressValue));
    }

    /* package */
    static Optional<AddressImpl> createAsEth(String address) {
        if (!BREthereumAddress.isValid(address)) {
            return Optional.absent();
        }

        CoreBRCryptoAddress cryptoAddress = CoreBRCryptoAddress.createAsEth(address);
        return Optional.of(new AddressImpl(cryptoAddress));
    }

    /* package */
    static AddressImpl createAsEth(BREthereumAddress address) {
        BREthereumAddress.ByValue addressValue = new BREthereumAddress.ByValue(address);
        return new AddressImpl(CoreBRCryptoAddress.createAsEth(addressValue));
    }

    private final CoreBRCryptoAddress core;

    private AddressImpl(CoreBRCryptoAddress core) {
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

        AddressImpl address = (AddressImpl) o;
        return core.isIdentical(address.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
