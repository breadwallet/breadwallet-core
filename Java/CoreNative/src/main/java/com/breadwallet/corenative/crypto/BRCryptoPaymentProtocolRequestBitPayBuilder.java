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
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

import java.util.Date;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

public class BRCryptoPaymentProtocolRequestBitPayBuilder extends PointerType {

    public static Optional<BRCryptoPaymentProtocolRequestBitPayBuilder> create(BRCryptoNetwork network,
                                                                               BRCryptoCurrency currency,
                                                                               BRCryptoPayProtReqBitPayAndBip70Callbacks callbacks,
                                                                               String networkName,
                                                                               Date time,
                                                                               Date expires,
                                                                               double feePerByte,
                                                                               String memo,
                                                                               String paymentUrl,
                                                                               @Nullable byte[] merchantData) {
        return Optional.fromNullable(
            CryptoLibraryDirect.cryptoPaymentProtocolRequestBitPayBuilderCreate(
                    network.getPointer(),
                    currency.getPointer(),
                    callbacks.toByValue(),
                    networkName,
                    TimeUnit.MILLISECONDS.toSeconds(time.getTime()),
                    TimeUnit.MILLISECONDS.toSeconds(expires.getTime()),
                    feePerByte,
                    memo,
                    paymentUrl,
                    merchantData,
                    new SizeT(null == merchantData ? 0 : merchantData.length)
            )
        ).transform(
                BRCryptoPaymentProtocolRequestBitPayBuilder::new
        );
    }

    public BRCryptoPaymentProtocolRequestBitPayBuilder() {
        super();
    }

    public BRCryptoPaymentProtocolRequestBitPayBuilder(Pointer address) {
        super(address);
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoPaymentProtocolRequestBitPayBuilderGive(thisPtr);
    }

    public void addOutput(String address, UnsignedLong amount) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoPaymentProtocolRequestBitPayBuilderAddOutput(thisPtr, address, amount.longValue());
    }

    public Optional<BRCryptoPaymentProtocolRequest> build() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoPaymentProtocolRequestBitPayBuilderBuild(thisPtr)
        ).transform(
                BRCryptoPaymentProtocolRequest::new
        );
    }
}
