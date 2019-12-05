/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/29/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoPaymentProtocolPaymentAck;
import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.base.Optional;

import java.util.List;

import static com.google.common.base.Preconditions.checkNotNull;

/* package */
final class PaymentProtocolPaymentAck implements com.breadwallet.crypto.PaymentProtocolPaymentAck {

    /* package */
    static Optional<PaymentProtocolPaymentAck> createForBip70(byte[] serialization) {
        Optional<BRCryptoPaymentProtocolPaymentAck> maybeAck = BRCryptoPaymentProtocolPaymentAck.createForBip70(serialization);
        return maybeAck.transform(PaymentProtocolPaymentAck::create);
    }

    /* package */
    static Optional<PaymentProtocolPaymentAck> createForBitPay(String json) {
        Optional<BitPayAck> maybeAck = BitPayAck.asBitPayAck(json);
        return maybeAck.transform(PaymentProtocolPaymentAck::create);
    }

    private static PaymentProtocolPaymentAck create(BRCryptoPaymentProtocolPaymentAck core) {
        com.breadwallet.crypto.PaymentProtocolPaymentAck paymentAck = new PaymentProtocolPaymentBip70Ack(core);
        ReferenceCleaner.register(paymentAck, core::give);
        return new PaymentProtocolPaymentAck(paymentAck);
    }

    private static PaymentProtocolPaymentAck create(BitPayAck ack) {
        com.breadwallet.crypto.PaymentProtocolPaymentAck paymentAck = new PaymentProtocolPaymentBitPayAck(ack);
        return new PaymentProtocolPaymentAck(paymentAck);
    }

    private final com.breadwallet.crypto.PaymentProtocolPaymentAck impl;

    private PaymentProtocolPaymentAck(com.breadwallet.crypto.PaymentProtocolPaymentAck impl) {
        this.impl = impl;
    }

    @Override
    public Optional<String> getMemo() {
        return impl.getMemo();
    }

    private static class PaymentProtocolPaymentBip70Ack implements com.breadwallet.crypto.PaymentProtocolPaymentAck {

        final BRCryptoPaymentProtocolPaymentAck core;

        PaymentProtocolPaymentBip70Ack(BRCryptoPaymentProtocolPaymentAck core) {
            this.core = core;
        }

        @Override
        public Optional<String> getMemo() {
            return core.getMemo();
        }
    }

    private static class PaymentProtocolPaymentBitPayAck implements com.breadwallet.crypto.PaymentProtocolPaymentAck {

        final BitPayAck ack;

        PaymentProtocolPaymentBitPayAck(BitPayAck ack) {
            this.ack = ack;
        }

        @Override
        public Optional<String> getMemo() {
            return Optional.fromNullable(ack.memo);
        }
    }

    private static final class BitPayAck {

        static Optional<BitPayAck> asBitPayAck(String json) {
            ObjectMapper mapper = new ObjectMapper();

            BitPayAck ack;
            try {
                ack = mapper.readValue(json, BitPayAck.class);
            } catch (JsonProcessingException e) {
                ack = null;
            }

            return Optional.fromNullable(ack);
        }

        @JsonCreator
        static BitPayAck create(@JsonProperty("memo") String memo,
                                @JsonProperty("payment") BitPayPayment payment) {
            return new BitPayAck(
                    memo,
                    payment
            );
        }

        private final @Nullable String memo;
        private final BitPayPayment payment;

        private BitPayAck(@Nullable String memo,
                          BitPayPayment payment) {
            this.memo = memo;
            this.payment = payment;
        }

    }

    private static final class BitPayPayment {

        @JsonCreator
        static BitPayPayment create(@JsonProperty("currency") String currency,
                                    @JsonProperty("transactions") List<String> transactions) {
            return new BitPayPayment(
                    checkNotNull(currency),
                    checkNotNull(transactions)
            );
        }

        private final String currency;
        private final List<String> transactions;

        private BitPayPayment(String currency,
                              List<String> transactions) {
            this.currency = currency;
            this.transactions = transactions;
        }
    }

}
