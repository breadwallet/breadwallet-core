package com.breadwallet.cryptodemo;

import android.app.Activity;
import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferHash;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.transfer.TransferChangedEvent;
import com.breadwallet.crypto.events.transfer.TransferConfirmationEvent;
import com.breadwallet.crypto.events.transfer.TransferCreatedEvent;
import com.breadwallet.crypto.events.transfer.TransferDeletedEvent;
import com.breadwallet.crypto.events.transfer.TransferEventVisitor;
import com.breadwallet.crypto.events.transfer.TransferListener;

import java.text.DateFormat;
import java.util.Date;

import javax.annotation.Nullable;

public class TransferDetailsActivity extends AppCompatActivity implements TransferListener {

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

    private TextView amountView;
    private TextView feeView;
    private TextView dateView;
    private TextView senderView;
    private TextView receiverView;
    private TextView identifierView;
    private TextView confirmationView;
    private TextView confirmationCountView;
    private TextView stateView;

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
        wallet = getWallet(intent);
        if (null == transfer) {
            finish();
            return;
        }

        amountView = findViewById(R.id.amount_view);
        feeView = findViewById(R.id.fee_view);
        dateView = findViewById(R.id.date_view);
        senderView = findViewById(R.id.sender_view);
        receiverView = findViewById(R.id.receiver_view);
        identifierView = findViewById(R.id.identifier_view);
        confirmationView = findViewById(R.id.confirmation_view);
        confirmationCountView = findViewById(R.id.confirmation_count_view);
        stateView = findViewById(R.id.state_view);
    }

    @Override
    protected void onResume() {
        super.onResume();

        CoreCryptoApplication.getListener().addListener(this);

        String amountText = transfer.getAmountDirected().toString();
        amountView.setText(amountText);

        String feeText = transfer.getFee().toString();
        feeView.setText(feeText);

        String dateText = transfer.getConfirmation().transform((c) -> DATE_FORMAT.format(new Date(c.getTimestamp() * 1000))).or("<pending>");
        dateView.setText(dateText);

        String senderText = transfer.getSource().transform(Address::toString).or("<unknown>");
        senderView.setText(senderText);

        String receiverText = transfer.getTarget().transform(Address::toString).or("<unknown>");
        receiverView.setText(receiverText);

        String identifierText = transfer.getHash().transform(TransferHash::toString).or("<pending>");
        identifierView.setText(identifierText);

        String confirmationText = transfer.getConfirmation().transform((c) -> "Yes @ " + c.getBlockNumber()).or("No");
        confirmationView.setText(confirmationText);

        String confirmationCountText = transfer.getConfirmations().transform(String::valueOf).or("");
        confirmationCountView.setText(confirmationCountText);

        String stateText = transfer.getState().toString();
        stateView.setText(stateText);
    }

    @Override
    protected void onPause() {
        super.onPause();

        CoreCryptoApplication.getListener().removeListener(this);
    }

    @Override
    public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
        // TODO: This should be filtered to only events on our transfer
        runOnUiThread(() -> {
            event.accept(new TransferEventVisitor<Void>() {
                @Override
                public Void visit(TransferChangedEvent event) {
                    String confirmationText = transfer.getConfirmation().transform((c) -> "Yes @ " + c.getBlockNumber()).or("No");
                    confirmationView.setText(confirmationText);
                    return null;
                }

                @Override
                public Void visit(TransferConfirmationEvent event) {
                    String confirmationCountText = transfer.getConfirmations().transform(String::valueOf).or("");
                    confirmationCountView.setText(confirmationCountText);
                    return null;
                }

                @Override
                public Void visit(TransferCreatedEvent event) {
                    return null;
                }

                @Override
                public Void visit(TransferDeletedEvent event) {
                    return null;
                }
            });
        });
    }
}
