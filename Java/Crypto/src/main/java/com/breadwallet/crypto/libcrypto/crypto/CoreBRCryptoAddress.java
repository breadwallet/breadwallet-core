package com.breadwallet.crypto.libcrypto.crypto;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;
import com.breadwallet.crypto.libcrypto.ethereum.BREthereumAddress;
import com.breadwallet.crypto.libcrypto.support.BRAddress;

public interface CoreBRCryptoAddress {

    static CoreBRCryptoAddress createAsBtc(String address) {
        BRAddress.ByValue addressValue = new BRAddress.ByValue(BRAddress.addressFill(address));
        return new OwnedBRCryptoAddress(CryptoLibrary.INSTANCE.cryptoAddressCreateAsBTC(addressValue));
    }

    static CoreBRCryptoAddress createAsBtc(BRAddress.ByValue address) {
        return new OwnedBRCryptoAddress(CryptoLibrary.INSTANCE.cryptoAddressCreateAsBTC(address));
    }

    static CoreBRCryptoAddress createAsEth(String address) {
        BREthereumAddress.ByValue addressValue = CryptoLibrary.INSTANCE.addressCreate(address);
        return new OwnedBRCryptoAddress(CryptoLibrary.INSTANCE.cryptoAddressCreateAsETH(addressValue));
    }

    static CoreBRCryptoAddress createAsEth(BREthereumAddress.ByValue address) {
        return new OwnedBRCryptoAddress(CryptoLibrary.INSTANCE.cryptoAddressCreateAsETH(address));
    }

    boolean isIdentical(CoreBRCryptoAddress address);

    String toString();

    BRCryptoAddress asBRCryptoAddress();
}
