package com.breadwallet.cryptodemo;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

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
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import javax.annotation.Nullable;

public class TransferListActivity extends AppCompatActivity implements TransferListener {

    private static final DateFormat DATE_FORMAT = DateFormat.getDateTimeInstance(DateFormat.LONG, DateFormat.LONG);

    private static final String EXTRA_WALLET_NAME = "com.breadwallet.cryptodemo,TransferListActivity.EXTRA_WALLET_NAME";

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
    private List<Transfer> transfers = new ArrayList<>();

    private Button createView;
    private RecyclerView transfersView;
    private RecyclerView.Adapter transferAdapter;
    private RecyclerView.LayoutManager transferLayoutManager;

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

        createView = findViewById(R.id.create_view);
        createView.setOnClickListener(v -> TransferCreateActivity.start(TransferListActivity.this, wallet));

        transfersView = findViewById(R.id.transfer_recycler_view);
        transfersView.addItemDecoration(new DividerItemDecoration(getApplicationContext(), DividerItemDecoration.VERTICAL));

        transferLayoutManager = new LinearLayoutManager(this);
        transfersView.setLayoutManager(transferLayoutManager);

        transferAdapter = new Adapter(transfers, (transfer) -> TransferDetailsActivity.start(this, wallet, transfer));
        transfersView.setAdapter(transferAdapter);

        ActionBar actionBar = getSupportActionBar();
        actionBar.setTitle(String.format("Wallet: %s", wallet.getName()));
    }

    @Override
    protected void onResume() {
        super.onResume();

        CoreCryptoApplication.getListener().addListener(this);

        transfers.clear();
        transfers.addAll(wallet.getTransfers());
        transferAdapter.notifyDataSetChanged();
    }

    @Override
    protected void onPause() {
        super.onPause();

        CoreCryptoApplication.getListener().removeListener(this);
    }

    @Override
    public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
        runOnUiThread(() -> {
            event.accept(new TransferEventVisitor<Void>() {
                @Override
                public Void visit(TransferChangedEvent event) {
                    int index = transfers.indexOf(transfer);
                    if (index != -1) {
                        transferAdapter.notifyItemChanged(index);
                    }
                    return null;
                }

                @Override
                public Void visit(TransferConfirmationEvent event) {
                    int index = transfers.indexOf(transfer);
                    if (index != -1) {
                        transferAdapter.notifyItemChanged(index);
                    }
                    return null;
                }

                @Override
                public Void visit(TransferCreatedEvent event) {
                    int index = transfers.indexOf(transfer);
                    if (index == -1) {
                        index = transfers.size();
                        transfers.add(index, transfer);
                        transferAdapter.notifyItemInserted(index);
                    }
                    return null;
                }

                @Override
                public Void visit(TransferDeletedEvent event) {
                    int index = transfers.indexOf(transfer);
                    if (index != -1) {
                        transfers.remove(index);
                        transferAdapter.notifyItemRemoved(index);
                    }
                    return null;
                }
            });
        });
    }

    private interface OnItemClickListener<T> {
        void onItemClick(T item);
    }

    private static class Adapter extends RecyclerView.Adapter<ViewHolder> {

        private List<Transfer> transfers;
        private OnItemClickListener<Transfer> listener;

        Adapter(List<Transfer> transfers, OnItemClickListener<Transfer> listener) {
            this.transfers = transfers;
            this.listener = listener;
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
                    .transform((c) -> DATE_FORMAT.format(new Date(c.getTimestamp() * 1000))).or("<pending>");

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
    }

    private static class ViewHolder extends RecyclerView.ViewHolder {

        public TextView dateView;
        public TextView amountView;
        public TextView addressView;
        public TextView feeView;
        public TextView stateView;

        public ViewHolder(@NonNull View view) {
            super(view);

            dateView = view.findViewById(R.id.item_date);
            amountView = view.findViewById(R.id.item_amount);
            addressView = view.findViewById(R.id.item_address);
            feeView = view.findViewById(R.id.item_fee);
            stateView = view.findViewById(R.id.item_state);
        }
    }
}
