package com.breadwallet.cryptodemo;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.Nullable;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.widget.Button;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.TextView;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferFeeBasis;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.google.common.base.Optional;

public class TransferCreateActivity extends AppCompatActivity {

    private static final double MIN_VALUE = 0.0;
    private static final double MAX_VALUE = 0.001;

    private static final String EXTRA_WALLET_NAME = "com.breadwallet.cryptodemo,TransferListActivity.EXTRA_WALLET_NAME";

    public static void start(Activity callerActivity, Wallet wallet) {
        Intent intent = new Intent(callerActivity, TransferCreateActivity.class);
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

    private EditText receiverView;
    private SeekBar amountView;
    private TextView amountMinView;
    private TextView amountMaxView;
    private TextView amountValueView;
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

        receiverView = findViewById(R.id.receiver_view);
        amountView = findViewById(R.id.amount_view);
        amountMinView = findViewById(R.id.amount_min_view);
        amountMaxView = findViewById(R.id.amount_max_view);
        amountValueView = findViewById(R.id.amount_value_view);
        submitView = findViewById(R.id.submit_view);

        amountMinView.setText(String.valueOf(MIN_VALUE));
        amountMaxView.setText(String.valueOf(MAX_VALUE));

        amountView.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                Optional<Amount> amount = Amount.create(calculateValue(progress), wallet.getBaseUnit());
                amountValueView.setText(amount.transform((a) -> a.toString()).or("<nan>"));
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
        amountView.setProgress(50);

        receiverView.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {

            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {

            }

            @Override
            public void afterTextChanged(Editable s) {
                submitView.setEnabled(s.length() != 0);
            }
        });
        receiverView.setText(network.isMainnet() ? "" : wallet.getTarget().toString());

        submitView.setOnClickListener(v -> {
            String addressStr = receiverView.getText().toString();
            Optional<? extends Address> target = network.addressFor(addressStr);
            if (!target.isPresent()) {
                showError("Invalid target address");
                return;
            }

            Optional<Amount> amount = Amount.create(calculateValue(amountView.getProgress()), wallet.getBaseUnit());
            if (!amount.isPresent()) {
                showError("Invalid amount");
                return;
            }

            showConfirmTransfer(target.get(), amount.get());
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    private double calculateValue(int percentage) {
        return ((MAX_VALUE - MIN_VALUE) * percentage / 100) + MIN_VALUE;
    }

    private boolean isEthCurrency() {
        return wallet.getCurrency().getCode().equals(Currency.CODE_AS_ETH);
    }

    private boolean isBitCurrency() {
        return wallet.getCurrency().getCode().equals(Currency.CODE_AS_BTC) ||
                wallet.getCurrency().getCode().equals(Currency.CODE_AS_BCH);
    }

    private boolean isTokCurrency() {
        return wallet.getCurrency().getCode().equals(Currency.CODE_AS_ETH);
    }

    private TransferFeeBasis getFeeBasis() {
        if (isEthCurrency()) {
            // TODO: Implement this!
            throw new UnsupportedOperationException("Unsupported currency");
        } else {
            return wallet.getDefaultFeeBasis();
        }
    }

    private void showConfirmTransfer(Address target, Amount amount) {
        new AlertDialog.Builder(this)
                .setTitle("Confirmation")
                .setMessage(String.format("Send %s to %s?", amount, target.toString()))
                .setNegativeButton("Cancel", (dialog, which) -> {})
                .setPositiveButton("Continue", (dialog, which) -> {
                    Optional<? extends Transfer> transfer = wallet.createTransfer(target, amount, getFeeBasis());
                    if (!transfer.isPresent()) {
                        showError("Balance too low?");
                        return;
                    }

                    showConfirmSubmission(transfer.get());
                })
                .show();
    }

    private void showConfirmSubmission(Transfer transfer) {
        new AlertDialog.Builder(this)
                .setTitle("Confirmation")
                .setMessage(String.format("Proceed with %s as fee?", transfer.getFee()))
                .setNegativeButton("Cancel", (dialog, which) -> {})
                .setPositiveButton("Continue", (dialog, which) -> {
                    walletManager.submit(transfer, CoreCryptoApplication.getPaperKey());
                    finish();
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
