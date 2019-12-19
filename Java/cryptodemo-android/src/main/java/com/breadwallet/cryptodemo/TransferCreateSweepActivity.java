/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
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
import android.text.Editable;
import android.text.Html;
import android.text.Spanned;
import android.text.TextWatcher;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.Key;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.NetworkFee;
import com.breadwallet.crypto.TransferFeeBasis;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.WalletSweeper;
import com.breadwallet.crypto.errors.FeeEstimationError;
import com.breadwallet.crypto.errors.WalletSweeperError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;

import java.nio.charset.StandardCharsets;

public class TransferCreateSweepActivity extends AppCompatActivity {

    private static final String EXTRA_WALLET_NAME = "com.breadwallet.cryptodemo,TransferCreateSweepActivity.EXTRA_WALLET_NAME";

    public static void start(Activity callerActivity, Wallet wallet) {
        Intent intent = new Intent(callerActivity, TransferCreateSweepActivity.class);
        intent.putExtra(EXTRA_WALLET_NAME, wallet.getName());
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

    private Network network;
    private WalletManager walletManager;
    private Wallet wallet;
    private NetworkFee fee;
    @Nullable
    private TransferFeeBasis feeBasis;
    @Nullable
    private WalletSweeper sweeper;

    private EditText keyView;
    private TextView amountValueView;
    private TextView feeView;
    private Button submitView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_transfer_create_sweep);

        CoreCryptoApplication.initialize(this);

        wallet = getWallet(getIntent());
        if (null == wallet) {
            finish();
            return;
        }
        walletManager = wallet.getWalletManager();
        network = walletManager.getNetwork();
        fee = walletManager.getDefaultNetworkFee();

        keyView = findViewById(R.id.key_view);
        amountValueView = findViewById(R.id.amount_value_view);
        feeView = findViewById(R.id.fee_view);
        submitView = findViewById(R.id.submit_view);

        keyView.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {
                updateValues();
                updateViewOnChange(s);
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }
        });
        keyView.setText("");

        submitView.setOnClickListener(v -> {
            TransferFeeBasis feeBasis = this.feeBasis;
            if (feeBasis == null) {
                showError("Invalid fee");
                return;
            }

            WalletSweeper sweeper = this.sweeper;
            if (sweeper == null) {
                showError("Invalid sweeper");
                return;
            }

            Optional<? extends Amount> maybeAmount = sweeper.getBalance();
            if (!maybeAmount.isPresent()) {
                showError("Invalid amount");
                return;
            }

            showConfirmTransfer(sweeper, maybeAmount.get(), feeBasis);
        });

        Toolbar toolbar = findViewById(R.id.toolbar_view);
        setSupportActionBar(toolbar);

        updateValues();
        updateView("");
    }

    private void updateValues() {
        byte[] privateKeyUtf8 = keyView.getText().toString().getBytes(StandardCharsets.UTF_8);
        Optional<? extends Key> key = Key.createFromPrivateKeyString(privateKeyUtf8);
        if (!key.isPresent()) {
            return;
        }

        walletManager.createSweeper(wallet, key.get(), new CompletionHandler<WalletSweeper, WalletSweeperError>() {
            @Override
            public void handleData(WalletSweeper s) {
                runOnUiThread(() -> {
                    feeBasis = null;
                    sweeper = s;
                    updateViewOnSweeper();

                    s.estimate(fee, new CompletionHandler<TransferFeeBasis, FeeEstimationError>() {
                        @Override
                        public void handleData(TransferFeeBasis f) {
                            runOnUiThread(() -> {
                                if (s == TransferCreateSweepActivity.this.sweeper) {
                                    feeBasis = f;
                                    updateViewOnFeeEstimate();
                                }
                            });
                        }

                        @Override
                        public void handleError(FeeEstimationError error) {
                            runOnUiThread(() -> {
                                if (s == TransferCreateSweepActivity.this.sweeper) {
                                    feeBasis = null;
                                    updateViewOnFeeEstimate();
                                }
                            });
                        }
                    });
                });
            }

            @Override
            public void handleError(WalletSweeperError error) {
                runOnUiThread(() -> {
                    feeBasis = null;
                    sweeper = null;
                    updateViewOnSweeper();
                });
            }
        });
    }

    private void updateViewOnSweeper() {
        updateView(keyView.getText());
    }

    private void updateViewOnFeeEstimate() {
        updateView(keyView.getText());
    }

    private void updateViewOnChange(Editable receiver) {
        updateView(receiver);
    }

    private void updateView(CharSequence keySequence) {
        Optional<? extends Key> key = Key.createFromPrivateKeyString(keySequence.toString().getBytes(StandardCharsets.UTF_8));

        if (!key.isPresent()){
            // we don't have a valid target...
            submitView.setEnabled(false);
            amountValueView.setText("");
            feeView.setText("");
        } else if (feeBasis == null || sweeper == null) {
            // we have a valid target but it don't have info...
            submitView.setEnabled(false);
            amountValueView.setText("");
            feeView.setText("");
        } else {
            // we have it all!
            submitView.setEnabled(true);
            amountValueView.setText(sweeper.getBalance().get().toString());
            feeView.setText(feeBasis.getFee().toString());
        }
    }

    private void showConfirmTransfer(WalletSweeper sweeper, Amount amount, TransferFeeBasis feeBasis) {
        String escapedAmount = Html.escapeHtml(amount.toString());
        Spanned message = Html.fromHtml(String.format("Sweep <b>%s</b>?", escapedAmount));

        new AlertDialog.Builder(this)
                .setTitle("Confirmation")
                .setMessage(message)
                .setNegativeButton("Cancel", (dialog, which) -> {})
                .setPositiveButton("Continue", (dialog, which) -> {
                    sweeper.submit(feeBasis);
                    finish();
                })
                .show();
    }

    private void showError(String message) {
        new AlertDialog.Builder(this)
                .setTitle("Error")
                .setMessage(message)
                .setCancelable(false)
                .setNeutralButton("Ok", (dialog, which) -> { })
                .show();
    }
}
