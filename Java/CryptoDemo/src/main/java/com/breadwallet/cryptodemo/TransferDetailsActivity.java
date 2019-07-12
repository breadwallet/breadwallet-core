package com.breadwallet.cryptodemo;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferConfirmation;
import com.breadwallet.crypto.TransferHash;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.transfer.TransferListener;
import com.breadwallet.crypto.events.walletmanager.DefaultWalletManagerEventVisitor;
import com.breadwallet.crypto.events.walletmanager.WalletManagerBlockUpdatedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerListener;
import com.google.common.base.Optional;

import java.text.DateFormat;

import javax.annotation.Nullable;

public class TransferDetailsActivity extends AppCompatActivity implements TransferListener, WalletManagerListener {

    private static final DateFormat DATE_FORMAT = DateFormat.getDateTimeInstance(DateFormat.LONG, DateFormat.LONG);

    private static final String EXTRA_WALLET_NAME = "com.breadwallet.cryptodemo,TransferDetailsActivity.EXTRA_WALLET_NAME";
    private static final String EXTRA_TXN_ID = "com.breadwallet.cryptodemo,TransferDetailsActivity.EXTRA_TXN_ID";

    public static void start(Activity callerActivity, Wallet wallet, Transfer transfer) {
        Intent intent = new Intent(callerActivity, TransferDetailsActivity.class);
        intent.putExtra(EXTRA_WALLET_NAME, wallet.getName());
        intent.putExtra(EXTRA_TXN_ID, transfer.hashCode());
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
    private static Transfer getTransfer(Intent intent, Wallet wallet) {
        // TODO: This isn't a good way to do this; what in the existing API supports this type of operation?
        int txnId = intent.getIntExtra(EXTRA_TXN_ID, 0);
        for(Transfer transfer: wallet.getTransfers()) {
            if (transfer.hashCode() == txnId) {
                return transfer;
            }
        }
        return null;
    }

    private Wallet wallet;
    private Transfer transfer;
    private ClipboardManager clipboardManager;

    private TextView amountView;
    private TextView feeView;
    private TextView dateView;
    private TextView senderView;
    private TextView receiverView;
    private TextView identifierView;
    private TextView confirmationView;
    private TextView confirmationCountView;
    private View confirmationContainerView;
    private TextView stateView;
    private TextView directionView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_transfer_details);

        CoreCryptoApplication.initialize(this);

        Intent intent = getIntent();
        wallet = getWallet(intent);
        if (null == wallet) {
            finish();
            return;
        }

        transfer = getTransfer(intent, wallet);
        if (null == transfer) {
            finish();
            return;
        }

        clipboardManager = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);

        amountView = findViewById(R.id.amount_view);
        feeView = findViewById(R.id.fee_view);
        dateView = findViewById(R.id.date_view);
        senderView = findViewById(R.id.sender_view);
        receiverView = findViewById(R.id.receiver_view);
        identifierView = findViewById(R.id.identifier_view);
        confirmationView = findViewById(R.id.confirmation_view);
        confirmationCountView = findViewById(R.id.confirmation_count_view);
        confirmationContainerView = findViewById(R.id.confirmation_count_container_view);
        stateView = findViewById(R.id.state_view);
        directionView = findViewById(R.id.direction_view);
    }

    @Override
    protected void onResume() {
        super.onResume();

        CoreCryptoApplication.getListener().addListener((WalletManagerListener) this);
        CoreCryptoApplication.getListener().addListener((TransferListener) this);

        updateView();
    }

    @Override
    protected void onPause() {
        super.onPause();

        CoreCryptoApplication.getListener().removeListener((TransferListener) this);
        CoreCryptoApplication.getListener().removeListener((WalletManagerListener) this);
    }

    private void updateView() {
        Optional<TransferConfirmation> confirmation = transfer.getConfirmation();

        String amountText = transfer.getAmountDirected().toString();
        amountView.setText(amountText);
        amountView.setOnClickListener(v -> copyPlaintext("Amount", amountView.getText()));

        String feeText = transfer.getFee().toString();
        feeView.setText(feeText);
        feeView.setOnClickListener(v -> copyPlaintext("Fee", feeView.getText()));

        String dateText = confirmation.transform((c) -> DATE_FORMAT.format(c.getConfirmationTime())).or("<pending>");
        dateView.setText(dateText);

        String senderText = transfer.getSource().transform(Address::toString).or("<unknown>");
        senderView.setText(senderText);
        senderView.setOnClickListener(v -> copyPlaintext("Sender", senderView.getText()));

        String receiverText = transfer.getTarget().transform(Address::toString).or("<unknown>");
        receiverView.setText(receiverText);
        receiverView.setOnClickListener(v -> copyPlaintext("Receiver", receiverView.getText()));

        String identifierText = transfer.getHash().transform(TransferHash::toString).or("<pending>");
        identifierView.setText(identifierText);
        identifierView.setOnClickListener(v -> copyPlaintext("Identifier", identifierView.getText()));

        String confirmationText = confirmation.transform((c) -> "Yes @ " + c.getBlockNumber()).or("No");
        confirmationView.setText(confirmationText);

        String confirmationCountText = transfer.getConfirmations().transform(String::valueOf).or("");
        confirmationCountView.setText(confirmationCountText);

        confirmationContainerView.setVisibility(confirmation.isPresent() ? View.VISIBLE : View.INVISIBLE);

        String stateText = transfer.getState().toString();
        stateView.setText(stateText);

        String directionText = transfer.getDirection().toString();
        directionView.setText(directionText);
    }

    @Override
    public void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
        runOnUiThread(() -> {
            if (manager.equals(this.wallet.getWalletManager())) {
                event.accept(new DefaultWalletManagerEventVisitor<Void>() {
                    @Override
                    public Void visit(WalletManagerBlockUpdatedEvent event) {
                        updateView();
                        return null;
                    }
                });
            }
        });
    }

    @Override
    public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
        runOnUiThread(() -> {
            if (manager.equals(this.wallet.getWalletManager()) && wallet.equals(this.wallet) && transfer.equals(this.transfer)) {
                updateView();
            }
        });
    }

    private void copyPlaintext(String label, CharSequence value) {
        clipboardManager.setPrimaryClip(ClipData.newPlainText(label, value));
        Toast.makeText(getApplicationContext(), String.format("Copied \"%s\" to clipboard", value), Toast.LENGTH_SHORT).show();
    }
}
