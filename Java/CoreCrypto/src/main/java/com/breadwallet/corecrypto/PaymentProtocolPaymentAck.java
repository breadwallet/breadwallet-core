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
import com.google.common.base.Optional;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

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

    private static final class BitPayPayment {

        static Optional<BitPayPayment> asBitPayPayment(JSONObject json) {
            try {
                String currency = json.getString("currency");
                JSONArray transactionsJson = json.getJSONArray("transactions");

                List<String> transactions = new ArrayList<>();
                for (int i = 0; i < transactionsJson.length(); i++) {
                    transactions.add(transactionsJson.getString(i));
                }

                return Optional.of(
                        new BitPayPayment(
                                currency,
                                transactions
                        )
                );
            } catch (JSONException e) {
                return Optional.absent();
            }
        }

        final String currency;
        final List<String> transactions;

        BitPayPayment(String currency, List<String> transactions) {
            this.currency = currency;
            this.transactions = transactions;
        }

    }

    private static final class BitPayAck {

        static Optional<BitPayAck> asBitPayAck(String json) {
            try {
                return asBitPayAck(new JSONObject(json));
            } catch (JSONException e) {
                return Optional.absent();
            }
        }

        static Optional<BitPayAck> asBitPayAck(JSONObject json) {
            try {
                Optional<String> maybeMemo = Optional.fromNullable(json.optString("memo"));
                Optional<BitPayPayment> maybePayment = BitPayPayment.asBitPayPayment(json.getJSONObject("payment"));

                if (!maybePayment.isPresent()) {
                    return Optional.absent();
                }

                return Optional.of(
                        new BitPayAck(
                                maybeMemo.orNull(),
                                maybePayment.get()
                        )
                );
            } catch (JSONException e) {
                return Optional.absent();
            }
        }

        @Nullable
        String memo;
        BitPayPayment payment;

        BitPayAck(@Nullable String memo, BitPayPayment payment) {
            this.memo = memo;
            this.payment = payment;
        }
    }

}
