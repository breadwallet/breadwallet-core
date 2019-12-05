/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/29/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoPaymentProtocolPaymentAck extends PointerType {

    public static Optional<BRCryptoPaymentProtocolPaymentAck> createForBip70(byte[] serialization) {
        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoPaymentProtocolPaymentACKCreateForBip70(
                        serialization,
                        new SizeT(serialization.length)
                )
        ).transform(
                BRCryptoPaymentProtocolPaymentAck::new
        );
    }

    public BRCryptoPaymentProtocolPaymentAck() {
        super();
    }

    public BRCryptoPaymentProtocolPaymentAck(Pointer address) {
        super(address);
    }

    public Optional<String> getMemo() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(CryptoLibraryDirect.cryptoPaymentProtocolPaymentACKGetMemo(thisPtr))
                .transform(v -> v.getString(0, "UTF-8"));
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoPaymentProtocolPaymentACKGive(thisPtr);
    }
}
