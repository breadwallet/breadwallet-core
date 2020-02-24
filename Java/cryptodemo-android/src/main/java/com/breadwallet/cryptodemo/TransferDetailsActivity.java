/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.cryptodemo;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.Toolbar;
import android.text.Html;
import android.text.Spanned;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferAttribute;
import com.breadwallet.crypto.TransferConfirmation;
import com.breadwallet.crypto.TransferHash;
import com.breadwallet.crypto.TransferState;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.events.system.DefaultSystemListener;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.walletmanager.DefaultWalletManagerEventVisitor;
import com.breadwallet.crypto.events.walletmanager.WalletManagerBlockUpdatedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;
import com.google.common.base.Optional;

import java.text.DateFormat;

public class TransferDetailsActivity extends AppCompatActivity implements DefaultSystemListener {

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

    private Transfer transfer;
    private WalletManager walletManager;
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
    private TextView attributesView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_transfer_details);

        CoreCryptoApplication.initialize(this);

        Intent intent = getIntent();
        Wallet wallet = getWallet(intent);
        if (null == wallet) {
            finish();
            return;
        }

        transfer = getTransfer(intent, wallet);
        if (null == transfer) {
            finish();
            return;
        }

        walletManager = wallet.getWalletManager();
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
        // attributesView = findViewById(R.id.attributes_view);

        Toolbar toolbar = findViewById(R.id.toolbar_view);
        setSupportActionBar(toolbar);
    }

    @Override
    protected void onResume() {
        super.onResume();

        CoreCryptoApplication.getDispatchingSystemListener().addWalletManagerListener(walletManager, this);
        CoreCryptoApplication.getDispatchingSystemListener().addTransferListener(transfer, this);
        loadTransfer();
    }

    @Override
    protected void onPause() {
        super.onPause();

        CoreCryptoApplication.getDispatchingSystemListener().removeTransferListener(transfer, this);
        CoreCryptoApplication.getDispatchingSystemListener().removeWalletManagerListener(walletManager, this);
    }

    @Override
    public void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
        event.accept(new DefaultWalletManagerEventVisitor<Void>() {
            @Override
            public Void visit(WalletManagerBlockUpdatedEvent event) {
                loadTransfer();
                return null;
            }
        });
    }

    @Override
    public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
        loadTransfer();
    }

    private void loadTransfer() {
        ApplicationExecutors.runOnUiExecutor(() -> {
            TransferViewModel viewModel = new TransferViewModel(transfer);
            runOnUiThread(() -> bindView(viewModel));
        });
    }

    private void bindView(TransferViewModel viewModel) {
        String amountText = viewModel.amountText();
        amountView.setText(amountText);
        amountView.setOnClickListener(v -> copyPlaintext("Amount", amountText));

        String feeText = viewModel.feeText();
        feeView.setText(feeText);
        feeView.setOnClickListener(v -> copyPlaintext("Fee", feeText));

        String dateText = viewModel.dateText();
        dateView.setText(dateText);

        String senderText = viewModel.senderText();
        senderView.setText(senderText);
        senderView.setOnClickListener(v -> copyPlaintext("Sender", senderText));

        String receiverText = viewModel.receiverText();
        receiverView.setText(receiverText);
        receiverView.setOnClickListener(v -> copyPlaintext("Receiver", receiverText));

        String identifierText = viewModel.identifierText();
        identifierView.setText(identifierText);
        identifierView.setOnClickListener(v -> copyPlaintext("Identifier", identifierText));

        String confirmationText = viewModel.confirmationText();
        confirmationView.setText(confirmationText);

        String confirmationCountText = viewModel.confirmationCountText();
        confirmationCountView.setText(confirmationCountText);

        confirmationContainerView.setVisibility(viewModel.confirmationCountVisible() ? View.VISIBLE : View.INVISIBLE);

        String stateText = viewModel.stateText();
        stateView.setText(stateText);

        String directionText = viewModel.directionText();
        directionView.setText(directionText);

        String attributesText = viewModel.attributesText();
        // attributesView.setText(attributesText);
    }

    private void copyPlaintext(String label, CharSequence value) {
        ApplicationExecutors.runOnUiExecutor(() -> {
            clipboardManager.setPrimaryClip(ClipData.newPlainText(label, value));

            String escapedValue = Html.escapeHtml(value);
            Spanned message = Html.fromHtml(String.format("Copied <b>%s</b> to clipboard", escapedValue));

            runOnUiThread(() -> Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT).show());
        });
    }

    private static class TransferViewModel {

        static final ThreadLocal<DateFormat> DATE_FORMAT = new ThreadLocal<DateFormat>() {
            @Override protected DateFormat initialValue() {
                return DateFormat.getDateTimeInstance(DateFormat.LONG, DateFormat.LONG);
            }
        };

        final Transfer transfer;

        final String amountText;
        final String feeText;
        final String dateText;
        final String senderText;
        final String receiverText;
        final String identifierText;
        final String confirmationText;
        final String confirmationCountText;
        final boolean confirmationCountVisible;
        final String stateText;
        final String directionText;
        final String attributesText;

        TransferViewModel(Transfer transfer) {
            this.transfer = transfer;

            Optional<TransferConfirmation> conf = transfer.getConfirmation();
            TransferState state = transfer.getState();

            this.amountText = transfer.getAmountDirected().toString();
            this.feeText = transfer.getFee().toString();
            this.senderText = transfer.getSource().transform(Address::toString).or("<unknown>");
            this.receiverText = transfer.getTarget().transform(Address::toString).or("<unknown>");
            this.identifierText = transfer.getHash().transform(TransferHash::toString).or("<pending>");
            this.confirmationText = conf.transform((c) -> "Yes @ " + c.getBlockNumber()).or("No");
            this.confirmationCountText = transfer.getConfirmations().transform(String::valueOf).or("");
            this.dateText = conf.transform((c) -> DATE_FORMAT.get().format(c.getConfirmationTime())).or("<pending>");
            this.confirmationCountVisible = conf.isPresent();
            this.stateText = conf.transform((c) ->
                    state.toString() +
                            (c.getSuccess()
                                ? ""
                                : String.format (" (%s)", c.getError().or("err"))))
                .or(state.toString());
            this.directionText = transfer.getDirection().toString();

            StringBuffer sb = new StringBuffer();
            String prefix = "";
            for (TransferAttribute a : transfer.getAttributes()) {
                sb.append(prefix);
                sb.append(String.format("%s(%s):%s",
                        ((TransferAttribute) a).getKey(),
                        (((TransferAttribute) a).isRequired() ? "R" : "O"),
                        ((TransferAttribute) a).getValue().or("")));
                prefix = ", ";
            }
            this.attributesText = sb.toString();

//            this.attributesText = transfer.getAttributes().stream()
//                    .map(a -> String.format ("%s(%s):%s",
//                            ((TransferAttribute) a).getKey(),
//                            (((TransferAttribute) a).isRequired() ? "R" : "O"),
//                            ((TransferAttribute) a).getValue().or("")))
//                    .collect(Collectors.joining(", "));
        }

        String amountText() {
            return amountText;
        }

        String feeText() {
            return feeText;
        }

        String dateText() {
            return dateText;
        }

        String senderText() {
            return senderText;
        }

        String receiverText() {
            return receiverText;
        }

        String identifierText() {
            return identifierText;
        }

        String confirmationText() {
            return confirmationText;
        }

        String confirmationCountText() {
            return confirmationCountText;
        }

        boolean confirmationCountVisible() {
            return confirmationCountVisible;
        }

        String stateText() {
            return stateText;
        }

        String directionText() {
            return directionText;
        }

        String attributesText () {
            return attributesText;
        }
    }
}
