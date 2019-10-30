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
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoPaymentProtocolRequest extends PointerType {

    // must remain in sync with BRCryptoPaymentProtocolType
    private static final int CRYPTO_PAYMENT_PROTOCOL_TYPE_BITPAY    = 0;
    private static final int CRYPTO_PAYMENT_PROTOCOL_TYPE_BIP70     = 1;

    public static boolean validateForBitPay(BRCryptoNetwork cryptoNetwork,
                                            BRCryptoCurrency cryptoCurrency,
                                            BRCryptoWallet cryptoWallet) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoPaymentProtocolRequestValidateSupported(
                CRYPTO_PAYMENT_PROTOCOL_TYPE_BITPAY,
                cryptoNetwork.getPointer(),
                cryptoCurrency.getPointer(),
                cryptoWallet.getPointer()
        );
    }

    public static boolean validateForBip70(BRCryptoNetwork cryptoNetwork,
                                           BRCryptoCurrency cryptoCurrency,
                                           BRCryptoWallet cryptoWallet) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoPaymentProtocolRequestValidateSupported(
                CRYPTO_PAYMENT_PROTOCOL_TYPE_BIP70,
                cryptoNetwork.getPointer(),
                cryptoCurrency.getPointer(),
                cryptoWallet.getPointer()
        );
    }

    public static Optional<BRCryptoPaymentProtocolRequest> createForBip70(BRCryptoNetwork cryptoNetwork,
                                                                          BRCryptoCurrency cryptoCurrency,
                                                                          BRCryptoPayProtReqBitPayAndBip70Callbacks callbacks,
                                                                          byte[] serialization) {
        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoPaymentProtocolRequestCreateForBip70(
                        cryptoNetwork.getPointer(),
                        cryptoCurrency.getPointer(),
                        callbacks.toByValue(),
                        serialization,
                        new SizeT(serialization.length))
        ).transform(
                BRCryptoPaymentProtocolRequest::new
        );
    }

    public BRCryptoPaymentProtocolRequest() {
        super();
    }

    public BRCryptoPaymentProtocolRequest(Pointer address) {
        super(address);
    }

    public BRCryptoPaymentProtocolType getType() {
        Pointer thisPtr = this.getPointer();

        return BRCryptoPaymentProtocolType.fromCore(CryptoLibraryDirect.cryptoPaymentProtocolRequestGetType(thisPtr));
    }

    public boolean isSecure() {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoPaymentProtocolRequestIsSecure(thisPtr);
    }

    public Optional<String> getMemo() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(CryptoLibraryDirect.cryptoPaymentProtocolRequestGetMemo(thisPtr))
                .transform(v -> v.getString(0, "UTF-8"));
    }

    public Optional<String> getPaymentUrl() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(CryptoLibraryDirect.cryptoPaymentProtocolRequestGetPaymentURL(thisPtr))
                .transform(v -> v.getString(0, "UTF-8"));
    }

    public Optional<BRCryptoAmount> getTotalAmount() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(CryptoLibraryDirect.cryptoPaymentProtocolRequestGetTotalAmount(thisPtr))
                .transform(BRCryptoAmount::new);
    }

    public Optional<BRCryptoNetworkFee> getRequiredNetworkFee() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(CryptoLibraryDirect.cryptoPaymentProtocolRequestGetRequiredNetworkFee(thisPtr))
                .transform(BRCryptoNetworkFee::new);
    }

    public Optional<BRCryptoAddress> getPrimaryTargetAddress() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(CryptoLibraryDirect.cryptoPaymentProtocolRequestGetPrimaryTargetAddress(thisPtr))
                .transform(BRCryptoAddress::new);
    }

    public Optional<String> getCommonName() {
        Pointer thisPtr = this.getPointer();

        Pointer returnValue = CryptoLibraryDirect.cryptoPaymentProtocolRequestGetCommonName(thisPtr);
        try {
            return Optional.fromNullable(returnValue)
                    .transform(v -> v.getString(0, "UTF-8"));
        } finally {
            if (returnValue != null) Native.free(Pointer.nativeValue(returnValue));
        }
    }

    public BRCryptoPaymentProtocolError isValid() {
        Pointer thisPtr = this.getPointer();

        return BRCryptoPaymentProtocolError.fromCore(CryptoLibraryDirect.cryptoPaymentProtocolRequestIsValid(thisPtr));
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoPaymentProtocolRequestGive(thisPtr);
    }
}
