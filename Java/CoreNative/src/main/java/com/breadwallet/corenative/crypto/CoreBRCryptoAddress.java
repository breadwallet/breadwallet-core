/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.ethereum.BREthereumAddress;
import com.breadwallet.corenative.support.BRAddress;

public interface CoreBRCryptoAddress {

    static CoreBRCryptoAddress createAsBtc(String address) {
        BRAddress.ByValue addressValue = new BRAddress.ByValue(BRAddress.addressFill(address));
        return new OwnedBRCryptoAddress(CryptoLibrary.INSTANCE.cryptoAddressCreateAsBTC(addressValue));
    }

    static CoreBRCryptoAddress createAsEth(String address) {
        BREthereumAddress.ByValue addressValue = CryptoLibrary.INSTANCE.addressCreate(address);
        return new OwnedBRCryptoAddress(CryptoLibrary.INSTANCE.cryptoAddressCreateAsETH(addressValue));
    }

    boolean isIdentical(CoreBRCryptoAddress address);

    String toString();

    BRCryptoAddress asBRCryptoAddress();
}
