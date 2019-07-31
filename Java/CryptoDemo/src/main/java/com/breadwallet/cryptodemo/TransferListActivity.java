package com.breadwallet.cryptodemo;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.util.SortedList;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.util.SortedListAdapterCallback;
import android.text.Html;
import android.text.Spanned;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferConfirmation;
import com.breadwallet.crypto.TransferHash;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.events.wallet.DefaultWalletEventVisitor;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.wallet.WalletListener;
import com.breadwallet.crypto.events.wallet.WalletTransferAddedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferSubmittedEvent;
import com.google.common.base.Optional;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import javax.annotation.Nullable;

public class TransferListActivity extends AppCompatActivity implements WalletListener {

    private static final DateFormat DATE_FORMAT = DateFormat.getDateTimeInstance(DateFormat.LONG, DateFormat.LONG);

    private static final String EXTRA_WALLET_NAME = "com.breadwallet.cryptodemo,TransferListActivity.EXTRA_WALLET_NAME";

    private static final Comparator<Transfer> OLDEST_FIRST_COMPARATOR = (o1, o2) -> {
        Optional<TransferConfirmation> oc1 = o1.getConfirmation();
        Optional<TransferConfirmation> oc2 = o2.getConfirmation();

        if (oc1.isPresent() && oc2.isPresent()) {
            TransferConfirmation c1 = oc1.get();
            TransferConfirmation c2 = oc2.get();

            int blockCompare = c1.getBlockNumber().compareTo(c2.getBlockNumber());
            int indexCompare = c1.getTransactionIndex().compareTo(c2.getTransactionIndex());
            return (blockCompare != 0 ? blockCompare : indexCompare);

        } else if (oc1.isPresent()) {
            return -1;
        } else if (oc2.isPresent()) {
            return 1;
        } else {
            return o1.hashCode() - o2.hashCode();
        }
    };

    private static final Comparator<Transfer> NEWEST_FIRST_COMPARATOR = Collections.reverseOrder(OLDEST_FIRST_COMPARATOR);

    private static final Comparator<Transfer> DEFAULT_COMPARATOR = NEWEST_FIRST_COMPARATOR;

    public static void start(Activity callerActivity, Wallet wallet) {
        Intent intent = new Intent(callerActivity, TransferListActivity.class);
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

    private Wallet wallet;

    private Button sendView;
    private Button recvView;
    private Adapter transferAdapter;
    private RecyclerView transfersView;
    private RecyclerView.LayoutManager transferLayoutManager;

    private ClipboardManager clipboardManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_transfer_list);

        CoreCryptoApplication.initialize(this);

        wallet = getWallet(getIntent());
        if (null == wallet) {
            finish();
            return;
        }

        clipboardManager = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);

        sendView = findViewById(R.id.send_view);
        sendView.setOnClickListener(v -> TransferCreateActivity.start(TransferListActivity.this, wallet));

        recvView = findViewById(R.id.receive_view);
        recvView.setOnClickListener(v -> copyReceiveAddress());

        transfersView = findViewById(R.id.transfer_recycler_view);
        transfersView.addItemDecoration(new DividerItemDecoration(getApplicationContext(), DividerItemDecoration.VERTICAL));

        transferLayoutManager = new LinearLayoutManager(this);
        transfersView.setLayoutManager(transferLayoutManager);

        transferAdapter = new Adapter(DEFAULT_COMPARATOR, (transfer) -> TransferDetailsActivity.start(this, wallet, transfer));
        transfersView.setAdapter(transferAdapter);

        ActionBar actionBar = getSupportActionBar();
        actionBar.setTitle(String.format("Wallet: %s", wallet.getName()));
    }

    @Override
    protected void onResume() {
        super.onResume();

        CoreCryptoApplication.getDispatchingSystemListener().addWalletListener(wallet, this);

        transferAdapter.set(new ArrayList<>(wallet.getTransfers()));
    }

    @Override
    protected void onPause() {
        super.onPause();

        CoreCryptoApplication.getDispatchingSystemListener().removeWalletListener(wallet, this);
    }

    @Override
    public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        runOnUiThread(() -> {
            event.accept(new DefaultWalletEventVisitor<Void>() {
                @Override
                public Void visit(WalletTransferAddedEvent event) {
                    transferAdapter.add(event.getTransfer());
                    return null;
                }

                @Override
                public Void visit(WalletTransferChangedEvent event) {
                    transferAdapter.changed(event.getTransfer());
                    return null;
                }

                @Override
                public Void visit(WalletTransferDeletedEvent event) {
                    transferAdapter.remove(event.getTransfer());
                    return null;
                }

                @Override
                public Void visit(WalletTransferSubmittedEvent event) {
                    transferAdapter.changed(event.getTransfer());
                    return null;
                }
            });
        });
    }

    private void copyReceiveAddress() {
        String value = wallet.getTarget().toString();
        clipboardManager.setPrimaryClip(ClipData.newPlainText("ReceiveAddress", value));

        String escapedValue = Html.escapeHtml(value);
        Spanned message = Html.fromHtml(String.format("Copied receive address <b>%s</b> to clipboard", escapedValue));

        Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT).show();
    }

    private interface OnItemClickListener<T> {
        void onItemClick(T item);
    }

    private static class Adapter extends RecyclerView.Adapter<ViewHolder> {

        private final OnItemClickListener<Transfer> listener;
        private final SortedList<Transfer> transfers;

        Adapter(Comparator<Transfer> comparator, OnItemClickListener<Transfer> listener) {
            this.listener = listener;
            this.transfers = new SortedList<>(Transfer.class, new SortedListAdapterCallback<Transfer>(this) {
                @Override
                public int compare(Transfer t1, Transfer t2) {
                    return comparator.compare(t1, t2);
                }

                @Override
                public boolean areContentsTheSame(Transfer t1, Transfer t2) {
                    return false;
                }

                @Override
                public boolean areItemsTheSame(Transfer t1, Transfer t2) {
                    return t1.equals(t2);
                }
            });
        }

        @NonNull
        @Override
        public ViewHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int i) {
            View v = LayoutInflater.from(viewGroup.getContext()).inflate(R.layout.layout_transfer_item, viewGroup, false);
            return new ViewHolder(v);
        }

        @Override
        public void onBindViewHolder(@NonNull ViewHolder vh, int i) {
            Transfer transfer = transfers.get(i);

            String dateText = transfer.getConfirmation()
                    .transform((c) -> DATE_FORMAT.format(c.getConfirmationTime())).or("<pending>");

            String addressText = transfer.getHash().transform(TransferHash::toString).or("<pending>");
            addressText = String.format("Hash: %s", addressText);

            String amountText = transfer.getAmountDirected().toString();
            String feeText = String.format("Fee: %s", transfer.getFee());

            String stateText = String.format("State: %s", transfer.getState());

            vh.itemView.setOnClickListener(v -> listener.onItemClick(transfer));
            vh.dateView.setText(dateText);
            vh.amountView.setText(amountText);
            vh.addressView.setText(addressText);
            vh.feeView.setText(feeText);
            vh.stateView.setText(stateText);
        }

        @Override
        public int getItemCount() {
            return transfers.size();
        }

        private void set(List<Transfer> newTransfers) {
            transfers.replaceAll(newTransfers);
        }

        private void add(Transfer transfer) {
            transfers.add(transfer);
        }

        private void remove(Transfer transfer) {
            transfers.remove(transfer);
        }

        private void changed(Transfer transfer) {
            int index = transfers.indexOf(transfer);
            if (index != -1) {
                transfers.updateItemAt(index, transfer);
            }
        }
    }

    private static class ViewHolder extends RecyclerView.ViewHolder {

        private TextView dateView;
        private TextView amountView;
        private TextView addressView;
        private TextView feeView;
        private TextView stateView;

        private ViewHolder(@NonNull View view) {
            super(view);

            dateView = view.findViewById(R.id.item_date);
            amountView = view.findViewById(R.id.item_amount);
            addressView = view.findViewById(R.id.item_address);
            feeView = view.findViewById(R.id.item_fee);
            stateView = view.findViewById(R.id.item_state);
        }
    }
}
