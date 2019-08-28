package com.breadwallet.cryptodemo;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.Nullable;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.Toolbar;
import android.text.Editable;
import android.text.Html;
import android.text.Spanned;
import android.text.TextWatcher;
import android.widget.Button;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.TextView;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.NetworkFee;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferFeeBasis;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.errors.FeeEstimationError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;

public class TransferCreateSendActivity extends AppCompatActivity {

    private static final double MIN_VALUE = 0.0;
    private static final double MAX_VALUE = 0.001;

    private static final String EXTRA_WALLET_NAME = "com.breadwallet.cryptodemo,TransferCreateSendActivity.EXTRA_WALLET_NAME";

    public static void start(Activity callerActivity, Wallet wallet) {
        Intent intent = new Intent(callerActivity, TransferCreateSendActivity.class);
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
    private Unit baseUnit;
    private NetworkFee fee;
    @Nullable
    private TransferFeeBasis feeBasis;

    private double maxValue;

    private EditText receiverView;
    private SeekBar amountView;
    private TextView amountValueView;
    private TextView feeView;
    private Button submitView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_transfer_create);

        CoreCryptoApplication.initialize(this);

        wallet = getWallet(getIntent());
        if (null == wallet) {
            finish();
            return;
        }
        walletManager = wallet.getWalletManager();
        network = walletManager.getNetwork();
        fee = walletManager.getDefaultNetworkFee();
        baseUnit = wallet.getUnit();
        maxValue = wallet.getBalance().doubleAmount(baseUnit).or(MAX_VALUE);

        receiverView = findViewById(R.id.receiver_view);
        amountView = findViewById(R.id.amount_view);
        amountValueView = findViewById(R.id.amount_value_view);
        feeView = findViewById(R.id.fee_view);
        submitView = findViewById(R.id.submit_view);
        TextView amountMinView = findViewById(R.id.amount_min_view);
        TextView amountMaxView = findViewById(R.id.amount_max_view);

        amountMinView.setText(Amount.create(MIN_VALUE, baseUnit).toString());
        amountMaxView.setText(Amount.create(maxValue, baseUnit).toString());

        int amountViewProgress = 50;
        amountView.setProgress(amountViewProgress);
        amountView.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                updateFee();
                updateViewOnChange(progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        CharSequence receiverViewText = network.isMainnet() ? "" : wallet.getTarget().toString();
        receiverView.setText(receiverViewText);
        receiverView.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {
                updateFee();
                updateViewOnChange(s);
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }
        });

        submitView.setOnClickListener(v -> {
            String addressStr = receiverView.getText().toString();
            Optional<? extends Address> target = network.addressFor(addressStr);
            if (!target.isPresent()) {
                showError("Invalid target address");
                return;
            }

            TransferFeeBasis feeBasis = this.feeBasis;
            if (feeBasis == null) {
                showError("Invalid fee");
                return;
            }

            Amount amount = Amount.create(calculateValue(amountView.getProgress()), baseUnit);
            showConfirmTransfer(target.get(), amount, feeBasis);
        });

        Toolbar toolbar = findViewById(R.id.toolbar_view);
        setSupportActionBar(toolbar);

        updateFee();
        updateView(amountViewProgress, receiverViewText);;
    }

    private void updateFee() {
        String addressStr = receiverView.getText().toString();
        Optional<? extends Address> target = network.addressFor(addressStr);
        if (!target.isPresent()) {
            return;
        }

        Amount amount = Amount.create(calculateValue(amountView.getProgress()), baseUnit);
        wallet.estimateFee(target.get(), amount, fee, new CompletionHandler<TransferFeeBasis,
                FeeEstimationError>() {
            @Override
            public void handleData(TransferFeeBasis data) {
                runOnUiThread(() -> {
                    feeBasis = data;
                    updateViewOnFeeEstimate();
                });
            }

            @Override
            public void handleError(FeeEstimationError error) {
                runOnUiThread(() -> {
                    feeBasis = null;
                    updateViewOnFeeEstimate();
                });
            }
        });
    }

    private void updateViewOnFeeEstimate() {
        updateView(amountView.getProgress(), receiverView.getText());
    }

    private void updateViewOnChange(Editable receiver) {
        updateView(amountView.getProgress(), receiver);
    }

    private void updateViewOnChange(int progress) {
        updateView(progress, receiverView.getText());
    }

    private void updateView(int progress, CharSequence receiver) {
        double value = calculateValue(progress);

        Amount amount = Amount.create(value, baseUnit);
        Optional<? extends Address> target = network.addressFor(receiver.toString());

        if (value == 0) {
            // we have a valid amount but it is zero...
            submitView.setEnabled(false);
            amountValueView.setText(amount.toString());
            feeView.setText("");
        } else if (!target.isPresent()){
            // we don't have a valid target...
            submitView.setEnabled(false);
            amountValueView.setText(amount.toString());
            feeView.setText("");
        } else if (feeBasis == null) {
            // we have a valid target but it don't have a fee estimate...
            submitView.setEnabled(false);
            amountValueView.setText(amount.toString());
            feeView.setText("");
        } else {
            // we have it all!
            submitView.setEnabled(true);
            amountValueView.setText(amount.toString());
            feeView.setText(feeBasis.getFee().toString());
        }
    }

    private double calculateValue(int percentage) {
        return ((maxValue - MIN_VALUE) * percentage / 100) + MIN_VALUE;
    }

    private void showConfirmTransfer(Address target, Amount amount, TransferFeeBasis feeBasis) {
        String escapedTarget = Html.escapeHtml(target.toString());
        String escapedAmount = Html.escapeHtml(amount.toString());
        Spanned message = Html.fromHtml(String.format("Send <b>%s</b> to <b>%s</b>?", escapedAmount, escapedTarget));

        new AlertDialog.Builder(this)
                .setTitle("Confirmation")
                .setMessage(message)
                .setNegativeButton("Cancel", (dialog, which) -> {})
                .setPositiveButton("Continue", (dialog, which) -> {
                    Optional<? extends Transfer> transfer = wallet.createTransfer(target, amount, feeBasis);
                    if (!transfer.isPresent()) {
                        showError("Balance too low?");
                    } else {
                        walletManager.submit(transfer.get(), CoreCryptoApplication.getPaperKey());
                        finish();
                    }
                })
                .show();
    }

    private void showError(String message) {
        new AlertDialog.Builder(this)
                .setTitle("Error")
                .setMessage(message)
                .setNeutralButton("Ok", (dialog, which) -> { })
                .show();
    }
}
