/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/29/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoPaymentProtocolPayment;
import com.google.common.base.Optional;

/* package */
final class PaymentProtocolPayment implements com.breadwallet.crypto.PaymentProtocolPayment {

    /* package */
    static Optional<PaymentProtocolPayment> create(PaymentProtocolRequest request, Transfer transfer, Address target) {
        return BRCryptoPaymentProtocolPayment.create(
                request.getBRCryptoPaymentProtocolRequest(),
                transfer.getCoreBRCryptoTransfer(),
                target.getCoreBRCryptoAddress()
        ).transform(PaymentProtocolPayment::create);
    }

    /* package */
    static PaymentProtocolPayment create(BRCryptoPaymentProtocolPayment core) {;
        PaymentProtocolPayment payment = new PaymentProtocolPayment(core);
        ReferenceCleaner.register(payment, core::give);
        return payment;
    }

    private final BRCryptoPaymentProtocolPayment core;

    private PaymentProtocolPayment(BRCryptoPaymentProtocolPayment core) {
        this.core = core;
    }

    @Override
    public Optional<byte[]> encode() {
        return core.encode();
    }
}
