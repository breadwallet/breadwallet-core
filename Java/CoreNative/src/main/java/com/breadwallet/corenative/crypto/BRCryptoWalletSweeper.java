/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.base.Optional;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoWalletSweeper extends PointerType {

    public BRCryptoWalletSweeper(Pointer address) {
        super(address);
    }

    public BRCryptoWalletSweeper() {
        super();
    }

    public static BRCryptoWalletSweeperStatus validateSupported(BRCryptoNetwork network,
                                                                BRCryptoCurrency currency,
                                                                BRCryptoKey key,
                                                                BRCryptoWallet wallet) {
        return BRCryptoWalletSweeperStatus.fromCore(
                CryptoLibrary.INSTANCE.cryptoWalletSweeperValidateSupported(
                        network,
                        currency,
                        key,
                        wallet)
        );
    }

    public static BRCryptoWalletSweeper createAsBtc(BRCryptoNetwork network,
                                                    BRCryptoCurrency currency,
                                                    BRCryptoKey key,
                                                    BRCryptoAddressScheme scheme) {
        return CryptoLibrary.INSTANCE.cryptoWalletSweeperCreateAsBtc(network,
                currency, key, scheme.toCore());
    }

    public BRCryptoKey getKey() {
        return CryptoLibrary.INSTANCE.cryptoWalletSweeperGetKey(this);
    }

    public Optional<BRCryptoAmount> getBalance() {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoWalletSweeperGetBalance(this));
    }

    public Optional<String> getAddress() {
        Pointer ptr = CryptoLibrary.INSTANCE.cryptoWalletSweeperGetAddress(this);
        try {
            return Optional.fromNullable(ptr).transform(p -> p.getString(0, "UTF-8"));
        } finally {
            if (ptr != null) Native.free(Pointer.nativeValue(ptr));
        }
    }

    public BRCryptoWalletSweeperStatus handleTransactionAsBtc(byte[] transaction) {
        return BRCryptoWalletSweeperStatus.fromCore(
                CryptoLibrary.INSTANCE.cryptoWalletSweeperHandleTransactionAsBTC(
                        this,
                        transaction,
                        new SizeT(transaction.length))
        );
    }

    public BRCryptoWalletSweeperStatus validate() {
        return BRCryptoWalletSweeperStatus.fromCore(
                CryptoLibrary.INSTANCE.cryptoWalletSweeperValidate(this)
        );
    }

    public void give() {
        if (null != getPointer()) {
            CryptoLibrary.INSTANCE.cryptoWalletSweeperRelease(this);
        }
    }
}
