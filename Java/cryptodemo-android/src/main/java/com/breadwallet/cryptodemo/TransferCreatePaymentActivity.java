/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/30/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.cryptodemo;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.Html;
import android.widget.Button;
import android.widget.EditText;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.NetworkFee;
import com.breadwallet.crypto.PaymentProtocolPayment;
import com.breadwallet.crypto.PaymentProtocolPaymentAck;
import com.breadwallet.crypto.PaymentProtocolRequest;
import com.breadwallet.crypto.PaymentProtocolRequestType;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferFeeBasis;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.errors.FeeEstimationError;
import com.breadwallet.crypto.errors.PaymentProtocolError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Enums;
import com.google.common.base.Optional;

import org.jetbrains.annotations.NotNull;

import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.ResponseBody;

public class TransferCreatePaymentActivity extends AppCompatActivity {

    private static final Logger Log = Logger.getLogger(TransferCreatePaymentActivity.class.getName());

    private static final String MIME_TYPE_PAYMENT_REQUEST_BIP70     = "application/bitcoin-paymentrequest";
    private static final String MIME_TYPE_PAYMENT_REQUEST_BITPAY_V1 = "application/payment-request";

    private static final String MIME_TYPE_PAYMENT_PAYMENT_BIP70     = "application/bitcoin-payment";
    private static final String MIME_TYPE_PAYMENT_PAYMENT_BITPAY_V1 = "application/payment";

    private static final String MIME_TYPE_PAYMENT_ACK_BIP70         = "application/bitcoin-paymentack";
    private static final String MIME_TYPE_PAYMENT_ACK_BITPAY_V1     = "application/payment-ack";

    private static final String EXTRA_WALLET_NAME = "com.breadwallet.cryptodemo,TransferCreatePaymentActivity.EXTRA_WALLET_NAME";
    private static final String EXTRA_PROTOCOL = "com.breadwallet.cryptodemo,TransferCreatePaymentActivity.EXTRA_PROTOCOL";

    public static void start(Activity callerActivity, Wallet wallet) {
        start(callerActivity, wallet, PaymentProtocolRequestType.BIP70);
    }

    public static void start(Activity callerActivity, Wallet wallet, PaymentProtocolRequestType type) {
        Intent intent = new Intent(callerActivity, TransferCreatePaymentActivity.class);
        intent.putExtra(EXTRA_WALLET_NAME, wallet.getName());
        intent.putExtra(EXTRA_PROTOCOL, type.name());
        callerActivity.startActivity(intent);
    }

    @Nullable
    private static Wallet getWallet(Intent intent) {
        String walletName = intent.getStringExtra(EXTRA_WALLET_NAME);
        for(Wallet wallet: CoreCryptoApplication.getSystem().getWallets()) {
            if (wallet.getName().equals(walletName)) {
                return wallet;
            }
        }
        return null;
    }

    @Nullable
    private static PaymentProtocolRequestType getProtocolType(Intent intent) {
        String typeString = Optional.fromNullable(intent.getStringExtra(EXTRA_PROTOCOL)).or("");
        return Enums.getIfPresent(PaymentProtocolRequestType.class, typeString).orNull();
    }

    private Wallet wallet;
    private PaymentProtocolRequestType type;
    private OkHttpClient client;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_transfer_create_payment);

        CoreCryptoApplication.initialize(this);

        Intent intent = getIntent();

        wallet = getWallet(intent);
        if (null == wallet) {
            finish();
            return;
        }

        type = getProtocolType(intent);
        if (null == type) {
            finish();
            return;
        }

        client = new OkHttpClient();

        Button payView = findViewById(R.id.pay_view);
        EditText urlView = findViewById(R.id.url_view);

        payView.setText(String.format("Pay with %s", getPaymentTypeString()));
        payView.setOnClickListener(v -> getProtocolRequest(urlView.getText().toString()));

        Toolbar toolbar = findViewById(R.id.toolbar_view);
        setSupportActionBar(toolbar);
    }

    private String getPaymentTypeString() {
        switch (type) {
            case BIP70: return "BIP-70";
            case BITPAY: return "BitPay";
            default: throw new IllegalStateException("Invalid type");
        }
    }

    private String getProtocolRequestMimeType() {
        switch (type) {
            case BIP70: return MIME_TYPE_PAYMENT_REQUEST_BIP70;
            case BITPAY: return MIME_TYPE_PAYMENT_REQUEST_BITPAY_V1;
            default: throw new IllegalStateException("Invalid type");
        }
    }

    private String getProtocolPaymentMimeType() {
        switch (type) {
            case BIP70: return MIME_TYPE_PAYMENT_PAYMENT_BIP70;
            case BITPAY: return MIME_TYPE_PAYMENT_PAYMENT_BITPAY_V1;
            default: throw new IllegalStateException("Invalid type");
        }
    }

    private String getProtocolAckMimeType() {
        switch (type) {
            case BIP70: return MIME_TYPE_PAYMENT_ACK_BIP70;
            case BITPAY: return MIME_TYPE_PAYMENT_ACK_BITPAY_V1;
            default: throw new IllegalStateException("Invalid type");
        }
    }

    //
    // Protocol Request
    //

    private void getProtocolRequest(String url) {
        String expectedMimeType = getProtocolRequestMimeType();
        client
                .newCall(
                        new Request.Builder()
                                .url(url)
                                .header("Accept", expectedMimeType)
                                .get()
                                .build()
                ).enqueue(new Callback() {
                        @Override
                        public void onResponse(@NotNull Call call, @NotNull Response response) throws IOException {
                            if (!response.isSuccessful()) {
                                showError("Failed with bad protocol request response code");
                                return;
                            }

                            String mimeType = response.header("Content-Type");
                            if (null == mimeType) {
                                showError("Failed with missing protocol request content type");
                                return;
                            }

                            try (ResponseBody body = response.body()) {
                                if (null == body || !mimeType.startsWith(expectedMimeType)) {
                                    showError("Failed with unexpected protocol request content type");

                                } else if (mimeType.startsWith(MIME_TYPE_PAYMENT_REQUEST_BIP70)) {
                                    handleProtocolRequestResponseForBip70(body.bytes());

                                } else if (mimeType.startsWith(MIME_TYPE_PAYMENT_REQUEST_BITPAY_V1)) {
                                    handleProtocolRequestResponseForBitPay(body.string());
                                } else {
                                    showError("Failed with unsupported protocol request content type");
                                }
                            }
                        }

                        @Override
                        public void onFailure(@NotNull Call call, @NotNull IOException e) {
                            showError("Failed with protocol request exception");
                        }
        });
    }

    private void handleProtocolRequestResponseForBip70(byte[] response) {
        Optional<PaymentProtocolRequest> maybeRequest = PaymentProtocolRequest.createForBip70(wallet, response);
        if (!maybeRequest.isPresent()) {
            showError("Failed to parse BIP-70 request");
            return;
        }

        PaymentProtocolRequest request = maybeRequest.get();
        logProtocolRequest(request);
        estimatePaymentFee(request);
    }

    private void handleProtocolRequestResponseForBitPay(String response) {
        Optional<PaymentProtocolRequest> maybeRequest = PaymentProtocolRequest.createForBitPay(wallet, response);
        if (!maybeRequest.isPresent()) {
            showError("Failed to parse BitPay request");
            return;
        }

        PaymentProtocolRequest request = maybeRequest.get();
        logProtocolRequest(request);
        estimatePaymentFee(request);
    }

    //
    // Fee Estimation
    //

    private void estimatePaymentFee(PaymentProtocolRequest request) {
        NetworkFee requiredFee = request.getRequiredNetworkFee().orNull();
        NetworkFee defaultFee = wallet.getWalletManager().getDefaultNetworkFee();
        NetworkFee transferFee = requiredFee == null ? defaultFee :
                (requiredFee.getConfirmationTimeInMilliseconds().compareTo(defaultFee.getConfirmationTimeInMilliseconds()) < 0 ?
                        requiredFee : defaultFee);

        request.estimate(transferFee, new CompletionHandler<TransferFeeBasis, FeeEstimationError>() {
            @Override
            public void handleData(TransferFeeBasis feeBasis) {
                handlePaymentFeeEstimate(request, feeBasis);
            }

            @Override
            public void handleError(FeeEstimationError error) {
                showError("Failed to estimate transfer");
            }
        });
    }

    private void handlePaymentFeeEstimate(PaymentProtocolRequest request, TransferFeeBasis feeBasis) {
        logFeeEstimate(feeBasis);

        Optional<? extends Address> maybeAddress = request.getPrimaryTarget();
        Optional<? extends Amount> maybeAmount = request.getTotalAmount();

        if (!maybeAddress.isPresent()) {
            showError("Failed with missing address");

        } else if (!maybeAmount.isPresent()) {
            showError("Failed with missing amount");

        } else {
            Optional<PaymentProtocolError> maybeError = request.validate();
            Optional<String> maybeCommonName = request.getCommonName();
            Optional<String> maybeMemo = request.getMemo();
            boolean isSecure = request.isSecure();

            StringBuilder builder = new StringBuilder();
            builder.append(
                    String.format("<b>Secure</b>: %s", isSecure)
            );

            if (maybeError.isPresent()) {
                builder.append(
                        String.format("<br><b>Error</b>: %s", maybeError.get())
                );
            }

            if (maybeCommonName.isPresent()) {
                builder.append(
                        String.format("<br><b>To:</b> %s (%s)",
                                Html.escapeHtml(maybeCommonName.get()),
                                Html.escapeHtml(maybeAddress.get().toString()))
                );
            } else {
                builder.append(
                        String.format("<br><b>Address:</b> %s",
                                Html.escapeHtml(maybeAddress.get().toString()))
                );
            }

            builder.append(
                    String.format("<br><b>Amount:</b> %s",
                            Html.escapeHtml(maybeAmount.get().toString()))
            );

            builder.append(
                    String.format("<br><b>Fee:</b> %s",
                            Html.escapeHtml(feeBasis.getFee().toString()))
            );

            if (maybeMemo.isPresent()) {
                builder.append(
                        String.format("<br><b>Memo:</b> %s",
                                Html.escapeHtml(maybeMemo.get()))
                );
            }

            runOnUiThread(() -> new AlertDialog.Builder(this)
                    .setTitle("Payment Details")
                    .setMessage(Html.fromHtml(builder.toString()))
                    .setNegativeButton("Cancel", (dialog, which) -> {})
                    .setPositiveButton("Continue", (dialog, which) -> {
                        continuePayment(request, feeBasis);
                    })
                    .show());
        }
    }

    //
    // Protocol Payment
    //

    private void continuePayment(PaymentProtocolRequest request, TransferFeeBasis feeBasis) {
        Optional<? extends Transfer> maybeTransfer = request.createTransfer(feeBasis);
        if (!maybeTransfer.isPresent()) {
            showError("Failed to create transfer");
            return;
        }

        Transfer transfer = maybeTransfer.get();
        if (!request.signTransfer(transfer, CoreCryptoApplication.getPaperKey())) {
            showError("Failed to sign transfer");
            return;
        }

        request.submitTransfer(transfer);

        Optional<? extends PaymentProtocolPayment> maybePayment = request.createPayment(transfer);
        if (!maybePayment.isPresent()) {
            showError("Failed to create payment");
            return;
        }

        PaymentProtocolPayment payment = maybePayment.get();
        Optional<byte[]> maybeEncoded = payment.encode();
        if (!maybeEncoded.isPresent()) {
            showError("Failed to encode payment");
            return;
        }

        byte[] encoded = maybeEncoded.get();
        postProtocolPayment(request, encoded);
    }

    private void postProtocolPayment(PaymentProtocolRequest request, byte[] encoded) {
        Optional<String> maybePaymentUrl = request.getPaymentUrl();
        if (!maybePaymentUrl.isPresent()) {
            return;
        }

        String expectedMimeType = getProtocolAckMimeType();
        client
                .newCall(
                        new Request.Builder()
                                .url(maybePaymentUrl.get())
                                .header("Content-Type", getProtocolPaymentMimeType())
                                .header("Accept", expectedMimeType)
                                .post(RequestBody.create(encoded))
                                .build()
                ).enqueue(new Callback() {
                        @Override
                        public void onResponse(@NotNull Call call, @NotNull Response response) throws IOException {
                            if (!response.isSuccessful()) {
                                showError("Failed with bad protocol payment response code");
                                return;
                            }

                            String mimeType = response.header("Content-Type");
                            if (null == mimeType) {
                                showError("Failed with missing protocol payment content type");
                                return;
                            }

                            try (ResponseBody body = response.body()) {
                                if (!mimeType.startsWith(expectedMimeType)) {
                                    showError("Failed with unexpected protocol payment content type");

                                } else if (mimeType.startsWith(MIME_TYPE_PAYMENT_ACK_BIP70)) {
                                    handleProtocolPaymentResponseForBip70(body.bytes());

                                } else if (mimeType.startsWith(MIME_TYPE_PAYMENT_ACK_BITPAY_V1)) {
                                    handleProtocolPaymentResponseForBitPay(body.string());
                                } else {
                                    showError("Failed with unsupported protocol payment content type");
                                }
                            }
                        }

                        @Override
                        public void onFailure(@NotNull Call call, @NotNull IOException e) {
                            showError("Failed with protocol payment exception");
                        }
        });
    }

    private void handleProtocolPaymentResponseForBip70(byte[] response) {
        Optional<PaymentProtocolPaymentAck> maybeAck = PaymentProtocolPaymentAck.createForBip70(response);
        if (!maybeAck.isPresent()) {
            showError("Failed to parse BIP-70 ack");
            return;
        }

        PaymentProtocolPaymentAck ack = maybeAck.get();
        logProtocolAck(ack);
        handleProtocolPaymentResponse(ack);
    }

    private void handleProtocolPaymentResponseForBitPay(String response) {
        Optional<PaymentProtocolPaymentAck> maybeAck = PaymentProtocolPaymentAck.createForBitPay(response);
        if (!maybeAck.isPresent()) {
            showError("Failed to parse BitPay ack");
            return;
        }

        PaymentProtocolPaymentAck ack = maybeAck.get();
        logProtocolAck(ack);
        handleProtocolPaymentResponse(ack);
    }

    private void handleProtocolPaymentResponse(PaymentProtocolPaymentAck ack) {
        Optional<String> maybeMemo = ack.getMemo();

        StringBuilder builder = new StringBuilder("<b>Transaction:</b> Submitted");
        if (maybeMemo.isPresent()) {
            builder.append(
                    String.format("<br><b>Memo:</b> %s",
                            Html.escapeHtml(maybeMemo.get()))
            );
        }

        runOnUiThread(() -> new AlertDialog.Builder(this)
                .setTitle("Payment Received")
                .setMessage(Html.fromHtml(builder.toString()))
                .setCancelable(false)
                .setNeutralButton("Ok", (dialog, which) -> finish())
                .show());
    }

    //
    // Misc.
    //

    private void logProtocolRequest(PaymentProtocolRequest request) {
        String commonName = request.getCommonName().orNull();
        String memo = request.getMemo().orNull();
        String paymentUrl = request.getPaymentUrl().orNull();
        Address address = request.getPrimaryTarget().orNull();
        NetworkFee networkFee = request.getRequiredNetworkFee().orNull();
        Amount amount = request.getTotalAmount().orNull();
        boolean isSecure = request.isSecure();
        PaymentProtocolError error = request.validate().orNull();

        Log.log(Level.FINE, "Secure:       " + isSecure);
        Log.log(Level.FINE, "Common Name:  " + commonName);
        Log.log(Level.FINE, "Memo:         " + memo);
        Log.log(Level.FINE, "Payment URL:  " + paymentUrl);
        Log.log(Level.FINE, "Fee Required: " + (networkFee != null));
        Log.log(Level.FINE, "Address:      " + address);
        Log.log(Level.FINE, "Amount:       " + amount);
        Log.log(Level.FINE, "Error:        " + error);
    }

    private void logProtocolAck(PaymentProtocolPaymentAck ack) {
        String memo = ack.getMemo().orNull();

        Log.log(Level.FINE, "Memo (ACK):   " + memo);
    }

    private void logFeeEstimate(TransferFeeBasis feeBasis) {
        Log.log(Level.FINE, "Fee:          " + feeBasis.getFee().toString());
    }

    private void showError(String message) {
        runOnUiThread(() -> new AlertDialog.Builder(this)
                .setTitle("Error")
                .setMessage(message)
                .setCancelable(false)
                .setNeutralButton("Ok", (dialog, which) -> { })
                .show());
    }
}
