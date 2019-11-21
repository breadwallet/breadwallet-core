/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/29/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInts;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoPaymentProtocolPayment extends PointerType {

    public static Optional<BRCryptoPaymentProtocolPayment> create(BRCryptoPaymentProtocolRequest request,
                                                                  BRCryptoTransfer transfer,
                                                                  BRCryptoAddress refundAddress) {
        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoPaymentProtocolPaymentCreate(
                        request.getPointer(),
                        transfer.getPointer(),
                        refundAddress.getPointer())
        ).transform(
                BRCryptoPaymentProtocolPayment::new
        );
    }

    public BRCryptoPaymentProtocolPayment() {
        super();
    }

    public BRCryptoPaymentProtocolPayment(Pointer address) {
        super(address);
    }

    public Optional<byte[]> encode() {
        Pointer thisPtr = this.getPointer();

        SizeTByReference length = new SizeTByReference(UnsignedLong.ZERO);
        Pointer returnValue = CryptoLibraryDirect.cryptoPaymentProtocolPaymentEncode(thisPtr, length);
        try {
            return Optional.fromNullable(returnValue)
                    .transform(v -> v.getByteArray(0, UnsignedInts.checkedCast(length.getValue().longValue())));
        } finally {
            if (returnValue != null) Native.free(Pointer.nativeValue(returnValue));
        }
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoPaymentProtocolPaymentGive(thisPtr);
    }
}
