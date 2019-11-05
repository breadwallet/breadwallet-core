/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.base.Optional;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoWalletSweeper extends PointerType {

    public BRCryptoWalletSweeper() {
        super();
    }

    public BRCryptoWalletSweeper(Pointer address) {
        super(address);
    }

    public static BRCryptoWalletSweeperStatus validateSupported(BRCryptoNetwork network,
                                                                BRCryptoCurrency currency,
                                                                BRCryptoKey key,
                                                                BRCryptoWallet wallet) {
        return BRCryptoWalletSweeperStatus.fromCore(
                CryptoLibraryDirect.cryptoWalletSweeperValidateSupported(
                        network.getPointer(),
                        currency.getPointer(),
                        key.getPointer(),
                        wallet.getPointer()
                )
        );
    }

    public static BRCryptoWalletSweeper createAsBtc(BRCryptoNetwork network,
                                                    BRCryptoCurrency currency,
                                                    BRCryptoKey key,
                                                    BRCryptoAddressScheme scheme) {
        return new BRCryptoWalletSweeper(
                CryptoLibraryDirect.cryptoWalletSweeperCreateAsBtc(
                        network.getPointer(),
                        currency.getPointer(),
                        key.getPointer(),
                        scheme.toCore()
                )
        );
    }

    public BRCryptoKey getKey() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoKey(CryptoLibraryDirect.cryptoWalletSweeperGetKey(thisPtr));
    }

    public Optional<BRCryptoAmount> getBalance() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoWalletSweeperGetBalance(
                        thisPtr
                )
        ).transform(BRCryptoAmount::new);
    }

    public Optional<String> getAddress() {
        Pointer thisPtr = this.getPointer();

        Pointer ptr = CryptoLibraryDirect.cryptoWalletSweeperGetAddress(thisPtr);
        try {
            return Optional.fromNullable(ptr).transform(p -> p.getString(0, "UTF-8"));
        } finally {
            if (ptr != null) Native.free(Pointer.nativeValue(ptr));
        }
    }

    public BRCryptoWalletSweeperStatus handleTransactionAsBtc(byte[] transaction) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoWalletSweeperStatus.fromCore(
                CryptoLibraryDirect.cryptoWalletSweeperHandleTransactionAsBTC(
                        thisPtr,
                        transaction,
                        new SizeT(transaction.length))
        );
    }

    public BRCryptoWalletSweeperStatus validate() {
        Pointer thisPtr = this.getPointer();

        return BRCryptoWalletSweeperStatus.fromCore(
                CryptoLibraryDirect.cryptoWalletSweeperValidate(thisPtr)
        );
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletSweeperRelease(thisPtr);
    }
}
